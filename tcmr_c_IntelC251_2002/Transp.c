



/*
 *
 *
 *		$Header:   K:/Projects/Tcmr/Source/Transp.c_v   1.3   11 Jul 2003 14:48:28   PaulLC  $
 *		$Log:   K:/Projects/Tcmr/Source/Transp.c_v  $
 * 
 *    Rev 1.3   11 Jul 2003 14:48:28   PaulLC
 * Incorporated all changes since 0.60.B; Camera Trigger jumping fixes; 
 * Encoder noise filtering; Changes to supporte latest TCMRC HW; Limit switch configure.
 * 
 *    Rev 1.2   Apr 08 2002 14:32:14   PaulLC
 * Contains all of the changes made during beta development.
 * 
 *    Rev 1.1   Aug 27 2001 15:48:24   PaulLC
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

#include "transp.h"

#include "motorcs.h"

#include "tparam.h"

#include "pccomm.h"

#include "pevmon.h"



//////////////////////////////////////////////////
//
// Private Data
//
//////////////////////////////////////////////////

BYTE	near	moveSettleWait;

DWORD	near	findHomeSpeed		,
				locateSpeed			,
				backlashSpeed		,
				jogSpeed			,
				hitHomeLimitSpeed	;

WORD	near	backlashCount		,		
				homeDistanceFromNearLimit;

DWORD	near	startingVelocity,
				acceleration;


signed	long	near	currentPosition[MAX_TRANSPORTS];

signed	long	near	requestedPosition[MAX_TRANSPORTS];

signed	long	near	truePosition[MAX_TRANSPORTS];

signed	int		near	positionErrorCount[MAX_TRANSPORTS];

//

BYTE	near	transportSelect;

#define		TRANSPORT_IN_NEG_LIMIT		1
#define		TRANSPORT_IN_POS_LIMIT		2
#define		TRANSPORT_IN_MIDDLE			3

BYTE	near	transportLocation;

//
// Transport Direction Flags
// Current Move Direction
// 

BOOL	near	directionForward[MAX_TRANSPORTS];


#define		SET_TRANSPORT_MOVING_FORWARD(transpSel)		\
				(directionForward[transpSel] = SET)

#define		SET_TRANSPORT_MOVING_REVERSE(transpSel)		\
				(directionForward[transpSel] = CLEAR)

#define		IS_TRANSPORT_MOVING_FORWARD(transpSel)		\
				(directionForward[transpSel] == SET)


//
// Transport Calibration Flags
// 

BOOL	near	transportCalibrated[MAX_TRANSPORTS];


#define		SET_TRANSPORT_CALIBRATED(transpSel)			\
				(transportCalibrated[transpSel] = SET)

#define		SET_TRANSPORT_UNCALIBRATED(transpSel)		\
				(transportCalibrated[transpSel] = CLEAR)


//
//
//

BOOL	near	transportInLimit[MAX_TRANSPORTS];


#define		SET_TRANSPORT_IN_LIMIT(transpSel)			\
				(transportInLimit[transpSel] = SET)

#define		CLEAR_TRANSPORT_IN_LIMIT(transpSel)			\
				(transportInLimit[transpSel] = CLEAR)


//
//
//

BOOL	near	transportNotReady[MAX_TRANSPORTS];


#define		SET_TRANSPORT_NOT_READY(transpSel)			\
				(transportNotReady[transpSel] = SET)

#define		CLEAR_TRANSPORT_NOT_READY()					\
				(transportNotReady[transpSel] = CLEAR)


//
//
//

BOOL	near	transportInPositionError[MAX_TRANSPORTS];


#define		SET_TRANSPORT_IN_POSITION_ERROR(transpSel)			\
				(transportInPositionError[transpSel] = SET)

#define		CLEAR_TRANSPORT_IN_POSITION_ERROR(transpSel)		\
				(transportInPositionError[transpSel] = CLEAR)


//
//
//

BOOL	near	movingOutOfNegativeLimit[MAX_TRANSPORTS];

#define		SET_TRANSP_MOVING_OUT_OF_NEGATIVE_LIMIT(transpSel)		\
				(movingOutOfNegativeLimit[transpSel] = SET)

#define		CLEAR_TRANSP_MOVING_OUT_OF_NEGATIVE_LIMIT(transpSel)	\
				(movingOutOfNegativeLimit[transpSel] = CLEAR)

#define		SET_TRANSP_MOVING_OUT_OF_POSITIVE_LIMIT(transpSel)		\
					CLEAR_TRANSP_MOVING_OUT_OF_NEGATIVE_LIMIT(transpSel)

#define		IS_TRANSP_MOVING_OUT_OF_NEGATIVE_LIMIT(transpSel)		\
				(movingOutOfNegativeLimit[transpSel] == SET)




//////////////////////////////////////////////////
//
// 
//
//////////////////////////////////////////////////

BOOL	near	transportReversed[MAX_TRANSPORTS];

// Trasnport Orientation Masks

#define		TOP_TRANSP_REVERSED_ORIENTATION			0x01
#define		BOTTOM_TRANSP_REVERSED_ORIENTATION		0x02

void	SetTransportOrientation(BYTE trasnpOrientation)
{
	if(trasnpOrientation & TOP_TRANSP_REVERSED_ORIENTATION)
		SetTransportReversed(TOP_TRANSPORT_SEL);
	else
		ClearTransportReversed(TOP_TRANSPORT_SEL);

	if(trasnpOrientation & BOTTOM_TRANSP_REVERSED_ORIENTATION)
		SetTransportReversed(BOTTOM_TRANSPORT_SEL);
	else
		ClearTransportReversed(BOTTOM_TRANSPORT_SEL);
}

BYTE	GetTransportOrientation(void)
{
	BYTE transpOrient = 0x00;

		if(IsTransportReversed(TOP_TRANSPORT_SEL))
			transpOrient |= TOP_TRANSP_REVERSED_ORIENTATION;

		if(IsTransportReversed(BOTTOM_TRANSPORT_SEL))
			transpOrient |= BOTTOM_TRANSP_REVERSED_ORIENTATION;

	return transpOrient;
}

//////////////////////////////////////////////////
//
// 
//
//////////////////////////////////////////////////

BOOL	near	stepperEncoderAvailable[MAX_TRANSPORTS];

// Trasnport EncoderAvailable Masks

#define		TOP_TRANSP_ENCODER_AVAILABLE			0x01
#define		BOTTOM_TRANSP_ENCODER_AVAILABLE			0x02

void	SetTransportEncoderAvailable(BYTE trasnpEncAvail)
{
	if(trasnpEncAvail & TOP_TRANSP_ENCODER_AVAILABLE)
		SetStepperEncoderAvailable(TOP_TRANSPORT_SEL);
	else
		ClearStepperEncoderAvailable(TOP_TRANSPORT_SEL);

	if(trasnpEncAvail & BOTTOM_TRANSP_ENCODER_AVAILABLE)
		SetStepperEncoderAvailable(BOTTOM_TRANSPORT_SEL);
	else
		ClearStepperEncoderAvailable(BOTTOM_TRANSPORT_SEL);
}

BYTE	GetTransportEncoderAvailable(void)
{
	BYTE trasnpEncAvail = 0x00;

		if(IsStepperEncoderAvailable(TOP_TRANSPORT_SEL))
			trasnpEncAvail |= TOP_TRANSP_ENCODER_AVAILABLE;

		if(IsStepperEncoderAvailable(BOTTOM_TRANSPORT_SEL))
			trasnpEncAvail |= BOTTOM_TRANSP_ENCODER_AVAILABLE;

	return trasnpEncAvail;
}

//////////////////////////////////////////////////
//
// 
//
//////////////////////////////////////////////////

void	SetLimitConfig(BYTE limitConfig)
{
		CLEAR_LIMIT_CONFIG();

		limitConfig &= LIMIT_CONFIG_MASK;

		SET_EXT_OUT_PORT_B(GET_EXT_OUT_PORT_B() | limitConfig);
}

BYTE	GetLimitConfig(void)
{
	return	GET_LIMIT_CONFIG();
}

//////////////////////////////////////////////////
//
// SSH Private Functions
//
//////////////////////////////////////////////////

#define		IS_TOP_TRANSPORT_SELECTED()		\		
				(GetCurrSmId() == TopTransportHandlerID)



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

// Transport DELAY of 500 msec after transport reset

#define		TRANSPORT_RESET_DELAY		(100)

int	TRH_exitA(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
			transportSelect = TOP_TRANSPORT_SEL;
		else
			transportSelect = BOTTOM_TRANSPORT_SEL;


			SET_TRANSPORT_UNCALIBRATED(transportSelect);

			CLEAR_TRANSPORT_IN_LIMIT(transportSelect);
			CLEAR_TRANSPORT_IN_POSITION_ERROR(transportSelect);

			currentPosition[transportSelect] = 0x00000000;	
			requestedPosition[transportSelect] = 0x00000000;

		StartTimer(TRANSPORT_RESET_DELAY);

	return TRH_INIT_0;
}

//////////////////////////////////////////////////
//
// TimeOut
//		while in TRH_INIT_0
//
//////////////////////////////////////////////////

int	TRH_exitA0(void)
{	
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			RELEASE_TOP_TRANSPORT_RESET();
		}
		else
		{
			RELEASE_BOTTOM_TRANSPORT_RESET();
		}
		
		StartTimer(TRANSPORT_RESET_DELAY);

	return TRH_INIT_1;
}


//////////////////////////////////////////////////
//
// TimeOut while in TRH_INIT_1
//
//////////////////////////////////////////////////

int	TRH_exitA1(void)
{
	if(IS_TOP_TRANSPORT_SELECTED())
	{
		if(IsStepperEncoderAvailable(TOP_TRANSPORT_AXIS_SEL))
		{
			PEND_TOP_TRANSP_HANDLER_REQUEST();

			return TRH_INIT_2;
		}
		else
		{
			SendMessage
				(TopSubsystemHandlerID, TransportIsActive);
			
			return TRH_ACTIVE;
		}
	}
	else
	{
		if(IsStepperEncoderAvailable(BOTTOM_TRANSPORT_AXIS_SEL))
		{
			PEND_BOTTOM_TRANSP_HANDLER_REQUEST();

			return TRH_INIT_2;
		}
		else
		{
			SendMessage
				(BottomSubsystemHandlerID, TransportIsActive);
			
			return TRH_ACTIVE;
		}
	}
}

//////////////////////////////////////////////////
//
// MotorChipSetResourceGranted while in 
//	TRH_INIT_2
//
//////////////////////////////////////////////////

int	TRH_exitA2(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			MCS_SEND_SET_CURR_AXIS_1();
		}
		else
		{
			MCS_SEND_SET_CURR_AXIS_2();
		}

	return	TRH_INIT_3;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent while in
//	TRH_INIT_3
//
//////////////////////////////////////////////////

int	TRH_exitA3(void)
{
		MCS_SEND_SET_AUTO_UPDATE_ON();

	return	TRH_INIT_4;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent while in
//	TRH_INIT_4
//
//////////////////////////////////////////////////

int	TRH_exitA4(void)
{	
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			SendMessage
				(TopSubsystemHandlerID, TransportIsActive);
		}
		else
		{
			SendMessage
				(BottomSubsystemHandlerID, TransportIsActive);
		}
	
		SendMessage
			(MotorChipSetHandlerID, MotorChipSetReleaseResource);

	return TRH_ACTIVE;
}


//////////////////////////////////////////////////
//
// TransportFindHome 
//		while in ACTIVE
//
//////////////////////////////////////////////////

int	TRH_exitB(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			PEND_TOP_TRANSP_HANDLER_REQUEST();
		}
		else
		{
			PEND_BOTTOM_TRANSP_HANDLER_REQUEST();
		}
	
	return TRH_FI_HM_MCS_REQ_1;
}

//////////////////////////////////////////////////
//
// MotorChipSetResourceGranted while in 
//	TRH_FI_HM_MCS_REQ_1
//
//////////////////////////////////////////////////

int	TRH_exitB0A(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			MCS_SEND_SET_CURR_AXIS_1();
		}
		else
		{
			MCS_SEND_SET_CURR_AXIS_2();
		}

	return	TRH_FI_HM_SEND_ZERO_POS_1A;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent while in
//	TRH_FI_HM_SEND_ZERO_POS_1A
//
//////////////////////////////////////////////////

int	TRH_exitB0B(void)
{
		MCS_SEND_ZERO_POSITION();	

	return	TRH_FI_HM_SEND_ZERO_POS_1B;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent while in
//	TRH_FI_HM_SEND_ZERO_POS_1B
//
//////////////////////////////////////////////////

int	TRH_exitB1B(void)
{
		// Allows to move out of limits
		// if transport is in limits

		MCS_SEND_LIMIT_SWITCH_SENSE_OFF();

	return	TRH_FI_HM_SEND_LIMIT_OFF;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent while in
//	TRH_FI_HM_SEND_LIMIT_OFF
//
//////////////////////////////////////////////////

int	TRH_exitB1C(void)
{
		MCS_SEND_GET_LIMIT_SWITCH_STATE();

	return	TRH_FI_HM_SEND_GET_LIM_SW_1;
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent while in
//	TRH_FI_HM_SEND_GET_LIM_SW_1
//
//	This state "KICK STARTS" the machine so
//	that the transport is positioned outside of 
//	the limits
//
//////////////////////////////////////////////////

signed	long	near	targetPosition;
WORD			near	limitStatus[MAX_TRANSPORTS];

#define			AXIS_1_POSITIVE_LIM			BIT0MSK
#define			AXIS_1_NEGATIVE_LIM			BIT1MSK
#define			AXIS_2_POSITIVE_LIM			BIT2MSK
#define			AXIS_2_NEGATIVE_LIM			BIT3MSK


#define			IS_TOP_TRANSPORT_IN_NEG_LIM(transpIdx)		\
					((limitStatus[transpIdx] & AXIS_1_NEGATIVE_LIM) == CLEAR)
	
#define			IS_TOP_TRANSPORT_IN_POS_LIM(transpIdx)		\
					((limitStatus[transpIdx] & AXIS_1_POSITIVE_LIM) == CLEAR)

#define			IS_BOTTOM_TRANSPORT_IN_NEG_LIM(transpIdx)	\
					((limitStatus[transpIdx] & AXIS_2_NEGATIVE_LIM) == CLEAR)
	
#define			IS_BOTTOM_TRANSPORT_IN_POS_LIM(transpIdx)	\
					((limitStatus[transpIdx] & AXIS_2_POSITIVE_LIM) == CLEAR)

//
//
//

#define		MCS_DEFAULT_INTERRUPT_MASK						\
				(MOTION_COMPLETE | POSITIVE_LIMIT_SWITCH	\
					| NEGATIVE_LIMIT_SWITCH | MOTION_ERROR)
				
int	TRH_exitB2(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			transportSelect = TOP_TRANSPORT_SEL;
			
			limitStatus[transportSelect] = READ_MOTOR_CHIPSET_DATA1();			

			if(IS_TOP_TRANSPORT_IN_NEG_LIM(transportSelect))
				transportLocation = TRANSPORT_IN_NEG_LIMIT;
			else
			if(IS_TOP_TRANSPORT_IN_POS_LIM(transportSelect))
				transportLocation = TRANSPORT_IN_POS_LIMIT;
			else
				transportLocation = TRANSPORT_IN_MIDDLE;
		}
		else
		{
			transportSelect = BOTTOM_TRANSPORT_SEL;
			
			limitStatus[transportSelect] = READ_MOTOR_CHIPSET_DATA1();		

			if(IS_BOTTOM_TRANSPORT_IN_NEG_LIM(transportSelect))
				transportLocation = TRANSPORT_IN_NEG_LIMIT;
			else
			if(IS_BOTTOM_TRANSPORT_IN_POS_LIM(transportSelect))
				transportLocation = TRANSPORT_IN_POS_LIMIT;
			else
				transportLocation = TRANSPORT_IN_MIDDLE;
		}

		SET_TP_AXIS(transportSelect);

		if((transportLocation == TRANSPORT_IN_NEG_LIMIT) ||
			(transportLocation == TRANSPORT_IN_POS_LIMIT))
		{
			// TRH_FI_HM_
			
			SetCurrentPosition	
				(TRANSPORT_HOME_POSITION, transportSelect);

				if(transportLocation == TRANSPORT_IN_NEG_LIMIT)
				{
					// In NEG Limit, Setup Move Params

					SET_TRANSP_MOVING_OUT_OF_NEGATIVE_LIMIT(transportSelect);

					// Always go to the positive direction
					// to move out of the negative limit					
					
					SetPositionRequest
						(GetCurrentPosition(transportSelect) + 
						TRANSPORT_MOVE_OUT_DISTANCE, transportSelect);
				}
				else
				{
					// In POS Limit, Setup Move Params

					SET_TRANSP_MOVING_OUT_OF_POSITIVE_LIMIT(transportSelect);
					
					// Always go to the negative direction, 
					// to move out of the positive limit				
					
					SetPositionRequest
						(GetCurrentPosition(transportSelect) - 
						TRANSPORT_MOVE_OUT_DISTANCE, transportSelect);
				}

				targetPosition = GetRequestedPosition(transportSelect);

				SET_TP_TARGET_POSITION(targetPosition);
				SET_TP_START_VELOCITY(GetStartingVelocity());
				SET_TP_MAX_VELOCITY(GetLocateSpeed());
				SET_TP_ACCELERATION(GetAcceleration());
				SET_TP_INTERRUPT_MASK(MCS_DEFAULT_INTERRUPT_MASK);
		
				// Note: ACTUAL_AXIS_POSITION is already ZERO

			SendMessage
				(TransportParamHandlerID, SendTransportProfileParams);
			
			return 
				TRH_FI_HM_SEND_MOVE_LIMIT_PROFILE;
		}
		else		
		{
			// In Middle of Transport

			MCS_SEND_LIMIT_SWITCH_SENSE_ON();
		
			return
				TRH_FI_HM_SEND_LIMIT_ON;
		}
}


//////////////////////////////////////////////////
//
//  TransportProfileParamsSent while in 
//	TRH_FI_HM_SEND_MOVE_LIMIT_PROFILE
//
//////////////////////////////////////////////////

int	TRH_exitB3(void)
{
			if(IS_TOP_TRANSPORT_SELECTED())
			{
				SET_MONITOR_TRANSPORT_STATUS(TOP_TRANSPORT_SEL);
			}
			else
			{
				SET_MONITOR_TRANSPORT_STATUS(BOTTOM_TRANSPORT_SEL);
			}

		SendMessage
			(MotorChipSetHandlerID, MotorChipSetReleaseResource);

	return TRH_FI_HM_WAIT_MOVE_LIMIT_COMPLETE;
}

//////////////////////////////////////////////////
//
//  MotorChipSetMotionComplete	
//		while in _TRH_FI_HM_WAIT_MOVE_LIMIT_COMPLETE
//
//////////////////////////////////////////////////

int	TRH_exitB4(void)
{
			// IMPORTANT:
			// Since Find home is not a standard move
			// Need this specific update here

			if(IS_TOP_TRANSPORT_SELECTED())
			{
				SetCurrentPosition
					(GetRequestedPosition(TOP_TRANSPORT_AXIS_SEL), 
						TOP_TRANSPORT_AXIS_SEL);
			}
			else
			{	
				SetCurrentPosition
					(GetRequestedPosition(BOTTOM_TRANSPORT_AXIS_SEL), 
						BOTTOM_TRANSPORT_AXIS_SEL);
			}

		StartTimer(GetMoveSettleWait());

	return TRH_FI_HM_WAIT_MOVE_LIMIT_SETTLE;
}

//////////////////////////////////////////////////
//
//  TimeOut
//		while in TRH_FI_HM_WAIT_MOVE_LIMIT_SETTLE
//
//////////////////////////////////////////////////

int	TRH_exitB5(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			PEND_TOP_TRANSP_HANDLER_REQUEST();
		}
		else
		{
			PEND_BOTTOM_TRANSP_HANDLER_REQUEST();
		}

	return TRH_FI_HM_MCS_REQ_2;
}

//////////////////////////////////////////////////
//
//  MotorChipSetResourceGranted
//		while in TRH_FI_HM_MCS_REQ_2
//
//////////////////////////////////////////////////

int	TRH_exitB6(void)
{
	MCS_SEND_GET_LIMIT_SWITCH_STATE();

		if(IS_TOP_TRANSPORT_SELECTED())
		{
			if(IS_TRANSP_MOVING_OUT_OF_NEGATIVE_LIMIT
				(TOP_TRANSPORT_SEL))
			{
				return TRH_FI_HM_SEND_GET_NLIM_SW_2;
			}
			else
			{
				return TRH_FI_HM_SEND_GET_PLIM_SW_2;
			}
		}
		else
		{
			if(IS_TRANSP_MOVING_OUT_OF_NEGATIVE_LIMIT
				(BOTTOM_TRANSPORT_SEL))
			{
				return TRH_FI_HM_SEND_GET_NLIM_SW_2;
			}
			else
			{
				return TRH_FI_HM_SEND_GET_PLIM_SW_2;
			}
		}
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent while in
//	TRH_FI_HM_SEND_GET_NLIM_SW_2
//
//	This state continues to move the transport 
//	until it is positioned outside of 
//	the limits
//
//////////////////////////////////////////////////

//
// This hanldles the NEGATIVE LIMIT
//

int	TRH_exitB7A(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			transportSelect = TOP_TRANSPORT_SEL;
			
			limitStatus[transportSelect] = READ_MOTOR_CHIPSET_DATA1();			
			
			if(IS_TOP_TRANSPORT_IN_NEG_LIM(transportSelect))
				transportLocation = TRANSPORT_IN_NEG_LIMIT;
			else
			if(IS_TOP_TRANSPORT_IN_POS_LIM(transportSelect))
				transportLocation = TRANSPORT_IN_POS_LIMIT;
			else
				transportLocation = TRANSPORT_IN_MIDDLE;
		}
		else
		{
			transportSelect = BOTTOM_TRANSPORT_SEL;
			
			limitStatus[transportSelect] = READ_MOTOR_CHIPSET_DATA1();	

			if(IS_BOTTOM_TRANSPORT_IN_NEG_LIM(transportSelect))
				transportLocation = TRANSPORT_IN_NEG_LIMIT;
			else
			if(IS_BOTTOM_TRANSPORT_IN_POS_LIM(transportSelect))
				transportLocation = TRANSPORT_IN_POS_LIMIT;
			else
				transportLocation = TRANSPORT_IN_MIDDLE;
		}

		SET_TP_AXIS(transportSelect);
				
		SET_TP_START_VELOCITY(GetStartingVelocity());
		SET_TP_MAX_VELOCITY(GetLocateSpeed());
		SET_TP_ACCELERATION(GetAcceleration());
		SET_TP_INTERRUPT_MASK(MCS_DEFAULT_INTERRUPT_MASK);

		if(transportLocation == TRANSPORT_IN_NEG_LIMIT)
		{
				// TRH_FI_HM_
				// In NEG Limit

				SetPositionRequest
					(GetCurrentPosition(transportSelect) + 
					TRANSPORT_MOVE_OUT_DISTANCE, transportSelect);

				targetPosition = GetRequestedPosition(transportSelect);

				SET_TP_TARGET_POSITION(targetPosition);
		
			SendMessage
				(TransportParamHandlerID, SendTransportProfileParams);
		
			return 
				TRH_FI_HM_SEND_MOVE_LIMIT_PROFILE;
		}
		else
		{
			// Out of NEGATIVE Limit !!!

			if(IsTransportNormal(transportSelect))
			{
					SetPositionRequest
						(GetCurrentPosition(transportSelect) + 
							GetHomeDistanceFromNearLimit(), transportSelect);

					targetPosition = GetRequestedPosition(transportSelect);

					SET_TP_TARGET_POSITION(targetPosition);

				SendMessage
						(TransportParamHandlerID, SendTransportProfileParams);
			
				return
					TRH_FI_HM_SEND_GOING_HOME_PROFILE;
			}
			else
			{
				// Do a Find Home while in the MIDDLE of the transport

				SendMessage
					(MotorChipSetHandlerID, MotorChipSetReleaseResource);

				SendMessage(THIS_SM, TransportFindHome);

				return TRH_ACTIVE;
			}
		}
}

//////////////////////////////////////////////////
//
// MotorChipSetMessageSent while in
//	TRH_FI_HM_SEND_GET_PLIM_SW_2
//
//	This state CONTINUES to move the transport 
//	until it is positioned outside of 
//	the limits
//
//////////////////////////////////////////////////

//
// This hanldles the POSITIVE LIMIT
//

int	TRH_exitB7B(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			transportSelect = TOP_TRANSPORT_SEL;
			
			limitStatus[transportSelect] = READ_MOTOR_CHIPSET_DATA1();			
			
			if(IS_TOP_TRANSPORT_IN_NEG_LIM(transportSelect))
				transportLocation = TRANSPORT_IN_NEG_LIMIT;
			else
			if(IS_TOP_TRANSPORT_IN_POS_LIM(transportSelect))
				transportLocation = TRANSPORT_IN_POS_LIMIT;
			else
				transportLocation = TRANSPORT_IN_MIDDLE;
		}
		else
		{
			transportSelect = BOTTOM_TRANSPORT_SEL;
			
			limitStatus[transportSelect] = READ_MOTOR_CHIPSET_DATA1();	

			if(IS_BOTTOM_TRANSPORT_IN_NEG_LIM(transportSelect))
				transportLocation = TRANSPORT_IN_NEG_LIMIT;
			else
			if(IS_BOTTOM_TRANSPORT_IN_POS_LIM(transportSelect))
				transportLocation = TRANSPORT_IN_POS_LIMIT;
			else
				transportLocation = TRANSPORT_IN_MIDDLE;
		}


		SET_TP_AXIS(transportSelect);

		SET_TP_TARGET_POSITION(targetPosition);
		SET_TP_START_VELOCITY(GetStartingVelocity());
		SET_TP_MAX_VELOCITY(GetLocateSpeed());
		SET_TP_ACCELERATION(GetAcceleration());
		SET_TP_INTERRUPT_MASK(MCS_DEFAULT_INTERRUPT_MASK);

		if(transportLocation == TRANSPORT_IN_POS_LIMIT)
		{				
				// TRH_FI_HM_
				// In POS Limit

				SetPositionRequest
					(GetCurrentPosition(transportSelect) - 
					TRANSPORT_MOVE_OUT_DISTANCE, transportSelect);

				targetPosition = GetRequestedPosition(transportSelect);
		
			SendMessage
				(TransportParamHandlerID, SendTransportProfileParams);
		
			return 
				TRH_FI_HM_SEND_MOVE_LIMIT_PROFILE;
		}
		else
		{
			// Out of POSITIVE Limit !!!

			if(IsTransportNormal(transportSelect))
			{
				// Do a Find Home while in the MIDDLE of the transport

				SendMessage
					(MotorChipSetHandlerID, MotorChipSetReleaseResource);

				SendMessage(THIS_SM, TransportFindHome);

				return TRH_ACTIVE;
			}			
			else
			{
					SetPositionRequest
						(GetCurrentPosition(transportSelect) - 
							GetHomeDistanceFromNearLimit(), transportSelect);

					targetPosition = GetRequestedPosition(transportSelect);

					SET_TP_TARGET_POSITION(targetPosition);

				SendMessage
						(TransportParamHandlerID, SendTransportProfileParams);
			
				return
					TRH_FI_HM_SEND_GOING_HOME_PROFILE;
			}
		}
}

//////////////////////////////////////////////////
//
//  TransportProfileParamsSent while in 
//	TRH_FI_HM_SEND_GOING_HOME_PROFILE
//
//////////////////////////////////////////////////

int	TRH_exitB8(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			SET_MONITOR_TRANSPORT_STATUS(TOP_TRANSPORT_AXIS_SEL);
		}
		else
		{
			SET_MONITOR_TRANSPORT_STATUS(BOTTOM_TRANSPORT_AXIS_SEL);
		}

		SendMessage
			(MotorChipSetHandlerID, MotorChipSetReleaseResource);

	return TRH_FI_HM_WAIT_GOING_HOME_COMPLETE;
}

//////////////////////////////////////////////////
//
//  MotorChipSetMotionComplete	while in 
//	TRH_FI_HM_WAIT_GOING_HOME_COMPLETE
//
//////////////////////////////////////////////////

int	TRH_exitB9(void)
{
		StartTimer(GetMoveSettleWait());

	return TRH_FI_HM_GOING_HOME_SETTLE;
}


//////////////////////////////////////////////////
//
//  TimeOut
//		while in TRH_FI_HM_GOING_HOME_SETTLE
//
//////////////////////////////////////////////////

int	TRH_exitB10(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			PEND_TOP_TRANSP_HANDLER_REQUEST();
		}
		else
		{
			PEND_BOTTOM_TRANSP_HANDLER_REQUEST();
		}

	return TRH_FI_HM_MCS_REQ_3;
}

//////////////////////////////////////////////////
//
// MotorChipSetResourceGranted while in 
//	TRH_FI_HM_MCS_REQ_3
//
//////////////////////////////////////////////////

int	TRH_exitB11A(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			MCS_SEND_SET_CURR_AXIS_1();
		}
		else
		{
			MCS_SEND_SET_CURR_AXIS_2();
		}

	return	TRH_FI_HM_SEND_ZERO_POS_2A;
}

//////////////////////////////////////////////////
//
//  MotorChipSetMessageSent while in 
//	TRH_FI_HM_SEND_ZERO_POS_2A
//
//////////////////////////////////////////////////

int	TRH_exitB11B(void)
{
		MCS_SEND_ZERO_POSITION();

	return	TRH_FI_HM_SEND_ZERO_POS_2B;
}

//////////////////////////////////////////////////
//
//  MotorChipSetMessageSent while in 
//	TRH_FI_HM_SEND_ZERO_POS_2B
//
//////////////////////////////////////////////////

int	TRH_exitB11C(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
			transportSelect = TOP_TRANSPORT_SEL;
		else
			transportSelect = BOTTOM_TRANSPORT_SEL;

			SetCurrentPosition
				(TRANSPORT_HOME_POSITION, transportSelect);
			
			SetPositionRequest
				(TRANSPORT_HOME_POSITION, transportSelect);

		MCS_SEND_LIMIT_SWITCH_SENSE_ON();

	return	TRH_FI_HM_SEND_ZERO_POS_2C;
}

//////////////////////////////////////////////////
//
//  MotorChipSetMessageSent while in 
//	TRH_FI_HM_SEND_ZERO_POS_2C
//
//////////////////////////////////////////////////

int	TRH_exitB12(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
			transportSelect = TOP_TRANSPORT_SEL;
		else
			transportSelect = BOTTOM_TRANSPORT_SEL;

			SET_TRANSPORT_CALIBRATED(transportSelect);

			CLEAR_TRANSPORT_IN_LIMIT(transportSelect);
			CLEAR_TRANSPORT_IN_POSITION_ERROR(transportSelect);

			CLEAR_MONITOR_TRANSPORT_STATUS(transportSelect);

			ClearTransportProcessDoneFlag(GetCurrSmId());

		SendMessage
			(THIS_SM, TransportReadTruePosition);

	return TRH_READ_TRUE_POS_1;
}


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//   MotorChipSetMessageSent
//		while in TRH_FI_HM_SEND_LIMIT_ON
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

int	TRH_exitB13A(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
			transportSelect = TOP_TRANSPORT_SEL;
		else
			transportSelect = BOTTOM_TRANSPORT_SEL;

			SET_TP_AXIS(transportSelect);
		
			// In Middle of Transport

				if(IsTransportNormal(transportSelect))
					targetPosition = TRANSPORT_HIT_LIM_POSITION;
				else
					targetPosition = (-TRANSPORT_HIT_LIM_POSITION);

				SET_TP_TARGET_POSITION(targetPosition);

				SET_TP_START_VELOCITY(GetStartingVelocity());
				SET_TP_MAX_VELOCITY(GetHitHomeLimitSpeed());
				SET_TP_ACCELERATION(GetAcceleration());
				SET_TP_INTERRUPT_MASK(MCS_DEFAULT_INTERRUPT_MASK);

			SendMessage
				(TransportParamHandlerID, SendTransportProfileParams);
		
		return
			TRH_FI_HM_SEND_HIT_NEG_LIM_PROFILES;
}

//////////////////////////////////////////////////
//
//  TransportProfileParamsSent 
//		while in 
//		TRH_FI_HM_SEND_HIT_NEG_LIM_PROFILES
//
//////////////////////////////////////////////////

int	TRH_exitB13B(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			SET_MONITOR_TRANSPORT_STATUS(TOP_TRANSPORT_AXIS_SEL);
		}
		else
		{
			SET_MONITOR_TRANSPORT_STATUS(BOTTOM_TRANSPORT_AXIS_SEL);
		}

		SendMessage
			(MotorChipSetHandlerID, MotorChipSetReleaseResource);

	return TRH_FI_HM_WAIT_HIT_NEG_LIM_COMPLETE;
}

//////////////////////////////////////////////////
//
//  MotorChipSetNegativeLimitHit while in 
//	TRH_FI_HM_WAIT_HIT_NEG_LIM_COMPLETE
//
//////////////////////////////////////////////////

int	TRH_exitB14(void)
{
		StartTimer(TRANSPORT_HIT_LIM_SETTLE_DELAY);

	return TRH_FI_HM_NEG_LIM_HIT_SETTLE;
}

//////////////////////////////////////////////////
//
//  TimeOut while in 
//	TRH_FI_HM_NEG_LIM_HIT_SETTLE
//
//////////////////////////////////////////////////

int	TRH_exitB15(void)
{
		// Find Home while in NEG Limit

		SendMessage(THIS_SM, TransportFindHome);

	return TRH_ACTIVE;
}

//////////////////////////////////////////////////
//
// TransportGoHome 
//		while in ACTIVE
//
//////////////////////////////////////////////////

int	TRH_exitC(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
			transportSelect = TOP_TRANSPORT_SEL;
		else
			transportSelect = BOTTOM_TRANSPORT_SEL;


			if(GetCurrentPosition(transportSelect) <= TRANSPORT_HOME_POSITION)
			{
				CLEAR_MONITOR_TRANSPORT_STATUS(transportSelect);				
				ClearTransportProcessDoneFlag(GetCurrSmId());
	
				return SAME_STATE;
			}
			
		if(transportSelect == TOP_TRANSPORT_SEL)
		{
			PEND_TOP_TRANSP_HANDLER_REQUEST();
		}
		else
		{
			PEND_BOTTOM_TRANSP_HANDLER_REQUEST();
		}
		
	return TRH_GO_HOME_MCS_REQ;
}


//////////////////////////////////////////////////
//
// MotorChipSetResourceGranted 
//		while in TRH_GO_HOME_MCS_REQ
//
//////////////////////////////////////////////////

int	TRH_exitC0(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
			transportSelect = TOP_TRANSPORT_SEL;
		else
			transportSelect = BOTTOM_TRANSPORT_SEL;
			
			SET_TP_AXIS(transportSelect);

				SetPositionRequest
					(TRANSPORT_HOME_POSITION, transportSelect);

					targetPosition = GetRequestedPosition(transportSelect);

			SET_TP_TARGET_POSITION(targetPosition);
			SET_TP_START_VELOCITY(GetStartingVelocity());
			SET_TP_MAX_VELOCITY(GetGoHomeSpeed());
			SET_TP_ACCELERATION(GetAcceleration());
			SET_TP_INTERRUPT_MASK(MCS_DEFAULT_INTERRUPT_MASK);

		SendMessage
			(TransportParamHandlerID, SendTransportProfileParams);
		
	return TRH_GO_HOME_SEND_PROFILE_PARAMS;
}


//////////////////////////////////////////////////
//
//  TransportProfileParamsSent
//		while in TRH_GO_HOME_SEND_PROFILE_PARAMS
//
//////////////////////////////////////////////////

int	TRH_exitC1(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			SET_MONITOR_TRANSPORT_STATUS(TOP_TRANSPORT_AXIS_SEL);
		}
		else
		{
			SET_MONITOR_TRANSPORT_STATUS(BOTTOM_TRANSPORT_AXIS_SEL);
		}

		SendMessage
			(MotorChipSetHandlerID, MotorChipSetReleaseResource);

	return TRH_GO_HOME_WAIT_MOTION_COMPLETE;
}


//////////////////////////////////////////////////
//
//  MotorChipSetMotionComplete
//		while in TRH_GO_HOME_WAIT_MOTION_COMPLETE
//
//////////////////////////////////////////////////

int	TRH_exitC2(void)
{
		StartTimer(GetMoveSettleWait());

	return TRH_GO_HOME_WAIT_SETTLE;
}


//////////////////////////////////////////////////
//
//  TimeOut
//		while in TRH_GO_HOME_WAIT_SETTLE
//
//////////////////////////////////////////////////

int	TRH_exitC3(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			PEND_TOP_TRANSP_HANDLER_REQUEST();
		}
		else
		{
			PEND_BOTTOM_TRANSP_HANDLER_REQUEST();
		}

	return TRH_READ_TRUE_POS_MCS_REQ;
}


//////////////////////////////////////////////////
//
// TransportGotoPosition 
//		while in ACTIVE
//
//////////////////////////////////////////////////

int	TRH_exitD(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
			transportSelect = TOP_TRANSPORT_SEL;
		else
			transportSelect = BOTTOM_TRANSPORT_SEL;


			if(GetRequestedPosition(transportSelect) ==
				GetCurrentPosition(transportSelect))
			{
				CLEAR_MONITOR_TRANSPORT_STATUS(transportSelect);				
				ClearTransportProcessDoneFlag(GetCurrSmId());
			
				return SAME_STATE;
			}
			
			// Check Direction

			if(GetRequestedPosition(transportSelect) > 
				GetCurrentPosition(transportSelect))
				SET_TRANSPORT_MOVING_FORWARD(transportSelect);
			else
				SET_TRANSPORT_MOVING_REVERSE(transportSelect);

			if(transportSelect == TOP_TRANSPORT_SEL)
			{
				PEND_TOP_TRANSP_HANDLER_REQUEST();
			}
			else
			{
				PEND_BOTTOM_TRANSP_HANDLER_REQUEST();
			}

	return TRH_GOTO_POS_MCS_REQ;
}


//////////////////////////////////////////////////
//
// MotorChipSetResourceGranted 
//		while in TRH_GOTO_POS_MCS_REQ
//
//////////////////////////////////////////////////

int	TRH_exitD0(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
			transportSelect = TOP_TRANSPORT_SEL;
		else
			transportSelect = BOTTOM_TRANSPORT_SEL;


			SET_TP_AXIS(transportSelect);

			if(IS_TRANSPORT_MOVING_FORWARD(transportSelect))
			{
				SetPositionRequest
					(GetRequestedPosition(transportSelect)+ 
						GetBacklashCount(), transportSelect);				
			}
			else
			{
				// Already moving reverse, no backlash

				SetPositionRequest
					(GetRequestedPosition(transportSelect), transportSelect);
			}

			if(IsTransportNormal(transportSelect))
				targetPosition = GetRequestedPosition(transportSelect);
			else
				targetPosition = (-GetRequestedPosition(transportSelect));

			SET_TP_TARGET_POSITION(targetPosition);
			SET_TP_START_VELOCITY(GetStartingVelocity());
			SET_TP_MAX_VELOCITY(GetLocateSpeed());
			SET_TP_ACCELERATION(GetAcceleration());
			SET_TP_INTERRUPT_MASK(MCS_DEFAULT_INTERRUPT_MASK);
		
		SendMessage
			(TransportParamHandlerID, SendTransportProfileParams);
		
	return TRH_GOTO_POS_SENDING_PROFILE_PARAMS;
}


//////////////////////////////////////////////////
//
//  TransportProfileParamsSent
//		while in TRH_GOTO_POS_SENDING_PROFILE_PARAMS
//
//////////////////////////////////////////////////

int	TRH_exitD1(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			SET_MONITOR_TRANSPORT_STATUS(TOP_TRANSPORT_AXIS_SEL);
		}
		else
		{
			SET_MONITOR_TRANSPORT_STATUS(BOTTOM_TRANSPORT_AXIS_SEL);
		}

		SendMessage
			(MotorChipSetHandlerID, MotorChipSetReleaseResource);

	return TRH_GOTO_POS_WAIT_MOTION_COMPLETE;
}


//////////////////////////////////////////////////
//
//  MotorChipSetMotionComplete
//		while in TRH_GOTO_POS_WAIT_MOTION_COMPLETE
//
//////////////////////////////////////////////////

int	TRH_exitD2(void)
{
		StartTimer(GetMoveSettleWait());

	return TRH_GOTO_POS_WAIT_SETTLE;
}


//////////////////////////////////////////////////
//
//  TimeOut
//		while in TRH_GOTO_POS_WAIT_SETTLE
//
//////////////////////////////////////////////////

int	TRH_exitD3(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
			transportSelect = TOP_TRANSPORT_SEL;
		else
			transportSelect = BOTTOM_TRANSPORT_SEL;


			if(IS_TRANSPORT_MOVING_FORWARD(transportSelect) && HasBacklash())
			{
					SendMessage(THIS_SM, TransportBacklashMove);

				return TRH_ACTIVE;
			}
			else
			{
				if(transportSelect == TOP_TRANSPORT_SEL)
				{
					PEND_TOP_TRANSP_HANDLER_REQUEST();
				}
				else
				{
					PEND_BOTTOM_TRANSP_HANDLER_REQUEST();
				}

				return TRH_READ_TRUE_POS_MCS_REQ;
			}
}


//////////////////////////////////////////////////
//
// TransportJog 
//		while in ACTIVE
//
//////////////////////////////////////////////////

int	TRH_exitE(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
			transportSelect = TOP_TRANSPORT_SEL;
		else
			transportSelect = BOTTOM_TRANSPORT_SEL;


			if(GetRequestedPosition(transportSelect) ==
				GetCurrentPosition(transportSelect))
			{
				CLEAR_MONITOR_TRANSPORT_STATUS(transportSelect);
				ClearTransportProcessDoneFlag(GetCurrSmId());

				return SAME_STATE;
			}
			
			// Check Direction

			if(GetRequestedPosition(transportSelect) >
				GetCurrentPosition(transportSelect))
				SET_TRANSPORT_MOVING_FORWARD(transportSelect);
			else
				SET_TRANSPORT_MOVING_REVERSE(transportSelect);

			if(transportSelect == TOP_TRANSPORT_SEL)
			{
				PEND_TOP_TRANSP_HANDLER_REQUEST();
			}
			else
			{
				PEND_BOTTOM_TRANSP_HANDLER_REQUEST();
			}
			
	return TRH_JOG_MCS_REQ;
}


//////////////////////////////////////////////////
//
// MotorChipSetResourceGranted 
//		while in TRH_JOG_MCS_REQ
//
//////////////////////////////////////////////////

int	TRH_exitE0(void)
{
	signed	int	jogDistance = 
					GetRequestedPosition(transportSelect);

		if(IS_TOP_TRANSPORT_SELECTED())
			transportSelect = TOP_TRANSPORT_SEL;
		else
			transportSelect = BOTTOM_TRANSPORT_SEL;


			SET_TP_AXIS(transportSelect);

			if(IS_TRANSPORT_MOVING_FORWARD(transportSelect))
			{
				SetPositionRequest
					(GetCurrentPosition(transportSelect) +
						jogDistance + GetBacklashCount(), transportSelect);				
			}
			else
			{
				// Already moving reverse, no backlash

				SetPositionRequest
					(GetCurrentPosition(transportSelect) +
					jogDistance, transportSelect);
			}

			if(IsTransportNormal(transportSelect))
				targetPosition = GetRequestedPosition(transportSelect);
			else
				targetPosition = (-GetRequestedPosition(transportSelect));

			SET_TP_TARGET_POSITION(targetPosition);
			SET_TP_START_VELOCITY(GetStartingVelocity());
			SET_TP_MAX_VELOCITY(GetJogSpeed());
			SET_TP_ACCELERATION(GetAcceleration());
			SET_TP_INTERRUPT_MASK(MCS_DEFAULT_INTERRUPT_MASK);
			
		SendMessage
			(TransportParamHandlerID, SendTransportProfileParams);
		
	return TRH_JOG_SENDING_PROFILE_PARAMS;
}


//////////////////////////////////////////////////
//
//  TransportProfileParamsSent
//		while in TRH_JOG_SENDING_PROFILE_PARAMS
//
//////////////////////////////////////////////////

int	TRH_exitE1(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			SET_MONITOR_TRANSPORT_STATUS(TOP_TRANSPORT_AXIS_SEL);
		}
		else
		{
			SET_MONITOR_TRANSPORT_STATUS(BOTTOM_TRANSPORT_AXIS_SEL);
		}

		SendMessage
			(MotorChipSetHandlerID, MotorChipSetReleaseResource);

	return TRH_JOG_WAIT_MOTION_COMPLETE;
}


//////////////////////////////////////////////////
//
//  MotorChipSetMotionComplete
//		while in TRH_JOG_WAIT_MOTION_COMPLETE
//
//////////////////////////////////////////////////

int	TRH_exitE2(void)
{
		StartTimer(GetMoveSettleWait());

	return TRH_JOG_WAIT_SETTLE;
}


//////////////////////////////////////////////////
//
//  TimeOut
//		while in TRH_JOG_WAIT_SETTLE
//
//////////////////////////////////////////////////

int	TRH_exitE3(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
			transportSelect = TOP_TRANSPORT_SEL;
		else
			transportSelect = BOTTOM_TRANSPORT_SEL;


			if(IS_TRANSPORT_MOVING_FORWARD(transportSelect) 
				&& HasBacklash())
			{
					SendMessage(THIS_SM, TransportBacklashMove);

				return TRH_ACTIVE;
			}
			else
			{
				if(transportSelect == TOP_TRANSPORT_SEL)
				{
					PEND_TOP_TRANSP_HANDLER_REQUEST();
				}
				else
				{
					PEND_BOTTOM_TRANSP_HANDLER_REQUEST();
				}

				return TRH_READ_TRUE_POS_MCS_REQ;
			}
}


//////////////////////////////////////////////////
//
// TransportBacklashMove 
//		while in ACTIVE
//
//////////////////////////////////////////////////

int	TRH_exitF(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{	
			PEND_TOP_TRANSP_HANDLER_REQUEST();
		}
		else
		{
			PEND_BOTTOM_TRANSP_HANDLER_REQUEST();
		}
		
	return TRH_BACKLASH_MCS_REQ;
}

//////////////////////////////////////////////////
//
// MotorChipSetResourceGranted 
//		while in TRH_BACKLASH_MCS_REQ
//
//////////////////////////////////////////////////

int	TRH_exitF0(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
			transportSelect = TOP_TRANSPORT_SEL;
		else
			transportSelect = BOTTOM_TRANSPORT_SEL;


			SET_TP_AXIS(transportSelect);
			
			SetPositionRequest
				(GetCurrentPosition(transportSelect) - 
					GetBacklashCount(), transportSelect);				

			if(IsTransportNormal(transportSelect))
				targetPosition = GetRequestedPosition(transportSelect);
			else
				targetPosition = (-GetRequestedPosition(transportSelect));

			SET_TP_TARGET_POSITION(targetPosition);		

			SET_TP_START_VELOCITY(GetStartingVelocity());
			SET_TP_MAX_VELOCITY(GetBacklashSpeed());
			SET_TP_ACCELERATION(GetAcceleration());
			SET_TP_INTERRUPT_MASK(MCS_DEFAULT_INTERRUPT_MASK);

		SendMessage
			(TransportParamHandlerID, SendTransportProfileParams);
		
	return TRH_BACKLASH_SENDING_PROFILE_PARAMS;
}

//////////////////////////////////////////////////
//
//  TransportProfileParamsSent
//		while in TRH_BACKLASH_SENDING_PROFILE_PARAMS
//
//////////////////////////////////////////////////

int	TRH_exitF1(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			SET_MONITOR_TRANSPORT_STATUS(TOP_TRANSPORT_AXIS_SEL);
		}
		else
		{
			SET_MONITOR_TRANSPORT_STATUS(BOTTOM_TRANSPORT_AXIS_SEL);
		}

		SendMessage
			(MotorChipSetHandlerID, MotorChipSetReleaseResource);

	return TRH_BACKLASH_WAIT_MOTION_COMPLETE;
}


//////////////////////////////////////////////////
//
//  MotorChipSetMotionComplete
//		while in TRH_BACKLASH_WAIT_MOTION_COMPLETE
//
//////////////////////////////////////////////////

int	TRH_exitF2(void)
{
		StartTimer(GetMoveSettleWait());	

	return TRH_BACKLASH_WAIT_SETTLE;
}

//////////////////////////////////////////////////
//
//  TimeOut
//		while in TRH_BACKLASH_WAIT_SETTLE
//
//////////////////////////////////////////////////

int	TRH_exitF3(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			PEND_TOP_TRANSP_HANDLER_REQUEST();
		}
		else
		{
			PEND_BOTTOM_TRANSP_HANDLER_REQUEST();
		}

	return TRH_READ_TRUE_POS_MCS_REQ;
}

//////////////////////////////////////////////////
//
//  MotorChipSetNegativeLimitHit	
//	MotorChipSetPositiveLimitHit
//
//		while in _WAIT_MOTION_COMPLETE
//
//////////////////////////////////////////////////

int	TRH_exitL(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
			transportSelect = TOP_TRANSPORT_SEL;
		else
			transportSelect = BOTTOM_TRANSPORT_SEL;

		SET_TRANSPORT_IN_LIMIT(transportSelect);
		CLEAR_MONITOR_TRANSPORT_STATUS(transportSelect);
		ClearTransportProcessDoneFlag(GetCurrSmId());

	return TRH_ACTIVE;
}

//////////////////////////////////////////////////
//
//	TransportNotReadyDetected
//		while in _WAIT_MOTION_COMPLETE
//
//////////////////////////////////////////////////

int	TRH_exitM0(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			transportSelect = TOP_TRANSPORT_SEL;
			HOLD_TOP_TRANSPORT_RESET();
		}
		else
		{
			transportSelect = BOTTOM_TRANSPORT_SEL;
			HOLD_BOTTOM_TRANSPORT_RESET();
		}

		SET_TRANSPORT_NOT_READY(transportSelect);
		CLEAR_MONITOR_TRANSPORT_STATUS(transportSelect);

	return TRH_WAIT_PULSE_GEN_COMPLETE;
}

//////////////////////////////////////////////////
//
//  MotorChipSetMotionComplete
//		while in TRH_WAIT_PULSE_GEN_COMPLETE
//
//////////////////////////////////////////////////

int	TRH_exitM1(void)
{
		ClearTransportProcessDoneFlag(GetCurrSmId());

	return TRH_ACTIVE;
}


//////////////////////////////////////////////////
//
//	MotorChipSetPositionError
//		while in _WAIT_MOTION_COMPLETE
//
//////////////////////////////////////////////////

int	TRH_exitP0(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{			
			PEND_TOP_TRANSP_HANDLER_REQUEST();
		}
		else
		{
			PEND_BOTTOM_TRANSP_HANDLER_REQUEST();
		}

	return TRH_POS_ERR_MCS_REQ;
}

//////////////////////////////////////////////////
//
// MotorChipSetResourceGranted 
//		while in TRH_POS_ERR_MCS_REQ
//
//////////////////////////////////////////////////

int	TRH_exitP1(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			MCS_SEND_SET_CURR_AXIS_1();
		}
		else
		{
			MCS_SEND_SET_CURR_AXIS_2();
		}

	return TRH_POS_ERR_SET_AXIS;
}

//////////////////////////////////////////////////
//
//  MotorChipSetMessageSent
//		while in TRH_POS_ERR_SET_AXIS
//
//////////////////////////////////////////////////

int	TRH_exitP2(void)
{
		MCS_SEND_SYNCH_PROFILE();

	return TRH_POS_ERR_SEND_SYNCH_PROF;
}

//////////////////////////////////////////////////
//
//  MotorChipSetMessageSent
//		while in TRH_POS_ERR_SEND_SYNCH_PROF
//
//////////////////////////////////////////////////

int	TRH_exitP3(void)
{
		MCS_SEND_MOTOR_ON();

	return TRH_POS_ERR_SEND_MOTOR_ON;
}

//////////////////////////////////////////////////
//
//	MotorChipSetMessageSent
//		while in TRH_POS_ERR_SEND_MOTOR_ON
//
//////////////////////////////////////////////////

int	TRH_exitP4(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
			transportSelect = TOP_TRANSPORT_SEL;
		else
			transportSelect = BOTTOM_TRANSPORT_SEL;


			SET_TRANSPORT_IN_POSITION_ERROR(transportSelect);
			CLEAR_MONITOR_TRANSPORT_STATUS(transportSelect);
			ClearTransportProcessDoneFlag(GetCurrSmId());

		SendMessage
			(MotorChipSetHandlerID, MotorChipSetReleaseResource);

	return TRH_ACTIVE;
}

//////////////////////////////////////////////////
//
//	MotorChipSetResourceGranted
//		while in TRH_READ_TRUE_POS_MCS_REQ
//
//////////////////////////////////////////////////

int	TRH_exitR0(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
		{
			MCS_SEND_SET_CURR_AXIS_1();
		}
		else
		{
			MCS_SEND_SET_CURR_AXIS_2();
		}

	return TRH_READ_TRUE_POS_1;
}

//////////////////////////////////////////////////
//
//	MotorChipSetMessageSent
//		while in TRH_READ_TRUE_POS_1
//
//////////////////////////////////////////////////

int	TRH_exitR1(void)
{
		MCS_SEND_GET_ACTUAL_AXIS_POS();

	return TRH_READ_TRUE_POS_2;
}

//////////////////////////////////////////////////
//
//	MotorChipSetMessageSent
//		while in TRH_READ_TRUE_POS_2
//
//////////////////////////////////////////////////

int	TRH_exitR2(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
			transportSelect = TOP_TRANSPORT_SEL;
		else
			transportSelect = BOTTOM_TRANSPORT_SEL;

			// As set by the CCU

			SetCurrentPosition
				(GetRequestedPosition(transportSelect), transportSelect);

			// As recorded by the MCS encoder, after the move

			SetTruePosition
				(READ_MOTOR_CHIPSET_DATA_DWORD(), transportSelect);

		MCS_SEND_GET_ACTUAL_POSITION_ERROR();

	return TRH_READ_TRUE_POS_3;
}


//////////////////////////////////////////////////
//
//	MotorChipSetMessageSent
//		while in TRH_READ_TRUE_POS_3
//
//////////////////////////////////////////////////

int	TRH_exitR3(void)
{
		if(IS_TOP_TRANSPORT_SELECTED())
			transportSelect = TOP_TRANSPORT_SEL;
		else
			transportSelect = BOTTOM_TRANSPORT_SEL;

			SetPositionErrorCount
				(READ_MOTOR_CHIPSET_DATA1(), transportSelect);

			CLEAR_MONITOR_TRANSPORT_STATUS(transportSelect);
			ClearTransportProcessDoneFlag(GetCurrSmId());

		SendMessage
			(MotorChipSetHandlerID, MotorChipSetReleaseResource);

	return TRH_ACTIVE;
}


/////////////////////////////////////////////////
//
// STATE MACHINE MATRIX DEFINITIONS
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_TRH_IDLE)
	EV_HANDLER(GoActive, TRH_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_INIT_0)
	EV_HANDLER(TimeOut, TRH_exitA0)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_INIT_1)
	EV_HANDLER(TimeOut, TRH_exitA1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_INIT_2)
	EV_HANDLER(MotorChipSetResourceGranted, TRH_exitA2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_INIT_3)
	EV_HANDLER(MotorChipSetMessageSent, TRH_exitA3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_INIT_4)
	EV_HANDLER(MotorChipSetMessageSent, TRH_exitA4)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_GO_HOME_MCS_REQ)
	EV_HANDLER(MotorChipSetResourceGranted, TRH_exitC0)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_GO_HOME_SEND_PROFILE_PARAMS)
	EV_HANDLER(TransportProfileParamsSent, TRH_exitC1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_GO_HOME_WAIT_MOTION_COMPLETE)
	EV_HANDLER(MotorChipSetMotionComplete, TRH_exitC2),
	EV_HANDLER(MotorChipSetNegativeLimitHit, TRH_exitL),
	EV_HANDLER(MotorChipSetPositiveLimitHit, TRH_exitL),
	EV_HANDLER(TransportNotReadyDetected, TRH_exitM0),
	EV_HANDLER(MotorChipSetPositionError, TRH_exitP0)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_GO_HOME_WAIT_SETTLE)
	EV_HANDLER(TimeOut, TRH_exitC3)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_TRH_GOTO_POS_MCS_REQ)
	EV_HANDLER(MotorChipSetResourceGranted, TRH_exitD0)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_GOTO_POS_SENDING_PROFILE_PARAMS)
	EV_HANDLER(TransportProfileParamsSent, TRH_exitD1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_GOTO_POS_WAIT_MOTION_COMPLETE)
	EV_HANDLER(MotorChipSetMotionComplete, TRH_exitD2),
	EV_HANDLER(MotorChipSetNegativeLimitHit, TRH_exitL),
	EV_HANDLER(MotorChipSetPositiveLimitHit, TRH_exitL),
	EV_HANDLER(TransportNotReadyDetected, TRH_exitM0),
	EV_HANDLER(MotorChipSetPositionError, TRH_exitP0)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_GOTO_POS_WAIT_SETTLE)
	EV_HANDLER(TimeOut, TRH_exitD3)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_TRH_JOG_MCS_REQ)
	EV_HANDLER(MotorChipSetResourceGranted, TRH_exitE0)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_JOG_SENDING_PROFILE_PARAMS)
	EV_HANDLER(TransportProfileParamsSent, TRH_exitE1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_JOG_WAIT_MOTION_COMPLETE)
	EV_HANDLER(MotorChipSetMotionComplete, TRH_exitE2),
	EV_HANDLER(MotorChipSetNegativeLimitHit, TRH_exitL),
	EV_HANDLER(MotorChipSetPositiveLimitHit, TRH_exitL),
	EV_HANDLER(TransportNotReadyDetected, TRH_exitM0),
	EV_HANDLER(MotorChipSetPositionError, TRH_exitP0)	
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_JOG_WAIT_SETTLE)
	EV_HANDLER(TimeOut, TRH_exitE3)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_TRH_BACKLASH_MCS_REQ)
	EV_HANDLER(MotorChipSetResourceGranted, TRH_exitF0)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_BACKLASH_SENDING_PROFILE_PARAMS)
	EV_HANDLER(TransportProfileParamsSent, TRH_exitF1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_BACKLASH_WAIT_MOTION_COMPLETE)
	EV_HANDLER(MotorChipSetMotionComplete, TRH_exitF2),
	EV_HANDLER(MotorChipSetNegativeLimitHit, TRH_exitL),
	EV_HANDLER(MotorChipSetPositiveLimitHit, TRH_exitL),
	EV_HANDLER(TransportNotReadyDetected, TRH_exitM0),
	EV_HANDLER(MotorChipSetPositionError, TRH_exitP0)	
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_BACKLASH_WAIT_SETTLE)
	EV_HANDLER(TimeOut, TRH_exitF3)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_TRH_FI_HM_MCS_REQ_1)
	EV_HANDLER(MotorChipSetResourceGranted, TRH_exitB0A)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_SEND_ZERO_POS_1A)
	EV_HANDLER(MotorChipSetMessageSent, TRH_exitB0B)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_SEND_ZERO_POS_1B)
	EV_HANDLER(MotorChipSetMessageSent, TRH_exitB1B)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_SEND_LIMIT_OFF)
	EV_HANDLER(MotorChipSetMessageSent, TRH_exitB1C)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_SEND_GET_LIM_SW_1)
	EV_HANDLER(MotorChipSetMessageSent, TRH_exitB2)
STATE_TRANSITION_MATRIX_END;
	
STATE_TRANSITION_MATRIX(_TRH_FI_HM_SEND_MOVE_LIMIT_PROFILE)
	EV_HANDLER(TransportProfileParamsSent, TRH_exitB3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_WAIT_MOVE_LIMIT_COMPLETE)
	EV_HANDLER(MotorChipSetMotionComplete, TRH_exitB4),
	EV_HANDLER(TransportNotReadyDetected, TRH_exitM0),
	EV_HANDLER(MotorChipSetPositionError, TRH_exitP0)	
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_WAIT_MOVE_LIMIT_SETTLE)
	EV_HANDLER(TimeOut, TRH_exitB5)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_MCS_REQ_2)
	EV_HANDLER(MotorChipSetResourceGranted, TRH_exitB6)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_SEND_GET_NLIM_SW_2)
	EV_HANDLER(MotorChipSetMessageSent, TRH_exitB7A)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_SEND_GET_PLIM_SW_2)
	EV_HANDLER(MotorChipSetMessageSent, TRH_exitB7B)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_SEND_GOING_HOME_PROFILE)
	EV_HANDLER(TransportProfileParamsSent, TRH_exitB8)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_WAIT_GOING_HOME_COMPLETE)
	EV_HANDLER(MotorChipSetMotionComplete, TRH_exitB9),
	EV_HANDLER(TransportNotReadyDetected, TRH_exitM0),
	EV_HANDLER(MotorChipSetPositionError, TRH_exitP0)	
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_GOING_HOME_SETTLE)
	EV_HANDLER(TimeOut, TRH_exitB10)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_MCS_REQ_3)
	EV_HANDLER(MotorChipSetResourceGranted, TRH_exitB11A)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_SEND_ZERO_POS_2A)
	EV_HANDLER(MotorChipSetMessageSent, TRH_exitB11B)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_SEND_ZERO_POS_2B)
	EV_HANDLER(MotorChipSetMessageSent, TRH_exitB11C)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_SEND_ZERO_POS_2C)
	EV_HANDLER(MotorChipSetMessageSent, TRH_exitB12)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_SEND_LIMIT_ON)
	EV_HANDLER(MotorChipSetMessageSent, TRH_exitB13A)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_SEND_HIT_NEG_LIM_PROFILES)
	EV_HANDLER(TransportProfileParamsSent, TRH_exitB13B)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_WAIT_HIT_NEG_LIM_COMPLETE)
	EV_HANDLER(MotorChipSetNegativeLimitHit, TRH_exitB14),
	EV_HANDLER(MotorChipSetMotionComplete, TRH_exitB14),
	EV_HANDLER(MotorChipSetPositiveLimitHit, TRH_exitB14)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_FI_HM_NEG_LIM_HIT_SETTLE)
	EV_HANDLER(TimeOut, TRH_exitB15)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_TRH_POS_ERR_MCS_REQ)
	EV_HANDLER(MotorChipSetResourceGranted, TRH_exitP1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_POS_ERR_SET_AXIS)
	EV_HANDLER(MotorChipSetMessageSent, TRH_exitP2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_POS_ERR_SEND_SYNCH_PROF)
	EV_HANDLER(MotorChipSetMessageSent, TRH_exitP3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_POS_ERR_SEND_MOTOR_ON)
	EV_HANDLER(MotorChipSetMessageSent, TRH_exitP4)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_TRH_READ_TRUE_POS_MCS_REQ)
	EV_HANDLER(MotorChipSetResourceGranted, TRH_exitR0)	
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_READ_TRUE_POS_1)
	EV_HANDLER(MotorChipSetMessageSent, TRH_exitR1)	,
	EV_HANDLER(TransportReadTruePosition, TRH_exitR1)	
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_READ_TRUE_POS_2)
	EV_HANDLER(MotorChipSetMessageSent, TRH_exitR2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_READ_TRUE_POS_3)
	EV_HANDLER(MotorChipSetMessageSent, TRH_exitR3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_WAIT_PULSE_GEN_COMPLETE)
	EV_HANDLER(MotorChipSetMotionComplete, TRH_exitM1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TRH_ACTIVE)
	EV_HANDLER(TransportJog, TRH_exitE)					,
	EV_HANDLER(TransportGotoPosition, TRH_exitD)		,
	EV_HANDLER(TransportFindHome, TRH_exitB)			,
	EV_HANDLER(TransportGoHome, TRH_exitC)				,
	EV_HANDLER(TransportBacklashMove, TRH_exitF)		,
	EV_HANDLER(MotorChipSetNegativeLimitHit, TRH_exitL)	,
	EV_HANDLER(MotorChipSetPositiveLimitHit, TRH_exitL)	,
	EV_HANDLER(TransportNotReadyDetected, TRH_exitM0)	,
	EV_HANDLER(MotorChipSetPositionError, TRH_exitP0)	,
	EV_HANDLER(TransportReset, TRH_exitA)				
STATE_TRANSITION_MATRIX_END;



// 
// VERY IMPORTANT : 
//		State Entry definition order MUST match the 
//		order of the state definition in the .H File 
//

SM_RESPONSE_ENTRY(TRH_Entry)
	STATE(_TRH_IDLE)						,			// Id = 0
	STATE(_TRH_ACTIVE)						,

	STATE(_TRH_JOG_MCS_REQ)						,
	STATE(_TRH_JOG_SENDING_PROFILE_PARAMS)		,
	STATE(_TRH_JOG_WAIT_MOTION_COMPLETE)		,	
	STATE(_TRH_JOG_WAIT_SETTLE)					,

	STATE(_TRH_GOTO_POS_MCS_REQ)					,
	STATE(_TRH_GOTO_POS_SENDING_PROFILE_PARAMS)		,
	STATE(_TRH_GOTO_POS_WAIT_MOTION_COMPLETE)		,	
	STATE(_TRH_GOTO_POS_WAIT_SETTLE)				,	

	STATE(_TRH_BACKLASH_MCS_REQ)					,	// Id = 10
	STATE(_TRH_BACKLASH_SENDING_PROFILE_PARAMS)		,
	STATE(_TRH_BACKLASH_WAIT_MOTION_COMPLETE)		,
	STATE(_TRH_BACKLASH_WAIT_SETTLE)				,

	STATE(_TRH_GO_HOME_MCS_REQ)						,
	STATE(_TRH_GO_HOME_SEND_PROFILE_PARAMS)			,
	STATE(_TRH_GO_HOME_WAIT_MOTION_COMPLETE)		,	
	STATE(_TRH_GO_HOME_WAIT_SETTLE)					,

	STATE(_TRH_FI_HM_MCS_REQ_1)						,
	STATE(_TRH_FI_HM_SEND_ZERO_POS_1A)				,
	STATE(_TRH_FI_HM_SEND_ZERO_POS_1B)				,	// Id = 20
	STATE(_TRH_FI_HM_SEND_LIMIT_OFF)				,
	STATE(_TRH_FI_HM_SEND_GET_LIM_SW_1)				,

	STATE(_TRH_FI_HM_SEND_MOVE_LIMIT_PROFILE)		,	
	STATE(_TRH_FI_HM_WAIT_MOVE_LIMIT_COMPLETE)		,
	STATE(_TRH_FI_HM_WAIT_MOVE_LIMIT_SETTLE)		,
	STATE(_TRH_FI_HM_MCS_REQ_2)						,
	STATE(_TRH_FI_HM_SEND_GET_NLIM_SW_2)			,
	STATE(_TRH_FI_HM_SEND_GET_PLIM_SW_2)			,

	STATE(_TRH_FI_HM_SEND_GOING_HOME_PROFILE)		,
	STATE(_TRH_FI_HM_WAIT_GOING_HOME_COMPLETE)		,
	STATE(_TRH_FI_HM_GOING_HOME_SETTLE)				,

	STATE(_TRH_FI_HM_MCS_REQ_3)						,	
	STATE(_TRH_FI_HM_SEND_ZERO_POS_2A)				,
	STATE(_TRH_FI_HM_SEND_ZERO_POS_2B)				,
	STATE(_TRH_FI_HM_SEND_ZERO_POS_2C)				,

	STATE(_TRH_FI_HM_SEND_LIMIT_ON)					,
	STATE(_TRH_FI_HM_SEND_HIT_NEG_LIM_PROFILES)		,
	STATE(_TRH_FI_HM_WAIT_HIT_NEG_LIM_COMPLETE)		,
	STATE(_TRH_FI_HM_NEG_LIM_HIT_SETTLE)			,

	STATE(_TRH_POS_ERR_MCS_REQ)					,
	STATE(_TRH_POS_ERR_SET_AXIS)				,
	STATE(_TRH_POS_ERR_SEND_SYNCH_PROF)			,
	STATE(_TRH_POS_ERR_SEND_MOTOR_ON)			,

	STATE(_TRH_READ_TRUE_POS_MCS_REQ)			,
	STATE(_TRH_READ_TRUE_POS_1)					,
	STATE(_TRH_READ_TRUE_POS_2)					,
	STATE(_TRH_READ_TRUE_POS_3)					,

	STATE(_TRH_WAIT_PULSE_GEN_COMPLETE)			,

	STATE(_TRH_INIT_0)					,
	STATE(_TRH_INIT_1)					,			
	STATE(_TRH_INIT_2)					,
	STATE(_TRH_INIT_3)					,
	STATE(_TRH_INIT_4)			
SM_RESPONSE_END




