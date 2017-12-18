



/*
 *
 *
 *		$Header:   K:/Projects/Tcmr/Source/Camsync.c_v   1.2   Apr 08 2002 14:30:56   PaulLC  $
 *		$Log:   K:/Projects/Tcmr/Source/Camsync.c_v  $
 * 
 *    Rev 1.2   Apr 08 2002 14:30:56   PaulLC
 * Contains all of the changes made during beta development.
 * 
 *    Rev 1.1   Aug 27 2001 15:47:44   PaulLC
 * Changes made up to Engineering Version 00.25.A.
 * 
 *    Rev 1.0   Oct 06 2000 14:27:18   PaulLC
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

#include "camsync.h"

#include "webenc.h"

#include "pccomm.h"


//////////////////////////////////////////////////
//
// Private Data
//
//////////////////////////////////////////////////

WORD			near	camPreTriggerSetupIndexDistance;

EncoderMark		near	topCameraFlashEncoderMark;

EncoderMark		near	bottomCameraFlashEncoderMark;


BYTE			near	cameraSelect;

WORD			near	camTriggerRetardTime;

WORD			near	camShutterTime;

WORD			near	camFlashRechargeTime;



//////////////////////////////////////////////////
//
// CSH Public Functions
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
// This function examines an encoder index
// and returns a new value as necessary (if negative)
// This is to give a new value that is within the
// valid range based on the Impression Length Data
//
//////////////////////////////////////////////////

WORD	NormalizeEncoderIndex(signed int encoderIdx)
{
	if(encoderIdx < 0)
		encoderIdx += GetEncoderCountPerImpression();

	return encoderIdx;
}

//////////////////////////////////////////////////
//
// CSH Private Functions
//
//////////////////////////////////////////////////

#define		IS_TOP_CAMERA_SELECTED()					\		
				(GetCurrSmId() == TopCameraSynchronizerHandlerID)


//////////////////////////////////////////////////
//
// Exit Procedures
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
// GoActive or CameraReset
// while in IDLE
//
//////////////////////////////////////////////////

#define		CAMERA_RESET_DELAY		(100)

int	CSH_exitA(void)
{			
		if(IS_TOP_CAMERA_SELECTED())
		{
			DISABLE_TOP_CAMERA_TRIGGER();
			RESET_TOP_CAMERA_TRIGGER();
		}
		else
		{
			DISABLE_BOTTOM_CAMERA_TRIGGER();
			RESET_BOTTOM_CAMERA_TRIGGER();
		}

		StartTimer(CAMERA_RESET_DELAY);
		
	return CSH_INIT0;
}

//////////////////////////////////////////////////
//
// TimeOut
//		while in CSH_INIT_0
//
//////////////////////////////////////////////////

int	CSH_exitA1(void)
{			
		if(IS_TOP_CAMERA_SELECTED())
		{
			RELEASE_TOP_CAMERA_SYSTEM_RESET();
		}
		else
		{
			RELEASE_BOTTOM_CAMERA_SYSTEM_RESET();
		}

		StartTimer(CAMERA_RESET_DELAY);
		
	return CSH_INIT1;
}

//////////////////////////////////////////////////
//
// TimeOut
//		while in CSH_INIT_1
//
//////////////////////////////////////////////////

int	CSH_exitA2(void)
{			
		if(IS_TOP_CAMERA_SELECTED())
		{
			SendMessage
				(TopSubsystemHandlerID, CameraIsActive);
		}
		else
		{
			SendMessage
				(BottomSubsystemHandlerID, CameraIsActive);
		}
		
	return CSH_READY;
}


//////////////////////////////////////////////////
//
//	SyncCameraTrigger while in READY
//
//////////////////////////////////////////////////

int	CSH_exitB(void)
{
	int notifyIdx;

	if(IS_PRESS_IS_RUNNING())
	{
		if(IS_TOP_CAMERA_SELECTED())
		{
			notifyIdx = GetTopCameraFlashEncoderIndex() -
						GetCameraPreTriggerSetupIndexDistance();
			
			SetTopCameraEncoderIndexNotify
				(NormalizeEncoderIndex(notifyIdx));
		}
		else
		{
			notifyIdx = GetBottomCameraFlashEncoderIndex() -
						GetCameraPreTriggerSetupIndexDistance();
						
			SetBottomCameraEncoderIndexNotify
				(NormalizeEncoderIndex(notifyIdx));
		}
	
		return CSH_ENC_IDX_HIT_WAIT;
	}
	else
	{
			SendMessage(THIS_SM, CameraCaptureFast);

		return SAME_STATE;
	}
}


//////////////////////////////////////////////////
//
//	CameraCaptureFast while in READY
//
//////////////////////////////////////////////////

int	CSH_exitB2(void)
{
			if(IS_TOP_CAMERA_SELECTED())
			{
				// Top Force Trigger

				SET_TOP_CAMERA_TRIGGER();
			}
			else
			{
				// Bottom Force Trigger

				SET_BOTTOM_CAMERA_TRIGGER();
			}	
		
		SendMessage(THIS_SM, CameraTriggerDone);

	return CSH_CAM_TRIG_DONE_WAIT;
}

//////////////////////////////////////////////////
//
//	CameraEncoderIndexHit while 
//	in ENC_IDX_HIT_WAIT
//
//////////////////////////////////////////////////

WORD			topCameraTriggerOutputCompareTime,
				bottomCameraTriggerOutputCompareTime;

DWORD_UNION		tempRemainderCamera;

int				triggerIndexDistance;

void	CalculateTriggerIndexDistance(BYTE cameraSelect)
{
		if(cameraSelect == TOP_CAMERA)
			triggerIndexDistance = 
				(int)GetCurrEncoderIndex() - GetTopCameraFlashEncoderIndex();
		else
			triggerIndexDistance = 
				(int)GetCurrEncoderIndex() - GetBottomCameraFlashEncoderIndex();

		if(triggerIndexDistance < 0)
			triggerIndexDistance = -triggerIndexDistance;
		else
			triggerIndexDistance = 
				((int)(GetEncoderCountPerImpression() - GetCurrEncoderIndex()) +
						GetTopCameraFlashEncoderIndex());
}


int	CSH_exitC(void)	
{
		if(IS_TOP_CAMERA_SELECTED())
		{	
			CalculateTriggerIndexDistance(TOP_CAMERA);

			tempRemainderCamera.tempDWORD = 
				((DWORD)GetTopCameraFlashEncoderRemainder()) * 
				GetEncoderPeriod();

			topCameraTriggerOutputCompareTime	=
				GetLastEncoderTimeTag()						+		// Last Web Encoder Input Capture			
				(GetEncoderPeriod()	* triggerIndexDistance)	+		// 2 Web Encoder Index Time Ticks
				tempRemainderCamera.tempWORD				-		// Remainder Time Ticks
				GetCameraTriggerRetardTime();						// RETARD shutter, trigger

				SET_TOP_CAMERA_TRIGGER_OC_TIME
					(topCameraTriggerOutputCompareTime);

				ENABLE_TOP_CAMERA_TRIGGER();
		}
		else
		{
			CalculateTriggerIndexDistance(BOTTOM_CAMERA);

			tempRemainderCamera.tempDWORD = 
				((DWORD)GetBottomCameraFlashEncoderRemainder()) * 
				GetEncoderPeriod();

			bottomCameraTriggerOutputCompareTime	=
				GetLastEncoderTimeTag()						+		// Last Web Encoder Input Capture			
				(GetEncoderPeriod()	* triggerIndexDistance)	+		// 2 Web Encoder Index Time Ticks
				tempRemainderCamera.tempWORD				-		// Remainder Time Ticks
				GetCameraTriggerRetardTime();						// RETARD shutter, trigger

				SET_BOTTOM_CAMERA_TRIGGER_OC_TIME
					(bottomCameraTriggerOutputCompareTime);

				ENABLE_BOTTOM_CAMERA_TRIGGER();
		}

	return CSH_CAM_TRIG_DONE_WAIT;
}

//////////////////////////////////////////////////
//
//	PressSpeedZeroDetected while 
//	in ENC_IDX_HIT_WAIT
//
//////////////////////////////////////////////////

int	CSH_exitC1(void)	
{
		SendMessage(THIS_SM, CameraCaptureFast);

	return CSH_READY;
}

//////////////////////////////////////////////////
//
//	CameraTriggerDone while in TRIG_DONE_WAIT
//
//////////////////////////////////////////////////

int	CSH_exitD(void)
{
		if(IS_TOP_CAMERA_SELECTED())
		{
			DISABLE_TOP_CAMERA_TRIGGER();

			ClearCameraProcessDoneFlag(TopCameraSynchronizerHandlerID);
		}
		else
		{
			DISABLE_BOTTOM_CAMERA_TRIGGER();

			ClearCameraProcessDoneFlag(BottomCameraSynchronizerHandlerID);
		}

		StartTimer(MIN_KERNEL_TICKS);	// Short Delay before 
										// clearing the trigger signal
	return CSH_CAM_TRIG_CLEAR_WAIT;
}


//////////////////////////////////////////////////
//
//	TimeOut while in TRIG_CLEAR_WAIT
//
//////////////////////////////////////////////////

int	CSH_exitE(void)
{
		if(IS_TOP_CAMERA_SELECTED())
		{						
			RESET_TOP_CAMERA_TRIGGER();

			topCameraTriggerOutputCompareTime +=
				(GetCameraTriggerRetardTime() +
				 GetCameraFlashRechargeTime());

				SET_TOP_FLASH_RECHARGE_OC_TIME
					(topCameraTriggerOutputCompareTime);

			ENABLE_TOP_FLASH_RECHARGE_TIMER();
		}
		else
		{					
			RESET_BOTTOM_CAMERA_TRIGGER();

			bottomCameraTriggerOutputCompareTime +=
				(GetCameraTriggerRetardTime() +
				 GetCameraFlashRechargeTime());

				SET_BOTTOM_FLASH_RECHARGE_OC_TIME
					(bottomCameraTriggerOutputCompareTime);

			ENABLE_BOTTOM_FLASH_RECHARGE_TIMER();
		}

	return CSH_CAM_FLASH_RECHARGE_WAIT;
}

//////////////////////////////////////////////////
//
//	CameraTriggerDone while in FLASH_RECHARGE_WAIT
//
//////////////////////////////////////////////////

int	CSH_exitF(void)
{
		if(IS_TOP_CAMERA_SELECTED())
		{
			DISABLE_TOP_CAMERA_TRIGGER();

			ClearFlashRechargeDoneFlag(TopCameraSynchronizerHandlerID);
		}
		else
		{
			DISABLE_BOTTOM_CAMERA_TRIGGER();

			ClearFlashRechargeDoneFlag(BottomCameraSynchronizerHandlerID);
		}

	return CSH_READY;
}

//////////////////////////////////////////////////
//
//	CameraReset while 
//	in CSH_READY
//
//////////////////////////////////////////////////

int	CSH_exitCR(void)	
{
		SendMessage(THIS_SM, CameraReset);

	return CSH_IDLE;
}


//////////////////////////////////////////////////
//
// STATE MACHINE MATRIX DEFINITIONS
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_CSH_IDLE)
	EV_HANDLER(GoActive, CSH_exitA),
	EV_HANDLER(CameraReset, CSH_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_CSH_INIT0)
	EV_HANDLER(TimeOut, CSH_exitA1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_CSH_INIT1)
	EV_HANDLER(TimeOut, CSH_exitA2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_CSH_READY)
	EV_HANDLER(SyncCameraTrigger, CSH_exitB),
	EV_HANDLER(CameraCaptureFast, CSH_exitB2),
	EV_HANDLER(CameraReset, CSH_exitCR)
STATE_TRANSITION_MATRIX_END;
	

STATE_TRANSITION_MATRIX(_CSH_ENC_IDX_HIT_WAIT)
	EV_HANDLER(CameraEncoderIndexHit, CSH_exitC),
	EV_HANDLER(PressSpeedZeroDetected, CSH_exitC1)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_CSH_CAM_TRIG_DONE_WAIT)
	EV_HANDLER(CameraTriggerDone, CSH_exitD)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_CSH_CAM_TRIG_CLEAR_WAIT)
	EV_HANDLER(TimeOut, CSH_exitE)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_CSH_CAM_FLASH_RECHARGE_WAIT)
	EV_HANDLER(CameraTriggerDone, CSH_exitF)
STATE_TRANSITION_MATRIX_END;

// 
// VERY IMPORTANT : 
//		State Entry definition order MUST match the 
//		order of the state definition in the .H File 
//

SM_RESPONSE_ENTRY(CSH_Entry)
	STATE(_CSH_IDLE)						,
	STATE(_CSH_INIT0)						,
	STATE(_CSH_INIT1)						,
	STATE(_CSH_READY)						,
	STATE(_CSH_ENC_IDX_HIT_WAIT)			,
	STATE(_CSH_CAM_TRIG_DONE_WAIT)			,
	STATE(_CSH_CAM_TRIG_CLEAR_WAIT)			,
	STATE(_CSH_CAM_FLASH_RECHARGE_WAIT)	
SM_RESPONSE_END




