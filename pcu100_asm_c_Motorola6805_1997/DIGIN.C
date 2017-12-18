





//
//	#include "digin.h"
//




//////////////////////////////////////////////////
//
// Private Members
//
//////////////////////////////////////////////////

bits	currInputState;

BYTE	prevInputState, latchedInputState;

BYTE	currInput, currInputMask;


BYTE	debounceRegister;




//////////////////////////////////////////////////
//
// Virtual Message Interface	
//
//////////////////////////////////////////////////

BYTE	GetCurrInputState(void) {

	return	currInputState;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BYTE	GetLatchedInputState(void) {

	return	latchedInputState;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	ResetLatchedInputState(void) {

	latchedInputState = 0x00;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL	IsUpSwitchPressed(void)
{
	return(currInputState.UP_SWITCH) ?
		TRUE : FALSE ;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL	IsDownSwitchPressed(void)
{
	return(currInputState.DOWN_SWITCH) ?
		TRUE : FALSE ;
}



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
		currInput = INPUT_1;

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

		if(currInput > UP_SWITCH)
			currInput = INPUT_1;

		currInputMask = (0x01 << currInput);

		instInputState = (~PORT_C) >> 3;

		instInputState &= currInputMask;


		// If input state has changed from last scan
		// reset the debounce count...

		if(instInputState != (prevInputState & currInputMask))
		{
			debounceRegister &= ~currInputMask;
		}
		else
		{
			// input State has remained the same from prev

			// If debounce bit is clear, set it

			if(debounceRegister & currInputMask == 0)
			{
				debounceRegister &= currInputMask;
			}

			else

			// If debounce register already set
			{
				temp = currInputState & ~currInputMask;

				currInputState = temp | (instInputState & currInputMask);

				// If input is SET, Do Latch State

				if(currInputState & currInputMask)
					latchedInputState |= (currInputState & currInputMask);
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



