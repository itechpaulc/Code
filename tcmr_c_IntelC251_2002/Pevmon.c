



/*
 *
 *
 *		$Header:   K:/Projects/Tcmr/Source/Pevmon.c_v   1.2   Apr 08 2002 14:31:42   PaulLC  $
 *		$Log:   K:/Projects/Tcmr/Source/Pevmon.c_v  $
 * 
 *    Rev 1.2   Apr 08 2002 14:31:42   PaulLC
 * Contains all of the changes made during beta development.
 * 
 *    Rev 1.1   Aug 27 2001 15:48:24   PaulLC
 * Changes made up to Engineering Version 00.25.A.
 * 
 *    Rev 1.0   Oct 06 2000 14:27:26   PaulLC
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

#include "pevmon.h"

#include "motorcs.h"



//////////////////////////////////////////////////
//
// Private Data
//
//////////////////////////////////////////////////

BYTE	near	currPressStatus,
				prevPressStatus,
				instantaneousPressStatus;

BYTE	near	latchedPressStatus;
BYTE	near	pressStatusSystemParameter;

BYTE	near	currTransportStatus;

BYTE	near	prevTransportStatus;

BYTE	near	instantaneousTransportStatus;


BYTE	near	currTransportLimitStatus;

BOOL	near	monitorTransportStatus[MAX_TRANSPORTS];



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	ResetLatchedPressStatus(void)
{
	SetLatchedPressStatus(0x00);
}

void	PressStatusEventDetect(void)
{
	BYTE eventDetect = GetCurrPressStatus();

			eventDetect = ~(eventDetect ^ GetPressStatusSystemParameter());

			eventDetect |= GetLatchedPressStatus();

		SetLatchedPressStatus(eventDetect);
}

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

//
// Debounce 80 msec, 
// Kernel Tick 4 msec * 20
//

#define		DEBOUNCE_CHECK_TIME		20

int	PEM_exitA(void)
{	
			currPressStatus				= 0x00;
			instantaneousPressStatus	= 0x00;
			prevPressStatus				= 0x00;

			currTransportStatus				= 0x00;
			prevTransportStatus				= 0x00;
			instantaneousTransportStatus	= 0x00;

			ResetLatchedPressStatus();

		StartTimer(DEBOUNCE_CHECK_TIME);
		
	return PEM_MONITOR_PRESS_EVENT;
}

//////////////////////////////////////////////////
//
// TimeOut
//	while in PEM_MONITOR_PRESS_EVENT
//
//////////////////////////////////////////////////

BYTE	pressStatDebounceDetect, pressStat;
BYTE	transpStatDebounceDetect, transpStat;

int	PEM_exitB(void)
{
		instantaneousPressStatus = 0x00;

		// Press Status Monitor

		if(IS_PRESS_IN_WASH())
			instantaneousPressStatus |= IN_WASH;
			
		if(IS_PRESS_IN_SPLICE())
			instantaneousPressStatus |= IN_SPLICE;
		
		if(IS_PRESS_IMPRESSION_OFF())
			instantaneousPressStatus |= IMPRESSION_OFF;
			
		if(IS_SPARE_ON())
			instantaneousPressStatus |= SPARE_ON;

			pressStatDebounceDetect = 
				~(prevPressStatus ^ instantaneousPressStatus);

			pressStat &= ~pressStatDebounceDetect;

			pressStat |= 	
				pressStatDebounceDetect & instantaneousPressStatus;

			prevPressStatus =
				instantaneousPressStatus;

			SetCurrPressStatus(pressStat);

			PressStatusEventDetect();

		PEND_PRESS_EVENT_HANDLER_REQUEST();		

	return PEM_MCS_REQ;
}

//////////////////////////////////////////////////
//
// MotorChipSetResourceGranted
//	while in PEM_MCS_REQ
//
//////////////////////////////////////////////////

int	PEM_exitC(void)
{
		MCS_SEND_GET_LIMIT_SWITCH_STATE();

	return PEM_MONITOR_PRESS_TRANSPORT;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//	while in PEM_MONITOR_PRESS_TRANSPORT
//
//////////////////////////////////////////////////

int	PEM_exitD(void)
{
		SetCurrTransportLimitStatus
			((BYTE)READ_MOTOR_CHIPSET_DATA1());

		// Transport Status Monitor
	
		instantaneousTransportStatus = 0x00;

		if(IS_TOP_TRANSPORT_READY())
			instantaneousTransportStatus |= TOP_TRANSPORT_STATUS;
			
		if(IS_BOTTOM_TRANSPORT_READY())
			instantaneousTransportStatus |= BOTTOM_TRANSPORT_STATUS;
		
			transpStatDebounceDetect = 
				~(prevTransportStatus ^ instantaneousTransportStatus);

			transpStat &= ~transpStatDebounceDetect;

			transpStat |= 	
				transpStatDebounceDetect & instantaneousTransportStatus;

			prevTransportStatus =
				instantaneousTransportStatus;

			SetCurrTransportStatus(transpStat);


		// Check for Transport handler Status request
		// CurrTransportStatus == CLEAR indicates ERROR // DEBUG
		// ADD A MACRO

		if(MONITOR_TRANSPORT_STATUS(TOP_TRANSPORT_SEL) &&		
			(GetCurrTopTransportStatus() == CLEAR))
		{
			SendLowPriorityMessage
				(TopTransportHandlerID, TransportNotReadyDetected);
		}
		
		if(MONITOR_TRANSPORT_STATUS(BOTTOM_TRANSPORT_SEL) &&
			(GetCurrBottomTransportStatus() == CLEAR))
		{
			SendLowPriorityMessage
				(BottomTransportHandlerID, TransportNotReadyDetected);
		}


		// Release MCS, restart timer

		SendLowPriorityMessage
			(MotorChipSetHandlerID, MotorChipSetReleaseResource);

		StartTimer(DEBOUNCE_CHECK_TIME);		
		
	return PEM_MONITOR_PRESS_EVENT;
}


//////////////////////////////////////////////////
//
// STATE MACHINE MATRIX DEFINITIONS
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_PEM_IDLE)
	EV_HANDLER(GoActive, PEM_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_PEM_MONITOR_PRESS_EVENT)
	EV_HANDLER(TimeOut, PEM_exitB)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_PEM_MCS_REQ)
	EV_HANDLER(MotorChipSetResourceGranted, PEM_exitC)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_PEM_MONITOR_PRESS_TRANSPORT)
	EV_HANDLER(MotorChipSetMessageSent, PEM_exitD)
STATE_TRANSITION_MATRIX_END;


// 
// VERY IMPORTANT : 
//		State Entry definition order MUST match the 
//		order of the state definition in the .H File 
//

SM_RESPONSE_ENTRY(PEM_Entry)
	STATE(_PEM_IDLE)					,	
	STATE(_PEM_MONITOR_PRESS_EVENT)		,
	STATE(_PEM_MCS_REQ)					,
	STATE(_PEM_MONITOR_PRESS_TRANSPORT)	
SM_RESPONSE_END




