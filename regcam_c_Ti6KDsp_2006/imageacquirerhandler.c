




//
//
//
//  Author :    Paul Calinawan
//
//  Date:       February 2, 2006
//
//  Copyrights: Imaging Technologies Inc.
//
//  Product:    Intelli-Ribbon Control
//  
//  Subsystem:  Camera System
//  -------------------------------------------
//
//
//      CONFIDENTIAL DOCUMENT
//
//      Property of Imaging Technologies Inc.
//
//



////////////////////////////////////////////////////////////////////

#include <vcrt.h>
#include <vclib.h>
#include <macros.h>
#include <sysvar.h>


////////////////////////////////////////////////////////////////////


#include "itechsys.h"


////////////////////////////////////////////////////////////////////


#include "kernel.h"


////////////////////////////////////////////////////////////////////


#include "imageacquirerhandler.h"
#include "camerapositionmanager.h"
#include "spicomm.h"
#include "cameraconfig.h"
#include "scannermanager.h"
#include "imageprocessinghandler.h"

#include "flashpowercontroller.h"

//
// IMPORTANT:
// 
//  Camera Triggering is allowed only if the following 2 conditions are met
//
//      1. STORING Time is elapsed 33 msec from the last trigger
//      2. The CAMERA_READY_FOR_CAPTURE line is not asserted
//

////////////////////////////////////////////////////////////////////


// Frame Buffer Size in Bytes

#define FRAME_BUFFER_SIZE       (VC_FULL_FOV_WIDTH * VC_FULL_FOV_HEIGHT)

#define TRG_SETUP_DELAY         (40000)  //5ms delay at 125ns per tick
////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////

// TODO MALLOC

int                     *frameAddr;
FRAME_BUFFER_IMAGE      frameBufferImage[MAX_NUM_FRAME_BUFFERS];
FRAME_BUFFER_IMAGE      imageTXBuffer;
FRAME_BUFFER_IMAGE      imageDiagBuffer;
FRAME_BUFFER_IMAGE      *newestImage;


FRAME_BUFFER_IMAGE      *nextFrameBufferImageEmpty;
FRAME_BUFFER_IMAGE      *nextFrameBufferImageReady;

FRAME_BUFFER_IMAGE      *lastFrameBufferImageIndex;
FRAME_BUFFER_IMAGE      *singleFrameBuffer;

int                     frameBufferAvailableCount;



extern POSITION  cameraCircSearchPositions[MAX_CAMERA_SEARCH_POSITIONS_CIRC];
extern BYTE    searchMode;

BYTE            frameCaptureMode;
BYTE            currentFrameCaptureMode;
CAMERA_POSITION currrentCamPosition;

DWORD           currentPeriod;
MULTICUT_VARS        multiCuts;
BOOL                restartGroup;

///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////

void    InitializeFrameBufferArray(void);
void    InitializeFrameBufferAttributes(void);
void    ClearFrameBufferAttributes(int frameBufferIndex);

int     AllocateFrameBuffers(void);

void    ReleaseFrameBuffers(void);


void    ResetImageAcquirerHandler(void);
void    ResetFrameBuffers(void);


void    DecrementFrameBufferAvailable(void);

void    IncrementFrameBufferAvailable(void);

BOOL    IsFrameBufferAvailable(void);  

void    ShowFrameBuffers(void);

extern      void StartImageCaptureTimeOut();

////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////

void    Init_ImageAcquirerHandler(void)
{
    ResetImageAcquirerHandler();

    lastFrameBufferImageIndex = 
    &frameBufferImage[MAX_NUM_SEARCH_BUFFERS - 1];
}

void    ResetCameraCaptureMode(void)
{
    cancel_capture_rq();

    VC_SET_SHUTTER(GET_SHUTTER_TIME_PARAMETER());
    VC_VIDEO_MODE_STILL();

    VC_SET_TRIGOUT_EXPOSURE_MODE();

}

void   setupNextFastFrame(void)
{
    DWORD currentEncIndex, currentAperiodCount;
    DWORD pulseWidth;
    DWORD targetLocation;
    BOOL  gotFirstLocation = 0;
    BOOL  firstLocationIndex;
    DWORD currentLocation,distanceTraveled;
    DWORD nextCircLocation;
    int index;
    double pulseSetUpDelay;

    GetSPIDataWithRetries(GET_CURRENT_ENCODER_PERIOD, &currentPeriod );
    GetSPIDataWithRetries( GET_ENCODER_INDEX  , &currentEncIndex );
    GetSPIDataWithRetries( GET_CURRENT_A_COUNT  , &currentAperiodCount );


    pulseWidth = (currentSystemConfiguration.encoder.cutOffLength/currentSystemConfiguration.encoder.aPulsesPerImpression) + 1;

    currentLocation     = currentEncIndex * pulseWidth;   //Location = encoder index * distancePerPulse
    pulseSetUpDelay     = (double)(currentAperiodCount + TRG_SETUP_DELAY);
    distanceTraveled    = ( (double)pulseWidth * pulseSetUpDelay )/currentPeriod;  //Where in the pulse is the system currently.

    targetLocation = currentLocation + distanceTraveled;  //Current Location of camera to paper

    for ( index = 0; index < 256; index++ )
    {
        if ( cameraCircSearchPositions[index].isTaken )
            continue;

        if ( !gotFirstLocation )  //Find the index of the next impression trigger location
        {
            firstLocationIndex = index;
            gotFirstLocation = TRUE;
        }

        if ( cameraCircSearchPositions[index].position >= targetLocation )
        {
            nextCircLocation = GetCircumPositionAt(index);
            setFPGAframeCaptureLocation(nextCircLocation);
            //print("\nFastNewCircTrigLoc[%d]: %d Tgt%d Serach Type:%d\n",index, nextCircLocation, targetLocation, searchMode);
            currrentCamPosition.circumPosition = nextCircLocation;

            return ;
        }
    }

    nextCircLocation = GetCircumPositionAt(firstLocationIndex);

    setFPGAframeCaptureLocation(nextCircLocation);
//    //print("\nNewCircTrigLoc[%d]: %d Tgt:%d Type:%d\n",pictureCounter++, nextCircLocation, targetLocation, searchMode);
    currrentCamPosition.circumPosition = nextCircLocation;
}

inline SetUpAllTriggerPOS( DWORD startingCount )
{
    BYTE x;

    for ( x = 1; x < multiCuts.numberOfCuts; x++ )
    {
        multiCuts.impressionTgt[x] = ( startingCount + x );
    }
}
BOOL ReadyForNextTrigger()
{
    DWORD currentEncIndex, currentAperiodCount;
    DWORD pulseWidth;
    DWORD currentPosition;
    DWORD currentImpressionCount,nextImpression;
    DWORD currentLocation,distanceDelay, currentLocationPlusDelay, pulseDistanceTraveled;
    double pulseSetUpDelay;
    BYTE x;

    BOOL triggerNow = FALSE;
    BOOL triggerNext = FALSE;

    GetSPIDataWithRetries(GET_CURRENT_ENCODER_PERIOD, &currentPeriod );
    GetSPIDataWithRetries( GET_ENCODER_INDEX  , &currentEncIndex );
    GetSPIDataWithRetries( GET_CURRENT_A_COUNT  , &currentAperiodCount );
    GetSPIDataWithRetries( GET_IMPRESSION_COUNT  , &currentImpressionCount );

    pulseWidth = (currentSystemConfiguration.encoder.cutOffLength/currentSystemConfiguration.encoder.aPulsesPerImpression) + 1;

    currentLocation     = currentEncIndex * pulseWidth;   //Location = encoder index * distancePerPulse

    pulseSetUpDelay    =  (TRG_SETUP_DELAY /currentPeriod);  //Where in the pulse is the system currently.

    pulseDistanceTraveled        = ( (double)pulseWidth * currentAperiodCount )/currentPeriod;  //Where in the pulse is the system currently.

    currentPosition = currentLocation + pulseDistanceTraveled ;  //Current Location of camera to paper

    currentLocationPlusDelay = currentPosition + pulseDistanceTraveled;

//     printf(" PW:%d Ac:%d Per:%d CL:%d CA:%d\n",
//            pulseWidth,
//            currentEncIndex,
//            currentPeriod,
//            currentLocation,
//            currentAperiodCount);

    // printf(" TgtPos:%d  CurPos%d Imp:%d\n", singleLocationCirc.position,currentLocationPlusDelay, currentImpressionCount);

    if ( currentLocationPlusDelay < singleLocationCirc.position )
        triggerNow = TRUE;
    else if ( currentPosition > singleLocationCirc.position )
        triggerNext = TRUE;
    else
        return FALSE;

//Set Up first Capture of Group
    if ( multiCuts.impressionTgt[ 0 ] == 0 )
    {
        if ( triggerNow )
            multiCuts.impressionTgt[ 0 ] = currentImpressionCount;
        else
            multiCuts.impressionTgt[ 0 ] = currentImpressionCount + 1;

        SetUpAllTriggerPOS( multiCuts.impressionTgt[ 0 ] );

//        printf(" Set up Zero: %d, %d \n", multiCuts.impressionTgt[ 0 ], currentImpressionCount);

        multiCuts.needCapture[ 0 ] = FALSE;

        restartGroup = TRUE;

        return TRUE;
    }


    nextImpression = currentImpressionCount + 1;

    restartGroup = FALSE;

    if (multiCuts.numberOfCuts > MAX_CUTS)
    {
        printf("TOO Many CUTS: %d\n", multiCuts.numberOfCuts);
    }

    for ( x = 1; x < multiCuts.numberOfCuts; x++ )
    {
        if ( multiCuts.needCapture[ x ] )
        {
            if ( triggerNow && ( multiCuts.impressionTgt[x] == currentImpressionCount) )
            {
                multiCuts.needCapture[ x ] = FALSE;
    //            printf(" Set up trigger 1 [%d]: %d %d  %d %d\n", x, multiCuts.impressionTgt[ x ], currentImpressionCount, triggerNow, triggerNext);
                return TRUE;
            }
            else if ( triggerNext && (multiCuts.impressionTgt[x] == nextImpression) )
            {
                multiCuts.needCapture[ x ] = FALSE;
   //             printf(" Set up trigger 2 [%d]: %d %d %d %d\n", x, multiCuts.impressionTgt[ x ], currentImpressionCount, triggerNow, triggerNext);
                return TRUE;
            }

            if ( currentImpressionCount > multiCuts.impressionTgt[x] )
                multiCuts.impressionTgt[x] += multiCuts.numberOfCuts;
        }
    }

    return FALSE;
}

void CheckImageCaptureFlags()
{
    BYTE x;

    //Check if you need continure capturing Images
    for ( x = 0; x < MAX_CUTS; x++ )
    {
        if ( multiCuts.needCapture[x] == TRUE )
            return;
    }

    for ( x = 0; x < MAX_CUTS; x++ )
    {
        if ( x < multiCuts.numberOfCuts )
            multiCuts.needCapture[x] = TRUE;
        else
            multiCuts.needCapture[x] = FALSE;
    }

    multiCuts.impressionTgt[0] = 0;

    return ;
}

void initMultiCuts()
{
	BYTE x;

    for (x = 0; x < MAX_CUTS; x++ )
    {
        multiCuts.impressionTgt[x] = 0L;
        multiCuts.needCapture[x]    = FALSE;
    }
}

BOOL   setupSingleFrame(void)
{

//     DWORD currentEncIndex, currentAperiodCount;
//     DWORD currentImpressionCount, currentPeriod;

    if ( multiCuts.numberOfCuts > 1 )
    {
        CheckImageCaptureFlags();

        if ( !ReadyForNextTrigger() )
        {
            return FALSE;
        }
    }

//     GetSPIDataWithRetries(GET_CURRENT_ENCODER_PERIOD, &currentPeriod );
//     GetSPIDataWithRetries( GET_ENCODER_INDEX  , &currentEncIndex );
//     GetSPIDataWithRetries( GET_CURRENT_A_COUNT  , &currentAperiodCount );
//     GetSPIDataWithRetries( GET_IMPRESSION_COUNT  , &currentImpressionCount );
//
  //  printf("SetUp Per:%d, Ind:%d , Ac:%d, Imp %d\n", currentPeriod,currentEncIndex,currentAperiodCount,currentImpressionCount);

    setFPGAframeCaptureLocation(singleLocationCirc.position);
    currrentCamPosition.circumPosition = singleLocationCirc.position;
    return TRUE;
}

////////////////////////////////////////////////////////////////////
//
// Exit Procedures
//
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//
// GoActive
//
////////////////////////////////////////////////////////////////////

NEW_STATE   IAH_exitA(void)
{
    int allocateResult;

    // One-Time MEMORY ALLOCATION

    InitializeFrameBufferArray();

    allocateResult = AllocateFrameBuffers();


    //   if ( allocateResult == ALLOCATION_FAILURE )
    //       print("ERROR: IAH ALLOCATION_FAILURE\n");
    //   else
    //       print("ALLOCATION_SUCCESSFUL %d frame buffers\n", MAX_NUM_FRAME_BUFFERS);

    ResetImageAcquirerHandler();
    ShowFrameBuffers();

    VC_TAKE_PICTURE_NOW();

    SendMessage(THIS_MACHINE, CheckForImageFrameTransferComplete);

    frameCaptureMode = FAST_FRAME_ANALYSIS;

    multiCuts.numberOfCuts = 1;
    initMultiCuts();

    return IAH_CHECK_TO_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// Timeout while in IAH_CHECK_TO_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   IAH_exitA1(void)
{
    if ( VC_IMAGE_IS_READY() )
    {
        // print("\nIAH A1 NOW ACTIVE");
        return IAH_ACTIVE;
    }

    SendMessage(THIS_MACHINE, CheckForImageFrameTransferComplete);

    return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
// DoStartImageAcquisition while in ACTIVE
//
////////////////////////////////////////////////////////////////////

// TODO

NEW_STATE   IAH_exitB(void)
{
    //print("IAH B FAST SCAN\n");

    InitializeFrameBufferAttributes();

    ResetImageAcquirerHandler();

    VC_TAKE_PICTURE_NOW();

    SendMessage(THIS_MACHINE, CheckForImageFrameTransferComplete);

    StartTimer(300);

    return IAH__WAIT_FOR_FIRST_IMAGE_TRANSFER_COMPLETE;
}

////////////////////////////////////////////////////////////////////
//
//  CheckForImageFrameTransferComplete 
//  while in IAH__WAIT_FOR_FIRST_IMAGE_TRANSFER_COMPLETE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   IAH_exitB1(void)
{
    //DWORD nextCircLocation;

    if ( VC_IMAGE_IS_READY() )
    {
        //print("IAH B1 FAST SCAN\n");
        SendMessage(ImageAcquirerHandler, DoStartImageAcquisition);

        return IAH_SETUP_NEXT_FRAME;
    }

    SendMessage(THIS_MACHINE, CheckForImageFrameTransferComplete);

    return  SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
// NewVcImageReady 
//  while in IAH_ACTIVE_WAIT_FOR_KERNEL_IMAGE_READY
//
////////////////////////////////////////////////////////////////////

NEW_STATE   IAH_exitC(void)
{
    int interruptEnable;
    //   void *ptr;
//    DWORD tempTranPosition1;
//    DWORD tempTranPosition;

    if ( frameCaptureMode == SINGLE_FRAME_ANALYSIS )
    {
        VC_SET_SCREEN_CAPTURE_PAGE(singleFrameBuffer->frameImage.st);
//        ptr = (void *)singleFrameBuffer->frameImage.st;
    }
    else
    {
        if ( !IsFrameBufferAvailable() )
        {
            //print("IAH C FRAME BUFFER FULL\n");

            StartTimer(500);
            return  IAH_ACTIVE_FRAME_BUFFER_FULL;
        }

        VC_SET_SCREEN_CAPTURE_PAGE(nextFrameBufferImageEmpty->frameImage.st);
//        ptr = (void *)nextFrameBufferImageEmpty->frameImage.st;
    }

    //  print("FRAME Capture Address 0x%p\n",ptr);

    interruptEnable = VC_ENABLE_PICTURE_INTERRUPT();

    //   if ( interruptEnable == -1 )
    //   {
    //       print("IAH Interrupt Enable -1.\n");
    //
    //   }

    if ( frameCaptureMode == SINGLE_FRAME_ANALYSIS )
    {
        currentFrameCaptureMode = SINGLE_FRAME_ANALYSIS;
        if ( !setupSingleFrame() )
        {
            SendMessage(THIS_MACHINE, DoStartImageAcquisition);

            return SAME_STATE;
        }
        singleFrameBuffer->circPosition = currrentCamPosition.circumPosition;
        // singleFrameBuffer->latPosition  = currrentCamPosition.lateralPosition;

        //       tempTranPosition = singleFrameBuffer->latPosition;
        SPI_GET_TRANSPORT_POSITION( &singleFrameBuffer->latPosition );
        MakeTransportPositionPositive(  &singleFrameBuffer->latPosition );
        singleFrameBuffer->flashPowerLevel  = (GetFlashPowerLevelControl()/FPL_SCALE);

//        print("IAH C Single Addr:%p, Lat: %d Circ:%d  \n", singleFrameBuffer
//              , singleFrameBuffer->latPosition
//              , singleFrameBuffer->circPosition);
    }
    else
    {
        multiCuts.impressionTgt[0] = 0;

        currentFrameCaptureMode = FAST_FRAME_ANALYSIS;

        if ( cameraSearchNumberOfCircPositions <= 0 )
        {
            SendMessage(ScannerManager, CircFrameCaptureDone);
            //           print("\n C CircLocationsDone\n");
            return IAH_SETUP_NEXT_FRAME;
        }

        setupNextFastFrame();
        nextFrameBufferImageEmpty->circPosition = currrentCamPosition.circumPosition;
        nextFrameBufferImageEmpty->latPosition  = currrentCamPosition.lateralPosition;
        nextFrameBufferImageEmpty->flashPowerLevel  = (GetFlashPowerLevelControl()/FPL_SCALE);


        //       print("IAH C Fast Addr:%p, Lat: %d Circ:%d  \n", nextFrameBufferImageEmpty
        //             ,nextFrameBufferImageEmpty->latPosition
        //             , nextFrameBufferImageEmpty->circPosition);
    }

    SendMessage(TriggerMonitor, RestartImageTracking);

    StartImageCaptureTimeOut();
    return IAH_WAIT_FOR_IMAGE_RDY;
}

NEW_STATE   IAH_exitC1(void)
{
    if ( !diagnosticsEnabled )
        SendMessage(ImageTransmitter, CheckForNewImage);

    if ( frameCaptureMode == SINGLE_FRAME_ANALYSIS )
    {
        singleFrameBuffer->flashCount = GetFlashCount(); //Update Before moving

        if ( !diagnosticsEnabled )
            newestImage = singleFrameBuffer;

        SendMessage(ScannerManager, ImageAcquirerReady);

        if ( currentFrameCaptureMode == SINGLE_FRAME_ANALYSIS )
            PostMessageAndData(ImageProcessingHandler,ProcessImage, SINGLE_FRAME_ANALYSIS, 0);
    }
    else
    {
        nextFrameBufferImageEmpty->flashCount = GetFlashCount();
        if ( !diagnosticsEnabled )
            newestImage = nextFrameBufferImageEmpty;

        //print("IAH C1 Flash Counter: %d\n",nextFrameBufferImageEmpty->flashCount);
        DecrementFrameBufferAvailable();

        if ( cameraSearchNumberOfCircPositions <= 0 )
        {
            SendMessage(ScannerManager, CircFrameCaptureDone);
            //           print("\n C CircLocationsDone\n");
            return IAH_SETUP_NEXT_FRAME;
        }

        if ( frameCaptureMode == FAST_FRAME_ANALYSIS )
            PostMessageAndData(ImageProcessingHandler,ProcessImage, FAST_FRAME_ANALYSIS, 0);
        SendMessage(THIS_MACHINE, DoStartImageAcquisition);
    }

    return IAH_SETUP_NEXT_FRAME;
}

////////////////////////////////////////////////////////////////////
//
//  ImageAcquirerGoWaitActive while in 
//              IAH_ACTIVE_WAIT_FOR_KERNEL_IMAGE_READY
//
////////////////////////////////////////////////////////////////////

NEW_STATE   IAH_exitD(void)
{
    //print("IAH BACK to ACTIVE\n");
    CancelTimer();

    return IAH_ACTIVE;
}

NEW_STATE   IAH_exitD1(void)
{
    frameCaptureMode  =  GetMessageData1();
    return SAME_STATE;
}

NEW_STATE   IAH_exitD2(void)
{
    SendMessage(ScannerManager, ImageAcquirerReady);
    return SAME_STATE;
}

NEW_STATE   IAH_exitD3(void)
{
    SendMessage(GetSmSourceId(), ImageAcquirerNotReady); 

    return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
// ImageTakenFromRingBuffer while in 
//              IAH_ACTIVE_WAIT_FOR_KERNEL_IMAGE_READY
//
////////////////////////////////////////////////////////////////////

NEW_STATE   IAH_exitE(void)
{
    //print("IAH_exitE\n");

    //Do Nothing
    if ( IsNewImageAvailable() && (frameCaptureMode == FAST_FRAME_ANALYSIS) )
        PostMessageAndData(ImageProcessingHandler,ProcessImage, FAST_FRAME_ANALYSIS, 0);

    return SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
// ImageTakenFromRingBuffer while in IAH_ACTIVE_FRAME_BUFFER_FULL
//
////////////////////////////////////////////////////////////////////

NEW_STATE   IAH_exitF(void)
{
    CancelTimer();
    SendMessage(THIS_MACHINE, DoStartImageAcquisition);

    return IAH_SETUP_NEXT_FRAME;
}

NEW_STATE   IAH_exitG(void)
{
    CancelTimer();
    ResetCameraCaptureMode();
    SendMessage(THIS_MACHINE, DoStartImageAcquisition);

    return IAH_SETUP_NEXT_FRAME;
}

NEW_STATE   IAH_exitT1(void)
{
    //print("IAH_exitT1.\n");
    return IAH_ACTIVE;
}

NEW_STATE   IAH_exitT2(void)
{
    //print("IAH_exitT2.\n");
    PostMessageAndData(ImageProcessingHandler,ProcessImage, FAST_FRAME_ANALYSIS, 0);

    return SAME_STATE;
}

// TODO
//
// StopImageFastscan
//

////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////

void    InitializeFrameBufferArray(void)
{
    int fb;

    VC_IMAGE    tempImage = 
    {0L, VC_FULL_FOV_WIDTH, VC_FULL_FOV_HEIGHT, VC_PITCH_A};

    for ( fb=0; fb<MAX_NUM_FRAME_BUFFERS; fb++ )
    {
        frameBufferImage[fb].frameImage = tempImage;

        frameBufferImage[fb].trackNumber = 0;
        frameBufferImage[fb].flashCount = 0;

        frameBufferImage[fb].whiteLevel = 0;
        frameBufferImage[fb].blackLevel = 0;
        frameBufferImage[fb].contrast = 0;
        frameBufferImage[fb].threshold = 0;

//        memset(frameBufferImage[fb].histogram, 
//               0, sizeof(frameBufferImage[fb].histogram));
    }
}

void    InitializeFrameBufferAttributes(void)
{
    // TODO duplicate?

    int fb;

    for ( fb=0; fb<MAX_NUM_FRAME_BUFFERS; fb++ )
    {

        frameBufferImage[fb].trackNumber = 0;
        frameBufferImage[fb].flashCount = 0;

        frameBufferImage[fb].whiteLevel = 0;
        frameBufferImage[fb].blackLevel = 0;
        frameBufferImage[fb].contrast = 0;
        frameBufferImage[fb].threshold = 0;

//        memset(frameBufferImage[fb].histogram, 
//               0, sizeof(frameBufferImage[fb].histogram));
    }

    newestImage->flashCount = 0;
}


void    ClearFrameBufferAttributes(int frameBufferIndex)
{
    int fb = frameBufferIndex;

    frameBufferImage[fb].trackNumber = 0;
    frameBufferImage[fb].flashCount = 0;

    frameBufferImage[fb].whiteLevel = 0;
    frameBufferImage[fb].blackLevel = 0;
    frameBufferImage[fb].contrast = 0;
    frameBufferImage[fb].threshold = 0;

//    memset(frameBufferImage[fb].histogram, 
//           0, sizeof(frameBufferImage[fb].histogram));
}

//
//

#define FIRST_FRAME_BUFFER      0

int AllocateFrameBuffers(void)
{
    int fb;
//    int startMem, endMem, diff;

    //frameBufferImage[FIRST_FRAME_BUFFER].frameImage.st =
    //    VC_GET_START_CAPTURE_PAGE();

    imageTXBuffer.frameImage.st = (int)DRAMScreenMalloc();

    if ( (int)imageTXBuffer.frameImage.st == 0 )
        return ALLOCATION_FAILURE;

    imageDiagBuffer.frameImage.st = (int)DRAMScreenMalloc();

    if ( (int)imageDiagBuffer.frameImage.st == 0 )
        return ALLOCATION_FAILURE;


    frameAddr =  (int *)sysmalloc(MAX_NUM_FRAME_BUFFERS, MDATA);

    if ( (int)frameAddr == 0 )
        return ALLOCATION_FAILURE;

    frameBufferImage[0].frameImage.st = VC_GET_START_CAPTURE_PAGE();

//    startMem = sysmemfree();

    for ( fb=1; fb<MAX_NUM_FRAME_BUFFERS; fb++ )
    {
        frameAddr[fb] =  (int)DRAMScreenMalloc();

        if ( frameAddr[fb] == 0 )
        {
            for ( fb-=1; fb > 0 ; fb-- )
                DRAMPgFree(frameAddr[fb]);

            sysfree( (void *)frameAddr);

            return ALLOCATION_FAILURE;
        }

        frameBufferImage[fb].frameImage.st = (frameAddr[fb] & 0xFFFFFC00) + 1024;

        if ( frameBufferImage[fb].frameImage.st == 0 )
            return ALLOCATION_FAILURE;
    }

//    endMem = sysmemfree();
//    diff = startMem-endMem;

    singleFrameBuffer = &frameBufferImage[ MAX_NUM_FRAME_BUFFERS-1 ];
//	imageTXBuffer     = &frameBufferImage[ MAX_NUM_FRAME_BUFFERS-2 ];
//	imageDiagBuffer   = &frameBufferImage[ MAX_NUM_FRAME_BUFFERS-3 ];

    return ALLOCATION_SUCCESSFUL;
}

//
//

void ReleaseFrameBuffers(void)
{
    int fb;

    //print("RELEASE FRAME BUFFERS");

    VC_SCREEN_PAGE_FREE(imageTXBuffer.frameImage.st);

    imageTXBuffer.frameImage.st = 0;

    VC_SCREEN_PAGE_FREE(imageDiagBuffer.frameImage.st);

    imageDiagBuffer.frameImage.st = 0;

    for ( fb=0; fb<MAX_NUM_FRAME_BUFFERS; fb++ )
    {
        if ( frameBufferImage[fb].frameImage.st != 0 )
        {
            VC_SCREEN_PAGE_FREE(frameBufferImage[fb].frameImage.st);

            frameBufferImage[fb].frameImage.st = 0;
        }
    }
}

//
//

void    ResetImageAcquirerHandler(void)
{
    ResetFrameBuffers();
    ResetCameraCaptureMode();
}

void    ResetFrameBuffers(void)
{
    frameBufferAvailableCount = MAX_NUM_SEARCH_BUFFERS;

    nextFrameBufferImageEmpty = frameBufferImage;
    nextFrameBufferImageReady = frameBufferImage;
}

//
//

void    DecrementFrameBufferAvailable(void)
{
    if ( frameBufferAvailableCount > 0 )
    {
        --frameBufferAvailableCount;

        if ( nextFrameBufferImageEmpty == lastFrameBufferImageIndex )
            nextFrameBufferImageEmpty = frameBufferImage;
        else
            ++nextFrameBufferImageEmpty;
    }
}

//
//

void    IncrementFrameBufferAvailable(void)
{

    if ( frameBufferAvailableCount < MAX_NUM_SEARCH_BUFFERS )
        ++frameBufferAvailableCount;

    if ( nextFrameBufferImageReady == lastFrameBufferImageIndex )
        nextFrameBufferImageReady = frameBufferImage;
    else
        ++nextFrameBufferImageReady;
    //   print("FramesAvailable:%d\n",frameBufferAvailableCount);
}

//
//

BOOL    IsFrameBufferAvailable(void)        
{
    return(frameBufferAvailableCount != 0) ? TRUE : FALSE;
}

BOOL    IsNewImageAvailable(void)        
{
    return(frameBufferAvailableCount != MAX_NUM_SEARCH_BUFFERS) ? TRUE : FALSE;
}


void ShowFrameBuffers(void)
{
    int fb;
    void    *ptr;

    //print("FRAME BUFFERS\n\n");

    //print("FRAME BUFFERS AVAILABLE %d\n\n", frameBufferAvailableCount);
    //print("FRAME BUFFER NEXT %p\n\n", nextFrameBufferImageReady);
    //print("FRAME BUFFERS EMPTY %p\n\n", nextFrameBufferImageEmpty);

    for ( fb=0; fb<MAX_NUM_FRAME_BUFFERS; fb++ )
    {
        if ( frameBufferImage[fb].frameImage.st != 0 )
        {
            ptr = (void *) frameBufferImage[fb].frameImage.st;

            //print("FRAME %d Address 0x%p\n", fb, ptr);
        }
    }
    //print("Single          FRAME Address 0x%p \n", singleFrameBuffer->frameImage.st );
    //print("imageTXBuffer   FRAME Address 0x%p \n", imageTXBuffer.frameImage.st );
    //print("imageDiagBuffer FRAME Address 0x%p \n", imageDiagBuffer.frameImage.st );
}




////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_IAH_IDLE)
EV_HANDLER(GoActive, IAH_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_IAH_CHECK_TO_ACTIVE)   
EV_HANDLER(CheckForImageFrameTransferComplete, IAH_exitA1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_IAH_ACTIVE)    
EV_HANDLER(DoStartImageAcquisition, IAH_exitB),
EV_HANDLER(SetUpFrameCaptureMode, IAH_exitD1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_IAH__WAIT_FOR_FIRST_IMAGE_TRANSFER_COMPLETE)
EV_HANDLER(CheckForImageFrameTransferComplete, IAH_exitB1),
EV_HANDLER(ImageAcquirerGoWaitActive, IAH_exitD),
EV_HANDLER(TimeOut, IAH_exitB)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_IAH_SETUP_NEXT_FRAME)
EV_HANDLER(DoStartImageAcquisition, IAH_exitC),
EV_HANDLER(ImageAcquirerGoWaitActive, IAH_exitD),
EV_HANDLER(ImageTakenFromRingBuffer, IAH_exitE),
EV_HANDLER(SetUpFrameCaptureMode, IAH_exitD1),
EV_HANDLER(AreYouReady,           IAH_exitD3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_IAH_WAIT_FOR_IMAGE_RDY)
EV_HANDLER(NewVcImageReady, IAH_exitC1),
EV_HANDLER(ImageAcquirerGoWaitActive, IAH_exitD),
EV_HANDLER(ImageTakenFromRingBuffer, IAH_exitE),
EV_HANDLER(SetUpFrameCaptureMode, IAH_exitD1),
EV_HANDLER(AreYouReady,           IAH_exitD2),
EV_HANDLER(TimeOut,           IAH_exitT1),
EV_HANDLER(DoStartImageAcquisition, IAH_exitG)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_IAH_ACTIVE_FRAME_BUFFER_FULL)
EV_HANDLER(ImageTakenFromRingBuffer, IAH_exitF),
EV_HANDLER(ImageAcquirerGoWaitActive, IAH_exitD),
EV_HANDLER(TimeOut, IAH_exitT2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_IAH_ERROR)
EV_HANDLER(NULL_MESSAGE_ID, NULL_RESPONSE)
STATE_TRANSITION_MATRIX_END;



// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(IAH_Main_Entry)
STATE(_IAH_IDLE)                                        ,           
STATE(_IAH_CHECK_TO_ACTIVE)                             ,
STATE(_IAH_ACTIVE)                                      ,
STATE(_IAH_ERROR)                                       ,
STATE(_IAH_SETUP_NEXT_FRAME)          ,
STATE(_IAH_ACTIVE_FRAME_BUFFER_FULL)                    ,
STATE(_IAH__WAIT_FOR_FIRST_IMAGE_TRANSFER_COMPLETE)     ,
STATE(_IAH_WAIT_FOR_IMAGE_RDY)
SM_RESPONSE_END


