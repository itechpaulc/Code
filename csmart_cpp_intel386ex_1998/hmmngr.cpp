


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

#include "hccomm.h"

#include <stdlib.h>



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
HeadMachinesManager::LinkAll
(TableParametersDataManager *pTPDM   , MeasurementCommMachine *pMCM,
 MeasurementDataManager *pMDM        , MotorCommMachine *pMTRC,
 HeadCommandCommMachine *pHCC        , ScanParametersDataManager *pSPDM,
 TableControl *pTC                   , MeasurementFlashTimer *pMFT) {

        // pass to the individual link routines

        LinkTableParametersDataManager(pTPDM);
        LinkMeasurementCommMachine(pMCM);
        LinkMeasurementDataManager(pMDM);
        LinkMotorCommMachine(pMTRC);
        LinkHeadCommandCommMachine(pHCC);
        LinkScanParametersDataManager(pSPDM);
        LinkTableControl(pTC);
        LinkMeasurementFlashTimer(pMFT);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadMachinesManager::LinkTableControl(TableControl *pTC) {

    ptrTC = pTC;
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

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadMachinesManager::LinkHeadCommandCommMachine(HeadCommandCommMachine *pHCC) {

    ptrHCC = pHCC;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadMachinesManager::
LinkTableParametersDataManager(TableParametersDataManager *pTPDM) {

    ptrTPDM = pTPDM;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadMachinesManager::
LinkMeasurementCommMachine(MeasurementCommMachine *pMCM) {

    ptrMCM = pMCM;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadMachinesManager::
LinkMeasurementDataManager(MeasurementDataManager *pMDM) {

    ptrMDM = pMDM;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadMachinesManager::LinkScanParametersDataManager(ScanParametersDataManager *pSPDM) {

    ptrSPDM = pSPDM;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadMachinesManager::LinkMeasurementFlashTimer(MeasurementFlashTimer *pMFT) {

    ptrMFT = pMFT;
}


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
HeadMachinesManager::SetTargetAlignmentData(TargetAlignmentData alignData) {

    targetAlignmentData = alignData;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadMachinesManager::GetTargetAlignmentData(TargetAlignmentData &alignData) {

    alignData = targetAlignmentData;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadMachinesManager::SetAlignmentScanData(AlignmentScanData alignData) {

    alignmentScanData = alignData;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadMachinesManager::GetAlignmentScanData(AlignmentScanData &alignData) {

    alignData = alignmentScanData;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL
HeadMachinesManager::IsAirOnProbeHeadDown(void) {

    return(ptrTC->IsAirOnProbeHeadDown());
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
    EV_HANDLER(MoveToPositionWithBacklashCompensation, HMM_h11a),
    EV_HANDLER(PointTargetLamp, HMM_h14),
    EV_HANDLER(AirOnHeadDown, HMM_h16),
    EV_HANDLER(AirOffHeadUp, HMM_h18),
    EV_HANDLER(StrobeMeasurementFlash, HMM_h24),
    EV_HANDLER(MeasureOnTheSpot, HMM_h27),
    EV_HANDLER(MeasureAtTargetLamp, HMM_h30),
    EV_HANDLER(CalibrateAtWhitePlaque, HMM_h37),
    EV_HANDLER(CalibrateAtBlackHole, HMM_h42),
    EV_HANDLER(DoTargetAlignmentScan, HMM_h47),
    EV_HANDLER(DoPointToPointScan, HMM_h52),
    EV_HANDLER(DoColorBarScan, HMM_h57),
    EV_HANDLER(MeasureAtPoint, HMM_h62),
    EV_HANDLER(DoAlignmentScan, HMM_h67)
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
    EV_HANDLER(MeasurementFlashTimerExpired, HMM_h26)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_ON_THE_SPOT_WAITING_FOR_HEAD_REPLY)
    EV_HANDLER(ProbeHeadCommandAcked, HMM_h28)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_ON_THE_SPOT_WAITING_FOR_MEASUREMENT_DATABLOCK)
    EV_HANDLER(MeasurementBlockReceived, HMM_h29)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_ON_THE_SPOT_MEASUREMENT_FLASH_RECHARGE_WAIT)
    EV_HANDLER(MeasurementFlashTimerExpired, HMM_h29a)
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
    EV_HANDLER(MeasurementBlockReceived, HMM_h34)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_AT_TLAMP_MOVING_BACK_TO_ORIGIN)
    EV_HANDLER(XYAxisOnTarget, HMM_h35)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_AT_TLAMP_FINAL_MOVE_SETTLE_WAIT)
    EV_HANDLER(TimeOut, HMM_h36)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_AT_TLAMP_MEASUREMENT_FLASH_RECHARGE_WAIT)
    EV_HANDLER(MeasurementFlashTimerExpired, HMM_h36a)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_CAL_AT_WHITE_MOVING_TO_WHITE_PLAQUE)
    EV_HANDLER(XYAxisOnTarget, HMM_h38)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_CAL_AT_WHITE_WAITING_FOR_HEAD_REPLY)
    EV_HANDLER(ProbeHeadCommandAcked, HMM_h39)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_CAL_AT_WHITE_WAITING_FOR_MEASUREMENT_DATABLOCK)
    EV_HANDLER(MeasurementBlockReceived, HMM_h40)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_CAL_AT_WHITE_MEASUREMENT_FLASH_RECHARGE_WAIT)
    EV_HANDLER(MeasurementFlashTimerExpired, HMM_h41)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_CAL_AT_BLACK_MOVING_TO_BLACK_HOLE)
    EV_HANDLER(XYAxisOnTarget, HMM_h43)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_CAL_AT_BLACK_WAITING_FOR_HEAD_REPLY)
    EV_HANDLER(ProbeHeadCommandAcked, HMM_h44)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_CAL_AT_BLACK_WAITING_FOR_MEASUREMENT_DATABLOCK)
    EV_HANDLER(MeasurementBlockReceived, HMM_h45)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_CAL_AT_BLACK_MEASUREMENT_FLASH_RECHARGE_WAIT)
    EV_HANDLER(MeasurementFlashTimerExpired, HMM_h46)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_TARGET_ALIGN_SCAN_MOVING_TO_NEXT_POINT)
    EV_HANDLER(XYAxisOnTarget, HMM_h48)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_TARGET_ALIGN_SCAN_MOVE_SETTLE_WAIT)
    EV_HANDLER(TimeOut, HMM_h48a)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_TARGET_ALIGN_SCAN_MEASUREMENT_FLASH_RECHARGE_WAIT)
    EV_HANDLER(MeasurementFlashTimerExpired, HMM_h49)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_TARGET_ALIGN_SCAN_WAITING_FOR_HEAD_REPLY)
    EV_HANDLER(ProbeHeadCommandAcked, HMM_h50)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_TARGET_ALIGN_SCAN_WAITING_FOR_MEASUREMENT_DATABLOCK)
    EV_HANDLER(MeasurementBlockReceived, HMM_h51)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_PT_TO_PT_SCAN_MOVING_TO_NEXT_POINT)
    EV_HANDLER(XYAxisOnTarget, HMM_h53)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_PT_TO_PT_SCAN_MOVE_SETTLE_WAIT)
    EV_HANDLER(TimeOut, HMM_h53a)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_PT_TO_PT_SCAN_MEASUREMENT_FLASH_RECHARGE_WAIT)
    EV_HANDLER(MeasurementFlashTimerExpired, HMM_h54)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_PT_TO_PT_SCAN_WAITING_FOR_HEAD_REPLY)
    EV_HANDLER(ProbeHeadCommandAcked, HMM_h55)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_PT_TO_PT_SCAN_WAITING_FOR_MEASUREMENT_DATABLOCK)
    EV_HANDLER(MeasurementBlockReceived, HMM_h56)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_COLORBAR_SCAN_GOING_TO_STARTING_POSITION)
    EV_HANDLER(XYAxisOnTarget, HMM_h58)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_COLORBAR_SCAN_WAITING_FOR_BREAKPOINT_TRIGGER)
    EV_HANDLER(BreakPointTriggered, HMM_h59)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_COLORBAR_SCAN_WAITING_FOR_MEASUREMENT_DATABLOCK)
    EV_HANDLER(MeasurementBlockReceived, HMM_h60)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_COLORBAR_SCAN_WAITING_FOR_CONTINUOUS_MOVE_DONE)
    EV_HANDLER(ContinuousMoveDone, HMM_h61)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_AT_POINT_MOVING_TO_POINT)
    EV_HANDLER(XYAxisOnTarget, HMM_h63)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_AT_POINT_MOVE_SETTLE_WAIT)
    EV_HANDLER(TimeOut, HMM_h64)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_AT_POINT_WAITING_FOR_HEAD_REPLY)
    EV_HANDLER(ProbeHeadCommandAcked, HMM_h65)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_AT_POINT_WAITING_FOR_MEASUREMENT_DATABLOCK)
    EV_HANDLER(MeasurementBlockReceived, HMM_h66)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_MEASURE_AT_POINT_MEASUREMENT_FLASH_RECHARGE_WAIT)
    EV_HANDLER(MeasurementFlashTimerExpired, HMM_h66a)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_ALIGN_SCAN_GOING_TO_STARTING_POSITION)
    EV_HANDLER(XYAxisOnTarget, HMM_h68)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_ALIGN_SCAN_WAITING_FOR_BREAKPOINT_TRIGGER)
    EV_HANDLER(BreakPointTriggered, HMM_h69)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_ALIGN_SCAN_WAITING_FOR_MEASUREMENT_DATABLOCK)
    EV_HANDLER(MeasurementBlockReceived, HMM_h70)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadMachinesManager, _HMM_ALIGN_SCAN_WAITING_FOR_CONTINUOUS_MOVE_DONE)
    EV_HANDLER(ContinuousMoveDone, HMM_h71)
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
    STATE_MATRIX_ENTRY(_HMM_MEASURE_ON_THE_SPOT_MEASUREMENT_FLASH_RECHARGE_WAIT),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_AT_TLAMP_MOVING_TO_TLAMP),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_AT_TLAMP_MOVE_SETTLE_WAIT),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_AT_TLAMP_WAITING_FOR_HEAD_REPLY),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_AT_TLAMP_WAITING_FOR_MEASUREMENT_DATABLOCK),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_AT_TLAMP_MOVING_BACK_TO_ORIGIN),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_AT_TLAMP_FINAL_MOVE_SETTLE_WAIT),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_AT_TLAMP_MEASUREMENT_FLASH_RECHARGE_WAIT),
    STATE_MATRIX_ENTRY(_HMM_CAL_AT_WHITE_MOVING_TO_WHITE_PLAQUE),
    STATE_MATRIX_ENTRY(_HMM_CAL_AT_WHITE_WAITING_FOR_HEAD_REPLY),
    STATE_MATRIX_ENTRY(_HMM_CAL_AT_WHITE_WAITING_FOR_MEASUREMENT_DATABLOCK),
    STATE_MATRIX_ENTRY(_HMM_CAL_AT_WHITE_MEASUREMENT_FLASH_RECHARGE_WAIT),
    STATE_MATRIX_ENTRY(_HMM_CAL_AT_BLACK_MOVING_TO_BLACK_HOLE),
    STATE_MATRIX_ENTRY(_HMM_CAL_AT_BLACK_WAITING_FOR_HEAD_REPLY),
    STATE_MATRIX_ENTRY(_HMM_CAL_AT_BLACK_WAITING_FOR_MEASUREMENT_DATABLOCK),
    STATE_MATRIX_ENTRY(_HMM_CAL_AT_BLACK_MEASUREMENT_FLASH_RECHARGE_WAIT),
    STATE_MATRIX_ENTRY(_HMM_TARGET_ALIGN_SCAN_MOVING_TO_NEXT_POINT),
    STATE_MATRIX_ENTRY(_HMM_TARGET_ALIGN_SCAN_MOVE_SETTLE_WAIT),
    STATE_MATRIX_ENTRY(_HMM_TARGET_ALIGN_SCAN_MEASUREMENT_FLASH_RECHARGE_WAIT),
    STATE_MATRIX_ENTRY(_HMM_TARGET_ALIGN_SCAN_WAITING_FOR_HEAD_REPLY),
    STATE_MATRIX_ENTRY(_HMM_TARGET_ALIGN_SCAN_WAITING_FOR_MEASUREMENT_DATABLOCK),
    STATE_MATRIX_ENTRY(_HMM_PT_TO_PT_SCAN_MOVING_TO_NEXT_POINT),
    STATE_MATRIX_ENTRY(_HMM_PT_TO_PT_SCAN_MOVE_SETTLE_WAIT),
    STATE_MATRIX_ENTRY(_HMM_PT_TO_PT_SCAN_MEASUREMENT_FLASH_RECHARGE_WAIT),
    STATE_MATRIX_ENTRY(_HMM_PT_TO_PT_SCAN_WAITING_FOR_HEAD_REPLY),
    STATE_MATRIX_ENTRY(_HMM_PT_TO_PT_SCAN_WAITING_FOR_MEASUREMENT_DATABLOCK),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_AT_POINT_MEASUREMENT_FLASH_RECHARGE_WAIT),
    STATE_MATRIX_ENTRY(_HMM_COLORBAR_SCAN_GOING_TO_STARTING_POSITION),
    STATE_MATRIX_ENTRY(_HMM_COLORBAR_SCAN_WAITING_FOR_BREAKPOINT_TRIGGER),
    STATE_MATRIX_ENTRY(_HMM_COLORBAR_SCAN_WAITING_FOR_MEASUREMENT_DATABLOCK),
    STATE_MATRIX_ENTRY(_HMM_COLORBAR_SCAN_WAITING_FOR_CONTINUOUS_MOVE_DONE),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_AT_POINT_MOVING_TO_POINT),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_AT_POINT_MOVE_SETTLE_WAIT),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_AT_POINT_WAITING_FOR_HEAD_REPLY),
    STATE_MATRIX_ENTRY(_HMM_MEASURE_AT_POINT_WAITING_FOR_MEASUREMENT_DATABLOCK),
    STATE_MATRIX_ENTRY(_HMM_ALIGN_SCAN_GOING_TO_STARTING_POSITION),
    STATE_MATRIX_ENTRY(_HMM_ALIGN_SCAN_WAITING_FOR_BREAKPOINT_TRIGGER),
    STATE_MATRIX_ENTRY(_HMM_ALIGN_SCAN_WAITING_FOR_MEASUREMENT_DATABLOCK),
    STATE_MATRIX_ENTRY(_HMM_ALIGN_SCAN_WAITING_FOR_CONTINUOUS_MOVE_DONE),
RESPONSE_TABLE_END;




//////////////////////////////////////////////////
//
// Static Member Definitions
//
//////////////////////////////////////////////////

TableControl                * HeadMachinesManager::ptrTC = 0;
MotorCommMachine            * HeadMachinesManager::ptrMTRC = 0;
HeadCommandCommMachine      * HeadMachinesManager::ptrHCC = 0;
MeasurementFlashTimer       * HeadMachinesManager::ptrMFT = 0;

WORD    HeadMachinesManager::scanStartIndex = 0;
WORD    HeadMachinesManager::scanEndIndex = 0;
WORD    HeadMachinesManager::currScanIndex = 0;


TargetAlignmentData     HeadMachinesManager::targetAlignmentData;
AlignmentScanData       HeadMachinesManager::alignmentScanData;

ALIGN_SCAN_DIRECTION    HeadMachinesManager::currAlignDirection = ALIGN_SCAN_RIGHT;
COLOR_BAR_DIRECTION     HeadMachinesManager::currColorBarDirection = COLOR_BAR_LEFT;


TableParametersDataManager  * HeadMachinesManager::ptrTPDM = 0;
MeasurementCommMachine      * HeadMachinesManager::ptrMCM = 0;
MeasurementDataManager      * HeadMachinesManager::ptrMDM = 0;
ScanParametersDataManager   * HeadMachinesManager::ptrSPDM = 0;

int     HeadMachinesManager::xOrigin = 0;
int     HeadMachinesManager::yOrigin = 0;

WORD    HeadMachinesManager::pointsLeftToScan = 0;

BOOL    HeadMachinesManager::useFlashForWhitePlaqueMeasurement = TRUE;

int     HeadMachinesManager::endingCoordinateX = 0;
int     HeadMachinesManager::endingCoordinateY = 0;
int     HeadMachinesManager::startingCoordinateX = 0;
int     HeadMachinesManager::startingCoordinateY = 0;
int     HeadMachinesManager::requestedPointCoordinateX = 0;
int     HeadMachinesManager::requestedPointCoordinateY = 0;
int     HeadMachinesManager::prevPointCoordinateX = 0;
int     HeadMachinesManager::prevPointCoordinateY = 0;

int     HeadMachinesManager::breakPointXCoordinate = 0;
int     HeadMachinesManager::breakPointYCoordinate = 0;

STATE_MACHINE_ID        HeadMachinesManager::requestorID = NULL_SM_ID;

//////////////////////////////////////////////////
//
// HeadMachinesManager - Constructors, Destructors
//
//////////////////////////////////////////////////

HeadMachinesManager::HeadMachinesManager(STATE_MACHINE_ID sMsysID)
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

        //Debug
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

        //Debug
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

        //Debug
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

                //Debug
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

        //Debug
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
//           (NO Backlash Compensation)
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h11(void) {

    // Remember who made the request

    requestorID = GetCurrEvent().senderId;

    // Get the data from the message

    requestedPointCoordinateX = (int)(GetCurrEvent().msgData1);
    requestedPointCoordinateY = (int)(GetCurrEvent().msgData2);

        // Make sure to leave table backlash

        ptrMTRC->DisableBacklashCompensation();

        SendHiPrMsg(MotorCommID,  GotoXYTarget,
                    requestedPointCoordinateX, requestedPointCoordinateY);

    return HMM_MOVING_TO_TARGET;
}


//////////////////////////////////////////////////
//
//  Message: Move To Position
//           With Backlash Compensation
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h11a(void) {

    // Remember who made the request

    requestorID = GetCurrEvent().senderId;

    // Get the data from the message

    requestedPointCoordinateX = (int)(GetCurrEvent().msgData1);
    requestedPointCoordinateY = (int)(GetCurrEvent().msgData2);

        // Make sure to remove table backlash

        ptrMTRC->EnableBacklashCompensation();

        SendHiPrMsg(MotorCommID,  GotoXYTarget,
                    requestedPointCoordinateX, requestedPointCoordinateY);

    return HMM_MOVING_TO_TARGET;
}


//////////////////////////////////////////////////
//
//  Message: XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h12(void) {

        // Use a generic settle time

        StartHiPriorityTimer(ptrTPDM->GetReverseBacklashSettleTime());

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

        // Debug
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

        ptrHCC->SetTransmitBuffer(PHC_FlashMeasurementLamp);

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

        SendHiPrMsg(MeasurementFlashTimerID, StartMeasurementFlashTimer,
                    ptrTPDM->GetMeasurementFlashRechargeTime(), NULL_MESSAGE_ID);

    return HMM_MEASUREMENT_FLASH_RECHARGE_WAIT;
}


//////////////////////////////////////////////////
//
//  Message: Measurement Flash Timer Expired
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

        // Tell MCM to be ready for the Measurement data

        SendHiPrMsg(MeasurementCommMachineID, WaitForMeasurementBlock);

        // Setup transmit buffer and request HCC
        // make a transmission

        ptrHCC->SetTransmitBuffer(PHC_MeasureWithFlash);

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

        // Begin timing the Meaurement Flash recharge
        // to make sure flash will be ready for the next command

        SendHiPrMsg(MeasurementFlashTimerID, StartMeasurementFlashTimer,
                    ptrTPDM->GetMeasurementFlashRechargeTime(), NULL_MESSAGE_ID);

        // Just wait for the completion of the
        // Measurement data transmission

    return HMM_MEASURE_ON_THE_SPOT_WAITING_FOR_MEASUREMENT_DATABLOCK;
}


//////////////////////////////////////////////////
//
//  Message: Measurement Block Received
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h29(void) {

        // Get the Measurement data from MCM
        // and store it using MDM

        ptrMDM->PutNext(ptrMCM->GetScanMeasurementData());

    return HMM_MEASURE_ON_THE_SPOT_MEASUREMENT_FLASH_RECHARGE_WAIT;
}


//////////////////////////////////////////////////
//
//  Message: Measurement Flash Timer Expired
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h29a(void) {

        // Get the Measurement data from MCM
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
    // on current Measurement position

    int tLampXCoordinate = xOrigin +
            ptrTPDM->GetTargetLampOffset().GetXCoordinate();

    int tLampYCoordinate = yOrigin +
            ptrTPDM->GetTargetLampOffset().GetYCoordinate();

        // Make sure to remove table backlash

        ptrMTRC->EnableBacklashCompensation();

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

    // Tell MCM to be ready for the Measurement data

    SendHiPrMsg(MeasurementCommMachineID, WaitForMeasurementBlock);

    WORD    moveSettleDelay = ptrTPDM->GetPointToPointMoveSettleTime();

        if(moveSettleDelay != 0)
        {
               StartHiPriorityTimer(moveSettleDelay);

            return HMM_MEASURE_AT_TLAMP_MOVE_SETTLE_WAIT;
        }


        // Setup transmit buffer and request HCC
        // make a transmission

        ptrHCC->SetTransmitBuffer(PHC_MeasureWithFlash);

        SendHiPrMsg(HeadCommandCommID, SendProbeHeadCommand);

    return HMM_MEASURE_AT_TLAMP_WAITING_FOR_HEAD_REPLY;
}


//////////////////////////////////////////////////
//
//  Message: TimeOut
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h32(void) {

        // Setup transmit buffer and request HCC
        // make a transmission

        ptrHCC->SetTransmitBuffer(PHC_MeasureWithFlash);

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

        // Begin timing the Meaurement Flash recharge
        // to make sure flash will be ready for the next command

        SendHiPrMsg(MeasurementFlashTimerID, StartMeasurementFlashTimer,
                    ptrTPDM->GetMeasurementFlashRechargeTime(), NULL_MESSAGE_ID);

        // Just wait for the completion of the
        // Measurement data transmission

    return HMM_MEASURE_AT_TLAMP_WAITING_FOR_MEASUREMENT_DATABLOCK;
}


//////////////////////////////////////////////////
//
//  Message: Measurement Block Received
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h34(void) {

        // Get the Measurement data from MCM
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

    WORD    moveSettleDelay = ptrTPDM->GetPointToPointMoveSettleTime();

        if(moveSettleDelay != 0)
        {
                StartHiPriorityTimer(moveSettleDelay);

            return HMM_MEASURE_AT_TLAMP_FINAL_MOVE_SETTLE_WAIT;
        }

        if(ptrMFT->IsMeasurementFlashTimerExpired())
        {
                // Inform the HOC about the completion of the task

                SendHiPrMsg(HeadOperationsCoordinatorMachineID, MeasureAtTargetLampDone);

            return HMM_IDLE;
        }

    return HMM_MEASURE_AT_TLAMP_MEASUREMENT_FLASH_RECHARGE_WAIT;
}

//////////////////////////////////////////////////
//
//  Message: TimeOut
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h36(void) {

        if(ptrMFT->IsMeasurementFlashTimerExpired() == FALSE)
            return HMM_MEASURE_AT_TLAMP_MEASUREMENT_FLASH_RECHARGE_WAIT;

        // Inform the HOC about the completion of the task

        SendHiPrMsg(HeadOperationsCoordinatorMachineID, MeasureAtTargetLampDone);

    return HMM_IDLE;
}


//////////////////////////////////////////////////
//
//  Message: Measurement Flash Timer Expired
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h36a(void) {

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

    useFlashForWhitePlaqueMeasurement = (BOOL)GetCurrEvent().msgData1;

    requestedPointCoordinateX = ptrTPDM->GetWhitePlaqueLocation().GetXCoordinate();
    requestedPointCoordinateY = ptrTPDM->GetWhitePlaqueLocation().GetYCoordinate();

        // Tell MTRC to Move to the White Plaque position

        SendHiPrMsg(MotorCommID,  GotoXTarget,
                    requestedPointCoordinateX, requestedPointCoordinateY);

    return HMM_CAL_AT_WHITE_MOVING_TO_WHITE_PLAQUE;
}


//////////////////////////////////////////////////
//
//  Message: XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h38(void) {

        // Tell MCM to be ready for the Measurement data

        SendHiPrMsg(MeasurementCommMachineID, WaitForMeasurementBlock);

        // Setup transmit buffer and request HCC
        // make a transmission

        if(useFlashForWhitePlaqueMeasurement)
            ptrHCC->SetTransmitBuffer(PHC_MeasureWithFlash);
        else
            ptrHCC->SetTransmitBuffer(PHC_MeasureWithOutFlash);

        SendHiPrMsg(HeadCommandCommID, SendProbeHeadCommand);

    return HMM_CAL_AT_WHITE_WAITING_FOR_HEAD_REPLY;
}


//////////////////////////////////////////////////
//
//  Message: Probe Head Command Acked
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h39(void) {

        // Begin timing the Meaurement Flash recharge
        // to make sure flash will be ready for the next command

        SendHiPrMsg(MeasurementFlashTimerID, StartMeasurementFlashTimer,
                    ptrTPDM->GetMeasurementFlashRechargeTime(), NULL_MESSAGE_ID);

    // Wait for the completion of the Measurement data transmission

    return HMM_CAL_AT_WHITE_WAITING_FOR_MEASUREMENT_DATABLOCK;
}


//////////////////////////////////////////////////
//
//  Message: Measurement Block Received
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h40(void) {

        // Get the Measurement data from MCM
        // and store it using MDM

        ptrMDM->PutNext(ptrMCM->GetScanMeasurementData());

    return HMM_CAL_AT_WHITE_MEASUREMENT_FLASH_RECHARGE_WAIT;
}


//////////////////////////////////////////////////
//
//  Message:  Measurement Flash Timer Expired
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h41(void) {

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
    requestedPointCoordinateX = ptrTPDM->GetBlackHoleLocation().GetXCoordinate();
    requestedPointCoordinateY = ptrTPDM->GetBlackHoleLocation().GetYCoordinate();

        // Tell MTRC to Move to the Black Hole position

        SendHiPrMsg(MotorCommID,  GotoXTarget,
                    requestedPointCoordinateX, requestedPointCoordinateY);

    return HMM_CAL_AT_BLACK_MOVING_TO_BLACK_HOLE;
}


//////////////////////////////////////////////////
//
//  Message: XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h43(void) {

        // Tell MCM to be ready for the Measurement data

        SendHiPrMsg(MeasurementCommMachineID, WaitForMeasurementBlock);

        SendHiPrMsg(HeadCommandCommID, SendProbeHeadCommand);

    return HMM_CAL_AT_BLACK_WAITING_FOR_HEAD_REPLY;
}


//////////////////////////////////////////////////
//
//  Message: Probe Head Command Acked
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h44(void) {

        // Begin timing the Meaurement Flash recharge
        // to make sure flash will be ready for the next command

        SendHiPrMsg(MeasurementFlashTimerID, StartMeasurementFlashTimer,
                    ptrTPDM->GetMeasurementFlashRechargeTime(), NULL_MESSAGE_ID);

    // Wait for the completion of the Measurement data transmission

    return HMM_CAL_AT_BLACK_WAITING_FOR_MEASUREMENT_DATABLOCK;
}


//////////////////////////////////////////////////
//
//  Message: Measurement Block Received
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h45(void) {

        // Get the Measurement data from MCM
        // and store it using MDM

        ptrMDM->PutNext(ptrMCM->GetScanMeasurementData());

    return HMM_CAL_AT_BLACK_MEASUREMENT_FLASH_RECHARGE_WAIT;
}


//////////////////////////////////////////////////
//
//  Message: Measurement Flash Timer Expired
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h46(void) {

        // Inform HOC about the completion of the request

        SendHiPrMsg(HeadOperationsCoordinatorMachineID, CalibrateAtBlackHoleDone);

    return HMM_IDLE;
}


//////////////////////////////////////////////////
//
//  Message: Do Target Alignment Scan
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h47(void) {

    startingCoordinateX = targetAlignmentData.alignmentStart.GetXCoordinate();
    startingCoordinateY = targetAlignmentData.alignmentStart.GetYCoordinate();

        // Determine the alignment scan direction
        // X Axis or Y axis direction

        if(startingCoordinateX == targetAlignmentData.alignmentEnd.GetXCoordinate())
        {
            // determine the x axis direction, LEFT or RIGHT

            if(startingCoordinateX < targetAlignmentData.alignmentEnd.GetXCoordinate())
                    currAlignDirection = ALIGN_SCAN_RIGHT;
                else
                    currAlignDirection = ALIGN_SCAN_LEFT;

            // Initialize pointsLeftToScan counter, towards positive x axis

            pointsLeftToScan =
                abs(startingCoordinateX - targetAlignmentData.alignmentEnd.GetXCoordinate());
        }
        else
        {
            // determine the y axis direction

            if(startingCoordinateY < targetAlignmentData.alignmentEnd.GetYCoordinate())
                    currAlignDirection = ALIGN_SCAN_UP;
                else
                    currAlignDirection = ALIGN_SCAN_DOWN;

            // Initialize pointsLeftToScan counter, towards negative y axis

            pointsLeftToScan =
                abs(startingCoordinateY - targetAlignmentData.alignmentEnd.GetYCoordinate());
        }

        // Tell MTRC to Move to the first point

        SendHiPrMsg(MotorCommID, GotoXTarget, startingCoordinateX, startingCoordinateY);

    return HMM_TARGET_ALIGN_SCAN_MOVING_TO_NEXT_POINT;
}


//////////////////////////////////////////////////
//
//  Message: XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h48(void) {

        StartHiPriorityTimer(ptrTPDM->GetTargetAlignMoveSettleTime());

    return HMM_TARGET_ALIGN_SCAN_MOVE_SETTLE_WAIT;
}


//////////////////////////////////////////////////
//
//  Message: TimeOut
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h48a(void) {

        // Tell MCM to be ready for the Measurement data

        SendHiPrMsg(MeasurementCommMachineID, WaitForMeasurementBlock);

        if(ptrMFT->IsMeasurementFlashTimerExpired())
        {
                // Setup transmit buffer and request HCC
                // make a transmission

                ptrHCC->SetTransmitBuffer(PHC_MeasureWithFlash);

                SendHiPrMsg(HeadCommandCommID, SendProbeHeadCommand);

            return HMM_TARGET_ALIGN_SCAN_WAITING_FOR_HEAD_REPLY;
        }

    return HMM_TARGET_ALIGN_SCAN_MEASUREMENT_FLASH_RECHARGE_WAIT;
}



//////////////////////////////////////////////////
//
//  Message: Measurement Flash Timer Expired
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h49(void) {

        // Measurement Flash Is Ready

        // Setup transmit buffer and request HCC
        // make a transmission

        ptrHCC->SetTransmitBuffer(PHC_MeasureWithFlash);

        SendHiPrMsg(HeadCommandCommID, SendProbeHeadCommand);

    return HMM_TARGET_ALIGN_SCAN_WAITING_FOR_HEAD_REPLY;
}


//////////////////////////////////////////////////
//
//  Message: Probe Head Command Acked
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h50(void) {

        // Begin timing the Meaurement Flash recharge
        // to make sure flash will be ready for the next command

        SendHiPrMsg(MeasurementFlashTimerID, StartMeasurementFlashTimer,
                    ptrTPDM->GetMeasurementFlashRechargeTime(), NULL_MESSAGE_ID);

        // Wait for the completion of the
        // Measurement data transmission

    return HMM_TARGET_ALIGN_SCAN_WAITING_FOR_MEASUREMENT_DATABLOCK;
}


//////////////////////////////////////////////////
//
//  Message: Measurement Block Received
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h51(void) {

            // Get the Measurement data from MCM
            // and store it using MDM

            ptrMDM->PutNext(ptrMCM->GetScanMeasurementData());

            --pointsLeftToScan;

            if(pointsLeftToScan != 0)   // more ?
            {
                    switch(currAlignDirection)
                    {
                        case ALIGN_SCAN_UP  :   requestedPointCoordinateX = startingCoordinateX;
                                                requestedPointCoordinateY++;
                                                break;
                        case ALIGN_SCAN_RIGHT : requestedPointCoordinateX++;
                                                requestedPointCoordinateY = startingCoordinateY;
                                                break;
                        case ALIGN_SCAN_DOWN  : requestedPointCoordinateX = startingCoordinateX;
                                                requestedPointCoordinateY--;
                                                break;
                            default : // ALIGN_SCAN_LEFT
                                                requestedPointCoordinateX--;
                                                requestedPointCoordinateY = startingCoordinateY;
                                                break;
                    }

                    // Tell MTRC to Move to the next point

                    SendHiPrMsg(MotorCommID, GotoXTarget,
                                requestedPointCoordinateX, requestedPointCoordinateY);

                return  HMM_TARGET_ALIGN_SCAN_MOVING_TO_NEXT_POINT;
            }

            CancelTimer(); // Measurement Flash recharge Timer

            // Inform HOC about the completion of the request

            SendHiPrMsg(HeadOperationsCoordinatorMachineID, TargetAlignmentScanDone);

        return HMM_IDLE;
}


//////////////////////////////////////////////////
//
//  Message: Point To Point Scan
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h52(void) {

        currScanIndex  =
        scanStartIndex = (int)(GetCurrEvent().msgData1);

        scanEndIndex   = (int)(GetCurrEvent().msgData2);

        prevPointCoordinateX      =
        requestedPointCoordinateX = ptrSPDM->GetAt(currScanIndex).GetXCoordinate();

        prevPointCoordinateY      =
        requestedPointCoordinateY = ptrSPDM->GetAt(currScanIndex).GetYCoordinate();

        // Initialize pointsLeftToScan counter

        pointsLeftToScan = scanEndIndex - scanStartIndex;

            // First Point To Point Scan Coordinate
            // Make sure to remove table backlash

            ptrMTRC->EnableBacklashCompensation();

            // Tell MTRC to Move to the first point

            SendHiPrMsg(MotorCommID, GotoXTarget,
                        requestedPointCoordinateX, requestedPointCoordinateY);

    return HMM_PT_TO_PT_SCAN_MOVING_TO_NEXT_POINT;
}


//////////////////////////////////////////////////
//
//  Message: XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h53(void) {

    // Tell MCM to be ready for the Measurement data

    SendHiPrMsg(MeasurementCommMachineID, WaitForMeasurementBlock);

    WORD moveSettleDelay = ptrTPDM->GetPointToPointMoveSettleTime();

        if(moveSettleDelay != 0)
        {
                StartHiPriorityTimer(moveSettleDelay);

            return HMM_PT_TO_PT_SCAN_MOVE_SETTLE_WAIT;
        }

        if(ptrMFT->IsMeasurementFlashTimerExpired())
        {
                // Setup transmit buffer and request HCC
                // make a transmission

                ptrHCC->SetTransmitBuffer(PHC_MeasureWithFlash);

                SendHiPrMsg(HeadCommandCommID, SendProbeHeadCommand);

            return HMM_PT_TO_PT_SCAN_WAITING_FOR_HEAD_REPLY;
        }

    return HMM_PT_TO_PT_SCAN_MEASUREMENT_FLASH_RECHARGE_WAIT;
}


//////////////////////////////////////////////////
//
//  Message: TimeOut
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h53a(void) {

        if(ptrMFT->IsMeasurementFlashTimerExpired())
        {
                // Setup transmit buffer and request HCC
                // make a transmission

                ptrHCC->SetTransmitBuffer(PHC_MeasureWithFlash);

                SendHiPrMsg(HeadCommandCommID, SendProbeHeadCommand);

            return HMM_PT_TO_PT_SCAN_WAITING_FOR_HEAD_REPLY;
        }

    return HMM_PT_TO_PT_SCAN_MEASUREMENT_FLASH_RECHARGE_WAIT;
}


//////////////////////////////////////////////////
//
//  Message: Measurement Flash Timer Expired
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h54(void) {

        // Measurement Flash Is Ready

        // Setup transmit buffer and request HCC
        // make a transmission

        ptrHCC->SetTransmitBuffer(PHC_MeasureWithFlash);

        SendHiPrMsg(HeadCommandCommID, SendProbeHeadCommand);

    return HMM_PT_TO_PT_SCAN_WAITING_FOR_HEAD_REPLY;
}


//////////////////////////////////////////////////
//
//  Message: Probe Head Command Acked
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h55(void) {

        // Begin timing the Meaurement Flash recharge
        // to make sure flash will be ready for the next command

        SendHiPrMsg(MeasurementFlashTimerID, StartMeasurementFlashTimer,
                    ptrTPDM->GetMeasurementFlashRechargeTime(), NULL_MESSAGE_ID);

        // Wait for the completion of the
        // Measurement data transmission

    return HMM_PT_TO_PT_SCAN_WAITING_FOR_MEASUREMENT_DATABLOCK;
}


//////////////////////////////////////////////////
//
//  Message: Measurement Block Received
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h56(void) {

            // Inform HOC that a point was just measured

            SendHiPrMsg(HeadOperationsCoordinatorMachineID, PointMeasured);

            // Get the Measurement data from MCM
            // and store it using MDM

            ptrMDM->PutNext(ptrMCM->GetScanMeasurementData());

            // Update counters and index

            --pointsLeftToScan;
            ++currScanIndex;

            if(pointsLeftToScan != 0)   // more ?
            {
                    requestedPointCoordinateX = ptrSPDM->GetAt(currScanIndex).GetXCoordinate();
                    requestedPointCoordinateY = ptrSPDM->GetAt(currScanIndex).GetYCoordinate();

                    // Check if next point coordinate does not need backlash compensation
                    // by checking if the next move is an "anti-backlash" move, ie
                    // Towards Positive X and Negative Y

                    if((requestedPointCoordinateX >= prevPointCoordinateX) &&
                       (requestedPointCoordinateY <= prevPointCoordinateY))
                            ptrMTRC->DisableBacklashCompensation();
                        else
                            ptrMTRC->EnableBacklashCompensation();


                    // Update prev values

                    prevPointCoordinateX = requestedPointCoordinateX;
                    prevPointCoordinateY = requestedPointCoordinateY;

                    // Tell MTRC to Move to the next point

                    SendHiPrMsg(MotorCommID, GotoXTarget,
                                requestedPointCoordinateX, requestedPointCoordinateY);

                return  HMM_PT_TO_PT_SCAN_MOVING_TO_NEXT_POINT;
            }

            // Inform HOC about the completion of the request

            SendHiPrMsg(HeadOperationsCoordinatorMachineID, PointToPointScanDone);

        return HMM_IDLE;
}


//////////////////////////////////////////////////
//
//  Message: Do Color Bar Scan
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h57(void) {

    //
    // IMPORTANT : Set Kernel to be ready for the
    //             Continuous Scan Measurements
    //

    SetMeasurementMode(CONTINOUS_SCAN_MEASUREMENT);


    // Get the message data

    currScanIndex  =
    scanStartIndex = (int)(GetCurrEvent().msgData1);

    scanEndIndex   = (int)(GetCurrEvent().msgData2);

    // Determine direction and the probe head's starting point
    // Also note the first breakpoint

    startingCoordinateX = breakPointXCoordinate =
        ptrSPDM->GetAt(scanStartIndex).GetXCoordinate();

    startingCoordinateY =
        ptrSPDM->GetAt(scanStartIndex).GetYCoordinate();

        if(startingCoordinateX < endingCoordinateX)
        {
            currColorBarDirection = COLOR_BAR_RIGHT;

            // Calculate X starting offset

            startingCoordinateX -= (ptrTPDM->GetColorBarStartOffsetX());
        }
        else
        {
            currColorBarDirection = COLOR_BAR_LEFT;

            // Calculate X starting offset

            startingCoordinateX += (ptrTPDM->GetColorBarStartOffsetX());
        }

        // Initialize pointsLeftToScan counter

        pointsLeftToScan = scanEndIndex - scanStartIndex;

        // Backlash compensation needed to move to the starting position

        ptrMTRC->EnableBacklashCompensation();

        //
        //SendHiPrMsg(MotorCommID, PROFILECHANGE);
        //

        // Tell MTRC to Move to the starting position

        SendHiPrMsg(MotorCommID, GotoXTarget,
                    startingCoordinateX, startingCoordinateY);

    return HMM_COLORBAR_SCAN_GOING_TO_STARTING_POSITION;
}


//////////////////////////////////////////////////
//
//  Message:  XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h58(void) {

    endingCoordinateX = ptrSPDM->GetAt(scanEndIndex).GetXCoordinate();

    // Initialize Y coordinate which may be adjusted
    // while in the process of the continuous move

    requestedPointCoordinateY =
    endingCoordinateY         = ptrSPDM->GetAt(scanEndIndex).GetYCoordinate();

        // Calculate X ending offset based on the
        // color bar scan direction

        if(currColorBarDirection == COLOR_BAR_RIGHT)
        {
            endingCoordinateX += ptrTPDM->GetColorBarEndOffsetX();
        }
        else
        {
            // currColorBarDirection == COLOR_BAR_LEFT

            endingCoordinateX -= ptrTPDM->GetColorBarEndOffsetX();
        }

        // Tell HCC to be in Continuous Scan mode

        SendHiPrMsg(HeadCommandCommID, StartContinuousScanMode);

        // Tell MCM to be ready for the Measurement data

        SendHiPrMsg(MeasurementCommMachineID, WaitForMeasurementBlock);

        // Debug
        //
        //SendHiPrMsg(MotorCommID, PROFILECHANGE);
        //

        // Backlash not needed anymore for the continuous move

        ptrMTRC->DisableBacklashCompensation();

        // Tell MTRC to begin the color bar move

        SendHiPrMsg(MotorCommID, StartContinuousMove,
                    endingCoordinateX, endingCoordinateY);

        // Set First Break point

        SendHiPrMsg(MotorCommID, SetBreakPointX,
                    breakPointXCoordinate, NULL_MESSAGE_ID);

    return HMM_COLORBAR_SCAN_WAITING_FOR_BREAKPOINT_TRIGGER;
}


//////////////////////////////////////////////////
//
//  Message: Break Point Triggered
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h59(void) {

        // Just wait for the completion of the
        // Measurement data transmission

    return HMM_COLORBAR_SCAN_WAITING_FOR_MEASUREMENT_DATABLOCK;
}


//////////////////////////////////////////////////
//
//  Message: Measurement Block Received
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h60(void) {

            // Inform HOC that a point was just measured

            SendHiPrMsg(HeadOperationsCoordinatorMachineID, PointMeasured);

            // Get the Measurement data from MCM
            // and store it using MDM

            ptrMDM->PutNext(ptrMCM->GetScanMeasurementData());

            // Update counters and index

            --pointsLeftToScan;
            ++currScanIndex;

            if(pointsLeftToScan != 0)   // more ?
            {
                    // Y Axis Changed ?

                    prevPointCoordinateY = requestedPointCoordinateY;
                    requestedPointCoordinateY = ptrSPDM->GetAt(currScanIndex).GetYCoordinate();

                    if(requestedPointCoordinateY != prevPointCoordinateY)
                    {
                        // Tell MTRC to make a Y axis adjustment

                        SendHiPrMsg(MotorCommID, AdjustYTarget,
                                    requestedPointCoordinateY, NULL_MESSAGE_ID);
                    }

                    // Tell MCM to be ready for the Measurement data

                    SendHiPrMsg(MeasurementCommMachineID, WaitForMeasurementBlock);

                    // Set NEW Motor Chipset Break Points

                    breakPointXCoordinate = ptrSPDM->GetAt(currScanIndex).GetXCoordinate();

                    SendHiPrMsg(MotorCommID, SetBreakPointX,
                                breakPointXCoordinate, NULL_MESSAGE_ID);

                return  HMM_COLORBAR_SCAN_WAITING_FOR_BREAKPOINT_TRIGGER;
            }

    return HMM_COLORBAR_SCAN_WAITING_FOR_CONTINUOUS_MOVE_DONE;
}


//////////////////////////////////////////////////
//
//  Message: Continuous Move Done
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h61(void) {

        //
        // IMPORTANT : Set Kernel back to the
        //             Normal Scan Measurements mode
        //

        SetMeasurementMode(NORMAL_SCAN_MEASUREMENT);


        // Tell HCC to exit the Continuous Scan mode

        SendHiPrMsg(HeadCommandCommID, EndContinuousScanMode);

        // Inform HOC about the completion of the request

        SendHiPrMsg(HeadOperationsCoordinatorMachineID, ColorBarScanDone);

    return HMM_IDLE;
}


//////////////////////////////////////////////////
//
//  Message: Measure At Point
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h62(void) {

    int pointXCoordinate = (int)(GetCurrEvent().msgData1);
    int pointYCoordinate = (int)(GetCurrEvent().msgData2);

        // Check if we are already ON Target

        if(pointXCoordinate == ptrMTRC->GetCurrXCoordinate() &&
            pointYCoordinate == ptrMTRC->GetCurrYCoordinate())
        {
                // Already ON Target

                // Tell MCM to be ready for the Measurement data

                SendHiPrMsg(MeasurementCommMachineID, WaitForMeasurementBlock);

                // Setup transmit buffer and request HCC
                // make a transmission

                ptrHCC->SetTransmitBuffer(PHC_MeasureWithFlash);

                SendHiPrMsg(HeadCommandCommID, SendProbeHeadCommand);

            return  HMM_MEASURE_AT_POINT_WAITING_FOR_HEAD_REPLY;
        }

        // Make sure to remove table backlash

        ptrMTRC->EnableBacklashCompensation();

        // Tell MTRC to Move to

        SendHiPrMsg(MotorCommID,  GotoXTarget, pointXCoordinate, pointYCoordinate);

    return HMM_MEASURE_AT_POINT_MOVING_TO_POINT;
}


//////////////////////////////////////////////////
//
//  Message: XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h63(void) {

    // Tell MCM to be ready for the Measurement data

    SendHiPrMsg(MeasurementCommMachineID, WaitForMeasurementBlock);

    WORD    moveSettleDelay = ptrTPDM->GetPointToPointMoveSettleTime();

        if(moveSettleDelay != 0)
        {
                StartHiPriorityTimer(moveSettleDelay);

            return HMM_MEASURE_AT_POINT_MOVE_SETTLE_WAIT;
        }

        // Setup transmit buffer and request HCC
        // make a transmission

        ptrHCC->SetTransmitBuffer(PHC_MeasureWithFlash);

        SendHiPrMsg(HeadCommandCommID, SendProbeHeadCommand);

    return HMM_MEASURE_AT_POINT_WAITING_FOR_HEAD_REPLY;
}


//////////////////////////////////////////////////
//
//  Message: TimeOut
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h64(void) {

        // Setup transmit buffer and request HCC
        // make a transmission

        ptrHCC->SetTransmitBuffer(PHC_MeasureWithFlash);

        SendHiPrMsg(HeadCommandCommID, SendProbeHeadCommand);

    return HMM_MEASURE_AT_POINT_WAITING_FOR_HEAD_REPLY;
}


//////////////////////////////////////////////////
//
//  Message: Probe Head Command Acked
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h65(void) {

        // Begin timing the Meaurement Flash recharge
        // to make sure flash will be ready for the next command

        SendHiPrMsg(MeasurementFlashTimerID, StartMeasurementFlashTimer,
                    ptrTPDM->GetMeasurementFlashRechargeTime(), NULL_MESSAGE_ID);

        // Wait for the completion of the
        // Measurement data transmission

    return HMM_MEASURE_AT_POINT_WAITING_FOR_MEASUREMENT_DATABLOCK;
}


//////////////////////////////////////////////////
//
//  Message: Measurement Block Received
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h66(void) {

        // Get the Measurement data from MCM
        // and store it using MDM

        ptrMDM->PutNext(ptrMCM->GetScanMeasurementData());

    return HMM_MEASURE_AT_POINT_MEASUREMENT_FLASH_RECHARGE_WAIT;
}


//////////////////////////////////////////////////
//
//  Message: Measurement Flash Timer Expired
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h66a(void) {

        // Inform HOC about the completion of the request

        SendHiPrMsg(HeadOperationsCoordinatorMachineID, MeasureAtPointDone);

    return HMM_IDLE;
}


//////////////////////////////////////////////////
//
//  Message: Do Alignment Scan
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h67(void) {

    //
    // IMPORTANT : Set Kernel to be ready for the
    //             Continuous Scan Measurements
    //

    SetMeasurementMode(CONTINOUS_SCAN_MEASUREMENT);


    // Determine alignment scan direction and the probe head's starting point
    // RIGHT or DOWN. Also note the first breakpoint

    startingCoordinateX = alignmentScanData.alignmentStart.GetXCoordinate();

    startingCoordinateY = alignmentScanData.alignmentStart.GetYCoordinate();

    endingCoordinateX = alignmentScanData.alignmentEnd.GetXCoordinate();

    endingCoordinateY = alignmentScanData.alignmentEnd.GetYCoordinate();

        if(startingCoordinateX == alignmentScanData.alignmentEnd.GetXCoordinate())
        {
            currAlignDirection = ALIGN_SCAN_RIGHT;

            // Initialize pointsLeftToScan counter

            pointsLeftToScan
                = (endingCoordinateX - startingCoordinateX) / alignmentScanData.alignmentSteps;

            // initialize starting break point

            breakPointXCoordinate = startingCoordinateX;

            // Calculate X starting offset

            startingCoordinateX -= (ptrTPDM->GetAlignScanStartOffset());
        }
        else
        {
            currAlignDirection = ALIGN_SCAN_DOWN;

            // Initialize pointsLeftToScan counter

            pointsLeftToScan = 1 +
                ((startingCoordinateY - endingCoordinateY) / alignmentScanData.alignmentSteps);

            // initialize starting break point

            breakPointYCoordinate = startingCoordinateY;

            // Calculate X starting offset

            startingCoordinateY += (ptrTPDM->GetAlignScanStartOffset());
        }

        // Backlash compensation needed to move to the starting position

        ptrMTRC->EnableBacklashCompensation();

        //
        //SendHiPrMsg(MotorCommID, PROFILECHANGE);
        //

        // Tell MTRC to Move to the starting position

        SendHiPrMsg(MotorCommID, GotoXTarget,
                    startingCoordinateX, startingCoordinateY);

    return HMM_ALIGN_SCAN_GOING_TO_STARTING_POSITION;
}


//////////////////////////////////////////////////
//
//  Message:  XYAxis On Target
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h68(void) {

        // Calculate ending offset based on the align scan direction

        if(currAlignDirection == ALIGN_SCAN_RIGHT)
        {
            endingCoordinateX += ptrTPDM->GetAlignScanEndOffset();

            endingCoordinateY = startingCoordinateY; // Y axis stays the same
        }
        else
        {
            // currAlignDirection == ALIGN_SCAN_DOWN

            endingCoordinateY -= ptrTPDM->GetAlignScanEndOffset();

            endingCoordinateX = startingCoordinateX; // X axis stays the same
        }

        // Tell HCC to be in Continuous Scan mode

        SendHiPrMsg(HeadCommandCommID, StartContinuousScanMode);

        // Tell MCM to be ready for the Measurement data

        SendHiPrMsg(MeasurementCommMachineID, WaitForMeasurementBlock);

        //
        //SendHiPrMsg(MotorCommID, PROFILECHANGE);
        //

        // Backlash not needed anymore for the continuous move

        ptrMTRC->DisableBacklashCompensation();

        // Tell MTRC to begin the color bar move

        SendHiPrMsg(MotorCommID, StartContinuousMove,
                    endingCoordinateX, endingCoordinateY);

        // Set First Break point based on align scan direction

        if(currAlignDirection == ALIGN_SCAN_RIGHT)
        {
            SendHiPrMsg(MotorCommID, SetBreakPointX,
                        breakPointXCoordinate, NULL_MESSAGE_ID);
        }
        else
        {
            // currAlignDirection == ALIGN_SCAN_DOWN

            SendHiPrMsg(MotorCommID, SetBreakPointY,
                        breakPointYCoordinate, NULL_MESSAGE_ID);
        }

    return HMM_ALIGN_SCAN_WAITING_FOR_BREAKPOINT_TRIGGER;
}


//////////////////////////////////////////////////
//
//  Message: Break Point Triggered
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h69(void) {

        // Just wait for the completion of the
        // Measurement data transmission

    return HMM_ALIGN_SCAN_WAITING_FOR_MEASUREMENT_DATABLOCK;
}


//////////////////////////////////////////////////
//
//  Message: Measurement Block Received
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h70(void) {

            // Get the Measurement data from MCM
            // and store it using MDM

            ptrMDM->PutNext(ptrMCM->GetScanMeasurementData());

            // Update counters

            --pointsLeftToScan;

            if(pointsLeftToScan != 0)   // more ?
            {
                    // Tell MCM to be ready for the Measurement data

                    SendHiPrMsg(MeasurementCommMachineID, WaitForMeasurementBlock);

                    // Set NEW Motor Chipset Break Points
                    // based on the scan direction

                    if(currAlignDirection == ALIGN_SCAN_RIGHT)
                    {
                        breakPointXCoordinate += alignmentScanData.alignmentSteps;

                        SendHiPrMsg(MotorCommID, SetBreakPointX,
                                    breakPointXCoordinate, NULL_MESSAGE_ID);
                    }
                    else
                    {
                        // currAlignDirection == ALIGN_SCAN_DOWN

                        breakPointYCoordinate -= alignmentScanData.alignmentSteps;

                        SendHiPrMsg(MotorCommID, SetBreakPointY,
                                    breakPointYCoordinate, NULL_MESSAGE_ID);
                    }

                return  HMM_ALIGN_SCAN_WAITING_FOR_BREAKPOINT_TRIGGER;
            }

    return HMM_ALIGN_SCAN_WAITING_FOR_CONTINUOUS_MOVE_DONE;
}


//////////////////////////////////////////////////
//
//  Message: Continuous Move Done
//
//////////////////////////////////////////////////

WORD
HeadMachinesManager::HMM_h71(void) {

        //
        // IMPORTANT : Set Kernel back to the
        //             Normal Scan Measurements mode
        //

        SetMeasurementMode(NORMAL_SCAN_MEASUREMENT);


        // Tell HCC to exit the Continuous Scan mode

        SendHiPrMsg(HeadCommandCommID, EndContinuousScanMode);

        // Inform HOC about the completion of the request

        SendHiPrMsg(HeadOperationsCoordinatorMachineID, AlignmentScanDone);

    return HMM_IDLE;
}








