

//
//	#include "kernel.h"
//



/////////////////////////////////////////////////////////////////////////////
//
//
//	$Header:   N:/pvcs52/projects/pcu100~1/kernel.c_v   1.3   May 07 1997 09:11:36   Paul L C  $
//	$Log:   N:/pvcs52/projects/pcu100~1/kernel.c_v  $
//
//   Rev 1.3   May 07 1997 09:11:36   Paul L C
//Made functions to be expanded "inline" to reduce RAM stack usage.
//Created an inlined version of sendMessage for the ISRs.
//
//   Rev 1.2   Mar 25 1997 09:50:54   Paul L C
//CRC checking is now done outside of the interrupt service
//routine of the Comm Machine.
//
//   Rev 1.1   Mar 06 1997 11:07:24   Paul L C
//Corrected some comments.
//
//   Rev 1.0   Feb 26 1997 10:54:36   Paul L C
//Initial Revision
//
//
/////////////////////////////////////////////////////////////////////////////



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
// Important : a state machine can only have
//		a maximum of 127 states !
//
//////////////////////////////////////////////////

BYTE	SM_States[SM_COUNT];



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// These address constants
// will be loaded at ROM PAGE 0
//

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
	fdb		_UPDOWNSWMONITOR	// SM 8
	fdb		_SHEETCOUNTER		// SM 9

#endasm


const	WORD * SM_TableEntries[SM_COUNT] @SM_TEX;


//
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////



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
}

//////////////////////////////////////////////////
//
// Inlined SendMessage, for ISRs
//
//////////////////////////////////////////////////

#define	SendISRMessage(msg,sm_id) {	*(lastMessage+1) = (sm_id); 			\
									*lastMessage = (msg); 					\
									if(lastMessage == LAST_MESSAGE_SLOT)	\
										lastMessage = messageQueue; 		\
									else									\
										lastMessage+=2; }


//////////////////////////////////////////////////
//
// Aside from the kernel's intertask communication
// and timing facility, the kernel is also a state
// machine. It is available to perform system
// house keeping.
//
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
// Kernel Initialization
//
//////////////////////////////////////////////////

BYTE	Construct_Kernel()
{
		currMessage = lastMessage = messageQueue;

	return KERNEL_ACTIVE;
}


//////////////////////////////////////////////////
//
// This is the main kernel processing routine.
// The message queue is checked for message
// avaiablitity. If a message is avaiable, the
// kernel then checks the receiving state machine
// for its readiness to receive the message. An
// exit function is then performed if the message
// is accepted.
//
//////////////////////////////////////////////////

far 	* __tablePtr;

void	RunKernel(void)
{
	BYTE 	msgSearch;
	BYTE    currSMtimer;

	if(currMessage != lastMessage)
	{
		currSM_ID		= *(currMessage+1);
		currMSG_ID		= *currMessage;
		currSM_STATE	= SM_States[currSM_ID];

		// Point to State Machine entry table start
		// OffSet to the message destination machine

		__tablePtr = SM_TableEntries[currSM_ID];

		// Point to the message Search Table Based on
		// the machine's current state used as an offset

		//__tablePtr += (2 * currSM_STATE);

		#asm
			LDA		currSM_STATE
			LSLA
			ADD 	__tablePtr+1
			STA     __tablePtr+1
			LDA     __tablePtr
			ADC		#$00
			STA		__tablePtr
		#endasm

		// Get the Address of the Message Matrix

		 __tablePtr = (*(__tablePtr));

		// Search for a message match within the state

		while(1)
		{
			msgSearch = *(__tablePtr);

			if(msgSearch == currMSG_ID)
			{
				__tablePtr++;					// Point to the address of the
												// exit function

				__tablePtr = (*(__tablePtr));	// Load the address of the
												// exit function
				#asm
					LDA	#$CC 					// 6805 JMP instruction
					STA	__tablePtr-1
					JSR	__tablePtr-1
				#endasm

				SM_States[currSM_ID] = ac;		// update the state of the
												// current machine
				break;
			}

			if(msgSearch == NULL_MESSAGE)
			{
#ifdef DEVELOPMENT
				DoUnhandledMsg();
#endif
				break;							// message was not accepted
			}

			__tablePtr += 3;					// Point to the next message
		}


		if(currMessage == LAST_MESSAGE_SLOT)	// Update circular message buffer's
			currMessage = messageQueue;			// currMessage pointer
		else
			currMessage+=2;
	}


	//////////////////////////////////////////////////
	// Process Timer , flagged by ISR
	//////////////////////////////////////////////////

	if(SYS_TICK_ELAPSED)
	{
		CLEAR_SYSTEM_TICK_FLAG;

		for(currSMtimer=0; currSMtimer<SM_COUNT; currSMtimer++)
		{
			if(sysTimers[currSMtimer] != 0)
			{
				sysTimers[currSMtimer]--;

				if(sysTimers[currSMtimer] == 0)
					SendISRMessage(TimeOut, currSMtimer);
			}
		}
	}


	//////////////////////////////////////////////////
	// Process SPI , flagged by ISR
	//////////////////////////////////////////////////

	if(SYS_SPI_DATA_READY)
	{
		CLEAR_SYS_SPI_FLAG;

		SendISRMessage(SPIdataReady, ATOD_DRIVER_SM_ID);
	}


	//////////////////////////////////////////////////
	// Process SCI , flagged by ISR
	//////////////////////////////////////////////////

	switch(SystemSCIstatus)
	{
		case SCI_NO_STATUS 		:
			break;

		case SCI_BYTE_SENT	:
			SendISRMessage(ByteSent, SPUCOMM_SM_ID);
			SystemSCIstatus = SCI_NO_STATUS;
			break;

		case SCI_CMD_41_READY	:
			SendISRMessage(CommGoIdle, SPUCOMM_SM_ID);
			SendISRMessage(CommandReceived, PCUMNGR_SM_ID);
			SystemSCIstatus = SCI_NO_STATUS;
			break;

		case SCI_CMD_READY 		:
			SendMessage(CommGoIdle, SPUCOMM_SM_ID);

			if(IsCrcOk())
				SendISRMessage(CommandReceived, PCUMNGR_SM_ID);
					else
				SendISRMessage(InvalidPacket, PCUMNGR_SM_ID);

			SystemSCIstatus = SCI_NO_STATUS;
			break;

		case SCI_INVALID_PACKET	:
			SendISRMessage(CommGoIdle, SPUCOMM_SM_ID);
			SendISRMessage(InvalidPacket, PCUMNGR_SM_ID);
			SystemSCIstatus = SCI_NO_STATUS;
			break;
	}
}


//////////////////////////////////////////////////
//
// If a message is sent to a machine and that
// machine is not ready for the message, the
// message is lost and considered unhandled.
// This allows us to see these cases while in
// the development stages.
//
//////////////////////////////////////////////////

#ifdef DEVELOPMENT
	BYTE 	unhandledMsgCount;
	BYTE	debug1, debug2;

	void	DoUnhandledMsg(void)
	{
		unhandledMsgCount++;

		debug1 = currMSG_ID;
		debug2 = currSM_ID;
	}
#endif


//////////////////////////////////////////////////
//
// The kernel is a state machine in itself and
// is intended for background processing of
// system tasks.
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


