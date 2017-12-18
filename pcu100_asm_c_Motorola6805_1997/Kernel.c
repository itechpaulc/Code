

//
//	#include "kernel.h"
//

void	*currMessage;
void	*lastMessage;

BYTE	currSM_ID;
BYTE	currMSG_ID;
BYTE	currSM_STATE;


#define		THIS_SM		currSM_ID
#define		SAME_STATE	currSM_STATE

#define     MAX_MESSAGE_COUNT   14


SM_message	messageQueue[MAX_MESSAGE_COUNT];



#define		MSG_QUEUE_SIZE		MAX_MESSAGE_COUNT*2

#define		LAST_MESSAGE_SLOT	messageQueue+MSG_QUEUE_SIZE-2



//////////////////////////////////////////////////
//
// Contains the Current State of the
// individual machines
//
//////////////////////////////////////////////////

BYTE	SM_States[SM_COUNT];
BYTE	msgCount;


#asm
SM_TEX:

	fdb		_KERNEL 			// SM 0
	fdb		_LEDSTATUS      	// SM 1
	fdb		_SPUCOMM			// SM 2
	fdb		_PCUMANAGER			// SM 3
	fdb		_MOTORCONTROLLER    // SM 4
	fdb		_ATODDRIVER			// SM 5
	fdb		_DIGITALINMONITOR	// SM 6
	fdb		_DIGITALOUTCNTRLR	// SM 7

#endasm


const	WORD * SM_TableEntries[SM_COUNT] @SM_TEX;



//////////////////////////////////////////////////
//
// Kernel Interface
//
//////////////////////////////////////////////////

void	SendMessage(BYTE msg, BYTE sm_id)
{
	*(lastMessage+1) = sm_id;
	*lastMessage = msg;


	if(lastMessage == LAST_MESSAGE_SLOT)
		lastMessage = messageQueue;
	else
		lastMessage+=2;


	if(lastMessage == currMessage)
		while(1) // Error
			;
}


BYTE	GetState(BYTE sm_id)
{
	return	SM_States[sm_id];
}


BYTE	Construct_Kernel()
{
		currMessage = lastMessage = messageQueue;

	return KERNEL_ACTIVE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////


far 	* __tablePtr;

void	RunKernel(void)
{
	BYTE 	msgSearch;
	BYTE    currSMtimer;

	BYTE	bug;

	if(currMessage != lastMessage)
	{
		currSM_ID		= *(currMessage+1);
		currMSG_ID		= *currMessage;
		currSM_STATE	= SM_States[currSM_ID];

		// Point to State Machine entry table start
		// OffSet to the message destination machine

		__longIX = SM_TableEntries[currSM_ID];

		// Point to the message Search Table Based on
		// the machine's current state used as an offset

		__longIX += (2 * currSM_STATE);

		// Get the Address of the Message Matrix

		__tablePtr = (*(__longIX));

		// Search for a message match within the state

		for(;;)
		{
			msgSearch = *(__tablePtr);

			if(msgSearch == currMSG_ID)
			{
				__tablePtr++;					// Point to the address of the
												// exit function

				__tablePtr = (*(__tablePtr));	// Load the address of the
												// exit function
				__longIX = __tablePtr;

				#asm
					LDA	#$CC 					// 6805 JSR instruction
					STA	__longIX-1
					JSR	__longIX-1
				#endasm

				SM_States[currSM_ID] = ac;
				break;
			}
			if(msgSearch == NULL_MESSAGE)
			{
				DoUnhandledMsg();
				break;
			}

			__tablePtr += 3;					// Point to the next message
		}


		if(currMessage == LAST_MESSAGE_SLOT)
			currMessage = messageQueue;
		else
			currMessage+=2;

	}

	// Process Timer

	if(SYS_TICK_ELAPSED)
	{
		CLEAR_SYSTEM_TICK_FLAG;

		for(currSMtimer=0; currSMtimer<SM_COUNT; currSMtimer++)
		{
			if(sysTimers[currSMtimer] != 0)
			{
				sysTimers[currSMtimer]--;

				if(sysTimers[currSMtimer] == 0)
				{
					// debug
					if(currSMtimer == 6)
						bug = 1;

					SendMessage(TimeOut, currSMtimer);
				}
			}
		}
	}

	// Process SPI

	if(SYS_SPI_DATA_READY)
	{
		CLEAR_SYS_SPI_FLAG;

		SendMessage(SPIdataReady, ATOD_DRIVER_SM_ID);
	}

	// Process SCI

	switch(SystemSCIstatus)
	{
		case SCI_NO_STATUS 		:
			break;

		case SCI_CMD_READY 		:
			SendMessage(CommGoIdle, SPUCOMM_SM_ID);
			SendMessage(CommandReceived, PCUMNGR_SM_ID);
			SystemSCIstatus = SCI_NO_STATUS;
			break;

		case SCI_BYTE_SENT	:
            SendMessage(ByteSent, SPUCOMM_SM_ID);
			SystemSCIstatus = SCI_NO_STATUS;
			break;

		case SCI_INVALID_PACKET	:
			SendMessage(CommGoIdle, SPUCOMM_SM_ID);
			SendMessage(InvalidPacket, PCUMNGR_SM_ID);
			SystemSCIstatus = SCI_NO_STATUS;
			break;

	}
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoUnhandledMsg(void)
{
	/*
	BYTE 	unhandledMsgCount;
	BYTE	debug1, debug2;

	unhandledMsgCount++;

	debug1 = currMSG_ID;
	debug2 = currSM_ID;
    */
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BYTE	KERNEL_exitA(void)
{
	return  KERNEL_ACTIVE;
}



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

#asm
_KERNEL:
	fcb     xKERNEL_ACTIVE
	fdb     xKERNEL_ACTIVE_MATRIX:
#endasm

#asm
xKERNEL_ACTIVE_MATRIX:
	fcb		KernelGoActive
	fdb		KERNEL_exitA
	fcb     NULL_MESSAGE
#endasm


