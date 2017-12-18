



/*
 *
 *
 *		$Header:   K:/Projects/Tcmr/Source/Isr.c_v   1.3   11 Jul 2003 14:48:26   PaulLC  $
 *		$Log:   K:/Projects/Tcmr/Source/Isr.c_v  $
 * 
 *    Rev 1.3   11 Jul 2003 14:48:26   PaulLC
 * Incorporated all changes since 0.60.B; Camera Trigger jumping fixes; 
 * Encoder noise filtering; Changes to supporte latest TCMRC HW; Limit switch configure.
 * 
 *    Rev 1.2   Apr 08 2002 14:31:16   PaulLC
 * Contains all of the changes made during beta development.
 * 
 *    Rev 1.1   Aug 27 2001 15:48:12   PaulLC
 * Changes made up to Engineering Version 00.25.A.
 * 
 *    Rev 1.0   Oct 06 2000 14:27:22   PaulLC
 * Checked in from initial workfile by PVCS Version Manager Project Assistant.
 * 
 *
 *		Author : Paul Calinawan
 *
 *			May 2000
 *
 *			Graphics Microsystems Inc
 *			1284 Forgewood Ave
 *			Sunnyvale, CA 94089
 *
 *			(408) 745-7745
 *
 *
 *		Print Quick Camera Control Module
 *	-------------------------------------------
 *
 *
*/

/////////////////////////////////////////////////////////////////////////////
//
//    NOTE:
//
//    This document contains CONFIDENTIAL and proprietary information
//    which is the property of Graphics Microsystems, Inc. It may not
//    be copied or transmitted in whole or in part by any means to any
//    media without Graphics Microsystems Inc's prior written permission.
//
/////////////////////////////////////////////////////////////////////////////



#include "tcmr.h"
#include "kernel.h"


//////////////////////////////////////////////////
//
// Private Data
//
//////////////////////////////////////////////////

WORD			currKernelTimerTag;

signed long		tempInputCaptureTimeTag;

WORD			prevInputCaptureTimeTag;
WORD			currInputCaptureTimeTag;

BYTE			encoderPeriodFilterType;
DWORD			encoderPeriodFilter;

WORD			instantaneousEncoderPeriod;
WORD			currEncoderPeriod;
WORD			prevEncoderPeriod;
BYTE			encoderTick;

BYTE			pcMessageReceiveCheckSum;

BYTE			xdata	toPcInterruptByte		_at_    0xBFE;
BYTE			xdata	fromPcInterruptByte		_at_    0xBFF;


//////////////////////////////////////////////////
//
// ISR Local Functions
// Forward declaration
//
//////////////////////////////////////////////////

void	DoWebEncoderTdcPulseEvent(void);

void	DoTopCameraTriggerEvent(void);

void	DoBottomCameraTriggerEvent(void);

void	DoWebEncoderPulseEvent(void);

void	DoKernelTimerEvent(void);


//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

void	InitISR(void)
{
	SetWebEncoderPeriodFilterType
		(TYPE_25_FILTER);
}

//////////////////////////////////////////////////
//
// Interrupt Handlers
//
//////////////////////////////////////////////////

//
//           INTERRUPT VECTORS
//
//       LOCAT   TYPE   CC#  Program Location
//
//       00     poweron     POWER ON RESET
//       03     ext 0    0  MOTOR CHIPSET INTERRUPT
//       0B     tmr 0    1  ----
//       13     ext 1    2  PC MESSAGE AVAIL (DUAL PORT RAM)
//       1B     tmr 1    3  ----
//       23     serial   4  ----
//       2B     tmr 2    5  ----
//       33     pca      6  TIME BASED EVENTS
//


//////////////////////////////////////////////////
//
// Head Transport
// Motor ChipSet Control
//
// Using register bank 0
//
//////////////////////////////////////////////////

External0_Handler(void) interrupt 0 using 0 
{ 
	DISABLE_MOTOR_CHIPSET_INTERRUPT();

	// Tell Kernel about interrupt event

		SET_MOTOR_CHIPSET_EVENT();
}

//////////////////////////////////////////////////
//
// UNUSED
//
//////////////////////////////////////////////////

Timer0_Handler(void) interrupt 1 { }


//////////////////////////////////////////////////
//
// PC Communications
//
// TCMR RECEIVE INTERRUPT
//
// PC Message Data Available
//
// Using register bank 1
//
//////////////////////////////////////////////////

External1_Handler(void) interrupt 2 using 1
{
	// Clear Interrupt Flag by reading the checksum byte
	// This will reset the mailbox

	pcMessageReceiveCheckSum = ReadFromPcInterruptByte();

		// Tell Kernel about interrupt event

		SET_PC_MESSAGE_RECEIVED_EVENT();
}


//////////////////////////////////////////////////
//
// UNUSED
//
//////////////////////////////////////////////////

Timer1_Handler(void) interrupt 3 { }


//////////////////////////////////////////////////
//
// UNUSED
//
//////////////////////////////////////////////////

SerialPort_Handler(void) interrupt 4 { }


//////////////////////////////////////////////////
//
// UNUSED
//
//////////////////////////////////////////////////

Timer2_Handler(void) interrupt 5 { }


//////////////////////////////////////////////////
//
//	PCA handler
//
//  Compare 0   - processes Encoder TDC Pulse
//
//  Compare 1   - toggles the Top Camera Flash
//
//  Capture 2   - toggles the Bottom Camera Flash
//
//  Compare 3   - processes Encoder A/B Pulse
//
//  Compare 4   - Kernel Timer
//
//  Using register bank 1
//
//////////////////////////////////////////////////

PCA_Handler(void) interrupt 6 using 2
{
	if(CCF0 == SET)
	{ 
		CCF0 = CLEAR;	
		DoWebEncoderTdcPulseEvent();
		SET_WEB_ENCODER_TDC_EVENT();			// Tell Kernel about 
												// Top Dead Center IRQ event
	}

	if(CCF1 == SET)
	{
		CCF1 = CLEAR;	
		DoTopCameraTriggerEvent();
		SET_TOP_CAMERA_TRIGGER_EVENT();			// Tell Kernel about 
												// TRIG IRQ event
	}

	if(CCF2 == SET)
	{
		CCF2 = CLEAR;
		DoBottomCameraTriggerEvent();
		SET_BOTTOM_CAMERA_TRIGGER_EVENT();		// Tell Kernel about
												// TRIG IRQ event
	}

	if(CCF3 == SET)
	{
		CCF3 = CLEAR;
		DoWebEncoderPulseEvent();
		SET_WEB_ENCODER_PULSE_EVENT();			// Tell Kernel about
												// Web Encoder Pulse IRQ event
	}

	if(CCF4 == SET)
	{
		CCF4 = CLEAR;
		DoKernelTimerEvent();
		SET_KERNEL_TIMER_EVENT();				// Tell Kernel about
												// Timer Elapsing IRQ event
	}

    // if(CF == SET)							// Not Used

}



//////////////////////////////////////////////////
//
// Web Encoder TDC Pulse Handling
//
//	Input Capture Trigger
//
//////////////////////////////////////////////////

void	DoWebEncoderTdcPulseEvent(void) { }


//////////////////////////////////////////////////
//
// Top
//	Camera Shutter and Flash Trigger Handling
//	Also used for Flash Recharge Timer
//
//	Output Compare Trigger
//
//////////////////////////////////////////////////

void	DoTopCameraTriggerEvent(void) { }


//////////////////////////////////////////////////
//
// BOTTOM
//	Camera Shutter and Flash Trigger Handling
//	Also used for Flash Recharge Timer
//
//	Output Compare Trigger
//
//////////////////////////////////////////////////

void	DoBottomCameraTriggerEvent(void) { }


//////////////////////////////////////////////////
//
// Web Encoder Pulse Handling
//
//	Input Capture Trigger
//
//////////////////////////////////////////////////

void	DoWebEncoderPulseEvent(void)
{
	++encoderTick;

	currInputCaptureTimeTag = (CCAP3H << 8) | CCAP3L;

	tempInputCaptureTimeTag =
		(long)currInputCaptureTimeTag - (long)prevInputCaptureTimeTag;

	if( tempInputCaptureTimeTag < 0 )
		tempInputCaptureTimeTag += 0x10000;

	instantaneousEncoderPeriod = (WORD)tempInputCaptureTimeTag;

	prevInputCaptureTimeTag = currInputCaptureTimeTag;

	switch(encoderPeriodFilterType)
	{
		case TYPE_NO_FILTER:
			currEncoderPeriod = instantaneousEncoderPeriod;
			break;

		case TYPE_93_FILTER:	/* 93% curr 7% prev */
			encoderPeriodFilter = 
				((instantaneousEncoderPeriod * 15) + prevEncoderPeriod);
			currEncoderPeriod = encoderPeriodFilter >> 4;
			break;

		case TYPE_75_FILTER:	/* 75% curr 25% prev */
			encoderPeriodFilter = 
				((instantaneousEncoderPeriod * 3) + prevEncoderPeriod);
			currEncoderPeriod = encoderPeriodFilter >> 2;
			break;

		case TYPE_50_FILTER:	/* 50% curr 50% prev */
			encoderPeriodFilter = 
				(instantaneousEncoderPeriod + prevEncoderPeriod);
			currEncoderPeriod = encoderPeriodFilter >> 1;
			break;

		case TYPE_25_FILTER:	/* 25% curr 75% prev */
			encoderPeriodFilter = 
				((prevEncoderPeriod * 3) + instantaneousEncoderPeriod);
			currEncoderPeriod =	encoderPeriodFilter >> 2; 
			break;

		case TYPE_7_FILTER:		/* 7% curr 93% prev */
			encoderPeriodFilter = 
				((prevEncoderPeriod * 15) + instantaneousEncoderPeriod);
			currEncoderPeriod = encoderPeriodFilter >> 4;
			break;
	}

	prevEncoderPeriod = currEncoderPeriod;
}

#pragma disable
void	DecrementEncoderTick(void) { --encoderTick; }

#pragma disable
BYTE	GetEncoderTick(void) { return encoderTick; }




//////////////////////////////////////////////////
//
// Kernel Timer Handling
//
//	Output Compare Trigger
//
//////////////////////////////////////////////////

#define		TIMER_WRAP_AROUND_READ_THRESH	(0xFFFF - TIMER_READ_THRESH)
#define		MAX_CPU_TIME_READ_RETRY			2
#define		TIMER_READ_THRESH				16

#define		GET_CURR_CPU_TIME()				((CH << 8) | CL)

// PCA Tick = 10 microsecond 
// Using 100 Khz Clock Reference on ECI Port

#define	KERNEL_TICK_4_MSEC	0x190

void	DoKernelTimerEvent(void) { }

BYTE	cpuTimeReadRetry;
WORD	cpuTime1, cpuTime2;

void	ReStartKernelTimer(void)
{
	cpuTimeReadRetry = 0;

		do // Wrap around check
		{	
			cpuTime1 = GET_CURR_CPU_TIME();
			cpuTime2 = GET_CURR_CPU_TIME();

			if(((cpuTime2-cpuTime1) < TIMER_READ_THRESH) ||
				((cpuTime1-cpuTime2) > TIMER_WRAP_AROUND_READ_THRESH))
					break;

			++cpuTimeReadRetry;

		}while(cpuTimeReadRetry < MAX_CPU_TIME_READ_RETRY);

		currKernelTimerTag = cpuTime1 + KERNEL_TICK_4_MSEC;

	CCAP4L = LOBYTE(currKernelTimerTag);
	CCAP4H = HIBYTE(currKernelTimerTag);
}

