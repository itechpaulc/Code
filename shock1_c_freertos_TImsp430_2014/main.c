
//----------------------------------------------------------------------------
// File : main.c
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

// RTOS includes
#include "FreeRTOS.h"
#include "task.h"

//

#include "init.h"

#include "usbcomm.h"

//
//
//
// Todo : for constant string embed in object code

#define	COPYRIGHT_TEXT	\
			"This document contains CONFIDENTIAL and proprietary \
			information which is the property of Lansmont.\
			It may not be copied or transmitted in whole or in \
			part by any means to any media without Lansmonts's \
			prior written permission."

const char FW_COPYRIGHT[] =	COPYRIGHT_TEXT;

//----------------------------------------------------------------------------
// main
//
// Perform any remaining hardware initialization that is needed.
// Starts tasks and the scheduler.
//
// In:
//   nothing
//
// Out:
//   nothing
//
// Returns:
//   nothing (control is taken by the scheduler, and once it is started, it
//            does not return)
//----------------------------------------------------------------------------

int main(void)
{
	// Perform remaining processor and hardware initialization required
	// before setting up peripheral devices

	SaverProcessorInit();

	ProcessorMemInit();


	UsbProcessInit();


	// Initialize the Saver

	if (SaverInit())
		vTaskStartScheduler();

		// Critical initialization failed

		// TODO: set LEDs for error or go into a deep sleep state instead
		//       of going into a dead loop

		for (;;)
		{
			// TODO:
			//WatchdogHit();
			portNOP();
		}
}
