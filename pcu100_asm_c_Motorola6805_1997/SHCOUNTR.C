


//
//	#include "shcountr.h"
//



/////////////////////////////////////////////////////////////////////////////
//
//
//	$Header:   N:/pvcs52/projects/pcu100~1/shcountr.c_v   1.4   May 02 1997 13:46:26   Paul L C  $
//	$Log:   N:/pvcs52/projects/pcu100~1/shcountr.c_v  $
//
//   Rev 1.4   May 02 1997 13:46:26   Paul L C
//Changed macros to follow convention.
//
//   Rev 1.3   Apr 25 1997 09:58:08   Paul L C
//Made changes to accomodate CPU port remapping.
//
//   Rev 1.2   Apr 11 1997 10:00:40   Paul L C
//A potential problem was discovered where the ClearSheetCount
//function is called consecutively. This is now addressed. Also added
//variable initialization.
//
//   Rev 1.1   Mar 10 1997 11:04:28   Paul L C
//Added a message to GoIdle.
//
//   Rev 1.0   Feb 26 1997 10:54:46   Paul L C
//Initial Revision
//
//
/////////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////
//
// Private Members
//
//////////////////////////////////////////////////

WORD	currSheetCount,

		lastRequestedSheetCount;


//////////////////////////////////////////////////
//
// Virtual Message Interface
//
//////////////////////////////////////////////////

WORD	GetSheetCount(void) {

	lastRequestedSheetCount = currSheetCount,

	return	lastRequestedSheetCount;
}


//////////////////////////////////////////////////
//
// After the SPU retreives the sheet count data
// it can issue a clearSheetCount command to
// reset the sheetCount. Any Sheet Counts detected
// inbetween the Request and the Reset Command
// is not lost.
//
//////////////////////////////////////////////////

void	ClearSheetCount(void) {

	currSheetCount -= lastRequestedSheetCount;

	lastRequestedSheetCount = 0x0000;
}


//////////////////////////////////////////////////
//
// These macros detect the state of the
// Sheet Counter Input Port.
//
//////////////////////////////////////////////////

#define		IsShtCntInHi()	(PORT_D.SHEET_COUNT_IN == SET)

#define		IsShtCntInLow()	(PORT_D.SHEET_COUNT_IN == CLEAR)




//////////////////////////////////////////////////
//
// State Machine Initialization
//
//////////////////////////////////////////////////

BYTE	Construct_SheetCounter(void) {

		scSamplingRate = INIT_SC_SAMPLE_RATE;

		currSheetCount =
		lastRequestedSheetCount = 0x0000;

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

		if(IsShtCntInHi())
			return	SCM_FALL_WAIT;

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

		if(IsShtCntInLow()) {

			currSheetCount++;

			// Check if sheet counter overflows

			if(currSheetCount == 0x0000)
				LogShCountOvrFlowErr;

			return	SCM_RISE_WAIT;
		}

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// GoIdle message, while in FALL or RISE WAIT
//
//////////////////////////////////////////////////

BYTE	SCM_exitD(void)
{
		CancelTimer();

	return SCM_IDLE;
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
	fcb		SCounterGoIdle
	fdb		SCM_exitD
	fcb     NULL_MESSAGE
#endasm

#asm
xSCM_FALL_MATRIX:
	fcb		TimeOut
	fdb		SCM_exitC
	fcb     SCounterGoIdle
	fdb		SCM_exitD
	fcb     NULL_MESSAGE
#endasm

