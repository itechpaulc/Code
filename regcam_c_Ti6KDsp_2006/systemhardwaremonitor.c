




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


#include "systemhardwaremonitor.h"

#include "tcpcomm.h"

#include "spicomm.h"

#include "imageacquirerhandler.h"

////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

DWORD   sysEncoderTicksPerAPulse;
DWORD   sysEncoderZCountImpression;
DWORD   sysCameraTemperatureCelcius;
DWORD   sysCameraVoltage;

////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////



// TODO

//
//
//
// Camera Temperature 10 bits, MSBit is the sign, each count is 0.25 Celcius
// Camera Voltage is 8 bits, each count is 0.01285 = Ref 3.3V/255 = Camera 24.0V/255 
//

typedef struct
{
    DWORD   encoderTicksPerAPulse;
    DWORD   encoderZCountImpression;
    DWORD   cameraTemperatureCelcius;
    DWORD   cameraVoltage;
    DWORD   cameraCurrentMode;
    DWORD   serverScanRequestMode;
    DWORD   transportStatus;
    DWORD   imageTXmode;
    DWORD   imageTXType;
    DWORD   simRequestMode;
    DWORD   ImageAnalysis;
    DWORD   Objectlabel;
    DWORD   tgtPositionX;
    DWORD   tgtPositionY;
    DWORD   replySeqenceNumber;
    DWORD   deviceRebooted;

} CAM_STATUS;

#define         CAM_STATUS_SIZE     (sizeof(CAM_STATUS))

CAM_STATUS      currentCameraStatus;


///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////
extern SCANNER_POSITION    GetRequestedScannerStartPosition(void);

////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////

void    Init_SystemHardwareMonitor(void)
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

#define     SIM_Z_COUNT_INCREMENTS      (11)
#define     SIM_TICKS_PER_A_PULSE       (137)
#define     ZERO_SPEED_DETECTOR_COUNT   (8)
#define     ZERO_SPEED                  (0xFFFFFFFF)

int         simFpmVariation = 0;

NEW_STATE   SHM_exitA(void)
{
    //print("SHM ACTIVE\n");

//        StartTimer(SECONDS(1));

    SetSysEncoderTicksPerAPulse(10000);
    SetSysEncoderZCountImpression(0);
    SetSysCameraTemperatureCelcius(0);
    SetSysCameraVoltage(0);

    return SHM_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// TimeOut
// WaitForServerMsgSlotAvailable
//      while in SHM_ACTIVE
//
////////////////////////////////////////////////////////////////////


NEW_STATE   SHM_exitB(void)
{
    BOOL checkPost;

    ITECH_TCP_MESSAGE camStatusItechTcpMessage;

    static BYTE impressionResetCounter = 0 ;

    static DWORD    lastImpressionCount;

    SCANNER_POSITION reqScanTgtPos = GetRequestedScannerStartPosition();



    // TODO
    // SPI COMM States

    if ( IsServerMessageSlotAvailable() )
    {
//         if(GetCurrentSimulationMode() == SIMULATION_RUN)
//         {
//             SetSysEncoderZCountImpression(GetSysEncoderZCountImpression() + SIM_Z_COUNT_INCREMENTS);
//
//             currentCameraStatus.encoderZCountImpression = GetSysEncoderZCountImpression();
//
//             if(++simFpmVariation > 7) simFpmVariation = 0;
//             SetSysEncoderTicksPerAPulse(SIM_TICKS_PER_A_PULSE + simFpmVariation);
//
//             currentCameraStatus.encoderTicksPerAPulse = GetSysEncoderTicksPerAPulse();
//         }
//         else
        {
            GetSPIDataWithRetries(GET_IMPRESSION_COUNT, &currentCameraStatus.encoderZCountImpression);
            GetSPIDataWithRetries(GET_CURRENT_ENCODER_PERIOD, &currentPeriod );
            currentCameraStatus.encoderTicksPerAPulse = currentPeriod;
        }

        if ( lastImpressionCount == currentCameraStatus.encoderZCountImpression )
        {
            if ( impressionResetCounter < ZERO_SPEED_DETECTOR_COUNT )
                ++impressionResetCounter;
            else
            {
                currentCameraStatus.encoderTicksPerAPulse = ZERO_SPEED;
            }
        }
        else
        {
            lastImpressionCount = currentCameraStatus.encoderZCountImpression;
            impressionResetCounter  =   0;
        }

        GetSPIDataWithRetries(GET_CAMERA_TEMPERATURE, &currentCameraStatus.cameraTemperatureCelcius);
        GetSPIDataWithRetries(GET_CAMERA_MAIN_POWER_LEVEL, &currentCameraStatus.cameraVoltage);

        currentCameraStatus.serverScanRequestMode   = GetRequestedScannerScanMode();
        currentCameraStatus.cameraCurrentMode       = GetCurrentCameraMode();
        currentCameraStatus.transportStatus         = GetTransportStatus();
        currentCameraStatus.imageTXmode             = GetRequestedScannerImageCollectMode();  
        currentCameraStatus.simRequestMode          = GetCurrentSimulationMode();
        currentCameraStatus.imageTXType             = GetRequestedScannerTransmitType();
        currentCameraStatus.ImageAnalysis           = GetCurrentScannerImageAnalysis();
        currentCameraStatus.Objectlabel             = GetLabelObjects();

        currentCameraStatus.tgtPositionX            = reqScanTgtPos.lateralPosition;
        currentCameraStatus.tgtPositionY            = reqScanTgtPos.circumPosition;
        currentCameraStatus.replySeqenceNumber      = GetMessageData1();
        currentCameraStatus.deviceRebooted          = deviceRebooted;

        //print("SEND CAMERA STATUS\n");

        InitBuildTcpPacket(&camStatusItechTcpMessage.header);
        SetTcpMessageId(&camStatusItechTcpMessage.header, ITECH_MSG_CamSysStatusReply);

        SetTcpParam1234(&camStatusItechTcpMessage.header, 0, 0, 0, currentCameraStatus.replySeqenceNumber);


        SetTcpData(&camStatusItechTcpMessage, (BYTE *)&currentCameraStatus, CAM_STATUS_SIZE);
        checkPost = PostServerItechMessage(&camStatusItechTcpMessage);

    //    print("System Status Reply:\n   CurrentMode: %d   Request: %d Period: %x\n",
    //          currentCameraStatus.cameraCurrentMode,
    //          currentCameraStatus.serverScanRequestMode,
    //          currentCameraStatus.encoderTicksPerAPulse);
    }
    else
    {
     //   print("No Slots Available\n       ");
        SendMessage(THIS_MACHINE, WaitForServerMsgSlotAvailable);
    }

    return SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
// GetVersion
//
////////////////////////////////////////////////////////////////////

// TODO STATE for spi comm

#define     GET_FPGA_VERSION( x )          (GetSPIDataWithRetries(GET_FPGA_VERSION,x))

DWORD fpgaVer;

NEW_STATE   SHM_exitC(void)
{
    BOOL checkPost;
    int  major, minor;
    DWORD   osVer;
    int   camID;

    ITECH_TCP_MESSAGE verReplyItechTcpMessage;

    GET_FPGA_VERSION(&fpgaVer);

    osVer   = getvar(VERSION);
    major   = osVer/100;
    minor   = osVer - (major * 100);

    if ( minor < 0 )
        minor = 0;

    //print("FPGA VER 0x%x\n", fpgaVer);

    //print("SEND VERSION\n");

    GetCameraID( &camID);

    InitBuildTcpPacket(&verReplyItechTcpMessage.header);
    SetTcpMessageId(&verReplyItechTcpMessage.header, ITECH_MSG_CameraVersionReply);
    SetTcpParam1234(&verReplyItechTcpMessage.header, fpgaVer, GET_FW_VERSION(), 
                   GET_OS_VERSION(major, minor), camID );

    verReplyItechTcpMessage.header.sequence = 0x00000000;

    checkPost = PostServerItechMessage(&verReplyItechTcpMessage);

    //print("POST MESSAGE: ITECH_MSG_CameraVersionReply %d\n", checkPost);

    //   StartTimer(MILLISECONDS(100));

    return SAME_STATE;
}



////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_SHM_IDLE)
EV_HANDLER(GoActive, SHM_exitA)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_SHM_ACTIVE)
EV_HANDLER(TimeOut, SHM_exitB),
EV_HANDLER(WaitForServerMsgSlotAvailable, SHM_exitB),
EV_HANDLER(GetCamSysStatus, SHM_exitB),
EV_HANDLER(GetVersion, SHM_exitC)
STATE_TRANSITION_MATRIX_END;


// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(SHM_Main_Entry)
STATE(_SHM_IDLE)            ,           
STATE(_SHM_ACTIVE)   
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////

DWORD   GetSysEncoderTicksPerAPulse(void)
{
    return sysEncoderTicksPerAPulse;
}

DWORD   GetSysEncoderZCountImpression(void)
{
    return sysEncoderZCountImpression;
}

DWORD   GetSysCameraTemperatureCelcius(void)
{
    return sysCameraTemperatureCelcius;
}

DWORD   GetSysCameraVoltage(void)
{
    return sysCameraVoltage;
}

//

void    SetSysEncoderTicksPerAPulse(DWORD ticksPerAPulse)
{
    sysEncoderTicksPerAPulse = ticksPerAPulse;
}

void    SetSysEncoderZCountImpression(DWORD zCountImpression)
{
    sysEncoderZCountImpression = zCountImpression;
}

void    SetSysCameraTemperatureCelcius(DWORD camTemp)
{
    sysCameraTemperatureCelcius = camTemp;
}

void    SetSysCameraVoltage(DWORD camVoltage)
{
    sysCameraVoltage = camVoltage;
}


