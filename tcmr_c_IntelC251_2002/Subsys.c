



/*
 *
 *
 *		$Header:   K:/Projects/Tcmr/Source/Subsys.c_v   1.1   Aug 27 2001 15:48:24   PaulLC  $
 *		$Log:   K:/Projects/Tcmr/Source/Subsys.c_v  $
 * 
 *    Rev 1.1   Aug 27 2001 15:48:24   PaulLC
 * Changes made up to Engineering Version 00.25.A.
 * 
 *    Rev 1.0   Oct 06 2000 14:27:28   PaulLC
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

#include "subsys.h"

#include "pccomm.h"


//////////////////////////////////////////////////
//
// Private Data
//
//////////////////////////////////////////////////




//////////////////////////////////////////////////
//
// SSH Private Functions
//
//////////////////////////////////////////////////

#define		IS_TOP_SYSTEM_SELECTED(void)		\		
				(GetCurrSmId() == TopSubsystemHandlerID)


//////////////////////////////////////////////////
//
// Exit Procedures
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
// GoReset while in IDLE
//
//////////////////////////////////////////////////

int	SSH_exitA(void)
{	
		if(IS_TOP_SYSTEM_SELECTED())
		{
			SendMessage(TopTransportHandlerID, GoActive);
		}
		else
		{	
			SendMessage(BottomTransportHandlerID, GoActive);
		}

	return SSH_RESET_WAIT_1;
}


//////////////////////////////////////////////////
//
// TransportIsActive while in SSH_RESET_WAIT_1
//
//////////////////////////////////////////////////

int	SSH_exitA1(void)
{	
		if(IS_TOP_SYSTEM_SELECTED())
		{
			ClearTransportResetDoneFlag(TopTransportHandlerID);

			SendMessage(TopCameraSynchronizerHandlerID, GoActive);	
		}
		else
		{
			ClearTransportResetDoneFlag(BottomTransportHandlerID);

			SendMessage(BottomCameraSynchronizerHandlerID, GoActive);	
		}

	return SSH_RESET_WAIT_2;
}


//////////////////////////////////////////////////
//
// CameraIsActive while in SSH_RESET_WAIT_2
//
//////////////////////////////////////////////////

int	SSH_exitA2(void)
{	
		if(IS_TOP_SYSTEM_SELECTED())
		{
			ClearCameraResetDoneFlag(TopCameraSynchronizerHandlerID);

			SendMessage(TCMR_SystemManagerID, TopSubsystemResetDone);
		}
		else
		{
			ClearCameraResetDoneFlag(BottomCameraSynchronizerHandlerID);

			SendMessage(TCMR_SystemManagerID, BottomSubsystemResetDone);
		}

	return SSH_READY;
}


//////////////////////////////////////////////////
//
// SyncCameraTrigger while in SSH_READY
//
//////////////////////////////////////////////////

int	SSH_exitB(void)
{
		if(IS_TOP_SYSTEM_SELECTED())
		{
			SendMessage(TopCameraSynchronizerHandlerID, SyncCameraTrigger);
		}
		else
		{
			SendMessage(BottomCameraSynchronizerHandlerID, SyncCameraTrigger);	
		}

	return SAME_STATE;
}

//////////////////////////////////////////////////
//
// 	while in SSH_READY
//
//////////////////////////////////////////////////

int	SSH_exitB2(void)
{
		if(IS_TOP_SYSTEM_SELECTED())
		{
			SendMessage(TopCameraSynchronizerHandlerID, CameraCaptureFast);
		}
		else
		{
			SendMessage(BottomCameraSynchronizerHandlerID, CameraCaptureFast);	
		}

	return SAME_STATE;
}

//////////////////////////////////////////////////
//
// 	while in SSH_READY
//
//////////////////////////////////////////////////

int	SSH_exitB3(void)
{
		if(IS_TOP_SYSTEM_SELECTED())
		{
			SendMessage(TopTransportHandlerID, TransportFindHome);
		}
		else
		{
			SendMessage(BottomTransportHandlerID, TransportFindHome);	
		}

	return SAME_STATE;
}

//////////////////////////////////////////////////
//
// 	while in SSH_READY
//
//////////////////////////////////////////////////

int	SSH_exitB4(void)
{
		if(IS_TOP_SYSTEM_SELECTED())
		{
			SendMessage(TopTransportHandlerID, TransportGoHome);
		}
		else
		{
			SendMessage(BottomTransportHandlerID, TransportGoHome);	
		}

	return SAME_STATE;
}

//////////////////////////////////////////////////
//
// 	while in SSH_READY
//
//////////////////////////////////////////////////

int	SSH_exitB5(void)
{
		if(IS_TOP_SYSTEM_SELECTED())
		{
			SendMessage(TopTransportHandlerID, TransportGotoPosition);
		}
		else
		{
			SendMessage(BottomTransportHandlerID, TransportGotoPosition);	
		}

	return SAME_STATE;
}

//////////////////////////////////////////////////
//
// 	while in SSH_READY
//
//////////////////////////////////////////////////

int	SSH_exitB6(void)
{
		if(IS_TOP_SYSTEM_SELECTED())
		{
			SendMessage(TopTransportHandlerID, TransportJog);
		}
		else
		{
			SendMessage(BottomTransportHandlerID, TransportJog);	
		}

	return SAME_STATE;
}

//////////////////////////////////////////////////
//
// Camera Reset
//
//////////////////////////////////////////////////

int	SSH_exitCR1(void)
{
		if(IS_TOP_SYSTEM_SELECTED())
		{
			SendMessage(TopCameraSynchronizerHandlerID, CameraReset);

		}
		else
		{
			SendMessage(BottomCameraSynchronizerHandlerID, CameraReset);	
		}

	return SSH_CR_WAIT1;
}

//////////////////////////////////////////////////
//
// CameraIsActive while in SSH_CR_WAIT1
//
//////////////////////////////////////////////////

int	SSH_exitCR2(void)
{	
		if(IS_TOP_SYSTEM_SELECTED())
		{
			ClearCameraResetDoneFlag(TopCameraSynchronizerHandlerID);
		}
		else
		{
			ClearCameraResetDoneFlag(BottomCameraSynchronizerHandlerID);
		}

	return SSH_READY;
}


//////////////////////////////////////////////////
//
// Transport Reset
//
//////////////////////////////////////////////////

int	SSH_exitTR1(void)
{
		if(IS_TOP_SYSTEM_SELECTED())
		{
			SendMessage(TopTransportHandlerID, TransportReset);
		}
		else
		{
			SendMessage(BottomTransportHandlerID, TransportReset);
		}

	return SSH_TR_WAIT1;
}

//////////////////////////////////////////////////
//
// TransportIsActive while in SSH_TR_WAIT1
//
//////////////////////////////////////////////////

int	SSH_exitTR2(void)
{	
		if(IS_TOP_SYSTEM_SELECTED())
		{
			ClearTransportResetDoneFlag(TopTransportHandlerID);
		}
		else
		{
			ClearTransportResetDoneFlag(BottomTransportHandlerID);
		}

	return SSH_READY;
}



//////////////////////////////////////////////////
//
// STATE MACHINE MATRIX DEFINITIONS
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_SSH_IDLE)
	EV_HANDLER(GoReset, SSH_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SSH_RESET_WAIT_1)
	EV_HANDLER(TransportIsActive, SSH_exitA1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SSH_RESET_WAIT_2)
	EV_HANDLER(CameraIsActive, SSH_exitA2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SSH_CR_WAIT1)
	EV_HANDLER(CameraIsActive, SSH_exitCR2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SSH_TR_WAIT1)
	EV_HANDLER(TransportIsActive, SSH_exitTR2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SSH_READY)
	EV_HANDLER(SyncCameraTrigger, SSH_exitB),
	EV_HANDLER(CameraCaptureFast, SSH_exitB2),
	EV_HANDLER(TransportFindHome, SSH_exitB3),
	EV_HANDLER(TransportGoHome, SSH_exitB4),
	EV_HANDLER(TransportGotoPosition, SSH_exitB5),
	EV_HANDLER(TransportJog, SSH_exitB6),
	EV_HANDLER(CameraReset, SSH_exitCR1),
	EV_HANDLER(TransportReset, SSH_exitTR1)
STATE_TRANSITION_MATRIX_END;


// 
// VERY IMPORTANT : 
//		State Entry definition order MUST match the 
//		order of the state definition in the .H File 
//

SM_RESPONSE_ENTRY(SSH_Entry)
	STATE(_SSH_IDLE)				,
	STATE(_SSH_READY)				,
	STATE(_SSH_RESET_WAIT_1)		,
	STATE(_SSH_RESET_WAIT_2)		,	
	STATE(_SSH_CR_WAIT1)			,
	STATE(_SSH_TR_WAIT1)
SM_RESPONSE_END




