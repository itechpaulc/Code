




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


#include "ribboncameramanager.h"

#include "scannermanager.h"

#include "cameraconfig.h"

#include "tcpcomm.h"

#include "imagetransmitter.h"


////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////





///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////

void    Init_RibCameraManager(void)
{


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

NEW_STATE   RCM_exitA(void)
{   
   // print("RCM ACTIVE\n");

    InitializeSystemConfiguration();
    InitializeCameraPositionManager();

    StartTimer(MILLISECONDS(100));

    return RCM_DELAY_TO_ACTIVATE_SUBMACHINES;
}

////////////////////////////////////////////////////////////////////
//
// TimeOut while in RCM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   RCM_exitB(void)
{
    SendMessage(TransportManager, GoActive);
    SendMessage(SPI_Manager, GoActive);
    SendMessage(ServerCommunicationManager, GoActive);
    SendMessage(TransmitManager, GoActive);
    SendMessage(ServerMessageParser, GoActive);

    SendMessage(SystemHardwareMonitor, GoActive);

    StartTimer(SECONDS(1));

    return RCM_DELAY_TO_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// TimeOut while in RCM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   RCM_exitC(void)
{
    //TODO

    //SetRequestedScannerScanMode(LIMITED_ALTERNATING_SEARCH);
    //SendMessage(ScannerManager, SetScannerMode);

    //SetRequestedScannerScanMode(FULL_LINEAR_SEARCH);
    //SendMessage(ScannerManager, SetScanernMode);

    // TODO

    //SendMessage(ImageProcessingHandler, DoStartFastVisualScanJpeg);

    //SendMessage(ImageProcessingHandler, DoStartFastVisualScanRlc);

    //SendMessage(ImageProcessingHandler, DoStartFastVisualScanSubsamp2x);
    //SendMessage(ImageProcessingHandler, DoStartFastVisualScanSubsamp4x);


    SendMessage(THIS_MACHINE, SetSystemModeNormal);

    return RCM_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// SetSystemModeNormal while in RCM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   RCM_exitD(void)
{
    //print("RCM NORMAL\n");

    return RCM_SYS_NORMAL;
}


////////////////////////////////////////////////////////////////////
//
//  SetScannerMode while in RCM_SYS_NORMAL
//
////////////////////////////////////////////////////////////////////

NEW_STATE   RCM_exitE1(void)
{   
    int checkPost;
    ITECH_TCP_MESSAGE scanEnReplyItechTcpMessage;

    SCAN_REQUEST   requestedScanMode = GetMessageData1();

    //print("RCM SET SCANNER MODE\n");

    SetRequestedScannerScanMode(requestedScanMode);

    InitBuildTcpPacket(&scanEnReplyItechTcpMessage.header);
    SetTcpMessageId(&scanEnReplyItechTcpMessage.header, ITECH_MSG_ScannerModeReply);

    if ( requestedScanMode == SCANNER_ENABLED )
    {
        SetTcpParam1(&scanEnReplyItechTcpMessage.header, SCANNER_ENABLED);
    }
    else
    {
        SetTcpParam1(&scanEnReplyItechTcpMessage.header, SCANNER_DISABLED);
    }

    checkPost = PostServerItechMessage(&scanEnReplyItechTcpMessage);
    //print("POST MESSAGE: ITECH_MSG_ScannerModeReply %d\n", checkPost);

    SendMessage(ScannerManager, ScannerModeChangeRequested);

    return SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
//  SetImageCollectMode while in RCM_SYS_NORMAL
//
////////////////////////////////////////////////////////////////////

NEW_STATE   RCM_exitE2(void)
{
    int checkPost;
    ITECH_TCP_MESSAGE imageCollectModeReplyItechTcpMessage;

    SCANNER_IMAGE_COLLECT_MODE   requestedImageCollect = GetMessageData1();

    //print("RCM SET IMAGE COLLECT MODE: %d\n", requestedImageCollect);

    SetRequestedScannerImageCollectMode(requestedImageCollect);
 //   SendMessage(ImageTransmitter, CheckForNewImage);

    InitBuildTcpPacket(&imageCollectModeReplyItechTcpMessage.header);
    SetTcpMessageId(&imageCollectModeReplyItechTcpMessage.header, ITECH_MSG_ImageCollectReply);

    if ( requestedImageCollect == SCANNER_IMAGE_COLLECT_ENABLED )
    {
        SetTcpParam1(&imageCollectModeReplyItechTcpMessage.header, SCANNER_IMAGE_COLLECT_ENABLED);
    }
    else
    {
        SetTcpParam1(&imageCollectModeReplyItechTcpMessage.header, SCANNER_IMAGE_COLLECT_DISABLED);
    }

    checkPost = PostServerItechMessage(&imageCollectModeReplyItechTcpMessage);
    //print("POST MESSAGE: ITECH_MSG_ImageCollectReply %d\n", checkPost);

    return SAME_STATE;
}



////////////////////////////////////////////////////////////////////
//
//  SetXYPositions while in RCM_SYS_NORMAL
//
////////////////////////////////////////////////////////////////////

NEW_STATE   RCM_exitE3(void)
{
    DWORD   relativeTranspPosition = GetMessageData1();
    DWORD   absCameraTrigPosition = GetMessageData2();
    BYTE    transportStatus;

    SCANNER_POSITION scannerPos;

    scannerPos.lateralPosition = relativeTranspPosition;
    scannerPos.circumPosition = absCameraTrigPosition;

    SetRequestedScannerStartPosition(scannerPos);

    //print("RCM SET X Y POSITIONS +++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

  //  print("RCM SETTING LAT:%d(0x%x)  CAM:%d(0x%x) \n", 
  //        scannerPos.lateralPosition, scannerPos.lateralPosition,
  //        scannerPos.circumPosition, scannerPos.circumPosition);

    return SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
//  SetImageTransmitType while in RCM_SYS_NORMAL
//
////////////////////////////////////////////////////////////////////

NEW_STATE   RCM_exitE4(void)
{
    DWORD   imageTxType = GetMessageData1();

    SetRequestedScannerTransmitType(imageTxType);
    //print("RCM REQUESTED IMAGE Tx Type\n");

    return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
//  SetImageAnalysis while in RCM_SYS_NORMAL
//
////////////////////////////////////////////////////////////////////

NEW_STATE   RCM_exitE5(void)
{
    SCANNER_IMAGE_ANALYSIS  imageAnalysis = 
    (SCANNER_IMAGE_ANALYSIS)GetMessageData1();

    BOOL    simMode = GetMessageData2();
    SetCurrentScannerImageAnalysis(imageAnalysis);
    //SetRequestedScannerImageAnalysis(imageAnalysis);
    //  SetRequestedSimulationMode(simMode);
    SetCurrentSimulationMode(simMode);

    //print("RCM SET IMAGE Analysis and Sim Mode\n");

    return SAME_STATE;
}



////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_RCM_IDLE)
EV_HANDLER(GoActive, RCM_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_RCM_DELAY_TO_ACTIVATE_SUBMACHINES)
EV_HANDLER(TimeOut, RCM_exitB)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_RCM_DELAY_TO_ACTIVE)
EV_HANDLER(TimeOut, RCM_exitC)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_RCM_ACTIVE)
EV_HANDLER(SetSystemModeNormal, RCM_exitD)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_RCM_SYS_NORMAL)
EV_HANDLER(SetScannerMode, RCM_exitE1),
EV_HANDLER(SetImageCollectMode, RCM_exitE2),
EV_HANDLER(SetXYPositions, RCM_exitE3),
EV_HANDLER(SetImageTransmitType, RCM_exitE4),
EV_HANDLER(SetImageAnalysis, RCM_exitE5)
STATE_TRANSITION_MATRIX_END;

// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(RCM_Main_Entry)
STATE(_RCM_IDLE)                            ,           
STATE(_RCM_DELAY_TO_ACTIVATE_SUBMACHINES)   ,
STATE(_RCM_DELAY_TO_ACTIVE)                 ,
STATE(_RCM_ACTIVE)                          ,
STATE(_RCM_SYS_NORMAL)
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////


