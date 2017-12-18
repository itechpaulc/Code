



/*
 *
 *
 *		$Header:   K:/Projects/Tcmr/Source/Tparam.c_v   1.2   Apr 08 2002 14:32:14   PaulLC  $
 *		$Log:   K:/Projects/Tcmr/Source/Tparam.c_v  $
 * 
 *    Rev 1.2   Apr 08 2002 14:32:14   PaulLC
 * Contains all of the changes made during beta development.
 * 
 *    Rev 1.1   Aug 27 2001 15:48:24   PaulLC
 * Changes made up to Engineering Version 00.25.A.
 * 
 *    Rev 1.0   Oct 06 2000 14:27:30   PaulLC
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

#include "tparam.h"

#include "transp.h"

#include "motorcs.h"



//////////////////////////////////////////////////
//
// Private Data
//
//////////////////////////////////////////////////

BYTE		tpAxisSelect;


signed	long	tpTargetPosition;


DWORD			tpStartVelocity,
				tpMaxVelocity,
				tpAcceleration;

WORD			tpInterruptMask;
			
DWORD			mcsTargetPosition[MAX_TRANSPORTS];
DWORD			mcsActualPosition[MAX_TRANSPORTS];


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

int	TPH_exitA(void)
{		
	return TPH_ACTIVE;
}

//////////////////////////////////////////////////
//
// SendTransportProfileParams
//	 while in ACTIVE
//
//////////////////////////////////////////////////

int	TPH_exitB(void)
{
		if(GET_TP_AXIS() == TOP_TRANSPORT_AXIS_SEL)
		{
			MCS_SEND_SET_CURR_AXIS_1();
		}
		else
		{
			MCS_SEND_SET_CURR_AXIS_2();
		}

	return TPH_SENDING_AXIS;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//	 while in TPH_SENDING_AXIS
//
//////////////////////////////////////////////////

int	TPH_exitB1(void)
{
		MCS_SEND_SET_START_VELOCITY
			(GET_TP_START_VELOCITY());

	return TPH_SENDING_START_VELOCITY;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//	 while in TPH_SENDING_START_VELOCITY
//
//////////////////////////////////////////////////

int	TPH_exitB2(void)
{		
	WORD	interruptMask =
				GET_TP_INTERRUPT_MASK();

		if(IsStepperEncoderDisabled(GET_TP_AXIS()))
		{
			// Clear Motion Error Mask 
		
			interruptMask &= ~MOTION_ERROR;
		}

		MCS_SEND_SET_INTERRUPT_MASK(interruptMask);

	return TPH_SENDING_INTERRUPT_MASK;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//	 while in TPH_SENDING_INTERRUPT_MASK
//
//////////////////////////////////////////////////

int	TPH_exitB3(void)
{
		MCS_SEND_SET_MAX_VELOCITY
			(GET_TP_MAX_VELOCITY());

	return TPH_SENDING_MAX_VELOCITY;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//	 while in TPH_SENDING_MAX_VELOCITY
//
//////////////////////////////////////////////////

int	TPH_exitB4(void)
{
		MCS_SEND_SET_ACCELERATION
			(GET_TP_ACCELERATION());

	return TPH_SENDING_ACCELERATION;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//	 while in TPH_SENDING_ACCELERATION
//
//////////////////////////////////////////////////

int	TPH_exitB5(void)
{
		MCS_SEND_SET_POSITION
			(GET_TP_TARGET_POSITION());

	return TPH_SENDING_POSITION;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//	 while in TPH_SENDING_POSITION
//
//////////////////////////////////////////////////

int	TPH_exitB6(void)
{
		MCS_SEND_GET_POSITION();

	return TPH_SENDING_GET_POS;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//	 while in TPH_SENDING_POSITION
//
//////////////////////////////////////////////////

int	TPH_exitB7(void)
{
		// Verify Read Target Position, by MCS

		if(GET_TP_AXIS() == TOP_TRANSPORT_AXIS_SEL)
		{
			mcsTargetPosition[TOP_TRANSPORT_AXIS_SEL] =
				READ_MOTOR_CHIPSET_DATA_DWORD();
		}
		else
		{
			mcsTargetPosition[BOTTOM_TRANSPORT_AXIS_SEL] =
				READ_MOTOR_CHIPSET_DATA_DWORD();
		}

		MCS_SEND_GET_ACTUAL_AXIS_POS();

	return TPH_SENDING_GET_ACTUAL_POS;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//	 while in TPH_SENDING_POSITION
//
//////////////////////////////////////////////////

int	TPH_exitB8(void)
{
		// Verify Read Actual Position, before the move

		if(GET_TP_AXIS() == TOP_TRANSPORT_AXIS_SEL)
		{
			mcsActualPosition[TOP_TRANSPORT_AXIS_SEL] =
				READ_MOTOR_CHIPSET_DATA_DWORD();
		}
		else
		{
			mcsActualPosition[BOTTOM_TRANSPORT_AXIS_SEL] =
				READ_MOTOR_CHIPSET_DATA_DWORD();
		}

		MCS_SEND_UPDATE_PARAM();

	return TPH_SENDING_UPDATE_PARAM;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//	 while in TPH_SENDING_UPDATE_PARAM
//
//////////////////////////////////////////////////

int	TPH_exitB9(void)
{
		if(GET_TP_AXIS() == TOP_TRANSPORT_AXIS_SEL)
		{
			SendMessage
				(TopTransportHandlerID, TransportProfileParamsSent);
		}
		else
		{
			SendMessage
				(BottomTransportHandlerID, TransportProfileParamsSent);
		}

	return TPH_ACTIVE;
}


//////////////////////////////////////////////////
//
// STATE MACHINE MATRIX DEFINITIONS
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_TPH_IDLE)
	EV_HANDLER(GoActive, TPH_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TPH_ACTIVE)
	EV_HANDLER(SendTransportProfileParams, TPH_exitB)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TPH_SENDING_AXIS)
	EV_HANDLER(MotorChipSetMessageSent, TPH_exitB1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TPH_SENDING_START_VELOCITY)
	EV_HANDLER(MotorChipSetMessageSent, TPH_exitB2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TPH_SENDING_INTERRUPT_MASK)
	EV_HANDLER(MotorChipSetMessageSent, TPH_exitB3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TPH_SENDING_MAX_VELOCITY)
	EV_HANDLER(MotorChipSetMessageSent, TPH_exitB4)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TPH_SENDING_ACCELERATION)
	EV_HANDLER(MotorChipSetMessageSent, TPH_exitB5)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TPH_SENDING_POSITION)
	EV_HANDLER(MotorChipSetMessageSent, TPH_exitB6)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TPH_SENDING_GET_POS)
	EV_HANDLER(MotorChipSetMessageSent, TPH_exitB7)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TPH_SENDING_GET_ACTUAL_POS)
	EV_HANDLER(MotorChipSetMessageSent, TPH_exitB8)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TPH_SENDING_UPDATE_PARAM)
	EV_HANDLER(MotorChipSetMessageSent, TPH_exitB9)
STATE_TRANSITION_MATRIX_END;


// 
// VERY IMPORTANT : 
//		State Entry definition order MUST match the 
//		order of the state definition in the .H File 
//

SM_RESPONSE_ENTRY(TPH_Entry)
	STATE(_TPH_IDLE)					,			
	STATE(_TPH_ACTIVE)					,
	STATE(_TPH_SENDING_AXIS)			,
	STATE(_TPH_SENDING_START_VELOCITY)	,
	STATE(_TPH_SENDING_INTERRUPT_MASK)	,	
	STATE(_TPH_SENDING_MAX_VELOCITY)	,
	STATE(_TPH_SENDING_ACCELERATION)	,
	STATE(_TPH_SENDING_POSITION)		,	
	STATE(_TPH_SENDING_GET_POS)			,
	STATE(_TPH_SENDING_GET_ACTUAL_POS)	,
	STATE(_TPH_SENDING_UPDATE_PARAM)	
SM_RESPONSE_END




