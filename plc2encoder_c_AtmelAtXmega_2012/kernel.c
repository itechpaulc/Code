

//
//
//
//  Author :    Paul Calinawan
//
//  Date:       April 4 , 2011
//
//  Copyrights: Imaging Technologies Inc.
//
//  Product:    ITECH PLC2
//  
//  Subsystem:  Absolute Encoder Monitor Board
//
//  -------------------------------------------
//
//
//      CONFIDENTIAL DOCUMENT
//
//      Property of Imaging Technologies Inc.
//
//






////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////

#include <avr/io.h>
#include <avr/pgmspace.h> 


#include <avr/interrupt.h>

#include "itechsys.h"


////////////////////////////////////////////////////////////////////


#include "kernel.h"


////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////
//
// State Machine Header Files
//
////////////////////////////////////////////////////////////////////

#include "heartbeat.h"
#include "encoderhandler.h"

#include "spidevicemanager.h"

#include "displaysystemmanager.h"
#include "displayintensitycontroller.h"

#include "commledmanager.h"
#include "communicationmanager.h"


#include "systemmanager.h"

#include "linkdebouncemanager.h"
#include "linkbuttonmanager.h"

#include "watchdogmanager.h"



////////////////////////////////////////////////////////////////////
//
//  Local Kernel Variables
//
////////////////////////////////////////////////////////////////////

SYSTEM_EVENT			*lastMessageIndex,
						*nextMessage,
						*currentMessage;

U8						kernelMessageCounter; 


//
// The Message QUEUE
//

SYSTEM_EVENT			kernelMessageQueue[MAX_MESSAGE_COUNT];

void					*   smEventHandlerEntries[MAX_STATE_MACHINES_COUNT];

NEW_STATE					smStates[MAX_STATE_MACHINES_COUNT];

//
//
//

NEW_STATE				currentSmState;

STATE_MACHINE_ID		currentDestinationSmId,
						currentSourceSmId;

DATA					currentMessageData1,
						currentMessageData2;

//
//
//

//EVENT_HANDLER			 * eventHandlerSelect;

void					 **responseEntry;

NEW_STATE				newState;

//
//
//
//

U8						kernelCommTimerElapsed,
						kernelTimerElapsed;

U16						systemTimers[MAX_STATE_MACHINES_COUNT];

U8						activeTimersToDecrement,
						currentSmTimer;

//

U8						kernelSpiTransferComplete;

//

U8						kernelUartTransferComplete;
						

U8						kernelUartTxBuffer[KERNEL_MAX_UART_MESSAGE_LENGTH];
U8						kernelUartRxBuffer[KERNEL_MAX_UART_MESSAGE_LENGTH];


U8						*kernelUartTxPtr,
						*kernelUartRxPtr;

U8						kernelUartTxBytesToSend,
						kernelUartRxBytesReceived;


U8						kernelIsrTxState;
U8						kernelIsrRxState;

//

U32						kernelIsrRxByte,
						kernelRxUartStatus,
						kernelUartCommFail;

U8						kernelIsrBusIdleTimer,
						kernelCommPacketReceived,
						kernelCommBadPacketReceived;

////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

void		DoHandleLostEvent(void);

void		InitializeKernelAndMachines(void);

void		InitializeKernelTimers(void);

void		InitializeKernelSpiComm(void);


void		InitializeStateMachines(void);
	
void		DoKernelTimerCheck(void);

void		DoKernelSpiTransferCompleteCheck(void);

void		DoKernelCommCheck(void);

void		ClearKernelUartCommFail(void);



////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

void    SendKernelMessage
			(STATE_MACHINE_ID smID, SYSTEM_MESSAGE_ID msgID);

void    SendKernelMessageAndData
			(STATE_MACHINE_ID smID, SYSTEM_MESSAGE_ID msgID, 
			U16 msgDAT1, U16 msgDAT2);


////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

void	SetKernelCommTimerElapsed(void)
{
		kernelCommTimerElapsed = SET;
}

void	ClearKernelCommTimerElapsed(void)
{
		kernelCommTimerElapsed = CLEAR;
}

//
//

U8    IsKernelCommPacketReceived(void)
{
		return kernelCommPacketReceived;
}

void	SetKernelCommPacketReceived(void)
{
		kernelCommPacketReceived = SET;
}

void	ClearKernelCommPacketReceived(void)
{
		kernelCommPacketReceived = CLEAR;
}

//
//

U8    IsKernelCommBadPacketReceived(void)
{
		return kernelCommBadPacketReceived;
}

void	SetKernelCommBadPacketReceived(void)
{
		kernelCommBadPacketReceived = SET;
}

void	ClearKernelCommBadPacketReceived(void)
{
		kernelCommBadPacketReceived = CLEAR;
}

//
//

U8    IsKernelTimerElapsed(void)
{
		return kernelTimerElapsed;
}


void	SetKernelTimerElapsed(void)
{
		kernelTimerElapsed = SET;
}


void	ClearKernelTimerElapsed(void)
{
		kernelTimerElapsed = CLEAR;
}

//
//

U8    IsKernelSpiTransferComplete(void)
{
		return kernelSpiTransferComplete;
}



void	SetKernelSpiTransferComplete(void)
{
		kernelSpiTransferComplete = SET;
}


void	ClearKernelSpiTransferComplete(void)
{
		kernelSpiTransferComplete = CLEAR;
}



//
//

//

void	SetKernelIsrTxState(U8 isrTxSt)
{
	kernelIsrTxState = isrTxSt;
}

void	SetKernelIsrRxState(U8 isrRxSt)
{
	kernelIsrRxState = isrRxSt;
}


//
//

#define	KERNEL_COMM_BUS_IDLE_TIMEOUT		(5)

#define	KERNEL_BUS_IDLE_TIMER_DISABLE		(0xFF)

//

void	StartKernelBusIdleTimer(void)
{
	kernelIsrBusIdleTimer = 0;
	
	ClearKernelCommTimerElapsed();
}

void	CancelKernelBusIdleTimer(void)
{
	kernelIsrBusIdleTimer = KERNEL_BUS_IDLE_TIMER_DISABLE;
	
	ClearKernelCommTimerElapsed();
}

void	DisableKernelBusIdleTimer(void)
{
	kernelIsrBusIdleTimer = KERNEL_BUS_IDLE_TIMER_DISABLE;
}

BOOL	IsKernelIsrBusIdleTimerActive(void)
{
		if(kernelIsrBusIdleTimer != KERNEL_BUS_IDLE_TIMER_DISABLE)
			return TRUE;
	
	return FALSE;
}


//
//

void	ClearTxBuffer(void)
{
	U8 c;
	
		for(c=0; c<KERNEL_MAX_UART_MESSAGE_LENGTH; c++)
		{
			kernelUartTxBuffer[c] = 0x00;	
		}	
}

void	ClearRxBuffer(void)
{
	U8 c;
	
		for(c=0; c<KERNEL_MAX_UART_MESSAGE_LENGTH; c++)
		{
			kernelUartRxBuffer[c] = 0x00;		
		}	
}

void		ResetKernelUart(void)
{
		kernelUartTxPtr = kernelUartTxBuffer;
		kernelUartRxPtr = kernelUartRxBuffer;	
		
		SetKernelIsrTxState(TX_STATE_IDLE);	
		SetKernelIsrRxState(RX_STATE_IDLE);
		
		kernelUartRxBytesReceived = 0;
		
		DisableKernelBusIdleTimer();
		
		ClearKernelUartCommFail();
}



U8    IsKernelUartTransferComplete(void)
{
		return kernelUartTransferComplete;
}



void	SetKernelUartTransferComplete(void)
{
		kernelUartTransferComplete = SET;
}


void	ClearKernelUartTransferComplete(void)
{
		kernelUartTransferComplete = CLEAR;
}

//
//
//

U8    IsKernelUartCommFail(void)
{
		return kernelUartCommFail;
}



void	SetKernelUartCommFail(void)
{
		kernelUartCommFail = SET;
}


void	ClearKernelUartCommFail(void)
{			
		kernelUartCommFail = 0x00;
}

//
//
//


void	DoKernelCommCheck(void)
{
	if(IsKernelCommPacketReceived())
	{
		ClearKernelCommPacketReceived();
		
		SendKernelMessage(CommunicationManager, PacketReceived);
	}	
	else
	if(IsKernelUartTransferComplete())
	{
		ClearKernelUartTransferComplete();
		
		SendKernelMessage(CommunicationManager, PacketTransmitted);
	}
	if(IsKernelCommBadPacketReceived())
	{
		ClearKernelCommBadPacketReceived();
		
		SendKernelMessage(CommunicationManager, BadPacketReceived);
	}	
}



//
//
//

///////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

#define F_CPU				8000000 
#define DOUBLE

//#define USART_BAUDRATE		((115200UL) >> 1)
#define USART_BAUDRATE		((76800UL) >> 1)
//#define USART_BAUDRATE		((38400UL) >> 1)
//#define USART_BAUDRATE		((9600UL) >> 1)

 
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1) 

void		InitializeKernelUartComm(void)
{	   
		// Turn OFF the transmission and reception circuitry 	
		
		SystemDeActivateTx();
		SystemDeActivateRx();
   
		// Use 8-bit character sizes
		// ASYNCHRONOUS / 8 Bit / 1 start/ 1 stop / No Parity / Rising Edge Polarity
		    
		UCSR0C |= ((1 << UCSZ01) | (1 << UCSZ00));
		
		
		// Load upper 8-bits of the baud rate value into the high byte of the UBRR register 
		// Load lower 8-bits of the baud rate value into the low byte of the UBRR register
		
		UBRR0 = (BAUD_PRESCALE | (BAUD_PRESCALE >> 8)); 	   
		  
		// Normal Speed Mode
	
		// DEBUG DOUBLE SPEED MODE
		//UCSR0A |= (0<<U2X0); 
		UCSR0A |= (1<<U2X0); 

		// Clear all associated flags
		
		ClearKernelUartTransferComplete();
		ClearKernelCommPacketReceived();
		ClearKernelCommBadPacketReceived();
		
		ClearKernelCommTimerElapsed();
		CancelKernelBusIdleTimer();
		
		ClearTxBuffer();
		ClearRxBuffer();
				
		ResetKernelUart();
}


///////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

void    InitializeKernel(void)
{
		InitializeKernelSpiComm();
		
		InitializeKernelUartComm();
		
		
		InitializeKernelAndMachines();

		InitializeKernelTimers();


		//
		// Kick Start Each Machine
		//
		
		SendKernelMessage(WatchDogManager, GoActive);
				
		SendKernelMessage(HeartBeatHandler, GoActive);
				
		SendKernelMessage(SpiDeviceManager, GoActive);		
		
		SendKernelMessage(DisplaySystemManager, GoActive);		
		SendKernelMessage(DisplayIntensityController, GoActive);	
		SendKernelMessage(CommLedManager, GoActive);	
		
		SendKernelMessage(SystemManager, GoActive);
		
		
		SendKernelMessage(LinkDebounceManager, GoActive);	
		SendKernelMessage(LinkButtonManager, GoActive);	
				
}



////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

U32				eventHandlerSelectData;
U16				currentEventHandlerSelect;

U16				thisEventId;
NEW_STATE		(*thisExitProcedure)(void);
	
	
void    RunKernel(void)
{
    while ( TRUE )
    {
		
        if ( currentMessage != nextMessage )
        {	
            currentSmState = smStates[currentMessage->smDestinationId];
            currentDestinationSmId  = currentMessage->smDestinationId;
            
			currentSourceSmId       = currentMessage->smSourceId;
			
            currentMessageData1 = currentMessage->messageData1;
            currentMessageData2 = currentMessage->messageData2;

            //
            // Point to the state machine matrix entry
            //			

            responseEntry  =  
				(void **)smEventHandlerEntries[currentDestinationSmId];
				
			currentEventHandlerSelect = pgm_read_word(responseEntry + currentSmState);			
								
			eventHandlerSelectData = pgm_read_dword(currentEventHandlerSelect);				
			
			thisEventId = (U16)(eventHandlerSelectData);
			thisExitProcedure = (void *)(eventHandlerSelectData >> 16);
			
					
            while ( TRUE )
            {				
                // search for the state matrix for matching event id
 
			    if ( thisEventId == currentMessage->messageId )
                {
                    // call the exit procedure     
					
					newState = thisExitProcedure();
                    smStates[currentDestinationSmId] = newState;
					
                    break;
                }

				if( thisEventId == NULL_MESSAGE_ID )
                {
                    DoHandleLostEvent();

                    break;
                }

                // select the next in the event handler entries
								
				currentEventHandlerSelect += sizeof(unsigned long);
				
				eventHandlerSelectData = pgm_read_dword(currentEventHandlerSelect);
								
				thisEventId = (U16)(eventHandlerSelectData);									
				thisExitProcedure = (void *)(eventHandlerSelectData >> 16);		
						          
            }	
			

            if ( currentMessage == LAST_MESSAGE_INDEX )
                currentMessage = kernelMessageQueue;
            else
                ++currentMessage;

            kernelMessageCounter--;			
        }

        //
        // Check other system wide events
        //

        DoKernelTimerCheck();
		
		DoKernelSpiTransferCompleteCheck();
		
		DoKernelCommCheck();

    }
}



////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////
//
// Kernel Timer Services
//
////////////////////////////////////////////////////////////////////
  
void    StartTimer(unsigned int ticks)
{
	if ( ticks < 1 ) ticks = 1;

		if ( systemTimers[currentDestinationSmId] == 0 )		// Make Sure it's Not Active

			systemTimers[currentDestinationSmId] = ticks;       
}
 
void    CancelTimer(void)
{
		if ( systemTimers[currentDestinationSmId] != 0 )		//Make Sure it's Not already Active
		{
			systemTimers[currentDestinationSmId] = 0;
		}
}

void    DoKernelTimerCheck(void)
{
    if ( IsKernelTimerElapsed() )
    {
            for ( currentSmTimer=0; 
                currentSmTimer<MAX_STATE_MACHINES_COUNT; currentSmTimer++ )
            {
                if ( systemTimers[currentSmTimer] != 0 )
                {
                    --systemTimers[currentSmTimer];

                    if ( systemTimers[currentSmTimer] == 0 )
                    {
                        SendKernelMessage((STATE_MACHINE_ID)currentSmTimer, TimeOut);
                    }
                }
            }
        		
		ClearKernelTimerElapsed();
    }
}


DATA                
KernelGetMessageData1(void)
{
    return currentMessage->messageData1;
} 

DATA                
KernelGetMessageData2(void)
{
    return currentMessage->messageData2;
} 

STATE_MACHINE_ID    
KernelGetSmSourceId(void)
{
    return currentMessage->smSourceId;
} 



////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

void    InitializeKernelAndMachines(void)
{
		// Reset all message queue pointers

		currentMessage  = kernelMessageQueue;
		nextMessage     = kernelMessageQueue;

		// Define the end of the queue

		lastMessageIndex = 
			&kernelMessageQueue[MAX_MESSAGE_COUNT - 1];

		kernelMessageCounter = 0;
		
		//  
		//  Build the State Machine Response Entries
		//  and Initialize the State Machine States
		//

		smEventHandlerEntries[HeartBeatHandler]			=   &HBH_Main_Entry;
		smStates[HeartBeatHandler]						=   INIT_IDLE_STATE;

		smEventHandlerEntries[EncoderHandler]			=   &EH_Main_Entry;
		smStates[EncoderHandler]						=   INIT_IDLE_STATE;
		
		smEventHandlerEntries[SpiDeviceManager]			=   &SDM_Main_Entry;
		smStates[SpiDeviceManager]						=   INIT_IDLE_STATE;	
			

		smEventHandlerEntries[DisplaySystemManager]		=   &DSM_Main_Entry;
		smStates[DisplaySystemManager]					=   INIT_IDLE_STATE;		

		smEventHandlerEntries[DisplayIntensityController]		=   &DIC_Main_Entry;
		smStates[DisplayIntensityController]					=   INIT_IDLE_STATE;	
		
		
		smEventHandlerEntries[CommLedManager]			=   &CLM_Main_Entry;
		smStates[CommLedManager]						=   INIT_IDLE_STATE;			

		smEventHandlerEntries[CommunicationManager]		=   &CM_Main_Entry;
		smStates[CommunicationManager]					=   INIT_IDLE_STATE;	
		
		
		smEventHandlerEntries[SystemManager]			=   &SM_Main_Entry;
		smStates[SystemManager]							=   INIT_IDLE_STATE;	


		smEventHandlerEntries[LinkDebounceManager]		=   &LDM_Main_Entry;
		smStates[LinkDebounceManager]					=   INIT_IDLE_STATE;	
		
		smEventHandlerEntries[LinkButtonManager]		=   &LBM_Main_Entry;
		smStates[LinkButtonManager]						=   INIT_IDLE_STATE;	
					
		smEventHandlerEntries[WatchDogManager]			=   &WDM_Main_Entry;
		smStates[WatchDogManager]						=   INIT_IDLE_STATE;		
		
		//
		
		InitializeStateMachines();
}


////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

void    InitializeStateMachines(void)
{
		Init_HeartBeatHandler();
		Init_EncoderHandler();
		
		Init_SpiDeviceManager();
		
		Init_DisplaySystemManager();
		Init_DisplayIntensityController();
		Init_CommLedManager();
		
		Init_SystemManager();		
		
		Init_LinkDebounceManager();	
		Init_LinkButtonManager();	
		
		Init_CommunicationManager();
		
		Init_WatchdogManager();
}


////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

//
//		1 microsecond - clock input prescaled * 100
//
//		100 microsecond TICKS
//

#define	KERNEL_TICK_INTERVAL			(0x64)

ISR	(TIMER0_COMPA_vect)		
{	
		// Clear Flag
			
		TIFR0 |= (SET << OCF0A);
			
		OCR0A += KERNEL_TICK_INTERVAL;
			
		SetKernelTimerElapsed();					
			
		if(IsKernelIsrBusIdleTimerActive())
		{
			kernelCommTimerElapsed++;
			
			if(kernelCommTimerElapsed > KERNEL_COMM_BUS_IDLE_TIMEOUT)
			{
				SystemDeActivateRx();
				
				DisableKernelBusIdleTimer();
				
				SetKernelCommTimerElapsed();
				
				if(IsKernelUartCommFail())
					SetKernelCommBadPacketReceived();
				else
					SetKernelCommPacketReceived();				
			}			
		}	
}

void		KickStartKernelTimer0(void)
{
		// Timer 0 Reset
		
		TIMSK0	= 0x00;
		TCCR0A	= 0x00;
		TCCR0B	= 0x00;
		TCNT0	= 0x00;
		OCR0A	= 0x00;
		
		// Interrupt Enable
		
		TIMSK0 |= (SET << OCIE0A);				
		
		// Set Timer Control Register A 
		// OC0A disconnected
		// COM0A1 = 0x00;
		// COM0A0 = 0x00;
		
		TCCR0A |= ((CLEAR << COM0A1) | (CLEAR << COM0A0));
		
		// Set Timer Control Register B
		// Select Clock Source and prescaler
		//
		// 8Mhz (Internal OSC) 
		// CLOCK/8 = 8Mhz =  1 usec
		// CS00 = 0x00
		// CS01 = 0x01
		// CS02 = 0x00
		
		TCCR0B |= ((CLEAR <<CS00) | (SET <<CS01) | (CLEAR <<CS02));
									
		// Clear Flag
			
		TIFR0 |= (SET << OCF0A);
		
		// Next Interrupt Time set
		
		OCR0A += KERNEL_TICK_INTERVAL;
}

void		InitializeKernelTimers(void)
{
	U8		timerCount;
	
				kernelTimerElapsed = CLEAR;
			
				// Reset all timers to zero

				activeTimersToDecrement = 0;

				for ( timerCount=0; timerCount<MAX_STATE_MACHINES_COUNT ;timerCount++ )
				{
					systemTimers[timerCount] = 0;  
				}
			
			ClearKernelTimerElapsed();
			
			KickStartKernelTimer0();
}

//
//
//

void		InitializeKernelSpiComm(void)
{	
		ClearKernelSpiTransferComplete();
}


void		DoKernelSpiTransferCompleteCheck(void)
{
	if(IsKernelSpiTransferComplete())
	{
			SendKernelMessage(SpiDeviceManager,  SpiTransmitComplete);		
			
			ClearKernelSpiTransferComplete();
	}		
}

//
//
//

void	TxLineOn(void)
{
	PORTC |= TX_LINE_MASK;
}

void	TxLineOff(void)
{
	PORTC &= (~TX_LINE_MASK);
}


//
//
//


void	UartComm_EnableReceiveInterrupt(void)
{
	UCSR0B |= (1 << RXCIE0); // Enable the USART Receive Complete interrupt (USART_RXC)
};

void	UartComm_DisableReceiveInterrupt(void)
{
	UCSR0B &= (~(1 << RXCIE0)); // 
};


void	UartComm_EnableTransmitCompleteInterrupt(void)
{
	UCSR0B |= (1 << TXCIE0); // 
	
	/// UDRIE0 ??
};

void	UartComm_DisableTransmitCompleteInterrupt(void)
{
	UCSR0B &= (~(1 << TXCIE0)); // 
};


void	UartComm_EnableDataRegisterEmptyInterrupt(void)
{
	UCSR0B |= (1 << UDRIE0); // 
};

void	UartComm_DisableDataRegisterEmptyInterrupt(void)
{
	UCSR0B &= (~(1 << UDRIE0)); // 
};

//
//
//



void	SystemActivateTx(void)
{	
	UCSR0B = (1 << TXEN0); 	
	
	SetKernelIsrTxState(TX_STATE_IDLE);	 

	UartComm_DisableReceiveInterrupt();
	
		TxLineOn();
	
	UartComm_EnableDataRegisterEmptyInterrupt();
}

void	SystemDeActivateTx(void)
{
	TxLineOff();
	
	UCSR0B &= (~(1 << TXEN0));
	
	SetKernelIsrTxState(TX_STATE_IDLE);
	
	UartComm_DisableTransmitCompleteInterrupt();  
	UartComm_DisableDataRegisterEmptyInterrupt();
}

void	SystemActivateRx(void)
{
	TxLineOff();
		
	UCSR0B = (1 << RXEN0);
	
	SetKernelIsrRxState(RX_STATE_IDLE);	
	
	// FLUSH ???
		 	
	UartComm_DisableTransmitCompleteInterrupt();  
	UartComm_DisableDataRegisterEmptyInterrupt();
	
	UartComm_EnableReceiveInterrupt();	
}

void	SystemDeActivateRx(void)
{
	TxLineOff();
	
	UCSR0B &= (~(1 << RXEN0));  
	
	SetKernelIsrRxState(RX_STATE_IDLE);
	
	UartComm_DisableReceiveInterrupt();	
}


//
//
//

//
// UART has Received 1 Byte 
//

ISR	(USART_RX_vect)		
{
	kernelRxUartStatus = UCSR0A;
	
	kernelIsrRxByte = UDR0;

		if(kernelRxUartStatus & (1<<FE0))
		{		 
			// Error in Receive
			// FRAME ERROR
		
			StartKernelBusIdleTimer();
		
			// DEBUG
			// SET ERROR FLAGS
		
			SetKernelUartCommFail();
			
			SystemDeActivateRx();
		}
		else
		if(kernelRxUartStatus & (1<<DOR0))
		{		 
			// Error in Receive
	  		// DATA OVERRUN ERROR
		
			StartKernelBusIdleTimer();
		
			// DEBUG
			// SET ERROR FLAGS
		
			SetKernelUartCommFail();
			
			SystemDeActivateRx();
		}
		else	
		switch(kernelIsrRxState)
		{		
			case RX_STATE_IDLE:
								
				(*kernelUartRxPtr) = kernelIsrRxByte;
			
				kernelUartRxPtr++;
				kernelUartRxBytesReceived++;
			
				SetKernelIsrRxState(RX_STATE_RECEIVING_PACKET);
			
				StartKernelBusIdleTimer();
			
				break;
		
			case RX_STATE_RECEIVING_PACKET:
									
				(*kernelUartRxPtr) = kernelIsrRxByte;
			
				kernelUartRxPtr++;
				kernelUartRxBytesReceived++;
			
				StartKernelBusIdleTimer();
			
				if(kernelUartRxBytesReceived > KERNEL_MAX_UART_MESSAGE_LENGTH)
				{
				
				}
			
				break;
			
			default:
		
				break;
		}	
}

//
//
//

U8		isrTxBytesToSend;
U8		isrTxByte;

//
// Transmit DATA Register is EMPTY
//


ISR	(USART_UDRE_vect)		
{	
	switch(kernelIsrTxState)
	{
		// TO DO Settle 100 usec
		
		case TX_STATE_IDLE:
							
			// Stuff into TX Register
			
			isrTxByte = (U8)(*kernelUartTxPtr);			
			UDR0 = isrTxByte;
			
			kernelUartTxPtr++;		
			kernelUartTxBytesToSend--;
			
			SetKernelIsrTxState(TX_STATE_TRANSMITTING);
			
			break;
			
		case TX_STATE_TRANSMITTING:			
			
			isrTxByte = (U8)(*kernelUartTxPtr);	
			UDR0 = isrTxByte;	
			
			kernelUartTxPtr++;	
			kernelUartTxBytesToSend--;
			
			if(kernelUartTxBytesToSend == 0)
				SetKernelIsrTxState(TX_STATE_WAIT_LAST_BYTE_SENT);
							
			break;
		
		case TX_STATE_WAIT_LAST_BYTE_SENT:
		
			UartComm_DisableDataRegisterEmptyInterrupt();
			UartComm_EnableTransmitCompleteInterrupt();
			
			break;
			
		default:			
		
			break;
	}	
}

//
// Transmit of LAST BYTE is complete
//

ISR (USART_TX_vect)
{
		// TO DO Settle 100 usec
			
		TxLineOff();
			
		SetKernelUartTransferComplete();
						
		SetKernelIsrTxState(TX_STATE_IDLE);	
}


////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

inline  
    void    SendKernelMessage
    (STATE_MACHINE_ID smID, SYSTEM_MESSAGE_ID msgID)
{
		nextMessage->smDestinationId    = smID;            
		nextMessage->messageId          = msgID;

		nextMessage->smSourceId         = KERNEL_SM_ID; 

		nextMessage->messageData1       = NO_DATA;
		nextMessage->messageData2       = NO_DATA;

		if ( nextMessage == LAST_MESSAGE_INDEX )
			nextMessage = kernelMessageQueue;
		else
			++nextMessage;

		kernelMessageCounter++;
}


inline  
    void    SendKernelMessageAndData
    (STATE_MACHINE_ID smID, SYSTEM_MESSAGE_ID msgID, 
     U16 msgDAT1, U16 msgDAT2)
{
			nextMessage->smDestinationId    = smID;            
			nextMessage->messageId          = msgID;

			nextMessage->smSourceId         = KERNEL_SM_ID; 

			nextMessage->messageData1       = msgDAT1;
			nextMessage->messageData2       = msgDAT2;

			if ( nextMessage == LAST_MESSAGE_INDEX )
				nextMessage = kernelMessageQueue;
			else
				++nextMessage;

			kernelMessageCounter++;
}

void    SendMessage
    (STATE_MACHINE_ID smID, SYSTEM_MESSAGE_ID msgID)
{
		nextMessage->smDestinationId    = smID;            
		nextMessage->messageId          = msgID;

		nextMessage->smSourceId         = currentDestinationSmId; 

		nextMessage->messageData1       = 0x0000;
		nextMessage->messageData2       = 0x0000;

		if ( nextMessage == LAST_MESSAGE_INDEX )
			nextMessage = kernelMessageQueue;
		else
			++nextMessage;

		kernelMessageCounter++;
}


void    SendMessageAndData
    (STATE_MACHINE_ID smID, SYSTEM_MESSAGE_ID msgID, 
     U16 msgDAT1, U16 msgDAT2)
{
		nextMessage->smDestinationId    = smID;            
		nextMessage->messageId          = msgID;

		nextMessage->smSourceId         = currentDestinationSmId; 

		nextMessage->messageData1       = msgDAT1;
		nextMessage->messageData2       = msgDAT2;

		if ( nextMessage == LAST_MESSAGE_INDEX )
			nextMessage = kernelMessageQueue;
		else
			++nextMessage;

		kernelMessageCounter++;
}

////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

static		U16		lostEventCount;

void    DoHandleLostEvent(void)
{
	++lostEventCount;
}



NEW_STATE   DoNothing(void)
{
    return SAME_STATE;
}

