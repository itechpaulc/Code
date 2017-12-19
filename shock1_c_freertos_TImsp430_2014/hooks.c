//----------------------------------------------------------------------------
// File : hooks.c
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

// RTOS includes
#include "FreeRTOS.h"
#include "task.h"

// Application includes
#include "board.h"

//----------------------------------------------------------------------------
// vApplicationSetupTimerInterrupt
//
// Configure timer used for the tick interrupt.
//
// In:
//   nothing
//
// Out:
//   nothing
//
// Returns:
//   nothing
//----------------------------------------------------------------------------
void vApplicationSetupTimerInterrupt( void )
{
#if 1
	// Ensure the timer is stopped
	TA0CTL = 0;

	// Run the timer from the ACLK (divided by 1)
	TA0CTL = TASSEL__ACLK;

	// Clear everything to start with
	TA0CTL |= TACLR;

	// Set the compare match value according to the tick rate we want
	TA0CCR0 = configACLK_FREQUENCY_HZ / configTICK_RATE_HZ;

	// Enable the interrupts
	TA0CCTL0 = CCIE;

	// Start up clean
	TA0CTL |= TACLR;

	// Up mode
	TA0CTL |= MC_1;
#else	// For testing, the following uses SMCLK for a more accurate timer tick
	// Ensure the timer is stopped
	TA0CTL = 0;

	// Run the timer from the SMCLK (divided by 8)
	TA0CTL = TASSEL__SMCLK | ID__8;

	// Clear everything to start with
	TA0CTL |= TACLR;

	// Set the compare match value according to the tick rate we want
	TA0CCR0 = ((configSMCLK_FREQUENCY_HZ / configTICK_RATE_HZ) >> 3) - 1;

	// Enable the interrupts
	TA0CCTL0 = CCIE;

	// Start up clean
	TA0CTL |= TACLR;

	// Up mode
	TA0CTL |= MC_1;
#endif
}

#if configCHECK_FOR_STACK_OVERFLOW
//----------------------------------------------------------------------------
// vApplicationStackOverflowHook
//
// FreeRTOS hook for debugging and catching stack overflows.
//
// In:
//   task     : handle to task in which the stack overflowed
//   taskName : name given to task when it was created
//
// Out:
//   nothing
//
// Returns:
//   pdTRUE if successful, pdFALSE if not
//----------------------------------------------------------------------------
void vApplicationStackOverflowHook(
	xTaskHandle		*task,
	signed portCHAR	*taskName)
{
	(void) task;
	(void) taskName;

	taskDISABLE_INTERRUPTS();

	// Dead loop for debugging and catching stack overflows
	// (if the watchdog is enabled, a reset will occur!)
	for (;;);
}
#endif

#if configUSE_IDLE_HOOK
//----------------------------------------------------------------------------
// vApplicationIdleHook
//
// FreeRTOS hook called when the system is idle.
//
// In:
//   nothing
//
// Out:
//   nothing
//
// Returns:
//   nothing
//----------------------------------------------------------------------------

void vApplicationIdleHook(void)
{
	// Called on each iteration of the idle task.
	// In this case the idle task just enters a
	// low(ish) power mode.

	__bis_SR_register(LPM0_bits + GIE);

	// Lower Power Mode
	//__bis_SR_register(LPM3_bits + GIE);

	// Required only for debugger
	portNOP();
}
#endif

#if configUSE_TICK_HOOK
//----------------------------------------------------------------------------
// vApplicationTickHook
//
// FreeRTOS hook called each timer tick.
//
// In:
//   nothing
//
// Out:
//   nothing
//
// Returns:
//   nothing
//----------------------------------------------------------------------------
void vApplicationTickHook()
{
	const Pin	pinTP2 = TP2_PIN;

	PioToggleFromISR(&pinTP2);
}
#endif

#if configUSE_MALLOC_FAILED_HOOK
//----------------------------------------------------------------------------
// vApplicationMallocFailedHook
//
// FreeRTOS hook called if pvPortMalloc() fails because there is
// insufficient free memory available in the FreeRTOS heap.
//
// In:
//   nothing
//
// Out:
//   nothing
//
// Returns:
//   nothing
//----------------------------------------------------------------------------
void vApplicationMallocFailedHook(void)
{
	// Called if pvPortMalloc() fails because there is insufficient
	// free memory available in the FreeRTOS heap.  This is mostly
	// for debug to set a breakpoint and catch problems at startup
	// since pvPortMalloc is only used on this platform during
	// initialization of all tasks and drivers.
	taskDISABLE_INTERRUPTS();
	for(;;);
}
#endif
