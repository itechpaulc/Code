


//----------------------------------------------------------------------------
// File : command.c
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

// RTOS includes
#include "FreeRTOS.h"
#include "task.h"

//

#include "usbcomm.h"

#include "command.h"

//

#include "saver_overlayhandler.h"

//

#include "../USB_config/descriptors.h"

#include "../USB_app/usbConstructs.h"


//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

void	TxHeaderBuild(uint16_t cmdId, uint16_t dataLength)
{
	// Todo : device id is a config item

	uint16_t deviceId = 0x0000;

	usbRxTxDataBuffer[DEV_ID_OFFSET_0] = LOW_BYTE(deviceId);
	usbRxTxDataBuffer[DEV_ID_OFFSET_1] = HIGH_BYTE(deviceId);

	usbRxTxDataBuffer[CMD_ID_OFFSET_0] = LOW_BYTE(cmdId);
	usbRxTxDataBuffer[CMD_ID_OFFSET_1] = HIGH_BYTE(cmdId);

	usbRxTxDataBuffer[DATA_LENGTH_OFFSET_0] = LOW_BYTE(dataLength);
	usbRxTxDataBuffer[DATA_LENGTH_OFFSET_1] = HIGH_BYTE(dataLength);
}

void	TxPacketChecksumBuild(uint16_t dataLength)
{
	uint16_t 	chksum, c;
	uint8_t		dat;

	uint16_t	packetLength = dataLength + DATA_START_OFFSET;

		chksum = 0x0000;

		for(c=0; c < packetLength; c++)
		{
			dat = usbRxTxDataBuffer[c];

			chksum += dat;
		}

		usbRxTxDataBuffer[c] = LOW_BYTE(chksum);
		usbRxTxDataBuffer[c+1] = HIGH_BYTE(chksum);
}

//----------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------

void	TxPacketDataBuild(uint16_t dataWordLength, uint16_t *dataStartPtr)
{
	uint16_t 	i = 0,
				c = DATA_START_OFFSET,
				datByteLen = dataWordLength * 2;

	uint16_t	*dataPtr = (uint16_t *)dataStartPtr;

		if(datByteLen > NO_DATA_LENGHT)
		{
			for(i=0; i < datByteLen; i++)
			{
				usbRxTxDataBuffer[c] = LOW_BYTE(*dataPtr);
				usbRxTxDataBuffer[c+1] = HIGH_BYTE(*dataPtr);

				c += 2;

				dataPtr++;
			}
		}

	TxPacketChecksumBuild(datByteLen);
}

//----------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------

void TxPacketTransmit(uint16_t dataLength)
{
	uint8_t 	sendError = pdFALSE;

	uint16_t	packetLength;

				dataLength = dataLength * 2;

				packetLength = DATA_START_OFFSET + dataLength +  + CHKSUM_LENGTH;

		if (cdcSendDataInBackground
				((uint8_t*)usbRxTxDataBuffer, packetLength, CDC0_INTFNUM, 1))
		{
			// flag that something went wrong.

			sendError = pdTRUE;
		}

		if(sendError == pdFALSE)
		{

		}
		else
		{


		}
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

#define	CMD_GET_CURR_ACCEL_DATA_LENGTH	2

void	TxTxGetCurrAccelInfoReplySend(SaverInterface *saver)
{
	uint16_t	accelInfo[CMD_GET_CURR_ACCEL_DATA_LENGTH] = {};

	TxHeaderBuild(CMD_GET_CURR_ACCEL_DATA_FULL, CMD_GET_CURR_ACCEL_DATA_LENGTH);

		accelInfo[0] = saver->AccelScanData.RingBufIndex;
		accelInfo[1] = saver->AccelScanData.BufIndex;

	TxPacketDataBuild(CMD_GET_CURR_ACCEL_DATA_LENGTH, accelInfo);

	TxPacketTransmit(CMD_GET_CURR_ACCEL_DATA_LENGTH);
}

void	TxTxGetCurrAccelDataReplySend(SaverInterface *saver)
{
	uint8_t SendError = FALSE;

	uint16_t saverAccelTxByteCount = (ADC_CHNL_SAMPLES_MAX_COUNT * 2); // 2 bytes per sample

		if (cdcSendDataInBackground
				((uint8_t*)(&saver->AccelScanData.Ring), saverAccelTxByteCount, CDC0_INTFNUM, 1))
		{
			// flag that something went wrong.

			SendError = TRUE;
		}

		if(SendError == FALSE)
		{

		}
		else
		{

		}
}

void	TxGetCurrAccelDataReplySend(SaverInterface *saver)
{
	TxTxGetCurrAccelInfoReplySend(saver);

	TxTxGetCurrAccelDataReplySend(saver);
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

#define CMD_GET_VERSION_DATA_LENGTH			(4)

void	TxGetFwVersionReplySend(void)
{
	uint16_t	fwVersion[CMD_GET_VERSION_DATA_LENGTH] = {};

	TxHeaderBuild(CMD_GET_VERSION, CMD_GET_VERSION_DATA_LENGTH);

		fwVersion[0] = FW_REV_MAJOR;
		fwVersion[1] = FW_REV_MINOR;
		fwVersion[2] = FW_REV_BUILD;
		fwVersion[3] = FW_REV_REVISION;

	TxPacketDataBuild(CMD_GET_VERSION_DATA_LENGTH, fwVersion);

	TxPacketTransmit(CMD_GET_VERSION_DATA_LENGTH);
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

#define	CMD_GET_DEVICE_MODE_DATA_LENGTH		(1)

void	TxGetDeviceModeReplySend(SaverInterface *saver)
{
	uint16_t	devMode = saver->Mode;

		TxHeaderBuild(CMD_GET_DEVICE_MODE, CMD_GET_DEVICE_MODE_DATA_LENGTH);

		TxPacketDataBuild(CMD_GET_DEVICE_MODE_DATA_LENGTH, &devMode);

		TxPacketTransmit(CMD_GET_DEVICE_MODE_DATA_LENGTH);
}


//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

#define	CMD_GET_DATE_TIME_DATA_WORD_LENGTH	(sizeof(int64_t) / 2)

void	TxGetCurrDateTimeReplySend(SaverInterface *saver)
{
	int64_t		secondsEpoch1970;
	uint16_t 	*secondsEpoch1970ptr = (uint16_t *)&secondsEpoch1970;

		RTCTimeGet(saver->Devices.RTC, &secondsEpoch1970, RTC_TIMEOUT);

		TxHeaderBuild(CMD_GET_CURR_DATE_TIME, CMD_GET_DATE_TIME_DATA_WORD_LENGTH);

		TxPacketDataBuild(CMD_GET_DATE_TIME_DATA_WORD_LENGTH, secondsEpoch1970ptr);

		TxPacketTransmit(CMD_GET_DATE_TIME_DATA_WORD_LENGTH);
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

#define		LED_PARAM_WORD_COUNT					(sizeof(LedSettingType) / 2)
#define		CMD_GET_OVERLAYLED_DATA_WORD_LENGTH		(LED_DISPLAY_COUNT * LED_PARAM_WORD_COUNT)

void	TxGetOverlayLedReplySend(SaverInterface *saver)
{
	TxHeaderBuild(CMD_GET_OVERLAYLED, CMD_GET_OVERLAYLED_DATA_WORD_LENGTH);

	TxPacketDataBuild(CMD_GET_OVERLAYLED_DATA_WORD_LENGTH, (uint16_t *)(saver->LedSettingSystem));

	TxPacketTransmit(CMD_GET_OVERLAYLED_DATA_WORD_LENGTH);
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

#define	CMD_ACK_LENGTH			(1)

void	TxAckReplyBuild(uint16_t cmdId)
{
	uint16_t	ack = CMD_ACK;

	TxHeaderBuild(cmdId, CMD_ACK_LENGTH);

	TxPacketDataBuild(CMD_ACK_LENGTH, &ack);

	TxPacketTransmit(CMD_ACK_LENGTH);
}

