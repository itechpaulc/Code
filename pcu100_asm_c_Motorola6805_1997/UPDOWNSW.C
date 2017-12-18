
//
//	#include "updownsw.h"
//



/////////////////////////////////////////////////////////////////////////////
//
//
//	$Header:   N:/pvcs52/projects/pcu100~1/updownsw.c_v   1.1   Mar 06 1997 11:09:02   Paul L C  $
//	$Log:   N:/pvcs52/projects/pcu100~1/updownsw.c_v  $
//
//   Rev 1.1   Mar 06 1997 11:09:02   Paul L C
//Starting Nudge OnTime was not being done correctly.
//This is now fixed.
//
//   Rev 1.0   Feb 26 1997 10:54:54   Paul L C
//Initial Revision
//
//
/////////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////
//
// Virtual Message Interface
//
//////////////////////////////////////////////////




//////////////////////////////////////////////////
//
// Private Members
//
//////////////////////////////////////////////////

BYTE	upDownMonitorRate,

		nudgeOnTimeAccumulator;



//////////////////////////////////////////////////
//
// State Machine Initialization
//
//////////////////////////////////////////////////

BYTE	Construct_UpDownSwMonitor(void) {

		nudgeAccelerateFactor	= INIT_NUDGE_ACCELERATE_FACTOR;

		startNudgeOnTime    	= INIT_START_NUDGE_ON_TIME;

		endNudgeOnTime 			= INIT_END_NUDGE_ON_TIME;

	return	UDM_IDLE;
}


//////////////////////////////////////////////////
//
// UpDown Switch Machine
// GoActive message while in IDLE
//
//////////////////////////////////////////////////

BYTE	UDM_exitA(void) {

	upDownMonitorRate 		= SLOWEST_DETECT_RATE;
	nudgeOnTimeAccumulator 	= startNudgeOnTime;
	prevNudgeTime 			= GetNudgeOnTime();

	StartTimer(upDownMonitorRate);

	SetNudgeOnTime(startNudgeOnTime);

		if(IsUpSwitchPressed()) {

				SendMessage(MotorNudgeUp, PCUMNGR_SM_ID);

			return UDM_UP_ACTIVE_DETECT;
		}

		SendMessage(MotorNudgeDown, PCUMNGR_SM_ID);

	return	UDM_DN_ACTIVE_DETECT;
}


//////////////////////////////////////////////////
//
// TimeOut message while in UP ACTIVE DETECT
//
//////////////////////////////////////////////////

BYTE	UDM_exitB(void)
{
		if(IsUpSwitchPressed()) {

				StartTimer(upDownMonitorRate);

				SendMessage(MotorNudgeUp, PCUMNGR_SM_ID);

			return	UDM_UP_PROGRESS_DETECT;
		}

		// Up Switch was released

		SendMessage(MotorStopNudging, PCUMNGR_SM_ID);

	return	UDM_IDLE;
}


//////////////////////////////////////////////////
//
// TimeOut message while in DN ACTIVE DETECT
//
//////////////////////////////////////////////////

BYTE	UDM_exitC(void)
{
		if(IsDownSwitchPressed()) {

				StartTimer(upDownMonitorRate);

				SendMessage(MotorNudgeDown, PCUMNGR_SM_ID);

			return	UDM_DN_PROGRESS_DETECT;
		}

		// Down Switch was released

		SendMessage(MotorStopNudging, PCUMNGR_SM_ID);

	return	UDM_IDLE;
}


//////////////////////////////////////////////////
//
// TimeOut message while in UP PROGRESSING DETECT
//
//////////////////////////////////////////////////

BYTE	UDM_exitD(void)
{
		if(IsUpSwitchPressed()) {

				UpdateNudgeRate();

				StartTimer(upDownMonitorRate);

				SendMessage(MotorNudgeUp, PCUMNGR_SM_ID);

			return	UDM_UP_PROGRESS_DETECT;
		}

		// Up Switch was released

		SetNudgeOnTime(prevNudgeTime);

		SendMessage(MotorStopNudging, PCUMNGR_SM_ID);

	return	UDM_IDLE;
}


//////////////////////////////////////////////////
//
// TimeOut message while in DN PROGRESSING DETECT
//
//////////////////////////////////////////////////

BYTE	UDM_exitE(void)
{
		if(IsDownSwitchPressed()) {

				UpdateNudgeRate();

				StartTimer(upDownMonitorRate);

				SendMessage(MotorNudgeDown, PCUMNGR_SM_ID);

			return	UDM_DN_PROGRESS_DETECT;
		}

		// Down Switch was released
		// Cancel Nudge Move

		SetNudgeOnTime(prevNudgeTime);

		SendMessage(MotorStopNudging, PCUMNGR_SM_ID);

	return	UDM_IDLE;
}



//////////////////////////////////////////////////
//
// Private Helper Functions	Definition
//
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
// This function speeds up the rate at which the
// up/down switch is monitored, the rate at
// the target position is increased
//
//////////////////////////////////////////////////

#define		MONITOR_PROGRESS_FACTOR		7

void		UpdateNudgeRate(void) {

		upDownMonitorRate -= MONITOR_PROGRESS_FACTOR;

		if(upDownMonitorRate < FASTEST_DETECT_RATE)
			upDownMonitorRate = FASTEST_DETECT_RATE;

		//////////////////////////////////////////////////
		// If the switch has been scanned by this monitor
		// as "Pressed" for 3 consecutive times, then
		// speed up the motor move by increasing the
		// nudge onTime value.
		//////////////////////////////////////////////////

		if(upDownMonitorRate < (SLOWEST_DETECT_RATE - (3 * MONITOR_PROGRESS_FACTOR)))
		{
			nudgeOnTimeAccumulator += nudgeAccelerateFactor;

			if(nudgeOnTimeAccumulator >= endNudgeOnTime)
				nudgeOnTimeAccumulator = endNudgeOnTime;

			SetNudgeOnTime(nudgeOnTimeAccumulator);
		}
}



//////////////////////////////////////////////////
//
// State Matrix Table
//
//////////////////////////////////////////////////

#asm
_UPDOWNSWMONITOR:
	fdb 	xUDM_IDLE_MATRIX
	fdb 	xUDM_UP_ACTIVE_MATRIX
	fdb 	xUDM_DN_ACTIVE_MATRIX
	fdb 	xUDM_UP_PROGRS_MATRIX
	fdb 	xUDM_DN_PROGRS_MATRIX
#endasm


//////////////////////////////////////////////////
//
// Message/Exit Function Matrix Table
//
//////////////////////////////////////////////////

#asm
xUDM_IDLE_MATRIX:
	fcb		UpDnSwGoActive
	fdb		UDM_exitA
	fcb		NULL_MESSAGE
#endasm

#asm
xUDM_UP_ACTIVE_MATRIX:
	fcb		TimeOut
	fdb		UDM_exitB
	fcb		NULL_MESSAGE
#endasm

#asm
xUDM_DN_ACTIVE_MATRIX:
	fcb		TimeOut
	fdb		UDM_exitC
	fcb		NULL_MESSAGE
#endasm

#asm
xUDM_UP_PROGRS_MATRIX:
	fcb		TimeOut
	fdb		UDM_exitD
	fcb		NULL_MESSAGE
#endasm

#asm
xUDM_DN_PROGRS_MATRIX:
	fcb		TimeOut
	fdb		UDM_exitE
	fcb		NULL_MESSAGE
#endasm

