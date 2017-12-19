//----------------------------------------------------------------------------
// File : saver_ctrl.c
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
// 1.00 06/24/13 Brandon W. Initial revision.
//----------------------------------------------------------------------------

// Standard includes
#include <stdint.h>
#include <string.h>
#include <time.h>
#ifdef GCC_MSP430
#include <legacymsp430.h>
#endif

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

//

#include "saver_overlayhandler.h"

#include "saver_cmdhandler.h"

#include "init.h"

#include "saverstatemachine.h"

//

#include "usbcomm.h"

#include "command.h"

#include "saver_manager.h"



//
//
//

#include "../USB_config/descriptors.h"

#include "../USB_API/USB_Common/defMSP430USB.h"
#include "../USB_API/USB_Common/usb.h"

#include "../USB_API/USB_Common/UsbIsr.h"

#include "../USB_API/USB_CDC_API/UsbCdc.h"


/*----------------------------------------------------------------------------+
| External Variables                                                          |
+----------------------------------------------------------------------------*/

extern __no_init tEDB0 __data16 tEndPoint0DescriptorBlock;

extern WORD wUsbEventMask;

//

// Need to keep private global pointer to the Saver interface for ISRs to use
static SaverInterface *saverInstrumentUsb = NULL;

//

/*----------------------------------------------------------------------------+
| General Subroutines                                                         |
+----------------------------------------------------------------------------*/
#pragma vector=USB_UBM_VECTOR
__interrupt void iUsbInterruptHandler(void)
{
	portBASE_TYPE	wakeHigherPriorityTask	= pdFALSE;

	SaverInterface	*saver					= saverInstrumentUsb;

	SaverSystemMessage						saverUsbMsg;

	//

	// TODO:
	// if(saver) ??
	//

	uint8_t bWakeUp = FALSE;

    //Check if the setup interrupt is pending.
    //We need to check it before other interrupts,
    //to work around that the Setup Int has lower priority then Input Endpoint 0
    if (USBIFG & SETUPIFG)
    {
        bWakeUp = SetupPacketInterruptHandler();
#ifdef USB10_WORKAROUND
        USBIEPCNF_0 &= ~EPCNF_UBME; // Clear ME to gate off SETUPIFG clear event
        USBOEPCNF_0 &= ~EPCNF_UBME; // Clear ME to gate off SETUPIFG clear event
#endif
        USBIFG &= ~SETUPIFG;    // clear the interrupt bit
#ifdef USB10_WORKAROUND
       USBIEPCNF_0 |= EPCNF_UBME; // Set ME to continue with normal operation
       USBOEPCNF_0 |= EPCNF_UBME; // Set ME to continue with normal operation
#endif
    }

    switch (__even_in_range(USBVECINT & 0x3f, USBVECINT_OUTPUT_ENDPOINT7))
    {
		case USBVECINT_NONE:
		  break;
		case USBVECINT_PWR_DROP:
		  __no_operation();
		  break;
		case USBVECINT_PLL_LOCK:
		  break;
		case USBVECINT_PLL_SIGNAL:
		  break;
		case USBVECINT_PLL_RANGE:
		  if (wUsbEventMask & kUSB_clockFaultEvent)
		  {
			  bWakeUp = USB_handleClockEvent();
		  }
		  break;
		case USBVECINT_PWR_VBUSOn:
		  PWRVBUSonHandler();
		  if (wUsbEventMask & kUSB_VbusOnEvent)
		  {
			  bWakeUp = USB_handleVbusOnEvent();
		  }
		  break;
		case USBVECINT_PWR_VBUSOff:
		  PWRVBUSoffHandler();
		  if (wUsbEventMask & kUSB_VbusOffEvent)
		  {
			  bWakeUp = USB_handleVbusOffEvent();
		  }
		  break;
		case USBVECINT_USB_TIMESTAMP:
		  break;

		  // INPUT and OUTPUT
		  // ENDPOINT0 USB - SETUPS / ENUMERATION etc

		case USBVECINT_INPUT_ENDPOINT0:
		  IEP0InterruptHandler();
		  break;

		case USBVECINT_OUTPUT_ENDPOINT0:
		  OEP0InterruptHandler();
		  break;

		  //

		case USBVECINT_RSTR:
		  USB_reset();
		  if (wUsbEventMask & kUSB_UsbResetEvent)
		  {
			  bWakeUp = USB_handleResetEvent();
		  }
		  break;
		case USBVECINT_SUSR:
		  USB_suspend();
		  if (wUsbEventMask & kUSB_UsbSuspendEvent)
		  {
			  bWakeUp = USB_handleSuspendEvent();
		  }
		  break;
		case USBVECINT_RESR:
		  USB_resume();
		  if (wUsbEventMask & kUSB_UsbResumeEvent)
		  {
			  bWakeUp = USB_handleResumeEvent();
		  }
		  //-- after resume we will wake up! Independ what event handler says.
		  bWakeUp = TRUE;
		  break;

		case USBVECINT_SETUP_PACKET_RECEIVED:
		  // NAK both IEP and OEP endpoints
		  tEndPoint0DescriptorBlock.bIEPBCNT = EPBCNT_NAK;
		  tEndPoint0DescriptorBlock.bOEPBCNT = EPBCNT_NAK;
		  SetupPacketInterruptHandler();
		  break;

		case USBVECINT_STPOW_PACKET_RECEIVED:
		  break;
		case USBVECINT_INPUT_ENDPOINT1:
		  break;

		  // ENDPOINT2 - USB TRANSMIT

		case USBVECINT_INPUT_ENDPOINT2:
		  //send saved bytes from buffer...
		  bWakeUp = CdcToHostFromBuffer(CDC0_INTFNUM);
				break;

		//

		case USBVECINT_INPUT_ENDPOINT3:
		  break;
		case USBVECINT_INPUT_ENDPOINT4:
		  break;
		case USBVECINT_INPUT_ENDPOINT5:
		  break;
		case USBVECINT_INPUT_ENDPOINT6:
		  break;
		case USBVECINT_INPUT_ENDPOINT7:
		  break;
		case USBVECINT_OUTPUT_ENDPOINT1:
		  break;

		  // ENDPOINT2 - USB RECEIVE

		case USBVECINT_OUTPUT_ENDPOINT2:
		  //call callback function if no receive operation is underway
		  if (!CdcIsReceiveInProgress(CDC0_INTFNUM) && USBCDC_bytesInUSBBuffer(CDC0_INTFNUM))
		  {
			  if (wUsbEventMask & kUSB_dataReceivedEvent)
			  {
				  bWakeUp = USBCDC_handleDataReceived(CDC0_INTFNUM);

				  saverUsbMsg.msgId = UsbPacketReceived;

				  // Todo: check for return
				  // portYIELD_FROM_ISR() from isr, final process
				  xQueueSendFromISR(saver->TaskCmdNotify, &saverUsbMsg, &wakeHigherPriorityTask);
			  }
		  }
		  else
		  {
			  //complete receive opereation - copy data to user buffer
			  bWakeUp = CdcToBufferFromHost(CDC0_INTFNUM);

			  saverUsbMsg.msgId = UsbPacketReceived;

			  // Todo: check for return
			  // portYIELD_FROM_ISR() from isr
			  xQueueSendFromISR(saver->TaskCmdNotify, &saverUsbMsg, &wakeHigherPriorityTask);
		  }
		  break;

		  //

		case USBVECINT_OUTPUT_ENDPOINT3:
		  break;
		case USBVECINT_OUTPUT_ENDPOINT4:
		  break;
		case USBVECINT_OUTPUT_ENDPOINT5:
		  break;
		case USBVECINT_OUTPUT_ENDPOINT6:
		  break;
		case USBVECINT_OUTPUT_ENDPOINT7:
		  break;

		default:
		  break;
		}

    if (bWakeUp)
    {
    	// Todo make same as original saver...
    	//__bis_SR_register(LPM0_bits + GIE);

         __bic_SR_register_on_exit(LPM3_bits);   // Exit LPM0-3
         __no_operation();                       // Required for debugger
    }
}

//
// Packet handling
//

uint16_t	PacketChecksumGet(uint8_t *ptrBuff, uint16_t buffLen)
{
	uint16_t 	chksum, idx;
	uint8_t		dat;

		chksum = 0x0000;

		for(idx=0; idx < buffLen; idx++)
		{
			dat = ptrBuff[idx];
			chksum += dat;
		}

	return chksum;
}

uint8_t		PacketChecksumValidate(void)
{
	uint16_t chksumCalc = 0x0000;
	uint16_t msgChksum = 0x0000;

		chksumCalc = PacketChecksumGet(usbRxTxDataBuffer, hostMessageRxByteCount-2);

		msgChksum = (uint16_t)(usbRxTxDataBuffer[hostMessageRxByteCount-2]);

		if(msgChksum == chksumCalc)
			return pdTRUE;

	return pdFALSE;
}

uint8_t		PacketDeviceIdValidate(void)
{
	// Todo : device id is a config item
	uint16_t deviceId = 0x0000;

	uint16_t msgDeviceId = 0x0000;

		msgDeviceId = (uint16_t)(usbRxTxDataBuffer[0]);

		if(msgDeviceId == deviceId)
			return pdTRUE;

	return pdFALSE;
}

void	UsbPacketReceivedProcess(SaverInterface *saver)
{
	SaverSystemMessage		saverMngrMsg;

	if(packetReceived)
	{
		//
		// Validate Packet
		if(PacketDeviceIdValidate())
		{
			if(PacketChecksumValidate())
			{
				saverMngrMsg.msgId = GoodPacketReceived;
				xQueueSend(saver->TaskManagerNotify, &saverMngrMsg, GENERAL_TASK_TIMEOUT);
			}
			else
			{
				// BAD Packet Received
			}
		}
	}
}

//

void GoTransmitAckPacketProcess(SaverSystemMessage *saverCmdMsg)
{
	TxAckReplyBuild(saverCmdMsg->data1);
}


void GoTransmitReplyPacketProcess(SaverInterface *saver, SaverSystemMessage *saverCmdMsg)
{
	switch(saverCmdMsg->data1)
	{
		case COMM_MSG_GET_CURR_ACCEL_DATA_REPLY:
			TxGetCurrAccelDataReplySend(saver);
			break;

		case COMM_MSG_GET_FW_VERSION_REPLY:
			TxGetFwVersionReplySend();
			break;

		case COMM_MSG_GET_DEVICE_MODE_REPLY:
			TxGetDeviceModeReplySend(saver);
			break;

		case COMM_MSG_GET_DATE_TIME_REPLY:
			TxGetCurrDateTimeReplySend(saver);
			break;

		case COMM_MSG_GET_OVERLAYLED_REPLY:
			TxGetOverlayLedReplySend(saver);
			break;

		default:
			break;
	}

}

//

void	SaverCmdHandlerActiveProcess(SaverInterface *saver, SaverSystemMessage	*saverCmdMsg);

//----------------------------------------------------------------------------
// SaverCmdHdlrTask
//
// Saver command processing loop.  Right now this is just for debug and
// experimenting with the hardware.  Later this could be where external
// commands are processed from USB or the external serial port.
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
portTASK_FUNCTION(SaverCmdHndlrTask, parameters)
{
	SaverHandle		handle		= (SaverHandle) parameters;
	SaverInterface	*saver		= (SaverInterface*) parameters;

	SaverSystemMessage		saverCmdMsg;

	// Save private global pointer to Saver interface (needed for ISRs)
	saverInstrumentUsb = saver;

	// Perform late hardware initialization that can only be completed
	// after starting the task scheduler.
	SaverDevInitLate(handle);


	// All other Saver tasks can be resumed after completing the late
	// hardware initialization

	if (saver->TaskDataAcquire)
		vTaskResume(saver->TaskDataAcquire);

	if (saver->TaskEventStore)
		vTaskResume(saver->TaskEventStore);

	if (saver->TaskManager)
	{
		saver->TaskManagerState = MNGR_Run;
		vTaskResume(saver->TaskManager);
	}

	if (saver->TaskOverlay)
	{
		saver->TaskManagerState = MNGR_Run;
		vTaskResume(saver->TaskOverlay);
	}

	saver->TaskCmdHndlrState = SaverCmdHandlerActive;

	//

		for (;;)
		{
			// Wait for notification of data ready
			if (xQueueReceive(saver->TaskCmdNotify, &saverCmdMsg, portMAX_DELAY) == pdPASS)
			{
				switch(saver->TaskCmdHndlrState)
				{
					case SaverCmdHandlerActive:
						SaverCmdHandlerActiveProcess(saver, &saverCmdMsg);
						break;

					default:
						break;
				}
			}
		}
}

//
//
//

void	SaverCmdHandlerActiveProcess(SaverInterface *saver, SaverSystemMessage	*saverCmdMsg)
{
		switch(saverCmdMsg->msgId)
		{
			// Do Receive

			case UsbPacketReceived:
				UsbProcess();
				UsbPacketReceivedProcess(saver);
				break;

			// Do Transmit

			case GoTransmitReplyPacket:
				GoTransmitReplyPacketProcess(saver, saverCmdMsg);
				break;

			case GoTransmitAckPacket:
				GoTransmitAckPacketProcess(saverCmdMsg);
				break;

			default:
				break;

		}
}




