




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


#include "transportmanager.h"


#include "scannermanager.h"

#include "spicomm.h"

#include "cameraconfig.h"

////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////

TRANSPORT_MOVE_PARAM transMoveParam;

BOOL    transportReady;
BOOL    transportNeedsHome;
TRANSPORT_CURRENT_STATE   currentTransportStatus;

extern SCANNER_POSITION    currentScannerPosition;


///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////
void MakeTransportPositionPositive( I32 *position )
{
    if ( *position < 0 )
        *position *= -1;
}
BOOL AtHomeLimit( DWORD   transportStatus )
{
    DWORD homeDir = currentSystemConfiguration.cameraImaging.homeDirection;;

    if ( homeDir == MOVE_TO_LEFT )
        return AT_LEFT_LIMIT( transportStatus );
    else
        return AT_RIGTH_LIMIT( transportStatus );
}


void SetTransportStatus( TRANSPORT_CURRENT_STATE curState )
{
    static TRANSPORT_CURRENT_STATE lastState;

    if ( lastState != curState )
    {
        lastState = curState;
        SendMessage(SystemHardwareMonitor, GetCamSysStatus);
    }
    currentTransportStatus = curState;
}

TRANSPORT_CURRENT_STATE GetTransportStatus( void )
{
    return currentTransportStatus;
}


////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////

void    Init_TransportManager( void )
{
    //print("\n ******************Init_TransportManager\n");
    SPI_DISABLE_TRANSPORT();
    SPI_ENABLE_TRANSPORT();
    SPI_SET_TRANSPORT_LOCATION(0);
    SPI_SET_TRANSPORT_START_SPEED( START_SPEED );
    SPI_SET_TRANSPORT_MAX_SPEED( MAX_SPEED );
    SPI_SET_TRANSPORT_ACCEL_FREQ(ACCEL_FREQ);
    SPI_SET_TRANSPORT_ACCELERATION(ACCELERATION);

    transportReady = FALSE;
}

////////////////////////////////////////////////////////////////////
//
// Exit Procedures
//
////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////
//
// Idle
//
////////////////////////////////////////////////////////////////////

NEW_STATE   TRM_exitA(void)
{
    //FILE *fp;

    //fp = io_fopen("fd:/notran.001","r");
    //if ( fp )
    {
        //print("********************** TRM_ExitA - No Transport\n");
  //      io_fclose(fp);

        SPI_DISABLE_TRANSPORT();

        SetTransportStatus( TRANSPORT_DISABLED );
        transportNeedsHome = FALSE;
        transportReady = TRUE;

        return TRM_N0_TRANSPORT;
    }

    //print("********************** TRM_ExitA - With Transport\n");

    //io_fclose(fp);

    //transportNeedsHome = TRUE;

   // return TRM_NEEDS_HOME;
}

NEW_STATE   TRM_exitA1(void)
{
    //print("********************** TRM_ExitA1 \n");
    transportReady = FALSE;
    transportNeedsHome = FALSE;
    SetTransportStatus( TRANSPORT_MOVING );

    SetMoveParameters( TRUE,  FIND_HOME_START_LOC);
    SendMessage(SPI_Manager, SetupTransportMove);

    return TRM_FH_SENDING_FH_MOVE_PROFILE;
}

NEW_STATE   TRM_exitA1A(void)
{
    transportReady = TRUE;
    return SAME_STATE;
}

NEW_STATE   TRM_exitA2(void)
{
    SendMessage(GetSmSourceId(), TransportNeedsHome); 
    return SAME_STATE;
}


NEW_STATE   TRM_exitC(void)
{
    //print("\n********************** TRM_Exitc");
    return TRM_IDLE;
}

////////////////////////////////////////////////////////////////////
//
//  Send  Move profile to SPI_Manager
//
////////////////////////////////////////////////////////////////////

NEW_STATE   TRM_exitB(void)
{
    BOOL   towardsHome   =   GetMessageData1();
    DWORD   stepCount   =   GetMessageData2();

    //print("***** TRM_ExitB\n");
    transportReady = FALSE;
    SetTransportStatus( TRANSPORT_MOVING );

    SetMoveParameters( towardsHome,  stepCount);

    SendMessage(SPI_Manager, SetupTransportMove);

    return TRM_SENDING_MOVE_PROFILE;
}

////////////////////////////////////////////////////////////////////
//
//  Move profile Sent
//
////////////////////////////////////////////////////////////////////

NEW_STATE   TRM_exitB1(void)
{
    //print("***** TRM_ExitB1\n");
    SendMessage(SPI_Manager, GetTransporStatus);

    return TRM_TRANSPORT_MOVING;
}

////////////////////////////////////////////////////////////////////
//
// MoveTransport
//
////////////////////////////////////////////////////////////////////

NEW_STATE   TRM_exitB2(void)
{
    DWORD   transportStatus = GetMessageData1();

    //print("\n********************** TRM_ExitB2");
    if ( AT_LIMITS( transportStatus ) )
    {
      //  LogString("Transport at Limit while moving\n");
        SendMessage(ScannerManager, TransportAtLimits);

        transportNeedsHome = TRUE;
        return TRM_NEEDS_HOME;
    }

    if ( IS_TRANSPORT_MOVING( transportStatus ) )
    {
        SendMessage(SPI_Manager, GetTransporStatus);
        return SAME_STATE;
    }

    SendMessage(ScannerManager, TransporMoveDone);

    GetSPIDataWithRetries(GET_TRANSPORT_POSITION, &currentScannerPosition.lateralPosition);

    MakeTransportPositionPositive( &currentScannerPosition.lateralPosition );

    transportReady = TRUE;
    SetTransportStatus( TRANSPORT_IDLE );
    return TRM_READY;
}

////////////////////////////////////////////////////////////////////
//
// Wait for Move profile send to be complete
//
////////////////////////////////////////////////////////////////////

NEW_STATE   TRM_exitC2(void)
{
    //print("***** TRM_Exitc2\n");
    SendMessage(SPI_Manager, GetTransporStatus);

    return TRM_FH_WAIT_FOR_LIMIT;
}

////////////////////////////////////////////////////////////////////
//
// Waiting for limit
//
////////////////////////////////////////////////////////////////////

NEW_STATE   TRM_exitC3(void)
{
    DWORD   transportStatus = GetMessageData1();

    if ( AtHomeLimit( transportStatus ) )
    {
        //print("***** TRM_Exitc3 - Found Home Limit\n");
        SetMoveParameters( FALSE,  SINGLE_STEP);

        SendMessage(SPI_Manager, SetupTransportMove);

        return TRM_FH_SENDING_SS_MOVE_PROFILE;
    }
    else if ( !IS_TRANSPORT_MOVING( transportStatus ) )
    {
        //print("***** TRM_Exitc3 -Stopped Moving and Not At Limit: %x\n", transportStatus);
        SetMoveParameters( TRUE,  FIND_HOME_START_LOC);
        SendMessage(SPI_Manager, SetupTransportMove);
        return TRM_FH_SENDING_FH_MOVE_PROFILE;
    }

    SendMessage(SPI_Manager, GetTransporStatus);

    return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
// Waiting for move command to be issued
//
////////////////////////////////////////////////////////////////////

NEW_STATE   TRM_exitC4(void)
{
    //print("***** TRM_Exitc4\n");
    SendMessage(SPI_Manager, GetTransporStatus);

    return TRM_FH_TRANSPORT_SS_MOVING;
}

////////////////////////////////////////////////////////////////////
//
// Waiting for single step to complete
//
////////////////////////////////////////////////////////////////////

NEW_STATE   TRM_exitC5(void)
{
    DWORD   transportStatus = GetMessageData1();
    DWORD homeDirection = currentSystemConfiguration.cameraImaging.homeDirection;
    DWORD transportMoveDirection = MOVE_TO_LEFT;

    static  BYTE  invalidMsgCount = 0;

    if ( CHECK_TRANSPORT_DIRECTION( transportStatus ) )
        transportMoveDirection = MOVE_TO_RIGHT;

    if ( IS_TRANSPORT_MOVING( transportStatus ) )
    {
        SendMessage(SPI_Manager, GetTransporStatus);
        return SAME_STATE;
    }

    if ( CHECK_TRANSPORT_MOVING_FLAG( transportStatus ) )
        invalidMsgCount++;
    else if ( homeDirection == transportMoveDirection )
        invalidMsgCount++;
    else
        invalidMsgCount = 0;

    if ( invalidMsgCount == 20)
    {
        invalidMsgCount = 0;
        //printf("Invalid Transport Status\n");
        SendMessageAndData(WatchDog_Manager, ResetWDT,FALSE,0);
    }

    //print("***** TRM_Exitc5 - Stopped: 0x%x, 0x%x\n",transportStatus, IS_TRANSPORT_MOVING( transportStatus ));
    StartTimer(MILLISECONDS(45));

    return TRM_FH_LIMIT_SETTLE_DELAY;
}

////////////////////////////////////////////////////////////////////
//
// TimeOut
// While in _TRM_FH_LIMIT_SETTLE_DELAY 
//
////////////////////////////////////////////////////////////////////

NEW_STATE   TRM_exitC10(void)
{
    SendMessage(SPI_Manager, GetTransporStatus);
    return TRM_FH_WAITING_FOR_LIMIT_BACKOFF;
}

////////////////////////////////////////////////////////////////////
//
// Check if at a limit
//
////////////////////////////////////////////////////////////////////

NEW_STATE   TRM_exitC6(void)
{
    DWORD   transportStatus = GetMessageData1();
    //  static  BYTE    backoffValidation = 0;


    //print("***** TRM_Exitc6: %x",transportStatus);
    if ( AtHomeLimit( transportStatus ) )
    {
        SetMoveParameters( FALSE,  SINGLE_STEP);

        SendMessage(SPI_Manager, SetupTransportMove);

        return TRM_FH_SENDING_SS_MOVE_PROFILE;
    }
    else
    {
        SetMoveParameters( FALSE,  BACKOFF_STEPS);

        SendMessage(SPI_Manager, SetupTransportMove);

        return TRM_FH_SENDING_BACKOFF_MOVE_PROFILE;
    }
}

////////////////////////////////////////////////////////////////////
//
// Sending BackOff Move Profile
//
////////////////////////////////////////////////////////////////////

NEW_STATE   TRM_exitC7(void)
{
    //print("\n********************** TRM_Exitc7");
    SendMessage(SPI_Manager, GetTransporStatus);

    return TRM_FH_TRANSPORT_BACKOFF_MOVING;
}

////////////////////////////////////////////////////////////////////
//
// Waiting for backOff move complete
//
////////////////////////////////////////////////////////////////////

NEW_STATE   TRM_exitC8(void)
{
    DWORD   transportStatus = GetMessageData1();

    if ( AT_LIMITS( transportStatus ) )
    {
       // LogString("Transport at Limit while backing off");
        SPI_DISABLE_TRANSPORT();
        SPI_ENABLE_TRANSPORT(); 

        SendMessage(THIS_MACHINE, FindHomeTransport);

        return TRM_NEEDS_HOME;
    }

    if ( IS_TRANSPORT_MOVING( transportStatus ) )
    {
        SendMessage(SPI_Manager, GetTransporStatus);

        return SAME_STATE;
    }
    else
    {
        //print("\n********************** TRM_Exitc8 - Backoff Complete");
        SendMessageAndData(SPI_Manager, SetTranLocation,0,0);
        return TRM_FH_ZEROING_HOME_POSITION;
    }
}

////////////////////////////////////////////////////////////////////
//
// Reseting Home Position
//
////////////////////////////////////////////////////////////////////

NEW_STATE   TRM_exitC9(void)
{
    // print("\n********************** TRM_Exitc9");
    SendMessage(ScannerManager, TransporMoveDone);
    GetSPIDataWithRetries(GET_TRANSPORT_POSITION, &currentScannerPosition.lateralPosition);
    MakeTransportPositionPositive( &currentScannerPosition.lateralPosition);

    transportNeedsHome = FALSE;
    transportReady = TRUE;
    SetTransportStatus( TRANSPORT_IDLE );

    return TRM_READY;
}

NEW_STATE   TRM_exitZ(void)
{
    //print("\n********************** TRM_ExitZ\n");
    SendMessage(ScannerManager, TransporMoveDone);
    // GetSPIDataWithRetries(GET_TRANSPORT_POSITION, &currentScannerPosition.lateralPosition);

    transportReady = TRUE;

    return SAME_STATE;
}


// TODO
// Check Move Done States
// Check Limit Switches
// Moving States
// Error In Limit States
// Finding Home States
//

////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////
STATE_TRANSITION_MATRIX(_TRM_IDLE)
EV_HANDLER(GoActive, TRM_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRM_NEEDS_HOME)
EV_HANDLER(FindHomeTransport, TRM_exitA1),
EV_HANDLER(IsTransportReady, TRM_exitA2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRM_READY)
EV_HANDLER(MoveTransport, TRM_exitB),
EV_HANDLER(FindHomeTransport, TRM_exitA1),
EV_HANDLER(IsTransportReady, TRM_exitA1A)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRM_SENDING_MOVE_PROFILE)
EV_HANDLER(TransportMoveSent, TRM_exitB1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRM_TRANSPORT_MOVING)
EV_HANDLER(TransportStatus, TRM_exitB2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRM_FH_SENDING_FH_MOVE_PROFILE)
EV_HANDLER(TransportMoveSent, TRM_exitC2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRM_FH_WAIT_FOR_LIMIT)
EV_HANDLER(TransportStatus, TRM_exitC3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRM_FH_SENDING_SS_MOVE_PROFILE)
EV_HANDLER(TransportMoveSent, TRM_exitC4)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRM_FH_TRANSPORT_SS_MOVING)
EV_HANDLER(TransportStatus, TRM_exitC5)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRM_FH_LIMIT_SETTLE_DELAY)
EV_HANDLER(TimeOut, TRM_exitC10)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRM_FH_WAITING_FOR_LIMIT_BACKOFF)
EV_HANDLER(TransportStatus, TRM_exitC6)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRM_FH_SENDING_BACKOFF_MOVE_PROFILE)
EV_HANDLER(TransportMoveSent, TRM_exitC7)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRM_FH_TRANSPORT_BACKOFF_MOVING)
EV_HANDLER(TransportStatus, TRM_exitC8)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRM_FH_ZEROING_HOME_POSITION)
EV_HANDLER(TransportLocationSet, TRM_exitC9)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRM_N0_TRANSPORT)
EV_HANDLER(MoveTransport, TRM_exitZ),
EV_HANDLER(FindHomeTransport, TRM_exitZ)
STATE_TRANSITION_MATRIX_END;

// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(TRM_Main_Entry)
STATE(_TRM_IDLE),           
STATE(_TRM_NEEDS_HOME),   
STATE(_TRM_READY),   
STATE(_TRM_FH_SENDING_FH_MOVE_PROFILE),   
STATE(_TRM_FH_WAIT_FOR_LIMIT),   
STATE(_TRM_FH_SENDING_SS_MOVE_PROFILE),   
STATE(_TRM_FH_TRANSPORT_SS_MOVING),   
STATE(_TRM_FH_LIMIT_SETTLE_DELAY),   
STATE(_TRM_FH_WAITING_FOR_LIMIT_BACKOFF),   
STATE(_TRM_FH_SENDING_BACKOFF_MOVE_PROFILE),   
STATE(_TRM_FH_TRANSPORT_BACKOFF_MOVING),   
STATE(_TRM_FH_ZEROING_HOME_POSITION),   
STATE(_TRM_SENDING_MOVE_PROFILE),   
STATE(_TRM_TRANSPORT_MOVING),   
STATE(_TRM_N0_TRANSPORT)   
SM_RESPONSE_END
////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////
void SetMoveParameters( BOOL towardsHome, DWORD steps )
{
    DWORD accelTarget, decelTarget;
    DWORD homeDirection = currentSystemConfiguration.cameraImaging.homeDirection;
    DWORD moveStepsAndDirection = 0x00000000;

    int numberOfAccelerations   = (START_SPEED-MAX_SPEED) / ACCELERATION; //Number of changes to reach max speed.
    int numberOfStepsForMax     = ACCEL_FREQ * numberOfAccelerations; //Total Number of Steps to reach Max speed


    if ( towardsHome )
        moveStepsAndDirection       =    homeDirection;
    else
        moveStepsAndDirection       =   (~homeDirection & 0x80000000); 

    //printf("SetMoveParmeter:  Move: %x\n", moveStepsAndDirection);

    moveStepsAndDirection |= (steps & STEP_COUNT_MASK);

    //printf("SetMoveParmeter: TH: %d Move: %x\n", towardsHome, moveStepsAndDirection);

    if ( abs(steps) > (numberOfStepsForMax * 2) )
    {
        accelTarget     = numberOfStepsForMax;
        decelTarget     = steps - numberOfStepsForMax;
    }
    else
    {
        if ( abs(steps) > 3 )
        {
            accelTarget     = (steps/2) - 1 ;
            decelTarget     = (steps/2) + 1;
        }
        else
            //accelTarget = decelTarget = currentLocation + steps/2;
            accelTarget = decelTarget = steps;
    }

    transMoveParam.accelTarget = accelTarget;
    transMoveParam.decelTarget = decelTarget;
    transMoveParam.moveTarget  = moveStepsAndDirection;
}











