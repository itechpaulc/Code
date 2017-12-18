


//
//	#include "motor.h"
//

//////////////////////////////////////////////////
//
// Private Members
//
//////////////////////////////////////////////////

WORD	targetPosition;

WORD	prevPosition;

WORD	minValidMove;


BYTE	timeOutTimer;



//////////////////////////////////////////////////
//
// Motor Status Definitions


BYTE	motorStatus;

#define		Stall			0x00
#define		MoveDone		0x04

//
//////////////////////////////////////////////////



#define		TurnMotorForwardOn		(PORT_A.FRWRD_PORT = CLEAR)
#define		TurnMotorReverseOn		(PORT_A.RVRSE_PORT = CLEAR)

#define		MotorForwardOff			(PORT_A.FRWRD_PORT = SET)
#define		MotorReverseOff			(PORT_A.RVRSE_PORT = SET)

#define		TurnMotorOff			MotorForwardOff;MotorReverseOff;



#define		ResetTimeOutTimer       (timeOutTimer = 0)
#define		IncrementTimeOutTimer   (timeOutTimer++)

#define		LogMotorStalledStatus	(motorStatus = Stall)
#define		LogMotorMoveDone        (motorStatus = MoveDone)

#define		LogStallSystemError		(SystemErrors.ERR_MOTOR_STALL = TRUE)

#define		LogStall				LogMotorStalledStatus;LogStallSystemError;






//////////////////////////////////////////////////
//
// Virtual Message Interface
//
//////////////////////////////////////////////////

void	SetTarget(WORD	trgtPos) {

	// Check against Limits

	if(trgtPos > maxPosition)
		targetPosition = maxPosition;
	else

	if(trgtPos < minPosition)
		targetPosition = minPosition;

	else

	// Target is OK

	targetPosition = trgtPos;
}






//////////////////////////////////////////////////
//
// State Machine Initialization
//
//////////////////////////////////////////////////

BYTE	Construct_MotorController(void) {

		TurnMotorOff;

		minMove = INITIAL_MIN_MOVE;

		movingTimeOut = INITIAL_MOVING_TIMEOUT;
		coastingTimeOut = INITIAL_COASTING_TIMEOUT;

		minPosition = maxPosition = NO_POS_LIMITS;

	return MCM_IDLE;
}


//////////////////////////////////////////////////
//
// MoveToTarget message, while in Idle
//
//////////////////////////////////////////////////

BYTE	MCM_exitA(void)
{
	if(IsPositionLimitSet() == FALSE)
		return SAME_STATE;

	ResetTimeOutTimer;

		UpdateMinForwardMove();

		if(targetPosition > minValidMove) {

				SetAtoDsamplingRate(FAST_SAMPLING_RATE);
				StartTimer(MOVING_TIMER_TICK);
				TurnMotorForward;
				LatchForwardBit();

			return MCM_FRWRD_MOVING;
		}

		UpdateMinReverseMove();

		if(targetPosition < minValidMove) {

				SetAtoDsamplingRate(FAST_SAMPLING_RATE);
				StartTimer(MOVING_TIMER_TICK);
				TurnMotorReverse;
				LatchReverseBit();

			return MCM_RVRSE_MOVING;
		}


		// Already in Target !

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
			ResetTimeOutTimer;
		else
		{
			IncrementTimeOutTimer;

			if(timeOutTimer == movingTimeOut)
			{
					// Motor has stalled
					// while in Moving Forward

					LogStall;
					TurnMotorOff;
					StartTimer(POSITION_SETTLE_TIMEOUT);

				return	MCM_POSITION_SETTLE;
			}
		}

		UpdateMinForwardMove();
		StartTimer(MOVING_TIMER_TICK);

	return SAME_STATE;
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
			goto posChngedFrwrdOK;
		}


		IncrementTimeOutTimer;

		if(timeOutTimer == coastingTimeOut) {

				// Motor has stopped while
				// in coasting Forward

				StartTimer(POSITION_SETTLE_TIMEOUT);

			return MCM_POSITION_SETTLE;
		}

posChngedFrwrdOK:

		UpdateMinForwardMove();
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
			ResetTimeOutTimer;
		else
		{
			IncrementTimeOutTimer;

			if(timeOutTimer == movingTimeOut)
			{
					// Motor has stalled
					// while in Moving Reverse

					LogStall;
					TurnMotorOff;
					StartTimer(POSITION_SETTLE_TIMEOUT);

				return	MCM_POSITION_SETTLE;
			}
		}

		UpdateMinReverseMove();
		StartTimer(MOVING_TIMER_TICK);

	return SAME_STATE;
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
			goto posChngedRvrsOK;
		}

		IncrementTimeOutTimer;

		if(timeOutTimer == coastingTimeOut) {

				// Motor has stopped while
				// in coasting Reverse

				StartTimer(POSITION_SETTLE_TIMEOUT);

			return MCM_POSITION_SETTLE; // Motor has stopped
		}

posChngedRvrsOK:

		UpdateMinReverseMove();
		StartTimer(COASTING_TIMER_TICK);

	return SAME_STATE;
}


// debug fudging stall check !

//////////////////////////////////////////////////
//
// Fudge to Position message, while in IDLE
//
//////////////////////////////////////////////////

BYTE	MCM_exitF(void)
{
	if(IsPositionLimitSet() == FALSE)
		return SAME_STATE;

	ResetTimeOutTimer;

		UpdateMinForwardMove();

		if(targetPosition > minValidMove) {

				SetAtoDsamplingRate(FAST_SAMPLING_RATE);
				StartTimer(FUDGE_MOTOR_ON_TIME);
				TurnMotorForward;
				LatchForwardBit();

			return MCM_FRWRD_FUDGING;
		}

		UpdateMinReverseMove();

		if(targetPosition < minValidMove) {

				SetAtoDsamplingRate(FAST_SAMPLING_RATE);
				StartTimer(FUDGE_MOTOR_ON_TIME);
				TurnMotorReverse;
				LatchReverseBit();

			return MCM_RVRSE_FUDGING;
		}

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// TimeOut, while in Forward Fudging
//
//////////////////////////////////////////////////

BYTE	MCM_exitG(void)
{
		StartTimer(FUDGE_SAMPLE_WAIT);

		TurnMotorOff;

	return MCM_FRWRD_SAMPLING;
}


//////////////////////////////////////////////////
//
// TimeOut, while in Forward Sampling
//
//////////////////////////////////////////////////

BYTE	MCM_exitH(void)
{
		if(targetPosition >= GetCurrPosition()) {

			SetAtoDsamplingRate(NORMAL_SAMPLING_RATE);

			return MCM_IDLE;
		}

		StartTimer(FUDGE_MOTOR_ON_TIME);

		TurnMotorForward;

	return MCM_FRWRD_FUDGING;
}


//////////////////////////////////////////////////
//
// TimeOut, while in Reverse Fudging
//
//////////////////////////////////////////////////

BYTE	MCM_exitI(void)
{
		StartTimer(FUDGE_SAMPLE_WAIT);

		TurnMotorOff;

	return MCM_RVRSE_SAMPLING;
}


//////////////////////////////////////////////////
//
// TimeOut, while in Reverse Sampling
//
//////////////////////////////////////////////////

BYTE	MCM_exitJ(void)
{
		if(targetPosition <= GetCurrPosition()) {

			SetAtoDsamplingRate(NORMAL_SAMPLING_RATE);

			return MCM_IDLE;
		}

		StartTimer(FUDGE_MOTOR_ON_TIME);

		TurnMotorReverse;

	return MCM_RVRSE_FUDGING;
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

		LogMotorMoveDone;

	return	MCM_IDLE;
}

//////////////////////////////////////////////////
//
// Private Helper Functions	Definition
//
//////////////////////////////////////////////////


void	LatchForwardBit(void) {

	SystemStatus.MOTOR_FORWARD = TRUE;
	SystemStatus.MOTOR_REVERSE = FALSE;
}


void	LatchReverseBit(void) {

	SystemStatus.MOTOR_FORWARD = FALSE;
	SystemStatus.MOTOR_REVERSE = TRUE;
}


BYTE	GetMotorStatus(void) {

	return motorStatus;
}

//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

void	UpdateMinForwardMove(void) {

	minValidMove = GetCurrPosition() + minMove;

	if(minValidMove > maxPosition)
		minValidMove = maxPosition;
}

//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

void	UpdateMinReverseMove(void) {

	minValidMove = GetCurrPosition() - minMove;

	if( (minValidMove > MAX_ATOD_POS) // OverFlow ?
	   ||(minValidMove < minPosition))
		minValidMove = minPosition;
}


BOOL	IsPositionLimitSet(void) {

	if(   (minPosition == NO_POS_LIMITS)
		||(maxPosition == NO_POS_LIMITS))
			return FALSE;

	return TRUE;
}



BOOL	PosChangedForward(void) {

	return (GetCurrPosition() > minValidMove) ?
		TRUE : FALSE;
}


BOOL	PosChangedReverse(void) {

	return (GetCurrPosition() < minValidMove) ?
		TRUE : FALSE;
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
	fdb		xMCM_RVRSE_COSTING_MATRIX
	fdb		xMCM_POSITION_SETTLE_MATRIX
	fdb		xMCM_FRWRD_FUDGING_MATRIX
	fdb		xMCM_FRWRD_SAMPLING_MATRIX
	fdb		xMCM_RVRSE_FUDGING_MATRIX
	fdb		xMCM_RVRSE_SAMPLING_MATRIX
#endasm


//////////////////////////////////////////////////
//
// Message/Exit Function Matrix Table	
//
//////////////////////////////////////////////////

#asm
xMCM_IDLE_MATRIX:
	fcb		MoveToTarget
	fdb		MCM_exitA
	fcb		FudgeToTarget
	fdb		MCM_exitF
	fcb		NULL_MESSAGE
#endasm

#asm
xMCM_FRWRD_MOVING_MATRIX:
	fcb		TimeOut
	fdb		MCM_exitB
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_FRWRD_COASTING_MATRIX:
	fcb		TimeOut
	fdb		MCM_exitC
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_RVRSE_MOVING_MATRIX:
	fcb		TimeOut
	fdb		MCM_exitD
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_RVRSE_COASTING_MATRIX:
	fcb		TimeOut
	fdb		MCM_exitE
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_POSITION_SETTLE_MATRIX
	fcb		TimeOut
	fdb		MCM_exitK
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_FRWRD_FUDGING_MATRIX:
	fcb		TimeOut
	fdb		MCM_exitG
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_FRWRD_SAMPLING_MATRIX
	fcb		TimeOut
	fdb		MCM_exitH
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_RVRSE_FUDGING_MATRIX:
	fcb		TimeOut
	fdb		MCM_exitI
	fcb     NULL_MESSAGE
#endasm

#asm
xMCM_RVRSE_SAMPLING_MATRIX
	fcb		TimeOut
	fdb		MCM_exitJ
	fcb     NULL_MESSAGE
#endasm



