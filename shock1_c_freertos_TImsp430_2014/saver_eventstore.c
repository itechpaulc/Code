//----------------------------------------------------------------------------
// File : saver_eventstore.c
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
// 1.00 08/17/13 Brandon W. Initial revision.
//----------------------------------------------------------------------------

// Standard includes
#include <stdint.h>
#include <string.h>
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
#include "saver_eventstore.h"

#include "saverinterface.h"

//----------------------------------------------------------------------------
// SaverEventStoreTask
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
portTASK_FUNCTION(SaverEventStoreTask, parameters)
{
	portTickType	start	= xTaskGetTickCount();
	SaverInterface	*saver	= (SaverInterface*) parameters;
	SaverEvent		event;

	int16_t			data[ADC_CHNL_SAMPLES_ACQUIRE_PER_NOTIFY * ADC_CHNL_COUNT];
	uint16_t		dataRemaining;
	uint16_t		dataReadLength;

	for (;;)
	{
		// Wait for notification of event ready
		if (xQueueReceive(saver->TaskEventNotify, &event, portMAX_DELAY) == pdPASS)
		{
			dataRemaining = event.DataLength;

			while (dataRemaining > 0)
			{
				--dataRemaining;
			}

//			while (dataRemaining > 0)
//			{
//				if (dataRemaining > sizeof(data))
//					dataReadLength = sizeof(data);
//				else
//					dataReadLength = dataRemaining;
//
//				// TODO: NO Longer writing to FRAM
//				// But directly from MCU Upper memory to FLASH
//
//				if (FRAM25XRead(saver->Devices.FRAM,
//					event.DataOffset, (uint8_t*)(data),
//					sizeof(data), FRAM_READ_TIMEOUT))
//				{
//					dataRemaining -= dataReadLength;
//
//					// TODO: some type of delay is needed here,
//					//       depending on the timing and how long
//					//       reads take, doing multiple can interfere
//					//       with the task which copies samples from
//					//       the ring buffer to FRAM enough where it
//					//       could fall behind and the ring buffer
//					//       will overrun it
//
//					// 3 ms works well at 5000 samples/sec.
//
//					// NOTE:
//					//   On Rev B of the board, flash will be on a
//					//   separate SPI bus, and it just so happens
//					//   that a flash page write can take up to 3 ms.
//					//   Nice... that's exactly what we need to be
//					//   doing during this time as well.
//					vTaskDelay(((portTickType) 3 / portTICK_RATE_MS));
//				}
//				else
//				{
//					// TODO: handle FRAM read error...
//					portNOP();
//				}
//			}
		}
	}
}
