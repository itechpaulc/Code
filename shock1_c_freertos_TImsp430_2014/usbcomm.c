

//----------------------------------------------------------------------------
// File : usbcomm.c
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


/* --COPYRIGHT--,BSD
 * Copyright (c) 2013, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
/*
 * ======== main.c ========
 * Local Echo Demo:
 *
 * This example simply echoes back characters it receives from the host.
 * Unless the terminal application has a built-in echo feature turned on,
 * typing characters into it only causes them to be sent; not displayed locally.
 * This application causes typing in Hyperterminal to feel like typing into any
 * other PC application – characters get displayed.
 *
 * ----------------------------------------------------------------------------+
 * Please refer to the Examples Guide for more details.
 * ---------------------------------------------------------------------------*/

#include <string.h>

#include "../driverlib/MSP430F5xx_6xx/inc/hw_memmap.h"

#include "../driverlib/MSP430F5xx_6xx/gpio.h"

#include "../driverlib/MSP430F5xx_6xx/ucs.h"

#include "../driverlib/MSP430F5xx_6xx/sfr.h"

#include "../driverlib/MSP430F5xx_6xx/timer_a.h"

#include "../USB_config/descriptors.h"

#include "../USB_API/USB_Common/device.h"
#include "../USB_API/USB_Common/types.h"               // Basic Type declarations
#include "../USB_API/USB_Common/usb.h"                 // USB-specific functions
#include "../USB_API/USB_CDC_API/UsbCdc.h"

#include "../USB_app/usbConstructs.h"

#include "../driverlib/MSP430F5xx_6xx/pmm.h"

//

#include "saver.h"

#include "usbcomm.h"

#include "command.h"



/*
 * NOTE: Modify hal.h to select a specific evaluation board and customize for
 * your own board.
 */




// Global flags set by events
//volatile uint8_t bCDCDataReceived_event = pdFALSE; // Flag set by event handler to
                                               	   // indicate data has been
                                               	   // received into USB buffer

volatile uint8_t bCDCDataReceived_event = 0; 		// Flag set by event handler to
                                               	   // indicate data has been
                                               	   // received into USB buffer


uint8_t 	usbRxTxDataBuffer[USB_RX_TX_BUFFER_SIZE] = "";

char nl[2] = "\n";

uint8_t 	packetReceived;

uint16_t	hostMessageRxByteCount;
uint16_t	saverMessageTxByteCount;

/*
 * ======== UNMI_ISR ========
 */

#pragma vector = UNMI_VECTOR
__interrupt void UNMI_ISR (void)
{
    switch (__even_in_range(SYSUNIV, SYSUNIV_BUSIFG ))
    {
        case SYSUNIV_NONE:
            __no_operation();
            break;

        case SYSUNIV_NMIIFG:
            __no_operation();
            break;

        case SYSUNIV_OFIFG:
            UCS_clearFaultFlag(UCS_BASE, UCS_XT2OFFG);
            UCS_clearFaultFlag(UCS_BASE, UCS_DCOFFG);
            SFR_clearInterrupt(SFR_BASE, SFR_OSCILLATOR_FAULT_INTERRUPT);
            break;

        case SYSUNIV_ACCVIFG:
            __no_operation();
            break;

        case SYSUNIV_BUSIFG:
            // If the CPU accesses USB memory while the USB module is
            // suspended, a "bus error" can occur.  This generates an NMI.  If
            // USB is automatically disconnecting in your software, set a
            // breakpoint here and see if execution hits it.  See the
            // Programmer's Guide for more information.
            SYSBERRIV = 0; // clear bus error flag
            USB_disable(); // Disable
    }
}

/*----------------------------------------------------------------------------+
 | Init Routine                                                                |
 +----------------------------------------------------------------------------*/

void UsbProcessInit(void)
{
	//PMM_setVCore(PMM_BASE, PMMCOREV_2);

    USB_setup(TRUE, TRUE); // Init USB & events; if a host is present, connect

    //__enable_interrupt();  // Enable interrupts globally
}

/*----------------------------------------------------------------------------+
 | Main Routine                                                                |
 +----------------------------------------------------------------------------*/

void UsbProcess(void)
{
	uint8_t ReceiveError = 0,
			SendError = 0;

	WORD rxCount;

        // Check the USB state and directly main loop accordingly
        switch (USB_connectionState())
        {
            // This case is executed while your device is enumerated on the
            // USB host
            case ST_ENUM_ACTIVE:

                // Sleep if there are no bytes to process.
                __disable_interrupt();

                if (!USBCDC_bytesInUSBBuffer(CDC0_INTFNUM))
                {
                    // Enter LPM0 until awakened by an event handler

                    //__bis_SR_register(LPM0_bits + GIE);
                }

                __enable_interrupt();

                // Exit LPM because of a data-receive event, and
                // fetch the received data

                if (bCDCDataReceived_event)
                {
                    // Clear flag early -- just in case execution breaks
                    // below because of an error
                    bCDCDataReceived_event = FALSE;

                    rxCount = cdcReceiveDataInBuffer
                    			((uint8_t*)usbRxTxDataBuffer, USB_RX_TX_BUFFER_SIZE, CDC0_INTFNUM);

                    packetReceived = 1;
                    hostMessageRxByteCount = rxCount;
                }

                break;

            // These cases are executed while your device is disconnected from
            // the host (meaning, not enumerated); enumerated but suspended
            // by the host, or connected to a powered hub without a USB host
            // present.
            case ST_PHYS_DISCONNECTED:
            case ST_ENUM_SUSPENDED:
            case ST_PHYS_CONNECTED_NOENUM_SUSP:
                //__bis_SR_register(LPM3_bits + GIE);
                _NOP();
                break;

            // The default is executed for the momentary state
            // ST_ENUM_IN_PROGRESS.  Usually, this state only last a few
            // seconds.  Be sure not to enter LPM3 in this state; USB
            // communication is taking place here, and therefore the mode must
            // be LPM0 or active-CPU.
            case ST_ENUM_IN_PROGRESS:

            default:
            	;
        }

        if (ReceiveError || SendError)
        {
            // TO DO: User can place code here to handle error
        }
}

//
//
//

void SaverMessageTransmit(void)
{
	uint8_t SendError = FALSE;

	if (cdcSendDataInBackground
			((uint8_t*)usbRxTxDataBuffer, saverMessageTxByteCount, CDC0_INTFNUM, 1))
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

//
//Based on TI CDC example...
//Released_Version_4_00_00
