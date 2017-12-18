



/*
 *
 *
 *		$Header:   K:/Projects/Tcmr/Source/TcmrMngr.c_v   1.2   Apr 08 2002 14:32:12   PaulLC  $
 *		$Log:   K:/Projects/Tcmr/Source/TcmrMngr.c_v  $
 * 
 *    Rev 1.2   Apr 08 2002 14:32:12   PaulLC
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


#include "tcmr.h"

#include "kernel.h"

#include "tcmrmngr.h"

#include "motorcs.h"

#include "transp.h"

#include "hbeat.h"

#include "pccomm.h"


//////////////////////////////////////////////////
//
// Private Data
//
//////////////////////////////////////////////////

BYTE	tempAxisSelect;

WORD	mcsStepRatio;


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
// General System
//	Half Second delay 
//

#define		SYSTEM_DELAY_WAIT		0x7F

int	TCMR_exitA1(void)
{
		tempAxisSelect = TOP_TRANSPORT_AXIS_SEL;

		StartTimer(SYSTEM_DELAY_WAIT);

	return TCMR_WAIT_POWERUP;
}


//////////////////////////////////////////////////
//
// TimeOut
//	while in TCMR_WAIT_POWERUP
//
//////////////////////////////////////////////////

int	TCMR_exitA2(void)
{
		SendLowPriorityMessage(HeartBeatHandlerID, GoActive);
	 					
		SendLowPriorityMessage(PcCommunicationHandlerID, GoSystemParameterConfig);
		
		SendLowPriorityMessage(TransportParamHandlerID, GoActive);

	return TCMR_WAIT_SYS_PARAM_CONFIG;
}

//////////////////////////////////////////////////
//
// SystemParameterConfigDone
//
//////////////////////////////////////////////////

int	TCMR_exitA3(void)
{	
		SendMessage(MotorChipSetHandlerID, GoReset);

	return TCMR_WAIT_MOTOR_CS_RESET0;
}

//////////////////////////////////////////////////
//
//	MotorChipSetHardResetDone
//
//////////////////////////////////////////////////

int	TCMR_exitB0(void)
{	
		// SOFT RESET :               
		//
		// Defaults the following (* means init changes param)
		// 
		// All Actual Axis Positions		-	0
		// All Capture Registers			-	0
		// All Event Conditions				-	CLEARED
		// Host Interrupt Signal			-	NOT ACTIVE
		// All Interrupt Masks				-	0
		// All Profile Modes				-	TRAPEZOIDAL
		// All Profile Parameter Values		-	0
		// All BreakPoint Comparison Values	-	0
		// All Auto Update					-*	ENABLED
		// Capture Input Mode				-	INDEX
		// Limit Switch Sensing				-	ENABLED
		// All Pulse Generator Modes		-	STANDARD
		// All Pulse Rates					-	0? 
		// 
		//
		//


		// Note: MCS has no owner/requestor at this point
		//			so no request is necessary

		MCS_SEND_RESET_CHIPSET();
		
	return TCMR_WAIT_MOTOR_CS_RESET1;
}

//////////////////////////////////////////////////
//
// MotorChipSetSoftResetDone
//		while in TCMR_WAIT_MOTOR_CS_RESET1
//
//////////////////////////////////////////////////

int	TCMR_exitB1(void)
{
		StartTimer(MOTOR_CHIPSET_DELAY_WAIT);

	return TCMR_WAIT_MOTOR_CS_RESET2;
}

//////////////////////////////////////////////////
//
// TimeOut
//		while in TCMR_WAIT_MOTOR_CS_RESET2
//
//////////////////////////////////////////////////

int	TCMR_exitB2(void)
{	
		// Activate the MCS Interrupt Handler

		SendMessage(MotorChipSetInterruptHandlerID, GoActive);

		MCS_SEND_GET_MCS_VERSION();

	return TCMR_WAIT_MOTOR_CS_RESET3;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//		while in TCMR_WAIT_MOTOR_CS_RESET3
//
//////////////////////////////////////////////////

// All limits are ACTIVE LOW

#define		TRANSPORT_LIMITS_SENSE		0x000F

int	TCMR_exitB3(void)
{	
		SetMotorChipSetVersion(READ_MOTOR_CHIPSET_DATA1());
		
		// GLOBAL

		MCS_SEND_SET_LIMIT_SWITCH_SENSE(TRANSPORT_LIMITS_SENSE);
		
	return TCMR_WAIT_MOTOR_CS_RESET4;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//		while in TCMR_WAIT_MOTOR_CS_RESET4
//
//////////////////////////////////////////////////

int	TCMR_exitB4(void)
{	
		// SELECT AXIS 1

		MCS_SEND_SET_CURR_AXIS_1();		

	return TCMR_WAIT_MOTOR_CS_RESET5;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//		while in TCMR_WAIT_MOTOR_CS_RESET5
//
//////////////////////////////////////////////////

int	TCMR_exitB5(void)
{	
		MCS_SEND_CLEAR_STATUS();

	return TCMR_WAIT_MOTOR_CS_RESET6;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//		while in TCMR_WAIT_MOTOR_CS_RESET6
//
//////////////////////////////////////////////////

int	TCMR_exitB6(void)
{
		MCS_SEND_SET_AUTO_UPDATE_OFF();

	return TCMR_WAIT_MOTOR_CS_RESET7;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//		while in TCMR_WAIT_MOTOR_CS_RESET7
//
//////////////////////////////////////////////////

int	TCMR_exitB7(void)
{		
		MCS_SEND_SET_STEP_RATIO
			((WORD) ENCODER_ROTATION_STEP_COUNT / 
					MOTOR_ROTATION_STEP_COUNT * 256);	

	return TCMR_WAIT_MOTOR_CS_RESET7A;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//		while in TCMR_WAIT_MOTOR_CS_RESET7A
//
//////////////////////////////////////////////////

int	TCMR_exitB7A(void)
{		
		MCS_SEND_GET_STEP_RATIO();

	return TCMR_WAIT_MOTOR_CS_RESET8;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//		while in TCMR_WAIT_MOTOR_CS_RESET8
//
//////////////////////////////////////////////////

int	TCMR_exitB8(void)
{			
			// Validate step ratio parameter 

			mcsStepRatio = READ_MOTOR_CHIPSET_DATA1();

		MCS_SEND_SET_MAX_POS_ERROR(MAX_POSITION_ERROR);		

	return TCMR_WAIT_MOTOR_CS_RESET9;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//		while in TCMR_WAIT_MOTOR_CS_RESET9
//
//////////////////////////////////////////////////

int	TCMR_exitB9(void)
{	
		if(IsStepperEncoderAvailable(tempAxisSelect))
		{
			MCS_SEND_SET_AUTO_STOP_ON();			
		}
		else
		{
			MCS_SEND_SET_AUTO_STOP_OFF()
		}

	return TCMR_WAIT_MOTOR_CS_RESET10;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent
//		while in TCMR_WAIT_MOTOR_CS_RESET10
//
//////////////////////////////////////////////////

int	TCMR_exitB10(void)
{	
		if(tempAxisSelect == TOP_TRANSPORT_AXIS_SEL)
		{
			// Select the next axis
		
			tempAxisSelect = BOTTOM_TRANSPORT_AXIS_SEL;

			MCS_SEND_SET_CURR_AXIS_2();

			return TCMR_WAIT_MOTOR_CS_RESET5;
		}
		
		SendMessage(TopSubsystemHandlerID, GoReset);

	return TCMR_WAIT_TOP_SS_RESET;
}

//////////////////////////////////////////////////
//
// TopSubsystemResetDone
//		while in TCMR_WAIT_TOP_SS_RESET
//
//////////////////////////////////////////////////

int	TCMR_exitC1(void)
{	
		SendMessage(BottomSubsystemHandlerID, GoReset);

	return TCMR_WAIT_BOTTOM_SS_RESET;
}

//////////////////////////////////////////////////
//
// BottomSubsystemResetDone
//		while in TCMR_WAIT_BOTTOM_SS_RESET
//
///////////////////////////////////////////// /////

int	TCMR_exitC2(void)
{		 
		// Activate the rest of the SM
		
		SendMessage(PressEventMonitorID, GoActive);

		SendMessage(WebEncoderHandlerID, GoActive);				


		// Now Ready for CCU Commands

		SendMessage(PcCommunicationHandlerID, GoActiveDone);


		// Normal Heartbeat

		SetCadence(NORMAL_PULSE);

	return TCMR_ACTIVE;
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

int	TCMR_exitD(void)
{
		if(GetCurrMsgData() == TOP_SYSTEM_SELECT)
		{
			SendMessage
				(TopSubsystemHandlerID, SyncCameraTrigger);
		}
		else
		{
			SendMessage
				(BottomSubsystemHandlerID, SyncCameraTrigger);		
		}

	return SAME_STATE;
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

int	TCMR_exitE(void)
{
		if(GetCurrMsgData() == TOP_SYSTEM_SELECT)
		{
			SendMessage
				(TopSubsystemHandlerID, CameraCaptureFast);
		}
		else
		{
			SendMessage
				(BottomSubsystemHandlerID, CameraCaptureFast);		
		}

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// TransportFindHome
//
//////////////////////////////////////////////////

int	TCMR_exitF(void)
{
		if(GetCurrMsgData() == TOP_SYSTEM_SELECT)
		{
			SendMessage
				(TopSubsystemHandlerID, TransportFindHome);
		}
		else
		{
			SendMessage
				(BottomSubsystemHandlerID, TransportFindHome);		
		}

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// TransportGoHome
//
//////////////////////////////////////////////////

int	TCMR_exitG(void)
{
		if(GetCurrMsgData() == TOP_SYSTEM_SELECT)
		{
			SendMessage
				(TopSubsystemHandlerID, TransportGoHome);
		}
		else
		{
			SendMessage
				(BottomSubsystemHandlerID, TransportGoHome);		
		}

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// TransportGotoPosition
//
//////////////////////////////////////////////////

int	TCMR_exitH(void)
{
		if(GetCurrMsgData() == TOP_SYSTEM_SELECT)
		{
			SendMessage
				(TopSubsystemHandlerID, TransportGotoPosition);
		}
		else
		{
			SendMessage
				(BottomSubsystemHandlerID, TransportGotoPosition);		
		}

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// TransportJog
//
//////////////////////////////////////////////////

int	TCMR_exitI(void)
{
		if(GetCurrMsgData() == TOP_SYSTEM_SELECT)
		{
			SendMessage
				(TopSubsystemHandlerID, TransportJog);
		}
		else
		{
			SendMessage
				(BottomSubsystemHandlerID, TransportJog);		
		}

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// CameraReset
//
//////////////////////////////////////////////////

int	TCMR_exitCR(void)
{
	BYTE currSystemReset = GetCurrMsgData();

		if(currSystemReset == TOP_SYSTEM_SELECT)
		{
			SendMessage
				(TopSubsystemHandlerID, CameraReset);
		}
		else
		{
			SendMessage
				(BottomSubsystemHandlerID, CameraReset);		
		}

	return TCMR_ACTIVE;
}


//////////////////////////////////////////////////
//
// TransportReset
//
//////////////////////////////////////////////////

int	TCMR_exitTR(void)
{
	BYTE currSystemReset = GetCurrMsgData();

		if(currSystemReset == TOP_SYSTEM_SELECT)
		{
			SendMessage
				(TopSubsystemHandlerID, TransportReset);
		}
		else
		{
			SendMessage
				(BottomSubsystemHandlerID, TransportReset);		
		}

	return TCMR_ACTIVE;
}


//////////////////////////////////////////////////
//
// STATE MACHINE MATRIX DEFINITIONS
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_TCMR_IDLE)
	EV_HANDLER(GoActive, TCMR_exitA1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TCMR_ACTIVE)
	EV_HANDLER(SyncCameraTrigger, TCMR_exitD),
	EV_HANDLER(CameraCaptureFast, TCMR_exitE),
	EV_HANDLER(TransportFindHome, TCMR_exitF),
	EV_HANDLER(TransportGoHome, TCMR_exitG),
	EV_HANDLER(TransportGotoPosition, TCMR_exitH),
	EV_HANDLER(TransportJog, TCMR_exitI),
	EV_HANDLER(CameraReset, TCMR_exitCR),
	EV_HANDLER(TransportReset, TCMR_exitTR)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TCMR_WAIT_POWERUP)
	EV_HANDLER(TimeOut, TCMR_exitA2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TCMR_WAIT_SYS_PARAM_CONFIG)
	EV_HANDLER(SystemParameterConfigDone, TCMR_exitA3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TCMR_WAIT_MOTOR_CS_RESET0)
	EV_HANDLER(MotorChipSetHardResetDone, TCMR_exitB0)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TCMR_WAIT_MOTOR_CS_RESET1)
	EV_HANDLER(MotorChipSetSoftResetDone, TCMR_exitB1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TCMR_WAIT_MOTOR_CS_RESET2)
	EV_HANDLER(TimeOut, TCMR_exitB2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TCMR_WAIT_MOTOR_CS_RESET3)
	EV_HANDLER(MotorChipSetMessageSent, TCMR_exitB3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TCMR_WAIT_MOTOR_CS_RESET4)
	EV_HANDLER(MotorChipSetMessageSent, TCMR_exitB4)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TCMR_WAIT_MOTOR_CS_RESET5)
	EV_HANDLER(MotorChipSetMessageSent, TCMR_exitB5)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TCMR_WAIT_MOTOR_CS_RESET6)
	EV_HANDLER(MotorChipSetMessageSent, TCMR_exitB6)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TCMR_WAIT_MOTOR_CS_RESET7)
	EV_HANDLER(MotorChipSetMessageSent, TCMR_exitB7)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TCMR_WAIT_MOTOR_CS_RESET7A)
	EV_HANDLER(MotorChipSetMessageSent, TCMR_exitB7A)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TCMR_WAIT_MOTOR_CS_RESET8)
	EV_HANDLER(MotorChipSetMessageSent, TCMR_exitB8)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TCMR_WAIT_MOTOR_CS_RESET9)
	EV_HANDLER(MotorChipSetMessageSent, TCMR_exitB9)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TCMR_WAIT_MOTOR_CS_RESET10)
	EV_HANDLER(MotorChipSetMessageSent, TCMR_exitB10) 
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TCMR_WAIT_TOP_SS_RESET)
	EV_HANDLER(TopSubsystemResetDone, TCMR_exitC1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TCMR_WAIT_BOTTOM_SS_RESET)
	EV_HANDLER(BottomSubsystemResetDone, TCMR_exitC2)
STATE_TRANSITION_MATRIX_END;


// 
// VERY IMPORTANT : 
//		State Entry definition order MUST match the 
//		order of the state definition in the .H File 
//

SM_RESPONSE_ENTRY(TCMR_Entry)
	STATE(_TCMR_IDLE)					,
	STATE(_TCMR_ACTIVE)					,
	
	STATE(_TCMR_WAIT_POWERUP)			,
	STATE(_TCMR_WAIT_SYS_PARAM_CONFIG)	,

	STATE(_TCMR_WAIT_MOTOR_CS_RESET0)	,
	STATE(_TCMR_WAIT_MOTOR_CS_RESET1)	,
	STATE(_TCMR_WAIT_MOTOR_CS_RESET2)	,
	STATE(_TCMR_WAIT_MOTOR_CS_RESET3)	,
	STATE(_TCMR_WAIT_MOTOR_CS_RESET4)	,
	STATE(_TCMR_WAIT_MOTOR_CS_RESET5)	,
	STATE(_TCMR_WAIT_MOTOR_CS_RESET6)	,
	STATE(_TCMR_WAIT_MOTOR_CS_RESET7)	,
	STATE(_TCMR_WAIT_MOTOR_CS_RESET7A)	,
	STATE(_TCMR_WAIT_MOTOR_CS_RESET8)	,
	STATE(_TCMR_WAIT_MOTOR_CS_RESET9)	,
	STATE(_TCMR_WAIT_MOTOR_CS_RESET10)	,

	STATE(_TCMR_WAIT_TOP_SS_RESET)		,
	STATE(_TCMR_WAIT_BOTTOM_SS_RESET)	
SM_RESPONSE_END




