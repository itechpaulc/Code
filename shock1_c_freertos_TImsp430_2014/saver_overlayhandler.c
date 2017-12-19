//----------------------------------------------------------------------------
// File : saver_ledhandler.c
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
// 1.01 08/01/13 Brandon W. Implemented better coordination with other
//                          tasks to ensure steps in a repeating LED
//                          sequence are dropped or queued correctly
//                          after the sequence has been reprogrammed.
// 1.00 06/24/13 Brandon W. Initial revision.
//----------------------------------------------------------------------------

// Standard includes
#include <stdint.h>
#include <string.h>

// RTOS includes
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

// Application includes
#include "board.h"
#include "pio.h"

#include "saver_overlayhandler.h"

#include "saverstatemachine.h"



//
//

static	void 	ButtonProcess(SaverInterface *saver);

static	void 	LedProcess(SaverInterface *saver);


//

void	LedSettingDisplay(SaverInterface *saver, LedDisplayIdx ledDispIdx,
			LedDisplayColor color, LedPatternIdx patIdx, uint8_t rpt);

void	LedSettingClearDisplay(SaverInterface *saver, LedDisplayIdx ledDispIdx);

void	LedSettingClearAllDisplay(SaverInterface *saver);

void 	LedLoadTestPattern(SaverInterface *saver);



// Timeout for commands performed during initialization
#define LED_TIMEOUT					((portTickType) 25 / portTICK_RATE_MS)

//#define LED_TASK_STACK_SIZE			configMINIMAL_STACK_SIZE

//

const Pin		LedPinsArray[]	= { LED_PINS };

//

#define	LED_PATTERN_COUNT 			LedPatternEndIdx

#define	LED_BYTE_PATTERN_COUNT		24

const uint8_t	LedPatterns[LED_PATTERN_COUNT][LED_BYTE_PATTERN_COUNT] =
{
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // Off

		{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, // On

		{ 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
				0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00 }, // Flash

		{ 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0xF0, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Double Pulse

		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0}, // Strobe

		{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Short On

		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x55},  // Multi Strobe

		{ 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
				0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55},  // Twiddler
		//

		{ 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // TestPat1

		{ 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // TestPat2

		{ 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // TestPat3

		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // TestPat4

		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // TestPat5

		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // TestPat6

		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // TestPat7

		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // TestPat8

		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00 } // TestPat9

};

#define		OVERLAY_BUTTON_IDLE			0x00
#define		OVERLAY_BUTTON_VALIDATE		0x01
#define		OVERLAY_BUTTON_WAIT_OFF		0x02

//
//
//

static	void 	LedInitialize(SaverInterface *saver);

static	void 	LedBitProcess(SaverInterface *saver, LedIdx lIdx);

static	void	LedLoadPattern(SaverInterface *saver, LedIdx ledIdx, LedPatternIdx patIdx, uint8_t rpt);

static	void 	LedLoadPatternOff(SaverInterface *saver, LedIdx ledIdx);

static	void 	LedAllClearPattern(SaverInterface *saver);

//

static	void 	LedLoadTest(SaverInterface *saver);

//
// TODO: need to integrate button with overlay files
// Need to keep private global pointer to the Saver interface for ISRs to use

SaverInterface *saverInstrumentButton = NULL;

// Todo: put in saver structure...

uint8_t		overlayButtonState = OVERLAY_BUTTON_IDLE;
uint8_t		overlayButtonLatchCount = 0;


//----------------------------------------------------------------------------
// OverlayTask
//
// Task for executing an LED update sequence.
//
// In:
//   parameters :
//
// Out:
//   nothing
//
// Returns:
//   nothing
//----------------------------------------------------------------------------
portTASK_FUNCTION(SaverOverlayTask, parameters)
{
	SaverInterface	*saver	= (SaverInterface*) parameters;

	SaverSystemMessage		saverCmdMsg;



	saverInstrumentButton = saver;

	//

	LedInitialize(saver);

	//LedLoadTest(saver);
	//LedLoadTestPattern(saver);
	LedSettingDisplay(saver, LedShock, LedColorRed, LedTwiddler, pdTRUE);
	LedSettingDisplay(saver, LedTempRh, LedColorRed, LedTwiddler, pdTRUE);
	LedSettingDisplay(saver, LedPressTilt, LedColorRed, LedTwiddler, pdTRUE);

	for(;;)
	{

		if (xQueueReceive(saver->TaskOverlayNotify, &saverCmdMsg, LED_TIMEOUT) == pdPASS)
		{
			// Process Command

			if(saverCmdMsg.msgId == ButtonDepressed)
			{
				overlayButtonState = OVERLAY_BUTTON_VALIDATE;
				overlayButtonLatchCount = 0;
			}
			else
			if(saverCmdMsg.msgId == ButtonReleased)
			{
				overlayButtonState = OVERLAY_BUTTON_IDLE;
				overlayButtonLatchCount = 0;
			}
		}
		else
		{
			// Timeout Handling

			ButtonProcess(saver);

			LedProcess(saver);
		}
	}
}

//

#define LED_START_MASK		0x01

void LedBitProcess(SaverInterface *saver, LedIdx lIdx)
{
	uint8_t patternByte, patternBitMask, byteCount;

	LedType 		*ledsPtr = saver->LedsSystem;

	PatternSetting	*patSet = &(ledsPtr[lIdx].LedPatSet);

		// Current Bit Mask shift and reset

		patSet->CurrBitMask <<= 1;

		if(patSet->CurrBitMask == 0)
		{
			patSet->CurrBitMask = LED_START_MASK;

				// Point to Next Byte, and Repeat check

				patSet->CurrByte++;

				if(patSet->CurrByte == LED_BYTE_PATTERN_COUNT)
				{
					if(patSet->Repeat == pdFALSE)
					{
						LedLoadPatternOff(saver, lIdx);
					}
					else
					{
						patSet->CurrByte = 0;
					}
				}
		}

		// Pattern Load

		byteCount = patSet->CurrByte;
		patternByte = *(patSet->PatternStartPtr + byteCount);
		patternBitMask = patSet->CurrBitMask;

		if(patternBitMask & patternByte)
		{
			if(patSet->IsPioActivated == pdFALSE)
			{
				PIOActivate(ledsPtr[lIdx].LedPin);
				patSet->IsPioActivated = pdTRUE;
			}
		}
		else
		{
			if(patSet->IsPioActivated == pdTRUE)
			{
				PIODeactivate(ledsPtr[lIdx].LedPin);
				patSet->IsPioActivated = pdFALSE;
			}
		}
}

//

#define	OVERLAY_BUTTON_DEBOUNCE_COUNT 	4

void ButtonProcess(SaverInterface *saver)
{
	SaverSystemMessage	overlayHandlrMsg;

	if(overlayButtonState == OVERLAY_BUTTON_IDLE)
		return;

	if(overlayButtonState == OVERLAY_BUTTON_VALIDATE)
	{
		overlayButtonLatchCount++;

		if(overlayButtonLatchCount > OVERLAY_BUTTON_DEBOUNCE_COUNT)
		{
			overlayHandlrMsg.msgId = ButtonRunStopDetect;

			xQueueSend(saver->TaskManagerNotify, &overlayHandlrMsg, GENERAL_TASK_TIMEOUT);

			overlayButtonState = OVERLAY_BUTTON_WAIT_OFF;
		}
	}
	else
	if(overlayButtonState == OVERLAY_BUTTON_WAIT_OFF)
	{

	}
}

//

void LedProcess(SaverInterface *saver)
{
	LedIdx 		ledIdx;

	for(ledIdx=LedRunStopGreen; ledIdx < LedEndIdx; ledIdx++ )
	{
		if(saver->LedsSystem[ledIdx].LedPatSet.IsActive)
			LedBitProcess(saver, ledIdx);
	}
}

//
//
//

void LedInitialize(SaverInterface *saver)
{
	LedType *ledsPtr = saver->LedsSystem;;

	LedIdx ledIdx;

		for(ledIdx=LedRunStopGreen; ledIdx < LedEndIdx; ledIdx++ )
		{
			ledsPtr->LedPin = &LedPinsArray[ledIdx];

			PIOConfigure(&LedPinsArray[ledIdx], 1);

			ledsPtr++;
		}

	LedAllClearPattern(saver);

	LedSettingClearAllDisplay(saver);
}

//
//
//

void LedAllClearPattern(SaverInterface *saver)
{
	LedIdx lIdx;

		for(lIdx=LedRunStopGreen; lIdx < LedEndIdx; lIdx++ )
		{
			LedLoadPatternOff(saver, lIdx);
		}
}

//
//
//

void	LedLoadPattern(SaverInterface *saver, LedIdx ledIdx, LedPatternIdx patIdx, uint8_t rpt)
{
		LedType *ledsPtr = saver->LedsSystem + ledIdx;

		ledsPtr->LedPatSet.IsActive = pdTRUE;
		ledsPtr->LedPatSet.Repeat = rpt;

		ledsPtr->LedPatSet.PatternStartPtr = (uint8_t *)(&LedPatterns[patIdx]);
		ledsPtr->LedPatSet.CurrByte = 0;
		ledsPtr->LedPatSet.CurrBitMask = LED_START_MASK;

		ledsPtr->LedPatSet.IsPioActivated = pdFALSE;
}

//
//
//

void 	LedLoadPatternOff(SaverInterface *saver, LedIdx ledIdx)
{
	LedType *ledsPtr = saver->LedsSystem + ledIdx;

		ledsPtr->LedPatSet.IsActive = pdTRUE;
		ledsPtr->LedPatSet.Repeat = pdFALSE;

		ledsPtr->LedPatSet.PatternStartPtr = (uint8_t *)(&LedPatterns[LedOff]);
		ledsPtr->LedPatSet.CurrByte = 0;
		ledsPtr->LedPatSet.CurrBitMask = LED_START_MASK;

		PIODeactivate(ledsPtr->LedPin);

		ledsPtr->LedPatSet.IsPioActivated = pdFALSE;
}

//
//
//
//
//void 	LedLoadTest(SaverInterface *saver)
//{
//		LedLoadPattern(saver, LedRunStopAmber, LedMultiStrobe, pdTRUE);
//		LedLoadPattern(saver, LedRunStopGreen, LedShortOn, pdTRUE);
//
//		LedLoadPattern(saver, LedBatteryAmber, LedStrobe, pdTRUE);
//		LedLoadPattern(saver, LedBatteryGreen, LedFlash, pdTRUE);
//
//		LedLoadPattern(saver, LedNetworkAmber, LedDoublePulse, pdTRUE);
//		LedLoadPattern(saver, LedNetworkGreen, LedOn, pdTRUE);
//
//		LedLoadPattern(saver, LedShockRed, LedFlash, pdTRUE);
//		LedLoadPattern(saver, LedTempRhRed, LedStrobe, pdTRUE);
//		LedLoadPattern(saver, LedPressTiltRed, LedMultiStrobe, pdTRUE);
//
//
//	LedSettingDisplay(saver, LedRunStop, LedColorAmber, LedShortOn, pdTRUE);
//	LedSettingDisplay(saver, LedBattery, LedColorGreen, LedMultiStrobe, pdTRUE);
//	LedSettingDisplay(saver, LedNetwork, LedColorGreen, LedOn, pdTRUE);
//
//	LedSettingDisplay(saver, LedShock, LedColorRed, LedStrobe, pdTRUE);
//	LedSettingDisplay(saver, LedTempRh, LedColorRed, LedFlash, pdTRUE);
//	LedSettingDisplay(saver, LedPressTilt, LedColorRed, LedDoublePulse, pdTRUE);
//
//	LedSettingDisplay(LedRunStop, LedColorGreen, LedStrobe, pdTRUE);
//}

void 	LedLoadTestPattern(SaverInterface *saver)
{
	LedLoadPattern(saver, LedRunStopAmber, LedTestPat1, pdTRUE);
	LedLoadPattern(saver, LedRunStopGreen, LedTestPat2, pdTRUE);

	LedLoadPattern(saver, LedBatteryAmber, LedTestPat3, pdTRUE);
	LedLoadPattern(saver, LedBatteryGreen, LedTestPat4, pdTRUE);

	LedLoadPattern(saver, LedNetworkAmber, LedTestPat5, pdTRUE);
	LedLoadPattern(saver, LedNetworkGreen, LedTestPat6, pdTRUE);

	LedLoadPattern(saver, LedShockRed, LedTestPat7, pdTRUE);
	LedLoadPattern(saver, LedTempRhRed, LedTestPat8, pdTRUE);
	LedLoadPattern(saver, LedPressTiltRed, LedTestPat9, pdTRUE);


	saver->LedSettingSystem[LedRunStop].PatternId = LedTestPat1;
	saver->LedSettingSystem[LedBattery].PatternId = LedTestPat2;
	saver->LedSettingSystem[LedNetwork].PatternId = LedTestPat3;
	saver->LedSettingSystem[LedShock].PatternId = LedTestPat4;
	saver->LedSettingSystem[LedTempRh].PatternId = LedTestPat5;
	saver->LedSettingSystem[LedPressTilt].PatternId = LedTestPat6;
}

//
//
//
void	LedSettingProcessCommData(SaverInterface *saver, uint8_t *lDispStartAddr)
{
	LedSettingType		ledSetting;
	LedDisplayIdx 	ledDispIdx;

		LedSettingClearAllDisplay(saver);

		for(ledDispIdx=LedRunStop; ledDispIdx < LedDisplayEndIdx; ledDispIdx++ )
		{
			memcpy(&ledSetting,	lDispStartAddr, sizeof(LedSettingType));

			LedSettingDisplay(saver, ledDispIdx,
					ledSetting.Color, ledSetting.PatternId,
					ledSetting.Repeat);

			lDispStartAddr += sizeof(LedSettingType);
		}
}

void	LedSettingDisplay
			(SaverInterface *saver, LedDisplayIdx ledDispIdx, LedDisplayColor color,
				LedPatternIdx patIdx, uint8_t rpt)
{
	saver->LedSettingSystem[ledDispIdx].Color = color;
	saver->LedSettingSystem[ledDispIdx].PatternId = patIdx;
	saver->LedSettingSystem[ledDispIdx].Repeat = rpt;

	// Handle Exclusive Colors

	switch(ledDispIdx)
	{
		case LedRunStop:
			if(color == LedColorGreen)
			{
				LedLoadPattern(saver, LedRunStopGreen, patIdx, rpt);
				LedLoadPatternOff(saver, LedRunStopAmber);
			}
			else
			{
				LedLoadPattern(saver, LedRunStopAmber, patIdx, rpt);
				LedLoadPatternOff(saver, LedRunStopGreen);
			}
		break;

		case LedBattery:
			if(color == LedColorGreen)
			{
				LedLoadPattern(saver, LedBatteryGreen, patIdx, rpt);
				LedLoadPatternOff(saver, LedBatteryAmber);
			}
			else
			{
				LedLoadPattern(saver, LedBatteryAmber, patIdx, rpt);
				LedLoadPatternOff(saver, LedBatteryGreen);
			}
		break;

		case LedNetwork:
			if(color == LedColorGreen)
			{
				LedLoadPattern(saver, LedNetworkGreen, patIdx, rpt);
				LedLoadPatternOff(saver, LedNetworkAmber);
			}
			else
			{
				LedLoadPattern(saver, LedNetworkAmber, patIdx, rpt);
				LedLoadPatternOff(saver, LedNetworkGreen);
			}
		break;

		case LedShock:
			LedLoadPattern(saver, LedShockRed, patIdx, rpt);
			break;

		case LedTempRh:
			LedLoadPattern(saver, LedTempRhRed, patIdx, rpt);
			break;

		case LedPressTilt:
			LedLoadPattern(saver, LedPressTiltRed, patIdx, rpt);
			break;

	}
}


void	LedSettingClearDisplay(SaverInterface *saver, LedDisplayIdx ledDispIdx)
{
	saver->LedSettingSystem[ledDispIdx].Color = LedColorOff;
	saver->LedSettingSystem[ledDispIdx].PatternId = LedOff;
	saver->LedSettingSystem[ledDispIdx].Repeat = pdFALSE;
}

void	LedSettingClearAllDisplay(SaverInterface *saver)
{
	LedDisplayIdx ledDispIdx;

		for(ledDispIdx=LedRunStop; ledDispIdx < LedDisplayEndIdx; ledDispIdx++ )
		{
			LedSettingClearDisplay(saver, ledDispIdx);
		}

	LedAllClearPattern(saver);
}



