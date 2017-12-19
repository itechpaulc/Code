//----------------------------------------------------------------------------
// File : saver_manager.c
//----------------------------------------------------------------------------
//
//                  Copyright (c) 2013 Lansmont Corporation
//                            ALL RIGHTS RESERVED
//
//----------------------------------------------------------------------------
//                      R E V I S I O N    H I S T O R Y
//----------------------------------------------------------------------------
// Rev  Date     Name       Description
// ---- -------- ---------- ------------------------------------------------
// 1.00 12/17/13 Paul C. Initial revision.
//----------------------------------------------------------------------------

// Standard includes
#include <stdint.h>
#include <string.h>

// RTOS includes
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

// Application includes
#include "board.h"
#include "delay.h"
#include "ticks.h"
#include "saver.h"

#include "button.h"

#include "saver_manager.h"

#include "saverstatemachine.h"

//

#include "usbcomm.h"

#include "command.h"

//
//
//

void	SaverManagerRunProcess(SaverInterface *saver, SaverSystemMessage *saverMngrMsg);
void	SaverManagerLiveProcess(SaverInterface *saverSaver, SaverSystemMessage *saverMngrMsg);
void	SaverManagerEngineeringProcess(SaverInterface *saverSaver, SaverSystemMessage *saverMngrMsg);
void	SaverManagerParkProcess(SaverInterface *saver, SaverSystemMessage *saverMngrMsg);

//

void	GoodPacketReceivedProcess(SaverInterface *saver);

//

void	SaverManagerRunProcess(SaverInterface *saver, SaverSystemMessage *saverMngrMsg);
void	SaverManagerLiveProcess(SaverInterface *saver, SaverSystemMessage *saverMngrMsg);
void	SaverManagerEngineeringProcess(SaverInterface *saver, SaverSystemMessage *saverMngrMsg);
void	SaverManagerParkProcess(SaverInterface *saver, SaverSystemMessage *saverMngrMsg);

//

void	ButtonRunStopProcess(SaverInterface *saver);

//

void	SaverManagerRunTimeout(SaverInterface *saver);

//

static void	SaverManagerMachineProcess(SaverInterface *saver, SaverSystemMessage *saverMngrMsg);


//----------------------------------------------------------------------------
// SaverManagerTask
//
// This task sleeps indefinitely until it receives notification there is
// event that needs to be transferred from FRAM to flash.  This function
// should not be called directly.  One instance of it should be created
// with the xTaskCreate function.
//
// In:
//   parameters : pointer to Saver interface structure (must not be NULL)
//
// Out:
//   nothing
//
// Returns:
//   nothing
//----------------------------------------------------------------------------

//#define TRACKER_STRING_LEN	64
//
//char					trackerString[TRACKER_STRING_LEN];
//uint8_t					ch;
//uint16_t				len = TRACKER_STRING_LEN;
//int						gpsCount;
//int 					trap = 0;
//
//battTestState = BATT_TEST_0;

portTASK_FUNCTION(SaverManagerTask, parameters)
{
	SaverInterface	*saver	= (SaverInterface*) parameters;

	SaverSystemMessage		saverMngrMsg;

		for (;;)
		{
			// Wait for notification of event ready
			if (xQueueReceive(saver->TaskManagerNotify, &saverMngrMsg, SAVER_MANAGER_TIMEOUT) == pdPASS)
			{
				switch(saver->TaskManagerState)
				{
					case MNGR_Run:
						SaverManagerRunProcess(saver, &saverMngrMsg);
						break;
					case MNGR_Live:
						SaverManagerLiveProcess(saver, &saverMngrMsg);
						break;
					case MNGR_Engineering:
						SaverManagerEngineeringProcess(saver, &saverMngrMsg);
						break;
					case MNGR_Park:
						SaverManagerParkProcess(saver, &saverMngrMsg);
						break;

					default:
						break;
				}
			}
			else
			{
				// Timer Timeout

				switch(saver->TaskManagerState)
				{
					case MNGR_Run:
						SaverManagerRunTimeout(saver);

					default:
						break;
				}
			}

			SaverManagerMachineProcess(saver, &saverMngrMsg);
		}
}

//

void	SaverManagerRunProcess(SaverInterface *saver, SaverSystemMessage *saverMngrMsg)
{
	switch(saverMngrMsg->msgId)
	{
		case GoodPacketReceived:
			GoodPacketReceivedProcess(saver);
		break;

		case ButtonRunStopDetect:
			ButtonRunStopProcess(saver);
		break;


		default:
			break;
	}


}

//

void	SaverManagerLiveProcess(SaverInterface *saver, SaverSystemMessage *saverMngrMsg)
{
	switch(saverMngrMsg->msgId)
	{
		case GoodPacketReceived:
			GoodPacketReceivedProcess(saver);
		break;

		default:
			break;
	}
}

//

void	SaverManagerEngineeringProcess(SaverInterface *saver, SaverSystemMessage *saverMngrMsg)
{
	switch(saverMngrMsg->msgId)
	{
		case GoodPacketReceived:
			GoodPacketReceivedProcess(saver);
		break;

		default:
			break;
	}
}

//

void	SaverManagerParkProcess(SaverInterface *saver, SaverSystemMessage *saverMngrMsg)
{
	switch(saverMngrMsg->msgId)
	{
		case GoodPacketReceived:
			GoodPacketReceivedProcess(saver);
		break;

		default:
			break;
	}
}

//


void	GoodPacketReceivedProcess(SaverInterface *saver)
{
	SaverSystemMessage		saverMngrMsg;

	uint16_t cmdId = 0x0000;

	cmdId |= usbRxTxDataBuffer[CMD_ID_OFFSET_0];
	cmdId |= usbRxTxDataBuffer[CMD_ID_OFFSET_1] << 8;

	switch(cmdId)
	{
		case CMD_GET_CURR_ACCEL_DATA_FULL:
			saverMngrMsg.msgId = GoTransmitReplyPacket;
			saverMngrMsg.data1 = COMM_MSG_GET_CURR_ACCEL_DATA_REPLY;
			xQueueSend(saver->TaskCmdNotify, &saverMngrMsg, GENERAL_TASK_TIMEOUT);
		break;

		case CMD_SET_ACCEL_SETTING:
			AccelSettingProcessCommData(saver, &usbRxTxDataBuffer[DATA_START_OFFSET]);
			saverMngrMsg.msgId = GoTransmitAckPacket;
			saverMngrMsg.data1 = CMD_SET_ACCEL_SETTING;
			xQueueSend(saver->TaskCmdNotify, &saverMngrMsg, GENERAL_TASK_TIMEOUT);
			break;

		case CMD_GET_VERSION:
			saverMngrMsg.msgId = GoTransmitReplyPacket;
			saverMngrMsg.data1 = COMM_MSG_GET_FW_VERSION_REPLY;
			xQueueSend(saver->TaskCmdNotify, &saverMngrMsg, GENERAL_TASK_TIMEOUT);
			break;

		case CMD_GET_CURR_DATE_TIME:
			saverMngrMsg.msgId = GoTransmitReplyPacket;
			saverMngrMsg.data1 = COMM_MSG_GET_DATE_TIME_REPLY;
			xQueueSend(saver->TaskCmdNotify, &saverMngrMsg, GENERAL_TASK_TIMEOUT);
			break;

		case CMD_SET_DEVICE_RUN_MODE:
			saver->TaskManagerState = MNGR_Run;
			saver->Mode = RUN;

			saverMngrMsg.msgId = GoTransmitAckPacket;
			saverMngrMsg.data1 = CMD_SET_DEVICE_RUN_MODE;
			xQueueSend(saver->TaskCmdNotify, &saverMngrMsg, GENERAL_TASK_TIMEOUT);
			break;

		case CMD_SET_DEVICE_ENGINEERING_MODE:
			saver->TaskManagerState = MNGR_Engineering;
			saver->Mode = ENGINEERING;

			saverMngrMsg.msgId = GoTransmitAckPacket;
			saverMngrMsg.data1 = CMD_SET_DEVICE_ENGINEERING_MODE;
			xQueueSend(saver->TaskCmdNotify, &saverMngrMsg, GENERAL_TASK_TIMEOUT);
			break;

		case CMD_SET_DEVICE_LIVE_MODE:
			saver->TaskManagerState = MNGR_Live;
			saver->Mode = LIVE;

			saverMngrMsg.msgId = GoTransmitAckPacket;
			saverMngrMsg.data1 = CMD_SET_DEVICE_LIVE_MODE;
			xQueueSend(saver->TaskCmdNotify, &saverMngrMsg, GENERAL_TASK_TIMEOUT);
			break;

		case CMD_SET_DEVICE_PARK_MODE:
			saver->TaskManagerState = MNGR_Park;
			saver->Mode = PARK;

			saverMngrMsg.msgId = GoTransmitAckPacket;
			saverMngrMsg.data1 = CMD_SET_DEVICE_PARK_MODE;
			xQueueSend(saver->TaskCmdNotify, &saverMngrMsg, GENERAL_TASK_TIMEOUT);
			break;

		case CMD_GET_DEVICE_MODE:
			saverMngrMsg.msgId = GoTransmitReplyPacket;
			saverMngrMsg.data1 = COMM_MSG_GET_DEVICE_MODE_REPLY;
			xQueueSend(saver->TaskCmdNotify, &saverMngrMsg, GENERAL_TASK_TIMEOUT);
			break;

		//

		case CMD_GET_OVERLAYLED:
			saverMngrMsg.msgId = GoTransmitReplyPacket;
			saverMngrMsg.data1 = COMM_MSG_GET_OVERLAYLED_REPLY;
			xQueueSend(saver->TaskCmdNotify, &saverMngrMsg, GENERAL_TASK_TIMEOUT);
			break;

		case CMD_SET_OVERLAYLED_PATTERN:
			LedSettingProcessCommData(saver, &usbRxTxDataBuffer[DATA_START_OFFSET]);
			saverMngrMsg.msgId = GoTransmitAckPacket;
			saverMngrMsg.data1 = CMD_SET_OVERLAYLED_PATTERN;
			xQueueSend(saver->TaskCmdNotify, &saverMngrMsg, GENERAL_TASK_TIMEOUT);
			break;

		case CMD_SET_OVERLAYLED_TEST_PATTERN:


			saverMngrMsg.msgId = GoTransmitAckPacket;
			saverMngrMsg.data1 = CMD_SET_OVERLAYLED_TEST_PATTERN;
			xQueueSend(saver->TaskCmdNotify, &saverMngrMsg, GENERAL_TASK_TIMEOUT);
			break;



		default:
			break;
	}
}

//
//
//


#define		ARMING_TIME					(100)
#define		ARMED_CONFIRM_TIME			(20)
#define		DIS_ARMING_TIME				(20)
#define		DIS_ARMING_CONFIRM_TIME		(5)

int armCountDown = 0;

void	ButtonRunStopProcess(SaverInterface *saver)
{
	SaverSystemMessage		saverMngrMsg;

//	switch(saver->ArmState)
//	{
//		case ARM_INIT:
//
//			break;
//
//		case ARM_IDLE:
//			LedSettingClearAllDisplay(saver);
//			//LedSettingDisplay(saver, LedPressTilt, LedColorRed, LedTwiddler, pdTRUE);
//			LedSettingDisplay(saver, LedRunStop, LedColorGreen, LedDoublePulse, pdTRUE);
//
//			armCountDown = ARMING_TIME;
//			saver->ArmState = ARM_ARMING;
//
//			break;
//
//		case ARM_ARMING:
//		case ARM_ARMED_CONFIRM:
//
//			// Can be disarmed immediately
//
//			LedSettingClearAllDisplay(saver);
//			//LedSettingDisplay(saver, LedPressTilt, LedColorRed, LedTwiddler, pdTRUE);
//			LedSettingDisplay(saver, LedRunStop, LedColorAmber, LedOn, pdFALSE);
//
//			armCountDown = DIS_ARMING_TIME;
//			saver->ArmState = ARM_DIS_ARMING;
//			break;
//
//		case ARM_ARMED:
//
//			LedSettingClearAllDisplay(saver);
//			//LedSettingDisplay(saver, LedPressTilt, LedColorRed, LedTwiddler, pdTRUE);
//
//			armCountDown = DIS_ARMING_CONFIRM_TIME;
//			saver->ArmState = ARM_DIS_ARMING_CONFIRM;
//
//			break;
//
//		case ARM_DIS_ARMING:
//			LedSettingClearAllDisplay(saver);
//			//LedSettingDisplay(saver, LedPressTilt, LedColorRed, LedTwiddler, pdTRUE);
//			LedSettingDisplay(saver, LedRunStop, LedColorGreen, LedDoublePulse, pdTRUE);
//
//			armCountDown = ARMING_TIME;
//			saver->ArmState = ARM_ARMING;
//			break;
//
//		case ARM_DIS_ARMING_CONFIRM:
//
//			break;
//
//		case ARM_DISARMED:
//
//			break;
//	}
}

//
//

void	SaverManagerRunTimeout(SaverInterface *saver)
{
//	switch(saver->ArmState)
//	{
//		case ARM_INIT:
//
//			break;
//
//		case ARM_IDLE: // Disarmed
//
//			break;
//
//		case ARM_ARMING:
//			armCountDown--;
//
//			if(armCountDown == 0)
//			{
//				LedSettingClearAllDisplay(saver);
//				//LedSettingDisplay(saver, LedPressTilt, LedColorRed, LedTwiddler, pdTRUE);
//				LedSettingDisplay(saver, LedRunStop, LedColorGreen, LedOn, pdTRUE);
//
//				armCountDown = ARMED_CONFIRM_TIME;
//				saver->ArmState = ARM_ARMED_CONFIRM;
//			}
//			break;
//
//		case ARM_ARMED_CONFIRM:
//			armCountDown--;
//
//			if(armCountDown == 0)
//			{
//				LedSettingClearAllDisplay(saver);
//				//LedSettingDisplay(saver, LedPressTilt, LedColorRed, LedTwiddler, pdTRUE);
//				//LedSettingDisplay(saver, LedShock, LedColorRed, LedTwiddler, pdTRUE);
//				LedSettingDisplay(saver, LedRunStop, LedColorGreen, LedStrobe, pdTRUE);
//
//				saver->ArmState = ARM_ARMED;
//			}
//			break;
//
//		case ARM_ARMED:
//
//			break;
//
//		case ARM_DIS_ARMING_CONFIRM:
//			armCountDown--;
//
//			if(armCountDown == 0)
//			{
//				LedSettingClearAllDisplay(saver);
//				//LedSettingDisplay(saver, LedPressTilt, LedColorRed, LedTwiddler, pdTRUE);
//				LedSettingDisplay(saver, LedRunStop, LedColorAmber, LedOn, pdTRUE);
//
//				armCountDown = DIS_ARMING_TIME;
//				saver->ArmState = ARM_DISARMED;
//			}
//			break;
//
//		case ARM_DIS_ARMING:
//			armCountDown--;
//
//			if(armCountDown == 0)
//			{
//				LedSettingClearAllDisplay(saver);
//				//LedSettingDisplay(saver, LedPressTilt, LedColorRed, LedTwiddler, pdTRUE);
//				//LedSettingDisplay(saver, LedTempRh, LedColorRed, LedTwiddler, pdTRUE);
//
//				saver->ArmState = ARM_IDLE;
//			}
//			break;
//
//		case ARM_DISARMED:
//			armCountDown--;
//
//			if(armCountDown == 0)
//			{
//				LedSettingClearAllDisplay(saver);
//				//LedSettingDisplay(saver, LedPressTilt, LedColorRed, LedTwiddler, pdTRUE);
//				//LedSettingDisplay(saver, LedTempRh, LedColorRed, LedTwiddler, pdTRUE);
//
//				saver->ArmState = ARM_IDLE;
//			}
//			break;
//	}
}

//
//
//

static void	SaverManagerMachineProcess(SaverInterface *saver, SaverSystemMessage *saverMngrMsg)
{
//	uint16_t	currentSmState;
//
//	uint16_t	currentMessageData1 = saverMngrMsg->data1;
//	uint16_t	currentMessageData2 = saverMngrMsg->data2;
//
//    	currentSmState = saver->TaskManagerState;
//
//        //currentDestinationSmId  = currentMessage->smDestinationId;
//		//currentSourceSmId       = currentMessage->smSourceId;
//
//        currentMessageData1 = currentMessage->messageData1;
//        currentMessageData2 = currentMessage->messageData2;
//
//        //
//        // Point to the state machine matrix entry
//        //
//
//        responseEntry  =
//			(void **)smEventHandlerEntries[currentDestinationSmId];
//
//		currentEventHandlerSelect = pgm_read_word(responseEntry + currentSmState);
//
//		eventHandlerSelectData = pgm_read_dword(currentEventHandlerSelect);
//
//		thisEventId = (U16)(eventHandlerSelectData);
//		thisExitProcedure = (void *)(eventHandlerSelectData >> 16);
//
//
//        while ( TRUE )
//        {
//            // search for the state matrix for matching event id
//
//		    if ( thisEventId == currentMessage->messageId )
//            {
//                // call the exit procedure
//
//				newState = thisExitProcedure();
//                smStates[currentDestinationSmId] = newState;
//
//                break;
//            }
//
//			if( thisEventId == NULL_MESSAGE_ID )
//            {
//                //DoHandleLostEvent();
//
//                break;
//            }
//
//            // select the next in the event handler entries
//
//			currentEventHandlerSelect += sizeof(unsigned long);
//
//			eventHandlerSelectData = pgm_read_dword(currentEventHandlerSelect);
//
//			thisEventId = (U16)(eventHandlerSelectData);
//			thisExitProcedure = (void *)(eventHandlerSelectData >> 16);
//
//        }
}


typedef enum
{
	Idle 	= 0x0001,
	Active
} DAH_STATES;


NEW_STATE	DAH_exitA(void)
{

	return Idle;
}

NEW_STATE	DAH_exitB(void)
{

	return Active;
}


STATE_TRANSITION_MATRIX(_DAH_IDLE)
EV_HANDLER(TimeOut, DAH_exitA),
EV_HANDLER(TimeOut, DAH_exitB)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_DAH_RUNNING)
EV_HANDLER(TimeOut, DAH_exitA),
EV_HANDLER(TimeOut, DAH_exitB)
STATE_TRANSITION_MATRIX_END;


SM_RESPONSE_ENTRY(DAH_Main_Entry)
	STATE(_DAH_IDLE),
	STATE(_DAH_RUNNING),
SM_RESPONSE_END


//void * eh;
//
//eh = (void *)DAH_Main_Entry;
//
//if(eh)
//	eh = (void *)0x0100;


//
//
//

//#define	BATT_TEST_0		0
//#define	BATT_TEST_1		1
//#define	BATT_TEST_2		2
//#define	BATT_TEST_3		3
//#define	BATT_TEST_4		4
//#define	BATT_TEST_5		5
//#define	BATT_TEST_6		6
//
//int		battTestState;
//int		buttonPushed = pdFALSE;

// Tracker TEST Code was in Task loop

//success = UARTStringSend(saver->Devices.UART1, UART_TX_TIMEOUT, "AT?\r");

// Signal Strength
//success =UARTStringSend(saver->Devices.UART1, UART_TX_TIMEOUT, "at+csq\r");

//
// Initialize GPS
//				success =UARTStringSend(saver->Devices.UART1, UART_TX_TIMEOUT, "AT$GPSNMUN=1,0,0,0,0,1,0\r");
//				vTaskDelay(500);
//				len = TRACKER_STRING_LEN;
//				UARTBufferReceive(saver->Devices.UART1, 500, trackerString, &len);
//
//				// Enable GPS
//				success =UARTStringSend(saver->Devices.UART1, UART_TX_TIMEOUT, "AT$GPSP=1\r");
//				vTaskDelay(500);
//				len = TRACKER_STRING_LEN;
//				UARTBufferReceive(saver->Devices.UART1, 500, trackerString, &len);
//
//				for(gpsCount=0; gpsCount < 100; gpsCount++)
//				{
//					vTaskDelay(500);
//					len = TRACKER_STRING_LEN;
//					UARTBufferReceive(saver->Devices.UART1, 500, trackerString, &len);
//
//					vTaskDelay(5000);
//				}

// Disable GPS
//success =UARTStringSend(saver->Devices.UART1, UART_TX_TIMEOUT, "AT$GPSP=0\r");


//
// Send SMS
//				success =UARTStringSend(saver->Devices.UART1, UART_TX_TIMEOUT, "AT+CMGF=1\r");
//				vTaskDelay(500);
//				len = TRACKER_STRING_LEN;
//				UARTBufferReceive(saver->Devices.UART1, 500, trackerString, &len);
//
//				success =UARTStringSend(saver->Devices.UART1, UART_TX_TIMEOUT, "AT+CMGS=4084275066\r");
//				vTaskDelay(500);
//				len = TRACKER_STRING_LEN;
//				UARTBufferReceive(saver->Devices.UART1, 500, trackerString, &len);
//
//				success =UARTStringSend(saver->Devices.UART1, UART_TX_TIMEOUT, "Hello From Lansmont\x1A\r");
//				vTaskDelay(500);
//				len = TRACKER_STRING_LEN;
//				UARTBufferReceive(saver->Devices.UART1, 500, trackerString, &len);

//				vTaskDelay(500);
//
//				if(success)
//				{
//					len = TRACKER_STRING_LEN;
//
//					UARTBufferReceive(saver->Devices.UART1, 500, trackerString, &len);
//
//					memset(trackerString, 0x00, TRACKER_STRING_LEN);
//
//					//UARTFlush(saver->Devices.UART1, 5000);
//
//					portNOP();


//					while (UARTReceive(saver->Devices.UART1, 1000, &ch))
//					{
//						portNOP();
//					}
//				}


// Battery TEST CODE, was in Engineering Mode

//	if(buttonState == BUTTON_ON)
//		buttonPushed = pdTRUE;
//
//	if(buttonPushed)
//	{
//		buttonPushed = pdFALSE;
//
//		battTestState++;
//
//		if(battTestState > BATT_TEST_6)
//			battTestState = BATT_TEST_0;
//	}
//
//	switch(battTestState)
//	{
//		case BATT_TEST_0:
//			LedSettingClearAllDisplay(saver);
//			LedSettingDisplay(saver, LedRunStop, LedColorGreen, LedStrobe, pdTRUE);
//
//			ChargerDisable();
//			DischargerDisable();
//
//			break;
//
//		case BATT_TEST_1:
//			LedSettingClearAllDisplay(saver);
//			LedSettingDisplay(saver, LedRunStop, LedColorAmber, LedFlash, pdTRUE);
//
//			ChargerEnable();
//			ChargeRateSet(CHARGE_TRICKLE_100_MA);
//			DischargerDisable();
//
//			break;
//
//		case BATT_TEST_2:
//			LedSettingClearAllDisplay(saver);
//			LedSettingDisplay(saver, LedBattery, LedColorAmber, LedFlash, pdTRUE);
//
//			ChargerEnable();
//			ChargeRateSet(CHARGE_NORMAL_500_MA);
//			DischargerDisable();
//
//			break;
//
//		case BATT_TEST_3:
//			LedSettingClearAllDisplay(saver);
//			LedSettingDisplay(saver, LedNetwork, LedColorAmber, LedFlash, pdTRUE);
//
//			ChargerEnable();
//			ChargeRateSet(CHARGE_FAST_800_MA);
//			DischargerDisable();
//
//			break;
//
//		case BATT_TEST_4:
//			LedSettingClearAllDisplay(saver);
//			LedSettingDisplay(saver, LedShock, LedColorRed, LedFlash, pdTRUE);
//
//			ChargerEnable();
//			ChargeRateSet(CHARGE_SUSPEND_0_MA);
//			DischargerDisable();
//
//			break;
//
//		case BATT_TEST_5:
//			LedSettingClearAllDisplay(saver);
//			LedSettingDisplay(saver, LedTempRh, LedColorRed, LedFlash, pdTRUE);
//
//			ChargerDisable();
//			ChargeRateSet(CHARGE_SUSPEND_0_MA);
//			DischargerEnable();
//
//			break;
//
//		case BATT_TEST_6:
//			LedSettingClearAllDisplay(saver);
//			LedSettingDisplay(saver, LedPressTilt, LedColorRed, LedFlash, pdTRUE);
//
//			ChargerDisable();
//			ChargeRateSet(CHARGE_SUSPEND_0_MA);
//			DischargerDisable();
//
//			break;
//	}




