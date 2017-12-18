



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

#include <avr/eeprom.h>


////////////////////////////////////////////////////////////////////


#include "kernel.h"


////////////////////////////////////////////////////////////////////


#include "systemmanager.h"

#include "linkbuttonmanager.h"

#include "communicationmanager.h"

#include "encoderhandler.h"


////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////

U8		deviceAddress;

U8		deviceAddressIsProgrammed;

U8		deviceIsConfigured;




///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////

void	ProcessReceivedMessage(void);	



///////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////

U8		GetDeviceAddress(void)
{
	return deviceAddress;
}

void	SetDeviceAddress(U8 devAddr)
{
	deviceAddress = devAddr;
}

void	IncrementDeviceAddress(void)
{
	deviceAddress++;
	
	if(deviceAddress > DEVICE_ADDRESS_MAX)
		deviceAddress = DEVICE_ADDRESS_MIN;
		
	if(deviceAddress == DEVICE_ADDRESS_NONE)
		deviceAddress = DEVICE_ADDRESS_MIN;	
}

void	StoreDeviceAddressToEE(void)
{
	eeprom_write_byte(EE_DEVICE_ADDRESS, GetDeviceAddress());
}

///////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////

U8		IsDeviceAddressProgrammed(void)
{
	return deviceAddressIsProgrammed;
}

void	SetDeviceAddressProgrammed(void)
{
	deviceAddressIsProgrammed = TRUE;
}

void	ClearDeviceAddressProgrammed(void)
{
	deviceAddressIsProgrammed = FALSE;
}

void	StoreDeviceAddressProgSignatureToEE(void)
{
	eeprom_write_byte
		(EE_DEVICE_ADDRESS_PROG, DEVICE_ADDRESS_PROG_SIGNATURE);
}


///////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////

U8		IsDeviceConfigured(void)
{
	return deviceIsConfigured;
}

void	SetDeviceConfigured(void)
{
	deviceIsConfigured = TRUE;
}

void	ClearDeviceConfigured(void)
{
	deviceIsConfigured = FALSE;
}


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////

void    Init_SystemManager(void)
{
	U8	EEprogSignature;
	U8	EEdevAddress;
	
#ifdef DEBUG_ONE_ADDRESS
	EEdevAddress = 0x01;
	EEprogSignature = DEVICE_ADDRESS_PROG_SIGNATURE;
#else
	EEdevAddress = eeprom_read_byte(EE_DEVICE_ADDRESS);	
	EEprogSignature = eeprom_read_byte(EE_DEVICE_ADDRESS_PROG);
#endif
	
	SetDeviceAddress(EEdevAddress);
	
	if(EEprogSignature == DEVICE_ADDRESS_PROG_SIGNATURE)
		SetDeviceAddressProgrammed();
	else
		ClearDeviceAddressProgrammed();
		
		
	ClearDeviceConfigured();
}

////////////////////////////////////////////////////////////////////
//
// Exit Procedures
//
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//
// GoActive
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SM_exitA(void)
{
		if(IsDeviceAddressProgrammed())
		{
			SendMessage(THIS_MACHINE, LinkButtonPushShort);		
		}			
		else
		{
			SendMessage(DisplaySystemManager, GoDisplayTestMode);
		}		
			
    return SM_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// LinkButtonPushShort
//		While in SM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SM_exitB(void)
{		
	CancelTimer();
	
	SendMessage(WatchDogManager, EndWatchDogMonitor);
	
	SendMessage(EncoderHandler, GoIdle);
	SendMessage(CommunicationManager, GoIdle);
		
	
		if(IsDeviceAddressProgrammed())
		{			
			SendMessage(DisplaySystemManager, GoDisplayAddressMode);			
				
			LinkLEDON();
			
			return SM_DISPLAY_ADDRESS_WAIT;
		}		
			
	return SAME_STATE;    
}


////////////////////////////////////////////////////////////////////
//
// Timeout
//		While in SM_DISPLAY_ADDRESS_WAIT
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SM_exitC(void)
{					
		if(IsDeviceAddressProgrammed())
		{
			SendMessage(DisplaySystemManager, GoDisplayEncoderMode);
			
			SendMessage(EncoderHandler, GoActive);
			
			SendMessage(CommunicationManager, GoActive);
			
			SendMessage(WatchDogManager, BeginWatchdogMonitor);
		}
				
		LinkLEDOFF();
				
    return SM_ACTIVE;
}


////////////////////////////////////////////////////////////////////
//
// LinkButtonPushLong
//		While in SM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SM_exitD(void)
{		
		CancelTimer();
		
			SendMessage(DisplaySystemManager, GoDisplayAddressUpdateMode);
		
			SendMessage(EncoderHandler, GoIdle);
		
			SendMessage(CommunicationManager, GoIdle);
		
			SendMessage(WatchDogManager, EndWatchDogMonitor);
		
		LinkLEDON();
		
    return SM_INCREMENTING_ADDRESS;
}


////////////////////////////////////////////////////////////////////
//
// LinkButtonPushLong
//		While in SM_INCREMENTING_ADDRESS
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SM_exitE(void)
{			
		IncrementDeviceAddress();		
		
    return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
// LinkLongButtonPushDone
//		While in SM_INCREMENTING_ADDRESS
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SM_exitF(void)
{		
		StartTimer(MILLISECONDS(1500));
						
    return SM_INCREMENTING_ADDRESS_DELAY;
}

////////////////////////////////////////////////////////////////////
//
// Timeout
//		While in SM_INCREMENTING_ADDRESS_DELAY
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SM_exitG(void)
{						
		SendMessage(THIS_MACHINE, LinkButtonPushShort);
		
		StoreDeviceAddressToEE();
		StoreDeviceAddressProgSignatureToEE();
		
		SetDeviceAddressProgrammed();	
		
		LinkLEDOFF();
						
    return SM_ACTIVE;
}


////////////////////////////////////////////////////////////////////
//
// GoodPacketReceived
//		While in SM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SM_exitH(void)
{
									
		ProcessReceivedMessage();		
		
		SendMessage(CommLedManager, FlashRxLed);
		
		SendMessage(WatchDogManager, CommOkWatchDogReset);
		
    return SM_ACTIVE;
}


////////////////////////////////////////////////////////////////////
//
// MismatchPacketReceived
//		While in SM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SM_exitI1(void)
{
		// Listen again immediately
								
		SendMessage(CommunicationManager, GoReceivePacket);	
		
		SendMessage(CommLedManager, PulseRxLed);
		
    return SM_ACTIVE;
}


////////////////////////////////////////////////////////////////////
//
// BadPacketReceived
//		While in SM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SM_exitI2(void)
{
		// Listen again immediately
								
		SendMessage(CommunicationManager, GoReceivePacket);	
		
		SendMessage(CommLedManager, PulseRxLed);
		SendMessage(CommLedManager, PulseTxLed);
		
    return SM_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// PacketTransmitted
//		While in SM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SM_exitJ(void)
{
		// Listen again immediately
								
		SendMessage(CommunicationManager, GoReceivePacket);	
				
		SendMessage(CommLedManager, FlashTxLed);		
		
	return SM_ACTIVE;
}



////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

int encoderTypeCount;
int encoderTypeCountData;

int test = 0;

void	ProcessReceivedMessage(void)
{
	switch(rxMessage.messageId)
	{
		// Critical System Commands
		
		case COMM_MSG_GET_FW_VERSION_CMD:
			SendMessageAndData
				(CommunicationManager, GoTransmitPacket, 
				COMM_MSG_GET_FW_VERSION_REPLY, 0);
			break;
			
		case COMM_MSG_GET_ENCODER_POSITION_CMD:
				if(IsDeviceConfigured())
				{
					SendMessageAndData
					(CommunicationManager, GoTransmitPacket, 
					COMM_MSG_GET_ENCODER_POSITION_REPLY, 0);	
				}
				else
				{
					SendMessageAndData
					(CommunicationManager, GoTransmitPacket, 
					COMM_MSG_DEVICE_NOT_CONFIGURED_REPLY, 0);					
				}				
			break;		
		
				
		case COMM_MSG_SET_CONFIGURATION_CMD:
				
				encoderTypeCount = 0x00;
								
				encoderTypeCount |= (*(U8 *)(rxMessage.dataPointer));
				encoderTypeCount |= (*(U8 *)(rxMessage.dataPointer + 1) << 8);
		
				SetEncoderTypeCount(encoderTypeCount);

				
				if(*(U8 *)(rxMessage.dataPointer + 2))
					SetEncoderReversed();
				else
					ClearEncoderReversed();				
				
				SetEncoderMovingTolerance(*(U8 *)(rxMessage.dataPointer + 3));
					
				SetEncoderFilter(*(U8 *)(rxMessage.dataPointer + 4));
				
				SetEncoderSamplingRate(*(U8 *)(rxMessage.dataPointer + 5));
									
				SendMessageAndData
				(CommunicationManager, GoTransmitPacket, 
				COMM_MSG_SET_CONFIGURATION_REPLY, 0);
				
				SetDeviceConfigured();
						
			break;
				
						
												
		// Get Commands
		
		case COMM_MSG_GET_CONFIGURATION_CMD:
				if(IsDeviceConfigured())
				{
					
				}
				else
				{
					SendMessageAndData
					(CommunicationManager, GoTransmitPacket, 
					COMM_MSG_DEVICE_NOT_CONFIGURED_REPLY, 0);					
				}		
			break;
	
									
			
		// Future

		case COMM_MSG_GET_ENCODER_POWER_LEVEL_CMD:
		
			break;		
				
		case COMM_MSG_GET_TEMPERATURE_CMD:
		
			break;		
			
			
		case COMM_MSG_SET_ENCODER_POWER_LEVEL_CMD:
		
				//SetEncoderPowerLevel(*(U8 *)(rxMessage.dataPointer));
		
				SendMessageAndData
				(CommunicationManager, GoTransmitPacket, 
				COMM_MSG_SET_ENCODER_POWER_LEVEL_REPLY, 0);		
			break;						
			
		case COMM_MSG_DO_SOFTRESET_CMD:
			// TODO
			// Send Message to WDM
			// DoForcedWatchDogReset();
			break;		
	}
	
}


////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_SM_IDLE)
EV_HANDLER(GoActive, SM_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SM_ACTIVE)
EV_HANDLER(GoodPacketReceived, SM_exitH),
EV_HANDLER(MismatchPacketReceived, SM_exitI1),
EV_HANDLER(BadPacketReceived, SM_exitI2),
EV_HANDLER(PacketTransmitted, SM_exitJ),
EV_HANDLER(LinkButtonPushShort, SM_exitB),
EV_HANDLER(LinkButtonPushLong, SM_exitD)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SM_DISPLAY_ADDRESS_WAIT)
EV_HANDLER(DisplayAddressDone, SM_exitC),
EV_HANDLER(LinkButtonPushShort, SM_exitB),
EV_HANDLER(LinkButtonPushLong, SM_exitD)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SM_INCREMENTING_ADDRESS)
EV_HANDLER(LinkButtonPushLong, SM_exitE),
EV_HANDLER(LinkLongButtonPushDone, SM_exitF)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SM_INCREMENTING_ADDRESS_DELAY)
EV_HANDLER(TimeOut, SM_exitG)
STATE_TRANSITION_MATRIX_END;

// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(SM_Main_Entry)
	STATE(_SM_IDLE)							,
	STATE(_SM_ACTIVE)						,	
	STATE(_SM_DISPLAY_ADDRESS_WAIT)			,
	STATE(_SM_INCREMENTING_ADDRESS)			,
	STATE(_SM_INCREMENTING_ADDRESS_DELAY)	
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////


