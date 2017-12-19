//----------------------------------------------------------------------------
// File : delay.c
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

// RTOS includes
#include "FreeRTOS.h"

// Application includes
#include "delay.h"

#define CYCLES_PER_US		(configCPU_CLOCK_HZ / 1000000)

//----------------------------------------------------------------------------
// DelayUS
//
// Delay the specified number of microseconds.
//
// NOTE:
// This routine currently does not use timers.  It is a crude delay
// that delays a number of cycles based on the MCLK for the CPU.
// Obviously, this delay is not very accurate, and interrupts are
// not disabled either. This means a task switch could even occur
// while delaying.  Consider this delay requested as a minimum number
// of microseconds to wait.
//
// In:
//   count : number of microseconds to delay
//
// Out:
//   nothing
//
// Returns:
//   nothing
//----------------------------------------------------------------------------
void DelayUS(uint16_t count)
{
	while (count > 0)
	{
		--count;
		__delay_cycles(CYCLES_PER_US);
	}
}
