

/////////////////////////////////////////////////////////////////////////////
//
//
//    $Header:      $
//    $Log:         $
//
//
//    Author : Paul Calinawan        January 1998
//
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//    NOTE:
//
//    This document contains CONFIDENTIAL and proprietary information
//    which is the property of Graphics Microsystems, Inc. It may not
//    be copied or transmitted in whole or in part by any means to any
//    media without Graphics Microsystems Inc's prior written permission.
//
/////////////////////////////////////////////////////////////////////////////



#include "80386ex.h"

#include "mtrcomm.h"




//
// Used to force the motor to run into its limit
// switches during position calibration
//

#define     X_NEGATIVE_LIMIT   (-1000000000)
#define     Y_NEGATIVE_LIMIT   (-1000000000)


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  MotorCommMachine
//
//      - public interface functions :
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

void
MotorCommMachine::SetTargetCoordinates(int targetX, int targetY) {

    SetXTargetCoordinate(targetX);
    SetYTargetCoordinate(targetY);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MotorCommMachine::SetXTargetCoordinate(int targetX)
{
    if((targetX <= ptrTPDM->GetMaxTablePosition().GetXCoordinate())
        && (targetX >= ptrTPDM->GetMinTablePosition().GetXCoordinate())
        || (targetX == X_NEGATIVE_LIMIT))
    {
        targetXCoordinate = targetX;
    }
    else // Error Conditions
    if(targetX > ptrTPDM->GetMaxTablePosition().GetXCoordinate())
    {
        // Reset target to max, log error

        targetXCoordinate = ptrTPDM->GetMaxTablePosition().GetXCoordinate();
        ++errorCount;
    }
    else
    {
        // Reset target to min, log error

        targetXCoordinate = ptrTPDM->GetMinTablePosition().GetXCoordinate();
        ++errorCount;
    }
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MotorCommMachine::SetYTargetCoordinate(int targetY)
{
    if((targetY <= ptrTPDM->GetMaxTablePosition().GetYCoordinate())
        && (targetY >= ptrTPDM->GetMinTablePosition().GetYCoordinate())
        || (targetY == Y_NEGATIVE_LIMIT))
    {
        targetYCoordinate = targetY;
    }
    else // Error Condition
    if(targetY > ptrTPDM->GetMaxTablePosition().GetYCoordinate())
    {
        // Reset target to max, log error

        targetYCoordinate = ptrTPDM->GetMaxTablePosition().GetYCoordinate();
        ++errorCount;
    }
    else
    {
        // Reset target to min, log error

        targetYCoordinate = ptrTPDM->GetMinTablePosition().GetYCoordinate();
        ++errorCount;
    }
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MotorCommMachine::GetCurrCoordinates(int &currX, int &currY) {

    currX = currXCoordinate;
    currY = currYCoordinate;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

int
MotorCommMachine::GetCurrXCoordinate(void) {

    return currXCoordinate;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

int
MotorCommMachine::GetCurrYCoordinate(void) {

    return currYCoordinate;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL
MotorCommMachine::IsXAxisOnTarget(void) {

    return  (targetXCoordinate == currXCoordinate) ?
        TRUE : FALSE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL
MotorCommMachine::IsYAxisOnTarget(void) {

    return  (targetYCoordinate == currYCoordinate) ?
        TRUE : FALSE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL
MotorCommMachine::IsXAxisOnIntermediateTarget(void) {

    return  (intermediateXCoordinate == currXCoordinate) ?
        TRUE : FALSE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL
MotorCommMachine::IsYAxisOnIntermediateTarget(void) {

    return  (intermediateYCoordinate == currYCoordinate) ?
        TRUE : FALSE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MotorCommMachine::EnableBacklashCompensation(void) {

    backlashCompensationEnabled = TRUE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MotorCommMachine::DisableBacklashCompensation(void) {

    backlashCompensationEnabled = FALSE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL
MotorCommMachine::IsBacklashCompensationEnabled(void) {

    return backlashCompensationEnabled;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MotorCommMachine::SetCurrXMoveProfile(MoveProfile &newMoveProf) {

    SendMcsCommand(SET_CURR_AXIS_1); // Set Axis to X

    // Send Changes only

    // Check Starting Velocity

    if(currXMoveProfile.startingVelocity != newMoveProf.startingVelocity)
        SendMcsDWordCommand(SET_START_VELOCITY, newMoveProf.startingVelocity);

    // Check Max Velocity

    if(currXMoveProfile.maxVelocity != newMoveProf.maxVelocity)
        SendMcsWordCommand(SET_MAX_VELOCITY, newMoveProf.maxVelocity);

    // Check Acceleration

    if(currXMoveProfile.acceleration != newMoveProf.acceleration)
        SendMcsDWordCommand(SET_ACCELERATION, newMoveProf.acceleration);

    SendMcsCommand(UPDATE_PARAM);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

MoveProfile &
MotorCommMachine::GetCurrXMoveProfile(void) {

    return currXMoveProfile;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MotorCommMachine::SetCurrYMoveProfile(MoveProfile &newMoveProf) {

    SendMcsCommand(SET_CURR_AXIS_2); // Set Axis to Y

    // Send Changes only

    // Check Starting Velocity

    if(currYMoveProfile.startingVelocity != newMoveProf.startingVelocity)
        SendMcsDWordCommand(SET_START_VELOCITY, newMoveProf.startingVelocity);

    // Check Max Velocity

    if(currYMoveProfile.maxVelocity != newMoveProf.maxVelocity)
        SendMcsWordCommand(SET_MAX_VELOCITY, newMoveProf.maxVelocity);

    // Check Acceleration

    if(currYMoveProfile.acceleration != newMoveProf.acceleration)
        SendMcsDWordCommand(SET_ACCELERATION, newMoveProf.acceleration);

    SendMcsCommand(UPDATE_PARAM);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

MoveProfile &
MotorCommMachine::GetCurrYMoveProfile(void) {

    return currXMoveProfile;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MotorCommMachine::LinkTableParametersDataManager(TableParametersDataManager *pTPDM) {

    ptrTPDM = pTPDM;
}


void
MotorCommMachine::LinkXAxisTimer(XAxisTimer *pXAT) {

    ptrXAT = pXAT;
}

void
MotorCommMachine::LinkYAxisTimer(YAxisTimer *pYAT) {

    ptrYAT = pYAT;
}



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// MotorCommMachine - private helper functions
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
//  MotorCommMachine -
//
//  Motor Chip Set functions
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
// Port Defines
//
//////////////////////////////////////////////////

#define     DATA_WRITE_PORT     0x300

#define     COMMAND_PORT        0x301

#define     DATA_READ_PORT      0x302


//////////////////////////////////////////////////
//
// Command with NO Parameters
//
//////////////////////////////////////////////////

void    SendMcsCommand(BYTE cmd) {

    while(!IsMotorChipSetReady())
        ;

    WriteToIO(COMMAND_PORT, cmd);
}


//////////////////////////////////////////////////
//
// Command with a 1 WORD Parameter
// (High Byte, Low Byte)
//
//////////////////////////////////////////////////

void    SendMcsWordCommand(BYTE cmd, WORD data) {

    while(!IsMotorChipSetReady())
        ;

    WriteToIO(COMMAND_PORT, cmd);
    WriteToIO(COMMAND_PORT, HIBYTE(data));
    WriteToIO(COMMAND_PORT, LOBYTE(data));
}


//////////////////////////////////////////////////
//
// Command with a 2 WORD Parameter
// (Data1 High Word, Data2 Low Word)
//
//////////////////////////////////////////////////

void    SendMcsDWordCommand(BYTE cmd, DWORD data1) {

    while(!IsMotorChipSetReady())
        ;

    WriteToIO(COMMAND_PORT, cmd);

    WriteToIO(COMMAND_PORT, HIBYTE(HIWORD(data1)));
    WriteToIO(COMMAND_PORT, LOBYTE(HIWORD(data1)));
    WriteToIO(COMMAND_PORT, HIBYTE(LOWORD(data1)));
    WriteToIO(COMMAND_PORT, LOBYTE(LOWORD(data1)));
}


//////////////////////////////////////////////////
//
// Read Hi byte then Low byte
// Motor Chip Set Data
//
//////////////////////////////////////////////////

WORD    ReadMcs(void) {

    WORD    motorChipSetData;

        motorChipSetData = WORD(ReadFromIO(DATA_READ_PORT) << 8);
        motorChipSetData |= WORD(ReadFromIO(DATA_READ_PORT));

    return  motorChipSetData;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL    IsMotorChipSetReady(void) {

    return ((GetIO1Latch() & HOST_READY_PORT) == CLEAR) ?
        TRUE : FALSE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    SendMcsMoveCommand(AxisId axisId, int targetDestination) {

    if(axisId == X_AXIS)
        SendMcsCommand(SET_CURR_AXIS_1); // Set Axis to X
    else
        SendMcsCommand(SET_CURR_AXIS_2); // Set Axis to Y

    SendMcsWordCommand(SET_INTERRUPT_MASK, WORD(MOTION_COMPLETE));
    SendMcsDWordCommand(SET_POSITION, targetDestination);
    SendMcsCommand(UPDATE_PARAM);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    SendMcsChangeProfileCommand(AxisId axisId, MoveProfile & moveProf) {

    if(axisId == X_AXIS)
        SendMcsCommand(SET_CURR_AXIS_1); // Set Axis to X
    else
        SendMcsCommand(SET_CURR_AXIS_2); // Set Axis to Y

    SendMcsWordCommand(SET_START_VELOCITY, moveProf.startingVelocity);
    SendMcsWordCommand(SET_MAX_VELOCITY, moveProf.maxVelocity);
    SendMcsWordCommand(SET_ACCELERATION, moveProf.acceleration);

    SendMcsCommand(UPDATE_PARAM);
}


//////////////////////////////////////////////////
//
// MotorCommMachine - RESPONSE ENTRIES
//
//////////////////////////////////////////////////



//////////////////////////////////////////////////
//
// State Transition Matrices
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(MotorCommMachine, _MTRC_IDLE)
    EV_HANDLER(GotoXYTarget, MTRC_h1),
    EV_HANDLER(FindXLimit, MTRC_h6),
    EV_HANDLER(FindYLimit, MTRC_h8)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(MotorCommMachine, _MTRC_WAIT_ANY_AXIS_ON_TARGET)
    EV_HANDLER(XAxisOnTarget, MTRC_h2),
    EV_HANDLER(YAxisOnTarget, MTRC_h3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(MotorCommMachine, _MTRC_WAIT_X_AXIS_ON_TARGET)
    EV_HANDLER(XAxisOnTarget, MTRC_h5)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(MotorCommMachine, _MTRC_WAIT_Y_AXIS_ON_TARGET)
    EV_HANDLER(YAxisOnTarget, MTRC_h4)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(MotorCommMachine, _MTRC_WAIT_X_LIMIT_FOUND)
    EV_HANDLER(XLimitFound, MTRC_h7)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(MotorCommMachine, _MTRC_WAIT_Y_LIMIT_FOUND)
    EV_HANDLER(YLimitFound, MTRC_h9)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(MotorCommMachine, _MTRC_WAIT_ANY_AXIS_ON_INTERMEDIATE_TARGET)
    EV_HANDLER(XAxisOnTarget, MTRC_h10),
    EV_HANDLER(YAxisOnTarget, MTRC_h11)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(MotorCommMachine, _MTRC_WAIT_X_AXIS_ON_INTERMEDIATE_TARGET)
    EV_HANDLER(XAxisOnTarget, MTRC_h13)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(MotorCommMachine, _MTRC_WAIT_Y_AXIS_ON_INTERMEDIATE_TARGET)
    EV_HANDLER(YAxisOnTarget, MTRC_h12)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(MotorCommMachine, _MTRC_WAIT_ANY_AXIS_TIMER_EXPIRE)
    EV_HANDLER(XAxisTimerExpired, MTRC_h14),
    EV_HANDLER(YAxisTimerExpired, MTRC_h15)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(MotorCommMachine, _MTRC_WAIT_X_AXIS_TIMER_EXPIRE)
    EV_HANDLER(XAxisTimerExpired, MTRC_h17)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(MotorCommMachine, _MTRC_WAIT_Y_AXIS_TIMER_EXPIRE)
    EV_HANDLER(YAxisTimerExpired, MTRC_h16)
STATE_TRANSITION_MATRIX_END;



//////////////////////////////////////////////////
//
// Matrix Table
//
//////////////////////////////////////////////////

DEFINE_RESPONSE_TABLE_ENTRY(MotorCommMachine)
    STATE_MATRIX_ENTRY(_MTRC_IDLE),
    STATE_MATRIX_ENTRY(_MTRC_WAIT_ANY_AXIS_ON_TARGET),
    STATE_MATRIX_ENTRY(_MTRC_WAIT_X_AXIS_ON_TARGET),
    STATE_MATRIX_ENTRY(_MTRC_WAIT_Y_AXIS_ON_TARGET),
    STATE_MATRIX_ENTRY(_MTRC_WAIT_X_LIMIT_FOUND),
    STATE_MATRIX_ENTRY(_MTRC_WAIT_Y_LIMIT_FOUND),
    STATE_MATRIX_ENTRY(_MTRC_WAIT_ANY_AXIS_ON_INTERMEDIATE_TARGET),
    STATE_MATRIX_ENTRY(_MTRC_WAIT_X_AXIS_ON_INTERMEDIATE_TARGET),
    STATE_MATRIX_ENTRY(_MTRC_WAIT_Y_AXIS_ON_INTERMEDIATE_TARGET),
    STATE_MATRIX_ENTRY(_MTRC_WAIT_ANY_AXIS_TIMER_EXPIRE),
    STATE_MATRIX_ENTRY(_MTRC_WAIT_X_AXIS_TIMER_EXPIRE),
    STATE_MATRIX_ENTRY(_MTRC_WAIT_Y_AXIS_TIMER_EXPIRE)
RESPONSE_TABLE_END;




//////////////////////////////////////////////////
//
// Static Member Definitions
//
//////////////////////////////////////////////////

XAxisTimer  * MotorCommMachine::ptrXAT = 0;
YAxisTimer  * MotorCommMachine::ptrYAT = 0;

int         MotorCommMachine::currXCoordinate;
int         MotorCommMachine::currYCoordinate;

int         MotorCommMachine::intermediateXCoordinate;
int         MotorCommMachine::intermediateYCoordinate;

int         MotorCommMachine::targetXCoordinate;
int         MotorCommMachine::targetYCoordinate;

int         MotorCommMachine::prevXCoordinate;
int         MotorCommMachine::prevYCoordinate;

BOOL        MotorCommMachine::backlashCompensationEnabled;

MoveProfile MotorCommMachine::currXMoveProfile;
MoveProfile MotorCommMachine::currYMoveProfile;

TableParametersDataManager  * MotorCommMachine::ptrTPDM = 0;

WORD        MotorCommMachine::errorCount = 0;


            
//////////////////////////////////////////////////
//
// MotorCommMachine - Constructors, Destructors
//
//////////////////////////////////////////////////

MotorCommMachine::MotorCommMachine(BYTE sMsysID)
    :StateMachine(sMsysID)
{
    ASSIGN_RESPONSE_TABLE();

    SetCurrState(MTRC_IDLE);

    // Reset Motor Controller Chip



}

MotorCommMachine::~MotorCommMachine(void) { }


WORD    MotorCommMachine::GetErrorCount(void) {

    return  MotorCommMachine::errorCount;
}



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// MotorCommMachine - private EXIT PROCEDURES
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h1(void) {

    // Get the data from the message

    targetXCoordinate = (int)(GetCurrEvent().msgData1);
    targetYCoordinate = (int)(GetCurrEvent().msgData2);

    // Check if both axis are already on target

    if(IsXAxisOnTarget() && IsYAxisOnTarget())
    {
            SendHiPrMsg(HeadMachinesManagerID, XYAxisOnTarget);

        return  MTRC_IDLE;
    }

    // At least 1 of the axis is NOT on target

    // Check if we need to do backlash compensation

    if(IsBacklashCompensationEnabled())
    {
        // Set the intermediate targets

        intermediateXCoordinate =
            targetXCoordinate - (ptrTPDM->GetBacklashOffset().GetXCoordinate());

        intermediateYCoordinate =
            targetXCoordinate - (ptrTPDM->GetBacklashOffset().GetYCoordinate());

        // Check if both axis are already on their intermediate target

        if(IsXAxisOnIntermediateTarget() && IsYAxisOnIntermediateTarget())
        {
                // If so just move to the final targets

                if(!IsXAxisOnTarget())
                    SendMcsMoveCommand(X_AXIS, targetXCoordinate);

                if(!IsYAxisOnTarget())
                    SendMcsMoveCommand(Y_AXIS, targetYCoordinate);

            return MTRC_WAIT_ANY_AXIS_ON_FINAL_TARGET;
        }
        else
        {
                // Move to the intermediate targets

                if(!IsXAxisOnIntermediateTarget())
                    SendMcsMoveCommand(X_AXIS, intermediateXCoordinate);

                if(!IsYAxisOnIntermediateTarget())
                    SendMcsMoveCommand(Y_AXIS, intermediateYCoordinate);

            return MTRC_WAIT_ANY_AXIS_ON_INTERMEDIATE_TARGET;
        }
    }
        else    // NO Backlash Compensation
    {
            // Determine which axis needs to be moved

            if(!IsXAxisOnTarget())
                SendMcsMoveCommand(X_AXIS, targetXCoordinate);

            if(!IsYAxisOnTarget())
                SendMcsMoveCommand(Y_AXIS, targetYCoordinate);

        return  MTRC_WAIT_ANY_AXIS_ON_TARGET;
    }
}


//////////////////////////////////////////////////
//
// Message: Goto X Target
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h1a(void) {

        targetXCoordinate = (int)(GetCurrEvent().msgData1);

        SendMcsMoveCommand(X_AXIS, targetXCoordinate);

    return MTRC_WAIT_X_AXIS_ON_TARGET_2;
}


//////////////////////////////////////////////////
//
// Message: X Axis On Target
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h1b(void) {

        SendHiPrMsg(HeadMachinesManagerID, XYAxisOnTarget);

    return MTRC_IDLE;
}


//////////////////////////////////////////////////
//
// Message: Goto Y Target
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h1c(void) {

        targetYCoordinate = (int)(GetCurrEvent().msgData1);

        SendMcsMoveCommand(Y_AXIS, targetYCoordinate);

    return MTRC_WAIT_Y_AXIS_ON_TARGET_2;
}


//////////////////////////////////////////////////
//
// Message: Y Axis On Target
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h1d(void) {

        SendHiPrMsg(HeadMachinesManagerID, XYAxisOnTarget);

    return MTRC_IDLE;
}


//////////////////////////////////////////////////
//
// Message: X Axis On Target
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h2(void) {

    currXCoordinate = targetXCoordinate;    // Update curr value

        if(IsYAxisOnTarget()) {

            // Both are already on target

            SendHiPrMsg(HeadMachinesManagerID, XYAxisOnTarget);

            return  MTRC_IDLE;
        }

    return  MTRC_WAIT_Y_AXIS_ON_TARGET;
}


//////////////////////////////////////////////////
//
// Message: Y Axis On Target
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h3(void) {

    currYCoordinate = targetYCoordinate;    // Update curr value

        if(IsXAxisOnTarget()) {

                // Both are already on target

                SendHiPrMsg(HeadMachinesManagerID, XYAxisOnTarget);

            return  MTRC_IDLE;
        }

    return  MTRC_WAIT_X_AXIS_ON_TARGET;
}


//////////////////////////////////////////////////
//
// Message: Y Axis On Target
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h4(void) {

        currYCoordinate = targetYCoordinate;    // Update curr value

        SendHiPrMsg(HeadMachinesManagerID, XYAxisOnTarget);

    return  MTRC_IDLE;
}


//////////////////////////////////////////////////
//
// Message: X Axis On Target
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h5(void) {

        currXCoordinate = targetXCoordinate;    // Update curr value

        SendHiPrMsg(HeadMachinesManagerID, XYAxisOnTarget);

    return  MTRC_IDLE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h6(void) {

        SendMcsCommand(SET_CURR_AXIS_1); // Set Axis to X

        SendMcsWordCommand(SET_INTERRUPT_MASK, WORD(NEGATIVE_LIMIT_SWITCH));

        SetXTargetCoordinate(X_NEGATIVE_LIMIT);

        SendMcsDWordCommand(SET_POSITION, targetXCoordinate);

        SendMcsCommand(UPDATE_PARAM);

    return  MTRC_IDLE;
}


//////////////////////////////////////////////////
//
// Message: X Limit Found
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h7(void) {

        currXCoordinate = targetXCoordinate;    // Update curr value

        // Pass message to HMM

        SendHiPrMsg(HeadMachinesManagerID, XLimitFound);

    return  MTRC_IDLE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h8(void) {

        SendMcsCommand(SET_CURR_AXIS_2); // Set Axis to Y

        SendMcsWordCommand(SET_INTERRUPT_MASK, WORD(NEGATIVE_LIMIT_SWITCH));

        SetXTargetCoordinate(Y_NEGATIVE_LIMIT);

        SendMcsDWordCommand(SET_POSITION, targetXCoordinate);

        SendMcsCommand(UPDATE_PARAM);

    return  MTRC_IDLE;
}


//////////////////////////////////////////////////
//
// Message: Y Limit Found
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h9(void) {

        currYCoordinate = targetYCoordinate;    // Update curr value

        // Pass message to HMM

        SendHiPrMsg(HeadMachinesManagerID, YLimitFound);

    return  MTRC_IDLE;
}


//////////////////////////////////////////////////
//
// Message: X Axis On Target
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h10(void) {

        currXCoordinate = intermediateXCoordinate;    // Update curr value

        // Start a timer for the X axis

        SendHiPrMsg(XAxisTimerID, StartXAxisTimer);

        if(IsYAxisOnIntermediateTarget())
        {
            // Both axis are on target

            return MTRC_WAIT_ANY_AXIS_TIMER_EXPIRE;
        }

    return MTRC_WAIT_Y_AXIS_ON_INTERMEDIATE_TARGET;
}


//////////////////////////////////////////////////
//
// Message: Y Axis On Target
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h11(void) {

        currYCoordinate = intermediateYCoordinate;    // Update curr value

        // Start a timer for the Y axis

        SendHiPrMsg(YAxisTimerID, StartYAxisTimer);

        if(IsXAxisOnIntermediateTarget())
        {
            // Both axis are on target

            return MTRC_WAIT_ANY_AXIS_TIMER_EXPIRE;
        }

    return MTRC_WAIT_X_AXIS_ON_INTERMEDIATE_TARGET;
}


//////////////////////////////////////////////////
//
// Message: Y Axis On Target
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h12(void) {

        // Start a timer for the Y axis

        SendHiPrMsg(YAxisTimerID, StartYAxisTimer);

    return MTRC_WAIT_ANY_AXIS_TIMER_EXPIRE;
}


//////////////////////////////////////////////////
//
// Message: X Axis On Target
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h13(void) {

        // Start a timer for the Y axis

        SendHiPrMsg(XAxisTimerID, StartXAxisTimer);

    return MTRC_WAIT_ANY_AXIS_TIMER_EXPIRE;
}


//////////////////////////////////////////////////
//
// Message: X Axis Timer Expired
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h14(void) {

        if(ptrYAT->IsYAxisTimerExpired())
        {
                // Both axis timers have expired, Move to the final targets

                if(!IsXAxisOnTarget())
                    SendMcsMoveCommand(X_AXIS, targetXCoordinate);

                if(!IsYAxisOnTarget())
                    SendMcsMoveCommand(Y_AXIS, targetYCoordinate);

            return MTRC_WAIT_ANY_AXIS_ON_FINAL_TARGET;
        }

    // Y axis timer is still running

    return MTRC_WAIT_Y_AXIS_TIMER_EXPIRE;
}


//////////////////////////////////////////////////
//
// Message: Y Axis Timer Expired
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h15(void) {

        if(ptrXAT->IsXAxisTimerExpired())
        {
                // Both axis timers have expired, Move to the final targets

                if(!IsXAxisOnTarget())
                    SendMcsMoveCommand(X_AXIS, targetXCoordinate);

                if(!IsYAxisOnTarget())
                    SendMcsMoveCommand(Y_AXIS, targetYCoordinate);

            return MTRC_WAIT_ANY_AXIS_ON_FINAL_TARGET;
        }

    // X axis timer is still running

    return MTRC_WAIT_X_AXIS_TIMER_EXPIRE;
}


//////////////////////////////////////////////////
//
// Message: Y Axis Timer Expired
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h16(void) {

        // Both axis timers have expired, Move to the final targets

        if(!IsXAxisOnTarget())
            SendMcsMoveCommand(X_AXIS, targetXCoordinate);

        if(!IsYAxisOnTarget())
            SendMcsMoveCommand(Y_AXIS, targetYCoordinate);

    return MTRC_WAIT_ANY_AXIS_ON_FINAL_TARGET;
}


//////////////////////////////////////////////////
//
// Message: X Axis Timer Expired
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h17(void) {

        // Both axis timers have expired, Move to the final targets

        if(!IsXAxisOnTarget())
            SendMcsMoveCommand(X_AXIS, targetXCoordinate);

        if(!IsYAxisOnTarget())
            SendMcsMoveCommand(Y_AXIS, targetYCoordinate);

    return MTRC_WAIT_ANY_AXIS_ON_FINAL_TARGET;
}


//////////////////////////////////////////////////
//
// Message: X Axis On Target
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h18(void) {

    return MTRC_WAIT_Y_AXIS_ON_FINAL_TARGET;
}


//////////////////////////////////////////////////
//
// Message: Y Axis On Target
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h19(void) {

    return MTRC_WAIT_X_AXIS_ON_FINAL_TARGET;
}

//////////////////////////////////////////////////
//
// Message: Y Axis On Target
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h20(void) {

        // Both axis are now on their final positions
        // Inform HMM about the completion of the request

        SendHiPrMsg(HeadMachinesManagerID, XYAxisOnTarget);

    return MTRC_IDLE;
}


//////////////////////////////////////////////////
//
// Message: Y Axis On Target
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h21(void) {

        // Both axis are now on their final positions
        // Inform HMM about the completion of the request

        SendHiPrMsg(HeadMachinesManagerID, XYAxisOnTarget);

    return MTRC_IDLE;
}


//////////////////////////////////////////////////
//
// Message: Set New Move Profile
// (for Any of the 2 Axis)
//
//////////////////////////////////////////////////

WORD
MotorCommMachine::MTRC_h22(void) {

    // Get the data from the message

    AxisId axisSelected = (AxisId)(GetCurrEvent().msgData1);
    MoveProfileIndex profileSelected = (MoveProfileIndex)(GetCurrEvent().msgData2);

        // Update the move profile of the axis

        SendMcsChangeProfileCommand
            (axisSelected, ptrTPDM->GetMoveProfile(profileSelected));

        // Inform HMM about the completion of the request

        SendHiPrMsg(HeadMachinesManagerID, SetNewMoveProfileDone);

    return MTRC_IDLE;
}







