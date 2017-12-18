



/*
 *
 *
 *		$Header:   K:/Projects/Tcmr/Source/Hbeat.c_v   1.0   Oct 06 2000 14:27:20   PaulLC  $
 *		$Log:   K:/Projects/Tcmr/Source/Hbeat.c_v  $
 * 
 *    Rev 1.0   Oct 06 2000 14:27:20   PaulLC
 * Checked in from initial workfile by PVCS Version Manager Project Assistant.
 * 
 *
 *		Author : Paul Calinawan
 *
 *			May 2000
 *
 *			Graphics Microsystems Inc
 *			1284 Forgewood Ave
 *			Sunnyvale, CA 94089
 *
 *			(408) 745-7745
 *
 *
 *		Print Quick Camera Control Module
 *	-------------------------------------------
 *
 *
*/

/////////////////////////////////////////////////////////////////////////////
//
//    NOTE:
//
//    This document contains CONFIDENTIAL and proprietary information
//    which is the property of Graphics Microsystems, Inc. It may not
//    be copied or transmitted in whole or in part by any means to any
//    media without Graphics Microsystems Inc's prior written permission.
//
/////////////////////////////////////////////////////////////////////////////



#include "kernel.h"

#include "hbeat.h"


//////////////////////////////////////////////////
//
// Private Data
//
//////////////////////////////////////////////////

BYTE	near	LEDmask,
				LEDmaskShift,
				startLEDcadence,
				CurrCadence; 			// Index to one of 
										// the 4 candence bytes


//////////////////////////////////////////////////
//
// Cadence Table
//
//////////////////////////////////////////////////

BYTE	code	LEDcadenceTbl[] = {

	// 128 State Cadence

	0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff,		// Slow Pulse
	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,

	0xff, 0xff, 0x00, 0x00,	0xff, 0xff,	0x00, 0x00,		// Normal Pulse
	0xff, 0xff, 0x00, 0x00,	0xff, 0xff,	0x00, 0x00,

	0xaa, 0xaa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		// Fast Pulse
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		// Strobed
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


//////////////////////////////////////////////////
//
// Exit Procedures
//
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
// GoActive
//
//////////////////////////////////////////////////

int	HBH_exitA(void)
{
			SetCadence(SLOW_PULSE);
		
			CurrCadence = 0;

			LEDmaskShift = 0;
			LEDmask = 0x80;

			HEART_BEAT_OFF();

		StartTimer(LED_UPDATE_RATE);
		
	return HBH_ACTIVE;
}

//////////////////////////////////////////////////
//
// TimeOut
//
//////////////////////////////////////////////////

int	HBH_exitB(void)
{
	if(LEDmask & (LEDcadenceTbl[startLEDcadence+CurrCadence]))
			HEART_BEAT_ON();
		else 
			HEART_BEAT_OFF();

		// Adjust Masks

		if(++LEDmaskShift > 7) 
		{
			LEDmaskShift = 0;
			LEDmask = 0x80;

			if(++CurrCadence > 15)	// End of table ?
				CurrCadence = 0;
		}
		else 
		{
			LEDmask >>= 1;
		}

		StartTimer(LED_UPDATE_RATE);		
		
	return SAME_STATE;
}



//////////////////////////////////////////////////
//
// STATE MACHINE MATRIX DEFINITIONS
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_HBH_IDLE)
	EV_HANDLER(GoActive, HBH_exitA)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_HBH_ACTIVE)
	EV_HANDLER(TimeOut, HBH_exitB)
STATE_TRANSITION_MATRIX_END;



// 
// VERY IMPORTANT : 
//		State Entry definition order MUST match the 
//		order of the state definition in the .H File 
//

SM_RESPONSE_ENTRY(HBH_Entry)
	STATE(_HBH_IDLE)			,
	STATE(_HBH_ACTIVE)			
SM_RESPONSE_END




