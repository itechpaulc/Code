



//
//
//
//  Author :    Paul Calinawan
//
//  Date:       Jan 14 , 2012
//
//  Copyrights: Imaging Technologies Inc.
//
//  Product:    ITECH PLC2
//  
//  Subsystem:  Absolute Encoder Monitor Board
//
//  -------------------------------------------
//
//
//      CONFIDENTIAL DOCUMENT
//
//      Property of Imaging Technologies Inc.
//
//



////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////

#include "itechsys.h"


////////////////////////////////////////////////////////////////////


#include "kernel.h"


////////////////////////////////////////////////////////////////////

#include <avr/io.h>

#include "linkbuttonmanager.h"


////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////

#define		DISPLAY_ADDRESS_TIMER			(MILLISECONDS(350))

#define		INCREMENT_ADDRESS_TIMER_BEGIN	(MILLISECONDS(1500))

#define		INCREMENT_ADDRESS_TIMER_ACCEL	(MILLISECONDS(200))
#define		INCREMENT_ADDRESS_TIMER_MIN     (MILLISECONDS(300))




///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////

void LinkLEDON(void)
{
	PORTB |= LINK_LED_MASK;
}

void LinkLEDOFF(void)
{
	PORTB &= (~LINK_LED_MASK);
}


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

U16		accelTimer;


////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////

void    Init_LinkButtonManager(void)
{

}

////////////////////////////////////////////////////////////////////
//
// Exit Procedures
//
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//
// GoActive
//
////////////////////////////////////////////////////////////////////

NEW_STATE   LBM_exitA(void)
{

    return LBM_ACTIVE;
}



////////////////////////////////////////////////////////////////////
//
// LinkButtonOn
//		while in LBM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   LBM_exitB(void)
{
		StartTimer(DISPLAY_ADDRESS_TIMER);	

    return LBM_WAIT_DISPLAY_ADDRESS;
}

////////////////////////////////////////////////////////////////////
//
// LinkButtonOff
//		while in LBM_WAIT_DISPLAY_ADDRESS
//
////////////////////////////////////////////////////////////////////

NEW_STATE   LBM_exitC(void)
{
		// Link Button was not pressed long enough
		
		CancelTimer();
	
    return LBM_ACTIVE;
}


////////////////////////////////////////////////////////////////////
//
// TimeOut
//		while in LBM_WAIT_DISPLAY_ADDRESS
//
////////////////////////////////////////////////////////////////////

NEW_STATE   LBM_exitD(void)
{		
		StartTimer(INCREMENT_ADDRESS_TIMER_BEGIN);
	
		// Link button pushed at least for the Short duration
		
		SendMessage(SystemManager, LinkButtonPushShort);
	
    return LBM_WAIT_INC_ADDRESS;
}

////////////////////////////////////////////////////////////////////
//
// LinkButtonOff
//		while in LBM_WAIT_INC_ADDRESS
//
////////////////////////////////////////////////////////////////////

NEW_STATE   LBM_exitE(void)
{
		// Link Button was not pressed long enough
		// Just to display the address
	
		CancelTimer();
		
		SendMessage(SystemManager, LinkButtonPushShort);
	
    return LBM_ACTIVE;
}


////////////////////////////////////////////////////////////////////
//
// TimeOut
//
////////////////////////////////////////////////////////////////////

NEW_STATE   LBM_exitF(void)
{
		// Link Button was not pressed long enough
		// to begin updating the address value
		
		StartTimer(MILLISECONDS(accelTimer));
		
		accelTimer = INCREMENT_ADDRESS_TIMER_BEGIN;
			
		SendMessage(SystemManager, LinkButtonPushLong);
	
    return LBM_INC_ADDRESS;
}


////////////////////////////////////////////////////////////////////
//
// LinkButtonOff
//		while in LBM_INC_ADDRESS
//
////////////////////////////////////////////////////////////////////

NEW_STATE   LBM_exitG(void)
{
		CancelTimer();
		
		SendMessage(SystemManager, LinkLongButtonPushDone);
	
    return LBM_ACTIVE;
}


////////////////////////////////////////////////////////////////////
//
// TimeOut
//		while in LBM_INC_ADDRESS
//
////////////////////////////////////////////////////////////////////

NEW_STATE   LBM_exitH(void)
{
		accelTimer -= INCREMENT_ADDRESS_TIMER_ACCEL;
		
		if(accelTimer < INCREMENT_ADDRESS_TIMER_MIN)
			accelTimer = INCREMENT_ADDRESS_TIMER_MIN;
		
		StartTimer(accelTimer);
		
		SendMessage(SystemManager, LinkButtonPushLong);
	
    return SAME_STATE;
}



////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_LBM_IDLE)
EV_HANDLER(GoActive, LBM_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_LBM_ACTIVE)
EV_HANDLER(LinkButtonOn, LBM_exitB)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_LBM_WAIT_DISPLAY_ADDRESS)
EV_HANDLER(LinkButtonOff, LBM_exitC),
EV_HANDLER(TimeOut, LBM_exitD)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_LBM_WAIT_INC_ADDRESS)
EV_HANDLER(LinkButtonOff, LBM_exitE),
EV_HANDLER(TimeOut, LBM_exitF)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_LBM_INC_ADDRESS)
EV_HANDLER(LinkButtonOff, LBM_exitG),
EV_HANDLER(TimeOut, LBM_exitH)
STATE_TRANSITION_MATRIX_END;


// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(LBM_Main_Entry)
	STATE(_LBM_IDLE)						,	
	STATE(_LBM_ACTIVE)						,
	STATE(_LBM_WAIT_DISPLAY_ADDRESS)		,
	STATE(_LBM_WAIT_INC_ADDRESS)			,
	STATE(_LBM_INC_ADDRESS)
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////


