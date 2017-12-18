

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



#include "csmngr.h"


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  ColorSmartManager
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

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
ColorSmartManager::LinkAll(PciTxCommMachine *pPTC      , PciRxCommMachine *pPRC,
                           HeadMachinesManager *pHMM   , ScanParametersDataManager *pSPDM,
                           MeasurementDataManager *pMDM, HeadOperationsCoordinator *pHOC) {

    // Call the individual link routines

    LinkPciTxCommMachine(pPTC);
    LinkPciRxCommMachine(pPRC);
    LinkHeadMachinesManager(pHMM);
    LinkScanParametersDataManager(pSPDM);
    LinkMeasurementDataManager(pMDM);
    LinkHeadOperationsCoordinator(pHOC);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ColorSmartManager::LinkPciTxCommMachine(PciTxCommMachine *pPTC) {

    ptrPTC = pPTC;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ColorSmartManager::LinkPciRxCommMachine(PciRxCommMachine *pPRC) {

    ptrPRC = pPRC;
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ColorSmartManager::LinkHeadMachinesManager(HeadMachinesManager *pHMM) {

    ptrHMM = pHMM;
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ColorSmartManager::LinkScanParametersDataManager(ScanParametersDataManager *pSPDM) {

    ptrSPDM = pSPDM;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ColorSmartManager::LinkMeasurementDataManager(MeasurementDataManager *pMDM) {

    ptrMDM = pMDM;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ColorSmartManager::LinkHeadOperationsCoordinator(HeadOperationsCoordinator *pHOC) {

    ptrHOC = pHOC;
}



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ColorSmartManager::SetTargetAlignmentData(TargetAlignmentData alignData) {

    ptrHMM->SetTargetAlignmentData(alignData);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ColorSmartManager::GetTargetAlignmentData(TargetAlignmentData &alignData) {

    ptrHMM->GetTargetAlignmentData(alignData);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ColorSmartManager::SetAlignmentScanData(AlignmentScanData alignData) {

    ptrHMM->SetAlignmentScanData(alignData);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ColorSmartManager::GetAlignmentScanData(AlignmentScanData &alignData) {

    ptrHMM->GetAlignmentScanData(alignData);
}



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// ColorSmartManager - private helper functions
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL
ColorSmartManager::parametersInitialized(void) {

    return ( tableParamInitialized &&
             timingParamInitialized &&
             moveProfilesInitialized ) ? TRUE : FALSE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ColorSmartManager::transmitPciMessage(void) {

        ptrPTC->QueueNextPciMessage(currTxMboxMsg);

    SendHiPrMsg(PciTxCommID, TransmitPciMessage);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ColorSmartManager::transmitPciMessageWithData(void) {

        ptrPTC->QueueNextPciMessage(currTxMboxMsg);

    SendHiPrMsg(PciTxCommID, TransmitPciMessage);
}


//////////////////////////////////////////////////
//
// ColorSmartManager - RESPONSE ENTRIES
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
// State Transition Matrices
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_UNINITIALIZED)
    EV_HANDLER(SetTableParameters, CSM_h1a),
    EV_HANDLER(SetTimingParameters, CSM_h1b),
    EV_HANDLER(SetMoveProfiles, CSM_h1c)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_SYSTEM_PARAMETERS_INITIALIZED)
    EV_HANDLER(FindHome, CSM_h2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_FINDING_LIMITS_FROM_SYSTEM_PARAMETERS_INITIALIZED)
    EV_HANDLER(LimitsFound, CSM_h3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_GOING_HOME_FROM_FINDING_LIMITS)
    EV_HANDLER(GoHomeDone, CSM_h4)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_IDLE_AT_HOME)
    EV_HANDLER(GoScanningMode, CSM_h5),
    EV_HANDLER(GoTrackballMode, CSM_h7),
    EV_HANDLER(SetTableParameters, CSM_h1d),
    EV_HANDLER(SetTimingParameters, CSM_h1e),
    EV_HANDLER(SetMoveProfiles, CSM_h1f),
    EV_HANDLER(GoShutDown, CSM_h9),
    EV_HANDLER(FindHome, CSM_h9a),
    EV_HANDLER(GoHomeIdle, CSM_h9a)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_IAH_FINDING_LIMITS_IAH)
    EV_HANDLER(LimitsFound, CSM_h9b)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_IAH_GOING_HOME_IAH)
    EV_HANDLER(GoHomeDone, CSM_h9c)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_IAH_DOING_FULL_CALIBRATION_SCM)
    EV_HANDLER(FullCalibrationDone, CSM_h6)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_IAH_DOING_FULL_CALIBRATION_TBM)
    EV_HANDLER(FullCalibrationDone, CSM_h8)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_IAH_SETTING_NEW_MOVE_PROFILE_TBM)
    EV_HANDLER(SetNewMoveProfileDone, CSM_h8a)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_SCANNING_MODE_IDLE)
    EV_HANDLER(GoHomeIdle, CSM_h10),
    EV_HANDLER(GoTrackballMode, CSM_h12),
    EV_HANDLER(SetScanParameterData, CSM_h13),
    EV_HANDLER(StartScanningWithReports, CSM_h14),
    EV_HANDLER(StartScanningNoReports, CSM_h15),
    EV_HANDLER(DoAlignmentScan, CSM_h18),
    EV_HANDLER(DoTargetAlignmentScan, CSM_h20),
    EV_HANDLER(FlashTargetLamp, CSM_h22),
    EV_HANDLER(PointTargetLamp, CSM_h26)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_SMI_SETTING_NEW_MOVE_PROFILE_TBM)
    EV_HANDLER(SetNewMoveProfileDone, CSM_h12a)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_SMI_GOING_HOME_IAH)
    EV_HANDLER(GoHomeDone, CSM_h11)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_SCANNING_MODE_SCANNING)
    EV_HANDLER(PercentDoneReached, CSM_h16),
    EV_HANDLER(ScanningDone, CSM_h17)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_ALIGNMENT_SCANNING)
    EV_HANDLER(AlignmentScanDone, CSM_h19)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_TARGET_ALIGNMENT_SCANNING)
    EV_HANDLER(TargetAlignmentScanDone, CSM_h21)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_SCANNING_MODE_WAITING_TARGET_LAMP_ON)
    EV_HANDLER(TargetLampIsOn, CSM_h23)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_SCANNING_MODE_WAITING_TARGET_LAMP_TIMEOUT)
    EV_HANDLER(TargetLampTimedOut, CSM_h24)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_SCANNING_MODE_WAITING_TARGET_LAMP_OFF)
    EV_HANDLER(TargetLampIsOff, CSM_h25)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_SCANNING_MODE_POINTING_TARGET_LAMP)
    EV_HANDLER(PointTargetLampDone, CSM_h27)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_TRACKBALL_MODE_IDLE)
    EV_HANDLER(GoScanningMode, CSM_h28),
    EV_HANDLER(GoHomeIdle, CSM_h30),
    EV_HANDLER(MoveToPosition, CSM_h33),
    EV_HANDLER(PointTargetLamp, CSM_h36),
    EV_HANDLER(TargetLampOff, CSM_h39),
    EV_HANDLER(TargetLampOn, CSM_h41),
    EV_HANDLER(BlinkTargetLamp, CSM_h43)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_TMI_WAITING_FOR_TARGET_LAMP_OFF_SMI)
    EV_HANDLER(TargetLampIsOff, CSM_h29)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_TMI_WAITING_FOR_TARGET_LAMP_OFF_IAH)
    EV_HANDLER(TargetLampIsOff, CSM_h31)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_TMI_GOING_HOME_IAH)
    EV_HANDLER(GoHomeDone, CSM_h32)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_TRACKBALL_MODE_WAITING_FOR_LAMP_ON_MTP)
    EV_HANDLER(TargetLampIsOn, CSM_h34)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_TRACKBALL_MODE_MOVING_TO_POSITION)
    EV_HANDLER(MoveToPositionDone, CSM_h35)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_TRACKBALL_MODE_WAITING_FOR_LAMP_ON_PTL)
    EV_HANDLER(TargetLampIsOn, CSM_h37)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_TRACKBALL_MODE_POINTING_TARGET_LAMP)
    EV_HANDLER(PointTargetLamp, CSM_h38)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_TRACKBALL_MODE_WAITING_TARGET_LAMP_OFF)
    EV_HANDLER(TargetLampIsOff, CSM_h40)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_TRACKBALL_MODE_WAITING_TARGET_LAMP_ON)
    EV_HANDLER(TargetLampIsOn, CSM_h42)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_TRACKBALL_MODE_WAITING_TARGET_LAMP_OFF_BLINK)
    EV_HANDLER(TargetLampIsOff, CSM_h44)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_TRACKBALL_MODE_BLINK_DELAY)
    EV_HANDLER(TimeOut, CSM_h45)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(ColorSmartManager, _CSM_TRACKBALL_MODE_WAITING_TARGET_LAMP_ON_BLINK)
    EV_HANDLER(TargetLampIsOn, CSM_h46)
STATE_TRANSITION_MATRIX_END;



//////////////////////////////////////////////////
//
// Matrix Table
//
//////////////////////////////////////////////////

DEFINE_RESPONSE_TABLE_ENTRY(ColorSmartManager)
    STATE_MATRIX_ENTRY(_CSM_UNINITIALIZED),
    STATE_MATRIX_ENTRY(_CSM_SYSTEM_PARAMETERS_INITIALIZED),
    STATE_MATRIX_ENTRY(_CSM_FINDING_LIMITS_FROM_SYSTEM_PARAMETERS_INITIALIZED),
    STATE_MATRIX_ENTRY(_CSM_GOING_HOME_FROM_FINDING_LIMITS),
    STATE_MATRIX_ENTRY(_CSM_IDLE_AT_HOME),
    STATE_MATRIX_ENTRY(_CSM_IAH_FINDING_LIMITS_IAH),
    STATE_MATRIX_ENTRY(_CSM_IAH_GOING_HOME_IAH),
    STATE_MATRIX_ENTRY(_CSM_IAH_DOING_FULL_CALIBRATION_SCM),
    STATE_MATRIX_ENTRY(_CSM_IAH_DOING_FULL_CALIBRATION_TBM),
    STATE_MATRIX_ENTRY(_CSM_IAH_SETTING_NEW_MOVE_PROFILE_TBM),
    STATE_MATRIX_ENTRY(_CSM_SCANNING_MODE_IDLE),
    STATE_MATRIX_ENTRY(_CSM_SMI_SETTING_NEW_MOVE_PROFILE_TBM),
    STATE_MATRIX_ENTRY(_CSM_SMI_GOING_HOME_IAH),
    STATE_MATRIX_ENTRY(_CSM_SCANNING_MODE_SCANNING),
    STATE_MATRIX_ENTRY(_CSM_ALIGNMENT_SCANNING),
    STATE_MATRIX_ENTRY(_CSM_TARGET_ALIGNMENT_SCANNING),
    STATE_MATRIX_ENTRY(_CSM_SCANNING_MODE_WAITING_TARGET_LAMP_ON),
    STATE_MATRIX_ENTRY(_CSM_SCANNING_MODE_WAITING_TARGET_LAMP_TIMEOUT),
    STATE_MATRIX_ENTRY(_CSM_SCANNING_MODE_WAITING_TARGET_LAMP_OFF),
    STATE_MATRIX_ENTRY(_CSM_SCANNING_MODE_POINTING_TARGET_LAMP),
    STATE_MATRIX_ENTRY(_CSM_TRACKBALL_MODE_IDLE),
    STATE_MATRIX_ENTRY(_CSM_TMI_WAITING_FOR_TARGET_LAMP_OFF_SMI),
    STATE_MATRIX_ENTRY(_CSM_TMI_WAITING_FOR_TARGET_LAMP_OFF_IAH),
    STATE_MATRIX_ENTRY(_CSM_TMI_GOING_HOME_IAH),
    STATE_MATRIX_ENTRY(_CSM_TRACKBALL_MODE_WAITING_FOR_LAMP_ON_MTP),
    STATE_MATRIX_ENTRY(_CSM_TRACKBALL_MODE_MOVING_TO_POSITION),
    STATE_MATRIX_ENTRY(_CSM_TRACKBALL_MODE_WAITING_FOR_LAMP_ON_PTL),
    STATE_MATRIX_ENTRY(_CSM_TRACKBALL_MODE_POINTING_TARGET_LAMP),
    STATE_MATRIX_ENTRY(_CSM_TRACKBALL_MODE_WAITING_TARGET_LAMP_OFF),
    STATE_MATRIX_ENTRY(_CSM_TRACKBALL_MODE_WAITING_TARGET_LAMP_ON),
    STATE_MATRIX_ENTRY(_CSM_TRACKBALL_MODE_WAITING_TARGET_LAMP_OFF_BLINK),
    STATE_MATRIX_ENTRY(_CSM_TRACKBALL_MODE_BLINK_DELAY),
    STATE_MATRIX_ENTRY(_CSM_TRACKBALL_MODE_WAITING_TARGET_LAMP_ON_BLINK)
RESPONSE_TABLE_END;



//////////////////////////////////////////////////
//
// Static Member Definitions
//
//////////////////////////////////////////////////

WORD        ColorSmartManager::errorCount = 0;

PciTxCommMachine            * ColorSmartManager::ptrPTC = 0;
PciRxCommMachine            * ColorSmartManager::ptrPRC = 0;

HeadMachinesManager         * ColorSmartManager::ptrHMM = 0;

ScanParametersDataManager   * ColorSmartManager::ptrSPDM = 0;
MeasurementDataManager      * ColorSmartManager::ptrMDM = 0;

HeadOperationsCoordinator   * ColorSmartManager::ptrHOC = 0;

BOOL    ColorSmartManager::tableParamInitialized = FALSE;
BOOL    ColorSmartManager::timingParamInitialized = FALSE;
BOOL    ColorSmartManager::moveProfilesInitialized = FALSE;


MAILBOX_MESSAGE     ColorSmartManager::currTxMboxMsg;

BOOL    ColorSmartManager::percentReportingEnabled =  FALSE;

BOOL    ColorSmartManager::ntTargetLampOn = FALSE;
BOOL    ColorSmartManager::targetLampIsOff = TRUE;

WORD    ColorSmartManager::targetLampFlashTime = 0x0000;
WORD    ColorSmartManager::targetLampBlinkTime = 0x0000;



//////////////////////////////////////////////////
//
// ColorSmartManager - Constructors, Destructors
//
//////////////////////////////////////////////////

ColorSmartManager::ColorSmartManager(STATE_MACHINE_ID sMsysID)
    :StateMachine(sMsysID)
{
    ASSIGN_RESPONSE_TABLE();

    SetCurrState(CSM_UNINITIALIZED);
}

ColorSmartManager::~ColorSmartManager(void) { }


WORD    ColorSmartManager::GetErrorCount(void) {

    return  ColorSmartManager::errorCount;
}



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// ColorSmartManager - private EXIT PROCEDURES
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
// Message: SetTableParameters
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h1a(void)
{
        // Set Parameters



        tableParamInitialized = TRUE;

        if(parametersInitialized())
            return  CSM_SYSTEM_PARAMETERS_INITIALIZED;

    return  CSM_UNINITIALIZED;
}


//////////////////////////////////////////////////
//
// Message: SetTimingParameters
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h1b(void)
{
        // Set Parameters



        timingParamInitialized = TRUE;

        if(parametersInitialized())
            return  CSM_SYSTEM_PARAMETERS_INITIALIZED;

    return  CSM_UNINITIALIZED;
}


//////////////////////////////////////////////////
//
// Message: SetMoveProfiles
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h1c(void)
{
        // Set Parameters



        moveProfilesInitialized = TRUE;

        if(parametersInitialized())
            return  CSM_SYSTEM_PARAMETERS_INITIALIZED;

    return  CSM_UNINITIALIZED;
}


//////////////////////////////////////////////////
//
// Message: SetTableParameters
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h1d(void)
{
        // Set Parameters



    return  CSM_IDLE_AT_HOME;
}


//////////////////////////////////////////////////
//
// Message: SetTimingParameters
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h1e(void)
{
        // Set Parameters



    return  CSM_IDLE_AT_HOME;
}


//////////////////////////////////////////////////
//
// Message: SetMoveProfiles
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h1f(void)
{
        // Set Parameters



    return  CSM_IDLE_AT_HOME;
}


//////////////////////////////////////////////////
//
// Message: Find Home
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h2(void)
{
        // Tell HMM to find the limits

        SendHiPrMsg(HeadMachinesManagerID, FindLimits);

    return  CSM_FINDING_LIMITS_FROM_SYSTEM_PARAMETERS_INITIALIZED;
}


//////////////////////////////////////////////////
//
// Message: Limits Found
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h3(void)
{
        // Tell HMM to go home

        SendHiPrMsg(HeadMachinesManagerID, GoHome);

    return  CSM_GOING_HOME_FROM_FINDING_LIMITS;
}


//////////////////////////////////////////////////
//
// Message: Go Home Done
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h4(void)
{
        // Inform NT that we are at HOME IDLE and ready

        currTxMboxMsg.Clear();
        currTxMboxMsg.SetCommandAndExtension(CE_GoHomeIdle);
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        transmitPciMessage();

    return  CSM_IDLE_AT_HOME;
}


//////////////////////////////////////////////////
//
// Message: Go Scanning Mode
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h5(void)
{
        // Tell HOC to perform a full calibration

        SendHiPrMsg(HeadOperationsCoordinatorMachineID, DoFullCalibration);

    return  CSM_IAH_DOING_FULL_CALIBRATION_SCM;
}


//////////////////////////////////////////////////
//
// Message: Full Calibration Done
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h6(void)
{
        // Inform NT that we are at SCANNING MODE IDLE and ready

        currTxMboxMsg.Clear();
        currTxMboxMsg.SetCommandAndExtension(CE_GoScanningMode);
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        transmitPciMessage();

    return  CSM_IDLE_AT_HOME;
}


//////////////////////////////////////////////////
//
// Message: Go Trackball Mode
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h7(void)
{
        // Tell HOC to perform a full calibration

        SendHiPrMsg(HeadOperationsCoordinatorMachineID, DoFullCalibration);

    return  CSM_IAH_DOING_FULL_CALIBRATION_TBM;
}


//////////////////////////////////////////////////
//
// Message: Full Calibration Done
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h8(void)
{
        // Debug
        // Change Move profiles to TRACK BALL

        SendHiPrMsg(HeadMachinesManagerID, SetNewMoveProfile);

    return  CSM_IAH_SETTING_NEW_MOVE_PROFILE_TBM;
}


//////////////////////////////////////////////////
//
// Message: Set New Move Profile Done
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h8a(void)
{
        // Inform NT that we are at TRACKBALL MODE IDLE and ready

        currTxMboxMsg.Clear();
        //currTxMboxMsg.SetCommandAndExtension();
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        transmitPciMessage();

    return  CSM_TRACKBALL_MODE_IDLE;
}


//////////////////////////////////////////////////
//
// Message: Go Shut Down
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h9(void)
{
        // Power down the motors

        // Inform NT that we have just completed SHUT DOWN

        currTxMboxMsg.Clear();
        currTxMboxMsg.SetCommandAndExtension(CE_ReleaseEquipment);
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        transmitPciMessage();

    return  CSM_TRACKBALL_MODE_IDLE;
}


//////////////////////////////////////////////////
//
// Message: Find Home
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h9a(void)
{
        // Tell HMM to find the limits

        SendHiPrMsg(HeadMachinesManagerID, FindLimits);

    return  CSM_IAH_FINDING_LIMITS_IAH;
}


//////////////////////////////////////////////////
//
// Message: Limits Found
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h9b(void)
{
        // Tell HMM to go home

        SendHiPrMsg(HeadMachinesManagerID, GoHome);

    return  CSM_IAH_GOING_HOME_IAH;
}


//////////////////////////////////////////////////
//
// Message: Go Home Done
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h9c(void)
{
        // Inform NT that we are at HOME IDLE and ready

        currTxMboxMsg.Clear();
        // Debug
        //currTxMboxMsg.SetCommandAndExtension();
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        transmitPciMessage();

    return  CSM_IDLE_AT_HOME;
}


//////////////////////////////////////////////////
//
// Message: Go Home Idle
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h10(void)
{
        // Tell HMM to Go Home

        SendHiPrMsg(HeadMachinesManagerID, GoHome);

    return  CSM_SMI_GOING_HOME_IAH;
}


//////////////////////////////////////////////////
//
// Message: Go Home Idle Done
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h11(void)
{
        // Inform NT that we are now HOME IDLE

        currTxMboxMsg.Clear();
        currTxMboxMsg.SetCommandAndExtension(CE_GoHomeIdle);
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        transmitPciMessage();

    return  CSM_IDLE_AT_HOME;
}


//////////////////////////////////////////////////
//
// Message: Set New Move Profile Done
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h12(void)
{
        // Change Move profiles to TRACK BALL

        SendHiPrMsg(HeadMachinesManagerID, SetNewMoveProfile);

    return  CSM_SMI_SETTING_NEW_MOVE_PROFILE_TBM;
}


//////////////////////////////////////////////////
//
// Message: Go Trackball Mode
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h12a(void)
{
        // Inform NT that we are now in TRACKBALL MODE IDLE

        currTxMboxMsg.Clear();
        currTxMboxMsg.SetCommandAndExtension(CE_GoTrackballMode);
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        transmitPciMessage();

    return  CSM_TRACKBALL_MODE_IDLE;
}


//////////////////////////////////////////////////
//
// Message: Set Scan Parameters Data
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h13(void)
{
    // Create a pointer to the start of the data within the
    // Pci Buffer. The buffer starts with the number of entries
    // WORD size as stated in the communication protocol

    WORD    * ptrNumEntries = (WORD *)ptrPRC->GetPciRxCommBuffer();

    // The succeeding data is the Scan Coordinate data section

    ScanCoordinateData * ptrDataStart = (ScanCoordinateData *)(ptrNumEntries + 1);

    WORD    entryCount =  *ptrNumEntries;

        if(entryCount < MAX_SCANS)
        {
            // Clear all entries of the Scan Parameter Data Manager

            ptrSPDM->Reset();

            ptrSPDM->SetScanParameterData(ptrDataStart, entryCount);

            // Inform NT that the Scan Parameters Data
            // is now set

            currTxMboxMsg.Clear();
            currTxMboxMsg.SetCommandAndExtension(CE_SetScanParameterData);
            currTxMboxMsg.SetParam1(NTCOMPLETE);
        }
        else
        {
            // Inform NT that the Entry size was too large

            currTxMboxMsg.Clear();
            currTxMboxMsg.SetCommandAndExtension(CE_SetScanParameterData);
            // Debug
            //currTxMboxMsg.SetParam1();
        }

        transmitPciMessage();

    return  CSM_IDLE_AT_HOME;
}


//////////////////////////////////////////////////
//
// Message: Start Scanning With Reports
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h14(void)
{
    // Remember report disabled

    percentReportingEnabled = TRUE;

    // Get message data content

    DWORD   percentageCompleteMarker = GetCurrEvent().msgData1;

    // Debug
    // Check limits 1 - 100

        // Tell HOC to perform a scanning session

        SendHiPrMsg(HeadOperationsCoordinatorMachineID, StartScanning,
                        percentReportingEnabled, percentageCompleteMarker);

    return  CSM_SCANNING_MODE_SCANNING;
}


//////////////////////////////////////////////////
//
// Message:  Start Scanning No Reports
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h15(void)
{
    // Remember report enabled

    percentReportingEnabled = FALSE;

        // Tell HOC to perform a scanning session

        SendHiPrMsg(HeadOperationsCoordinatorMachineID, StartScanning,
                        percentReportingEnabled, NULL_MESSAGE_DATA);

    return  CSM_SCANNING_MODE_SCANNING;
}


//////////////////////////////////////////////////
//
// Message: Percent Done Reached
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h16(void)
{
    // Get message data content

    DWORD   percentageComplete = GetCurrEvent().msgData1;

        // Inform NT that a percentage is complete

        currTxMboxMsg.Clear();
        currTxMboxMsg.SetCommandAndExtension(CE_StartScanningWithReports);
        currTxMboxMsg.SetParam1(NTPROGRESS);
        currTxMboxMsg.SetParam2(percentageComplete);

        transmitPciMessage();

    return  CSM_SCANNING_MODE_SCANNING;
}


//////////////////////////////////////////////////
//
// Message: Scanning Done
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h17(void)
{
        // Inform NT that the scan is complete

        currTxMboxMsg.Clear();

        if(percentReportingEnabled)
            currTxMboxMsg.SetCommandAndExtension(CE_StartScanningWithReports);
        else
            currTxMboxMsg.SetCommandAndExtension(CE_StartScanningNoReports);

        currTxMboxMsg.SetParam1(NTCOMPLETE);


        // More info for Tx Buffer


        transmitPciMessageWithData();

    return  CSM_SCANNING_MODE_IDLE;
}


//////////////////////////////////////////////////
//
// Message: Do Alignment Scan
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h18(void)
{
    BOOL    probeHeadUpWhenDone  = (BOOL)GetCurrEvent().msgData1;
    WORD    measurementIntervals = GetCurrEvent().msgData2;

    // Extract data from the PCI Comm Buffer

    WORD            * dataCount = (WORD *)ptrPRC->GetPciRxCommBuffer();
    XYCoordinate    * startAlignment = (XYCoordinate *)dataCount+1;
    XYCoordinate    * endAlignment = (XYCoordinate *)startAlignment+1;

        // Set Alignment Scan Data for HOC

        ptrHOC->SetAlignmentStartData(*startAlignment);
        ptrHOC->SetAlignmentEndData(*endAlignment);


        // Tell HOC to perform an alignment scanning session

        SendHiPrMsg(HeadOperationsCoordinatorMachineID, DoAlignmentScan,
                        probeHeadUpWhenDone, measurementIntervals);

    return  CSM_ALIGNMENT_SCANNING;
}



//////////////////////////////////////////////////
//
// Message: Alignment Scan Done
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h19(void)
{
        // Inform NT that Alignment Scan is Done

        currTxMboxMsg.Clear();
        currTxMboxMsg.SetCommandAndExtension(CE_DoAlignmentScan);
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        // More info for Tx Buffer


        transmitPciMessageWithData();

    return  CSM_SCANNING_MODE_IDLE;
}


//////////////////////////////////////////////////
//
// Message:  Do Target Alignment Scan
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h20(void)
{
    BOOL    probeHeadUpWhenDone = (BOOL)GetCurrEvent().msgData1;

        // Tell HOC to perform a target alignment scanning session

        SendHiPrMsg(HeadOperationsCoordinatorMachineID, DoTargetAlignmentScan,
                        probeHeadUpWhenDone, NULL_MESSAGE_DATA);

    return  CSM_TARGET_ALIGNMENT_SCANNING;
}


//////////////////////////////////////////////////
//
// Message: Target Alignment Scan Done
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h21(void)
{
        // Inform NT that Target Alignment Scan is Done

        currTxMboxMsg.Clear();
        currTxMboxMsg.SetCommandAndExtension(CE_DoTargetAlignmentScan);
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        // More info for Tx Buffer


        transmitPciMessageWithData();

    return  CSM_SCANNING_MODE_IDLE;
}


//////////////////////////////////////////////////
//
// Message: Flash Target Lamp
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h22(void)
{
        targetLampFlashTime = (WORD)GetCurrEvent().msgData1;

        // Tell TLM to turn the light ON

        SendHiPrMsg(TargetLampManagerID, TargetLampOn,
                        targetLampFlashTime, NULL_MESSAGE_DATA);

    return  CSM_SCANNING_MODE_WAITING_TARGET_LAMP_ON;
}


//////////////////////////////////////////////////
//
// Message: Target Lamp Is On
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h23(void)
{
        // just wait for the target lamp timeout message

    return  CSM_SCANNING_MODE_WAITING_TARGET_LAMP_TIMEOUT;
}


//////////////////////////////////////////////////
//
// Message: Target Lamp TimedOut
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h24(void)
{
        // Tell TLM to turn the light OFF

        SendHiPrMsg(TargetLampManagerID, TargetLampOff);

    return  CSM_SCANNING_MODE_WAITING_TARGET_LAMP_OFF;
}


//////////////////////////////////////////////////
//
// Message: Target Lamp Is Off
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h25(void)
{
        // Inform NT that Flash process is done

        currTxMboxMsg.Clear();
        currTxMboxMsg.SetCommandAndExtension(CE_FlashTargetLamp);
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        transmitPciMessage();

    return  CSM_SCANNING_MODE_IDLE;
}


//////////////////////////////////////////////////
//
// Message: Point Target Lamp
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h26(void)
{
    // Get data from message

    int xCoordinate = (int)GetCurrEvent().msgData1;
    int yCoordinate = (int)GetCurrEvent().msgData2;

        // Tell HMM to point the target lamp

        SendHiPrMsg(HeadMachinesManagerID, PointTargetLamp,
                        xCoordinate, yCoordinate);

    return  CSM_SCANNING_MODE_POINTING_TARGET_LAMP;
}


//////////////////////////////////////////////////
//
// Message: Point Target Lamp Done
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h27(void)
{
        // Inform NT that Target Lamp is pointed

        currTxMboxMsg.Clear();
        currTxMboxMsg.SetCommandAndExtension(CE_PointTargetLamp);
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        transmitPciMessage();

    return  CSM_SCANNING_MODE_IDLE;
}



//////////////////////////////////////////////////
//
// Message: Go Scanning Mode
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h28(void)
{
        if(targetLampIsOff)
        {
                // Inform NT that we are now in Scanning Mode Idle

                currTxMboxMsg.Clear();
                currTxMboxMsg.SetCommandAndExtension(CE_GoScanningMode);
                currTxMboxMsg.SetParam1(NTCOMPLETE);

                transmitPciMessage();

            return  CSM_SCANNING_MODE_IDLE;
        }

        // Tell TLM to turn the light OFF

        SendHiPrMsg(TargetLampManagerID, TargetLampOff);

    return  CSM_TMI_WAITING_FOR_TARGET_LAMP_OFF_SMI;
}


//////////////////////////////////////////////////
//
// Message: Target Lamp Is Off
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h29(void)
{
        ntTargetLampOn = FALSE; // reset flags
        targetLampIsOff = TRUE;

        // Inform NT that we are now in Scanning Mode Idle

        currTxMboxMsg.Clear();
        currTxMboxMsg.SetCommandAndExtension(CE_GoScanningMode);
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        transmitPciMessage();

    return  CSM_SCANNING_MODE_IDLE;
}


//////////////////////////////////////////////////
//
// Message: Go Home Idle
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h30(void)
{
        if(targetLampIsOff)
        {
                // Tell HMM to go home

                SendHiPrMsg(HeadMachinesManagerID, GoHome);

            return  CSM_TMI_GOING_HOME_IAH;
        }

        // Tell TLM to turn the light OFF

        SendHiPrMsg(TargetLampManagerID, TargetLampOff);

    return  CSM_TMI_WAITING_FOR_TARGET_LAMP_OFF_SMI;
}


//////////////////////////////////////////////////
//
// Message: Target Lamp Is Off
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h31(void)
{
        // Tell HMM to go home

        SendHiPrMsg(HeadMachinesManagerID, GoHome);

    return  CSM_TMI_GOING_HOME_IAH;
}


//////////////////////////////////////////////////
//
// Message: Go Home Done
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h32(void)
{
        ntTargetLampOn = FALSE; // reset flags
        targetLampIsOff = TRUE;

        // Inform NT that we are now in Idle At Home

        currTxMboxMsg.Clear();
        currTxMboxMsg.SetCommandAndExtension(CE_GoHomeIdle);
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        transmitPciMessage();

    return  CSM_IDLE_AT_HOME;
}



//////////////////////////////////////////////////
//
// Message: Move To Position
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h33(void)
{
        // Check current Target Lamp Status

        if((ntTargetLampOn == TRUE) && targetLampIsOff)
        {
                // Tell TLM to turn the light ON

                SendHiPrMsg(TargetLampManagerID, TargetLampOn);

            return  CSM_TRACKBALL_MODE_WAITING_FOR_LAMP_ON_MTP;
        }

        // Tell HMM to move to position requested

        SendHiPrMsg(HeadMachinesManagerID, MoveToPosition);

    return  CSM_TRACKBALL_MODE_MOVING_TO_POSITION;
}


//////////////////////////////////////////////////
//
// Message: Target Lamp Is On
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h34(void)
{
        targetLampIsOff = FALSE; // T Lamp is now ON

        // Tell HMM to move to position requested

        SendHiPrMsg(HeadMachinesManagerID, MoveToPosition);

    return  CSM_TRACKBALL_MODE_MOVING_TO_POSITION;
}


//////////////////////////////////////////////////
//
// Message:  Move To Position Done
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h35(void)
{
        // Inform NT that move to position is now complete

        currTxMboxMsg.Clear();
        currTxMboxMsg.SetCommandAndExtension(CE_MoveToPosition);
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        transmitPciMessage();

    return  CSM_TRACKBALL_MODE_IDLE;
}


//////////////////////////////////////////////////
//
// Message: Point Target Lamp
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h36(void)
{
        // Check current Target Lamp Status

        if((ntTargetLampOn == TRUE) && targetLampIsOff)
        {
                // Tell TLM to turn the light ON

                SendHiPrMsg(TargetLampManagerID, TargetLampOn);

            return  CSM_TRACKBALL_MODE_WAITING_FOR_LAMP_ON_PTL;
        }

        // Tell HMM to point target lamp to position requested

        SendHiPrMsg(HeadMachinesManagerID, PointTargetLamp);

    return  CSM_TRACKBALL_MODE_MOVING_TO_POSITION;
}


//////////////////////////////////////////////////
//
// Message: Target Lamp Is On
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h37(void)
{
        targetLampIsOff = FALSE; // T Lamp is now ON

        // Tell HMM to point target lamp to position requested

        SendHiPrMsg(HeadMachinesManagerID, PointTargetLamp);

    return  CSM_TRACKBALL_MODE_MOVING_TO_POSITION;
}


//////////////////////////////////////////////////
//
// Message:  Point Target Lamp Done
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h38(void)
{
        // Inform NT that point target lamp is now complete

        currTxMboxMsg.Clear();
        currTxMboxMsg.SetCommandAndExtension(CE_PointTargetLamp);
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        transmitPciMessage();

    return  CSM_TRACKBALL_MODE_IDLE;
}


//////////////////////////////////////////////////
//
// Message: Target Lamp Off
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h39(void)
{
        if(targetLampIsOff)
        {
                // Inform NT that is already off

                currTxMboxMsg.Clear();
                currTxMboxMsg.SetCommandAndExtension(CE_TurnOffTargetLamp);
                currTxMboxMsg.SetParam1(NTCOMPLETE);

                transmitPciMessage();

            return  CSM_TRACKBALL_MODE_IDLE;
        }

        ntTargetLampOn = FALSE;

        // Tell TLM to turn the light OFF

        SendHiPrMsg(TargetLampManagerID, TargetLampOff);

    return  CSM_TRACKBALL_MODE_WAITING_TARGET_LAMP_OFF;
}


//////////////////////////////////////////////////
//
// Message: Target Lamp Is Off
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h40(void)
{
        targetLampIsOff = TRUE;

        // Inform NT that the target lamp is now OFF

        currTxMboxMsg.Clear();
        currTxMboxMsg.SetCommandAndExtension(CE_TurnOffTargetLamp);
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        transmitPciMessage();

    return  CSM_TRACKBALL_MODE_IDLE;
}


//////////////////////////////////////////////////
//
// Message: Target Lamp On
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h41(void)
{
        ntTargetLampOn = TRUE;

        if(targetLampIsOff)
        {
                // Tell TLM to turn the light ON

                // Debug
                // Use max Target Lamp On Time

                SendHiPrMsg(TargetLampManagerID, TargetLampOn);

            return  CSM_TRACKBALL_MODE_WAITING_TARGET_LAMP_ON;

        }

        // Inform NT that is already on

        currTxMboxMsg.Clear();
        currTxMboxMsg.SetCommandAndExtension(CE_TurnOnTargetLamp);
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        transmitPciMessage();

    return  CSM_TRACKBALL_MODE_IDLE;
}


//////////////////////////////////////////////////
//
// Message: Target Lamp Is On
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h42(void)
{
        targetLampIsOff = FALSE;

        // Inform NT that the target lamp is now ON

        currTxMboxMsg.Clear();
        currTxMboxMsg.SetCommandAndExtension(CE_TurnOnTargetLamp);
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        transmitPciMessage();

    return  CSM_TRACKBALL_MODE_IDLE;
}


//////////////////////////////////////////////////
//
// Message: Blink Target Lamp
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h43(void)
{
        targetLampBlinkTime = BLINK_TIME_UNITS((WORD)GetCurrEvent().msgData1);

        // Tell TLM to turn the light OFF

        SendHiPrMsg(TargetLampManagerID, TargetLampOff);

    return  CSM_TRACKBALL_MODE_WAITING_TARGET_LAMP_OFF_BLINK;
}


//////////////////////////////////////////////////
//
// Message: Target Lamp Is Off
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h44(void)
{
        StartHiPriorityTimer(targetLampBlinkTime);

        // wait for the timeout message

    return  CSM_TRACKBALL_MODE_BLINK_DELAY;
}


//////////////////////////////////////////////////
//
// Message: TimeOut
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h45(void)
{
        // Tell TLM to turn the light ON

        SendHiPrMsg(TargetLampManagerID, TargetLampOn);

    return  CSM_TRACKBALL_MODE_WAITING_TARGET_LAMP_ON_BLINK;
}


//////////////////////////////////////////////////
//
// Message: Target Lamp Is On
//
//////////////////////////////////////////////////

WORD
ColorSmartManager::CSM_h46(void)
{
        ntTargetLampOn = TRUE;
        targetLampIsOff = FALSE;

        // Inform NT that Blink process is done

        currTxMboxMsg.Clear();
        currTxMboxMsg.SetCommandAndExtension(CE_BlinkTargetLamp);
        currTxMboxMsg.SetParam1(NTCOMPLETE);

        transmitPciMessage();

    return  CSM_TRACKBALL_MODE_IDLE;
}













