


//
//	#include "ledstat.h"
//


//////////////////////////////////////////////////
//
// Private Members								
//
//////////////////////////////////////////////////

BYTE	LEDmask;
BYTE	LEDmaskShift,CurrCadence;
BYTE	CadenceCount;

BYTE	PrevLEDcadence;


//////////////////////////////////////////////////
//
// Cadence Table								
//
//////////////////////////////////////////////////

const BYTE	LEDcadenceTbl[] = {

	// 32 State Cadence	

	0xff, 0xff, 0xff, 0xff,		// Steady		
	0xff, 0xff, 0x00, 0x00,		// Slow Pulse
	0xaa, 0xaa, 0xaa, 0xaa,		// Fast Pulse
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

	LEDmask >>= 1;

	if(++LEDmaskShift > 7) {

		LEDmaskShift = 0;
		LEDmask = 0x80;

		if(++CurrCadence > 3)
			CurrCadence = 0;
	}

	if(LEDmask & (LEDcadenceTbl[StartLEDcadence+CurrCadence]))
		SetLED; else ClearLED;
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

		CadenceCount = 0;

	return LED_TRANSMIT;
}


//////////////////////////////////////////////////
//
// TimedOut while in Active State				
//												
// ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; 

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

BYTE	LED_exitD(void) {

		StartTimer(LED_PULSE_RATE);

		LatchLEDbit();

		if(++CadenceCount > 12) {

			StartLEDcadence = PrevLEDcadence;

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


