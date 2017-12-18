





//
//	#include "digin.h"
//



/////////////////////////////////////////////////////////////////////////////
//
//
//	$Header:   N:/pvcs52/projects/pcu100~1/digin.c_v   1.6   May 16 1997 15:05:14   Paul L C  $
//	$Log:   N:/pvcs52/projects/pcu100~1/digin.c_v  $
//
//   Rev 1.6   May 16 1997 15:05:14   Paul L C
//Changed a function name for consistency.
//
//   Rev 1.5   May 07 1997 09:17:42   Paul L C
//Made functions to be expanded "inline" to reduce RAM stack usage.
//Moved some variables into the header files for visibility
//
//   Rev 1.4   Apr 25 1997 09:57:38   Paul L C
//Made changes to accomodate CPU port remapping.
//
//   Rev 1.3   Apr 11 1997 09:56:24   Paul L C
//Added a check to make sure the UpDnSWdetected message is
//not sent if the UpDn State Machine is already active.
//
//   Rev 1.2   Mar 10 1997 11:06:36   Paul L C
//Added a statement which inhibits local control if 
//a Bit Flag is clear.
//
//   Rev 1.1   Mar 04 1997 15:37:24   Paul L C
//Added defines to mask unused bits in GetCurrInput
//and GetLatched input functions.
//
//   Rev 1.0   Feb 26 1997 10:54:30   Paul L C
//Initial Revision
//
//
/////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////
//
// Private Members
//
//////////////////////////////////////////////////

BYTE	prevInputState,

		currInput,

		currInputMask;


BYTE	debounceRegister;



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

BYTE	Construct_DigitalInMonitor(void) {

		currInputState = 0x00;
		latchedInputState = 0x00;

	return	DIM_IDLE;
}


//////////////////////////////////////////////////
//
// Digitial Input GoActive message,
// while in IDLE
//
//////////////////////////////////////////////////

BYTE	DIM_exitA(void)
{
		StartTimer(DIGITAL_IN_MONITOR_RATE);

		prevInputState = 0x00;
		currInput = DOWN_SWITCH;

	return DIM_ACTIVE;
}


//////////////////////////////////////////////////
//
// TimeOut message,	while in ACTIVE
//
//////////////////////////////////////////////////

BYTE	DIM_exitB(void)
{
		StartTimer(DIGITAL_IN_MONITOR_RATE);

		UpdateInputStates();

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// Private Helper Functions	Definition
//
//////////////////////////////////////////////////

void	UpdateInputStates(void)
{
	BYTE instInputState, temp;

		if(currInput > INPUT_3)
			currInput = DOWN_SWITCH;					// restart

		currInputMask = (0x01 << currInput);

		instInputState = (~PORT_B) >> 2;				// right justify
														// input data
		instInputState &= currInputMask;

		// If input state has changed from last scan
		// reset the debounce count...

		if(instInputState != (prevInputState & currInputMask))
		{
			debounceRegister &= ~currInputMask;
		}
		else
		{
			// input State has
			// remained the same from previous

			if(debounceRegister & currInputMask == 0)	// If debounce bit
			{                                     		// is clear, set it
				debounceRegister &= currInputMask;
			}

			else

			// If debounce register already set
			{
				temp = currInputState & ~currInputMask;

				currInputState = temp | (instInputState & currInputMask);

				// If input is SET, Do Latch State

				if(currInputState & currInputMask)
				{
					latchedInputState |= (currInputState & currInputMask);

					if(  (currInput == DOWN_SWITCH)
					   ||(currInput == UP_SWITCH))	// Monitoring Up Down SW ?
					{
						// Activate UpDnSw Monitor Machine if not yet active.
						// Also	check if it is OK to do local positioning.
						// Inform PCU manager of the SW detection

						if(IS_LOCAL_CTRL_ENABLED() && !IsLocalControlActive())
							SendMessage(UpDnSWdetected,PCUMNGR_SM_ID);
					}
				}
			}
		}

		// Update previous scan register

		temp = prevInputState & ~currInputMask;
		prevInputState = temp | (instInputState & currInputMask);

	// do Next input

	currInput++;
}



//////////////////////////////////////////////////
//
// State Matrix Table
//
//////////////////////////////////////////////////

#asm
_DIGITALINMONITOR:
	fdb 	xDIM_IDLE_MATRIX
	fdb		xDIM_ACTIVE_MATRIX
#endasm


//////////////////////////////////////////////////
//
// Message/Exit Function Matrix Table
//
//////////////////////////////////////////////////

#asm
xDIM_IDLE_MATRIX:
	fcb		DigiInGoActive
	fdb		DIM_exitA
	fcb		NULL_MESSAGE
#endasm

#asm
xDIM_ACTIVE_MATRIX:
	fcb		TimeOut
	fdb		DIM_exitB
	fcb     NULL_MESSAGE
#endasm



