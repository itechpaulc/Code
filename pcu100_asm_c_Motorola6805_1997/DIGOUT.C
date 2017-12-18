

//
//	#include "digout.h"
//


/////////////////////////////////////////////////////////////////////////////
//
//
//	$Header:   N:/pvcs52/projects/pcu100~1/digout.c_v   1.3   Apr 25 1997 09:57:40   Paul L C  $
//	$Log:   N:/pvcs52/projects/pcu100~1/digout.c_v  $
//
//   Rev 1.3   Apr 25 1997 09:57:40   Paul L C
//Made changes to accomodate CPU port remapping.
//
//   Rev 1.2   Mar 10 1997 11:05:36   Paul L C
//Added a message to GoIdle.
//
//   Rev 1.1   Mar 04 1997 15:38:18   Paul L C
//Added defines to mask unused bits in the LatchToOutput 
//function.
//
//   Rev 1.0   Feb 26 1997 10:54:32   Paul L C
//Initial Revision
//
//
/////////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////
//
// Private Members
//
//////////////////////////////////////////////////




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

BYTE	Construct_DOutputController(void) {

		digitalOutputState = 0x00;

	return	DOC_IDLE;
}


//////////////////////////////////////////////////
//
// Digitial Output GoActive message,
// while in IDLE
//
//////////////////////////////////////////////////

BYTE	DOC_exitA(void)
{
		StartTimer(DIGI_OUT_REFRESH_RATE);

	return DOC_ACTIVE;
}


//////////////////////////////////////////////////
//
// TimeOut message, while in ACITVE
//
//////////////////////////////////////////////////

BYTE	DOC_exitB(void)
{
		StartTimer(DIGI_OUT_REFRESH_RATE);

		LatchToOutputs();

	return SAME_STATE;
}



//////////////////////////////////////////////////
//
// GoIdle message, while in ACITVE
//
//////////////////////////////////////////////////

BYTE	DOC_exitC(void)
{
		digitalOutputState = 0x00;

		LatchToOutputs();

		CancelTimer();

	return DOC_IDLE;
}



//////////////////////////////////////////////////
//
// Private Helper Functions	Definition
//
//////////////////////////////////////////////////

#define		WRITE_OUT_MASK		0x07

void	LatchToOutputs(void) {

	BYTE temp;

		temp = PORT_A;			// Read curr portA states

		temp &= ~DIGI_OUT_MASK;	// Store state except the
								// digital outputs

		// Left justify the digital output data

		temp |= ((digitalOutputState & WRITE_OUT_MASK) << 5);

	// Write to port

	PORT_A = temp;
}


//////////////////////////////////////////////////
//
// State Matrix Table
//
//////////////////////////////////////////////////

#asm
_DIGITALOUTCNTRLR:
	fdb 	xDOC_IDLE_MATRIX
	fdb		xDOC_ACTIVE_MATRIX
#endasm


//////////////////////////////////////////////////
//
// Message/Exit Function Matrix Table
//
//////////////////////////////////////////////////

#asm
xDOC_IDLE_MATRIX:
	fcb		DigiOutGoActive
	fdb		DOC_exitA
	fcb		NULL_MESSAGE
#endasm

#asm
xDOC_ACTIVE_MATRIX:
	fcb		TimeOut
	fdb		DOC_exitB
	fcb		DigiOutGoIdle
	fdb		DOC_exitC
	fcb     NULL_MESSAGE
#endasm



