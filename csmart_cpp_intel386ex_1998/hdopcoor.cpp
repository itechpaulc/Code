




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




#include "hdopcoor.h"




//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  HeadOperationsCoordinator
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
HeadOperationsCoordinator::SetAlignmentStartData(XYCoordinate alignStartData) {

    alignmentStartData = alignStartData;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadOperationsCoordinator::SetAlignmentEndData(XYCoordinate alignEndData) {

    alignmentEndData = alignEndData;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadOperationsCoordinator::LinkAll(ScanParametersDataManager   * pSPDM,
                                   HeadMachinesManager         * pHMM) {

    LinkScanParametersDataManager(pSPDM);
    LinkHeadMachinesManager(pHMM);
}

void
HeadOperationsCoordinator::LinkScanParametersDataManager(ScanParametersDataManager *pSPDM)
{
    ptrSPDM = pSPDM;
}

void
HeadOperationsCoordinator::LinkHeadMachinesManager(HeadMachinesManager *pHMM)
{
    ptrHMM = pHMM;
}


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// HeadOperationsCoordinator - private helper functions
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadOperationsCoordinator::updateNextScanData(void)
{
        // Look at the first point coordinate attribute

        nextStartScanIndex = nextEndScanIndex = pointsScanned;

        if(ptrSPDM->GetAt(nextStartScanIndex).IsColorBarPoint())
        {
            currColorBarGroupId = ptrSPDM->GetAt(nextStartScanIndex).GetGroupingId();

            // Search for all the Color Bar, points
            // that has the same grouping identification

            while(  (ptrSPDM->GetAt(nextEndScanIndex+1).IsColorBarPoint())
                  &&(ptrSPDM->GetAt(nextEndScanIndex+1).GetGroupingId()==currColorBarGroupId)
                  &&(pointsScanned < totalPointsToScan))
            {
                ++nextEndScanIndex;
            }

            // Make sure there are at least 2 swatches

            if(nextStartScanIndex > nextEndScanIndex)
                nextScanType = COLOR_BAR_SCAN;
            else
                nextScanType = POINT_TO_POINT_SCAN;
        }
        else
        {
            nextScanType = POINT_TO_POINT_SCAN;

            // Search for all the point to point, points

            while( !(ptrSPDM->GetAt(nextEndScanIndex+1).IsColorBarPoint())
                  &&(pointsScanned < totalPointsToScan))
            {
                ++nextEndScanIndex;
            }
        }
}



//////////////////////////////////////////////////
//
// HeadOperationsCoordinator - RESPONSE ENTRIES
//
//////////////////////////////////////////////////



//////////////////////////////////////////////////
//
// State Transition Matrices
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_IDLE)
    EV_HANDLER(DoFullCalibration, HOC_h1),
    EV_HANDLER(StartScanning, HOC_h10),
    EV_HANDLER(DoAlignmentScan, HOC_h16),
    EV_HANDLER(DoTargetAlignmentScan, HOC_h21),
    EV_HANDLER(MeasureOnTheSpot, HOC_h25),
    EV_HANDLER(MeasureAtTargetLamp, HOC_h29),
    EV_HANDLER(MeasureAtPoint, HOC_h33)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_FULL_CALIBRATION_FINDING_LIMITS)
    EV_HANDLER(LimitsFound, HOC_h2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_FULL_CALIBRATION_GOING_HOME_1)
    EV_HANDLER(GoHomeDone, HOC_h3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_FULL_CALIBRATION_TURNING_AIR_ON_HEAD_DOWN)
    EV_HANDLER(AirIsOnHeadIsDown, HOC_h4)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_FULL_CALIBRATION_CAL_AT_BLACK)
    EV_HANDLER(CalibrateAtBlackHoleDone, HOC_h5)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_FULL_CALIBRATION_CAL_AT_WHITE_WITH_FLASH)
    EV_HANDLER(CalibrateAtWhitePlaqueDone, HOC_h6)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_FULL_CALIBRATION_CAL_AT_WHITE_WITHOUT_FLASH)
    EV_HANDLER(CalibrateAtWhitePlaqueDone, HOC_h7)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_FULL_CALIBRATION_TURNING_AIR_OFF_HEAD_UP)
    EV_HANDLER(AirIsOffHeadIsUp, HOC_h8)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_FULL_CALIBRATION_GOING_HOME_2)
    EV_HANDLER(GoHomeDone, HOC_h9)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_SCANNING_GOING_TO_INITIAL_SCAN_POSITION)
    EV_HANDLER(MoveToPositionDone, HOC_h11)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_SCANNING_TURNING_AIR_ON_HEAD_DOWN)
    EV_HANDLER(AirIsOnHeadIsDown, HOC_h12)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_WAITING_FOR_ANY_SCAN_TYPE_DONE)
    EV_HANDLER(ColorBarScanDone, HOC_h13),
    EV_HANDLER(PointToPointScanDone, HOC_h13),
    EV_HANDLER(PointMeasured, HOC_h14)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_SCANNING_TURNING_AIR_OFF_HEAD_UP)
    EV_HANDLER(AirIsOffHeadIsUp, HOC_h15)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_ALIGN_SCAN_GOING_TO_INITIAL_SCAN_POSITION)
    EV_HANDLER(MoveToPositionDone, HOC_h17)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_ALIGN_SCAN_TURNING_AIR_ON_HEAD_DOWN)
    EV_HANDLER(AirIsOnHeadIsDown, HOC_h18)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_ALIGN_SCAN_ALIGNMENT_SCANNING)
    EV_HANDLER(AlignmentScanDone, HOC_h19)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_ALIGN_SCAN_TURNING_AIR_OFF_HEAD_UP)
    EV_HANDLER(AirIsOffHeadIsUp, HOC_h20)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_TARGET_ALIGN_SCAN_GOING_TO_INITIAL_POSITION)
    EV_HANDLER(MoveToPositionDone, HOC_h22)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_TARGET_ALIGN_SCAN_TURNING_AIR_ON_HEAD_DOWN)
    EV_HANDLER(AirIsOnHeadIsDown, HOC_h23)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_TARGET_ALIGN_SCAN_TARGET_ALIGNMENT_SCANNING)
    EV_HANDLER(TargetAlignmentScanDone, HOC_h24)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_MEASURE_ON_THE_SPOT_TURNING_AIR_ON_HEAD_DOWN)
    EV_HANDLER(AirIsOnHeadIsDown, HOC_h26)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_MEASURE_ON_THE_SPOT_MEASURING_ON_THE_SPOT)
    EV_HANDLER(MeasureOnTheSpotDone, HOC_h27)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_MEASURE_ON_THE_SPOT_TURNING_AIR_OFF_HEAD_UP)
    EV_HANDLER(AirIsOffHeadIsUp, HOC_h28)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_MEASURE_AT_TARGET_LAMP_TURNING_AIR_ON_HEAD_DOWN)
    EV_HANDLER(AirIsOnHeadIsDown, HOC_h30)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_MEASURE_AT_TARGET_LAMP_MEASURING_AT_TARGET_LAMP)
    EV_HANDLER(MeasureAtTargetLampDone, HOC_h31)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_MEASURE_AT_TARGET_LAMP_TURNING_AIR_OFF_HEAD_UP)
    EV_HANDLER(AirIsOffHeadIsUp, HOC_h32)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_MEASURE_AT_POINT_TURNING_AIR_ON_HEAD_DOWN)
    EV_HANDLER(AirIsOnHeadIsDown, HOC_h34)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_MEASURE_AT_POINT_MEASURING_AT_POINT)
    EV_HANDLER(MeasureAtPointDone, HOC_h35)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_MEASURE_AT_POINT_TURNING_AIR_OFF_HEAD_UP)
    EV_HANDLER(AirIsOffHeadIsUp, HOC_h36)
STATE_TRANSITION_MATRIX_END;



//////////////////////////////////////////////////
//
// Matrix Table
//
//////////////////////////////////////////////////

DEFINE_RESPONSE_TABLE_ENTRY(HeadOperationsCoordinator)
    STATE_MATRIX_ENTRY(_HOC_IDLE),
    STATE_MATRIX_ENTRY(_HOC_FULL_CALIBRATION_FINDING_LIMITS),
    STATE_MATRIX_ENTRY(_HOC_FULL_CALIBRATION_GOING_HOME_1),
    STATE_MATRIX_ENTRY(_HOC_FULL_CALIBRATION_TURNING_AIR_ON_HEAD_DOWN),
    STATE_MATRIX_ENTRY(_HOC_FULL_CALIBRATION_CAL_AT_BLACK),
    STATE_MATRIX_ENTRY(_HOC_FULL_CALIBRATION_CAL_AT_WHITE_WITH_FLASH),
    STATE_MATRIX_ENTRY(_HOC_FULL_CALIBRATION_CAL_AT_WHITE_WITHOUT_FLASH),
    STATE_MATRIX_ENTRY(_HOC_FULL_CALIBRATION_TURNING_AIR_OFF_HEAD_UP),
    STATE_MATRIX_ENTRY(_HOC_FULL_CALIBRATION_GOING_HOME_2),
    STATE_MATRIX_ENTRY(_HOC_SCANNING_GOING_TO_INITIAL_SCAN_POSITION),
    STATE_MATRIX_ENTRY(_HOC_SCANNING_TURNING_AIR_ON_HEAD_DOWN),
    STATE_MATRIX_ENTRY(_HOC_WAITING_FOR_ANY_SCAN_TYPE_DONE),
    STATE_MATRIX_ENTRY(_HOC_SCANNING_TURNING_AIR_OFF_HEAD_UP),
    STATE_MATRIX_ENTRY(_HOC_ALIGN_SCAN_GOING_TO_INITIAL_SCAN_POSITION),
    STATE_MATRIX_ENTRY(_HOC_ALIGN_SCAN_TURNING_AIR_ON_HEAD_DOWN),
    STATE_MATRIX_ENTRY(_HOC_ALIGN_SCAN_ALIGNMENT_SCANNING),
    STATE_MATRIX_ENTRY(_HOC_ALIGN_SCAN_TURNING_AIR_OFF_HEAD_UP),
    STATE_MATRIX_ENTRY(_HOC_TARGET_ALIGN_SCAN_GOING_TO_INITIAL_POSITION),
    STATE_MATRIX_ENTRY(_HOC_TARGET_ALIGN_SCAN_TURNING_AIR_ON_HEAD_DOWN),
    STATE_MATRIX_ENTRY(_HOC_TARGET_ALIGN_SCAN_TARGET_ALIGNMENT_SCANNING),
    STATE_MATRIX_ENTRY(_HOC_MEASURE_ON_THE_SPOT_TURNING_AIR_ON_HEAD_DOWN),
    STATE_MATRIX_ENTRY(_HOC_MEASURE_ON_THE_SPOT_MEASURING_ON_THE_SPOT),
    STATE_MATRIX_ENTRY(_HOC_MEASURE_ON_THE_SPOT_TURNING_AIR_OFF_HEAD_UP),
    STATE_MATRIX_ENTRY(_HOC_MEASURE_AT_TARGET_LAMP_TURNING_AIR_ON_HEAD_DOWN),
    STATE_MATRIX_ENTRY(_HOC_MEASURE_AT_TARGET_LAMP_MEASURING_AT_TARGET_LAMP),
    STATE_MATRIX_ENTRY(_HOC_MEASURE_AT_TARGET_LAMP_TURNING_AIR_OFF_HEAD_UP),
    STATE_MATRIX_ENTRY(_HOC_MEASURE_AT_POINT_TURNING_AIR_ON_HEAD_DOWN),
    STATE_MATRIX_ENTRY(_HOC_MEASURE_AT_POINT_MEASURING_AT_POINT),
    STATE_MATRIX_ENTRY(_HOC_MEASURE_AT_POINT_TURNING_AIR_OFF_HEAD_UP)
RESPONSE_TABLE_END;




//////////////////////////////////////////////////
//
// Static Member Definitions
//
//////////////////////////////////////////////////

WORD        HeadOperationsCoordinator::totalPointsToScan = 0;
WORD        HeadOperationsCoordinator::pointsScanned = 0;

BOOL        HeadOperationsCoordinator::reportPercentageDone = FALSE;
WORD        HeadOperationsCoordinator::scanCountMarker = 0;

SCAN_TYPE   HeadOperationsCoordinator::nextScanType;

WORD        HeadOperationsCoordinator::nextStartScanIndex = 0;
WORD        HeadOperationsCoordinator::nextEndScanIndex = 0;

BOOL        HeadOperationsCoordinator::leaveAirOnHeadDown = FALSE;
BOOL        HeadOperationsCoordinator::airIsOnHeadIsDown = FALSE;


AlignmentScanData   HeadOperationsCoordinator::alignmentScanData;
XYCoordinate        HeadOperationsCoordinator::alignmentStartData;
XYCoordinate        HeadOperationsCoordinator::alignmentEndData;


BYTE        HeadOperationsCoordinator::currColorBarGroupId = 0;

ScanParametersDataManager   *   HeadOperationsCoordinator::ptrSPDM = 0;

HeadMachinesManager         *   HeadOperationsCoordinator::ptrHMM = 0;



//////////////////////////////////////////////////
//
// HeadOperationsCoordinator - Constructors, Destructors
//
//////////////////////////////////////////////////

HeadOperationsCoordinator::HeadOperationsCoordinator(STATE_MACHINE_ID sMsysID)
    :StateMachine(sMsysID)
{
    ASSIGN_RESPONSE_TABLE();

    SetCurrState(HOC_IDLE);
}

HeadOperationsCoordinator::~HeadOperationsCoordinator(void) { }



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// HeadOperationsCoordinator - private EXIT PROCEDURES
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
// Message: Do Full Calibration
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h1(void)
{
        SendHiPrMsg(HeadMachinesManagerID, FindLimits);

    return  HOC_FULL_CALIBRATION_FINDING_LIMITS;
}


//////////////////////////////////////////////////
//
// Message: Limits Found
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h2(void)
{
        SendHiPrMsg(HeadMachinesManagerID, GoHome);

    return  HOC_FULL_CALIBRATION_GOING_HOME_1;
}


//////////////////////////////////////////////////
//
// Message: Go Home Done
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h3(void)
{
        SendHiPrMsg(HeadMachinesManagerID, AirOnHeadDown);

    return  HOC_FULL_CALIBRATION_TURNING_AIR_ON_HEAD_DOWN;
}


//////////////////////////////////////////////////
//
// Message: Air Is On Head Is Down
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h4(void)
{
        SendHiPrMsg(HeadMachinesManagerID, CalibrateAtBlackHole);

    return  HOC_FULL_CALIBRATION_CAL_AT_BLACK;
}


//////////////////////////////////////////////////
//
// Message: Calibrate At Black Hole Done
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h5(void)
{
        BOOL measureWithFlash = TRUE;

            SendHiPrMsg(HeadMachinesManagerID, CalibrateAtWhitePlaque,
                        measureWithFlash, NULL_MESSAGE_DATA);

    return  HOC_FULL_CALIBRATION_CAL_AT_WHITE_WITH_FLASH;
}


//////////////////////////////////////////////////
//
// Message: Calibrate At White Plaque Done
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h6(void)
{
        BOOL measureWithFlash = FALSE;

            SendHiPrMsg(HeadMachinesManagerID, CalibrateAtWhitePlaque,
                        measureWithFlash, NULL_MESSAGE_DATA);

    return  HOC_FULL_CALIBRATION_CAL_AT_WHITE_WITHOUT_FLASH;
}


//////////////////////////////////////////////////
//
// Message:  Calibrate At White Plaque Done
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h7(void)
{
        SendHiPrMsg(HeadMachinesManagerID, AirOffHeadUp);

    return  HOC_FULL_CALIBRATION_TURNING_AIR_OFF_HEAD_UP;
}


//////////////////////////////////////////////////
//
// Message: Air Is Off Head Is Up
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h8(void)
{
        SendHiPrMsg(HeadMachinesManagerID, GoHome);

    return  HOC_FULL_CALIBRATION_GOING_HOME_2;
}


//////////////////////////////////////////////////
//
// Message: Go Home Done
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h9(void)
{
        // Inform CSM that request was completed

        SendHiPrMsg(ColorSmartManagerID, FullCalibrationDone);

    return  HOC_IDLE;
}


//////////////////////////////////////////////////
//
// Message: Start Scanning
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h10(void)
{
    totalPointsToScan = ptrSPDM->GetPointCoordiateCount();

        if(totalPointsToScan == 0)
        {
            // Send Message to CSM

            SendHiPrMsg(ColorSmartManagerID, ScanningDone);

            return  HOC_IDLE;
        }

        // Get data from message

        reportPercentageDone = (BOOL)GetCurrEvent().msgData1;

        if(reportPercentageDone)
        {
            // prepare for percentage done monitoring

            WORD    percentageDoneMarker = (WORD)GetCurrEvent().msgData2;

            // Convert % to scan count

            scanCountMarker = (percentageDoneMarker / 100) * totalPointsToScan;
        }
    
        // Determine the first point location and
        // move there

        int initialX = ptrSPDM->GetAt(1).GetXCoordinate();
        int initialY = ptrSPDM->GetAt(1).GetYCoordinate();

            SendHiPrMsg(HeadMachinesManagerID, MoveToPosition,
                        initialX, initialY);

    return HOC_SCANNING_GOING_TO_INITIAL_SCAN_POSITION;
}


//////////////////////////////////////////////////
//
// Message : Move To Position Done
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h11(void)
{
        SendHiPrMsg(HeadMachinesManagerID, AirOnHeadDown);

    return  HOC_SCANNING_TURNING_AIR_ON_HEAD_DOWN;
}


//////////////////////////////////////////////////
//
//  Message : Air Is On Head Is Down
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h12(void)
{
        // Determine the first scan's behavior

        updateNextScanData();

        if(nextScanType == COLOR_BAR_SCAN)
        {
            // Send Color Bar Scan request to HMM

            SendHiPrMsg(HeadMachinesManagerID, DoColorBarScan,
                        nextStartScanIndex, nextEndScanIndex);
        }
        else
        {
            // Send Point To Point Scan request to HMM

            SendHiPrMsg(HeadMachinesManagerID, DoPointToPointScan,
                        nextStartScanIndex, nextEndScanIndex);
        }

    return  HOC_WAITING_FOR_ANY_SCAN_TYPE_DONE;
}


//////////////////////////////////////////////////
//
//  Message :   Color Bar Scan Done
//              Point To Point Scan Done
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h13(void)
{
        if(pointsScanned == totalPointsToScan)
        {
                // scanning is done, turn the air off, head up

                SendHiPrMsg(HeadMachinesManagerID, AirOffHeadUp);

            return  HOC_SCANNING_TURNING_AIR_OFF_HEAD_UP;
        }

        // More Scanning to do
        // Determine the next scan's behavior

        updateNextScanData();

        if(nextScanType == COLOR_BAR_SCAN)
        {
            // Send Color Bar Scan request to HMM

            SendHiPrMsg(HeadMachinesManagerID, DoColorBarScan,
                        nextStartScanIndex, nextEndScanIndex);
        }
        else
        {
            // Send Point To Point Scan request to HMM

            SendHiPrMsg(HeadMachinesManagerID, DoPointToPointScan,
                        nextStartScanIndex, nextEndScanIndex);
        }

    return  HOC_WAITING_FOR_ANY_SCAN_TYPE_DONE;
}



//////////////////////////////////////////////////
//
// Message : Point Measured
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h14(void)
{
    // Keep track of the number of points scanned

    ++pointsScanned;

        if(reportPercentageDone)
        {
            // marker reached

            if(pointsScanned == scanCountMarker)
            {
                    // adjust marker

                    scanCountMarker += scanCountMarker;

                    // Send Message to CSM that a percentage is done

                SendHiPrMsg(ColorSmartManagerID, PercentDoneReached);
            }
        }

    return  HOC_WAITING_FOR_ANY_SCAN_TYPE_DONE;
}


//////////////////////////////////////////////////
//
// Message : Air Is Off Head Is Up
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h15(void)
{
        // Send Message to CSM

        SendHiPrMsg(ColorSmartManagerID, ScanningDone);

    return  HOC_IDLE;
}


//////////////////////////////////////////////////
//
// Message : Do Alignment Scan
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h16(void)
{
    // Get Data from message

    WORD    stepIntervals = (WORD)GetCurrEvent().msgData1;

            leaveAirOnHeadDown = (BOOL)GetCurrEvent().msgData2;

    // Build and Set Alignment Scan Parameters
    // for local copy

    alignmentScanData.alignmentStart = alignmentStartData;
    alignmentScanData.alignmentEnd   = alignmentEndData;
    alignmentScanData.alignmentSteps = stepIntervals;

    // Set Alignment Scan Parameters for HMM copy

    ptrHMM->SetAlignmentScanData(alignmentScanData);

        if(airIsOnHeadIsDown)
        {
                SendHiPrMsg(HeadMachinesManagerID, DoAlignmentScan);

            return  HOC_ALIGN_SCAN_ALIGNMENT_SCANNING;
        }

        // Determine the first point location and
        // move there

        int initialX = alignmentStartData.GetXCoordinate();
        int initialY = alignmentStartData.GetYCoordinate();

            SendHiPrMsg(HeadMachinesManagerID, MoveToPosition,
                        initialX, initialY);

    return  HOC_ALIGN_SCAN_GOING_TO_INITIAL_SCAN_POSITION;
}


//////////////////////////////////////////////////
//
// Message : Move To Position Done
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h17(void)
{
        SendHiPrMsg(HeadMachinesManagerID, AirOnHeadDown);

    return  HOC_ALIGN_SCAN_TURNING_AIR_ON_HEAD_DOWN;
}


//////////////////////////////////////////////////
//
// Message : Air Is On Head Is Down
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h18(void)
{
        airIsOnHeadIsDown = TRUE;

        // Tell HMM to perform an alignment scan

        SendHiPrMsg(HeadMachinesManagerID, DoAlignmentScan);

    return  HOC_ALIGN_SCAN_ALIGNMENT_SCANNING;
}


//////////////////////////////////////////////////
//
// Message : Alignment Scan Done
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h19(void)
{
        if(leaveAirOnHeadDown)
        {
                // Inform CSM alignment scan complete

                SendHiPrMsg(ColorSmartManagerID, AlignmentScanDone);

            return  HOC_IDLE;
        }

        SendHiPrMsg(HeadMachinesManagerID, AirOffHeadUp);

    return HOC_ALIGN_SCAN_TURNING_AIR_OFF_HEAD_UP;
}


//////////////////////////////////////////////////
//
// Message : Air Is Off Head Is Up
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h20(void)
{
        // Inform CSM alignment scan complete

        SendHiPrMsg(ColorSmartManagerID, AlignmentScanDone);

    return  HOC_IDLE;
}


//////////////////////////////////////////////////
//
// Message : Do Target Alignment Scan
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h21(void)
{
    BOOL compensateBacklash = (BOOL)GetCurrEvent().msgData1;

        if(compensateBacklash)
            SendHiPrMsg(HeadMachinesManagerID, MoveToPositionWithBacklashCompensation);
        else
            SendHiPrMsg(HeadMachinesManagerID, MoveToPosition);

        // Debug

        // Build and Set Alignment Scan Parameters
        // for local copy

        //alignmentScanData.alignmentStart = alignmentStartData;
        //alignmentScanData.alignmentEnd   = alignmentEndData;
        //alignmentScanData.alignmentSteps = stepIntervals;

        // Set Alignment Scan Parameters for HMM copy

        //ptrHMM->SetAlignmentScanData(alignmentScanData);


    return  HOC_TARGET_ALIGN_SCAN_GOING_TO_INITIAL_POSITION;
}


//////////////////////////////////////////////////
//
// Message : Move To Position Done
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h22(void)
{
        if(ptrHMM->IsAirOnProbeHeadDown())
        {
                SendHiPrMsg(HeadMachinesManagerID, DoTargetAlignmentScan);

            return HOC_TARGET_ALIGN_SCAN_TARGET_ALIGNMENT_SCANNING;
        }


        SendHiPrMsg(HeadMachinesManagerID, AirOnHeadDown);

    return  HOC_TARGET_ALIGN_SCAN_TURNING_AIR_ON_HEAD_DOWN;
}


//////////////////////////////////////////////////
//
// Message :  Air Is On Head Is Down
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h23(void)
{
        SendHiPrMsg(HeadMachinesManagerID, DoTargetAlignmentScan);

    return HOC_TARGET_ALIGN_SCAN_TARGET_ALIGNMENT_SCANNING;
}


//////////////////////////////////////////////////
//
// Message :  Target Alignment Scan Done
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h24(void)
{
        // Inform CSM target alignment scan complete

        SendHiPrMsg(ColorSmartManagerID, TargetAlignmentScanDone);

    return  HOC_IDLE;
}


//////////////////////////////////////////////////
//
// Message : Measure On The Spot
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h25(void)
{
        SendHiPrMsg(HeadMachinesManagerID, AirOnHeadDown);

    return  HOC_MEASURE_ON_THE_SPOT_TURNING_AIR_ON_HEAD_DOWN;
}


//////////////////////////////////////////////////
//
// Message :   Air Is On Head Is Down
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h26(void)
{
        SendHiPrMsg(HeadMachinesManagerID, MeasureOnTheSpot);

    return  HOC_MEASURE_ON_THE_SPOT_MEASURING_ON_THE_SPOT;
}


//////////////////////////////////////////////////
//
// Message :  Measure On The Spot Done
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h27(void)
{
        SendHiPrMsg(HeadMachinesManagerID, AirOffHeadUp);

    return  HOC_MEASURE_ON_THE_SPOT_TURNING_AIR_OFF_HEAD_UP;
}


//////////////////////////////////////////////////
//
// Message :   Air Is Off Head Is Up
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h28(void)
{
        // Inform CSM Measure On The spot is done

        SendHiPrMsg(ColorSmartManagerID, MeasureOnTheSpotDone);

    return  HOC_IDLE;
}


//////////////////////////////////////////////////
//
// Message : Measure At Target Lamp
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h29(void)
{
        SendHiPrMsg(HeadMachinesManagerID, AirOnHeadDown);

    return  HOC_MEASURE_AT_TARGET_LAMP_TURNING_AIR_ON_HEAD_DOWN;
}


//////////////////////////////////////////////////
//
// Message :   Air Is On Head Is Down
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h30(void)
{
        SendHiPrMsg(HeadMachinesManagerID, MeasureAtTargetLamp);

    return  HOC_MEASURE_AT_TARGET_LAMP_MEASURING_AT_TARGET_LAMP;
}


//////////////////////////////////////////////////
//
// Message :  Measure At Target Lamp Done
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h31(void)
{
        SendHiPrMsg(HeadMachinesManagerID, AirOffHeadUp);

    return  HOC_MEASURE_AT_TARGET_LAMP_TURNING_AIR_OFF_HEAD_UP;
}


//////////////////////////////////////////////////
//
// Message :   Air Is Off Head Is Up
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h32(void)
{
        // Inform CSM Measure At Target Lamp is done

        SendHiPrMsg(ColorSmartManagerID, MeasureAtTargetLampDone);

    return  HOC_IDLE;
}


//////////////////////////////////////////////////
//
// Message : Measure At Point
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h33(void)
{
        SendHiPrMsg(HeadMachinesManagerID, AirOnHeadDown);

    return  HOC_MEASURE_AT_POINT_TURNING_AIR_ON_HEAD_DOWN;
}


//////////////////////////////////////////////////
//
// Message :   Air Is On Head Is Down
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h34(void)
{
        // Get Data from message

        int xCoordinate = (int)GetCurrEvent().msgData1;
        int yCoordinate = (int)GetCurrEvent().msgData2;

            SendHiPrMsg(HeadMachinesManagerID, MeasureAtPoint,
                        xCoordinate,  yCoordinate);

    return  HOC_MEASURE_AT_POINT_MEASURING_AT_POINT;
}


//////////////////////////////////////////////////
//
// Message :  Measure At Point
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h35(void)
{
        SendHiPrMsg(HeadMachinesManagerID, AirOffHeadUp);

    return  HOC_MEASURE_AT_POINT_TURNING_AIR_OFF_HEAD_UP;
}


//////////////////////////////////////////////////
//
// Message :   Air Is Off Head Is Up
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h36(void)
{
        // Inform CSM Measure At Point is done

        SendHiPrMsg(ColorSmartManagerID, MeasureAtPointDone);

    return  HOC_IDLE;
}





