



/*
 *
 *
 *		$Header:   K:/Projects/Tcmr/Source/Motorcs.c_v   1.2   Apr 08 2002 14:31:42   PaulLC  $
 *		$Log:   K:/Projects/Tcmr/Source/Motorcs.c_v  $
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

#include "motorcs.h"

#include "pccomm.h"



//////////////////////////////////////////////////
//
// Private Data
//
//////////////////////////////////////////////////

BYTE	xdata	motorChipSetDataPort		_at_	0x0600;

BYTE	xdata	motorChipSetCommandPort		_at_	0x0601;


BYTE	near	currMotorChipSetMsgType;

BYTE	near	hostReadyRetryCount;

WORD	near	mcsCommandChecksum;
WORD	near	motorChipSetChecksum;

BYTE	near	mCSchecksumErrorCount;
BYTE	near	currMCSmsgTypeErrorCount;

WORD	near	motorChipSetVersion;


BOOL	near	mcsInterruptPending;

BOOL	near	topTransportHandlerRequesting;

BOOL	near	bottomTransportHandlerRequesting;

BOOL	near	pressEventHandlerRequesting;


STATE_MACHINE_ID	near	mcsOwner;

#define		SET_MCS_OWNER(mCSO)			(mcsOwner = mCSO)
#define		GET_MCS_OWNER()				(mcsOwner)
#define		SET_MCS_AVAILABLE()			(mcsOwner = NULL_SM_ID)
#define		IS_MCS_AVAILABLE()			(mcsOwner == NULL_SM_ID)


MotorChipSetMessage		near	motorChipSetMessageBuffer;


#define		MCS_MAX_HOST_READY_RETRY_COUNT		5



//////////////////////////////////////////////////
//
// MCSH Data Access Functions
//
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
// MCSH Private Functions
//
//////////////////////////////////////////////////


#define		WRITE_MOTOR_CHIPSET_COMMAND(mcsCmd)		\
				(motorChipSetCommandPort = mcsCmd)

void	WriteMotorChipSetCommand(BYTE command)
{
	WRITE_MOTOR_CHIPSET_COMMAND(command);

	mcsCommandChecksum += command;
}


#define		WRITE_MOTOR_CHIPSET_DATA(mcsDat)		\
				(motorChipSetDataPort = mcsDat)

void	WriteMotorChipSetData(WORD dataWord)			
{
	WRITE_MOTOR_CHIPSET_DATA(HIBYTE(dataWord));
	WRITE_MOTOR_CHIPSET_DATA(LOBYTE(dataWord));

	mcsCommandChecksum += dataWord;
}

BYTE	ForceReadMotorChipSetData(void)		
{
	return	motorChipSetDataPort;
}

WORD	ReadMotorChipSetData(void)				
{
	WORD dataRead;

	dataRead =
		(((WORD)ForceReadMotorChipSetData() << 8 ) |
		  (WORD)ForceReadMotorChipSetData());

		mcsCommandChecksum += dataRead;

	return dataRead;
}

WORD	ReadMotorChipSetCheckSum(void)				
{
	return
		(((WORD)ForceReadMotorChipSetData() << 8 ) |
		  (WORD)ForceReadMotorChipSetData());
}

#define		RESET_MCS_COMMAND_CHECKSUM()	\
				(mcsCommandChecksum = 0x0000)


void	IncMCSchecksumErrorCount(void)
{
	SetMCSchecksumErrorCountLog(++mCSchecksumErrorCount);
}

//////////////////////////////////////////////////
//
// Exit Procedures
//
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
// GoReset
//	while in IDLE
//
//////////////////////////////////////////////////

int	MCSH_exitA(void)
{
			MCS_CLEAR_COMMAND_BUFFER();

			SET_MCS_AVAILABLE();

			CLEAR_MCS_INTERRUPT_REQUEST();
			CLEAR_TOP_TRANSP_HANDLER_REQUEST();
			CLEAR_BOTTOM_TRANSP_HANDLER_REQUEST();
			CLEAR_PRESS_EVENT_HANDLER_REQUEST();

			HOLD_STEPPER_MOTOR_CHIPSET_RESET();

			mCSchecksumErrorCount = 0;
			currMCSmsgTypeErrorCount = 0;

		StartTimer(MOTOR_CHIPSET_RESET_HOLD);

	return MCSH_RESET_WAIT_1;
}

//////////////////////////////////////////////////
//
// TimeOut
//
//////////////////////////////////////////////////

int	MCSH_exitA2(void)
{
		// HARD RESET : 

		RELEASE_STEPPER_MOTOR_CHIPSET_RESET();

		StartTimer(MOTOR_CHIPSET_DELAY_WAIT);

	return MCSH_RESET_WAIT_2;
}

//////////////////////////////////////////////////
//
// TimeOut
//  while in MCSH_RESET_WAIT_2
//
//////////////////////////////////////////////////

int	MCSH_exitA3(void)
{
		SendLowPriorityMessage(TCMR_SystemManagerID, MotorChipSetHardResetDone);

	return MCSH_ACTIVE;
}


//////////////////////////////////////////////////
//
// MotorChipSetRequestResource 
//	while in ACTIVE
//
//////////////////////////////////////////////////

int	MCSH_exitB(void)
{
	if(IS_MCS_AVAILABLE())
	{
		if(IS_MCS_INTERRUPT_REQUESTING())
		{
			SET_MCS_OWNER(MotorChipSetInterruptHandlerID);

			SendLowPriorityMessage
				(MotorChipSetInterruptHandlerID, MotorChipSetResourceGranted);
		}
		else
		if(IS_TOP_TRANSP_HANDLER_REQUESTING())
		{
			SET_MCS_OWNER(TopTransportHandlerID);

			SendLowPriorityMessage
				(TopTransportHandlerID, MotorChipSetResourceGranted);
		}
		else
		if(IS_BOTTOM_TRANSP_HANDLER_REQUESTING())
		{
			SET_MCS_OWNER(BottomTransportHandlerID);

			SendLowPriorityMessage
				(BottomTransportHandlerID, MotorChipSetResourceGranted);
		}
		else
		if(IS_PRESS_EVENT_HANDLER_REQUESTING())
		{
			SET_MCS_OWNER(PressEventMonitorID);

			SendLowPriorityMessage
				(PressEventMonitorID, MotorChipSetResourceGranted);
		}
	}

	return MCSH_ACTIVE;
}

//////////////////////////////////////////////////
//
// MotorChipSetReleaseResource 
//	while in ACTIVE
//
//////////////////////////////////////////////////

int	MCSH_exitB1(void)
{
		switch(GET_MCS_OWNER())
		{
			// Clear the owner

			case MotorChipSetInterruptHandlerID:
					CLEAR_MCS_INTERRUPT_REQUEST();
				break;

			case TopTransportHandlerID:
					CLEAR_TOP_TRANSP_HANDLER_REQUEST();
				break;

			case BottomTransportHandlerID:
					CLEAR_BOTTOM_TRANSP_HANDLER_REQUEST();
				break;

			case PressEventMonitorID:
					CLEAR_PRESS_EVENT_HANDLER_REQUEST();
				break;

			default:
				break;
		}
		
		if(IS_TOP_TRANSP_HANDLER_REQUESTING()		| 
			IS_BOTTOM_TRANSP_HANDLER_REQUESTING()	|
			IS_MCS_INTERRUPT_REQUESTING()			|
			IS_PRESS_EVENT_HANDLER_REQUESTING())
		{
			SendLowPriorityMessage(THIS_SM, MotorChipSetRequestResource);
		}

		// Chipset is now available

		SET_MCS_AVAILABLE();

	return MCSH_ACTIVE;
}

//////////////////////////////////////////////////
//
// SendMotorChipSetCommand
//		while in MCSH_ACTIVE
//
//////////////////////////////////////////////////

int	MCSH_exitC(void)
{
			RESET_MCS_COMMAND_CHECKSUM();

			currMotorChipSetMsgType = motorChipSetMessageBuffer.msgType;
		
			if(IS_MOTOR_CHIPSET_HOST_READY())
			{
				WriteMotorChipSetCommand(motorChipSetMessageBuffer.cmd);
				
				switch(currMotorChipSetMsgType)
				{
					case	MOTORCS_MSG_TYPE_WRITE_CMD:
						
							if(motorChipSetMessageBuffer.cmd == RESET_CHIPSET)
							{
								SendLowPriorityMessage
									(motorChipSetMessageBuffer.requestorId, 
										MotorChipSetSoftResetDone);

								MCS_CLEAR_COMMAND_BUFFER();

								return MCSH_ACTIVE;							
							}
							else
							{
								SendLowPriorityMessage(THIS_SM, MotorChipSetCommandSent);

								return MCSH_ACTIVE_LAST_DATA_TRANSACTED;
							}

					case	MOTORCS_MSG_TYPE_WRITE_WORD:
					case	MOTORCS_MSG_TYPE_WRITE_DWORD:
					case	MOTORCS_MSG_TYPE_READ_WORD:
					case	MOTORCS_MSG_TYPE_READ_DWORD:
							
							SendLowPriorityMessage(THIS_SM, MotorChipSetCommandSent);

							return MCSH_ACTIVE_SENDING_CMD;

					default:
						
						// ERROR SHOULD NOT
						// BE REACHED
						return SAME_STATE;
				}
			}

		hostReadyRetryCount = 1;

		SendLowPriorityMessage(THIS_SM, RecheckHostReadyLine);

	return MCSH_ACTIVE_WAIT_CMD_DELAYED_HREADY;
}

//////////////////////////////////////////////////
//
// Timeout - While delayed and
//				waiting for host ready
//
//////////////////////////////////////////////////

int	MCSH_exitC1(void)
{
		if(IS_MOTOR_CHIPSET_HOST_READY())
		{
			WriteMotorChipSetCommand(motorChipSetMessageBuffer.cmd);

			SendLowPriorityMessage(THIS_SM, MotorChipSetCommandSent);
				
			switch(currMotorChipSetMsgType)
			{
				case	MOTORCS_MSG_TYPE_WRITE_CMD:
						return MCSH_ACTIVE_LAST_DATA_TRANSACTED;

				case	MOTORCS_MSG_TYPE_WRITE_WORD:
				case	MOTORCS_MSG_TYPE_WRITE_DWORD:
				case	MOTORCS_MSG_TYPE_READ_WORD:
				case	MOTORCS_MSG_TYPE_READ_DWORD:
							return MCSH_ACTIVE_SENDING_CMD;

				default:
					// ERROR SHOULD NOT BE REACHED
					SetCurrMCSmsgTypeErrorCountLog
						(++currMCSmsgTypeErrorCount);

					return SAME_STATE;
			}
		}
	
	++hostReadyRetryCount;

	if(hostReadyRetryCount > MCS_MAX_HOST_READY_RETRY_COUNT)
	{
		return MCSH_ERROR_HOST_READY;
	}
	else
	{
		SendLowPriorityMessage(THIS_SM, RecheckHostReadyLine);

		return	SAME_STATE;		
	}
}

//////////////////////////////////////////////////
//
// MotorChipSetCommandSent message while in
// MCSH_ACTIVE_SENDING_CMD
//
//////////////////////////////////////////////////

int	MCSH_exitD(void)
{
			if(IS_MOTOR_CHIPSET_HOST_READY())
			{					
				switch(currMotorChipSetMsgType)
				{
					case	MOTORCS_MSG_TYPE_WRITE_WORD:
							WriteMotorChipSetData
								(motorChipSetMessageBuffer.data1);

							SendLowPriorityMessage(THIS_SM, MotorChipSetDataWord1Sent);	
							return MCSH_ACTIVE_LAST_DATA_TRANSACTED;

					case	MOTORCS_MSG_TYPE_WRITE_DWORD:
							// Send Hi Word
							WriteMotorChipSetData
								(motorChipSetMessageBuffer.data2);	

							SendLowPriorityMessage(THIS_SM, MotorChipSetDataWord2Sent);
							return MCSH_ACTIVE_SENDING_WD2;
			
					case	MOTORCS_MSG_TYPE_READ_WORD:
						 		motorChipSetMessageBuffer.data1 = ReadMotorChipSetData();

							SendLowPriorityMessage(THIS_SM, MotorChipSetDataWord1Rcvd);
							return MCSH_ACTIVE_LAST_DATA_TRANSACTED;

					case	MOTORCS_MSG_TYPE_READ_DWORD:
								// Receive Hi Word
					 			motorChipSetMessageBuffer.data2 = ReadMotorChipSetData();
							
							SendLowPriorityMessage(THIS_SM, MotorChipSetDataWord2Rcvd);
							return MCSH_ACTIVE_RECEIVING_WD2;

					default:
						// ERROR SHOULD NOT BE REACHED
						SetCurrMCSmsgTypeErrorCountLog
							(++currMCSmsgTypeErrorCount);
						return SAME_STATE;
				}
			}

		hostReadyRetryCount = 1;

		SendLowPriorityMessage(THIS_SM, RecheckHostReadyLine);

	return MCSH_ACTIVE_WAIT_WR_DELAYED_HREADY;
}

//////////////////////////////////////////////////
//
// Timeout - While delayed and
//				waiting for host ready
//				MCSH_ACTIVE_WAIT_WR_DELAYED_HREADY
//
//////////////////////////////////////////////////

int	MCSH_exitD1(void)
{
		if(IS_MOTOR_CHIPSET_HOST_READY())
		{
			switch(currMotorChipSetMsgType)
			{
				case	MOTORCS_MSG_TYPE_WRITE_WORD:
						WriteMotorChipSetData
							(motorChipSetMessageBuffer.data1);

						SendLowPriorityMessage(THIS_SM, MotorChipSetDataWord1Sent);		

						return MCSH_ACTIVE_LAST_DATA_TRANSACTED;

				case	MOTORCS_MSG_TYPE_WRITE_DWORD:
						// Send Hi Word
						WriteMotorChipSetData
							(motorChipSetMessageBuffer.data2);	

						SendLowPriorityMessage(THIS_SM, MotorChipSetDataWord2Sent);														

						return MCSH_ACTIVE_SENDING_WD2;
			
				case	MOTORCS_MSG_TYPE_READ_WORD:
					 		motorChipSetMessageBuffer.data1 = ReadMotorChipSetData();
						
						SendLowPriorityMessage(THIS_SM, MotorChipSetDataWord1Rcvd);
						
						return MCSH_ACTIVE_LAST_DATA_TRANSACTED;

				case	MOTORCS_MSG_TYPE_READ_DWORD:
							// Receive Hi Word
					 		motorChipSetMessageBuffer.data1 = ReadMotorChipSetData();

						SendLowPriorityMessage(THIS_SM, MotorChipSetDataWord2Rcvd);

						return MCSH_ACTIVE_RECEIVING_WD2;

				default:
						// ERROR SHOULD NOT BE REACHED
						SetCurrMCSmsgTypeErrorCountLog
							(++currMCSmsgTypeErrorCount);

					return SAME_STATE;
			}
		}
		
	++hostReadyRetryCount;

	if(hostReadyRetryCount > MCS_MAX_HOST_READY_RETRY_COUNT)
	{
		return MCSH_ERROR_HOST_READY;
	}
	else
	{
		SendLowPriorityMessage(THIS_SM, RecheckHostReadyLine);

		return	SAME_STATE;		
	}
}



//////////////////////////////////////////////////
//
// MotorChipSetDataWord2Sent message while in
// MCSH_ACTIVE_SENDING_WD2
//
//////////////////////////////////////////////////

int	MCSH_exitE(void)
{
			if(IS_MOTOR_CHIPSET_HOST_READY())
			{					
				WriteMotorChipSetData
					(motorChipSetMessageBuffer.data1);

				SendLowPriorityMessage(THIS_SM, MotorChipSetDataWord1Sent);
				
				return MCSH_ACTIVE_LAST_DATA_TRANSACTED;
			}

		hostReadyRetryCount = 1;
		
		SendLowPriorityMessage(THIS_SM, RecheckHostReadyLine);

	return MCSH_ACTIVE_WAIT_WD1_DELAYED_HREADY;
}

//////////////////////////////////////////////////
//
// RecheckHostReadyLine - While delayed and
//				waiting for host ready
//				MCSH_ACTIVE_WAIT_WD1_DELAYED_HREADY
//
//////////////////////////////////////////////////

int	MCSH_exitE1(void)
{
		if(IS_MOTOR_CHIPSET_HOST_READY())
		{
				WriteMotorChipSetData
					(motorChipSetMessageBuffer.data1);

				SendLowPriorityMessage(THIS_SM, MotorChipSetDataWord1Sent);		

			return MCSH_ACTIVE_LAST_DATA_TRANSACTED;
		}
	
	++hostReadyRetryCount;

	if(hostReadyRetryCount > MCS_MAX_HOST_READY_RETRY_COUNT)
	{
		return MCSH_ERROR_HOST_READY;
	}
	else
	{
		SendLowPriorityMessage(THIS_SM, RecheckHostReadyLine);

		return	SAME_STATE;		
	}
}


//////////////////////////////////////////////////
//
// MotorChipSetDataWord2Rcvd message while in
// MCSH_ACTIVE_RECEIVING_WD2
//
//////////////////////////////////////////////////

int	MCSH_exitF(void)
{
			if(IS_MOTOR_CHIPSET_HOST_READY())
			{					
				motorChipSetMessageBuffer.data1 = ReadMotorChipSetData();

				SendLowPriorityMessage(THIS_SM, MotorChipSetDataWord1Rcvd);		
				
				return MCSH_ACTIVE_LAST_DATA_TRANSACTED;
			}

		hostReadyRetryCount = 1;
		
		SendLowPriorityMessage(THIS_SM, RecheckHostReadyLine);

	return MCSH_ACTIVE_WAIT_RD1_DELAYED_HREADY;
}

//////////////////////////////////////////////////
//
// Timeout - While delayed and
//				waiting for host ready
//				MCSH_ACTIVE_WAIT_WD1_DELAYED_HREADY
//
//////////////////////////////////////////////////

int	MCSH_exitF1(void)
{
		if(IS_MOTOR_CHIPSET_HOST_READY())
		{
			motorChipSetMessageBuffer.data1 = ReadMotorChipSetData();

			SendLowPriorityMessage(THIS_SM, MotorChipSetDataWord1Rcvd);	
				
			return MCSH_ACTIVE_LAST_DATA_TRANSACTED;
		}
	
	++hostReadyRetryCount;

	if(hostReadyRetryCount > MCS_MAX_HOST_READY_RETRY_COUNT)
	{
		return MCSH_ERROR_HOST_READY;
	}
	else
	{
		SendLowPriorityMessage(THIS_SM, RecheckHostReadyLine);

		return	SAME_STATE;		
	}
}


//////////////////////////////////////////////////
//
//	MotorChipSetCommandSent, 
//	MotorChipSetDataWord1Sent,
//	MotorChipSetDataWord1Rcvd,	while in
//		MCSH_ACTIVE_LAST_DATA_TRANSACTED
//
//////////////////////////////////////////////////

int	MCSH_exitK(void)
{	
			if(IS_MOTOR_CHIPSET_HOST_READY())
			{			
				motorChipSetChecksum = ReadMotorChipSetCheckSum();
				
				if(mcsCommandChecksum != motorChipSetChecksum)
				{
					IncMCSchecksumErrorCount();
				
					// Resend MCS Command

					SendLowPriorityMessage(THIS_SM, SendMotorChipSetCommand);
				}
				else
				{
					SendLowPriorityMessage
						(motorChipSetMessageBuffer.requestorId, 
							MotorChipSetMessageSent);
					
					MCS_CLEAR_COMMAND_BUFFER();
				}

				return MCSH_ACTIVE;
			}

		hostReadyRetryCount = 1;

		SendLowPriorityMessage(THIS_SM, RecheckHostReadyLine);

	return MCSH_ACTIVE_WAIT_CHK_DELAYED_HREADY;
}

//////////////////////////////////////////////////
//
// Timeout - While delayed and
//				waiting for host ready in
//				checksum verification
//				MCSH_ACTIVE_WAIT_CHK_DELAYED_HREADY
//
//////////////////////////////////////////////////

int	MCSH_exitK1(void)
{
		if(IS_MOTOR_CHIPSET_HOST_READY())
		{
				motorChipSetChecksum = ReadMotorChipSetCheckSum();

				if(mcsCommandChecksum != motorChipSetChecksum)
				{
					IncMCSchecksumErrorCount();

					// Resend MCS Command

					SendLowPriorityMessage(THIS_SM, SendMotorChipSetCommand);
				}	
				else
				{
					SendLowPriorityMessage
						(motorChipSetMessageBuffer.requestorId, MotorChipSetMessageSent);

					MCS_CLEAR_COMMAND_BUFFER();
				}

				return MCSH_ACTIVE;
		}
		
	++hostReadyRetryCount;

	if(hostReadyRetryCount > MCS_MAX_HOST_READY_RETRY_COUNT)
	{
			return MCSH_ERROR_HOST_READY;
	}
	else
	{
		SendLowPriorityMessage(THIS_SM, RecheckHostReadyLine);

		return	SAME_STATE;		
	}
}


//////////////////////////////////////////////////
//
// STATE MACHINE MATRIX DEFINITIONS
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_MCSH_IDLE)
	EV_HANDLER(GoReset, MCSH_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_MCSH_RESET_WAIT_1)
	EV_HANDLER(TimeOut, MCSH_exitA2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_MCSH_RESET_WAIT_2)
	EV_HANDLER(TimeOut, MCSH_exitA3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_MCSH_ACTIVE)
	EV_HANDLER(MotorChipSetRequestResource, MCSH_exitB)		,
	EV_HANDLER(MotorChipSetReleaseResource, MCSH_exitB1)	,
	EV_HANDLER(SendMotorChipSetCommand, MCSH_exitC)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_MCSH_ACTIVE_WAIT_CMD_DELAYED_HREADY)
	EV_HANDLER(RecheckHostReadyLine, MCSH_exitC1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_MCSH_ACTIVE_SENDING_CMD)
	EV_HANDLER(MotorChipSetCommandSent, MCSH_exitD)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_MCSH_ACTIVE_WAIT_WR_DELAYED_HREADY)
	EV_HANDLER(RecheckHostReadyLine, MCSH_exitD1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_MCSH_ACTIVE_SENDING_WD2)
	EV_HANDLER(MotorChipSetDataWord2Sent, MCSH_exitE)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_MCSH_ACTIVE_WAIT_WD1_DELAYED_HREADY)
	EV_HANDLER(RecheckHostReadyLine, MCSH_exitE1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_MCSH_ACTIVE_RECEIVING_WD2)
	EV_HANDLER(MotorChipSetDataWord2Rcvd, MCSH_exitF)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_MCSH_ACTIVE_WAIT_RD1_DELAYED_HREADY)
	EV_HANDLER(RecheckHostReadyLine, MCSH_exitF1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_MCSH_ACTIVE_LAST_DATA_TRANSACTED)
	EV_HANDLER(MotorChipSetCommandSent, MCSH_exitK),
	EV_HANDLER(MotorChipSetDataWord1Sent, MCSH_exitK),
	EV_HANDLER(MotorChipSetDataWord1Rcvd, MCSH_exitK)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_MCSH_ACTIVE_WAIT_CHK_DELAYED_HREADY)
	EV_HANDLER(RecheckHostReadyLine, MCSH_exitK1)
STATE_TRANSITION_MATRIX_END;


// 
// VERY IMPORTANT : 
//		State Entry definition order MUST match the 
//		order of the state definition in the .H File 
//

SM_RESPONSE_ENTRY(MCSH_Entry)
	STATE(_MCSH_IDLE)							,
	STATE(_MCSH_RESET_WAIT_1)					,
	STATE(_MCSH_RESET_WAIT_2)					,
	STATE(_MCSH_ACTIVE)							,

	STATE(_MCSH_ACTIVE_WAIT_CMD_DELAYED_HREADY)	,
	STATE(_MCSH_ACTIVE_SENDING_CMD)				,

	STATE(_MCSH_ACTIVE_WAIT_WR_DELAYED_HREADY)	,				

	STATE(_MCSH_ACTIVE_WAIT_WD1_DELAYED_HREADY)	,
	STATE(_MCSH_ACTIVE_SENDING_WD2)				,

	STATE(_MCSH_ACTIVE_WAIT_RD1_DELAYED_HREADY)	,
	STATE(_MCSH_ACTIVE_RECEIVING_WD2)			,

	STATE(_MCSH_ACTIVE_LAST_DATA_TRANSACTED)	,
	STATE(_MCSH_ACTIVE_WAIT_CHK_DELAYED_HREADY)	
	
	//
	//ERROR STATES
	//STATE(_MCSH_ERROR_HOST_NOT_READY)			
	//

SM_RESPONSE_END




