


//
//	#include "ledstat.h"
//


/////////////////////////////////////////////////////////////////////////////
//
//
//	$Header:   N:/pvcs52/projects/pcu100~1/ledstat.c_v   1.2   Apr 11 1997 09:53:12   Paul L C  $
//	$Log:   N:/pvcs52/projects/pcu100~1/ledstat.c_v  $
//
//   Rev 1.2   Apr 11 1997 09:53:12   Paul L C
//Discovered problems where the Full Cadence was not 
//being displayed. This is now corrected.
//
//   Rev 1.1   Mar 25 1997 09:44:48   Paul L C
//Changed cadence for transmit for more visibility.
//
//   Rev 1.0   Feb 26 1997 10:54:38   Paul L C
//Initial Revision
//
//
/////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////
//
// Private Members
//
//////////////////////////////////////////////////

BYTE	LEDmask,
		LEDmaskShift,
		CurrCadence, 			// Index to one of the 4 candence bytes
		TransmitCadenceCount,	// Index to one of the 8 cadence bits
		PrevLEDcadence;


//////////////////////////////////////////////////
//
// Cadence Table
//
//////////////////////////////////////////////////

const BYTE	LEDcadenceTbl[] = {

	// 32 State Cadence

	0xff, 0xff, 0xff, 0xff,		// Steady
	0xff, 0xff, 0x00, 0x00,		// Slow Pulse
	0xaa, 0xaa, 0x00, 0x00,		// Fast Pulse
	0x80, 0x00, 0x00, 0x00		// Strobed
};


//////////////////////////////////////////////////
//
// Virtual Message Interface
//
//////////////////////////////////////////////////



//////////////////////////////////////////////////
//
// Private Helper Function
//
//////////////////////////////////////////////////

void	LatchLEDbit(void) {

	if(LEDmask & (LEDcadenceTbl[StartLEDcadence+CurrCadence]))

			SetLED;	else ClearLED;


		// Adjust Masks

		if(++LEDmaskShift > 7) {

			LEDmaskShift = 0;
			LEDmask = 0x80;

			if(++CurrCadence > 3)
				CurrCadence = 0;

		} else {

			LEDmask >>= 1;
		}
}


//////////////////////////////////////////////////
//
// State Machine Initialization
//
//////////////////////////////////////////////////

BYTE	Construct_LEDstatus(void) {

		StartLEDcadence = STEADY;

	return	LED_IDLE;
}


//////////////////////////////////////////////////
//
// GoActive Message, While in Idle
//
//////////////////////////////////////////////////

BYTE	LED_exitA(void) {

		StartTimer(LED_PULSE_RATE);

		StartLEDcadence = SLOWPULSE;

	return LED_ACTIVE;
}


//////////////////////////////////////////////////
//
// Start Transmit States
//
//////////////////////////////////////////////////

BYTE	LED_exitB(void) {

		PrevLEDcadence = StartLEDcadence;

		StartLEDcadence = FASTPULSE;

		CurrCadence = TransmitCadenceCount = 0;
		LEDmaskShift = 0;
		LEDmask = 0x80;

	return LED_TRANSMIT;
}


//////////////////////////////////////////////////
//
// TimedOut while in Active State
//
//////////////////////////////////////////////////

BYTE	LED_exitC(void) {

		StartTimer(LED_PULSE_RATE);

		LatchLEDbit();

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// TimedOut while in Transmit State
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
// Number of pulses :
//

#define	LED_XMIT_DURATION	32

//
//////////////////////////////////////////////////

BYTE	LED_exitD(void) {

		StartTimer(LED_PULSE_RATE);

		LatchLEDbit();

		if(++TransmitCadenceCount >= LED_XMIT_DURATION) {

				StartLEDcadence = PrevLEDcadence;

				CurrCadence = TransmitCadenceCount = 0;
				LEDmaskShift = 0;
				LEDmask = 0x80;

			return LED_ACTIVE;
		}

	return LED_TRANSMIT;
}


//////////////////////////////////////////////////
//
// State Matrix Table
//
//////////////////////////////////////////////////

#asm
_LEDSTATUS:
	fdb 	xLED_IDLE_MATRIX
	fdb		xLED_ACTIVE_MATRIX
	fdb		xLED_XMIT_MATRIX
#endasm


//////////////////////////////////////////////////
//
// Message/Exit Function Matrix Table
//
//////////////////////////////////////////////////

#asm
xLED_IDLE_MATRIX:
	fcb		LedGoActive
	fdb		LED_exitA
	fcb		NULL_MESSAGE
#endasm

#asm
xLED_ACTIVE_MATRIX:
	fcb		LedGoTransmit
	fdb		LED_exitB
	fcb		TimeOut
	fdb		LED_exitC
	fcb     NULL_MESSAGE
#endasm

#asm
xLED_XMIT_MATRIX:
	fcb		TimeOut
	fdb		LED_exitD
	fcb     NULL_MESSAGE
#endasm


