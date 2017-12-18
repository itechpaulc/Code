



/*
 *
 *
 *    $Header:   L:/RR/PROJ829/tcm2/source/TASKS.C_v   1.5   05 Aug 1998 09:09:22   PaulLC  $
 *    $Log:   L:/RR/PROJ829/tcm2/source/TASKS.C_v  $
 * 
 *    Rev 1.5   05 Aug 1998 09:09:22   PaulLC
 * Camera Frame White Streak fixed in this version. A new message was added to allow the OP to query the 
 * TCM system parameters. The Camera/Measure capture fast, now controls the Camera/Measure Process Flags.
 * 
 *    Rev 1.4   31 Jul 1998 09:12:28   PaulLC
 * Fixed issue where the measurement flash could be delayed by as much as 65msec. 
 * The Measurement Flash setup index distance is now a parameter downloaded by the OP. 
 * 
 *    Rev 1.3   24 Jul 1998 11:19:22   PaulLC
 * Reset Measure FIFO was moved from the "Measure Flash Setup" section over to the 
 * "Synchronize Command" section. A Mirror flag was added for the Measurement Process
 * to allow the OP to check its completion. Relative Encoder Index is now updated on each
 * Encoder Pulse Task. The Last Free Run pulse is now triggered for the Measure Capture 
 * Fast Process.
 * 
 *    Rev 1.2   21 Jul 1998 16:42:42   PaulLC
 * Optimized floating point calculations to be done in the OP. The Synchronize message was
 * revised to accomodate the new OP Data. Divide by 64K were optimized for speed. Redesigned
 * Measure Free run and Synchronize Message task to be encoder-based processes.
 * 
 *    Rev 1.1   06 Jul 1998 17:43:22   MARKC
 * version 1.0.c
 * chgd NormalizeEncoderIndex
 * (to fix camera position bug at encoder index of zero)
 * 
 *    Rev 1.0   11 Jun 1998 13:12:16   Paul L C
 *  
 *
 *
 *      Author : Paul Calinawan, Mark Colvin
 *
 *      April 1998
 *
 *
 *          Graphics Microsystems Inc
 *          Color Measurement Group
 *          10375 Brockwood Road
 *          Dallas, TX 75238
 *
 *          (214) 340-4584
 *
 *
 *      Road Runner PC/AT Timing and Control Module
 *      -------------------------------------------
 *
*/

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



#include "tcm2.h"


#include <intrins.h>




//////////////////////////////////////////////////
//
// Local Functions
// Forward declaration
//
//////////////////////////////////////////////////

void    CheckForSynchronization(void);

void    CheckForPendingProcesses(void);

BOOL    IsInNextToLastMeasureReadEncoderIndex(void);

void    UpdateRelCurrentEncoderIndex(void);

//////////////////////////////////////////////////
//
// Camera and Measurement Position Markers
//
//////////////////////////////////////////////////

EncoderMark     cameraFlashEncoderMark;

WORD            cameraPreProcessSetupIndex;


EncoderMark     measurementFlashEncoderMark;

WORD            measurementPreProcessSetupIndex;

BYTE            measurementFlashSetupDistance;


EncoderMark     lastMeasFreeRunEncoderMark;

WORD            lastMeasFreeRunSetupIndex;


AdjustMark     	measurementFlashAdjustMark;


//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

EncoderMark     lastMeasFreeRunEncoderMark;

WORD            lastMeasFreeRunSetupIndex;


//////////////////////////////////////////////////
//
// Private Data
//
//////////////////////////////////////////////////

BYTE    bdata   PCA_EVENT_FLAGS = 0x00;


sbit    MEASUREMENT_FLASH_EVENT     = PCA_EVENT_FLAGS ^ 0;
sbit    CAMERA_FLASH_EVENT          = PCA_EVENT_FLAGS ^ 1;
sbit    ENCODER_PULSE_EVENT         = PCA_EVENT_FLAGS ^ 2;
sbit    END_OF_GRABFRAME_EVENT      = PCA_EVENT_FLAGS ^ 3;
sbit    FLASH_ADJUST_EVENT          = PCA_EVENT_FLAGS ^ 4;
sbit    OVER_FLOW_EVENT             = PCA_EVENT_FLAGS ^ 5;


BOOL    PC_MESSAGE_RECEIVED_FLAG    = FALSE;
BOOL    PC_REPLY_SENT_FLAG          = FALSE;
BOOL    FREE_RUN_TIMER_ELAPSED_FLAG = FALSE;


BYTE    bdata   ERROR_EVENT_FLAGS;

sbit    PC_COMM_ERROR           = ERROR_EVENT_FLAGS ^ 0;


BOOL        normalQuadratureEncoderDirection;
BOOL        useImpressionPulse;
BOOL        levelImpressionPulse;

BOOL        measurementProcessPendingFlag;	// normal encoder timed
BOOL        cameraProcessPendingFlag;		//   cam/meas cycles

BOOL        measureCaptureFastPendingFlag;	// non encoder timed
BOOL        cameraCaptureFastPendingFlag;	//   cam/meas cycles

BOOL        synchronizeFlag;

BOOL        holdProcessesFlag;              // hold cam/meas process 'til next
                                            // impression
BOOL        checkForPendingProcess;

BYTE        cameraState;                    // state for camera flash handling

BYTE        measurementFreeRunState;

BYTE    	encoderNotRunningCount;

BOOL        encoderRunning;                 // we have encoder processing
BOOL        pressSpeedZero;                 // press not running - no encoder

WORD        encoderPeriod;                  // avg time of encoder pulses(usec)


EncoderMark measReadFrameLeftOverEncoderDistance;


#define     IS_ENCODER_RUNNING()                (encoderRunning == TRUE)
#define     SET_ENCODER_RUNNING_FLAG()          (encoderRunning = TRUE)
#define     CLEAR_ENCODER_RUNNING_FLAG()        (encoderRunning = FALSE)

#define     SET_PRESS_SPEED_ZERO_FLAG()         (pressSpeedZero = TRUE)
#define     CLEAR_PRESS_SPEED_ZERO_FLAG()       (pressSpeedZero = FALSE)


WORD      	lastMeasReadStartFrameEncoderIdx;
WORD      	lastMeasReadEndFrameEncoderIdx;
WORD        lastMeasFreeRunIdx;

BOOL        adjustmentOkFlag, adjustmentNeededFlag, adjustmentLateFlag;

int         relCurrentEncoderIndex;


//////////////////////////////////////////////////
//
// System Data Parameters
//
// TCM Parameters, Down-Loadable
//
//////////////////////////////////////////////////

WORD            measFrameTime;

WORD            camFrameReadoutTime;

EncoderMark     impressionLength;

WORD            measurementFreeRunIntervalTime;

DWORD           encoderImpressionCount;



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    InitSystemData(void)
{
    SetMeasurementFrameTime(MEASUREMENT_FRAME_READOUT_TIME);
    SetCameraFrameReadoutTime(CAMERA_FRAME_READOUT_TIME);

    SetImpressionLength(IMPRESSION_LENGTH_ENCODER_INDEX,
                        IMPRESSION_LENGTH_ENCODER_REMAINDER);

    SetMeasurementFreeRunIntervalTime(MEASUREMENT_FREE_RUN_TIME_INTERVAL);

    ClearInstantaneousEncoderImpressionCount();
    ClearEncoderImpressionCount();

    SetNormalQuadratureEncoderDirection();

    SetMeasurementFlashSetupDistance(MEASUREMENT_PREPROCESS_SETUP_INDEX_DISTANCE);
}


//////////////////////////////////////////////////
//
// These times based on 87251SB running 12MHz and 
// On-chip program.
//
//////////////////////////////////////////////////

void    DELAY_2_USEC(void)
{
    _nop_();    _nop_();    _nop_();    _nop_();
    _nop_();    _nop_();    _nop_();    _nop_();
}

void    DELAY_5_USEC(void)
{
    _nop_();    _nop_();    _nop_();    _nop_();
    _nop_();    _nop_();    _nop_();    _nop_();
    _nop_();    _nop_();    _nop_();    _nop_();
    _nop_();    _nop_();    _nop_();    _nop_();
    _nop_();    _nop_();    _nop_();    _nop_();
}

//////////////////////////////////////////////////
//
// This function examines an encoder index
// and changes its value as necessary (if negative)
// to give it a new value that is within the
// valid range based on the Impression Length Data
//
//////////////////////////////////////////////////

void    NormalizeEncoderIndex(signed int *encoderIdx)
{
    if((*encoderIdx) < 0)
        (*encoderIdx) += GetImpressionLengthIndex();
}


//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

void    UpdateRelCurrentEncoderIndex(void)
{
    RelSetCurrentEncoderIndex
        (CalculateRelativeIndexBasedOnMeasureFlashIndex
        (GetCurrentEncoderIndex()));
}


//////////////////////////////////////////////////
//
// This function determines the timer ticks there
// are in a given Encoder Mark Distance. The
// number of Encoder pulses as well as the
// remainder data is used in the calculation.
// Calculation uses the current encoder speed
//
//////////////////////////////////////////////////

WORD    GetTimerTicksOnEncoderMarker(EncoderMark * ptrEncMark)
{
    DWORD_UNION tempEncoderTicks;

    WORD    encoderIdxTicks = 0;
    WORD    encoderRemainderTicks = 0;

        encoderIdxTicks = ptrEncMark->encoderIndex * GetEncoderPeriod();

        // Calculate, store in a DWORD, take HIWORD result
        // to divide by 64K

        tempEncoderTicks.tempDWORD = (DWORD)ptrEncMark->encoderRemainder * GetEncoderPeriod();
        encoderRemainderTicks = tempEncoderTicks.tempWORD;

    return (encoderIdxTicks + encoderRemainderTicks);
}



//////////////////////////////////////////////////
//                                                                     
//	Loads Encoder index and remainder into
//	EncoderMark structure
//
//////////////////////////////////////////////////

void    SetEncoderMark(EncoderMark *ptrEncMark, WORD eIdx, WORD eRem)
{
    ptrEncMark->encoderIndex = eIdx;
    ptrEncMark->encoderRemainder = eRem;
}


//////////////////////////////////////////////////
//
//	Loads Encoder Adjust index and replacement remainder into
//	AdjustMark structure
//
//////////////////////////////////////////////////

void    SetAdjustMark(AdjustMark *ptrAdjMark, signed int eIdx, WORD eRem)
{
    ptrAdjMark->encoderIndex = eIdx;
    ptrAdjMark->encoderRemainder = eRem;
}


//////////////////////////////////////////////////
//
//	Foreground task to cleanup after measure flash
//	(restart the freerun handler)
//
//////////////////////////////////////////////////

#define     MAX_MEAS_FLAG_CLEAR_RETRY    10

void    ClearMeasurementProcessFlagMirror(void)
{
    measurementProcessFlagMirror = MEAS_FLAG_CLEAR;
}

void    DoMeasurementFlashTask(void)
{
    WORD    timer0RestartPoint;

    BYTE    i = 0, flagForceRead;

        for(i; i < MAX_MEAS_FLAG_CLEAR_RETRY; i++)
        {
            ClearMeasurementProcessFlagMirror();

            // Check to make sure Dual Port Ram accepted data

            flagForceRead = measurementProcessFlagMirror;

            if(flagForceRead == MEAS_FLAG_CLEAR)
                break;
        }

        CLEAR_MEASUREMENT_PROCESS_PENDING_FLAG();

        CLEAR_MEASUREMENT_FLASH_EVENT();

        // Set up measurement Free run again

        timer0RestartPoint =
            GetFreeRunStartPoint(measurementFreeRunIntervalTime);

        TL0 = LOBYTE(timer0RestartPoint);
        TH0 = HIBYTE(timer0RestartPoint);

        measurementFreeRunState = MEASUREMENT_FREE_RUNNING;

        RESTART_MEASUREMENT_FREE_RUN_TIMER();
}

//////////////////////////////////////////////////
//
//	Foreground task to handle the camera cycle state
//	machine.
//	0 = idle
//	1 = time between encoder index start and camera
//		freerun stop (setup the camera flash time and
//		bit toggle)
//	2 = time between freerun stop and camera flash
//		(setup the frame grab time and toggle)
//
//////////////////////////////////////////////////

void    DoCameraFlashTask(void)
{
	WORD    cameraEventOutputCompareTime;

    	if(cameraState == WAITING_FOR_LAST_CAMERA_FREE_RUN)    // before camera flash
	    {
            // Set up Camera Flash Event Time (PCA TIMER 1)

            cameraEventOutputCompareTime = current_PCA_time + GetCameraFrameReadoutTime();

            CCAP1L = LOBYTE(cameraEventOutputCompareTime);
            CCAP1H = HIBYTE(cameraEventOutputCompareTime);

            ENABLE_FLASH_TOGGLE_1();                            // bit toggle now

            cameraState = WAITING_FOR_CAMERA_FLASH;             // toggle flash now
    	}
	    else if(cameraState == WAITING_FOR_CAMERA_FLASH)        // after camera flash
    	{
            // Set up End Of Grab Frame Event Time (PCA TIMER 3)

   	        cameraEventOutputCompareTime = current_PCA_time + GetCameraFrameReadoutTime();

            CCAP3L = LOBYTE(cameraEventOutputCompareTime);
            CCAP3H = HIBYTE(cameraEventOutputCompareTime);

            ENABLE_OUTPUT_COMPARE_3();

            cameraState = CAMERA_FREE_RUNNING;                  // idle state now

            CLEAR_CAMERA_PROCESS_PENDING_FLAG();
    	}

    CLEAR_CAMERA_FLASH_EVENT();
}


//////////////////////////////////////////////////
//
//	Foreground task to handle encoder pulses:
//	1. Check for encoder based processes
//	2. Handle encoder index movement
//	3. Check encoder index reset
//	4. Update encoder time average
//	5. Check impression pulse
//
//////////////////////////////////////////////////

void    DoEncoderPulseTask(void)
{
        // Update Foreground Variables

        SetEncoderPeriod(GetInstantaneousEncoderPeriod());
        SetCurrentEncoderIndex(GetInstantaneousCurrentEncoderIndex());
        SetCurrentEncoderRemainder(GetInstantaneousCurrentEncoderRemainder());
        SetEncoderImpressionCount(GetInstantaneousEncoderImpressionCount());

        UpdateRelCurrentEncoderIndex();

        // Unconditionally clear the holdoff flag if we are
        // at the Measurement Flash Index

        if(GetCurrentEncoderIndex() == GetMeasurementFlashEncoderIndex())
            CLEAR_HOLD_PROCESSES_FLAG();

        if(IS_SYNCHRONIZE_PROCESSES())
        {
            CheckForSynchronization();

            CLEAR_SYNCHRONIZE_PROCESSES_FLAG();
        }

        if(DO_CHECK_FOR_PENDING_PROCESS())
        {
            CheckForPendingProcesses();

            CLEAR_CHECK_FOR_PENDING_FLAG();
        }

    SET_ENCODER_RUNNING_FLAG();    // For press zero speed detect

    CLEAR_ENCODER_PULSE_EVENT();
}


//////////////////////////////////////////////////
//
// Both Camera and Measurement Processes were
// posted. This function decides if they need
// to be held off until the next impression.
//
//////////////////////////////////////////////////

void    CheckForSynchronization(void)
{
    WORD    timeToNextFreeRunPulse =
                TIMER_0_OVERFLOW_VALUE - ((TH0 << 8 ) | TL0);

    WORD    encoderIndexToNextFreeRunPulse =
                1 + GetCurrentEncoderIndex() + (timeToNextFreeRunPulse / GetEncoderPeriod());

    signed int      relEncoderIndexToNextFreeRunPulse;

        SET_MEASUREMENT_PROCESS_PENDING_FLAG();
        SET_CAMERA_PROCESS_PENDING_FLAG();

        relEncoderIndexToNextFreeRunPulse =
            CalculateRelativeIndexBasedOnMeasureFlashIndex(encoderIndexToNextFreeRunPulse);

        // Check the cases where the Camera and Measure
        // Process should be put on Hold.

        if((RelGetCurrentEncoderIndex() >= RelGetCameraSetupIndex()) ||
           (RelGetCurrentEncoderIndex() >= RelGetHoldOffIndex()) ||
           (relEncoderIndexToNextFreeRunPulse >= RelGetHoldOffIndex()))
        {
            SET_HOLD_PROCESSES_FLAG();
        }
}


//////////////////////////////////////////////////
//
//	Foreground task to check for processes which are
//	encoder index driven.
//
//////////////////////////////////////////////////

void    CheckForPendingProcesses(void)
{
    DWORD_UNION     tempCamMeas;

    WORD    measureFlashOutputCompareTime, measureRemainderTime;
    WORD    cameraFlashOutputCompareTime, cameraRemainderTime;

    WORD    timer0RestartPoint, timeToMeasureFlashFromLastEncoderIndex;

            // Check if Free Run has changed state to make way for the
            // camera and measurement process

            if((IS_MEASUREMENT_WAITING_FOR_LAST()) &&
               (GetCurrentEncoderIndex() == GetLastMeasReadEndFrameEncoderIdx()))
            {
                measReadFrameLeftOverEncoderDistance.encoderIndex =
                    CalculateRelativeIndexBasedOnMeasureFlashIndex(GetLastMeasFreeRunIndex()) -
                    RelGetCurrentEncoderIndex();

                measReadFrameLeftOverEncoderDistance.encoderRemainder =
                    GetLastMeasFreeRunRemainder();

                // Calculate time equivalent

                timeToMeasureFlashFromLastEncoderIndex =
                    GetTimerTicksOnEncoderMarker(&measReadFrameLeftOverEncoderDistance);

                timer0RestartPoint =
                    TIMER_0_OVERFLOW_VALUE - timeToMeasureFlashFromLastEncoderIndex;

                TL0 = LOBYTE(timer0RestartPoint);
                TH0 = HIBYTE(timer0RestartPoint);
            }

            // Check if time for Measurement Process/Flash to be set up
            // At Measurement Encoder Index -1, also make sure that
            // the Free run has paused to give way to the real measurement

            if((IS_MEASUREMENT_PROCESS_PENDING()) &&
               (IS_MEASUREMENT_FREE_RUN_PAUSED() || IS_MEASUREMENT_WAITING_FOR_LAST())&&
               (!IS_HOLD_PROCESSES()) &&
               (GetCurrentEncoderIndex() == GetMeasurementPreProcessSetupIndex()))
            {
                // Time to set the Measurement Flash Trigger
                // Prepare flash to be toggled/triggered
                // Setup the OUTPUT Compare times

                if(IS_ADJUSTMENT_NEEDED())
		    	{
                    // Use flash adjustment here

                    // Calculate, store in a DWORD, take HIWORD result
                    // to divide by 64K

                    tempCamMeas.tempDWORD = (DWORD)GetMeasurementAdjustRemainder() * GetEncoderPeriod();
                    measureRemainderTime = tempCamMeas.tempWORD;

                    // GetMeasurementFlashAdjustIndex() Valid Range : -1, 0, +1 Encoder Index                             

	                measureFlashOutputCompareTime =
	                    GetLastEncoderTimeTag() +                                                   // Last Encoder Input Capture
	                    (GetEncoderPeriod() * 
                        (GetMeasurementFlashSetupDistance() + GetMeasurementFlashAdjustIndex())) +  // + x Encoder Index Time Ticks
    	                measureRemainderTime;                                                       // +   Remainder Time Ticks
    			}
                else
	    		{
                    // No flash adjustment

                    // Calculate, store in a DWORD, take HIWORD result
                    // to divide by 64K

                    tempCamMeas.tempDWORD = (DWORD)GetMeasurementFlashEncoderRemainder() * GetEncoderPeriod();
                    measureRemainderTime = tempCamMeas.tempWORD;

    	            measureFlashOutputCompareTime =
	                    GetLastEncoderTimeTag() +                                   // Last Encoder Input Capture
	                    (GetMeasurementFlashSetupDistance() * GetEncoderPeriod()) + // + 1 Encoder Index Time Ticks
	                    measureRemainderTime;                                       // +   Remainder Time Ticks

				    SET_ADJUSTMENT_LATE_FLAG();	        // too late for flash adjust now.
                }

                CCAP0L = LOBYTE(measureFlashOutputCompareTime);
	            CCAP0H = HIBYTE(measureFlashOutputCompareTime);

                ENABLE_OUTPUT_COMPARE_0();

                //RESET_MEASURE_FIFO();
            }

            // Check if time for Camera Process/Flash to be set up
            //  At Camera Encoder Index -1

            if((IS_CAMERA_PROCESS_PENDING()) &&
               (!IS_HOLD_PROCESSES()) &&
               (GetCurrentEncoderIndex() == GetCameraPreProcessSetupIndex()))
            {
                cameraState = WAITING_FOR_LAST_CAMERA_FREE_RUN;

                // Time to set the Camera Flash Trigger
                // Setup the OUTPUT Compare times

                // Calculate, store in a DWORD, take HIWORD result
                // to divide by 64K

                tempCamMeas.tempDWORD = (DWORD)GetCameraFlashEncoderRemainder() * GetEncoderPeriod();
                cameraRemainderTime = tempCamMeas.tempWORD;

                cameraFlashOutputCompareTime =
                    GetLastEncoderTimeTag() +       // Last Encoder Input Capture
                    (GetEncoderPeriod()*2) +        // + 2 Encoder Index Time Ticks
                    cameraRemainderTime -           // +   Remainder Time Ticks
                    camFrameReadoutTime;            // - Camera Frame time

                CCAP1L = LOBYTE(cameraFlashOutputCompareTime);
                CCAP1H = HIBYTE(cameraFlashOutputCompareTime);

	            ENABLE_OUTPUT_COMPARE_1();		//just interrupt needed, not bit toggle
            }
}





//////////////////////////////////////////////////
//
//	Foreground task - Cleanup the camera cycle here
//
//////////////////////////////////////////////////

#define     MAX_CAM_FLAG_CLEAR_RETRY    10

void    ClearCameraProcessFlagMirror(void)
{
    cameraProcessFlagMirror = CAM_FLAG_CLEAR;
}

void    DoEndOfGrabFrameTask(void)
{
    BYTE i = 0, flagForceRead;

        for(i; i < MAX_CAM_FLAG_CLEAR_RETRY; i++)
        {
            ClearCameraProcessFlagMirror();

            // Check to make sure Dual Port Ram accepted data

            flagForceRead = cameraProcessFlagMirror;

            if(flagForceRead == CAM_FLAG_CLEAR)
                break;
        }

        CLEAR_CAMERA_CAPTURE_FAST_PENDING_FLAG();

        CLEAR_END_OF_GRABFRAME_EVENT();
}


//////////////////////////////////////////////////
//
//	Foreground task for Flash Adjust
//
//////////////////////////////////////////////////

void    DoFlashAdjustTask(void)
{

	// nothing to do

    CLEAR_FLASH_ADJUST_EVENT();
}


//////////////////////////////////////////////////
//
//	Foreground task
// Perform System Checks every Timer Overflows
// (65 msec)
//
//////////////////////////////////////////////////

#define     PRESS_ZERO_SPEED        16

void    DoSystemCheckTask(void)
{
    if(IS_ENCODER_RUNNING())
    {
        encoderNotRunningCount = 0;

        CLEAR_ENCODER_RUNNING_FLAG();
        CLEAR_PRESS_SPEED_ZERO_FLAG();
    }
    else
    {
        // Check for "Press == Zero Speed", 1 second
        // of NO Encoder Pulses

        if(!IS_PRESS_SPEED_ZERO())
        {
            ++encoderNotRunningCount;

            if(encoderNotRunningCount > PRESS_ZERO_SPEED)
            {
                GetEncoderPeriod() = 0;

                SET_PRESS_SPEED_ZERO_FLAG();
            }
        }
    }

    CLEAR_OVER_FLOW_EVENT();
}


//////////////////////////////////////////////////
//
//
// MR = Measure Read (Free Run)
//
//////////////////////////////////////////////////

BOOL    IsInNextToLastMeasureReadEncoderIndex(void)
{
        // Check if Relative Current Encoder index is within
        // Free Run's NextToLastMeasureRead Point Frame Limits

        if((RelGetCurrentEncoderIndex() >= RelGetMeasFreeRunStartIndex()) &&
           (RelGetCurrentEncoderIndex() <= RelGetMeasFreeRunEndIndex()))
        {
            return TRUE;
        }

    return FALSE;
}


//////////////////////////////////////////////////
//
//	Foreground task - Handle Measure Freerun timer
//
//////////////////////////////////////////////////

void    HandleFreeRunTimerElapsed(void)
{
    WORD    timer0RestartPoint;

    BYTE    i = 0, flagForceRead;

        if(IS_MEASUREMENT_PROCESS_PENDING() && (!IS_HOLD_PROCESSES()))
        {
            if(!IS_PRESS_SPEED_ZERO())
            {
                PULSE_MEAS_READ_TRIGGER();

                if(measurementFreeRunState == MEASUREMENT_FREE_RUNNING)
                {
                    if(IsInNextToLastMeasureReadEncoderIndex())
                    {
                        measurementFreeRunState = WAITING_FOR_LAST_MEASUREMENT_READ;
                    }
                    else
                    {
                        // Not yet. Do another Free Run

                        timer0RestartPoint =
                            GetFreeRunStartPoint(measurementFreeRunIntervalTime);

                        measurementFreeRunState = MEASUREMENT_FREE_RUNNING;
                    }
                }
                else
                {
                    // measurementFreeRunState == WAITING_FOR_LAST_MEASUREMENT_READ
                    // Last Measurement Read, Trigger the Measure Read Line

                    // PAUSING the measurement free run will allow the encoder processing
                    // task to decide when to setup the real measurement

                    PAUSE_MEASUREMENT_FREE_RUN_TIMER();

                    measurementFreeRunState = PAUSED_MEASUREMENT_FREE_RUN;
                }
            }
            else
            {
                // Press speed is ZERO
                // Need to perform Measurement without using Encoder Processing

                if(measurementFreeRunState == MEASUREMENT_FREE_RUNNING)
                {
                    timer0RestartPoint = GetFreeRunStartPoint(GetMeasurementFrameTime());

                    measurementFreeRunState = WAITING_FOR_MEASUREMENT_TIME;
                }
                else
                {
                    // Time to perform the real measurement, at Press speed Zero
                    // "Force" Toggle the measurement flash port

                    RESET_MEASURE_FIFO();

                    TRIGGER_MEASUREMENT_FLASH();

                    PULSE_MEAS_READ_TRIGGER();

                    CLEAR_MEASUREMENT_PROCESS_PENDING_FLAG();

                    measurementFreeRunState = MEASUREMENT_FREE_RUNNING;
                }
            }
        }
        else
        {
            if(IS_MEASURE_CAPTURE_FAST_PENDING())
            {
                // Measure ASAP without considering the Encoder Processing

                if(measurementFreeRunState == MEASUREMENT_FREE_RUNNING)
                {
                    PULSE_MEAS_READ_TRIGGER();  // Trigger the Measure Read Line

                    timer0RestartPoint =
                        GetFreeRunStartPoint(GetMeasurementFrameTime());

                    measurementFreeRunState = WAITING_FOR_MEASUREMENT_CAPTURE_FAST_TIME;
                }
                else
                {
                    // Time to perform a real measurement, at ANY Press speed at
                    // ANY encoder position, "Force" Toggle the measurement flash port

                    RESET_MEASURE_FIFO();

                    TRIGGER_MEASUREMENT_FLASH();

                    // Delay Measure Read After the Flash

                    DELAY_5_USEC();
                    DELAY_5_USEC();

                    PULSE_MEAS_READ_TRIGGER();

                    CLEAR_MEASURE_CAPTURE_FAST_PENDING_FLAG();

                    for(i; i < MAX_MEAS_FLAG_CLEAR_RETRY; i++)
                    {
                        ClearMeasurementProcessFlagMirror();

                        // Check to make sure Dual Port Ram accepted data

                        flagForceRead = measurementProcessFlagMirror;

                        if(flagForceRead == MEAS_FLAG_CLEAR)
                            break;
                    }

                    // Back to regular free run

                    timer0RestartPoint =
                        GetFreeRunStartPoint(measurementFreeRunIntervalTime);

                    measurementFreeRunState = MEASUREMENT_FREE_RUNNING;
                }
            }
            else
            {
                // Normal Measurement Read Free Run
                // Set up for the next regular free run time

                PULSE_MEAS_READ_TRIGGER();  // Trigger the Measure Read Line

                timer0RestartPoint =
                    GetFreeRunStartPoint(measurementFreeRunIntervalTime);

                measurementFreeRunState = MEASUREMENT_FREE_RUNNING;
            }

        } // end - IS_MEASUREMENT_PROCESS_PENDING()

        if((measurementFreeRunState == MEASUREMENT_FREE_RUNNING) ||
           (measurementFreeRunState == WAITING_FOR_MEASUREMENT_CAPTURE_FAST_TIME))
        {
            TL0 = LOBYTE(timer0RestartPoint);
            TH0 = HIBYTE(timer0RestartPoint);
        }

        CLEAR_FREE_RUN_TIMER_ELAPSED_FLAG();
}



