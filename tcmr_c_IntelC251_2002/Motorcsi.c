



/*
 *
 *
 *		$Header:   K:/Projects/Tcmr/Source/Motorcsi.c_v   1.2   Apr 08 2002 14:31:42   PaulLC  $
 *		$Log:   K:/Projects/Tcmr/Source/Motorcsi.c_v  $
 * 
 *    Rev 1.2   Apr 08 2002 14:31:42   PaulLC
 * Contains all of the changes made during beta development.
 * 
 *    Rev 1.1   Aug 27 2001 15:48:22   PaulLC
 * Changes made up to Engineering Version 00.25.A.
 * 
 *    Rev 1.0   Oct 06 2000 14:27:24   PaulLC
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

#include "motorcsi.h"

#include "motorcs.h"

#include "transp.h"



//////////////////////////////////////////////////
//
// Private Data
//
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
// Exit Procedures
//
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
// GoActive while in IDLE
//
//////////////////////////////////////////////////

int	MCSI_exitA(void)
{
		ENABLE_MOTOR_CHIPSET_INTERRUPT();
		
	return MCSI_ACTIVE;
}

//////////////////////////////////////////////////
//
// MotorChipSetEventDetected
//
//////////////////////////////////////////////////

int	MCSI_exitB(void)
{
		PEND_MCS_INTERRUPT_REQUEST();

	return MCSI_INTERRUPT_0;
}

//////////////////////////////////////////////////
//
// MotorChipSetResourceGranted
//
//////////////////////////////////////////////////

int	MCSI_exitB0(void)
{
		MCS_SEND_SET_CURR_AXIS_INTERRUPT();

	return MCSI_INTERRUPT_1;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//		while in MCSI_INTERRUPT_1
//
//////////////////////////////////////////////////

WORD interruptStat;

// Used by TCMR

#define		IS_MOTION_COMPLETE()							\
				(interruptStat & MOTION_COMPLETE)

#define		IS_IN_NEGATIVE_LIMIT()							\
				(interruptStat & NEGATIVE_LIMIT_SWITCH)

#define		IS_IN_POSITIVE_LIMIT()							\
				(interruptStat & POSITIVE_LIMIT_SWITCH)


// Encoder Stall Detect

#define		IS_MOTION_ERROR()								\
				(interruptStat & MOTION_ERROR)


// Interrupting Axis check

#define		IS_CURR_INTERRUPTING_AXIS_2()					\
				(interruptStat & CURR_AXIS_BIT_12)

#define		IS_CURR_INTERRUPTING_AXIS_1()					\
				(!IS_CURR_INTERRUPTING_AXIS_2())


// Not used

#define		IS_POSITION_WRAP_AROUND()							\
				(interruptStat & POSITION_WRAP_AROUND)

#define		IS_UPDATE_BREAK_POINT_REACHED()						\
				(interruptStat & UPDATE_BREAK_POINT_REACHED)
	
#define		IS_MOTION_COMMAND_ERROR()							\
				(interruptStat & COMMAND_ERROR)


int	MCSI_exitB1(void)
{
		interruptStat = READ_MOTOR_CHIPSET_DATA1();

		MCS_SEND_RESET_INTERRUPT(0x0000);

	return MCSI_INTERRUPT_2;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//		while in MCSI_INTERRUPT_2
//
//////////////////////////////////////////////////
	
STATE_MACHINE_ID	tempTransportHandlerId;

void	MCSI_InvalidInterrupt(void) { }

int	MCSI_exitB2(void)
{
		ENABLE_MOTOR_CHIPSET_INTERRUPT();

		SendMessage
			(MotorChipSetHandlerID, MotorChipSetReleaseResource);

		// Check interrupt origin

		if(IS_CURR_INTERRUPTING_AXIS_1())
			tempTransportHandlerId = TopTransportHandlerID;
		else
			tempTransportHandlerId = BottomTransportHandlerID;

		if(IS_IN_NEGATIVE_LIMIT())
		{
			SendMessage(tempTransportHandlerId, MotorChipSetNegativeLimitHit);
		}
		else
		if(IS_IN_POSITIVE_LIMIT())
		{
			SendMessage(tempTransportHandlerId, MotorChipSetPositiveLimitHit);
		}
		else
		if(IS_MOTION_ERROR() && 
			((IS_CURR_INTERRUPTING_AXIS_1() && IsStepperEncoderAvailable(TOP_TRANSPORT_AXIS_SEL)) ||
			 (IS_CURR_INTERRUPTING_AXIS_2() && IsStepperEncoderAvailable(BOTTOM_TRANSPORT_AXIS_SEL))))
		{
			SendMessage(tempTransportHandlerId, MotorChipSetPositionError);
		}
		else
		if(IS_MOTION_COMPLETE())
		{
			SendMessage(tempTransportHandlerId, MotorChipSetMotionComplete);
		}
		else
		{
			MCSI_InvalidInterrupt();
		}

	return MCSI_ACTIVE;
}

//////////////////////////////////////////////////
//
// STATE MACHINE MATRIX DEFINITIONS
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_MCSI_IDLE)
	EV_HANDLER(GoActive, MCSI_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_MCSI_ACTIVE)
	EV_HANDLER(MotorChipSetEventDetected, MCSI_exitB)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_MCSI_INTERRUPT_0)
	EV_HANDLER(MotorChipSetResourceGranted, MCSI_exitB0)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_MCSI_INTERRUPT_1)
	EV_HANDLER(MotorChipSetMessageSent, MCSI_exitB1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_MCSI_INTERRUPT_2)
	EV_HANDLER(MotorChipSetMessageSent, MCSI_exitB2)
STATE_TRANSITION_MATRIX_END;


// 
// VERY IMPORTANT : 
//		State Entry definition order MUST match the 
//		order of the state definition in the .H File 
//

SM_RESPONSE_ENTRY(MCSI_Entry)
	STATE(_MCSI_IDLE)			,			
	STATE(_MCSI_ACTIVE)			,
	STATE(_MCSI_INTERRUPT_0)	,
	STATE(_MCSI_INTERRUPT_1)	,
	STATE(_MCSI_INTERRUPT_2)
SM_RESPONSE_END




