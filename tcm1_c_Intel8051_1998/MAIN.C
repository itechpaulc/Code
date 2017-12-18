



/*
 *
 *
 *    $Header:   L:/RR/PROJ829/tcm2/source/MAIN.C_v   1.2   21 Jul 1998 16:42:28   PaulLC  $
 *    $Log:   L:/RR/PROJ829/tcm2/source/MAIN.C_v  $
 * 
 *    Rev 1.2   21 Jul 1998 16:42:28   PaulLC
 * Optimized floating point calculations to be done in the OP. The Synchronize message was
 * revised to accomodate the new OP Data. Divide by 64K were optimized for speed. Redesigned
 * Measure Free run and Synchronize Message task to be encoder-based processes.
 * 
 *    Rev 1.1   07 Jul 1998 13:39:50   PaulLC
 * Fixed ResetAtEncoderIndex feature. Added a new message to allow the Encoder
 * Directio bit to be querried. Added new defines for this message. Made an explicit 
 * call to Reset Measure FIFO when initialized on TCM reset.
 * 
 *    Rev 1.0   11 Jun 1998 13:11:56   Paul L C
 *  
 *
 *
 *      Author : Paul Calinawan, Mark Colvin
 *
 *      April 1998
 *
 *
 *          Graphics Microsystems Inc
 *          Color Measurement Group
 *          10375 Brockwood Road
 *          Dallas, TX 75238
 *
 *          (214) 340-4584
 *
 *
 *      Road Runner PC/AT Timing and Control Module
 *      -------------------------------------------
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



#include "tcm2.h"


WORD    mfrLatchedEncoderIndex,
        mfrLatchedEncoderTimeTag;

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void main(void)
{
    InitPorts();

    InitPCAModules();

    InitTimers();

    InitSerial();

    InitPcComm();

    InitSystemData();

    InitInterrupts();


    //////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////

    while(TRUE)
    {
        if(PCA_EVENT_ACTIVATED())
        {
            if(IS_MEASUREMENT_FLASH_EVENT_SET())
                DoMeasurementFlashTask();

            if(IS_CAMERA_FLASH_EVENT_SET())
                DoCameraFlashTask();

            if(IS_ENCODER_PULSE_EVENT_SET())
                DoEncoderPulseTask();

            if(IS_END_OF_GRABFRAME_EVENT_SET())
                DoEndOfGrabFrameTask();

            if(IS_FLASH_ADJUST_EVENT_SET())
                DoFlashAdjustTask();

            if(IS_OVER_FLOW_EVENT_SET())
                DoSystemCheckTask();
        }

        if(PC_MESSAGE_RECEIVED())
        {
            if(IsPcMessageOk())
                HandlePcMessageReceived();
            else
                HandlePcMessageError();
        }

        if(FREE_RUN_TIMER_ELAPSED())
        {
            HandleFreeRunTimerElapsed();
        }
    }
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    InitPorts(void)
{
    // Port 0 - Dual Port Address Lines
    // Port 2 - Dual Port Address Lines

    // Port 1

    // Port 3

    // Other Initialization


    RESET_CAMERA_FLASH_TRIGGER();
    RESET_MEASUREMENT_FLASH_TRIGGER();
    CLEAR_FRAME_START();
    SET_GRAB_FRAME();

    RELEASE_PROBE_HEAD_RESET();

    RESET_MEASURE_FIFO();
}


//////////////////////////////////////////////////
//
//  Compare 0   - toggles the Measurement Flash
//  Compare 1   - toggles the Camera Flash
//
//  Capture 2   - processes Encoder_Pulse
//                  (falling edge)
//
//  Compare 3   - Timer used for Grab Frame Off
//                  (End of Frame Grabbing)
//
//  Compare 4   - Timer used for cam/meas flash
//                  adjustment calculations
//
//////////////////////////////////////////////////

void InitPCAModules(void)
{
    // Enable the PCA Timer Overflow interrupt

    CMOD    = 0x01;

    // Module 0, Compare 0 - Start with No Operation

    CCAPM0 = 0x00;

    // Module 1, Compare 1 - Start with No Operation

    CCAPM1 = 0x00;

    // Module 2, Capture on Falling edge

    CCAPM2 = 0x10;

    // Module 3, Compare 3 - Start with No Operation

    CCAPM3 = 0x00;

    // Module 4, Compare 4 - Start with No Operation

    CCAPM4 = 0x00;


    CCON    = 0x40;     // Run the PCA Timers
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    InitTimers(void)
{
    // Timer 0 and 1 Settings

    TCON = 0x00;
    TMOD = 0x01; // Timer 0 Mode 16 bit counter


    // Timer 2 and 3 Settings - Both are unused

    T2CON = 0x00;
    T2MOD = 0x00;

    TL0 = LOBYTE(0);
    TH0 = HIBYTE(0);

    RESTART_MEASUREMENT_FREE_RUN_TIMER();
}



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    InitSerial(void)
{
    SCON = 0x00;

}


//////////////////////////////////////////////////
//
// Interrupt Priority Settings:
//
//      PCA                         - Highest
//      PC COMM (Ext Interrupts)    - Medium
//      TIMER 0                     - Low
//
//      others                      - Lowest
//
//      Others Disabled:
//          SERIAL
//          TIMER1
//          TIMER2
//
//////////////////////////////////////////////////

void    InitInterrupts(void)
{
    // Set Interrupt Priority

    IPH0 = 0x45;
    IPL0 = 0x42;

    // Enable PCA, EXT INTR 0,1 and TIMER0
    // Disable Other Interrupts

    // Set Register IE0

        EA  = SET;
        EC  = SET;
        ET0 = SET;
        EX0 = SET;

    DISABLE_OUTPUT_COMPARE_0();
    DISABLE_OUTPUT_COMPARE_1();

    ENABLE_ENCODER_CAPTURE();
}





