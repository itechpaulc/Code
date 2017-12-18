


//
//	#include "motor.h"
//



/////////////////////////////////////////////////////////////////////////////
//
//
//	$Header:   N:/pvcs52/projects/pcu100~1/motor.c_v   1.9   Jun 23 1997 10:28:54   Paul L C  $
//	$Log:   N:/pvcs52/projects/pcu100~1/motor.c_v  $
//
//   Rev 1.9   Jun 23 1997 10:28:54   Paul L C
//Corrected an error where the PCU does not  move if ANY of
//the limits are asserted. This is now change so that the PCU
//can get out of the limit position. A function name was changed
//for consistency.
//
//   Rev 1.8   May 16 1997 14:55:14   Paul L C
//Corrected an error where the motor can go out of bounds
//but does not detect the stall condition. Corrected an error
//where the motor reports a stall if the requested target is 
//the same as the current position.
//
//   Rev 1.7   May 07 1997 09:24:02   Paul L C
//Made functions to be expanded "inline" to reduce RAM stack usage.
//Moved some variables into the header files for visibility.
//
//   Rev 1.6   May 02 1997 14:01:14   Paul L C
//Implemented HardWare Limit Cheking wihtin the motor
//moving states. Errors are logged if detected.  Added setting 
//of the MinMax AtoD range. Corrected IsTargetWithinLimit funtion -
//it was not returning TRUE or FALSE.
//
//   Rev 1.5   Apr 11 1997 10:20:50   Paul L C
//Implemented new states to perform nudging as the final
//reverse positioning move. Cleaned up code.
//
//   Rev 1.4   Mar 25 1997 09:41:40   Paul L C
//Added a check to determine if the requested target is within
//range. The motor will not move if target is out of range. Added
//a function GetMinReverse/ForwardMove to integrate several
//procedure calls. Initialize MotorStatus register to MoveDone.
//Added Send Message for AtoD to GoFastActive before motor
//power is applied. Motor power now is applied in the moving
//state. This improves the consistency of the motor timers.
//
//
//
//   Rev 1.3   Mar 18 1997 08:30:46   Paul L C
//MCM_REVERSE_COASTING was not spelled correctly.
//This error was flagged by the new compiler and is now fixed.
//
//   Rev 1.2   Mar 06 1997 11:02:52   Paul L C
//Fixed an error where the first nudge "to position" was always
//big. Made PositionChanged? functions to be inline. Also inlined
//IsInMin/MaxPosition? functions. Improved nudge timing accuracy
//where we first expire the timer before starting a new one. Added a
//check for NudgeOnTime = 0, for motor to just remain IDLE.
//
//   Rev 1.1   Mar 04 1997 15:29:38   Paul L C
//Slow sampling rate implemented when motor is in
//the settling state. 10 samples are now taken after
//the motor is done moving and in the settling state.
//Added a define for position mask in
//the function SetTarget.
//
//   Rev 1.0   Feb 26 1997 10:54:40   Paul L C
//Initial Revision
//
//
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////
//
// Note :
//
//   Due to the RAM size limitation, functions-
//   calling-functions are avoided as much as
//   possible. (to conserver stack, RAM space).
//
//   ROM size is not a concern (@ 8K).
//
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
// Private Members
//
//////////////////////////////////////////////////

WORD	originalTarget,

		prevPosition;

BYTE	timeOutTimer;



#define		TurnMotorForward		(PORT_A.FRWRD_PORT = CLEAR)
#define		TurnMotorReverse		(PORT_A.RVRSE_PORT = CLEAR)

#define		MotorForwardOff			(PORT_A.FRWRD_PORT = SET)
#define		MotorReverseOff			(PORT_A.RVRSE_PORT = SET)

#define		TurnMotorOff			MotorForwardOff;MotorReverseOff;


#define		ResetTimeOutTimer       (timeOutTimer = 0)
#define		IncrementTimeOutTimer   (timeOutTimer++)

#define		LogMotorStalledStatus	(motorStatus = Stall)
#define		LogMotorMoveDone        (motorStatus = MoveDone)

#define		LogStall				LogMotorStalledStatus;LogStalledMotorErr;



//////////////////////////////////////////////////
//
// Virtual Message Interface
//
//////////////////////////////////////////////////




//////////////////////////////////////////////////
//
// State Machine Initialization
//
//////////////////////////////////////////////////

BYTE	Construct_MotorController(void) {

		TurnMotorOff;

		minMove = INITIAL_MIN_MOVE;

		nudgeOnTime		= INITIAL_NUDGE_ON_TIME;

		movingTimeOut 	= INITIAL_MOVING_TIMEOUT;
		coastingTimeOut = INITIAL_COASTING_TIMEOUT;

		distanceToNudge = INITIAL_DISTANCE_TO_NUDGE;

		// Initialize Limits

		SetMinPosition(MIN_MOTOR_LIMIT);
		SetMaxPosition(MAX_MOTOR_LIMIT);

		motorStatus = MoveDone;

	return MCM_IDLE;
}


//////////////////////////////////////////////////
//
// MoveToTarget message, while in Idle
//
//////////////////////////////////////////////////

#define	ATOD_RFRSH	5

BYTE	MCM_exitA(void)
{
	motorStatus = NoStatus;

		// Is target within range ?

		if(IsTargetWithinLimits())
		{
			ResetTimeOutTimer;

			if(targetPosition > GetMinForwardMove()) {

					// Make AtoD driver sample as fast as possible

					SendMessage(GoAtoDfastActive, ATOD_DRIVER_SM_ID);
					StartTimer(ATOD_RFRSH); // Time to Refresh AtoD

				return MCM_FRWRD_MOVING;
			}

			if(targetPosition < GetMinReverseMove()) {

					if(GetCurrPosition() - targetPosition < distanceToNudge)
					{
							// Distance is small enough to nudge

							SendMessage(NudgeToTarget,THIS_SM);

						return MCM_IDLE;
					}


					// Make AtoD driver sample as fast as possible

					SendMessage(GoAtoDfastActive, ATOD_DRIVER_SM_ID);
					StartTimer(ATOD_RFRSH); // Time to Refresh AtoD


					// Setup For Reverse Nudging as the final move

					originalTarget = targetPosition;	// keep original target

					targetPosition += distanceToNudge;

				return MCM_RVRSE_MOVING;
			}

			// just get ready for the next command

			LogMotorMoveDone;

			goto alreadyInMoveTarget;

		}

	// Target Out of range

	LogTargetRangeErr;


alreadyInMoveTarget:

	SendMessage(MotorMoveDone,PCUMNGR_SM_ID);

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// TimeOut while in Moving Forward
//
//////////////////////////////////////////////////

BYTE	MCM_exitB(void)
{
		if(GetCurrPosition() >= targetPosition)
		{
				// Forward Target is Reached !

				TurnMotorOff;
				ResetTimeOutTimer;
				UpdateMinForwardMove();
				StartTimer(COASTING_TIMER_TICK);

			return MCM_FRWRD_COASTING;
		}

		if(PosChangedForward())
		{
			ResetTimeOutTimer;

			UpdateMinForwardMove(); // New min move
		}
		else
		{
			if(IsInMaxHardwareLimit()) {

					LogHWlimitStall;

				goto frwrdMoveDone;
			}

			IncrementTimeOutTimer;

			if(timeOutTimer == movingTimeOut) {

					// Motor has stalled
					// while in Moving Forward

					LogStall;

				goto frwrdMoveDone;
			}
		}

		TurnOnForwardPower();

		StartTimer(MOVING_TIMER_TICK);

	return SAME_STATE;


frwrdMoveDone:

		TurnMotorOff;

		StartTimer(POSITION_SETTLE_TIMEOUT);

	return	MCM_POSITION_SETTLE;
}


//////////////////////////////////////////////////
//
// TimeOut while in Coasting Forward
//
//////////////////////////////////////////////////

BYTE	MCM_exitC(void)
{
		if(PosChangedForward()) {

				ResetTimeOutTimer;
				UpdateMinForwardMove();

			goto posChngedFrwrdOK;
		}

		if(IsInMaxHardwareLimit())
		{
				LogHWlimitStall;
				StartTimer(POSITION_SETTLE_TIMEOUT);

			return	MCM_POSITION_SETTLE;
		}

		IncrementTimeOutTimer;

		if(timeOutTimer == coastingTimeOut) {

				// Motor has stopped while
				// in coasting Forward

				LogMotorMoveDone;

				StartTimer(POSITION_SETTLE_TIMEOUT);

			return MCM_POSITION_SETTLE;
		}

posChngedFrwrdOK:

		StartTimer(COASTING_TIMER_TICK);

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// TimeOut while in Moving Reverse
//
//////////////////////////////////////////////////

BYTE	MCM_exitD(void)
{
		if(GetCurrPosition() <= targetPosition)
		{
				// Reverse Target is Reached !

				TurnMotorOff;
				ResetTimeOutTimer;
				UpdateMinReverseMove();
				StartTimer(COASTING_TIMER_TICK);

			return MCM_RVRSE_COASTING;
		}

		if(PosChangedReverse())
		{
			ResetTimeOutTimer;

			UpdateMinReverseMove(); // New min move
		}
		else
		{
			if(IsInMinHardwareLimit()) {

					LogHWlimitStall;

				goto rvrsedMoveDone;
			}

			IncrementTimeOutTimer;

			if(timeOutTimer == movingTimeOut) {

					// Motor has stalled
					// while in Moving Reverse

					LogStall;

				goto rvrsedMoveDone;
			}
		}

		TurnOnReversePower();

		StartTimer(MOVING_TIMER_TICK);

	return SAME_STATE;


rvrsedMoveDone:

		TurnMotorOff;

		StartTimer(POSITION_SETTLE_TIMEOUT);

	return	MCM_POSITION_SETTLE;
}


//////////////////////////////////////////////////
//
// TimeOut while in Coasting Reverse
//
//////////////////////////////////////////////////

BYTE	MCM_exitE(void)
{
		if(PosChangedReverse()) {

				ResetTimeOutTimer;
				UpdateMinReverseMove();

			goto posChngedRvrsOK;
		}

		if(IsInMinHardwareLimit())
		{
				LogHWlimitStall;
				StartTimer(POSITION_SETTLE_TIMEOUT);

			return	MCM_POSITION_SETTLE;
		}

		IncrementTimeOutTimer;

		if(timeOutTimer == coastingTimeOut) {

				// Motor has stopped while
				// in coasting Reverse

				LogMotorMoveDone;

				StartTimer(POSITION_SETTLE_TIMEOUT);

			return MCM_INMOV_SETTLE; // Motor has stopped
		}

posChngedRvrsOK:

		StartTimer(COASTING_TIMER_TICK);

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// Nudge to Position message, while in IDLE
//
//////////////////////////////////////////////////

BYTE	MCM_exitF(void)
{
	motorStatus = NoStatus;

		// Is target within range ?

		if(IsTargetWithinLimits())
		{
			ResetTimeOutTimer;

			if(targetPosition > GetMinForwardMove()) {

					// Make AtoD driver sample as fast as possible

					SendMessage(GoAtoDfastActive, ATOD_DRIVER_SM_ID);
					StartTimer(ATOD_RFRSH); // Time to Refresh AtoD

				return MCM_FRWRD_NUDGING;
			}

			if(targetPosition < GetMinReverseMove()) {

					// Make AtoD driver sample as fast as possible

					SendMessage(GoAtoDfastActive, ATOD_DRIVER_SM_ID);
					StartTimer(ATOD_RFRSH); // Time to Refresh AtoD

				return MCM_RVRSE_NUDGING;
			}

			// just get ready for the next command

			LogMotorMoveDone;

			goto alreadyInNudgeTarget;
		}

		// Target Out of Range

		LogTargetRangeErr;


alreadyInNudgeTarget:

	SendMessage(MotorMoveDone,PCUMNGR_SM_ID);

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// TimeOut, while in Forward Nudging
//
//////////////////////////////////////////////////

BYTE	MCM_exitG(void)
{
		TurnMotorOff;

		StartTimer(NUDGE_SAMPLE_WAIT);

	return MCM_FRWRD_SAMPLING;
}


//////////////////////////////////////////////////
//
// TimeOut, while in Forward Nudge Sampling
//
//////////////////////////////////////////////////

BYTE	MCM_exitH(void)
{
			if(GetCurrPosition() >= targetPosition) {

				// Forward Target Reached

				LogMotorMoveDone;

				goto frwrdNudgeDone;
			}

			if(IsInMaxHardwareLimit()) {

				LogHWlimitStall;

				goto frwrdNudgeDone;
			}

			if(PosChangedForward()) {

				ResetTimeOutTimer;

				UpdateMinForwardMove();

			} else {

				IncrementTimeOutTimer;

				if(timeOutTimer == MAX_NUDGE_COUNT)
				{
					LogStall;	// Motor failed to move
								// after several nudges

					goto frwrdNudgeDone;
				}
			}

		// Forward Nudge some more

		TurnOnForwardPower();
		StartTimer(nudgeOnTime);

	return MCM_FRWRD_NUDGING;


frwrdNudgeDone:

		StartTimer(POSITION_SETTLE_TIMEOUT);

	return MCM_POSITION_SETTLE;
}


//////////////////////////////////////////////////
//
// TimeOut, while in Reverse Nudging
//
//////////////////////////////////////////////////

BYTE	MCM_exitI(void)

{
		TurnMotorOff;

		StartTimer(NUDGE_SAMPLE_WAIT);

	return MCM_RVRSE_SAMPLING;
}


//////////////////////////////////////////////////
//
// TimeOut, while in Reverse Nudge Sampling
//
//////////////////////////////////////////////////

BYTE	MCM_exitJ(void)
{
			if(GetCurrPosition() <= targetPosition) {

				// Reverse Target Reached

				LogMotorMoveDone;

				goto rvrsNudgeDone;
			}

			if(IsInMinHardwareLimit()) {

					LogHWlimitStall;

				goto rvrsNudgeDone;
			}

			if(PosChangedReverse()) {

				ResetTimeOutTimer;

				UpdateMinReverseMove();

			} else {

				IncrementTimeOutTimer;

				if(timeOutTimer == MAX_NUDGE_COUNT)
				{
					LogStall;	// Motor failed to move
								// after several nudges

					goto rvrsNudgeDone;
				}
			}

		// Reverse Nudge some more

		TurnOnReversePower();
		StartTimer(nudgeOnTime);

	return MCM_RVRSE_NUDGING;


rvrsNudgeDone:

		StartTimer(POSITION_SETTLE_TIMEOUT);

	return MCM_POSITION_SETTLE;
}


//////////////////////////////////////////////////
//
// TimeOut, while in Position Settle
//
//////////////////////////////////////////////////

BYTE	MCM_exitK(void)
{
		SendMessage(MotorMoveDone,PCUMNGR_SM_ID);
		SetAtoDsamplingRate(NORMAL_SAMPLING_RATE);

	return	MCM_IDLE;
}


//////////////////////////////////////////////////
//
// MotorNudgeUp, while in Idle
//
//////////////////////////////////////////////////

BYTE	MCM_exitL(void)
{
	if(nudgeOnTime != 0) {

		StartTimer(1);	// Expire Timer

		return	MCM_EXPIR_UP_TIMER;
	}

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// MotorNudgeDown, while in Idle
//
//////////////////////////////////////////////////

BYTE	MCM_exitM(void)
{
	if(nudgeOnTime != 0) {

		StartTimer(1);	// Expire Timer

		return	MCM_EXPIR_DN_TIMER;
	}

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// TimeOut, while in Nudge Settling
//
//////////////////////////////////////////////////

BYTE	MCM_exitN(void)
{
	TurnMotorOff;

		if(nudgeCount != 0)
		{
			nudgeCount--;

			if(LAST_DIR_FORWARD)
				SendMessage(MotorNudgeUp,THIS_SM);
			else
				SendMessage(MotorNudgeDown,THIS_SM);
		}

	return	MCM_IDLE;
}


//////////////////////////////////////////////////
//
// Motor GoIdle, while in several states
//
//////////////////////////////////////////////////

BYTE	MCM_exitO(void)
{
		TurnMotorOff;

		StartTimer(POSITION_SETTLE_TIMEOUT);

	return MCM_POSITION_SETTLE;
}


//////////////////////////////////////////////////
//
// TimeOut, while waiting for nudge up timer
// to expire
//
//////////////////////////////////////////////////

BYTE	MCM_exitP(void)
{
		TurnOnForwardPower();

		StartTimer(nudgeOnTime);

	return	MCM_NUDGE_SETTLE;
}


//////////////////////////////////////////////////
//
// TimeOut, while waiting for nudge down timer
// to expire
//
//////////////////////////////////////////////////

BYTE	MCM_exitQ(void)
{
		TurnOnReversePower();

		StartTimer(nudgeOnTime);

	return	MCM_NUDGE_SETTLE;
}


//////////////////////////////////////////////////
//
// TimeOut, in Reverse Settle
//
//////////////////////////////////////////////////

BYTE	MCM_exitR(void)
{
		if(GetCurrPosition() < originalTarget) {

			// Major Move over shot, do not nudge

			SendMessage(MotorMoveDone,PCUMNGR_SM_ID);

		} else {

			// Restore original target then
			// Nudge to that as the final position

			targetPosition = originalTarget;

			SendMessage(NudgeToTarget,THIS_SM);
		}

	return	MCM_IDLE;
}




//////////////////////////////////////////////////
//
// Private Helper Functions	Definition
//
//////////////////////////////////////////////////





//////////////////////////////////////////////////
//
// A check to see if the position limits have
// already been defined. Motor moves will be
// prevented by the manager if the limits are
// not set.
//
//////////////////////////////////////////////////

BOOL	IsPositionLimitSet(void) {

	if(   (minPosition == NO_POS_LIMITS)
		||(maxPosition == NO_POS_LIMITS))
			return FALSE;

	return TRUE;
}


//////////////////////////////////////////////////
//
// A check to determine if current target is
// valid.
//
//////////////////////////////////////////////////

BOOL	IsTargetWithinLimits(void)
{
	return ((targetPosition <= maxPosition)
			&&(targetPosition >= minPosition)) ?
				TRUE : FALSE ;
}



//////////////////////////////////////////////////
//
// Returns the valid minimum
// Forward move position
//
//////////////////////////////////////////////////

WORD	GetMinForwardMove(void) {

		UpdateMinForwardMove();

	return minValidMove;
}


//////////////////////////////////////////////////
//
// Returns the valid minimum
// Reverse move position
//
//////////////////////////////////////////////////

WORD	GetMinReverseMove(void) {

		UpdateMinReverseMove();

	return minValidMove;
}



//////////////////////////////////////////////////
//
// State Matrix Table
//
//////////////////////////////////////////////////

#asm
_MOTORCONTROLLER:
	fdb 	xMCM_IDLE_MATRIX
	fdb		xMCM_FRWRD_MOVING_MATRIX
	fdb		xMCM_FRWRD_COASTING_MATRIX
	fdb		xMCM_RVRSE_MOVING_MATRIX
	fdb		xMCM_RVRSE_COASTING_MATRIX
	fdb		xMCM_INMOV_SETTLE_MATRIX
	fdb		xMCM_POSITION_SETTLE_MATRIX
	fdb		xMCM_FRWRD_NUDGING_MATRIX
	fdb		xMCM_FRWRD_SAMPLING_MATRIX
	fdb		xMCM_RVRSE_NUDGING_MATRIX
	fdb		xMCM_RVRSE_SAMPLING_MATRIX
	fdb		xMCM_NUDGE_SETTLE_MATRIX
	fdb		xMCM_EXPIR_UP_TIMER_MATRIX
	fdb		xMCM_EXPIR_DN_TIMER_MATRIX
#endasm


//////////////////////////////////////////////////
//// Message/Exit Function Matrix Table
//
//////////////////////////////////////////////////

#asm
xMCM_IDLE_MATRIX:
	fcb		MoveToTarget
	fdb		MCM_exitA
	fcb		NudgeToTarget
	fdb		MCM_exitF
	fcb		MotorNudgeUp
	fdb		MCM_exitL
	fcb		MotorNudgeDown
	fdb		MCM_exitM
	fcb		NULL_MESSAGE
#endasm

#asm
xMCM_FRWRD_MOVING_MATRIX:
	fcb		TimeOut
	fdb		MCM_exitB
	fcb		MotorGoIdle
	fdb		MCM_exitO
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_FRWRD_COASTING_MATRIX:
	fcb		TimeOut
	fdb		MCM_exitC
	fcb		MotorGoIdle
	fdb		MCM_exitO
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_RVRSE_MOVING_MATRIX:
	fcb		TimeOut
	fdb		MCM_exitD
	fcb		MotorGoIdle
	fdb		MCM_exitO
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_RVRSE_COASTING_MATRIX:
	fcb		TimeOut
	fdb		MCM_exitE
	fcb		MotorGoIdle
	fdb		MCM_exitO
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_INMOV_SETTLE_MATRIX:
	fcb		TimeOut
	fdb		MCM_exitR
	fcb		MotorGoIdle
	fdb		MCM_exitO
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_POSITION_SETTLE_MATRIX
	fcb		TimeOut
	fdb		MCM_exitK
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_FRWRD_NUDGING_MATRIX:
	fcb		TimeOut
	fdb		MCM_exitG
	fcb		MotorGoIdle
	fdb		MCM_exitO
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_FRWRD_SAMPLING_MATRIX
	fcb		TimeOut
	fdb		MCM_exitH
	fcb		MotorGoIdle
	fdb		MCM_exitO
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_RVRSE_NUDGING_MATRIX:
	fcb		TimeOut
	fdb		MCM_exitI
	fcb		MotorGoIdle
	fdb		MCM_exitO
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_RVRSE_SAMPLING_MATRIX
	fcb		TimeOut
	fdb		MCM_exitJ
	fcb		MotorGoIdle
	fdb		MCM_exitO
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_NUDGE_SETTLE_MATRIX
	fcb		TimeOut
	fdb		MCM_exitN
	fcb		MotorGoIdle
	fdb		MCM_exitO
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_EXPIR_UP_TIMER_MATRIX
	fcb		TimeOut
	fdb		MCM_exitP
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_EXPIR_DN_TIMER_MATRIX
	fcb		TimeOut
	fdb		MCM_exitQ
	fcb     NULL_MESSAGE
#endasm


