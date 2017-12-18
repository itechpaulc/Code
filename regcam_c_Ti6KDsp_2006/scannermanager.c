




//
//
//
//  Author :    Paul Calinawan
//
//  Date:       June 27, 2006
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


#include "scannermanager.h"

#include "camerapositionmanager.h"

#include "tcpcomm.h"

#include "spicomm.h"

#include "transportmanager.h"

#include "markrecognitionhandler.h"

#include "imageacquirerhandler.h"

#include "imagetransmitter.h"

#include "encoderhandler.h"

#define PIXEL_SIZE          ( 40 )

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

SCAN_REQUEST                requestedScannerScanMode,
currentScannerScanMode;

CAMERA_STATE                currentCameraMode;

SCANNER_POSITION            requestedScannerStartPosition,
currentScannerStartPosition;

SCANNER_POSITION            currentScannerPosition;

//

DWORD                       requestedScannerTransmitType,
currentScannerTransmitType;

SCANNER_IMAGE_ANALYSIS      requestedScannerImageAnalysis,
currentScannerImageAnalysis;


BOOL                        requestedSimulationMode,
currentSimulationMode;


int                         lockedSearchMaxRetry;
int                         lockedSearchRetryCount;


#define                     MIN_FOUND_GOOD_COUNT    (3)

int                         markFoundGoodCount;



////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////

// TODO part of Ribbon Manager??

BOOL    isCameraAddressChanged;
BOOL    isImageCaptureConfigChanged;
BOOL    isEncoderConfigChanged;
BOOL    isTransportConfigChanged;
BOOL    isSearchConfigChanged;
BOOL    isCameraDirectionConfigChanged;

BYTE    searchMode;
BOOL    markValid;

BOOL    topEdgeFirst; //PM don't check in
// isPositionManagerConfigChanged;

// isImageProcConfigChanged;

BOOL    IsRequestedScannerStartPositionChanged(void);




///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////


#ifdef 0
void    CheckForConfigurationChanges(void)
{

}

void    SetImageProcessorConfiguration(void)
{
    if ( isImageProcConfigChanged )
    {

    }
}

void    SetCameraDirectionConfiguration(void)
{
    if ( isCameraDirectionConfigChanged )
    {

    }
}

void    SetEncoderHandlerConfiguration(void)
{
    if ( isEncoderConfigChanged )
    {

    }
}

void    SetPositionManagerConfiguration(void)
{
    if ( isPositionManagerConfigChanged )
    {

    }
}

void    SetTransportManagerConfiguration(void)
{
    if ( isImageProcConfigChanged )
    {

    }
}
#endif

extern void ResetFrameBuffers(void);


void StartImageCaptureTimeOut()
{
//     DWORD toTimeMS;
//
//     //Calculate ms per Revolution
//     toTimeMS = (( currentPeriod * currentSystemConfiguration.encoder.aPulsesPerImpression)/8000);
//     toTimeMS = toTimeMS << 1;  //Make it two revolutions
    StartTimer( 2000 );
}

////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////

void    Init_ScannerManager(void)
{
    requestedScannerScanMode    = SCANNER_DISABLED;
    currentScannerScanMode      = SCANNER_DISABLED;

    requestedScannerStartPosition.circumPosition    =
    requestedScannerStartPosition.lateralPosition   = 
    NULL_SCANNER_POSITION;

    currentScannerStartPosition.circumPosition      = 
    currentScannerStartPosition.lateralPosition     =
    NULL_SCANNER_POSITION;

    currentScannerPosition = currentScannerStartPosition;

    requestedSimulationMode = 
    currentSimulationMode   = SIMULATION_RUN;

    requestedScannerTransmitType = 
    currentScannerTransmitType  = 5;

    requestedScannerImageAnalysis =
    currentScannerImageAnalysis   = SCANNER_IMAGE_ANALYSIS_ENABLED;

    lockedSearchMaxRetry = INIT_SEARCH_TO_LOCK_MAX_RETRY;
    lockedSearchRetryCount = 0;

    markFoundGoodCount = 0;
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

NEW_STATE   SCM_exitA(void)
{
    CAMERA_POSITION camReqScanStartPos;

    //print("SCM A ACTIVE\n");

    camReqScanStartPos.circumPosition = 0;
    camReqScanStartPos.lateralPosition = 0;

    SetRequestedScannerStartPosition(camReqScanStartPos);

    SendMessage(FlashCounterLogger, GoActive);
    SendMessage(EncoderHandler, GoActive);
    SendMessage(ImageTransmitter, GoActive);

    SendMessage(TriggerMonitor, GoActive);
    SendMessage(ImageAcquirerHandler, GoActive);
    SendMessage(ImageProcessingHandler, GoActive);

    SendMessage(DataTransmitter, GoActive);

    SendMessage(FlashPowerController, GoActive);
    SendMessage(TransportManager, FindHomeTransport);
    SetCurrentCameraMode( CAMERA_AT_HOME );

    SetRequestedScannerScanMode( SCANNER_DISABLED );
    SetCurrentScannerScanMode( SCANNER_DISABLED );

    // TODO
    //   StartTimer(SECONDS(15));

    SendMessage(THIS_MACHINE, CheckForTransportReady);

    return SCM_WAIT_FOR_TRANSPORT_READY;
}

NEW_STATE   SCM_exitA1(void)
{
    if ( transportNeedsHome )
    {
        //printf("SCM_exitA1 - Transport Needs Home");
        SendMessage(TransportManager, FindHomeTransport);
        return SAME_STATE;
    }

    if ( !transportReady )
    {
        SendMessage(THIS_MACHINE, CheckForTransportReady);
        PostMessage(TransportManager, IsTransportReady);
        return SAME_STATE;
    }

    SendMessage(THIS_MACHINE, ScannerModeChangeRequested);

    return SCM_ACTIVE;
}

NEW_STATE   SCM_exitA2(void)
{
    // TODO
    //printf("SCM_exitA2 - Transport Needs Home");
    SendMessage(TransportManager, FindHomeTransport);
    SendMessage(THIS_MACHINE, CheckForTransportReady);
    return SCM_WAIT_FOR_TRANSPORT_READY;
}

////////////////////////////////////////////////////////////////////
//
// ScannerModeChangeRequested
//      While in SCM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitB(void)
{
    SCAN_REQUEST scanMode = GetRequestedScannerScanMode();

    //TODO

    // Set ScannerSearchConfiguration - if changed
    // Set ImageProcessorConfiguration - if changed
    // Set CameraDirectionConfiguration - if changed
    // Set EncoderHandlerConfiguration - if changed
    // Set PositionManagerConfiguration - if changed
    // Set TransportManagerConfiguration - if changed


    // Set Other Configurations - if changed

    //print("MODE CHANGE %d\n", scanMode);

    SetCurrentScannerScanMode(scanMode);

    //SetCameraSearchStartPosition(GetRequestedScannerStartPosition());
    //SetCurrentScannerStartPosition(GetRequestedScannerStartPosition());

    switch ( scanMode )
    {
        case    SCANNER_DISABLED:
            SendMessage(THIS_MACHINE, StartScannerDisable);

            EnableFlashPower(FLASH_POWER_OFF);   
            return SCM_DISABLED_STARTING_SCANNER_DISABLE;

        case    SCANNER_ENABLED:
            EnableFlashPower(FLASH_POWER_ON);   

            if ( GetCurrentScannerImageAnalysis() == SCANNER_IMAGE_ANALYSIS_ENABLED ) // Temporary Mode
            {
                //print("************************ Manual Mode\n");
                searchMode  = MANUAL_FIND;
            }
            else
            {
                //print("************************ Auto Mode\n");
                searchMode  = LIMITED_AREA;
            }
            markValid   = FALSE;
            SendMessage(THIS_MACHINE, StartLimitedAlternatingSearch);

            SetCurrentCameraMode( CAMERA_SEARCHING );

            return SCM_LIM_SETUP_SEARCH_PATTERN;

//         case    FULL_ALTERNATING_SEARCH:
//             SendMessage(THIS_MACHINE, StartFullAlternatingSearch);
//             return SCM_STARTING_FULL_ALTERNATING_SEARCH;
//
//         case    FULL_LINEAR_SEARCH:
//             SendMessage(THIS_MACHINE, StartFullLinearSearch);
//             return SCM_STARTING_FULL_LINEAR_SEARCH;
//
//         case    FULL_SMART_SEARCH:
//             SendMessage(THIS_MACHINE, StartFullSmartSearch);
//             return SCM_STARTING_FULL_SMART_SEARCH;
//
//         case    FULL_CUSTOM_SEARCH:
//             SendMessage(THIS_MACHINE, StartFullCustomSearch);
//             return SCM_STARTING_FULL_CUSTOM_SEARCH;
//
//         case    LOCK_TARGET:
//             SendMessage(THIS_MACHINE, StartLockTarget);
//             return SCM_STARTING_LOCK_TARGET;

        case SCANNER_FREE_RUN:
            SendMessage(THIS_MACHINE, StartFreeRun);
            return SCM_STARTING_FREE_RUN;

        case    SCANNER_PAUSE:
            SendMessage(THIS_MACHINE, StartLevitate);
            return SCM_STARTING_LEVITATE;

        case    SCANNER_MANUAL:
            searchMode  = MANUAL_FIND;
            markValid   = FALSE;
            SendMessage(THIS_MACHINE, StartLimitedAlternatingSearch);
            return SCM_STARTING_MANUAL;

            // TODO
        default:
            return SCM_DISABLED;
    }
}


////////////////////////////////////////////////////////////////////
//
// StartScannerEnable
//      While in SCM_STARTING_SCANNER_ENABLE
//
////////////////////////////////////////////////////////////////////

//
//  
//
//  TODO        
//
//  IMAGE TRANSMITTER SETUP
//  ENCODER HANDLER SETUP
//

NEW_STATE   SCM_exitB1(void)
{
    //print("SCANNER ENABLED\n");

    //print("SCM PREPARING IMAGE ACQUISITION\n");

    SendMessage(ImageAcquirerHandler, DoStartImageAcquisition);

    return SCM_ENABLED_WAIT_FOR_IMAGE_ACQUISITION_READY;

}

////////////////////////////////////////////////////////////////////
//
// NewAcquirerImageReady
// DoNextImageCaptureCycle
//      While in SCM_ENABLED_WAIT_FOR_IMAGE_ACQUISITION_READY
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitB2(void)
{
    DWORD direction;
    int difference;

    SCANNER_POSITION curScanPos = GetCurrentScannerStartPosition();
    SCANNER_POSITION reqScanPos = GetRequestedScannerStartPosition();


    // TODO Find a better place

    if ( GetRequestedScannerTransmitType() != GetCurrentScannerTransmitType() )
    {
        SetCurrentScannerTransmitType(GetRequestedScannerTransmitType());
    }

    if ( GetRequestedSimulationMode() != GetCurrentSimulationMode() )
    {
        SetCurrentSimulationMode(GetRequestedSimulationMode());
    }

    if ( GetRequestedScannerImageAnalysis() != GetCurrentScannerImageAnalysis() )
    {
        SetCurrentScannerImageAnalysis(GetRequestedScannerImageAnalysis());
    }



    if ( curScanPos.lateralPosition != reqScanPos.lateralPosition )
    {
        //print("SCM_exitB2  SETTING TRANSPORT POSITION CURR:%d  REQ:%d \n", 
        //      curScanPos.lateralPosition, reqScanPos.lateralPosition);

//         difference = reqScanPos.lateralPosition - curScanPos.lateralPosition;
//
//         if ( difference > 0 )
//             direction = MOVE_TO_CW;
//         else
//             direction = MOVE_TO_LEFT;
//
//         SendMessageAndData(TransportManager, MoveTransport, direction, abs( difference) );
        calcTransportStepDirection( reqScanPos.lateralPosition );

        // TODO

        SetCurrentScannerStartPosition(GetRequestedScannerStartPosition());

        return SCM_ENABLED_MOVING_TRANSPORT;
    }

    //print("SCM NO TRANSPORT MOVES\n");

    SendMessage(THIS_MACHINE, SkipToNextState);

    return SCM_ENABLED_MOVING_TRANSPORT;
}

BOOL calcTransportStepDirection( DWORD transportTraget )
{
    BOOL towardsHome;
    int difference;

    if ( currentScannerPosition.lateralPosition != transportTraget )
    {
        //print("\nSCM cTSD SETTING TRANSPORT POSITION CURR:%d  REQ:%d \n", 
        //     currentScannerPosition.lateralPosition, transportTraget);

        difference = transportTraget - currentScannerPosition.lateralPosition;

        if ( difference > 0 )
            towardsHome = FALSE;
        else
            towardsHome = TRUE;

        SendMessageAndData(TransportManager, MoveTransport, towardsHome, abs( difference) );
        return TRUE;
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////
//
// TransportPostionSet
//      While in SCM_ENABLED_SETTING_TRANSPORT_POSITION
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitB3(void)
{
    //print("SCM MOVING TRANSPORT\n");

    SendMessage(TransportManager, MoveTransport);

    return  SCM_ENABLED_MOVING_TRANSPORT;
}

////////////////////////////////////////////////////////////////////
//
//  TransporMoveDone
//  SkipToNextState
//      While in SCM_ENABLED_MOVING_TRANSPORT
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitB4(void)
{
    DWORD   flashTriggerTarget = 
    GetRequestedScannerStartPosition().circumPosition;

    //print("SCM SETUP TRIG POSITION\n");

    SendMessageAndData(EncoderHandler, SetupTriggerPositions, flashTriggerTarget, NO_DATA);

    return  SCM_ENABLED_SETTING_TRIGGERS;
}

////////////////////////////////////////////////////////////////////
//
//  TransportAtLimits
//  SkipToNextState
//      While in SCM_ENABLED_MOVING_TRANSPORT
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitB11(void)
{
    //TODO Manage the limits proprely

    DWORD   flashTriggerTarget = 
    GetRequestedScannerStartPosition().circumPosition;

    //print("SCM SETUP TRIG POSITION at Limits\n");

    SendMessageAndData(EncoderHandler, SetupTriggerPositions, flashTriggerTarget, NO_DATA);

    return  SCM_ENABLED_SETTING_TRIGGERS;
}

////////////////////////////////////////////////////////////////////
//
// TriggerPositionSet
//      While in SCM_ENABLED_SETTING_TRIGGERS
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitB5(void)
{
    DWORD   flashTriggerTarget = 
    GetRequestedScannerStartPosition().circumPosition;

    //print("SCM BEGIN TRIGGER PROCESS\n");

    SendMessageAndData(EncoderHandler, BeginTriggerProcess, flashTriggerTarget, NO_DATA);

    return  SCM_ENABLED_WAIT_TRIGGER_PROCESS_BEGIN;
}

////////////////////////////////////////////////////////////////////
//
// TriggerProcessBegins
//      While in SCM_ENABLED_SETTING_TRIGGERS
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitB6(void)
{
    //print("SCM TAKING IMAGE...\n");

    // Time to allow the trigger to occur
    // and for the camera to acquire the CCD image
    // TODO change to impression count if in simulation mode

    StartTimer(MILLISECONDS(250));

    return  SCM_ENABLED_WAIT_TAKE_IMAGE_DONE;
}

////////////////////////////////////////////////////////////////////
//
// NewAcquirerImageReady
//      While in SCM_ENABLED_WAIT_TAKE_IMAGE_DONE
//
////////////////////////////////////////////////////////////////////
BOOL  latchedImpressionCountFlag = 0;
BYTE  retries = 0;

NEW_STATE   SCM_exitB7(void)
{
    //print("SCM PROCESSING IMAGE\n");

    SendMessage(FlashCounterLogger, IncrementFlashCount);

    SendMessage(ImageProcessingHandler, ProcessImage);

    CancelTimer();

    latchedImpressionCountFlag = 0;
    retries = 0;

    return  SCM_ENABLED_WAIT_IMAGE_PROCESSED;
}

////////////////////////////////////////////////////////////////////
//
// TimeOut
//      While in SCM_ENABLED_WAIT_TAKE_IMAGE_DONE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitB7A(void)
{
    static DWORD latchedImpressionCount;
    DWORD newImpressionCount;

    GetSPIDataWithRetries( GET_IMPRESSION_COUNT, &newImpressionCount );

    if ( !latchedImpressionCountFlag )
    {
        latchedImpressionCountFlag = 1;
        latchedImpressionCount = newImpressionCount;
    }
    else if ( ((latchedImpressionCount + 2) <= newImpressionCount) || (retries >= 4) )
    {
        //print("SCM IMAGE READY TIMEOUT !!!!! 0x%x, 0x%x, 0x%x\n",newImpressionCount,latchedImpressionCount,retries);

        latchedImpressionCountFlag = 0;
        retries = 0;
        StartTimer(MILLISECONDS(250));

        return  SCM_ENABLED_WAIT_IMAGE_TRANSMITTED_DELAY;
    }

    retries++;
    StartTimer(MILLISECONDS(250));
    return SAME_STATE;
    // TO DO just push to the next state
}

////////////////////////////////////////////////////////////////////
//
// ImageProcessed
//      While in SCM_ENABLED_WAIT_TAKE_IMAGE_DONE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitB8(void)
{

    if ( GetCurrentScannerImageCollectMode()!=GetRequestedScannerImageCollectMode() )
    {
        //print("SCM UPDATE TRANSMITTING IMAGE MODE\n");
        SetCurrentScannerImageCollectMode(GetRequestedScannerImageCollectMode());
    }

    SendMessage(DataTransmitter, SendCameraControlStatus);

    return  SCM_ENABLED_WAIT_IMAGE_TRANSMITTED;
}

////////////////////////////////////////////////////////////////////
//
// ImageTransmitted
// SkipToNextState
//      While in SCM_ENABLED_WAIT_IMAGE_TRANSMITTED
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitB9(void)
{
    //print("SCM NEXT CYCLE DELAY\n");

    StartTimer(1);

    return  SCM_ENABLED_WAIT_IMAGE_TRANSMITTED_DELAY;
}



////////////////////////////////////////////////////////////////////
//
// TimeOut
//      While in SCM_ENABLED_WAIT_IMAGE_TRANSMITTED_DELAY
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitB10(void)
{
    if ( GetRequestedScannerScanMode() != GetCurrentScannerScanMode() )
    {
        //print("SCM SCANNER MODE CHANGED\n");
        SendMessage(THIS_MACHINE, ScannerModeChangeRequested);


        // TODO make a state
        //print("SCM PUT IAH back to ACTIVE\n");
        SendMessage(ImageAcquirerHandler, ImageAcquirerGoWaitActive);

        return  SCM_ACTIVE;
    }
    else
    {
        SendMessage(THIS_MACHINE, DoNextImageCaptureCycle);
        return  SCM_ENABLED_WAIT_FOR_IMAGE_ACQUISITION_READY;
    }
}

////////////////////////////////////////////////////////////////////
//
// StartLimitedAlternatingSearch
//      While in SCM_STARTING_LIMITED_ALTERNATING_SEARCH
//
////////////////////////////////////////////////////////////////////

int remainingScannerNumSearch;

NEW_STATE   SCM_exitC(void)
{
    SCANNER_POSITION startLocation = GetRequestedScannerStartPosition() ;


    if ( searchMode == MANUAL_FIND )
    {
        //print("\nSTART MANUAL\n");
        frameCaptureMode = SINGLE_FRAME_ANALYSIS;
        singleLocationCirc.position = NormalizeCircumPosition( startLocation.circumPosition );

        if ( calcTransportStepDirection( startLocation.lateralPosition ) )
            return SCM_LIM_WAIT_TRANSPORT_DONE;

        SendMessage(ImageAcquirerHandler, DoStartImageAcquisition);
        StartImageCaptureTimeOut();
        return  SCM_LIM_WAIT_LOCK_SCAN_STATUS;
    }

    //print("\nSTART LIMITED\n");

    PrepareCameraSearchPositions(LIMITED_ALTERNATING_SEQUENCE);

    // TODO
    ShowAllCameraSearchPositions();

    frameCaptureMode = FAST_FRAME_ANALYSIS;
    SetCurrentCameraMode( CAMERA_SEARCHING );

    // SendMessageAndData(TransportManager, MoveTransport, GetNextCameraLatSeachPosition(), NULL_MESSAGE_ID);
    if ( calcTransportStepDirection( GetNextCameraLatSeachPosition() ) )
    {
//        StartTimer(MILLISECONDS(600));
        return SCM_LIM_WAIT_TRANSPORT_DONE;
    }

    SendMessage(ImageAcquirerHandler, DoStartImageAcquisition);
    StartImageCaptureTimeOut();
    return  SCM_LIM_WAIT_SEARCH_DONE;
}
////////////////////////////////////////////////////////////////////
//
// TransporMoveDone
//      While in SCM_LIM_WAIT_TRANSPORT_DONE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitC1(void)
{
    // CalcualateTriggers();
    // Set Shutter
    // Set Flash
    // Go

    // TODO

    //print("\nSCM C1 SENDING DoStartImageAcquisition\n");

    //   GetSPIDataWithRetries(GET_TRANSPORT_POSITION, &currentScannerPosition.lateralPosition);
    SendMessage(ImageAcquirerHandler, DoStartImageAcquisition);
    StartImageCaptureTimeOut();

    if ( searchMode == MANUAL_FIND )
    {
        return SCM_LIM_WAIT_LOCK_SCAN_STATUS;
    }

    return  SCM_LIM_WAIT_SEARCH_DONE;
}

NEW_STATE   SCM_exitC1A(void)
{
    //print("\nSCM C1A LimitDetected\n");

    SendMessage(TransportManager, FindHomeTransport);
    SetCurrentCameraMode( CAMERA_AT_HOME );

    return SCM_LIM_WAIT_TRANSPORT_HOME;
}


////////////////////////////////////////////////////////////////////
//
// NewAcquirerImageReady
//      While in SCM_LIM_WAIT_IMAGE_READY
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitC2(void)
{
    BYTE   markFound   =  (BYTE) GetMessageData1();

//    print("SCM exitC2\n");
    CancelTimer();

    if ( markFound == MARK_FOUND )
    {
        //print("SCM exitC2 Mark Found\n");

        frameCaptureMode = SINGLE_FRAME_ANALYSIS;

        SendMessage(ImageAcquirerHandler, AreYouReady);

        StartTimer(MILLISECONDS(2000));

        return SCM_LIM_WAIT_ACQUIRER_READY;
    }

    StartTimer(MILLISECONDS(600));

    return SAME_STATE;
}

NEW_STATE   SCM_exitC2A(void)
{
    DWORD   direction;
    int     newCircLocation;
    long    steps;
    int     stepDifference;

    BYTE markNumber;

    SCANNER_POSITION startLocation = GetRequestedScannerStartPosition() ;

    markNumber = GetReferenceMarkIndex();

//    print("SCM exitC2A\n");


    if ( searchMode == MANUAL_FIND )
    {

        singleLocationCirc.position = NormalizeCircumPosition( startLocation.circumPosition );
        singleLocationLat.position  = startLocation.lateralPosition;

        if ( calcTransportStepDirection( startLocation.lateralPosition) )
            return SCM_LIM_WAIT_TRANSPORT_CENTERED;

        SendMessage(ImageAcquirerHandler, DoStartImageAcquisition);
        StartImageCaptureTimeOut();
        return SCM_LIM_WAIT_LOCK_SCAN_STATUS;
    }

    //print("SCM_exitC2A MarkFound- SLat:%d SCirc:%d FLat:%d FCirc:%d Clat:%d CCirc:%d\n", 
    //   singleLocationLat.position,
    //    singleLocationCirc.position,
    //    markRecognitionData[ markNumber ].markFovLateral,
    //    markRecognitionData[ markNumber ].markFovCircum, 
    //     currentScannerPosition.lateralPosition,
    //     currentScannerPosition.circumPosition); 

    //PMDebug - Recenter based on camera going from top to bottom
    if ( abs(markRecognitionData[ markNumber ].markFovCircum)  > 1300 )
    {
        if ( topEdgeFirst )
            newCircLocation  = (int) (singleLocationCirc.position + ((markRecognitionData[ markNumber ].markFovCircum * PIXEL_SIZE )/200));
        else
            newCircLocation  = (int) (singleLocationCirc.position - ((markRecognitionData[ markNumber ].markFovCircum * PIXEL_SIZE )/200));
    }
    else
        newCircLocation = (int) singleLocationCirc.position;

    //print(" SCM_exitC2A NL SingleLoc:%d \n",newCircLocation); 

    singleLocationCirc.position = NormalizeCircumPosition( newCircLocation );

    //print(" SCM_exitC2A SingleLoc:%d \n",singleLocationCirc.position); 

    //steps = ((7179 * markRecognitionData[ markNumber ].markFovLateral)/1000000); //pixel to step conversion


    //steps = ((5708 * markRecognitionData[ markNumber ].markFovLateral)/100000000); //pixel to step conversion consider scaling

    steps =  markRecognitionData[ markNumber ].markFovLateral * PIXEL_SIZE; //Get Current Distance

    steps *=  10;  //Scale up to for micronPerStep lat is already 100x

    steps /=  currentSystemConfiguration.transport.micronsPerStep;;

    if ( abs(steps) > 35 )
    {
        if ( currentSystemConfiguration.cameraImaging.reverseTransportCentering )
            steps *= -1;

        steps += singleLocationLat.position;

        calcTransportStepDirection( (DWORD) steps);

        //    printf("Step %d Fov:%d\n", steps, markRecognitionData[ markNumber ].markFovLateral);
        return SCM_LIM_WAIT_TRANSPORT_CENTERED;
    }

    //  GetSPIDataWithRetries(GET_TRANSPORT_POSITION,&singleLocationLat.position);

    SendMessage(ImageAcquirerHandler, DoStartImageAcquisition);
    StartImageCaptureTimeOut();
    return SCM_LIM_WAIT_LOCK_SCAN_STATUS;
}
NEW_STATE   SCM_exitC2B(void)
{
    SendMessage(ScannerManager, ImageAcquirerReady);
    return SAME_STATE;
}
////////////////////////////////////////////////////////////////////
//
// NewAcquirerImageReady
//      While in SCM_LIM_WAIT_IMAGE_READY
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitC3(void)
{
    DWORD nextLatLoc;
    SCANNER_POSITION camStartPos;
    int numLatPos;

    CancelTimer();

    if ( IsNewImageAvailable() )
    {
        PostMessage(ImageProcessingHandler, ProcessImage);
        return SCM_LIM_WAIT_IMAGE_PROCESS_DONE;
    }

    numLatPos = GetCameraSearcNumberOfLatPositions();

    if ( numLatPos == 0 )
    {
        if ( searchMode == LIMITED_AREA )
        {
            //print("SCM C3 StartFull \n");

            searchMode  = FULL_CIRC;

            camStartPos = GetRequestedScannerStartPosition();

            SetCurrentCamLatPosition(camStartPos.lateralPosition);

            FillCircSearchPositions(camStartPos.circumPosition, searchMode);

            ShowAllCameraSearchPositions();

            if ( calcTransportStepDirection( camStartPos.lateralPosition ) )
                return SCM_LIM_WAIT_TRANSPORT_DONE;

            SendMessage(ImageAcquirerHandler, DoStartImageAcquisition);
            StartImageCaptureTimeOut();

            return SAME_STATE;
        }
        //print("SCM C3 Limited Circ \n");

        searchMode  = LIMITED_AREA;
        SendMessage(TransportManager, FindHomeTransport);
        SendMessageAndData(WatchDog_Manager, ResetWDT,TRUE,0);
        SetCurrentCameraMode( CAMERA_AT_HOME );

        //print("SCM C3 SendTranHome \n");
        return SCM_LIM_WAIT_TRANSPORT_HOME;
    }

    nextLatLoc = GetNextCameraLatSeachPosition();
    //print("SCM C3 NextLatLoc: %d NumLatPos:%d Time:%x:%d\n", nextLatLoc, numLatPos,getvar(SEC),getvar(MSEC));

    camStartPos = GetRequestedScannerStartPosition();
    FillCircSearchPositions(camStartPos.circumPosition, searchMode);

    if ( calcTransportStepDirection( nextLatLoc ) )
        return SCM_LIM_WAIT_TRANSPORT_DONE;

    SendMessage(ImageAcquirerHandler, DoStartImageAcquisition);
    StartImageCaptureTimeOut();
    return SAME_STATE;
}

NEW_STATE   SCM_exitC3A(void)
{
    BYTE   markFound   =  (BYTE) GetMessageData1();

//    print("SCM exitC2\n");
    CancelTimer();

    if ( markFound == MARK_FOUND )
    {
        //print("SCM_exitC3A Mark Found\n");

        frameCaptureMode = SINGLE_FRAME_ANALYSIS;

        SendMessage(ImageAcquirerHandler, AreYouReady);

        StartTimer(MILLISECONDS(2000));

        return SCM_LIM_WAIT_ACQUIRER_READY;
    }

    StartTimer(MILLISECONDS(600));

    SendMessage(THIS_MACHINE,CircFrameCaptureDone);

    return SCM_LIM_WAIT_SEARCH_DONE;
}

NEW_STATE   SCM_exitC4(void)
{
    //print("SCM C4\n");
    // GetSPIDataWithRetries(GET_TRANSPORT_POSITION, &currentScannerPosition.lateralPosition);
    FillSearchPositions(searchMode); //Reset All Search Locations and repeat
    SendMessage(THIS_MACHINE, StartLimitedAlternatingSearch);
    return SCM_LIM_SETUP_SEARCH_PATTERN;
}

NEW_STATE   SCM_exitC5(void)
{
    singleLocationLat.position = currentScannerPosition.lateralPosition;

    SendMessage(ImageAcquirerHandler, DoStartImageAcquisition);

    //print("SCM C5 Addr:%p, Lat: %d Circ:%d  \n", nextFrameBufferImageEmpty
    //      ,nextFrameBufferImageEmpty->latPosition
    //     , nextFrameBufferImageEmpty->circPosition);

    StartImageCaptureTimeOut();

    return SCM_LIM_WAIT_LOCK_SCAN_STATUS;
}

//
//
//

NEW_STATE   SCM_exitC6(void)
{
    BYTE    markFound   =  (BYTE) GetMessageData1();
    BYTE    refIndex = GetReferenceMarkIndex();

    static DWORD   lastCircFoundLocation, lastLatFoundLocation;

    CancelTimer();

    //print("SCM C6\n");

    if ( searchMode == MANUAL_FIND )
    {
        if ( markFound == MARK_FOUND )
        {
            if ( markValid == FALSE )
            {
                markValid = TRUE;
                SetCurrentCameraMode(CAMERA_LOCKED_ON);
            }

            SetStartingAOI( markRecognitionData[ refIndex ].markFovLateral, markRecognitionData[ refIndex ].markFovCircum ); 
        }
        else if ( markValid == TRUE )
        {
            markValid = FALSE;
            SetCurrentCameraMode(CAMERA_SEARCHING);
        }

        frameCaptureMode = SINGLE_FRAME_ANALYSIS;
        SendMessage(ScannerManager, ImageAcquirerReady);
        StartTimer(MILLISECONDS(2000));
        return SCM_LIM_WAIT_ACQUIRER_READY;
    }

    if ( markFound == MARK_FOUND )
    {
        //print("SCM exitC6 Mark Found: G:%d R:%d MR:%d V:%d\n",
        //      markFoundGoodCount, lockedSearchRetryCount, lockedSearchMaxRetry, markValid);

        frameCaptureMode = SINGLE_FRAME_ANALYSIS;

        if ( (markValid == FALSE) )
        {
            markFoundGoodCount++;

            if ( markFoundGoodCount >= MIN_FOUND_GOOD_COUNT )
            {
                markValid = TRUE;
                markFoundGoodCount = 0;
                searchMode  = LIMITED_AREA;
                lockedSearchMaxRetry = INIT_LOCKED_TO_SEARCH_MAX_RETRY;

                SetCurrentCameraMode(CAMERA_LOCKED_ON);

                ClearAllPositions();
                ResetFrameBuffers();
            }
        }
        else
        {
            lockedSearchRetryCount = 0; //Just reset your retry counter
            //Copy last found location
            lastCircFoundLocation   =   singleLocationCirc.position;
            lastLatFoundLocation    =   singleLocationLat.position;
            SetStartingAOI( markRecognitionData[ refIndex ].markFovLateral, markRecognitionData[ refIndex ].markFovCircum );
            SetCurrentCameraMode(CAMERA_LOCKED_ON);
        }
        //print("SCM C6 1 Single Frame: %d\n", singleLocationCirc.position);

        //  SendMessage(ImageAcquirerHandler, DoStartImageAcquisition);

        SendMessage(ScannerManager, ImageAcquirerReady);

        StartTimer(MILLISECONDS(2000));
        return SCM_LIM_WAIT_ACQUIRER_READY;
    }
    else
    {
        lockedSearchRetryCount++;

        //print("SCM exitC6 NotMark Found: G:%d R:%d MR:%d V:%d\n",
        //      markFoundGoodCount, lockedSearchRetryCount, lockedSearchMaxRetry, markValid);

        if ( lockedSearchRetryCount >=  lockedSearchMaxRetry )
        {
            frameCaptureMode = FAST_FRAME_ANALYSIS;

            if ( markValid == TRUE ) //Restart Scan from last know good location
            {
                SendMessage(THIS_MACHINE, CircFrameCaptureDone); //Kick Start Search

                ClearSearchPositions();
                FillCircSearchPositions(lastCircFoundLocation, LIMITED_AREA);
                FillLatSearchPositions(lastLatFoundLocation);
            }
            else //Continue Search
            {
                PostMessageAndData(ImageProcessingHandler,ProcessImage, FAST_FRAME_ANALYSIS, 0);
                SendMessage(ImageAcquirerHandler, DoStartImageAcquisition);
            }

            lockedSearchMaxRetry = INIT_SEARCH_TO_LOCK_MAX_RETRY;

            lockedSearchRetryCount = 0;
            markFoundGoodCount = 0;

            markValid = FALSE;

            SetCurrentCameraMode( CAMERA_SEARCHING );

            //print("SCM C6 2 Single Frame: %d\n", singleLocationCirc.position);
            StartImageCaptureTimeOut();
            return SCM_LIM_WAIT_SEARCH_DONE;
        }
    }

    SendMessage(ImageAcquirerHandler, DoStartImageAcquisition);
    StartImageCaptureTimeOut();
    return SAME_STATE;
}

NEW_STATE   SCM_exitC7(void)
{
    // printf("\nTranLocation: %d",currentScannerPosition.lateralPosition);
    if ( (GetRequestedScannerScanMode() != GetCurrentScannerScanMode()) ||  (GetRequestedScannerScanMode() == SCANNER_ENABLED) )
    {
        CancelTimer();
        //   StartTimer(SECONDS(20));
        SendMessage(THIS_MACHINE, CheckForTransportReady);
        SendMessage(ImageAcquirerHandler, ImageAcquirerGoWaitActive);

        return SCM_WAIT_FOR_TRANSPORT_READY;
    }

    return SAME_STATE;
}

NEW_STATE   SCM_exitT1(void)
{
    //print("SCM exitT1 TimeOut\n");
    cancel_capture_rq();

    //   SendMessage(ImageAcquirerHandler, ImageAcquirerGoWaitActive);
    // LogString("Image Acquistion Time Out");
    SendMessage(ImageAcquirerHandler, DoStartImageAcquisition);
    StartImageCaptureTimeOut();
    return SAME_STATE;
}

NEW_STATE   SCM_exitT2(void)
{
    //print("SCM exitT2 TimeOut\n");
    //LogString("Scanner Manager Found Mark but Acquire was never ready\n");
    SendMessage(ImageAcquirerHandler, ImageAcquirerGoWaitActive);
    SendMessage(THIS_MACHINE, StartLimitedAlternatingSearch);

    return SCM_LIM_SETUP_SEARCH_PATTERN;
}


////////////////////////////////////////////////////////////////////
//
// StartFullAlternatingSearch
//      While in STARTING_FULL_ALTERNATING_SEARCH
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitD(void)
{
    //print("\nSTART FULL ALT\n");

    PrepareCameraSearchPositions(FULL_ALTERNATING_SEQUENCE);

    // TODO
    ShowAllCameraSearchPositions();

    SendMessage(THIS_MACHINE, CheckForRemainingSearch);

    //return SCM_LIM_CHECK_REMAINING_SEARCH;

    return SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
// StartFullLinearSearch
//      While in STARTING_FULL_LINEAR_SEARCH
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitE(void)
{
    //print("\nSTART FULL LINEAR\n");

    PrepareCameraSearchPositions(FULL_LINEAR_SEQUENCE);

    // TODO
    ShowAllCameraSearchPositions();

    SendMessage(THIS_MACHINE, CheckForRemainingSearch);

    // return SCM_LIM_CHECK_REMAINING_SEARCH;

    return SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
// StartFullSmartSearch
//      While in STARTING_FULL_SMART_SEARCH
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitF(void)
{
    //print("\nSTART FULL SMART\n");

    PrepareCameraSearchPositions(FULL_SMART_SEQUENCE);

    // TODO
    ShowAllCameraSearchPositions();

    SendMessage(THIS_MACHINE, CheckForRemainingSearch);

    //return SCM_LIM_CHECK_REMAINING_SEARCH;

    return SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
// StartFullCustomSearch
//      While in STARTING_FULL_CUSTOM_SEARCH
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitG(void)
{
    //print("\nSTART FULL CUSTOM\n");

    PrepareCameraSearchPositions(FULL_CUSTOM_SEQUENCE);

    return SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
// StartLockTarget
//      While in STARTING_LOCK_TARGET
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitH(void)
{
//    SetCurrentScannerScanMode(LOCK_TARGET);

    return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
// StartFreeRun
//      While in STARTING_FREE_RUN
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitI(void)
{
//    SetCurrentScannerScanMode(FREE_RUN);

    return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
// StartLevitate
//      While in STARTING_LEVITATE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitJ(void)
{
//    SetCurrentScannerScanMode(LEVITATE);
    SetCurrentCameraMode(CAMERA_PAUSED);
    ResetCameraSearchPosition();
    ClearAllPositions();
    ResetFrameBuffers();
    frameCaptureMode = FAST_FRAME_ANALYSIS;
    SendMessage(ImageAcquirerHandler, ImageAcquirerGoWaitActive);

    return SCM_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// StartManual
//      While in STARTING_MANUAL
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitK(void)
{
//    SetCurrentScannerScanMode(MANUAL);

    return SAME_STATE;
}





////////////////////////////////////////////////////////////////////
//
// StartScannerDisable
//      While in SCM_STARTING_SCANNER_ENABLE
//
////////////////////////////////////////////////////////////////////

//
//  
//
//  TODO        
//
//  IMAGE TRANSMITTER SETUP
//  ENCODER HANDLER SETUP
//

NEW_STATE   SCM_exitL1(void)
{
    //   print("SCANNER DISABLED\n");

    // Send Home position 0 + N steps

    ResetCameraSearchPosition();
    ClearAllPositions();
    ResetFrameBuffers();
    frameCaptureMode = FAST_FRAME_ANALYSIS;
    SendMessage(ImageAcquirerHandler, ImageAcquirerGoWaitActive);

    //       calcTransportStepDirection( 0 );
    SendMessage(TransportManager, FindHomeTransport);
    SetCurrentCameraMode( CAMERA_AT_HOME );

    return SCM_DISABLED_SETTING_TRANSPORT_HOME;
}

////////////////////////////////////////////////////////////////////
//
// TransportPostionSet
//      While in SCM_DISABLED_SETTING_TRANSPORT_HOME
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitL2(void)
{
    return  SCM_ACTIVE;
}


////////////////////////////////////////////////////////////////////
//
// TransporMoveDone
//      While in SCM_DISABLED_MOVING_TRANSPORT
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitL3(void)
{
    //print("SCM PUT IAH back to ACTIVE\n");

    SendMessage(ImageAcquirerHandler, ImageAcquirerGoWaitActive);

    return SCM_DISABED_PUTTING_IMAGE_ACQUISITION_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// ImageAcquirerBackInActive
//      While in SCM_DISABLED_MOVING_TRANSPORT
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SCM_exitL4(void)
{
    //print("SCM IS NOW DISABLED\n");

    return SCM_DISABLED;
}



////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_SCM_IDLE)
EV_HANDLER(GoActive, SCM_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_ACTIVE)
EV_HANDLER(TimeOut, SCM_exitA1),
EV_HANDLER(ScannerModeChangeRequested, SCM_exitB)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_SCM_DISABLED)
EV_HANDLER(ScannerModeChangeRequested, SCM_exitB)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_SCM_ENABLED_STARTING_SCANNER_ENABLE)
EV_HANDLER(StartScannerEnable, SCM_exitB1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_ENABLED_WAIT_FOR_IMAGE_ACQUISITION_READY)
EV_HANDLER(ImageAcquirerReady, SCM_exitB2),
EV_HANDLER(DoNextImageCaptureCycle, SCM_exitB2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_ENABLED_SETTING_TRANSPORT_POSITION)
EV_HANDLER(TransportMoveSent, SCM_exitB3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_ENABLED_MOVING_TRANSPORT)
EV_HANDLER(TransporMoveDone, SCM_exitB4),
EV_HANDLER(SkipToNextState, SCM_exitB4),
EV_HANDLER(TransportAtLimits, SCM_exitB11)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_ENABLED_SETTING_TRIGGERS)
EV_HANDLER(TriggerPositionSet, SCM_exitB5)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_ENABLED_WAIT_TRIGGER_PROCESS_BEGIN)
EV_HANDLER(TriggerProcessBegins, SCM_exitB6)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_ENABLED_WAIT_TAKE_IMAGE_DONE)
EV_HANDLER(NewAcquirerImageReady, SCM_exitB7),
EV_HANDLER(TimeOut, SCM_exitB7A)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_ENABLED_WAIT_IMAGE_PROCESSED)
EV_HANDLER(ImageProcessed, SCM_exitB8)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_ENABLED_WAIT_IMAGE_TRANSMITTED)
EV_HANDLER(ImageTransmitted, SCM_exitB9),
EV_HANDLER(SkipToNextState, SCM_exitB9)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_ENABLED_WAIT_IMAGE_TRANSMITTED_DELAY)
EV_HANDLER(TimeOut, SCM_exitB10)
STATE_TRANSITION_MATRIX_END;



//Limiited area search
STATE_TRANSITION_MATRIX(_SCM_LIM_SETUP_SEARCH_PATTERN)
EV_HANDLER(StartLimitedAlternatingSearch, SCM_exitC),
EV_HANDLER(ScannerModeChangeRequested, SCM_exitC7)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_LIM_WAIT_TRANSPORT_DONE)
EV_HANDLER(TransporMoveDone, SCM_exitC1),
EV_HANDLER(TransportAtLimits, SCM_exitC1A),
EV_HANDLER(ScannerModeChangeRequested, SCM_exitC7)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_LIM_WAIT_SEARCH_DONE)
EV_HANDLER(ImageProcessed, SCM_exitC2),
EV_HANDLER(CircFrameCaptureDone, SCM_exitC3),
EV_HANDLER(ScannerModeChangeRequested, SCM_exitC7),
EV_HANDLER(TimeOut, SCM_exitT1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_LIM_WAIT_IMAGE_PROCESS_DONE)
EV_HANDLER(ImageProcessed, SCM_exitC3A),
EV_HANDLER(ScannerModeChangeRequested, SCM_exitC7)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_LIM_WAIT_TRANSPORT_HOME)
EV_HANDLER(TransporMoveDone, SCM_exitC4),
EV_HANDLER(ScannerModeChangeRequested, SCM_exitC7)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_LIM_WAIT_ACQUIRER_READY)
EV_HANDLER(ImageAcquirerReady, SCM_exitC2A),
EV_HANDLER(ImageAcquirerNotReady, SCM_exitC2B),
EV_HANDLER(ScannerModeChangeRequested, SCM_exitC7),
EV_HANDLER(TimeOut, SCM_exitT2)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_SCM_LIM_WAIT_TRANSPORT_CENTERED)
EV_HANDLER(TransporMoveDone, SCM_exitC5),
EV_HANDLER(TransportAtLimits, SCM_exitC1A),
EV_HANDLER(ScannerModeChangeRequested, SCM_exitC7)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_SCM_LIM_WAIT_LOCK_SCAN_STATUS)
EV_HANDLER(ImageProcessed, SCM_exitC6),
EV_HANDLER(ScannerModeChangeRequested, SCM_exitC7),
EV_HANDLER(TimeOut, SCM_exitT1)
STATE_TRANSITION_MATRIX_END;
//End Limiited area search

STATE_TRANSITION_MATRIX(_SCM_WAIT_FOR_TRANSPORT_READY)
EV_HANDLER(CheckForTransportReady,  SCM_exitA1),
EV_HANDLER(TransportNeedsHome,      SCM_exitA2)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_SCM_STARTING_FULL_SMART_SEARCH)
EV_HANDLER(StartFullSmartSearch, SCM_exitF)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_SCM_STARTING_FULL_CUSTOM_SEARCH)
EV_HANDLER(StartFullCustomSearch, SCM_exitG)
STATE_TRANSITION_MATRIX_END;



STATE_TRANSITION_MATRIX(_SCM_STARTING_LOCK_TARGET)
EV_HANDLER(StartLockTarget, SCM_exitH)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_SCM_STARTING_FREE_RUN)
EV_HANDLER(StartFreeRun, SCM_exitI)  
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_STARTING_LEVITATE)
EV_HANDLER(ScannerModeChangeRequested, SCM_exitC7),
EV_HANDLER(StartLevitate, SCM_exitJ)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_SCM_STARTING_MANUAL)
EV_HANDLER(StartManual, SCM_exitK)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_SCM_DISABLED_STARTING_SCANNER_DISABLE)
EV_HANDLER(StartScannerDisable, SCM_exitL1),
EV_HANDLER(ScannerModeChangeRequested, SCM_exitC7)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_DISABLED_SETTING_TRANSPORT_HOME)
EV_HANDLER(ScannerModeChangeRequested, SCM_exitC7),
EV_HANDLER(TransporMoveDone, SCM_exitL2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_DISABLED_MOVING_TRANSPORT)
EV_HANDLER(TransporMoveDone, SCM_exitL3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_DISABED_PUT_IMAGE_ACQUISITION_ACTIVE)
EV_HANDLER(ImageAcquirerBackInActive, SCM_exitL4)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_STARTING_FULL_ALTERNATING_SEARCH)
EV_HANDLER(ImageAcquirerBackInActive, SCM_exitL4)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SCM_STARTING_FULL_LINEAR_SEARCH)
EV_HANDLER(ImageAcquirerBackInActive, SCM_exitL4)
STATE_TRANSITION_MATRIX_END;

// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(SCM_Main_Entry)

STATE(_SCM_IDLE)                            ,           
STATE(_SCM_ACTIVE)                          ,

STATE(_SCM_DISABLED)                        ,

STATE(_SCM_ENABLED_STARTING_SCANNER_ENABLE)             ,
STATE(_SCM_ENABLED_WAIT_FOR_IMAGE_ACQUISITION_READY)    ,
STATE(_SCM_ENABLED_SETTING_TRANSPORT_POSITION)          ,
STATE(_SCM_ENABLED_MOVING_TRANSPORT)                    ,
STATE(_SCM_ENABLED_SETTING_TRIGGERS)                    ,
STATE(_SCM_ENABLED_WAIT_TRIGGER_PROCESS_BEGIN)          ,
STATE(_SCM_ENABLED_WAIT_TAKE_IMAGE_DONE)                ,
STATE(_SCM_ENABLED_WAIT_IMAGE_PROCESSED)                ,
STATE(_SCM_ENABLED_WAIT_IMAGE_TRANSMITTED)              ,
STATE(_SCM_ENABLED_WAIT_IMAGE_TRANSMITTED_DELAY)        ,

STATE(_SCM_DISABLED_STARTING_SCANNER_DISABLE)       ,
STATE(_SCM_DISABLED_SETTING_TRANSPORT_HOME)         ,
STATE(_SCM_DISABLED_MOVING_TRANSPORT)               ,
STATE(_SCM_DISABED_PUT_IMAGE_ACQUISITION_ACTIVE)    ,


//Limited Area Search
STATE(_SCM_LIM_SETUP_SEARCH_PATTERN)     ,
STATE(_SCM_LIM_WAIT_TRANSPORT_DONE)      ,
STATE(_SCM_LIM_WAIT_SEARCH_DONE)         ,
STATE(_SCM_LIM_WAIT_IMAGE_PROCESS_DONE)  ,
STATE(_SCM_LIM_WAIT_TRANSPORT_HOME)      ,
STATE(_SCM_LIM_WAIT_ACQUIRER_READY)      ,
STATE(_SCM_LIM_WAIT_TRANSPORT_CENTERED)  ,
STATE(_SCM_LIM_WAIT_LOCK_SCAN_STATUS)    ,

STATE(_SCM_STARTING_FULL_ALTERNATING_SEARCH)    ,

STATE(_SCM_STARTING_FULL_LINEAR_SEARCH)         ,

STATE(_SCM_STARTING_FULL_SMART_SEARCH)          ,

STATE(_SCM_STARTING_FULL_CUSTOM_SEARCH)         ,

STATE(_SCM_STARTING_LOCK_TARGET)                ,

STATE(_SCM_STARTING_FREE_RUN)                   ,
STATE(_SCM_STARTING_LEVITATE)                   ,
STATE(_SCM_WAIT_FOR_TRANSPORT_READY)            ,

STATE(_SCM_STARTING_MANUAL)

SM_RESPONSE_END



////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////
void SetCurrentCameraMode(CAMERA_STATE curMode)
{
    static CAMERA_STATE lastMode = CAMERA_UNDEFINED;

    if ( lastMode != curMode )
    {
        lastMode = curMode;
        SendMessage(SystemHardwareMonitor, GetCamSysStatus);
    }

    currentCameraMode = curMode;
}

CAMERA_STATE     GetCurrentCameraMode()
{
    return currentCameraMode;
}


void
SetRequestedScannerScanMode(SCAN_REQUEST reqScanMode)
{
    requestedScannerScanMode = reqScanMode;
}

SCAN_REQUEST   GetRequestedScannerScanMode(void)
{
    return  requestedScannerScanMode;
}

void
SetCurrentScannerScanMode(SCAN_REQUEST currScanMode)
{
    currentScannerScanMode = currScanMode;
}

SCAN_REQUEST   GetCurrentScannerScanMode(void)
{
    return  currentScannerScanMode;
}



void    SetCameraLockedSearchMaxRetry(int lsmr)
{
    lockedSearchMaxRetry = lsmr;
}

int     GetCameraLockedSearchMaxRetry(void)
{
    return lockedSearchMaxRetry;
}


////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

void                
SetRequestedScannerStartPosition(SCANNER_POSITION reqScanStartPos)
{
    requestedScannerStartPosition = reqScanStartPos;
}

SCANNER_POSITION GetRequestedScannerStartPosition(void)
{
    return requestedScannerStartPosition;
}

void SetCurrentScannerStartPosition(SCANNER_POSITION currScanStartPos)
{
    currentScannerStartPosition = currScanStartPos;
}

SCANNER_POSITION   GetCurrentScannerStartPosition(void)
{
    return currentScannerStartPosition;
}

BOOL IsRequestedScannerStartPositionChanged(void)
{
    if ( (requestedScannerStartPosition.circumPosition != 
          currentScannerStartPosition.circumPosition) ||
         (requestedScannerStartPosition.lateralPosition != 
          currentScannerStartPosition.lateralPosition) )
        return TRUE;

    return FALSE;
}


////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

void    SetRequestedScannerTransmitType(DWORD scanImgTxType)
{
    requestedScannerTransmitType = scanImgTxType;
}

DWORD   GetRequestedScannerTransmitType(void)
{
    return requestedScannerTransmitType;
}

void    SetCurrentScannerTransmitType(DWORD scanImgTxType)
{
    currentScannerTransmitType = scanImgTxType;
}

DWORD   GetCurrentScannerTransmitType(void)
{
    return currentScannerTransmitType;
}


////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

void    
SetRequestedScannerImageAnalysis(SCANNER_IMAGE_ANALYSIS scanImgAnalysis)
{
    requestedScannerImageAnalysis = scanImgAnalysis;
}

SCANNER_IMAGE_ANALYSIS      GetRequestedScannerImageAnalysis(void)
{
    return requestedScannerImageAnalysis;
}

void    
SetCurrentScannerImageAnalysis(SCANNER_IMAGE_ANALYSIS scanImgAnalysis)
{
    currentScannerImageAnalysis = scanImgAnalysis;
}

SCANNER_IMAGE_ANALYSIS      GetCurrentScannerImageAnalysis(void)
{
    return currentScannerImageAnalysis;
}


//


void SetRequestedSimulationMode(SCANNER_SIMULATION_MODE turnOnSimulation)
{
    requestedSimulationMode = turnOnSimulation;
}

SCANNER_SIMULATION_MODE      GetRequestedSimulationMode(void)
{
    return requestedSimulationMode;
}

void SetCurrentSimulationMode(SCANNER_SIMULATION_MODE turnOnSimulation)
{
    currentSimulationMode = turnOnSimulation;
}

SCANNER_SIMULATION_MODE      GetCurrentSimulationMode(void)
{
    return currentSimulationMode;
}





