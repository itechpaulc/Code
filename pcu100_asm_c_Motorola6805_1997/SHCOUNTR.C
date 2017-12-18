


//
//	#include "shcountr.h"
//



//////////////////////////////////////////////////
//
// Virtual Message Interface					
//
//////////////////////////////////////////////////

void	SetSamplingRate(BYTE scSmplRate) {

	scSamplingRate = scSmplRate;
}


WORD	GetSheetCount(void) {

	lastRequestedSheetCount = currSheetCount,

	return	lastRequestedSheetCount;
}


void	ClearSheetCount(void) {

	currSheetCount -= lastRequestedSheetCount;
}



//////////////////////////////////////////////////
//
// Private Members								
//
//////////////////////////////////////////////////

BYTE	scSamplingRate;

WORD	currSheetCount,
		lastRequestedSheetCount;



//////////////////////////////////////////////////
//
// State Machine Initialization					
//
//////////////////////////////////////////////////

BYTE	Construct_SheetCounter(void) {

	scSamplingRate = INIT_SC_SAMPLE_RATE;

	return	SCM_IDLE;
}


//////////////////////////////////////////////////
//
// SheetCounter GoActive message, while in IDLE	
//
//////////////////////////////////////////////////

BYTE	SCM_exitA(void)
{
	StartTimer(scSamplingRate);

	return SCM_RISE_WAIT;
}



//////////////////////////////////////////////////
//
// TimeOut message, while in RISE WAIT			
//
//////////////////////////////////////////////////

BYTE	SCM_exitB(void)
{
	StartTimer(scSamplingRate);

		if(SheetCountIsHi) {

			return	SCM_FALL_WAIT;
		}

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// TimeOut message, while in FALL WAIT			
//
//////////////////////////////////////////////////

BYTE	SCM_exitC(void)
{
	StartTimer(scSamplingRate);

		if(SheetCountIsHi) {

			currSheetCount++;

			return	SCM_RISE_WAIT;
		}

	return SAME_STATE;
}



//////////////////////////////////////////////////
//
// State Matrix Table							
//
//////////////////////////////////////////////////

#asm
_SHEETCOUNTER:
	fdb 	xSCM_IDLE_MATRIX
	fdb		xSCM_RISE_MATRIX
	fdb		xSCM_FALL_MATRIX
#endasm


//////////////////////////////////////////////////
//
// Message/Exit Function Matrix Table			
//
//////////////////////////////////////////////////

#asm
xSCM_IDLE_MATRIX:
	fcb		SCounterGoActive
	fdb		SCM_exitA
	fcb		NULL_MESSAGE
#endasm

#asm
xSCM_RISE_MATRIX:
	fcb		TimeOut
	fdb		SCM_exitB
	fcb     NULL_MESSAGE
#endasm

#asm
xSCM_FALL_MATRIX:
	fcb		TimeOut
	fdb		SCM_exitC
	fcb     NULL_MESSAGE
#endasm

