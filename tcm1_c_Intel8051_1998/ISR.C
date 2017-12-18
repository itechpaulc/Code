



/*
 *
 *
 *    $Header:   L:/RR/PROJ829/tcm2/source/ISR.C_v   1.3   05 Aug 1998 09:08:40   PaulLC  $
 *    $Log:   L:/RR/PROJ829/tcm2/source/ISR.C_v  $
 * 
 *    Rev 1.3   05 Aug 1998 09:08:40   PaulLC
 * Camera Frame White Streak fixed in this version. A new message was added to allow the OP to query the 
 * TCM system parameters. The Camera/Measure capture fast, now controls the Camera/Measure Process Flags.
 * 
 *    Rev 1.2   21 Jul 1998 16:42:24   PaulLC
 * Optimized floating point calculations to be done in the OP. The Synchronize message was
 * revised to accomodate the new OP Data. Divide by 64K were optimized for speed. Redesigned
 * Measure Free run and Synchronize Message task to be encoder-based processes.
 * 
 *    Rev 1.1   07 Jul 1998 13:39:44   PaulLC
 * Fixed ResetAtEncoderIndex feature. Added a new message to allow the Encoder
 * Directio bit to be querried. Added new defines for this message. Made an explicit 
 * call to Reset Measure FIFO when initialized on TCM reset.
 * 
 *    Rev 1.0   11 Jun 1998 13:11:48   Paul L C
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




//////////////////////////////////////////////////
//
// Private Data
//
//////////////////////////////////////////////////

EncoderMark     instantaneousCurrentEncoderMark;
EncoderMark     currentEncoderMark;

WORD    prevInputCaptureTimeTag;
WORD    currInputCaptureTimeTag;

BYTE    receiveCheckSum;

WORD    current_PCA_time;

WORD    instantaneousEncoderPeriod;

DWORD   instantaneousEncoderImpressionCount;


//////////////////////////////////////////////////
//
// Local Functions
// Forward declaration
//
//////////////////////////////////////////////////

void    IncrementCurrentEncoderMark(void);
void    DecrementCurrentEncoderMark(void);

#define     IncrementInstantaneousCurrentEncoderIndex()     \
                (++instantaneousCurrentEncoderMark.encoderIndex)

#define     DecrementInstantaneousCurrentEncoderIndex()     \
                (--instantaneousCurrentEncoderMark.encoderIndex)

#define     SetInstantaneousCurrentEncoderIndex(idx)        \
                (instantaneousCurrentEncoderMark.encoderIndex = idx)

#define     SetInstantaneousCurrentEncoderRemainder(rem)    \
                (instantaneousCurrentEncoderMark.encoderRemainder = rem)


#define     IncrementInstantaneousEncoderImpressionCount()  \
                (++instantaneousEncoderImpressionCount)


void    DoCameraPreFlashEvent(void);
void    DoCameraFlashEvent(void);



//////////////////////////////////////////////////
//
// PC Communication, interface
//
//////////////////////////////////////////////////

void        StartReplyTransmission(void)
{
    BYTE    xdata *ptrTxBuffer = GetTcmTxBuffer();

    BYTE    len = (*ptrTxBuffer);   // First Byte

    BYTE    l;

    BYTE    checkSum = len;

        --len;                          // 1 Less for the
                                        // checksum data slot

        // Calculate to Generate Checksum

        for(l=1; l < len; l++)
        {
            ++ptrTxBuffer;
            checkSum += (*ptrTxBuffer);
        }

        WriteToPcInterruptByte(checkSum);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    StartAckTransmission(BYTE cmdId)
{
    BYTE    xdata * ptrReplyBuffer = GetTcmTxBuffer();

        *(ptrReplyBuffer)   = 4;        // Length
        *(ptrReplyBuffer+1) = cmdId;    // Cmd
        *(ptrReplyBuffer+2) = ACK;      // Ack

        StartReplyTransmission();
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    StartNakTransmission(BYTE errorRegister)
{
    BYTE    xdata * ptrReplyBuffer = GetTcmTxBuffer();

        *(ptrReplyBuffer)   = 4;                // Length
        *(ptrReplyBuffer+1) = NAK;              // Nak
        *(ptrReplyBuffer+2) = errorRegister;    // Data1

        StartReplyTransmission();
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
//       03     ext 0    0  PC MESSAGE AVAIL
//       0B     tmr 0    1  MEASUREMENT FREE RUN TIMER
//       13     ext 1    2  ----
//       1B     tmr 1    3  ----
//       23     serial   4  ----
//       2B     tmr 2    5  ----
//       33     pca      6  TIME BASED EVENTS
//


//////////////////////////////////////////////////
//
// PC Communications
//
// TCM RECEIVE INTERRUPT
//
// PC Message Data Available
//
// Using register bank 3
//
//////////////////////////////////////////////////

External0_Handler(void) interrupt 0 using 3
{
    // Clear Interrupt Flag by reading the checksum byte
    // This will reset the mailbox

    receiveCheckSum = ReadFromPcInterruptByte();

        // Tell Foreground Task to execute

        SET_PC_MESSAGE_RECEIVED_FLAG();
}


//////////////////////////////////////////////////
//
// UNUSED
//
//////////////////////////////////////////////////

External1_Handler(void) interrupt 2 using 0 { }


//////////////////////////////////////////////////
//
// Timer 0 Overflows,
//
// Measurement Free Run Timer Elapsed
//
// Using register bank 0
//
//////////////////////////////////////////////////

Timer0_Handler(void) interrupt 1 using 0
{
    // Interrupt Flag Automatically cleared
    // Tell Foreground Task to execute

    SET_FREE_RUN_TIMER_ELAPSED_FLAG();
}


//////////////////////////////////////////////////
//
// UNUSED
//
//////////////////////////////////////////////////

Timer1_Handler(void) interrupt 3 using 0 { }


//////////////////////////////////////////////////
//
// UNUSED
//
//////////////////////////////////////////////////

SerialPort_Handler(void) interrupt 4 using 0 { }


//////////////////////////////////////////////////
//
// UNUSED
//
//////////////////////////////////////////////////

Timer2_Handler(void) interrupt 5 using 0 { }



//////////////////////////////////////////////////
//
//	PCA handler
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
//  Using register bank 1
//
//////////////////////////////////////////////////

PCA_Handler(void) interrupt 6 using 1
{
    if(CCF0 == SET)
    {
        CCF0 = CLEAR;
        DoMeasurementFlashEvent();
        SET_MEASUREMENT_FLASH_EVENT();  // Tell Foreground Task to execute
    }

    if(CCF1 == SET)
    {
        CCF1 = CLEAR;
        if(cameraState == WAITING_FOR_CAMERA_FLASH)
            DoCameraFlashEvent();
        else if(cameraState == WAITING_FOR_LAST_CAMERA_FREE_RUN)
            DoCameraPreFlashEvent();

        SET_CAMERA_FLASH_EVENT();       // Tell Foreground Task to execute
    }

    if(CCF2 == SET)
    {
        CCF2 = CLEAR;
        DoEncoderPulseEvent();
        SET_ENCODER_PULSE_EVENT();      // Tell Foreground Task to execute
    }

    if(CCF3 == SET)
    {
        CCF3 = CLEAR;
        DoEndOfGrabFrameEvent();
        SET_END_OF_GRABFRAME_EVENT();   // Tell Foreground Task to execute
    }

    if(CCF4 == SET)
    {
        CCF4 = CLEAR;
        DoFlashAdjustEvent();
        SET_FLASH_ADJUST_EVENT();       // Tell Foreground Task to execute
    }

    if(CF == SET)
    {
        CF = CLEAR;
        SET_OVER_FLOW_EVENT();          // Tell Foreground Task to execute
    }
}



//////////////////////////////////////////////////
//
// MEASUREMENT FLASH / Process
// (and what happens right after it)
// Output Compare Trigger
//
//////////////////////////////////////////////////

void    DoMeasurementFlashEvent(void)
{
    DISABLE_OUTPUT_COMPARE_0();

    DELAY_2_USEC();

    RESET_MEASUREMENT_FLASH_TRIGGER();

    // Measure Read

    PULSE_MEAS_READ_TRIGGER();
}


//////////////////////////////////////////////////
//
// CAMERA FLASH / Process
// (and what happens right after it)
// Output Compare Trigger
//
//////////////////////////////////////////////////

#define     TIMER_READ_THRESHOLD                16
#define     TIMER_WRAP_AROUND_READ_THRESHOLD    (0xFFFF - TIMER_READ_THRESHOLD)
#define     MAX_TIMER_READ_RETRY                2

void    DoCameraPreFlashEvent(void)
{
        SET_FRAME_START();

        DISABLE_OUTPUT_COMPARE_1();

    current_PCA_time = (CCAP1H << 8) | CCAP1L;
}

void    DoCameraFlashEvent(void)
{
    BYTE    readRetry = 0;
    WORD    timerRead1, timerRead2;

        DISABLE_OUTPUT_COMPARE_1();

        DELAY_2_USEC();
        RESET_CAMERA_FLASH_TRIGGER();

        DELAY_2_USEC();
        CLEAR_FRAME_START();

        DELAY_5_USEC();
        CLEAR_GRAB_FRAME();

    // Remember when this ISR was executed, to
    // determine when GRAB FRAME line should be SET.
    // Test to make sure timer data is valid.

    do
    {
        timerRead1 = GET_CPU_CURRENT_TIMER_TIME();
        timerRead2 = GET_CPU_CURRENT_TIMER_TIME();

        if(((timerRead2-timerRead1) < TIMER_READ_THRESHOLD) ||
           ((timerRead1-timerRead2) > TIMER_WRAP_AROUND_READ_THRESHOLD))
        {
            break;
        }

        ++readRetry;

    }while(readRetry <= MAX_TIMER_READ_RETRY);

    // Use either timer values as the latched timer data

    current_PCA_time = timerRead1;
}



//////////////////////////////////////////////////
//
//	Encoder Pulse Handling
//
// Input Capture Trigger
//
//////////////////////////////////////////////////

void    DoEncoderPulseEvent(void)
{
    signed long tempInputCaptureTimeTag;

        currInputCaptureTimeTag = (CCAP2H << 8) | CCAP2L;

        tempInputCaptureTimeTag =
            (long)currInputCaptureTimeTag - (long)prevInputCaptureTimeTag;

			if( tempInputCaptureTimeTag < 0 )
				tempInputCaptureTimeTag += 0x10000;

			instantaneousEncoderPeriod = (WORD)tempInputCaptureTimeTag;

            prevInputCaptureTimeTag = currInputCaptureTimeTag;


        // Check for encoder Index Increment/Decrement

        if(IsNormalQuadratureEncoderDirection())
        {
            if(IS_ENCODER_RUNNING_FORWARD())
            {
                IncrementCurrentEncoderMark();

                SET_CHECK_FOR_PENDING_FLAG();
            }
            else
            {
                DecrementCurrentEncoderMark();
            }
        }
        else
        {
            // REVERSED QUADRATURE DIRECTION

            if(IS_ENCODER_RUNNING_REVERSED())
            {
                IncrementCurrentEncoderMark();

                SET_CHECK_FOR_PENDING_FLAG();
            }
            else
            {
                DecrementCurrentEncoderMark();
            }
        }

        // Check for encoder reset command. Check if the reset
        // should be done at this "encoder index point"

        if(IS_RESET_ENCODER_AT_INDEX_PENDING())
            if(GetEncoderResetIndexPoint() == GetCurrentEncoderIndex())
            {
                SetInstantaneousCurrentEncoderIndex(0);
                SetInstantaneousCurrentEncoderRemainder(0);

                CLEAR_RESET_ENCODER_AT_INDEX_FLAG();
            }
}




//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    IncrementCurrentEncoderMark(void)
{
    signed long     remainder;

        IncrementInstantaneousCurrentEncoderIndex();

        // Check if it is time to reset the encoder index

        if(NoRemainderProcessing())
        {
            if(GetInstantaneousCurrentEncoderIndex() == impressionLength.encoderIndex)
            {
                SetInstantaneousCurrentEncoderIndex(0);

                IncrementInstantaneousEncoderImpressionCount();
            }
            else		// check Impression pulse if this is a start of
            if( IsUseImpressionPulseFlagSet() &&
                ((IS_IMPRESSION_PULSE_SET() && IsHighImpressionPulseLevelSet()) ||
                 (IS_IMPRESSION_PULSE_CLEAR() && !IsHighImpressionPulseLevelSet())) )
            {
                SetInstantaneousCurrentEncoderIndex(0);

                IncrementInstantaneousEncoderImpressionCount();
            }
        }
        else
        {
            // Perform calculations to take into consideration
            // the Remainder value of each impression

            if(GetInstantaneousCurrentEncoderIndex() > GetImpressionLengthIndex())
            {
                remainder = GetInstantaneousCurrentEncoderRemainder() +
                            (MAX_ENCODER_REMAINDER_VALUE - GetImpressionLengthRemainder());

                // Overflow check (WORD)

                if(remainder > MAX_ENCODER_REMAINDER_VALUE)
                    SetInstantaneousCurrentEncoderIndex(1);
                else
                    SetInstantaneousCurrentEncoderIndex(0);

                SetInstantaneousCurrentEncoderRemainder(remainder);

                IncrementInstantaneousEncoderImpressionCount();
            }
        }
}


//////////////////////////////////////////////////
//
// The Encoders are temporarily going in
// the reverse direction. Just keep track of the
// indexes.
//
// Don't Care about EncoderImpressionCount here.
//
//////////////////////////////////////////////////

void    DecrementCurrentEncoderMark(void)
{
    signed long   remainder;

	// Under flow check, if last index was 0

        if(GetInstantaneousCurrentEncoderIndex() == 0)
        {
            if(NoRemainderProcessing())
            {
                SetInstantaneousCurrentEncoderIndex(GetImpressionLengthIndex() - 1);
            }
            else
            {
                remainder = GetInstantaneousCurrentEncoderRemainder() +
                    (MAX_ENCODER_REMAINDER_VALUE - GetImpressionLengthRemainder());

                // Remainder Underflow check

                if(remainder > MAX_ENCODER_REMAINDER_VALUE)
                   SetInstantaneousCurrentEncoderIndex(GetImpressionLengthIndex() - 1);
                else
                   SetInstantaneousCurrentEncoderIndex(GetImpressionLengthIndex());

                SetInstantaneousCurrentEncoderRemainder(remainder);
            }
        }
        else
        {
            DecrementInstantaneousCurrentEncoderIndex();
        }
}


//////////////////////////////////////////////////
//
//
//	This is the end of the camera cycle. (to turn off
//	the frame grab data from the camera)
//
// Output Compare Trigger
//
//////////////////////////////////////////////////

void    DoEndOfGrabFrameEvent(void)
{
    DISABLE_OUTPUT_COMPARE_3();

    SET_GRAB_FRAME();			// end of camera data transfer

    CLEAR_FRAME_START();		// (just in case)
}


//////////////////////////////////////////////////
//
//	This timed trigger is to get the measure flash
//	adjusted as late as possible so that we have
//	a better chance of receiving the adjustment
//	before the flash occurs.
//
// Output Compare Trigger - not used at this time
//
//////////////////////////////////////////////////

void    DoFlashAdjustEvent(void)
{


}



