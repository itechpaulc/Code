



/*
 *
 *
 *		$Header:   K:/Projects/Tcmr/Source/WebEnc.c_v   1.3   11 Jul 2003 14:48:28   PaulLC  $
 *		$Log:   K:/Projects/Tcmr/Source/WebEnc.c_v  $
 * 
 *    Rev 1.3   11 Jul 2003 14:48:28   PaulLC
 * Incorporated all changes since 0.60.B; Camera Trigger jumping fixes; 
 * Encoder noise filtering; Changes to supporte latest TCMRC HW; Limit switch configure.
 * 
 *    Rev 1.2   Apr 08 2002 14:32:14   PaulLC
 * Contains all of the changes made during beta development.
 * 
 *    Rev 1.1   Aug 27 2001 15:48:26   PaulLC
 * Changes made up to Engineering Version 00.25.A.
 * 
 *    Rev 1.0   Oct 06 2000 14:27:32   PaulLC
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

#include "webenc.h"


//////////////////////////////////////////////////
//
// WEH public Functions
//
//////////////////////////////////////////////////

void	SetTdcPulseEdge(BOOL pE)
{
	tdcPulseEdge = pE;

	if(tdcPulseEdge == FALLING_EDGE)
		ENABLE_WEB_ENCODER_TDC_CAPTURE_FALL();
	else
		ENABLE_WEB_ENCODER_TDC_CAPTURE_RISE();
}

void	SetWebEncoderDivideBy(BYTE wEDB)
{
	webEncoderDivideBy = wEDB;

	switch(webEncoderDivideBy)
	{
		case WEB_ENCODER_DIVIDE_BY_NONE:
			SET_WEB_ENCODER_DIVIDE_BY_NONE();
			break;

		case WEB_ENCODER_DIVIDE_BY_2:
			SET_WEB_ENCODER_DIVIDE_BY_2();
			break;

		case WEB_ENCODER_DIVIDE_BY_4:
			SET_WEB_ENCODER_DIVIDE_BY_4();
			break;

		case WEB_ENCODER_DIVIDE_BY_8:
			SET_WEB_ENCODER_DIVIDE_BY_8();
			break;

		case WEB_ENCODER_DIVIDE_BY_16:
			SET_WEB_ENCODER_DIVIDE_BY_16();
			break;

		case WEB_ENCODER_DIVIDE_BY_32:
			SET_WEB_ENCODER_DIVIDE_BY_32();
			break;

		case WEB_ENCODER_DIVIDE_BY_64:
			SET_WEB_ENCODER_DIVIDE_BY_64();
			break;

		case WEB_ENCODER_DIVIDE_BY_128:
			SET_WEB_ENCODER_DIVIDE_BY_128();
			break;

		default:
			break;
	}
}

void	SetWebEncoderFilter(BYTE wEF)
{
	webEncoderFilter = wEF;

	switch(webEncoderFilter)
	{
		case ENCODER_DIGITAL_FILTER_0:
			SET_ENCODER_DIGITAL_FILTER_0();
			break;

		case ENCODER_DIGITAL_FILTER_1:
			SET_ENCODER_DIGITAL_FILTER_1();
			break;

		case ENCODER_DIGITAL_FILTER_2:
			SET_ENCODER_DIGITAL_FILTER_2();
			break;

		case ENCODER_DIGITAL_FILTER_3:
			SET_ENCODER_DIGITAL_FILTER_3();
			break;

		default:
			break;
	}
}


//////////////////////////////////////////////////
//
// WEH Private Data
//
//////////////////////////////////////////////////

WORD		near	encoderPeriod;

DWORD		near	impressionCount;

WORD		near	encoderCntPerImpression;		// System Parameter

WORD		near	currEncoderIndex;

int			near	topCameraEncoderIndexNotify;

int			near	bottomCameraEncoderIndexNotify;

BOOL		near	tdcPulseEdge;

BYTE		near	webEncoderDivideBy;

BYTE		near	webEncoderFilter;

BOOL		near	pressSpeedZeroFlag;



//////////////////////////////////////////////////
//
// WEH Private Functions
//
//////////////////////////////////////////////////

void	HandleEncoderUpdate(void)
{	
	CLEAR_PRESS_SPEED_ZERO();

	SetEncoderPeriod(GetCurrEncoderPeriod());

	// Check for notification requests

	if(TopCameraNeedsEncoderIndexNotification() &&
		topCameraEncoderIndexNotify == GetCurrEncoderIndex())
	{
		SendMessage
			(TopCameraSynchronizerHandlerID, CameraEncoderIndexHit);

		ClearTopCameraEncoderIndexNotify();
	}

	if(BottomCameraNeedsEncoderIndexNotification() &&
		bottomCameraEncoderIndexNotify == GetCurrEncoderIndex())
	{
		SendMessage
			(BottomCameraSynchronizerHandlerID, CameraEncoderIndexHit);

		ClearBottomCameraEncoderIndexNotify();
	}
}

//////////////////////////////////////////////////
//
// 
//
//////////////////////////////////////////////////

void	SetPressSpeedToZero(void)
{
	SetEncoderPeriod(0x0000);	// Zero Encoder
								// Period for CCU
	
	SET_PRESS_SPEED_ZERO();		// Flag
}

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

int	WEH_exitA(void)
{	
	// Encoder Parameters have
	// been set by CCU at this point

		ENABLE_WEB_ENCODER_CAPTURE_FALL();	// Start Encoder
											// Detection

		StartTimer(MAX_KERNEL_TICKS);		// Press Speed
											// Zero Detect

	return WEH_ACTIVE;
}


//////////////////////////////////////////////////
//
// EncoderPulseDetect
// while in ACTIVE
//
//////////////////////////////////////////////////

int	WEH_exitB(void)
{
		IncrementCurrEncoderIndex();

		HandleEncoderUpdate();

		StartTimer(MAX_KERNEL_TICKS);	// Press Speed
										// Zero Detect
	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// EncoderTdcPulseDetect while in ACTIVE
//
//////////////////////////////////////////////////

int	WEH_exitC(void)
{	
		IncrementImpressionCount();

		StartTimer(MAX_KERNEL_TICKS);	// Press Speed
										// Zero Detect
	return WEH_WAIT_TDC_SYNC;
}

//////////////////////////////////////////////////
//
// TimeOut while in ACTIVE
//
//////////////////////////////////////////////////

int	WEH_exitD(void)
{	
		SetPressSpeedToZero();	// Perid = 0 for CCU

		// Check for notification requests

		if(TopCameraNeedsEncoderIndexNotification())
		{
			SendMessage
				(TopCameraSynchronizerHandlerID, PressSpeedZeroDetected);

			ClearTopCameraEncoderIndexNotify();
		}

		if(BottomCameraNeedsEncoderIndexNotification())
		{
			SendMessage
				(BottomCameraSynchronizerHandlerID, PressSpeedZeroDetected);

			ClearBottomCameraEncoderIndexNotify();
		}

	return SAME_STATE;
}

//////////////////////////////////////////////////
//
//	EncoderPulseDetect
//	while in WAIT_TDC_SYNC
//
//////////////////////////////////////////////////

int	WEH_exitE(void)
{	
		ResetCurrEncoderIndex();
		
		HandleEncoderUpdate();

		StartTimer(MAX_KERNEL_TICKS);	// Press Speed
										// Zero Detect

	return WEH_ACTIVE;
}

//////////////////////////////////////////////////
//
// TimeOut while in WAIT_TDC_SYNC
//
//////////////////////////////////////////////////

int	WEH_exitF(void)
{	
		SetPressSpeedToZero();

	return SAME_STATE;
}

//////////////////////////////////////////////////
//
// STATE MACHINE MATRIX DEFINITIONS
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_WEH_IDLE)
	EV_HANDLER(GoActive, WEH_exitA)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_WEH_ACTIVE)
	EV_HANDLER(EncoderPulseDetect, WEH_exitB)		,
	EV_HANDLER(EncoderTdcPulseDetect, WEH_exitC)	,
	EV_HANDLER(TimeOut, WEH_exitD)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_WEH_WAIT_TDC_SYNC)
	EV_HANDLER(EncoderPulseDetect, WEH_exitE)		,
	EV_HANDLER(TimeOut, WEH_exitF)
STATE_TRANSITION_MATRIX_END;


// 
// VERY IMPORTANT : 
//		State Entry definition order MUST match the 
//		order of the state definition in the .H File 
//

SM_RESPONSE_ENTRY(WEH_Entry)
	STATE(_WEH_IDLE)			,
	STATE(_WEH_ACTIVE)			,
	STATE(_WEH_WAIT_TDC_SYNC)
SM_RESPONSE_END






