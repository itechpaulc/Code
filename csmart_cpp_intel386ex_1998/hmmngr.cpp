

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

#include "hmmngr.h"


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  HeadMachinesManager
//
//      - public interface functions :
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadMachinesManager::SetScanIndexes(WORD startIndex, WORD endIndex) {

    scanStartIndex = startIndex;
    scanEndIndex = endIndex;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadMachinesManager::GetScanIndexes(WORD &startIndex, WORD &endIndex) {

    startIndex = scanStartIndex;
    endIndex = scanEndIndex;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadMachinesManager::SetAlignmentData(AlignmentData alignData) {

    alignmentData = alignData;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadMachinesManager::GetAlignmentData(AlignmentData &alignData) {

    alignData = alignmentData;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadMachinesManager::LinkMotorCommMachine(MotorCommMachine *pMTRC) {

    ptrMTRC = pMTRC;
}

void
HeadMachinesManager::LinkTableControl(TableControl *pTC) {

    ptrTC = pTC;
}

void
HeadMachinesManager::
LinkTableParametersDataManager(TableParametersDataManager *pTPDM) {

    ptrTPDM = pTPDM;
}

void
HeadMachinesManager::
LinkMeasurementCommMachine(MeasurementCommMachine *pMCM) {

    ptrMCM = pMCM;
}

void
HeadMachinesManager::
LinkMeasurementDataManager(MeasurementDataManager *pMDM) {

    ptrMDM = pMDM;
}



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// HeadMachinesManager - private helper functions
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////




//////////////////////////////////////////////////
//
// HeadMachinesManager - RESPONSE ENTRIES
//
//////////////////////////////////////////////////



//////////////////////////////////////////////////
//
// State Transition Matrices
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_IDLE)
    EV_HANDLER(FindLimits, HMM_h2),
    EV_HANDLER(GoHome, HMM_h7),
    EV_HANDLER(MoveToPosition, HMM_h11),
    EV_HANDLER(PointTargetLamp, HMM_h14),
    EV_HANDLER(AirOnHeadDown, HMM_h16),
    EV_HANDLER(AirOffHeadUp, HMM_h18),
    EV_HANDLER(StrobeMeasurementFlash, HMM_h24),
    EV_HANDLER(MeasureOnTheSpot, HMM_h27),
    EV_HANDLER(MeasureAtTargetLamp, HMM_h30),
    EV_HANDLER(CalibrateAtWhitePlaque, HMM_h37),
    EV_HANDLER(CalibrateAtBlackHole, HMM_h42)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MOVING_AWAY_FROM_X)
    EV_HANDLER(XYAxisOnTarget, HMM_h3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MOVING_AWAY_FROM_Y)
    EV_HANDLER(XYAxisOnTarget, HMM_h4)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_FINDING_X_LIMIT)
    EV_HANDLER(XLimitFound, HMM_h5)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_FINDING_Y_LIMIT)
    EV_HANDLER(YLimitFound, HMM_h6)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MOVING_TO_Y_HOME)
    EV_HANDLER(XYAxisOnTarget, HMM_h8)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_FAST_MOVING_TO_X_HOME)
    EV_HANDLER(XYAxisOnTarget, HMM_h9)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_SLOW_MOVING_TO_X_HOME)
    EV_HANDLER(XYAxisOnTarget, HMM_h10)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MOVING_TO_TARGET)
    EV_HANDLER(XYAxisOnTarget, HMM_h12)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MOVE_SETTLE_WAIT)
    EV_HANDLER(TimeOut, HMM_h13)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_POINTING_TARGET_LAMP)
    EV_HANDLER(XYAxisOnTarget, HMM_h15)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_AIR_ON_HEAD_DOWN_DELAY)
    EV_HANDLER(TimeOut, HMM_h17)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_AIR_OFF_HEAD_UP_DELAY)
    EV_HANDLER(TimeOut, HMM_h19)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_STROBE_MEASUREMENT_FLASH_ACK_WAIT)
    EV_HANDLER(ProbeHeadCommandAcked, HMM_h25)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASUREMENT_FLASH_RECHARGE_WAIT)
    EV_HANDLER(TimeOut, HMM_h26)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_ON_THE_SPOT_WAITING_FOR_HEAD_REPLY)
    EV_HANDLER(ProbeHeadCommandAcked, HMM_h28)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_ON_THE_SPOT_WAITING_FOR_MEASUREMENT_DATABLOCK)
    EV_HANDLER(MeasurmentBlockReceived, HMM_h29)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_AT_TLAMP_MOVING_TO_TLAMP)
    EV_HANDLER(XYAxisOnTarget, HMM_h31)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_AT_TLAMP_MOVE_SETTLE_WAIT)
    EV_HANDLER(TimeOut, HMM_h32)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_AT_TLAMP_WAITING_FOR_HEAD_REPLY)
    EV_HANDLER(ProbeHeadCommandAcked, HMM_h33)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_AT_TLAMP_WAITING_FOR_MEASUREMENT_DATABLOCK)
    EV_HANDLER(MeasurmentBlockReceived, HMM_h34)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_AT_TLAMP_MOVING_BACK_TO_ORIGIN)
    EV_HANDLER(XYAxisOnTarget, HMM_h35)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_AT_TLAMP_FINAL_MOVE_SETTLE_WAIT)
    EV_HANDLER(TimeOut, HMM_h36)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_CAL_AT_WHITE_MOVING_TO_WHITE_PLAQUE)
    EV_HANDLER(XYAxisOnTarget, HMM_h38)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_CAL_AT_WHITE_MOVE_SETTLE_WAIT)
    EV_HANDLER(TimeOut, HMM_h39)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_CAL_AT_WHITE_WAITING_FOR_HEAD_REPLY)
    EV_HANDLER(ProbeHeadCommandAcked, HMM_h40)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_CAL_AT_WHITE_WAITING_FOR_MEASUREMENT_DATABLOCK)
    EV_HANDLER(MeasurmentBlockReceived, HMM_h41)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_CAL_AT_BLACK_MOVING_TO_BLACK_HOLE)
    EV_HANDLER(XYAxisOnTarget, HMM_h43)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_CAL_AT_BLACK_MOVE_SETTLE_WAIT)
    EV_HANDLER(TimeOut, HMM_h44)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_CAL_AT_BLACK_WAITING_FOR_HEAD_REPLY)
    EV_HANDLER(ProbeHeadCommandAcked, HMM_h45)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_CAL_AT_BLACK_WAITING_FOR_MEASUREMENT_DATABLOCK)
    EV_HANDLER(MeasurmentBlockReceived, HMM_h46)
STATE_TRANSITION_MATRIX_END;



//////////////////////////////////////////////////
//
// Matrix Table
//
//////////////////////////////////////////////////

DEFINE_RESPONSE_TABLE_ENTRY(HeadMachinesManager)
    STATE_MATRIX_ENTRY(_HMM_IDLE),
    STATE_MATRIX_ENTRY(_HMM_MOVING_AWAY_FROM_X),
    STATE_MATRIX_ENTRY(_HMM_MOVING_AWAY_FROM_Y),
    STATE_MATRIX_ENTRY(_HMM_FINDING_X_LIMIT),
    STATE_MATRIX_ENTRY(_HMM_FINDING_Y_LIMIT),
    STATE_MATRIX_ENTRY(_HMM_MOVING_TO_Y_HOME),
    STATE_MATRIX_ENTRY(_HMM_FAST_MOVING_TO_X_HOME),
    STATE_MATRIX_ENTRY(_HMM_SLOW_MOVING_TO_X_HOME),
    STATE_MATRIX_ENTRY(_HMM_MOVING_TO_TARGET),
    STATE_MATRIX_ENTRY(_HMM_MOVE_SETTLE_WAIT),
    STATE_MATRIX_ENTRY(_HMM_POINTING_TARGET_LAMP),
    STATE_MATRIX_ENTRY(_HMM_AIR_ON_HEAD_DOWN_DELAY),
    STATE_MATRIX_ENTRY(_HMM_AIR_OFF_HEAD_UP_DELAY),
    STATE_MATRIX_ENTRY(_HMM_STROBE_MEASUREMENT_FLASH_ACK_WAIT),
    STATE_MATRIX_ENTRY(_HMM_MEASUREMENT_FLASH_RECHARGE_WAIT),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_ON_THE_SPOT_WAITING_FOR_HEAD_REPLY),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_ON_THE_SPOT_WAITING_FOR_MEASUREMENT_DATABLOCK),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_AT_TLAMP_MOVING_TO_TLAMP),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_AT_TLAMP_MOVE_SETTLE_WAIT),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_AT_TLAMP_WAITING_FOR_HEAD_REPLY),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_AT_TLAMP_WAITING_FOR_MEASUREMENT_DATABLOCK),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_AT_TLAMP_MOVING_BACK_TO_ORIGIN),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_AT_TLAMP_FINAL_MOVE_SETTLE_WAIT),
    STATE_MATRIX_ENTRY(_HMM_CAL_AT_WHITE_MOVING_TO_WHITE_PLAQUE),
    STATE_MATRIX_ENTRY(_HMM_CAL_AT_WHITE_MOVE_SETTLE_WAIT),
    STATE_MATRIX_ENTRY(_HMM_CAL_AT_WHITE_WAITING_FOR_HEAD_REPLY),
    STATE_MATRIX_ENTRY(_HMM_CAL_AT_WHITE_WAITING_FOR_MEASUREMENT_DATABLOCK),
    STATE_MATRIX_ENTRY(_HMM_CAL_AT_BLACK_MOVING_TO_BLACK_HOLE),
    STATE_MATRIX_ENTRY(_HMM_CAL_AT_BLACK_MOVE_SETTLE_WAIT),
    STATE_MATRIX_ENTRY(_HMM_CAL_AT_BLACK_WAITING_FOR_HEAD_REPLY),
    STATE_MATRIX_ENTRY(_HMM_CAL_AT_BLACK_WAITING_FOR_MEASUREMENT_DATABLOCK)
RESPONSE_TABLE_END;




//////////////////////////////////////////////////
//
// Static Member Definitions
//
//////////////////////////////////////////////////

WORD    HeadMachinesManager::scanStartIndex = 0;
WORD    HeadMachinesManager::scanEndIndex = 0;

AlignmentData   HeadMachinesManager::alignmentData;

MotorCommMachine            * HeadMachinesManager::ptrMTRC = 0;
TableControl                * HeadMachinesManager::ptrTC = 0;
TableParametersDataManager  * HeadMachinesManager::ptrTPDM = 0;
MeasurementCommMachine      * HeadMachinesManager::ptrMCM = 0;

MeasurementDataManager      * HeadMachinesManager::ptrMDM = 0;


int     HeadMachinesManager::xOrigin = 0;
int     HeadMachinesManager::yOrigin = 0;

BYTE    HeadMachinesManager::requestorID = NULL_SM_ID;



//////////////////////////////////////////////////
//
// HeadMachinesManager - Constructors, Destructors
//
//////////////////////////////////////////////////

HeadMachinesManager::HeadMachinesManager(BYTE sMsysID)
    :StateMachine(sMsysID)
{
    ASSIGN_RESPONSE_TABLE();

    SetCurrState(HMM_IDLE);
}

HeadMachinesManager::~HeadMachinesManager(void) { }



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// HeadMachinesManager - private EXIT PROCEDURES
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
// Message: Find Limits
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h2(void) {

    // Remember who made the request

    requestorID = GetCurrEvent().senderId;

    // Get the current X axis position

    int xAway = ptrMTRC->GetCurrXCoordinate();

        // Set X - Away Target

        xAway +=
            ptrTPDM->GetFindLimitMoveOutDistance().GetXCoordinate();

        //
        //SendHiPrMsg(MotorCommID, PROFILECHANGE);
        //

        SendHiPrMsg(MotorCommID, GotoXTarget, xAway, NULL_MESSAGE_ID);

    return  HMM_MOVING_AWAY_FROM_X;
}


//////////////////////////////////////////////////
//
// Message: XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h3(void) {

    // Get the current Y axis position

    int yAway = ptrMTRC->GetCurrYCoordinate();

        // Set Y - Away Target

        yAway +=
            ptrTPDM->GetFindLimitMoveOutDistance().GetYCoordinate();

        //
        //SendHiPrMsg(MotorCommID, PROFILECHANGE);
        //

        SendHiPrMsg(MotorCommID, GotoYTarget, yAway, NULL_MESSAGE_ID);

    return  HMM_MOVING_AWAY_FROM_Y;
}


//////////////////////////////////////////////////
//
// Message: XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h4(void) {

        SendHiPrMsg(MotorCommID, FindXLimit);

    return  HMM_FINDING_X_LIMIT;
}


//////////////////////////////////////////////////
//
// Message: X Limit Found
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h5(void) {

        SendHiPrMsg(MotorCommID, FindYLimit);

    return  HMM_FINDING_Y_LIMIT;
}


//////////////////////////////////////////////////
//
// Message: Y Limit Found
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h6(void) {

        // inform sender about the completion of the request

        SendHiPrMsg(requestorID, LimitsFound);

    return  HMM_IDLE;
}


//////////////////////////////////////////////////
//
// Message: Go Home
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h7(void) {

    // Remember who made the request

    requestorID = GetCurrEvent().senderId;

    // Send head towards the Y home position

    int yHome = ptrTPDM->GetWhitePlaqueLocation().GetYCoordinate();

        //
        //SendHiPrMsg(MotorCommID,  PROFILECHANGE);
        //

        SendHiPrMsg(MotorCommID, GotoYTarget, yHome, NULL_MESSAGE_DATA);

    return  HMM_MOVING_TO_Y_HOME;
}

//////////////////////////////////////////////////
//
//  Message: XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h8(void) {

    // Send head towards the X home position

    int xSlowZone = ptrTPDM->GetGoingHomeSlowXPosition().GetXCoordinate();
    int xHomePosition = ptrTPDM->GetWhitePlaqueLocation().GetXCoordinate();

        // Are we in the slow X zone already ?

        if(ptrMTRC->GetCurrXCoordinate() < xSlowZone) {

                // Start moving slow

                //
                //SendHiPrMsg(MotorCommID,  PROFILECHANGE);
                //

                SendHiPrMsg(MotorCommID,  GotoXTarget,
                            xHomePosition, NULL_MESSAGE_DATA);

            return  HMM_SLOW_MOVING_TO_X_HOME;
        }

        // We are still in the Fast Zone
        // use full Speed to move towards the slow X Zone

        SendHiPrMsg(MotorCommID,  GotoXTarget,
                    xSlowZone, NULL_MESSAGE_DATA);

    return  HMM_FAST_MOVING_TO_X_HOME;
}

//////////////////////////////////////////////////
//
//  Message: XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h9(void) {

    int xHomePosition = ptrTPDM->GetWhitePlaqueLocation().GetXCoordinate();

        // Start moving slow

        //
        //SendHiPrMsg(MotorCommID,  PROFILECHANGE);
        //

        SendHiPrMsg(MotorCommID,  GotoXTarget,
                    xHomePosition, NULL_MESSAGE_DATA);

    return  HMM_SLOW_MOVING_TO_X_HOME;
}

//////////////////////////////////////////////////
//
//  Message: XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h10(void) {

        // Inform the REQUESTOR about the completion of the request

        SendHiPrMsg(requestorID,  GoHomeDone);

    return  HMM_IDLE;
}


//////////////////////////////////////////////////
//
//  Message: Move To Position
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h11(void) {

    // Remember who made the request

    requestorID = GetCurrEvent().senderId;

    // Get the data from the message

    int positionXCoordinate = (int)(GetCurrEvent().msgData1);
    int positionYCoordinate = (int)(GetCurrEvent().msgData2);

        SendHiPrMsg(MotorCommID,  GotoXYTarget,
                    positionXCoordinate, positionYCoordinate);

    return HMM_MOVING_TO_TARGET;
}


//////////////////////////////////////////////////
//
//  Message: XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h12(void) {

        StartHiPriorityTimer(ptrTPDM->GetFinalMoveSettleTime());

    return HMM_MOVE_SETTLE_WAIT;
}


//////////////////////////////////////////////////
//
//  Message: TimeOut
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h13(void) {

        // Inform the requestor about the completion of the task

        SendHiPrMsg(requestorID,  MoveToPositionDone);

    return HMM_IDLE;
}


//////////////////////////////////////////////////
//
//  Message: Point Target Lamp
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h14(void) {

    int finalXCoordinate, finalYCoordinate;

    // Get the data from the message

    int tLampXCoordinate = (int)(GetCurrEvent().msgData1);
    int tLampYCoordinate = (int)(GetCurrEvent().msgData2);

        // Based on the offset, calculate where the sensor should be located

        finalXCoordinate =
            tLampXCoordinate - ptrTPDM->GetTargetLampOffset().GetXCoordinate();

        finalYCoordinate =
            tLampYCoordinate - ptrTPDM->GetTargetLampOffset().GetYCoordinate();

        SendHiPrMsg(MotorCommID,  GotoXYTarget,
                    finalXCoordinate, finalYCoordinate);

    return HMM_POINTING_TARGET_LAMP;
}


//////////////////////////////////////////////////
//
//  Message: XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h15(void) {

        // Inform the CSM about the completion of the task

        SendHiPrMsg(ColorSmartManagerID, AirIsOnHeadIsDown);

    return HMM_IDLE;
}


//////////////////////////////////////////////////
//
//  Message: Air On Head Down
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h16(void) {

        // Remember who made the request

        requestorID = GetCurrEvent().senderId;

        //
        // Air On Head Down Control
        //

        // ptrTC->AirOnProbeHeadDown();

        StartHiPriorityTimer(ptrTPDM->GetAirOnHeadDownTime());

    return HMM_AIR_ON_HEAD_DOWN_DELAY;
}


//////////////////////////////////////////////////
//
//  Message: TimeOut
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h17(void) {

        // Inform the requestor about the completion of the task

        SendHiPrMsg(requestorID, PointTargetLampDone);

    return HMM_IDLE;
}


//////////////////////////////////////////////////
//
//  Message: Air Off Head Up
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h18(void) {

        // Remember who made the request

        requestorID = GetCurrEvent().senderId;

        //
        // Air Off Head Up Control
        //

        // ptrTC->AirOffProbeHeadUp();

        StartHiPriorityTimer(ptrTPDM->GetAirOffHeadUpTime());

    return HMM_AIR_OFF_HEAD_UP_DELAY;
}


//////////////////////////////////////////////////
//
//  Message: TimeOut
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h19(void) {

        // Inform the HOC about the completion of the task

        SendHiPrMsg(HeadOperationsCoordinatorMachineID, AirIsOffHeadIsUp);

    return HMM_IDLE;
}



//////////////////////////////////////////////////
//
//  Message:
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h20(void) {


    return 0;
}


//////////////////////////////////////////////////
//
//  Message:
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h21(void) {


    return 0;
}


//////////////////////////////////////////////////
//
//  Message:
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h22(void) {


    return 0;
}


//////////////////////////////////////////////////
//
//  Message:
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h23(void) {


    return 0;
}



//////////////////////////////////////////////////
//
//  Message: Strobe Measurement Flash
//
//////////////////////////////////////////////////

#define STROBE_MFLASH_CMD_LENGTH    1

WORD
HeadMachinesManager::HMM_h24(void) {

        //ptrHCC->SetTransmitBuffer("S", STROBE_MFLASH_CMD_LENGTH);

        SendHiPrMsg(HeadCommandCommID, SendProbeHeadCommand);

    return HMM_STROBE_MEASUREMENT_FLASH_ACK_WAIT;
}


//////////////////////////////////////////////////
//
//  Message: Probe Head Command Acked
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h25(void) {

        // Wait to make sure flash will be ready for the next command

        StartHiPriorityTimer(ptrTPDM->GetMeasurementFlashRechargeTime());

    return HMM_MEASUREMENT_FLASH_RECHARGE_WAIT;
}


//////////////////////////////////////////////////
//
//  Message: TimeOut
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h26(void) {

        // Inform CSM about completion of the request

        SendHiPrMsg(ColorSmartManagerID, StrobeMeasurementFlashDone);

    return HMM_IDLE;
}


//////////////////////////////////////////////////
//
//  Message: Measure On The Spot
//
//////////////////////////////////////////////////

#define MEASUREMENT_CMD_LENGTH      1

WORD
HeadMachinesManager::HMM_h27(void) {

        // Tell MCM to be ready for the measurement data

        SendHiPrMsg(MeasurementCommMachineID, WaitForMeasurementBlock);

        //ptrHCC->SetTransmitBuffer("M", MEASUREMENT_CMD_LENGTH);

        SendHiPrMsg(HeadCommandCommID, SendProbeHeadCommand);

    return HMM_MEASURE_ON_THE_SPOT_WAITING_FOR_HEAD_REPLY;
}


//////////////////////////////////////////////////
//
//  Message: Probe Head Command Acked
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h28(void) {

        // Just wait for the completion of the
        // measurement data transmission

    return HMM_MEASURE_ON_THE_SPOT_WAITING_FOR_MEASUREMENT_DATABLOCK;
}


//////////////////////////////////////////////////
//
//  Message: Measurment Block Received
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h29(void) {

        // Get the measurement data from MCM
        // and store it using MDM

        ptrMDM->PutNext(ptrMCM->GetScanMeasurementData());

        // Inform HOC about the completion of the request

        SendHiPrMsg(HeadOperationsCoordinatorMachineID, MeasureOnTheSpotDone);

    return HMM_IDLE;
}


//////////////////////////////////////////////////
//
//  Message: Measure At Target Lamp
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h30(void) {

    xOrigin = ptrMTRC->GetCurrXCoordinate();
    yOrigin = ptrMTRC->GetCurrYCoordinate();

    // calculate current target lamp position based
    // on current measurement position

    int tLampXCoordinate = xOrigin +
            ptrTPDM->GetTargetLampOffset().GetXCoordinate();

    int tLampYCoordinate = yOrigin +
            ptrTPDM->GetTargetLampOffset().GetYCoordinate();

        // Tell MTRC to Move to Target Lamp

        SendHiPrMsg(MotorCommID,  GotoXTarget,
                    tLampXCoordinate, tLampYCoordinate);

    return HMM_MEASURE_AT_TLAMP_MOVING_TO_TLAMP;
}


//////////////////////////////////////////////////
//
//  Message: XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h31(void) {

        StartHiPriorityTimer(ptrTPDM->GetFinalMoveSettleTime());

    return HMM_MEASURE_AT_TLAMP_MOVE_SETTLE_WAIT;
}


//////////////////////////////////////////////////
//
//  Message: TimeOut
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h32(void) {

        // Tell MCM to be ready for the measurement data

        SendHiPrMsg(MeasurementCommMachineID, WaitForMeasurementBlock);

        //ptrHCC->SetTransmitBuffer("M", MEASUREMENT_CMD_LENGTH);

        SendHiPrMsg(HeadCommandCommID, SendProbeHeadCommand);

    return HMM_MEASURE_AT_TLAMP_WAITING_FOR_HEAD_REPLY;
}


//////////////////////////////////////////////////
//
//  Message: Probe Head Command Acked
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h33(void) {

        // Just wait for the completion of the
        // measurement data transmission

    return HMM_MEASURE_AT_TLAMP_WAITING_FOR_MEASUREMENT_DATABLOCK;
}


//////////////////////////////////////////////////
//
//  Message: Measurment Block Received
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h34(void) {

        // Get the measurement data from MCM
        // and store it using MDM

        ptrMDM->PutNext(ptrMCM->GetScanMeasurementData());

        // Tell MTRC to Move to its original position

        SendHiPrMsg(MotorCommID,  GotoXTarget, xOrigin, yOrigin);

    return HMM_MEASURE_AT_TLAMP_MOVING_BACK_TO_ORIGIN;
}


//////////////////////////////////////////////////
//
//  Message: XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h35(void) {

        StartHiPriorityTimer(ptrTPDM->GetFinalMoveSettleTime());

    return HMM_MEASURE_AT_TLAMP_FINAL_MOVE_SETTLE_WAIT;
}

//////////////////////////////////////////////////
//
//  Message: TimeOut
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h36(void) {

        // Inform the requestor about the completion of the task

        SendHiPrMsg(HeadOperationsCoordinatorMachineID, MeasureAtTargetLampDone);

    return HMM_IDLE;
}


//////////////////////////////////////////////////
//
//  Message: Calibrate At White Plaque
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h37(void) {

    int whitePlaqueXCoordinate = ptrTPDM->GetWhitePlaqueLocation().GetXCoordinate();
    int whitePlaqueYCoordinate = ptrTPDM->GetWhitePlaqueLocation().GetYCoordinate();

        // Tell MTRC to Move to the White Plaque position

        SendHiPrMsg(MotorCommID,  GotoXTarget,
                    whitePlaqueXCoordinate, whitePlaqueYCoordinate);

    return HMM_CAL_AT_WHITE_MOVING_TO_WHITE_PLAQUE;
}


//////////////////////////////////////////////////
//
//  Message: XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h38(void) {

        StartHiPriorityTimer(ptrTPDM->GetFinalMoveSettleTime());

    return HMM_CAL_AT_WHITE_MOVE_SETTLE_WAIT;
}


//////////////////////////////////////////////////
//
//  Message: TimeOut
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h39(void) {

        // Tell MCM to be ready for the measurement data

        SendHiPrMsg(MeasurementCommMachineID, WaitForMeasurementBlock);

        //ptrHCC->SetTransmitBuffer("M", MEASUREMENT_CMD_LENGTH);

        SendHiPrMsg(HeadCommandCommID, SendProbeHeadCommand);

    return HMM_CAL_AT_WHITE_WAITING_FOR_HEAD_REPLY;
}


//////////////////////////////////////////////////
//
//  Message: Probe Head Command Acked
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h40(void) {

        // Just wait for the completion of the
        // measurement data transmission

    return HMM_CAL_AT_WHITE_WAITING_FOR_MEASUREMENT_DATABLOCK;
}


//////////////////////////////////////////////////
//
//  Message: Measurment Block Received
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h41(void) {

        // Get the measurement data from MCM
        // and store it using MDM

        ptrMDM->PutNext(ptrMCM->GetScanMeasurementData());

        // Inform HOC about the completion of the request

        SendHiPrMsg(HeadOperationsCoordinatorMachineID, CalibrateAtWhitePlaqueDone);

    return HMM_IDLE;
}


//////////////////////////////////////////////////
//
//  Message: Calibrate At Black Hole
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h42(void)
{
    int blackHoleXCoordinate = ptrTPDM->GetBlackHoleLocation().GetXCoordinate();
    int blackHoleYCoordinate = ptrTPDM->GetBlackHoleLocation().GetYCoordinate();

        // Tell MTRC to Move to the Black Hole position

        SendHiPrMsg
            (MotorCommID,  GotoXTarget,
             blackHoleXCoordinate, blackHoleYCoordinate);

    return HMM_CAL_AT_BLACK_MOVING_TO_BLACK_HOLE;
}


//////////////////////////////////////////////////
//
//  Message: XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h43(void) {

        StartHiPriorityTimer(ptrTPDM->GetFinalMoveSettleTime());

    return HMM_CAL_AT_BLACK_MOVE_SETTLE_WAIT;
}


//////////////////////////////////////////////////
//
//  Message: TimeOut
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h44(void) {

        // Tell MCM to be ready for the measurement data

        SendHiPrMsg(MeasurementCommMachineID, WaitForMeasurementBlock);

        //ptrHCC->SetTransmitBuffer("M", MEASUREMENT_CMD_LENGTH);

        SendHiPrMsg(HeadCommandCommID, SendProbeHeadCommand);

    return HMM_CAL_AT_BLACK_WAITING_FOR_HEAD_REPLY;
}


//////////////////////////////////////////////////
//
//  Message:  Probe Head Command Acked
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h45(void) {

        // Just wait for the completion of the
        // measurement data transmission

    return HMM_CAL_AT_BLACK_WAITING_FOR_MEASUREMENT_DATABLOCK;
}


//////////////////////////////////////////////////
//
//  Message:  Measurment Block Received
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h46(void) {

        // Get the measurement data from MCM
        // and store it using MDM

        ptrMDM->PutNext(ptrMCM->GetScanMeasurementData());

        // Inform HOC about the completion of the request

        SendHiPrMsg(HeadOperationsCoordinatorMachineID, CalibrateAtBlackHoleDone);

    return HMM_IDLE;
}



