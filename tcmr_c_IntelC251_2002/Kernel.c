



/*
 *
 *
 *		$Header:   K:/Projects/Tcmr/Source/Kernel.c_v   1.3   11 Jul 2003 14:48:26   PaulLC  $
 *		$Log:   K:/Projects/Tcmr/Source/Kernel.c_v  $
 * 
 *    Rev 1.3   11 Jul 2003 14:48:26   PaulLC
 * Incorporated all changes since 0.60.B; Camera Trigger jumping fixes; 
 * Encoder noise filtering; Changes to supporte latest TCMRC HW; Limit switch configure.
 * 
 *    Rev 1.2   Apr 08 2002 14:31:16   PaulLC
 * Contains all of the changes made during beta development.
 * 
 *    Rev 1.1   Aug 27 2001 15:48:12   PaulLC
 * Changes made up to Engineering Version 00.25.A.
 * 
 *    Rev 1.0   Oct 06 2000 14:27:22   PaulLC
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


//
// State Machine Header Files
//

#include "tcmrmngr.h"

#include "hbeat.h"

#include "webenc.h"

#include "camsync.h"

#include "transp.h"

#include "subsys.h"

#include "pccomm.h"

#include "tparam.h"

#include "motorcs.h"

#include "motorcsi.h"

#include "pevmon.h"



//////////////////////////////////////////////////
//
// Private Variables
//
//////////////////////////////////////////////////

// Hi Priority Message Queue

SYSTEM_EVENT			idata	messageQueue[MAX_MESSAGE_COUNT];

void					* data smEvHandlerEntries[MAX_STATE_MACHINES];

int						idata	smStates[MAX_STATE_MACHINES];


SYSTEM_EVENT			idata	*lastMsgSlot;
SYSTEM_EVENT			idata	*lastMsg;
SYSTEM_EVENT			idata	*currMsg;

int						data	currSmState;
int						data	currSmId;
WORD					data	currMsgData;


EVENT_HANDLER			* evHandlerSelect;

void					**respEntry;

BYTE					near	sysTimers[MAX_STATE_MACHINES];

BYTE					near	activeTimerCount;
BYTE					near	activeTimerToDecrement;

// Low Priority Message Queue

SYSTEM_EVENT			lPmessageQueue[MAX_MESSAGE_COUNT];

SYSTEM_EVENT			*lPlastMsgSlot;
SYSTEM_EVENT			*lPlastMsg;
SYSTEM_EVENT			*lPcurrMsg;


//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

WORD	bdata			KERNEL_EVENT_FLAGS = 0x00;


sbit	TOP_CAMERA_TRIGGER_EVENT		= KERNEL_EVENT_FLAGS ^ 0;

sbit	BOTTOM_CAMERA_TRIGGER_EVENT		= KERNEL_EVENT_FLAGS ^ 1;

sbit	WEB_ENCODER_PULSE_EVENT			= KERNEL_EVENT_FLAGS ^ 2;

sbit	WEB_ENCODER_TDC_EVENT			= KERNEL_EVENT_FLAGS ^ 3;

sbit	KERNEL_TIMER_EVENT				= KERNEL_EVENT_FLAGS ^ 4;

sbit	PC_MESSAGE_RECEIVED_EVENT		= KERNEL_EVENT_FLAGS ^ 5;

sbit	MOTOR_CHIPSET_EVENT				= KERNEL_EVENT_FLAGS ^ 6;


//////////////////////////////////////////////////
//
//////////////////////////////////////////////////

#pragma disable
WORD	GetCurrEncoderPeriod(void)
{
	return currEncoderPeriod;
}

//////////////////////////////////////////////////
//
// These times based on 87251Tx running 24MHz and 
// On-chip program.
//
//////////////////////////////////////////////////

void    DELAY_2_USEC(void)
{
	_nop_();	_nop_();	_nop_();	_nop_();
	_nop_();	_nop_();	_nop_();	_nop_();
	_nop_();	_nop_();	_nop_();	_nop_();
	_nop_();	_nop_();	_nop_();	_nop_();
}

void    DELAY_6_USEC(void)
{
	DELAY_2_USEC();
	DELAY_2_USEC();
	DELAY_2_USEC();
}

//////////////////////////////////////////////////
// 
//
//////////////////////////////////////////////////

#define			RESTART_KERNEL_TIMER()		ReStartKernelTimer()

//////////////////////////////////////////////////
// 
//
//////////////////////////////////////////////////
void	
SendMessage(STATE_MACHINE_ID smID, SYSTEM_MESSAGE_ID msgID)
{										
	lastMsg->smDestId	= smID;					
	lastMsg->msgId		= msgID;		

	if(lastMsg == LAST_MESSAGE_SLOT)	
		lastMsg = messageQueue;			
	else								
		++lastMsg;						
}

//////////////////////////////////////////////////
// 
//
//////////////////////////////////////////////////
void	
SendLowPriorityMessage(STATE_MACHINE_ID smID, SYSTEM_MESSAGE_ID msgID)
{										
	lPlastMsg->smDestId	= smID;					
	lPlastMsg->msgId	= msgID;		

	if(lPlastMsg == LP_LAST_MESSAGE_SLOT)	
		lPlastMsg = lPmessageQueue;			
	else								
		++lPlastMsg;						
}

//////////////////////////////////////////////////
// 
// External Event Handler Entry listing
// found in their respective source files
//
//////////////////////////////////////////////////

void	InitKernel(void)
{
	InitISR();

	activeTimerCount = 0;

	// Point to the beginning of the queue

	lastMsg		= messageQueue;
	currMsg		= messageQueue;

	lPlastMsg		= lPmessageQueue;
	lPcurrMsg		= lPmessageQueue;

	// Define the end of the queue

	lastMsgSlot		= &messageQueue[MAX_MESSAGE_COUNT - 1];

	lPlastMsgSlot	= &lPmessageQueue[MAX_MESSAGE_COUNT - 1];


	// Build SM Response Entries

	smEvHandlerEntries[TCMR_SystemManagerID]				=	&TCMR_Entry;

	smEvHandlerEntries[HeartBeatHandlerID]					=	&HBH_Entry;
	smEvHandlerEntries[WebEncoderHandlerID]					=	&WEH_Entry;

	smEvHandlerEntries[TopCameraSynchronizerHandlerID]		=	&CSH_Entry;
	smEvHandlerEntries[BottomCameraSynchronizerHandlerID]	=	&CSH_Entry;

	smEvHandlerEntries[TopTransportHandlerID]				=	&TRH_Entry;
	smEvHandlerEntries[BottomTransportHandlerID]			=	&TRH_Entry;

	smEvHandlerEntries[TopSubsystemHandlerID]				=	&SSH_Entry;
	smEvHandlerEntries[BottomSubsystemHandlerID]			=	&SSH_Entry;

	smEvHandlerEntries[PcCommunicationHandlerID]			=	&PCH_Entry;

	smEvHandlerEntries[TransportParamHandlerID]				=	&TPH_Entry;
	smEvHandlerEntries[MotorChipSetHandlerID]				=	&MCSH_Entry;
	smEvHandlerEntries[MotorChipSetInterruptHandlerID]		=	&MCSI_Entry;

	smEvHandlerEntries[PressEventMonitorID]					=	&PEM_Entry;

	//
	// Initialize SM States
	//
	
	smStates[TCMR_SystemManagerID]				= TCMR_IDLE;

	smStates[HeartBeatHandlerID]				= HBH_IDLE;
	
	smStates[WebEncoderHandlerID]				= WEH_IDLE;
	
	smStates[TopCameraSynchronizerHandlerID]	= CSH_IDLE;
	smStates[BottomCameraSynchronizerHandlerID]	= CSH_IDLE;

	smStates[TopTransportHandlerID]				= TRH_IDLE;
	smStates[BottomTransportHandlerID]			= TRH_IDLE;
	
	smStates[TopSubsystemHandlerID]				= SSH_IDLE;
	smStates[BottomSubsystemHandlerID]			= SSH_IDLE;

	smStates[PcCommunicationHandlerID]			= PCH_IDLE;

	smStates[TransportParamHandlerID]			= TPH_IDLE;
	smStates[MotorChipSetHandlerID]				= MCSH_IDLE;
	smStates[MotorChipSetInterruptHandlerID]	= MCSI_IDLE;

	smStates[PressEventMonitorID]				= PEM_IDLE;

	// Init Kernel Timer to begin Running
	// 4 msec Ticks @24 Mhz, Match and Compare mode

	ENABLE_OUTPUT_COMPARE_4();

	CCAP4L = 0x00;
	CCAP4H = 0x00;
}



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	debugTrap() { }

BYTE	debug001;

void	RunKernel(void)
{
	BYTE	currSMtimer;

	while(TRUE)
	{
		// Check for High Priority Messages

		if(currMsg != lastMsg)
		{
			currSmState = smStates[currMsg->smDestId];
			currSmId = currMsg->smDestId;
			currMsgData = currMsg->msgDat;

			// Point to the state matrix
			
			respEntry = (void **)smEvHandlerEntries[currSmId];
				
			evHandlerSelect = 
				(EVENT_HANDLER *)(0xFF0000 + (*(respEntry + currSmState)));

				while(TRUE)
				{
					// Search the state matrix for matching event id    

					if(evHandlerSelect->eventId == currMsg->msgId)
					{
						// Execute exit procedure			
						smStates[currSmId] = evHandlerSelect->exitProc();
						break;
					}
										
					if(evHandlerSelect->eventId == NULL_MESSAGE_ID)
					{
						DoUnhandledEvent();
						break;
					}

					++evHandlerSelect;	// select next entry				
				}

			if(currMsg == LAST_MESSAGE_SLOT)
				currMsg = messageQueue;
			else
				++currMsg;
		}
		else	// Check for Low Priority Messages
		if(lPcurrMsg != lPlastMsg)
		{
			currSmState = smStates[lPcurrMsg->smDestId];
			currSmId = lPcurrMsg->smDestId;
			currMsgData = lPcurrMsg->msgDat;

			// Point to the state matrix
			
			respEntry = (void **)smEvHandlerEntries[currSmId];
				
			evHandlerSelect = 
				(EVENT_HANDLER *)(0xFF0000 + (*(respEntry + currSmState)));

				while(TRUE)
				{
					// Search the state matrix for matching event id    

					if(evHandlerSelect->eventId == lPcurrMsg->msgId)
					{
						// Execute exit procedure			
						smStates[currSmId] = evHandlerSelect->exitProc();
						break;
					}
										
					if(evHandlerSelect->eventId == NULL_MESSAGE_ID)
					{
						DoUnhandledEvent();
						break;
					}

					++evHandlerSelect;	// select next entry				
				}

			if(lPcurrMsg == LP_LAST_MESSAGE_SLOT)
				lPcurrMsg = lPmessageQueue;
			else
				++lPcurrMsg;
		}

		//
		// Check other system events
		//

		if(KERNEL_EVENT_A_ACTIVATED())
		{	
			// Schedule the Service of One Interrupt Event

			if(IS_WEB_ENCODER_PULSE_EVENT_SET())
			{
				debug001 = GetEncoderTick();

				while(GetEncoderTick())
				{
					DecrementEncoderTick();

					InLineSendMessage(WebEncoderHandlerID, EncoderPulseDetect);
				}			

				CLEAR_WEB_ENCODER_PULSE_EVENT();
			}
			else
			if(IS_WEB_ENCODER_TDC_EVENT_SET())
			{			
				InLineSendMessage(WebEncoderHandlerID, EncoderTdcPulseDetect);

				CLEAR_WEB_ENCODER_TDC_EVENT();
			}	
			else
			if(IS_TOP_CAMERA_TRIGGER_EVENT_SET())
			{
				InLineSendMessage(TopCameraSynchronizerHandlerID, CameraTriggerDone);
		
				CLEAR_TOP_CAMERA_TRIGGER_EVENT();
			}
			else
			if(IS_BOTTOM_CAMERA_TRIGGER_EVENT_SET())
			{
				InLineSendMessage(BottomCameraSynchronizerHandlerID, CameraTriggerDone);

				CLEAR_BOTTOM_CAMERA_TRIGGER_EVENT();
			}
			else
			if(IS_KERNEL_TIMER_EVENT_SET())
			{
				if(KERNEL_HAS_RUNNING_TIMERS())
				{
					activeTimerToDecrement = activeTimerCount;

					for(currSMtimer=0; currSMtimer<MAX_STATE_MACHINES; currSMtimer++)
					{
						if(sysTimers[currSMtimer] != 0)
						{
							--sysTimers[currSMtimer];

							--activeTimerToDecrement;

							if(sysTimers[currSMtimer] == 0)
							{
								InLineSendLowPriorityMessage(currSMtimer, TimeOut);
								--activeTimerCount;
									
								if(activeTimerToDecrement != 0)
									continue;
								else
									break;
							}
						}
					}
				}
	
				RESTART_KERNEL_TIMER();

				CLEAR_KERNEL_TIMER_EVENT();
			}			
			else				
			if(IS_PC_MESSAGE_RECEIVED_EVENT_SET())
			{
				InLineSendLowPriorityMessage
					(PcCommunicationHandlerID, PcCommandReceived);

				CLEAR_PC_MESSAGE_RECEIVED_EVENT();
			}
			else
			if(IS_MOTOR_CHIPSET_EVENT_SET())
			{
				InLineSendLowPriorityMessage
					(MotorChipSetInterruptHandlerID, MotorChipSetEventDetected);

				CLEAR_MOTOR_CHIPSET_EVENT();
			}
		}

		if(debug001 > 1)
		{
			debug001 = 0;
			debugTrap();
		}
	}
}



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoUnhandledEvent(void)
{

}






