





//
//
//
//  Author :    Paul Calinawan
//
//  Date:       Jan 14 , 2012
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

#include "itechsys.h"


////////////////////////////////////////////////////////////////////


#include "kernel.h"


////////////////////////////////////////////////////////////////////


#include <avr/io.h>

#include <avr/interrupt.h>



#include "communicationmanager.h"

#include "systemmanager.h"


#include "encoderhandler.h"



////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////
  
 
////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////


U16		CalculateChecksum(U8 *ptrBuff, U8 buffLen)
{
	U16 chksum;
	U8	c, dat;
	
		chksum = 0;
	
		for(c=0; c < buffLen; c++)
		{	
			dat = ptrBuff[c];
			
			chksum += dat;
		}		
		
	return chksum;	
}

//

void	BuildKernelUartTxBuffer(COMM_MSG *ptrCommMsg)
{
	U8	data;
	
	U16 dataLen, packLen, checkSum;
	
		kernelUartTxBuffer[0] = LOW_BYTE(ptrCommMsg->deviceType);
		kernelUartTxBuffer[1] = HIGH_BYTE(ptrCommMsg->deviceType);
		
		kernelUartTxBuffer[2] = LOW_BYTE(ptrCommMsg->deviceId);
		kernelUartTxBuffer[3] = HIGH_BYTE(ptrCommMsg->deviceId);
	
		kernelUartTxBuffer[4] = LOW_BYTE(ptrCommMsg->messageId);
		kernelUartTxBuffer[5] = HIGH_BYTE(ptrCommMsg->messageId);
			
		//
	
		dataLen = ptrCommMsg->dataLenght;
	
		kernelUartTxBuffer[6] = LOW_BYTE(dataLen);
		kernelUartTxBuffer[7] = HIGH_BYTE(dataLen);
			
		//
		
		packLen = 8;
				
			for(int d=0; d<dataLen; d++)
			{
				data = ptrCommMsg->dataPointer[d];
			
				kernelUartTxBuffer[packLen] = data;
							
				++packLen;
			}
			
		checkSum = CalculateChecksum(&kernelUartTxBuffer[0], packLen);
		
		ptrCommMsg->checkSum = checkSum	;
		
		kernelUartTxBuffer[packLen] = LOW_BYTE(checkSum);
		kernelUartTxBuffer[++packLen] = HIGH_BYTE(checkSum);

		kernelUartTxBytesToSend = packLen + 1;
} 


//
//


COMM_MSG	txMessage;
U8			txData[MAX_TX_DATA_SIZE];

void Tx_BuildHeader(void)
{
	txMessage.deviceType = GET_THIS_DEVICE_TYPE();	
	
	txMessage.deviceId = GetDeviceAddress();
}

//

void Tx_BuildDeviceNotConfiguredReply(void)
{
	Tx_BuildHeader();
	
	txMessage.messageId = COMM_MSG_DEVICE_NOT_CONFIGURED_REPLY;
	
	txMessage.dataLenght = 0;
		
	txMessage.dataPointer = NULL;
	
	BuildKernelUartTxBuffer(&txMessage);
}

//

void Tx_BuildGetFirmwareVersionMsgReply(void)
{
	Tx_BuildHeader();
	
	txMessage.messageId = COMM_MSG_GET_FW_VERSION_REPLY;
	
	txMessage.dataLenght = 3;
	
	txData[0] = MAJOR_VERSION;
	txData[1] = MINOR_VERSION;
	txData[2] = ENGINEERING_VERSION;
	
	txMessage.dataPointer = &txData[0];
	
	BuildKernelUartTxBuffer(&txMessage);
}

//
//
//

void Tx_BuildSetConfigurationMsgReply(void)
{
	Tx_BuildHeader();
	
	txMessage.messageId = COMM_MSG_SET_CONFIGURATION_REPLY;
	
	txMessage.dataLenght = 0;
		
	txMessage.dataPointer = NULL;
	
	BuildKernelUartTxBuffer(&txMessage);
}

//
//
//

void Tx_BuildGetEncoderPositionMsgReply(void)
{
	Tx_BuildHeader();
	
	txMessage.messageId = COMM_MSG_GET_ENCODER_POSITION_REPLY;
	
	txMessage.dataLenght = 2;
	
	txData[0] = (U8)(GetEncoderTrueBinaryValueCurr());
	txData[1] = (U8)(GetEncoderTrueBinaryValueCurr() >> 8);
	
	txMessage.dataPointer = &txData[0];
	
	BuildKernelUartTxBuffer(&txMessage);
}

//
//
//

COMM_MSG	rxMessage;
U8			rxData[MAX_RX_DATA_SIZE];

void Rx_BuildHeader(void)
{
	rxMessage.deviceType = *(U16 *)(kernelUartRxBuffer);	
	rxMessage.deviceId = *(U16 *)(kernelUartRxBuffer+2);
}



////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void	BeginTransmitMessage(void)
{
	ResetKernelUart();
	
	SystemDeActivateRx();	
	SystemActivateTx();		
}

void	BeginReceiveMessage(void)
{
	ResetKernelUart();
	
	SystemDeActivateTx();	
	SystemActivateRx();
}
	
////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////


void    Init_CommunicationManager(void)
{
	ResetKernelUart();
			
	SystemDeActivateTx();
	SystemDeActivateRx();			
}

////////////////////////////////////////////////////////////////////
//
// Exit Procedures
//
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//
// GoActive
//		While in SM_IDLE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   CM_exitA(void)
{
		StartTimer(MILLISECONDS(100));

    return CM_ACTIVE_DELAY;
}

////////////////////////////////////////////////////////////////////
//
// TimeOut
//		While in CM_ACTIVE_DELAY
//
////////////////////////////////////////////////////////////////////

NEW_STATE   CM_exitB(void)
{
		// DEBUG TEST
	
#ifndef DEBUG_TEST_TX
	SendMessage(THIS_MACHINE, GoReceivePacket);
#endif
		

    return CM_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// GoReceivePacket
//		While in CM_ACTIVE
//
////////////////////////////////////////////////////////////////////


NEW_STATE   CM_exitC(void)
{
		BeginReceiveMessage();		

    return CM_ACTIVE_RECEIVING_PACKET;
}


////////////////////////////////////////////////////////////////////
//
// PacketReceived
//
////////////////////////////////////////////////////////////////////

NEW_STATE   CM_exitC1(void)
{
	U8 d, datLen, idx;
	
	U16 calcCheckSum = 0x0000;
	U16 rcvdCheckSum = 0x0000;
	
	rxMessage.dataPointer = NULL;
	
		Rx_BuildHeader();
		
		if((rxMessage.deviceType == GET_THIS_DEVICE_TYPE()) &&
				(rxMessage.deviceId == GetDeviceAddress()))
		{
			idx = 4;			
			rxMessage.messageId = *(U16 *)(kernelUartRxBuffer + idx);			
			idx += 2;
			
			datLen = *(U16 *)(kernelUartRxBuffer + idx);
			rxMessage.dataLenght = datLen;			
			idx += 2;
			
			for(d=0; d<datLen; d++)
			{				
				rxData[d] = kernelUartRxBuffer[idx];
				idx++;
			}
			
			rcvdCheckSum = *(U16 *)(kernelUartRxBuffer + idx);
			rxMessage.checkSum = rcvdCheckSum;
			
			
			calcCheckSum = 
				CalculateChecksum(&kernelUartRxBuffer[0], kernelUartRxBytesReceived-2);
			
			if(calcCheckSum == rcvdCheckSum)
			{
				rxMessage.dataPointer = &rxData[0];
				
				SendMessage(SystemManager, GoodPacketReceived);		
			}	
			else
			{
				SendMessage(SystemManager, BadPacketReceived);
				
				// Tell System Manager Bad Checksum				
				// And other errors. put in DATA 1
				
			}	
		}	
		else
		{
			SendMessage(SystemManager, MismatchPacketReceived);
			
			// Tell System Manager 
			// Dev Type not match or Address not match
				
		}		
			
    return CM_ACTIVE;
}


////////////////////////////////////////////////////////////////////
//
// BadPacketReceived
//
////////////////////////////////////////////////////////////////////

NEW_STATE   CM_exitC2(void)
{
			SendMessage(SystemManager, BadPacketReceived);
			
			// Tell System Manager 
			
    return CM_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// GoTransmitPacket
//		While in CM_ACTIVE
//
////////////////////////////////////////////////////////////////////

U16 sysManagerCmd;

NEW_STATE   CM_exitD(void)
{
		StartTimer(1);
				
		sysManagerCmd = KernelGetMessageData1();
		
	return CM_ACTIVE_DELAY_TRANSMIT;
}

////////////////////////////////////////////////////////////////////
//
// TimeOut
//		While in CM_ACTIVE_DELAY_TRANSMIT
//
////////////////////////////////////////////////////////////////////

#define		TX_EN_DELAY		1

NEW_STATE   CM_exitD1(void)
{	
		switch(sysManagerCmd)
		{
			case COMM_MSG_GET_FW_VERSION_REPLY:
				Tx_BuildGetFirmwareVersionMsgReply();
				break;
			
			case COMM_MSG_GET_ENCODER_POSITION_REPLY:
				Tx_BuildGetEncoderPositionMsgReply();
				break;

			case COMM_MSG_SET_CONFIGURATION_REPLY:
				Tx_BuildSetConfigurationMsgReply();
				break;
				
			case COMM_MSG_DEVICE_NOT_CONFIGURED_REPLY:
				Tx_BuildDeviceNotConfiguredReply();
				break;
							
			//
									
		}
		
		// Enable TxLine Earlier, before transmitting first BYTE
		// DEBUG - Should be part of the kernel code delay
		
		//TxLineOn();
		
		StartTimer(TX_EN_DELAY);

    return CM_ACTIVE_TRANSMITTING_PACKET_TXEN_DELAY;
}

////////////////////////////////////////////////////////////////////
//
// Timeout
//		While in CM_ACTIVE_TRANSMITTING_PACKET_DELAY
//
////////////////////////////////////////////////////////////////////

NEW_STATE   CM_exitD2(void)
{
		BeginTransmitMessage();
		
	return CM_ACTIVE_TRANSMITTING_PACKET;	
}

////////////////////////////////////////////////////////////////////
//
// PacketTransmitted
//		While in CM__ACTIVE_TRANSMITTING_PACKET
//
////////////////////////////////////////////////////////////////////

NEW_STATE   CM_exitD3(void)
{
		SendMessage(SystemManager, PacketTransmitted);
		
	return CM_ACTIVE;	
}


////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_CM_IDLE)
EV_HANDLER(GoActive, CM_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_CM_ACTIVE_DELAY)
EV_HANDLER(TimeOut, CM_exitB)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_CM_ACTIVE)
EV_HANDLER(GoReceivePacket, CM_exitC),
EV_HANDLER(GoTransmitPacket, CM_exitD)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_CM_ACTIVE_DELAY_TRANSMIT)
EV_HANDLER(TimeOut, CM_exitD1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_CM_ACTIVE_TRANSMITTING_PACKET_TXEN_DELAY)
EV_HANDLER(TimeOut, CM_exitD2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_CM_ACTIVE_TRANSMITTING_PACKET)
EV_HANDLER(PacketTransmitted, CM_exitD3)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_CM_ACTIVE_RECEIVING_PACKET)
EV_HANDLER(PacketReceived, CM_exitC1),
EV_HANDLER(BadPacketReceived, CM_exitC2)
STATE_TRANSITION_MATRIX_END;



// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(CM_Main_Entry)
	STATE(_CM_IDLE),
	STATE(_CM_ACTIVE_DELAY),
	STATE(_CM_ACTIVE),	
	STATE(_CM_ACTIVE_DELAY_TRANSMIT),
	STATE(_CM_ACTIVE_RECEIVING_PACKET),
	STATE(_CM_ACTIVE_TRANSMITTING_PACKET_TXEN_DELAY),
	STATE(_CM_ACTIVE_TRANSMITTING_PACKET)
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////


