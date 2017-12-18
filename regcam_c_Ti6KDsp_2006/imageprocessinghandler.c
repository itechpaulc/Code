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

#include "imageprocessinghandler.h"

#include "imageacquirerhandler.h"

#include "imaging.h"

#include "flashpowercontroller.h"

#include "scannermanager.h"

#include "camerapositionmanager.h"

#include "spicomm.h"

#include "tcpcomm.h"

#include "imagetransmitter.h"

#include "cameraconfig.h"


////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////

#define     MAX_IMG_FILTERS (10)
#define     BACKOFF_STEPS   (2)
typedef    enum    
{
    NONE            =   0x00,
    LINE_FILTER     =   0x01,
    ERODE           =   0x02,
    DILATE          =   0x03
} IMAGE_FILTERING;

IMAGE_FILTERING imageFilteringSequence[MAX_IMG_FILTERS];



image   aOiFullImage;
DWORD   processImageMode;

RLC_IMAGE_COPY rlcImageCopy;

// TODO
// Send Message to Manager 

FRAME_BUFFER_IMAGE *fbiPtr;

AOI     aoiQuadrants[ MAX_RIBBON_QUADRANTS ];   
AOI     startingAOI;
AOI     curAOI ;

BYTE    currentQuadrant;        


extern NEW_STATE            smStates[MAX_STATE_MACHINES_COUNT];  //PM Remove will be used only for testing
extern BYTE                 currentFrameCaptureMode;
extern FRAME_BUFFER_IMAGE   imageDiagBuffer;

extern DWORD tempVariable;
extern BOOL                restartGroup;

BOOL    diagnosticsEnabled;

int maxElementArea;

///////////////////////////////////////////////////////////////////
//
//
//
///////////////////////////////////////////////////////////////////
void SetStartingAOI( int x, int y )
{
    int aoiX = 0 ;
    int aoiY = 0 ;
    int halfAOI;

    if ( GetDeviceType() == DEV_TYPE_REGISTER )
    {

        if ( currentSystemConfiguration.cameraImaging.rotateImage )
        {
            aoiX = x;
            aoiY = y;
            startingAOI.areaX           = 320;
            startingAOI.areaY           = 240;
        }
        else
        {
            aoiX =  y;
            aoiY = -x;
            startingAOI.areaX           = 240;
            startingAOI.areaY           = 320;
        }
    }
    else
    {
        startingAOI.areaX           = 320;
        startingAOI.areaY           = 240;

        aoiX -= y;
        aoiY = x;
    }

    aoiX = CENTER_OF_FOV_X + aoiX;

    aoiX /= SUB_PIXEL_SCALE;
    halfAOI = ( startingAOI.areaX >> 1 );
    aoiX -= halfAOI; //Starting point is 50% of area

    //Make we don't over run frame
    if ( aoiX < 0 )
        aoiX = 0;
    else if ( (aoiX + startingAOI.areaX ) > 639 )
        aoiX = ( 639 - startingAOI.areaX ) ;

    aoiY = CENTER_OF_FOV_Y - aoiY;

    aoiY /= SUB_PIXEL_SCALE;
    halfAOI = ( startingAOI.areaY >> 1 );
    aoiY -= halfAOI; //Starting point is 50% of area

    //Make we don't over run frame
    if ( aoiY < 0 )
        aoiY = 0;
    else if ( (aoiY + startingAOI.areaY ) > 479 )
        aoiY = ( 479 - startingAOI.areaY ) ;

    startingAOI.startX          = aoiX;
    startingAOI.startY          = aoiY;

    startingAOI.enable          = TRUE;
    startingAOI.needToProcess   = TRUE;

    //print("FovX: %d, FovY: %d - AOIx: %d, AOIy: %d\n",x,y,startingAOI.startX,startingAOI.startY);
}

void ClearStartingAOI( void )
{
    startingAOI.areaX           = 0;
    startingAOI.areaY           = 0;
    startingAOI.enable          = FALSE;
    startingAOI.needToProcess   = FALSE;
}

BYTE    GetCurrentThreshold(void)
{
    return fbiPtr->threshold;
}


///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////

RECT GetWhiteSpaceRect( )
{
    POINT           tempTopPoint, tempBotPoint, topPoint, botPoint;

    RECT            markWhiteSpaceRect;

    BYTE markNumber;

    BOOL initWhiteSpaceRectDone = FALSE;

    for ( markNumber = 0 ; markNumber < MAX_MARKS; markNumber++ )
    {
        if ( IsRegMarkFound(markNumber) )
        {
            tempTopPoint = GetTopPointTwoObjects(&markRecognitionData[ markNumber ].firstObject.features,
                                                 &markRecognitionData[ markNumber ].lastObject.features);

            tempBotPoint = GetBotPointTwoObjects(&markRecognitionData[ markNumber ].firstObject.features,
                                                 &markRecognitionData[ markNumber ].lastObject.features);
            if ( initWhiteSpaceRectDone )
            {
                topPoint.x = MIN(topPoint.x, tempTopPoint.x);
                topPoint.y = MIN(topPoint.y, tempTopPoint.y);

                botPoint.x = MAX(botPoint.x, tempBotPoint.x);
                botPoint.y = MAX(botPoint.y, tempBotPoint.y);
            }
            else
            {
                topPoint = tempTopPoint;
                botPoint = tempBotPoint;

                initWhiteSpaceRectDone = TRUE;
            }
        }
    }

    markWhiteSpaceRect = GenerateRectFromPoints(topPoint, botPoint);

    markWhiteSpaceRect.top.x -= REGISTER_NOMINAL_WHITE_SPACE_MARGIN;
    markWhiteSpaceRect.top.y -= REGISTER_NOMINAL_WHITE_SPACE_MARGIN;

    markWhiteSpaceRect.bottom.x += REGISTER_NOMINAL_WHITE_SPACE_MARGIN;
    markWhiteSpaceRect.bottom.y += REGISTER_NOMINAL_WHITE_SPACE_MARGIN;

    return markWhiteSpaceRect;
}

void SetLabelObjects( BOOL setObjectLabel )
{
    diagnosticsEnabled = setObjectLabel;
}

BOOL GetLabelObjects( void )
{
    return diagnosticsEnabled;
}

void   CameraControlMessage(void)
{
    CAMERA_CONTROL_STATUS       camControlStatus;
    ITECH_TCP_MESSAGE           cameraControlStatusMessage;
    MARK_RECOGNITION_DATA       *markRecogData;

    int     checkPost;
    int     totalTXBytes;
    BYTE    x;

    CameraImaging   *camImagingPtr  = 
    &GetCurrSystemConfig()->cameraImaging;

    // Mark Status

    camControlStatus.flashCount =fbiPtr->flashCount;

    camControlStatus.camSearchMode = GetCurrentCameraMode();

    GetSPIDataWithRetries(GET_CURRENT_ENCODER_PERIOD, &currentPeriod );
    camControlStatus.encoderTicksPerAPulse = currentPeriod;
    GetSPIDataWithRetries(GET_IMPRESSION_COUNT, &camControlStatus.encoderZCountImpression);

    camControlStatus.encoderACountIndexTrig  = fbiPtr->encIndex;
    camControlStatus.encoderFractionalTrig   = fbiPtr->fractional;
    camControlStatus.transportPositionTrig   = fbiPtr->latPosition;


    camControlStatus.flashLevelControl   = fbiPtr->flashPowerLevel;
    camControlStatus.blackLevel          = fbiPtr->blackLevel;
    camControlStatus.whiteLevel          = fbiPtr->whiteLevel;
    camControlStatus.contrast            = fbiPtr->contrast;

    for ( x = 0,camControlStatus.numberOfmarks = 0; x < MAX_MARKS; x++ )
    {
        if ( camImagingPtr->markDefinition[x].enable )
        {
            markRecogData = GetMarkRecognitionData( x );

            camControlStatus.markStatus[camControlStatus.numberOfmarks].id               = x;
            camControlStatus.markStatus[camControlStatus.numberOfmarks].threshold        = markRecogData->threshold;

            if ( IsRegReferenceMarkFound() && (camControlStatus.camSearchMode == CAMERA_LOCKED_ON) )
                camControlStatus.markStatus[camControlStatus.numberOfmarks].markFoundStatus  = markRecogData->markSearchStatus;
            else
                camControlStatus.markStatus[camControlStatus.numberOfmarks].markFoundStatus  = MARK_NOT_FOUND;

            camControlStatus.markStatus[camControlStatus.numberOfmarks].markFovLateral   = markRecogData->markFovLateral;
            camControlStatus.markStatus[camControlStatus.numberOfmarks].markFovCircum    = markRecogData->markFovCircum;
            camControlStatus.numberOfmarks++;
        }
    }

    totalTXBytes     =  sizeof(CAMERA_CONTROL_STATUS);
    totalTXBytes    -=  (sizeof(CAM_MARK_STATUS)) * MAX_MARKS;
    totalTXBytes    +=  sizeof(CAM_MARK_STATUS) * camControlStatus.numberOfmarks;

    InitBuildTcpPacket(&cameraControlStatusMessage.header);
    SetTcpMessageId(&cameraControlStatusMessage.header, ITECH_MSG_CamControlStatus);
    SetTcpData(&cameraControlStatusMessage, (BYTE *)&camControlStatus, totalTXBytes);

    SetTcpParam1( &cameraControlStatusMessage.header, restartGroup );

    checkPost = PostServerItechMessage(&cameraControlStatusMessage);

//     if ( checkPost == TRUE )
//         print("POST OK CAMERA_CONTROL_STATUS\n");
//     else
//         print("POST FAIL CAMERA_CONTROL_STATUS\n");

    return ;
}


int    GetWhiteLevel(U32 histogram[])
{
    int     whiteLvl;
    DWORD   whiteLevelPixelCount = 0;

    for ( whiteLvl=MAX_HISTOGRAM_POINTS-1; whiteLvl > 0; whiteLvl-- )
    {
        whiteLevelPixelCount += (histogram[whiteLvl]);

        // To do based on white space
        if ( whiteLevelPixelCount > 3000 )
            break;
    }

    return  whiteLvl;
}

int    GetBlackLevel(U32 histogram[])
{
    int     blackLvl;
    DWORD   blackLevelPixelCount = 0;
    BYTE    referenceMark = GetReferenceMarkIndex();
    WORD    pixelCountTarget = 0;
    BYTE    numberOfPairs, x;
    BYTE    elementIndex;
    CameraImaging   *camImagingPtr  = &GetCurrSystemConfig()->cameraImaging;

    numberOfPairs = camImagingPtr->markDefinition[ referenceMark].numPairsToSearch;


    for ( x = 0; x < numberOfPairs; x++ )
    {
        if ( x == 0 )
        {
            elementIndex = camImagingPtr->markDefinition[ referenceMark].elementPairs[x].thisElementType;
            pixelCountTarget += camImagingPtr->elements[ elementIndex ].minAreaSize;

            elementIndex = camImagingPtr->markDefinition[ referenceMark].elementPairs[x].partnerElementType;
            pixelCountTarget += camImagingPtr->elements[ elementIndex ].minAreaSize;
        }
        else
        {
            elementIndex = camImagingPtr->markDefinition[ referenceMark].elementPairs[x].partnerElementType;
            pixelCountTarget += camImagingPtr->elements[ elementIndex ].minAreaSize;
        }
    }

    for ( blackLvl=0; blackLvl < MAX_HISTOGRAM_POINTS; blackLvl++ )
    {
        blackLevelPixelCount += (histogram[blackLvl]);

        // To do based on target element min sizes
        if ( blackLevelPixelCount > pixelCountTarget )
            //  if ( blackLevelPixelCount > 150 )
            break;
    }

    return blackLvl;
}

void    GetImageAttributes(FRAME_BUFFER_IMAGE *currframeBuffImage, VC_IMAGE *imageToAnalyze)
{
    int     blackLvl, whiteLvl, contr, thresh, fullRange, steppingThresh, startingThresh, 
    fpl, finalThresh, stepRange;   

    U32     histogram[MAX_HISTOGRAM_POINTS];

    VC_GET_IMAGE_HISTOGRAM(imageToAnalyze, histogram); 

    whiteLvl = currframeBuffImage->whiteLevel = GetWhiteLevel(histogram);
    blackLvl = currframeBuffImage->blackLevel = GetBlackLevel(histogram);

    if ( GetCurrentCameraMode() == CAMERA_SEARCHING )
    {
        SET_MAX_STEPPING_RETRIES_CURR(GET_MAX_STEPPING_RETRIES_SEARCHING() );
    }
    else
    {
        SET_MAX_STEPPING_RETRIES_CURR(GET_MAX_STEPPING_RETRIES_LOCKED_ON() );
    }

    fullRange = whiteLvl - blackLvl;

    stepRange = (fullRange >> 1);

    finalThresh = blackLvl + stepRange;

    if ( (blackLvl < 64) ||  ( GetDeviceType() == DEV_TYPE_REGISTER ) )
    //if ( (blackLvl < 64) ))
    {
        currframeBuffImage->startingThreshold   = 
        startingThresh  = blackLvl + (stepRange >> 1);

        currframeBuffImage->steppingThreshold   = 
        steppingThresh  = (finalThresh - startingThresh) / GET_MAX_STEPPING_RETRIES_CURR();
    }
    else
    {
        currframeBuffImage->steppingThreshold   = 
        steppingThresh  = (finalThresh - blackLvl) /  GET_MAX_STEPPING_RETRIES_CURR();

        currframeBuffImage->startingThreshold   = 
        startingThresh  = blackLvl -  (steppingThresh * BACKOFF_STEPS );
    }

    thresh = 
    currframeBuffImage->threshold = startingThresh;


    contr =
    currframeBuffImage->contrast = ((fullRange * 100)) >> 8; // Divide by MAX_WHITE_LEVEL

    fpl = (GetFlashPowerLevelControl()/FPL_SCALE);

//    print("Fpl:%d Blk:%d  Wht:%d  Thr:%d  Cntr:%d StpThr:%d StrtThr:%d FinalThr:%d\n", 
//          fpl, blackLvl, whiteLvl, thresh, contr, steppingThresh, startingThresh, finalThresh);
//
//    print("Threshold Retries: %d\n", currframeBuffImage->steppingRetries);
}

void    ResetMultiThresholding(FRAME_BUFFER_IMAGE *currframeBuffImage)
{
    currframeBuffImage->steppingRetries = 0;
}

void    IncrementMultiThresholding(FRAME_BUFFER_IMAGE *currframeBuffImage)
{
    ++currframeBuffImage->steppingRetries;

    currframeBuffImage->threshold += 
    currframeBuffImage->steppingThreshold;
    //(currframeBuffImage->steppingThreshold * (GET_INCREMENT_THRESHOLD_MULTIPLIER()));

    //print("ThrRetries:%d ThrNew:%d\n", currframeBuffImage->steppingRetries, currframeBuffImage->threshold);

    if ( currframeBuffImage->threshold > MAX_WHITE_LEVEL )
    {
        currframeBuffImage->threshold = MAX_WHITE_LEVEL;
        //print("Thresh Hold too large\n", currframeBuffImage->steppingThreshold);

    }
}

//void  ResetMarkRecognitionPerformance()


void SendImageStatusItems( MARK_STATUS markStatus )
{
    if ( processImageMode == FAST_FRAME_ANALYSIS )
        IncrementFrameBufferAvailable();

    if ( diagnosticsEnabled )
    {
        imageDiagBuffer.flashCount = fbiPtr->flashCount;
        SendMessage(ImageTransmitter, CheckForNewImage);
    }

    SendMessage(ImageAcquirerHandler, ImageTakenFromRingBuffer);

    SendMessageAndData(ScannerManager, ImageProcessed, markStatus, 0);

    CameraControlMessage();

}

///////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////

#define     MIN_CONTRAST        (25)

#define     MAX_BLACK_LEVEL     (145)

#define     MIN_WHITE_LEVEL     (GREY_50)

BOOL    ImageAttributesOkForAnalysis(FRAME_BUFFER_IMAGE *currframeBuffImage)
{
    if ( (currframeBuffImage->contrast > MIN_CONTRAST) &&
         (currframeBuffImage->whiteLevel > MIN_WHITE_LEVEL) &&
         (currframeBuffImage->blackLevel < MAX_BLACK_LEVEL) )
        return TRUE;

    return FALSE ;
}

////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

#define ERODE_COUNT             (2)

#define DILATE_COUNT            (2)

#ifdef JENCOAT_RIBBON_MARK
#define LINE_FILTER_COUNT       (6)
#endif

#ifdef STANDARD_RIBBON_MARK
#define LINE_FILTER_COUNT       (3)
#endif



BOOL    DetectObjects(FRAME_BUFFER_IMAGE *currframeBuffImage, VC_IMAGE *imageToAnalyze)
{
    // TODO Malloc this
    VC_FEATURES     objectfeatures[MAX_RAW_OBJECTS];

    POINT           objectCenter;

    U32    maxRlcLength    = 0x030000;

    U16 *runlengthCodeAddr1;
    U16 *runlengthCodeAddr2;

    U16 *addr1;
    U16 *addr2;
    U16 *addr3;

    U16 *segmentCodeAddr;

    char x;

    int objectCount, objNo;

    int circleCount, ovalCount;

    int     thresh = currframeBuffImage->threshold;

    // allocate DRAM Memory 

    //runlengthCodeAddr1 = VC_MALLOC_DRAM_WORD(maxRlcLength+16);
    //   runlengthCodeAddr1 = (U16 *) sysmalloc((maxRlcLength* sizeof(U16)+1) / sizeof(U32), MDATA);
    runlengthCodeAddr1 = rlcmalloc(maxRlcLength);

    if ( runlengthCodeAddr1 == NULL )
    {
        //print("DETECT OBJECTS: DRAM Memory overrun\n");
        sysfree(runlengthCodeAddr1);
        return FALSE;
    }

    // Create the Run Length Code

    runlengthCodeAddr2 = 
    VC_CREATE_RLC(imageToAnalyze, thresh, runlengthCodeAddr1, maxRlcLength);

    if ( runlengthCodeAddr2 == 0 )
    {
        //LogString("DETECT OBJECTS: RLC overrun"); 
        //VC_FREE_DRAM_WORD(runlengthCodeAddr1); 
        sysfree(runlengthCodeAddr1); 
        return FALSE;
    }

    // FIlter on or off
    // TODO filter here erode, dilate, rlc_mf etc
    // Line filter is a config
    // Also erode xTimes adn Dilate xTimes

    if ( TRUE )
    {
        addr1 = runlengthCodeAddr1;
        addr2 = runlengthCodeAddr2;

        for ( x = 0; x < MAX_IMG_FILTERS; x++ )
        {
            switch ( imageFilteringSequence[x] )
            {
                case LINE_FILTER:
                    addr3   =   rlc_mf(addr1, addr2, BLACK, LINE_FILTER_COUNT);
                    break;

                case ERODE:
                    addr3   =   erode(addr1, addr2);
                    break;

                case DILATE:
                    addr3   =   dilate(addr1, addr2);
                    break;

                case NONE:
                default:
                    x   =   MAX_IMG_FILTERS; //DONE FILTERING
            }

            if ( x < MAX_IMG_FILTERS )
            {
                addr1 = addr2;

                if ( x == 1 || x == 4 || x == 7 )
                    addr2 = runlengthCodeAddr1;
                else
                    addr2 = addr3;
            }
        }

        segmentCodeAddr     = addr2;
        runlengthCodeAddr2  = addr1;
    }
    else
    {
        segmentCodeAddr = runlengthCodeAddr2;
        runlengthCodeAddr2 = runlengthCodeAddr1;
    }

    if ( ( currentQuadrant == 0 ) && ( GetRequestedScannerTransmitType() == QUARTER_RLC ) )
    {
        rlcImageCopy.latestFlashCount = fbiPtr->flashCount;
        memcpy((void *)&rlcImageCopy.rlcCopy[0],(void*) runlengthCodeAddr2, RLC_COPY_SIZE);
        rlcImageCopy.dx  = curAOI.areaX;
        rlcImageCopy.dy  = curAOI.areaY;
    }

    // objectCount = 
    //     VC_CREATE_SEGMENTATION(runlengthCodeAddr2, segmentCodeAddr);

    //    if(objectCount == 0L)
    if ( VC_CREATE_SEGMENTATION(runlengthCodeAddr2, segmentCodeAddr) == 0L )
    {
        //LogString("DETECT OBJECTS: Object number overrun\n"); 
        //  VC_FREE_DRAM_WORD(runlengthCodeAddr1);
        sysfree(runlengthCodeAddr1);
        return FALSE;
    }


    objectCount = 
    VC_EXTRACT_FEATURES(runlengthCodeAddr2, objectfeatures, MAX_RAW_OBJECTS);

    //print("OBJECT COUNT %d\n", objectCount);

    if ( objectCount == VC_TOO_MANY_OBJECTS )
    {
        sysfree(runlengthCodeAddr1);
        return  FALSE;
    }
    //if (TRUE) 
    //    VC_OUTPUT_RLC_TO_IMAGE(imageToAnalyze, runlengthCodeAddr1, BLACK, WHITE);


    // TODO make the image processing stages into states

    if ( diagnosticsEnabled )
    {
        for ( objNo=0; objNo<objectCount; objNo++ )
        {
            if ( objectfeatures[objNo].color == BLACK )
            {
                objectCenter.x = objectfeatures[objNo].x_center + curAOI.startX;
                objectCenter.y = objectfeatures[objNo].y_center + curAOI.startY;

                //             DrawCrossHair(&imageDiagBuffer.frameImage, objectCenter);
            }
        }
    }
    // Mark Recognition

    ExtractValidObjects(objectfeatures, objectCount);

    sysfree(runlengthCodeAddr1);

    circleCount = GetObjectCount( 0 );
    ovalCount   = GetObjectCount( 1 );

    // LogString(logArray);

	if ( GetDeviceType() == DEV_TYPE_REGISTER )
	{
	    if ( MIN_CIRCLE_COUNT > circleCount )
	    {
	        print("REG Not Enough Objects - Circle:%d \n",circleCount);
	        return FALSE;
	    }
	}
	else
	{
	    if ( (MIN_OVAL_COUNT > ovalCount) || (MIN_CIRCLE_COUNT > circleCount) )
	    {
	        print("RIB Not Enough Objects - Circle:%d Oval:%d \n",circleCount, ovalCount);
	        return FALSE;
	    }
	}

    return TRUE;
}

////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////
void    ReenableAOIForProcessing()
{
    char x;

    for ( x = 0; x < MAX_RIBBON_QUADRANTS; x++ )
    {
        if ( aoiQuadrants[x].enable )
            aoiQuadrants[x].needToProcess = TRUE;
    }
}
void    Init_RibbonImageProcessingHandler(void)
{
    aoiQuadrants[0].enable          = TRUE;
    aoiQuadrants[0].needToProcess   = TRUE;
    aoiQuadrants[0].startX          = 160;
    aoiQuadrants[0].startY          = 120;
    aoiQuadrants[0].areaX           = 320;
    aoiQuadrants[0].areaY           = 240;

    aoiQuadrants[1].enable          = TRUE;
    aoiQuadrants[1].needToProcess   = TRUE;
    aoiQuadrants[1].startX          = 0;
    aoiQuadrants[1].startY          = 120;
    aoiQuadrants[1].areaX           = 195;
    aoiQuadrants[1].areaY           = 240;

    aoiQuadrants[2].enable          = TRUE;
    aoiQuadrants[2].needToProcess   = TRUE;
    aoiQuadrants[2].startX          = 445;
    aoiQuadrants[2].startY          = 120;
    aoiQuadrants[2].areaX           = 194;
    aoiQuadrants[2].areaY           = 240;

    aoiQuadrants[3].enable          = TRUE;
    aoiQuadrants[3].needToProcess   = TRUE;
    aoiQuadrants[3].startX          = 160;
    aoiQuadrants[3].startY          = 222;
    aoiQuadrants[3].areaX           = 320;
    aoiQuadrants[3].areaY           = 257;

    aoiQuadrants[4].enable          = TRUE;
    aoiQuadrants[4].needToProcess   = TRUE;
    aoiQuadrants[4].startX          = 160;
    aoiQuadrants[4].startY          = 0;
    aoiQuadrants[4].areaX           = 320;
    aoiQuadrants[4].areaY           = 258;

    aoiQuadrants[5].enable          = TRUE;
    aoiQuadrants[5].needToProcess   = TRUE;
    aoiQuadrants[5].startX          = 0;
    aoiQuadrants[5].startY          = 222;
    aoiQuadrants[5].areaX           = 195;
    aoiQuadrants[5].areaY           = 257;

    aoiQuadrants[6].enable          = TRUE;
    aoiQuadrants[6].needToProcess   = TRUE;
    aoiQuadrants[6].startX          = 0;
    aoiQuadrants[6].startY          = 0;
    aoiQuadrants[6].areaX           = 195;
    aoiQuadrants[6].areaY           = 257;

    aoiQuadrants[7].enable          = TRUE;
    aoiQuadrants[7].needToProcess   = TRUE;
    aoiQuadrants[7].startX          = 445;
    aoiQuadrants[7].startY          = 222;
    aoiQuadrants[7].areaX           = 194;
    aoiQuadrants[7].areaY           = 257;

    aoiQuadrants[8].enable          = TRUE;
    aoiQuadrants[8].needToProcess   = TRUE;
    aoiQuadrants[8].startX          = 445;
    aoiQuadrants[8].startY          = 0;
    aoiQuadrants[8].areaX           = 194;
    aoiQuadrants[8].areaY           = 258;


#ifdef STANDARD_RIBBON_MARK
    imageFilteringSequence[0] = DILATE;
    imageFilteringSequence[1] = DILATE;
    imageFilteringSequence[2] = ERODE;
    imageFilteringSequence[3] = ERODE; 
    imageFilteringSequence[4] = NONE; 
    imageFilteringSequence[5] = NONE;
    imageFilteringSequence[6] = NONE;
    imageFilteringSequence[7] = NONE;
    imageFilteringSequence[8] = NONE;
    imageFilteringSequence[9] = NONE;
#endif

#ifdef JENCOAT_RIBBON_MARK
	imageFilteringSequence[0] = LINE_FILTER;
    imageFilteringSequence[1] = ERODE;
    imageFilteringSequence[2] = ERODE;
    imageFilteringSequence[3] = ERODE; 
    imageFilteringSequence[4] = DILATE; 
    imageFilteringSequence[5] = DILATE;
    imageFilteringSequence[6] = DILATE;
    imageFilteringSequence[7] = NONE;
    imageFilteringSequence[8] = NONE;
    imageFilteringSequence[9] = NONE;
#endif

}

void    Init_RegImageProcessingHandler(void)
{
    aoiQuadrants[0].enable          = TRUE;
    aoiQuadrants[0].needToProcess   = TRUE;
    aoiQuadrants[0].startX          = 160;
    aoiQuadrants[0].startY          = 0;
    aoiQuadrants[0].areaX           = 320;
    aoiQuadrants[0].areaY           = 480;

    aoiQuadrants[1].enable          = TRUE;
    aoiQuadrants[1].needToProcess   = TRUE;
    aoiQuadrants[1].startX          = 0;
    aoiQuadrants[1].startY          = 0;
    aoiQuadrants[1].areaX           = 320;
    aoiQuadrants[1].areaY           = 480;

    aoiQuadrants[2].enable          = TRUE;
    aoiQuadrants[2].needToProcess   = TRUE;
    aoiQuadrants[2].startX          = 319;
    aoiQuadrants[2].startY          = 0;
    aoiQuadrants[2].areaX           = 320;
    aoiQuadrants[2].areaY           = 480;

    aoiQuadrants[3].enable          = FALSE;
    aoiQuadrants[3].needToProcess   = FALSE;
    aoiQuadrants[3].startX          = 160;
    aoiQuadrants[3].startY          = 222;
    aoiQuadrants[3].areaX           = 320;
    aoiQuadrants[3].areaY           = 257;

    aoiQuadrants[4].enable          = FALSE;
    aoiQuadrants[4].needToProcess   = FALSE;
    aoiQuadrants[4].startX          = 160;
    aoiQuadrants[4].startY          = 0;
    aoiQuadrants[4].areaX           = 320;
    aoiQuadrants[4].areaY           = 258;

    aoiQuadrants[5].enable          = FALSE;
    aoiQuadrants[5].needToProcess   = FALSE;
    aoiQuadrants[5].startX          = 0;
    aoiQuadrants[5].startY          = 222;
    aoiQuadrants[5].areaX           = 195;
    aoiQuadrants[5].areaY           = 257;

    aoiQuadrants[6].enable          = FALSE;
    aoiQuadrants[6].needToProcess   = FALSE;
    aoiQuadrants[6].startX          = 0;
    aoiQuadrants[6].startY          = 0;
    aoiQuadrants[6].areaX           = 195;
    aoiQuadrants[6].areaY           = 257;

    aoiQuadrants[7].enable          = FALSE;
    aoiQuadrants[7].needToProcess   = FALSE;
    aoiQuadrants[7].startX          = 445;
    aoiQuadrants[7].startY          = 222;
    aoiQuadrants[7].areaX           = 194;
    aoiQuadrants[7].areaY           = 257;

    aoiQuadrants[8].enable          = FALSE;
    aoiQuadrants[8].needToProcess   = FALSE;
    aoiQuadrants[8].startX          = 445;
    aoiQuadrants[8].startY          = 0;
    aoiQuadrants[8].areaX           = 194;
    aoiQuadrants[8].areaY           = 258;

    ClearStartingAOI( );

    imageFilteringSequence[0] = DILATE;
    imageFilteringSequence[1] = DILATE;
    imageFilteringSequence[2] = DILATE;
    imageFilteringSequence[3] = ERODE; 
    imageFilteringSequence[4] = ERODE;
    imageFilteringSequence[5] = NONE;
    imageFilteringSequence[6] = NONE;
    imageFilteringSequence[7] = NONE;
    imageFilteringSequence[8] = NONE;
    imageFilteringSequence[9] = NONE;
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

NEW_STATE   IPH_exitA(void)
{
    //print("IPH ACTIVE\n");

    diagnosticsEnabled = FALSE;

    SetLabelObjects( FALSE );

    return IPH_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// DoStartFastVisualScanRaw while in ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   IPH_exitB(void)
{
//    void *ptr;
    processImageMode   =  GetMessageData1();

    // TODO
    // Send Message to Manager 
    //print("IPH  B\n");

    if ( processImageMode == SINGLE_FRAME_ANALYSIS )
    {
        fbiPtr = GetSingleFrame();
        //print("IPH  B SingleImage[%p] - Lat:%d Circ:%d\n",fbiPtr, fbiPtr->latPosition, fbiPtr->circPosition);
    }
    else
    {
        if ( !IsNewImageAvailable() )
        {
            //print("IPH  B Image not Available\n");
            return SAME_STATE;
        }

        fbiPtr = GetNextFrameBufferImageReady();
        //print("IPH  B FastFrameBuff[%p]: Lat:%d Circ:%d\n",fbiPtr, fbiPtr->latPosition, fbiPtr->circPosition);
    }

    if ( diagnosticsEnabled )
    {
        //  imageDiagBuffer->flashCount = fbiPtr->flashCount;
        newestImage = &imageDiagBuffer;

        memcpy((void *)imageDiagBuffer.frameImage.st,(void *)fbiPtr->frameImage.st,((SizeOfScreen)+1024+BytesPerWord-1));
        VC_COPY_IMAGE(fbiPtr->frameImage, imageDiagBuffer.frameImage, 1, 1);

    }

    maxElementArea = 0;

    VC_SET_LOGICAL_CAPTURE_PAGE(fbiPtr->frameImage.st);

    ReenableAOIForProcessing();

    SendMessage(THIS_MACHINE, SetUpAOI);
    return IPH_SETUP_AOI;
}

////////////////////////////////////////////////////////////////////
//
// 
//
////////////////////////////////////////////////////////////////////

NEW_STATE   IPH_exitB0(void)
{
    BYTE x;
    int flashUpdateLevel;

  //  POINT start, end;  //Track starting point
    
    // Set Working Page

    if ( startingAOI.enable && startingAOI.needToProcess )
    {
        curAOI                      = startingAOI;
        startingAOI.needToProcess   = FALSE;
        currentQuadrant             = 0;

     //    start.x = curAOI.startX;
     //    start.y = curAOI.startY;

     //    end.x = start.x + curAOI.areaX;
     //    end.y = start.y + curAOI.areaY;

    //     if ( diagnosticsEnabled )
     //        DrawLine(&imageDiagBuffer.frameImage,start,end);
    }
    else
    {
        // if ( currentQuadrant >= MAX_RIBBON_QUADRANTS )
        for ( x = 0; x < MAX_RIBBON_QUADRANTS; x++ )  // Process Maximum number of Areas
        {
            if ( aoiQuadrants[x].needToProcess )
            {
                aoiQuadrants[x].needToProcess = FALSE;

                curAOI = aoiQuadrants[ x ];

                currentQuadrant = x;

                break;
            }
        }

        if ( x == MAX_RIBBON_QUADRANTS ) //If all areas failed
        {

            if ( GetDeviceType() == DEV_TYPE_REGISTER )
            {
                SendMessage(THIS_MACHINE, StartMarkResultValidate);
                return IPH_DO_MARK_RESULT_VALIDATE_REG;
            }
            else
                SendImageStatusItems(MARK_NOT_FOUND);

            return IPH_ACTIVE;

        }
    }

    InitMarkValidationData();
    SetStartingFovStarts(curAOI.startX, curAOI.startY);

    ResetMultiThresholding(fbiPtr);     

    ImageAssign(&aOiFullImage, fbiPtr->frameImage.st + (curAOI.startY * fbiPtr->frameImage.pitch) + curAOI.startX, 
                curAOI.areaX, curAOI.areaY, fbiPtr->frameImage.pitch);   

    GetImageAttributes(fbiPtr, &aOiFullImage);
    //print("Quadrant: %d\n",currentQuadrant);

    if ( (fbiPtr->whiteLevel != GET_WHITE_LEVEL_TARGET()) && (currentQuadrant == 0) )//Do it once per frame
    {
        // TODO as a message

        if ( GetCurrentCameraMode() == CAMERA_SEARCHING )
            flashUpdateLevel = FLASH_POWER_LEVEL_FAST_INCREMENT;
        else
            flashUpdateLevel = FLASH_POWER_LEVEL_SLOW_INCREMENT;


        if ( (fbiPtr->whiteLevel) < (GET_WHITE_LEVEL_TARGET()) )
        {
            IncrementFlashPowerLevel( flashUpdateLevel );
        }
        else
        {
            DecrementFlashPowerLevel( flashUpdateLevel );
        }

        CheckForFplFileUpdate();
    }

    if ( ImageAttributesOkForAnalysis(fbiPtr) == TRUE )
    {
        SendMessage(THIS_MACHINE, StartDetectObjects);

        if ( GetDeviceType() == DEV_TYPE_RIBBON )
            return IPH_DETECT_OBJECTS_RIB;
        else
            return IPH_DETECT_OBJECTS_REG;
    }

    //print("IMAGE NOT OK W:%d B:%d C:%d\n", 
    //    fbiPtr->whiteLevel, fbiPtr->blackLevel, fbiPtr->contrast);

    SendMessage(THIS_MACHINE, SetUpAOI);
    return IPH_SETUP_AOI;
}

////////////////////////////////////////////////////////////////////
//
// DoStartFastVisualScanJpeg while in ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   IPH_exitB1(void)
{
    BOOL objDetectStatus;

    // TODO
    // Send Message to Manager 

    //print("IPH B1\n");

    objDetectStatus = DetectObjects(fbiPtr, &aOiFullImage);

    if ( !objDetectStatus )
    {
        if ( fbiPtr->steppingRetries < (GET_MAX_STEPPING_RETRIES_CURR()) )
        {
            IncrementMultiThresholding(fbiPtr);

            SendMessage(THIS_MACHINE, StartDetectObjects);

            return IPH_DETECT_OBJECTS_RIB;
        }

        SendMessage(THIS_MACHINE, SetUpAOI);
        return IPH_SETUP_AOI;
    }

    SendMessage(THIS_MACHINE, StartMarkRecognition);

    return IPH_DO_MARK_RECOGNITGION_RIB;
}

////////////////////////////////////////////////////////////////////
//
// DoStartFastVisualScanRlc while in ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   IPH_exitB2(void)
{
    // TODO
    // Send Message to Manager 

    //print("IPH B2\n");

    DoMarkRecognition(&imageDiagBuffer.frameImage);

    SendMessage(THIS_MACHINE, StartMarkResultAnalysis);

    return IPH_DO_MARK_RESULT_ANALYSIS_RIB;
}

////////////////////////////////////////////////////////////////////
//
// DoStartFastVisualScanSubsamp2x while in ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   IPH_exitB3(void)
{
    MARK_STATUS markFoundStatus;

    // TODO
    // Send Message to Manager 

    //print("IPH B3\n");

    markFoundStatus   =   DoMarkRecognitionResultAnalysis(&aOiFullImage);

    if ( markFoundStatus == MARK_FOUND )
    {

        if ( currentFrameCaptureMode == FAST_FRAME_ANALYSIS )
        {
            singleLocationCirc.position = fbiPtr->circPosition;
            singleLocationLat.position  = fbiPtr->latPosition;
            //print("IPH B3 Mark Found: LatLocation: %d\n", singleLocationLat.position );
        }

        SendImageStatusItems(markFoundStatus);

        return IPH_ACTIVE;
    }
    else
        if ( fbiPtr->steppingRetries < (GET_MAX_STEPPING_RETRIES_CURR()) )
    {
        IncrementMultiThresholding(fbiPtr);

        SendMessage(THIS_MACHINE, StartDetectObjects);

        return IPH_DETECT_OBJECTS_RIB;
    }

    SendMessage(THIS_MACHINE, SetUpAOI);
    return IPH_SETUP_AOI;
}


////////////////////////////////////////////////////////////////////
//
// DoStartFastVisualScanJpeg while in ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   IPH_exitC1(void)
{
    BOOL objDetectStatus;

    // TODO
    // Send Message to Manager 

    //print("IPH C1\n");

    objDetectStatus = DetectObjects(fbiPtr, &aOiFullImage);

    if ( !objDetectStatus )
    {
        if ( fbiPtr->steppingRetries < (GET_MAX_STEPPING_RETRIES_CURR()) )
        {
            IncrementMultiThresholding(fbiPtr);

            SendMessage(THIS_MACHINE, StartDetectObjects);

            return IPH_DETECT_OBJECTS_REG;
        }

        SendMessage(THIS_MACHINE, SetUpAOI);
        return IPH_SETUP_AOI;
    }

    SendMessage(THIS_MACHINE, StartMarkRecognition);

    return IPH_DO_MARK_RECOGNITGION_REG;
}

////////////////////////////////////////////////////////////////////
//
// DoStartFastVisualScanRlc while in ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   IPH_exitC2(void)
{
    // TODO
    // Send Message to Manager 

    //print("IPH C2\n");

    DoRegMarkRecognition(&imageDiagBuffer.frameImage);

    SendMessage(THIS_MACHINE, StartMarkResultAnalysis);

    return IPH_DO_MARK_RESULT_ANALYSIS_REG;
}

////////////////////////////////////////////////////////////////////
//
// DoStartFastVisualScanSubsamp2x while in ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   IPH_exitC3(void)
{
    BOOL allMarkFoundStatus;

    // TODO
    // Send Message to Manager 

    //print("IPH C3\n");

    allMarkFoundStatus   =   
    DoRegMarkRecognitionResultAnalysis(&imageDiagBuffer.frameImage);

    if ( allMarkFoundStatus == TRUE )
    {
        if ( currentFrameCaptureMode == FAST_FRAME_ANALYSIS )
        {
            singleLocationCirc.position = fbiPtr->circPosition;
            singleLocationLat.position  = fbiPtr->latPosition;
            //print("IPH C3 Mark Found: LatLocation: %d\n", singleLocationLat.position );
        }

        SendMessage(THIS_MACHINE, StartMarkResultValidate);
        return IPH_DO_MARK_RESULT_VALIDATE_REG;
    }
    else
        if ( fbiPtr->steppingRetries < (GET_MAX_STEPPING_RETRIES_CURR()) )
    {
        IncrementMultiThresholding(fbiPtr);

        SendMessage(THIS_MACHINE, StartDetectObjects);

        return IPH_DETECT_OBJECTS_REG;
    }

    if ( IsRegReferenceMarkFound()  && IsRegMinMarksFound() )
    {
        SendMessage(THIS_MACHINE, StartMarkResultValidate);
        return IPH_DO_MARK_RESULT_VALIDATE_REG;
    }

    SendMessage(THIS_MACHINE, SetUpAOI);

    return IPH_SETUP_AOI;
}

NEW_STATE   IPH_exitC4(void)
{
    // TODO
    // Send Message to Manager 

    //print("IPH_exitC4\n");

    if ( DoRegMarkValidations(fbiPtr, &imageDiagBuffer.frameImage) )
    {
        if ( currentFrameCaptureMode == FAST_FRAME_ANALYSIS )
        {
            singleLocationCirc.position = fbiPtr->circPosition;
            singleLocationLat.position  = fbiPtr->latPosition;
            //print("IPH C4 Mark Found: LatLocation: %d\n", singleLocationLat.position );
        }

        SendImageStatusItems(MARK_FOUND);
    }
    else
        SendImageStatusItems(MARK_NOT_FOUND);

    return IPH_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// NewAcquirerImageReady while in _IPH_ACTVE_WAIT_FOR_IMAGE_RAW
//
////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_IPH_IDLE)
EV_HANDLER(GoActive, IPH_exitA)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_IPH_ACTIVE)
EV_HANDLER(ProcessImage, IPH_exitB)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_IPH_IPH_SETUP_AOI)
EV_HANDLER(SetUpAOI, IPH_exitB0)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_IPH_DETECT_OBJECTS_RIB)
EV_HANDLER(StartDetectObjects, IPH_exitB1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_IPH_DO_MARK_RECOGNITGION_RIB)
EV_HANDLER(StartMarkRecognition, IPH_exitB2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_IPH_DO_MARK_RESULT_ANALYSIS_RIB)
EV_HANDLER(StartMarkResultAnalysis, IPH_exitB3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_IPH_DETECT_OBJECTS_REG)
EV_HANDLER(StartDetectObjects, IPH_exitC1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_IPH_DO_MARK_RECOGNITGION_REG)
EV_HANDLER(StartMarkRecognition, IPH_exitC2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_IPH_DO_MARK_RESULT_ANALYSIS_REG)
EV_HANDLER(StartMarkResultAnalysis, IPH_exitC3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_IPH_DO_MARK_RESULT_VALIDATE_REG)
EV_HANDLER(StartMarkResultValidate, IPH_exitC4)
STATE_TRANSITION_MATRIX_END;

// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(IPH_Main_Entry)
STATE(_IPH_IDLE)                        ,           
STATE(_IPH_ACTIVE)                      ,
STATE(_IPH_IPH_SETUP_AOI)               ,
STATE(_IPH_DETECT_OBJECTS_RIB)              ,
STATE(_IPH_DO_MARK_RECOGNITGION_RIB)        ,
STATE(_IPH_DO_MARK_RESULT_ANALYSIS_RIB)     ,
STATE(_IPH_DETECT_OBJECTS_REG)              ,
STATE(_IPH_DO_MARK_RECOGNITGION_REG)        ,
STATE(_IPH_DO_MARK_RESULT_ANALYSIS_REG)     ,
STATE(_IPH_DO_MARK_RESULT_VALIDATE_REG)     
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////





























