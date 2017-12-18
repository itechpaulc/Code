
//
//	#include "updownsw.h"
//




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

BYTE	detectRate,
		detectRateCount;

BYTE	motorFudgeDelta;



//////////////////////////////////////////////////
//
// State Machine Initialization					
//
//////////////////////////////////////////////////

BYTE	Construct_UpDownSwMonitor(void) {

	motorFudgeDelta = INIT_MFUDGE_DELTA;

	return	UDM_IDLE;
}



//////////////////////////////////////////////////
//
// UpDown Switch Machine            			
// GoActive message while in IDLE				
//
//////////////////////////////////////////////////

BYTE	UDM_exitA(void) {

		StartTimer(FIRST_DETECT_RATE);

	return	UDM_1ST_DETECT;
}


//////////////////////////////////////////////////
//
// TimeOut message while in 1ST DETECT			
//
//////////////////////////////////////////////////

BYTE	UDM_exitB(void)
{
	if(IsUpSwitchPressed()) {

		StartTimer(NORMAL_DETECT_TICK);

		SendMessage(UpDownSwPressed, PCUMNGR_SM_ID);

		SetTarget(currPosition + motorFudgeDelta);

		detectRate = INIT_NORMAL_DETECT_RATE;
		detectRateCount = 0;

		return	UDM_ACTIVE_DETECT;
	}

	if(IsDownSwitchPressed()) {

		StartTimer(NORMAL_DETECT_TICK);

		SendMessage(UpDownSwPressed, PCUMNGR_SM_ID);

		SetTarget(currPosition - motorFudgeDelta);

		detectRate = INIT_NORMAL_DETECT_RATE;
		detectRateCount = 0;

		return	UDM_ACTIVE_DETECT;
	}

	StartTimer(FIRST_DETECT_RATE);

	return	SAME_STATE;
}


//////////////////////////////////////////////////
//
// TimeOut message while in ACTIVE DETECT		
//
//////////////////////////////////////////////////

BYTE	UDM_exitC(void)
{
	detectRateCount++;

	if(detectRate == detectRateCount)
	{


	}

	StartTimer(NORMAL_DETECT_TICK);

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// State Matrix Table							
//
//////////////////////////////////////////////////

#asm
_UPDOWNSWMONITOR:
	fdb 	xUDM_IDLE_MATRIX
	fdb		xUDM_1ST_DETECT_MATRIX
	fdb 	xUDM_ACTIVE_MATRIX
#endasm


//////////////////////////////////////////////////
//
// Message/Exit Function Matrix Table
//
//////////////////////////////////////////////////

#asm
xUDM_IDLE_MATRIX:
	fcb		UpDownSwitchGoActive
	fdb		UDM_exitA
	fcb		NULL_MESSAGE
#endasm

#asm
xUDM_1ST_DETECT_MATRIX:
	fcb
	fdb		UDM_exitA
	fcb		NULL_MESSAGE
#endasm

#asm
xUDM_ACTIVE_MATRIX:
	fcb
	fdb		UDM_exitA
	fcb		NULL_MESSAGE
#endasm