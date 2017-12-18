



/*
 *
 *
 *    $Header:   L:/RR/PROJ829/tcm2/source/TCM2.H_v   1.7   05 Aug 1998 09:09:30   PaulLC  $
 *    $Log:   L:/RR/PROJ829/tcm2/source/TCM2.H_v  $
 * 
 *    Rev 1.7   05 Aug 1998 09:09:30   PaulLC
 * Camera Frame White Streak fixed in this version. A new message was added to allow the OP to query the 
 * TCM system parameters. The Camera/Measure capture fast, now controls the Camera/Measure Process Flags.
 * 
 *    Rev 1.6   31 Jul 1998 09:12:36   PaulLC
 * Fixed issue where the measurement flash could be delayed by as much as 65msec. 
 * The Measurement Flash setup index distance is now a parameter downloaded by the OP. 
 * 
 *    Rev 1.5   24 Jul 1998 11:19:30   PaulLC
 * Reset Measure FIFO was moved from the "Measure Flash Setup" section over to the 
 * "Synchronize Command" section. A Mirror flag was added for the Measurement Process
 * to allow the OP to check its completion. Relative Encoder Index is now updated on each
 * Encoder Pulse Task. The Last Free Run pulse is now triggered for the Measure Capture 
 * Fast Process.
 * 
 *    Rev 1.4   21 Jul 1998 16:42:48   PaulLC
 * Optimized floating point calculations to be done in the OP. The Synchronize message was
 * revised to accomodate the new OP Data. Divide by 64K were optimized for speed. Redesigned
 * Measure Free run and Synchronize Message task to be encoder-based processes.
 * 
 *    Rev 1.3   07 Jul 1998 13:40:36   PaulLC
 * Fixed ResetAtEncoderIndex feature. Added a new message to allow the Encoder
 * Directio bit to be querried. Added new defines for this message. Made an explicit 
 * call to Reset Measure FIFO when initialized on TCM reset.
 * 
 *    Rev 1.2   06 Jul 1998 17:43:24   MARKC
 * version 1.0.c
 * chgd NormalizeEncoderIndex
 * (to fix camera position bug at encoder index of zero)
 * 
 *    Rev 1.1   17 Jun 1998 13:59:18   PaulLC
 * Fixed Reverse Encoder test problem by taking out debug sections.  Fixed Set Impression command
 * to update both background and foreground variables. Made TCM 2 version numbering to be 5 digits.
 * 
 *    Rev 1.0   11 Jun 1998 13:12:24   Paul L C
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



#ifndef __TCM2_H__
#define __TCM_H__


#include <reg251s.h>

#include <intrins.h>


//
// Source Code Markers
//

// OPTIMIZE ?
// COMMENT ?
// REVIEW ?



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

#define     MAJOR_VERSION           0x01
#define     MINOR_VERSION           0x00
#define     INTERMEDIATE_VERSION    0x0c  // 01.00L



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

typedef     unsigned long int       DWORD;

typedef     unsigned short int      WORD;

typedef     unsigned char           BYTE;

#define     LOBYTE(w)	    ((BYTE)(w))
#define     HIBYTE(w)       ((BYTE)(w >> 8))

#define     TRUE        1
#define     FALSE       0

typedef     BYTE        BOOL;

#define     SET         TRUE
#define     CLEAR       FALSE

typedef     union tagDWORD_UNION
{
    DWORD   tempDWORD;
    WORD    tempWORD;

}DWORD_UNION;


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

#define     MEASUREMENT_PREPROCESS_SETUP_INDEX_DISTANCE     1

#define     CAMERA_PREPROCESS_SETUP_INDEX_DISTANCE          2


//////////////////////////////////////////////////
//
// Function Macros
//
// For Interrupt Flags
//
//////////////////////////////////////////////////

extern  BYTE    bdata   PCA_EVENT_FLAGS;

extern  bit     MEASUREMENT_FLASH_EVENT,
                CAMERA_FLASH_EVENT,
                ENCODER_PULSE_EVENT,
                END_OF_GRABFRAME_EVENT,
                FLASH_ADJUST_EVENT,
                OVER_FLOW_EVENT;

#define     PCA_EVENT_ACTIVATED()               (PCA_EVENT_FLAGS)

#define     IS_MEASUREMENT_FLASH_EVENT_SET()    (MEASUREMENT_FLASH_EVENT==SET)
#define     IS_CAMERA_FLASH_EVENT_SET()         (CAMERA_FLASH_EVENT==SET)
#define     IS_ENCODER_PULSE_EVENT_SET()        (ENCODER_PULSE_EVENT==SET)
#define     IS_END_OF_GRABFRAME_EVENT_SET()     (END_OF_GRABFRAME_EVENT==SET)
#define     IS_FLASH_ADJUST_EVENT_SET()         (FLASH_ADJUST_EVENT==SET)
#define     IS_OVER_FLOW_EVENT_SET()            (OVER_FLOW_EVENT==SET)


#define     SET_MEASUREMENT_FLASH_EVENT()       (MEASUREMENT_FLASH_EVENT=SET)
#define     SET_CAMERA_FLASH_EVENT()            (CAMERA_FLASH_EVENT=SET)
#define     SET_ENCODER_PULSE_EVENT()           (ENCODER_PULSE_EVENT=SET)
#define     SET_END_OF_GRABFRAME_EVENT()        (END_OF_GRABFRAME_EVENT=SET)
#define     SET_FLASH_ADJUST_EVENT()            (FLASH_ADJUST_EVENT=SET)
#define     SET_OVER_FLOW_EVENT()               (OVER_FLOW_EVENT=SET)


#define     CLEAR_MEASUREMENT_FLASH_EVENT()     (MEASUREMENT_FLASH_EVENT=CLEAR)
#define     CLEAR_CAMERA_FLASH_EVENT()          (CAMERA_FLASH_EVENT=CLEAR)
#define     CLEAR_ENCODER_PULSE_EVENT()         (ENCODER_PULSE_EVENT=CLEAR)
#define     CLEAR_END_OF_GRABFRAME_EVENT()      (END_OF_GRABFRAME_EVENT=CLEAR)
#define     CLEAR_FLASH_ADJUST_EVENT()          (FLASH_ADJUST_EVENT=CLEAR)
#define     CLEAR_OVER_FLOW_EVENT()             (OVER_FLOW_EVENT=CLEAR)


extern      BOOL    PC_MESSAGE_RECEIVED_FLAG;

#define     PC_MESSAGE_RECEIVED()               (PC_MESSAGE_RECEIVED_FLAG==SET)
#define     SET_PC_MESSAGE_RECEIVED_FLAG()      (PC_MESSAGE_RECEIVED_FLAG=SET)
#define     CLEAR_PC_MESSAGE_RECEIVED_FLAG()    (PC_MESSAGE_RECEIVED_FLAG=CLEAR)


extern      BOOL    FREE_RUN_TIMER_ELAPSED_FLAG;

#define     FREE_RUN_TIMER_ELAPSED()            (FREE_RUN_TIMER_ELAPSED_FLAG==SET)
#define     SET_FREE_RUN_TIMER_ELAPSED_FLAG()   (FREE_RUN_TIMER_ELAPSED_FLAG=SET)
#define     CLEAR_FREE_RUN_TIMER_ELAPSED_FLAG() (FREE_RUN_TIMER_ELAPSED_FLAG=CLEAR)



//////////////////////////////////////////////////
//
// Port Definitions
//
//////////////////////////////////////////////////

//
//  BIT LABELS
//


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  ----PORT 0----
//
//  USED AS ADDRESS LINES to control the
//  Dual Port Ram Communication Channel
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////



//////////////////////////////////////////////////
//
//  ----PORT 1----
//
//////////////////////////////////////////////////

//
//GRAB_FRAME              BIT     P1.0    ;output to framgrbr,valid frame       (O)
//IMPRESSION_PULSE        BIT     P1.1    ;from press impression signal         (I)
//MEAS_WITH_FLASH_PORT    BIT     P1.2    ;output to meas sys, 1 = no flash     (O)
//MEAS_FLASH_TRIGGER      BIT     P1.3    ;counter compare for measur lamp      (O)
//CAMERA_FLASH_TRIGGER    BIT     P1.4    ;counter compare for camera lamp      (O)
//ENCODER_PULSE           BIT     P1.5    ;(input) from encoder                 (I)
//PROBE_HEAD_RESET        BIT     P1.6    ;PH Reset Line Control                (O)
//MEAS_READ_TRIGGER       BIT     P1.7    ;tell PC to get meas data             (O)
//

sbit        GRAB_FRAME                      = P1 ^ 0;
sbit        IMPRESSION_PULSE                = P1 ^ 1;
sbit        MEAS_WITH_FLASH_PORT            = P1 ^ 2;
sbit        MEAS_FLASH_TRIGGER              = P1 ^ 3;
sbit        CAMERA_FLASH_TRIGGER            = P1 ^ 4;

sbit        PROBE_HEAD_RESET_LINE           = P1 ^ 6;
sbit        MEAS_READ_TRIGGER               = P1 ^ 7;

#define     RESET_PROBE_HEAD()                          \
                HOLD_PROBE_HEAD_RESET();                \
                DELAY_5_USEC();                         \
                RELEASE_PROBE_HEAD_RESET();             \
                DELAY_5_USEC()


//////////////////////////////////////////////////
//
// PORT 1 Control\Query Line Macros
//
//////////////////////////////////////////////////

#define     SET_GRAB_FRAME()                        (GRAB_FRAME = SET)
#define     CLEAR_GRAB_FRAME()                      (GRAB_FRAME = CLEAR)

#define     IS_IMPRESSION_PULSE_SET()               (IMPRESSION_PULSE == SET)
#define     IS_IMPRESSION_PULSE_CLEAR()             (IMPRESSION_PULSE == CLEAR)

#define     SET_MEAS_WITH_FLASH_PORT()              (MEAS_WITH_FLASH_PORT = SET)
#define     CLEAR_MEAS_WITH_FLASH_PORT()            (MEAS_WITH_FLASH_PORT = CLEAR)

#define     SET_MEASUREMENT_FLASH_TRIGGER()         (MEAS_FLASH_TRIGGER = SET)
#define     RESET_MEASUREMENT_FLASH_TRIGGER()       (MEAS_FLASH_TRIGGER = CLEAR)

#define     RESET_CAMERA_FLASH_TRIGGER()            (CAMERA_FLASH_TRIGGER = CLEAR)

#define     HOLD_PROBE_HEAD_RESET()                 (PROBE_HEAD_RESET_LINE = SET)
#define     RELEASE_PROBE_HEAD_RESET()              (PROBE_HEAD_RESET_LINE = CLEAR)

#define     SET_MEAS_READ_TRIGGER()                 (MEAS_READ_TRIGGER = SET)
#define     CLEAR_MEAS_READ_TRIGGER()               (MEAS_READ_TRIGGER = CLEAR)


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

#define     MEASURE_WITH_FLASH()        (CLEAR_MEAS_WITH_FLASH_PORT())

#define     MEASURE_WITHOUT_FLASH()     (SET_MEAS_WITH_FLASH_PORT())

#define     TRIGGER_MEASUREMENT_FLASH()                 \
                DELAY_5_USEC();                         \
                SET_MEASUREMENT_FLASH_TRIGGER();        \
                DELAY_5_USEC();                         \
                RESET_MEASUREMENT_FLASH_TRIGGER()


#define     PULSE_MEAS_READ_TRIGGER()                   \
                DELAY_5_USEC();                         \
                SET_MEAS_READ_TRIGGER();                \
                DELAY_5_USEC();                         \
                CLEAR_MEAS_READ_TRIGGER()



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  ----PORT 2----
//
//  USED AS ADDRESS LINES to control the
//  Dual Port Ram Communication Channel
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////





//////////////////////////////////////////////////
//
//  ----PORT 3----
//
//////////////////////////////////////////////////

//
//ENCODER_DIRECTION         BIT     P3.0    ;PH Reset Line Control          (O)
//FRAME_START               BIT     P3.1    ;camera's exsync signal         (O)
//  MAILBOXfromPC           BIT     P3.2    ;PC has command in DPSram buffer(I)
//  unused                  BIT     P3.3    ;
//MEASURE_FIFO_RESET_LINE   BIT     P3.4    ;measure FIFO reset             (O)
//  unused                  BIT     P3.5    ;
//;write external           BIT     P3.6    ;used for external data access
//;read external            BIT     P3.7    ;
//

sbit        ENCODER_DIRECTION           = P3 ^ 0;
sbit        FRAME_START                 = P3 ^ 1;

sbit        MEASURE_FIFO_RESET_LINE     = P3 ^ 4;


#define     IS_ENCODER_RUNNING_FORWARD()            (ENCODER_DIRECTION == SET)

#define     IS_ENCODER_RUNNING_REVERSED()           (ENCODER_DIRECTION == CLEAR)

#define     SET_FRAME_START()                       (FRAME_START = SET)
#define     CLEAR_FRAME_START()                     (FRAME_START = CLEAR)

#define     HOLD_MEASURE_FIFO_RESET()               (MEASURE_FIFO_RESET_LINE = SET)
#define     RELEASE_MEASURE_FIFO_RESET()            (MEASURE_FIFO_RESET_LINE = CLEAR)

#define     RESET_MEASURE_FIFO()                        \
                HOLD_MEASURE_FIFO_RESET();              \
                DELAY_5_USEC();                         \
                RELEASE_MEASURE_FIFO_RESET();            \
                DELAY_5_USEC()


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

// Measurement - Flash

#define     ENABLE_OUTPUT_COMPARE_0()       (CCAPM0 = 0x4D)
#define     DISABLE_OUTPUT_COMPARE_0()      (CCAPM0 = 0x00)

// Camera - Flash

#define     ENABLE_OUTPUT_COMPARE_1()       (CCAPM1 = 0x49)
#define     ENABLE_FLASH_TOGGLE_1()         (CCAPM1 = 0x4D)
#define     DISABLE_OUTPUT_COMPARE_1()      (CCAPM1 = 0x00)

// Camera - Grab Frame

#define     ENABLE_OUTPUT_COMPARE_3()       (CCAPM3 = 0x49)
#define     DISABLE_OUTPUT_COMPARE_3()      (CCAPM3 = 0x00)

// Measurement Adjustment 

#define     ENABLE_OUTPUT_COMPARE_4()       (CCAPM4 = 0x49)
#define     DISABLE_OUTPUT_COMPARE_4()      (CCAPM4 = 0x00)

// Encoder

#define     ENABLE_ENCODER_CAPTURE()        (CCAPM2 = 0x11)




//////////////////////////////////////////////////
//
// Note: encoderIndex has a maximum of 0xfffe
//          0xffff is reserved and used when
//          detecting encoder move reversals
//
//////////////////////////////////////////////////

typedef     WORD   EncoderTimeTag;

struct EncoderMark {

    WORD    encoderIndex;
    WORD    encoderRemainder;
};

typedef struct EncoderMark     EncoderMark;

struct AdjustMark {

    signed int  encoderIndex;
    WORD    	encoderRemainder;
};

typedef struct AdjustMark     AdjustMark;


#define     MAX_ENCODER_INDEX_VALUE         0xffff

#define     MAX_ENCODER_REMAINDER_VALUE     0xffff

#define     TIMER_0_OVERFLOW_VALUE          0xffff



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    InitPorts(void);
void    InitPCAModules(void);
void    InitTimers(void);
void    InitSerial(void);
void    InitInterrupts(void);


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// ISR Tasks Forward Declaration
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

void    DoMeasurementFlashEvent(void);

void    DoCameraFlashEvent(void);

void    DoEncoderPulseEvent(void);

void    DoEndOfGrabFrameEvent(void);

void    DoFlashAdjustEvent(void);

void    DoSystemCheckTask(void);


#define     GET_ADJUSTMENT_OK_FLAG()        (adjustmentOkFlag)
#define     SET_ADJUSTMENT_OK_FLAG()        (adjustmentOkFlag = SET)
#define     CLEAR_ADJUSTMENT_OK_FLAG()      (adjustmentOkFlag = CLEAR)

#define     IS_ADJUSTMENT_NEEDED()          (adjustmentNeededFlag)
#define     SET_ADJUSTMENT_NEEDED_FLAG()    (adjustmentNeededFlag = SET)
#define     CLEAR_ADJUSTMENT_NEEDED_FLAG()  (adjustmentNeededFlag = CLEAR)


#define     IS_ADJUSTMENT_LATE()            (adjustmentLateFlag)
#define     SET_ADJUSTMENT_LATE_FLAG()      (adjustmentLateFlag = SET)
#define     CLEAR_ADJUSTMENT_LATE_FLAG()    (adjustmentLateFlag = CLEAR)


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// PC Communication, interface
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

#define     GetTcmRxBuffer()    (tcmRxBuffer)
#define     GetTcmTxBuffer()    (tcmTxBuffer)

#define     GetTcmRxCheckSum()  (receiveCheckSum)

#define     WriteToPcInterruptByte(d)   (toPcInterruptByte = d)
#define     ReadFromPcInterruptByte()   (fromPcInterruptByte)


#define     ENABLE_PC_TO_TCM_RECEPTION()        (EX0 = SET)
#define     DISABLE_PC_TO_TCM_RECEPTION()       (EX0 = CLEAR)

#define     ACK                         0x0F
#define     NAK                         0xF0

#define     LENGTH_SLOT                 0x00
#define     COMMAND_SLOT                0x01
#define     DATA_START_SLOT             0x02

#define     MAX_DATA_LENGTH             61

//
// 1 Length Byte + 1 Command Byte. The checksum byte is written
// on its own specific location
//

#define     HEADER_TRAILER_LENGTH       2
#define     MAX_PACKET_LENGTH           (MAX_DATA_LENGTH + HEADER_TRAILER_LENGTH)


#define     GetPcMessage()      (pcMessageBuffer)

#define     GetPcReplyBuffer()  (pcMessageBuffer)


void        InitPcComm(void);


void        StartReplyTransmission(void);
void        StartAckTransmission(BYTE commandId);
void        StartNakTransmission(BYTE commandId);


void        HandlePcMessageReceived(void);

BOOL        IsPcMessageOk(void);

void        HandlePcMessageError(void);



//////////////////////////////////////////////////
//
//  COMMAND ID Definitions
//
//////////////////////////////////////////////////

#define     GET_VERSION                             0x10

#define     SET_ADJUSTMENT_PARAMETERS               'A'

#define     SET_CAMERA_PARAMETERS                   'C'

#define     SET_SYSTEM_PARAMETERS                   'D'

#define     MEASUREMENT_FIFO_RESET                  'G'
#define     PROBE_HEAD_RESET                        'H'

#define     SET_IMPRESSION_COUNT                    'I'

#define     SET_MEASUREMENT_PARAMETERS              'M'

#define     QUERY_ENCODER_INDEX                     'Q'

#define     RESET_ENCODER_AT_ENCODER_INDEX          'R'

#define     SYNCHRONIZE_CAM_MEAS_PROCESS            'S'


#define     QUERY_ENCODER_PERIOD                    0x60
#define     QUERY_IMPRESSION_COUNT                  0x61
#define     QUERY_ADJUSTMENT_OK_FLAG                0x62
#define     QUERY_PROCESS_STATUS_FLAG               0x63
#define     QUERY_ENCODER_DIRECTION_STATUS_FLAG     0x64
#define     QUERY_SYSTEM_PARAMETERS                 0x65


#define     MEASURE_CAPTURE_FAST                    0x80
#define     CAMERA_CAPTURE_FAST                     0x81

#define     DUAL_PORT_MEMORY_TEST                   0xA0


#define     DEBUG_0_COMMAND                         0x00



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// Tasks
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
// System Data Default Values
//
//////////////////////////////////////////////////

// 6 msec Measurement Frame Time (1 usec ticks)

#define     MEASUREMENT_FRAME_READOUT_TIME          6000

// 350 usec Camera Frame Readout Time (1 usec ticks)

#define     CAMERA_FRAME_READOUT_TIME               350

// Impression Length

#define     IMPRESSION_LENGTH_ENCODER_INDEX         120
#define     IMPRESSION_LENGTH_ENCODER_REMAINDER     0

// 10 msec Measurement Freerun intervals (1 usec ticks)

#define     MEASUREMENT_FREE_RUN_TIME_INTERVAL      10000

#define     GetFreeRunStartPoint(mft)               (0xffff - mft)


//////////////////////////////////////////////////
//
// System Data Access
//
//////////////////////////////////////////////////

#define     SetMeasurementFrameTime(mft)            (measFrameTime = mft)
#define     GetMeasurementFrameTime()               (measFrameTime)

#define     SetCameraFrameReadoutTime(cfrot)        (camFrameReadoutTime = cfrot)
#define     GetCameraFrameReadoutTime()             (camFrameReadoutTime)

#define     SetImpressionLength(eI, eR)                 \
                SetEncoderMark(&impressionLength, eI, eR)

#define     GetImpressionLengthIndex()                  \
                    (impressionLength.encoderIndex)

#define     GetImpressionLengthRemainder()              \
                    (impressionLength.encoderRemainder)

#define     SetMeasurementFreeRunIntervalTime(frInt)    \
                (measurementFreeRunIntervalTime = frInt)

#define     GetMeasurementFreeRunIntervalTime()         \
                (measurementFreeRunIntervalTime)


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

#define     SetEncoderResetIndexPoint(rstIdxPt)       \
                ( resetIndexPoint = rstIdxPt)

#define     GetEncoderResetIndexPoint()             (resetIndexPoint)

#define     IS_RESET_ENCODER_AT_INDEX_PENDING()     (resetEncoderAtIndexFlag == SET)

#define     SET_RESET_ENCODER_AT_INDEX_FLAG()       (resetEncoderAtIndexFlag = SET)
#define     CLEAR_RESET_ENCODER_AT_INDEX_FLAG()     (resetEncoderAtIndexFlag = CLEAR)


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    InitSystemData(void);

void    DELAY_2_USEC(void);

void    DELAY_5_USEC(void);

WORD    GetTimerTicksOnEncoderMarker(EncoderMark * ptrEncMark);

void    NormalizeEncoderIndex(signed int *encoderIdx);


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

#define     SetCameraFlashEncoderMark(eI, eR)       \
                SetEncoderMark(&cameraFlashEncoderMark, eI, eR)

#define     SetMeasurementFlashEncoderMark(eI, eR)  \
                SetEncoderMark(&measurementFlashEncoderMark, eI, eR)

#define     SetMeasurementFlashAdjustMark(eI, eR)  \
                SetAdjustMark(&measurementFlashAdjustMark, eI, eR)

#define     GetCameraFlashEncoderIndex()            \
                (cameraFlashEncoderMark.encoderIndex)

#define     GetCameraFlashEncoderRemainder()             \
                (cameraFlashEncoderMark.encoderRemainder)

#define     GetMeasurementFlashEncoderIndex()       \
                (measurementFlashEncoderMark.encoderIndex)

#define     GetMeasurementFlashEncoderRemainder()        \
                (measurementFlashEncoderMark.encoderRemainder)

#define     GetMeasurementFlashAdjustIndex()       \
                (measurementFlashAdjustMark.encoderIndex)

#define     GetMeasurementAdjustRemainder()        \
                (measurementFlashAdjustMark.encoderRemainder)



#define     SetMeasurementPreProcessSetupIndex(mSIdx)   (measurementPreProcessSetupIndex = mSIdx)
#define     GetMeasurementPreProcessSetupIndex()        (measurementPreProcessSetupIndex)

#define     SetMeasurementFlashSetupDistance(mFSD)      (measurementFlashSetupDistance = mFSD)
#define     GetMeasurementFlashSetupDistance()          (measurementFlashSetupDistance)

#define     SetCameraPreProcessSetupIndex(cSIdx)        (cameraPreProcessSetupIndex = cSIdx)
#define     GetCameraPreProcessSetupIndex()             (cameraPreProcessSetupIndex)


#define     GetLastMeasFreeRunIndex()               \
                (lastMeasFreeRunEncoderMark.encoderIndex)

#define     GetLastMeasFreeRunRemainder()           \
                (lastMeasFreeRunEncoderMark.encoderRemainder)

#define     SetLastMeasFreeRunIndex(mLMFRI)         \
                (lastMeasFreeRunEncoderMark.encoderIndex = mLMFRI)

#define     SetLastMeasFreeRunRemainder(mLMFRR)     \
                (lastMeasFreeRunEncoderMark.encoderRemainder = mLMFRR)

#define     SetLastMeasReadStartFrameEncoderIdx(mRSFEI) (lastMeasReadStartFrameEncoderIdx = mRSFEI)
#define     GetLastMeasReadStartFrameEncoderIdx()       (lastMeasReadStartFrameEncoderIdx)

#define     SetLastMeasReadEndFrameEncoderIdx(mREFEI)   (lastMeasReadEndFrameEncoderIdx = mREFEI)
#define     GetLastMeasReadEndFrameEncoderIdx()         (lastMeasReadEndFrameEncoderIdx)


void        SetEncoderMark(EncoderMark *eMark, WORD eIdx, WORD eRem);
void        SetAdjustMark(AdjustMark *eMark, signed int eIdx, WORD eRem);



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

#define     IS_PRESS_SPEED_ZERO()       (pressSpeedZero == TRUE)


//////////////////////////////////////////////////
//
// Main Tasks Forward Declaration
//
//////////////////////////////////////////////////

void    DoMeasurementFlashTask(void);

void    DoCameraFlashTask(void);

void    DoEncoderPulseTask(void);

void    DoEndOfGrabFrameTask(void);

void    DoFlashAdjustTask(void);


void    HandleFreeRunTimerElapsed(void);

void    HandleErrors(void);



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

#define     GET_CPU_CURRENT_TIMER_TIME()            ((CH << 8) | CL)

#define     RUN_TIMER_0()                           (TR0 = SET)
#define     STOP_TIMER_0()                          (TR0 = CLEAR)

#define     PAUSE_MEASUREMENT_FREE_RUN_TIMER()      (STOP_TIMER_0())

#define     RESTART_MEASUREMENT_FREE_RUN_TIMER()    (RUN_TIMER_0())


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

#define     GetEncoderPeriod()                  (encoderPeriod)
#define     SetEncoderPeriod(eP)                (encoderPeriod = eP)

#define     GetInstantaneousEncoderPeriod()     (instantaneousEncoderPeriod)


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

#define     GetCurrentEncoderIndex()        (currentEncoderMark.encoderIndex)
#define     GetCurrentEncoderRemainder()    (currentEncoderMark.encoderRemainder)


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

#define     IsNormalQuadratureEncoderDirection()    \
                (normalQuadratureEncoderDirection == TRUE)

#define     SetNormalQuadratureEncoderDirection()   \
                (normalQuadratureEncoderDirection = TRUE)

#define     SetReverseQuadratureEncoderDirection()  \
                (normalQuadratureEncoderDirection = FALSE)

#define     IsUseImpressionPulseFlagSet()	    (useImpressionPulse == TRUE)
#define     SetUseImpressionPulseFlag()		    (useImpressionPulse = TRUE)
#define     ClearUseImpressionPulseFlag()	    (useImpressionPulse = FALSE)

#define		IsHighImpressionPulseLevelSet()	    (levelImpressionPulse == TRUE)
#define		SetHighImpressionPulseLevel()	    (levelImpressionPulse = TRUE)
#define		ClearHighImpressionPulseLevel()     (levelImpressionPulse = FALSE)


//////////////////////////////////////////////////
//
// This macro determines the approximate
// number of Encoder Indexes there are in a
// given span of time, at the current
//  encoder speed
//
//////////////////////////////////////////////////

#define     GetEncoderCountPerTotalTime(tt)     (tt/encoderPeriod)


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

// Process Synchronization States

#define     IS_SYNCHRONIZE_PROCESSES()                  (synchronizeFlag == SET)
#define     SET_SYNCHRONIZE_PROCESSES_FLAG()            (synchronizeFlag = SET)
#define     CLEAR_SYNCHRONIZE_PROCESSES_FLAG()          (synchronizeFlag = CLEAR)

#define     POSTED_PROCESS      1
#define     SET_PROCESS         2
#define     CLEAR_PROCESS       3

#define     IS_MEASUREMENT_PROCESS_POSTED()             (measurementProcessPendingFlag == POSTED_PROCESS)
#define     IS_MEASUREMENT_PROCESS_PENDING()            (measurementProcessPendingFlag == SET_PROCESS)

#define     POST_MEASUREMENT_PROCESS_PENDING_FLAG()     (measurementProcessPendingFlag = POSTED_PROCESS)
#define     SET_MEASUREMENT_PROCESS_PENDING_FLAG()      (measurementProcessPendingFlag = SET_PROCESS)
#define     CLEAR_MEASUREMENT_PROCESS_PENDING_FLAG()    (measurementProcessPendingFlag = CLEAR_PROCESS)


#define     IS_CAMERA_PROCESS_POSTED()                  (cameraProcessPendingFlag == POSTED_PROCESS)
#define     IS_CAMERA_PROCESS_PENDING()                 (cameraProcessPendingFlag == SET_PROCESS)

#define     POST_CAMERA_PROCESS_PENDING_FLAG()          (cameraProcessPendingFlag = POSTED_PROCESS)
#define     SET_CAMERA_PROCESS_PENDING_FLAG()           (cameraProcessPendingFlag = SET_PROCESS)
#define     CLEAR_CAMERA_PROCESS_PENDING_FLAG()         (cameraProcessPendingFlag = CLEAR_PROCESS)


#define     IS_MEASURE_CAPTURE_FAST_PENDING()           (measureCaptureFastPendingFlag == SET)
#define     SET_MEASURE_CAPTURE_FAST_PENDING_FLAG()     (measureCaptureFastPendingFlag = SET)
#define     CLEAR_MEASURE_CAPTURE_FAST_PENDING_FLAG()   (measureCaptureFastPendingFlag = CLEAR)

#define     IS_CAMERA_CAPTURE_FAST_PENDING()            (cameraCaptureFastPendingFlag == SET)
#define     SET_CAMERA_CAPTURE_FAST_PENDING_FLAG()      (cameraCaptureFastPendingFlag = SET)
#define     CLEAR_CAMERA_CAPTURE_FAST_PENDING_FLAG()    (cameraCaptureFastPendingFlag = CLEAR)

#define     IS_ANY_CAMERA_PROCESS_PENDING()             (IS_CAMERA_PROCESS_PENDING() || IS_CAMERA_CAPTURE_FAST_PENDING())



//////////////////////////////////////////////////
//
// Measurement Free Run States
//
//////////////////////////////////////////////////

#define     MEASUREMENT_FREE_RUNNING                    0
#define     WAITING_FOR_LAST_MEASUREMENT_READ           1
#define     PAUSED_MEASUREMENT_FREE_RUN                 2
#define     WAITING_FOR_MEASUREMENT_TIME                3
#define     WAITING_FOR_MEASUREMENT_CAPTURE_FAST_TIME   4

#define     IS_MEASUREMENT_FREE_RUNNING()               (measurementFreeRunState == MEASUREMENT_FREE_RUNNING)
#define     IS_MEASUREMENT_WAITING_FOR_LAST()           (measurementFreeRunState == WAITING_FOR_LAST_MEASUREMENT_READ)
#define     IS_MEASUREMENT_FREE_RUN_PAUSED()            (measurementFreeRunState == PAUSED_MEASUREMENT_FREE_RUN)

//////////////////////////////////////////////////
//
// Camera States
//
//////////////////////////////////////////////////

#define     CAMERA_FREE_RUNNING                         0
#define     WAITING_FOR_LAST_CAMERA_FREE_RUN            1
#define     WAITING_FOR_CAMERA_FLASH                    2


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

#define     DO_CHECK_FOR_PENDING_PROCESS()          (checkForPendingProcess == TRUE)

#define     CLEAR_CHECK_FOR_PENDING_FLAG()          (checkForPendingProcess = CLEAR)

#define     SET_CHECK_FOR_PENDING_FLAG()            (checkForPendingProcess = SET)


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

#define     IS_HOLD_PROCESSES()                      (holdProcessesFlag == TRUE)

#define     CLEAR_HOLD_PROCESSES_FLAG()              (holdProcessesFlag = CLEAR)

#define     SET_HOLD_PROCESSES_FLAG()                (holdProcessesFlag = SET)


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

#define     GetLastEncoderTimeTag()             (currInputCaptureTimeTag)

#define     SetCurrentEncoderIndex(idx)         (currentEncoderMark.encoderIndex = idx)
#define     GetCurrentEncoderIndex()            (currentEncoderMark.encoderIndex)

#define     SetCurrentEncoderRemainder(rem)     (currentEncoderMark.encoderRemainder = rem)
#define     GetCurrentEncoderRemainder()        (currentEncoderMark.encoderRemainder)

#define     GetInstantaneousCurrentEncoderIndex()           \
                (instantaneousCurrentEncoderMark.encoderIndex)

#define     GetInstantaneousCurrentEncoderRemainder()       \
                (instantaneousCurrentEncoderMark.encoderRemainder)


#define     SetInstantaneousEncoderImpressionCount(ic)      \
                (instantaneousEncoderImpressionCount = ic)

#define     GetInstantaneousEncoderImpressionCount()        \
                (instantaneousEncoderImpressionCount)

#define     ClearInstantaneousEncoderImpressionCount()      \
                (instantaneousEncoderImpressionCount = 0x0000)


#define     SetEncoderImpressionCount(iCount)   (encoderImpressionCount = iCount)
#define     GetEncoderImpressionCount()         (encoderImpressionCount)
#define     ClearEncoderImpressionCount()       (encoderImpressionCount = 0x0000)

#define     NoRemainderProcessing()             (impressionLength.encoderRemainder == 0)



//////////////////////////////////////////////////
//
// Private Data:
//
//////////////////////////////////////////////////

extern      WORD    currInputCaptureTimeTag;

extern      BYTE    xdata   tcmRxBuffer[MAX_PACKET_LENGTH];
extern      BYTE    xdata   tcmTxBuffer[MAX_PACKET_LENGTH];

extern      BYTE    receiveCheckSum;

extern      BYTE xdata toPcInterruptByte;
extern      BYTE xdata fromPcInterruptByte;


//////////////////////////////////////////////////
//
// Relative Indexing Data and Functions
//
//////////////////////////////////////////////////

extern int CalculateRelativeIndexBasedOnMeasureFlashIndex(WORD eIdx);

extern int  relCurrentEncoderIndex,
            relCameraSetupIndex,
            relHoldOffIndex,
            relMeasFreeRunStartIndex,
            relMeasFreeRunEndIndex;

#define     RelGetCurrentEncoderIndex()         (relCurrentEncoderIndex)
#define     RelSetCurrentEncoderIndex(rCEI)     (relCurrentEncoderIndex = rCEI)

#define     RelGetMeasSetupIndex()  \
                ((int)-MEASUREMENT_PREPROCESS_SETUP_INDEX_DISTANCE)

#define     RelSetCameraSetupIndex(rCSI)        (relCameraSetupIndex = rCSI)
#define     RelGetCameraSetupIndex()            (relCameraSetupIndex)

#define     RelSetMeasFreeRunStartIndex(rMFRSI) (relMeasFreeRunStartIndex = rMFRSI)
#define     RelGetMeasFreeRunStartIndex()       (relMeasFreeRunStartIndex)

#define     RelSetMeasFreeRunEndIndex(rMFREI)   (relMeasFreeRunEndIndex = rMFREI)
#define     RelGetMeasFreeRunEndIndex()         (relMeasFreeRunEndIndex)

#define     RelSetHoldOffIndex(rHOI)            (relHoldOffIndex = rHOI)
#define     RelGetHoldOffIndex()                (relHoldOffIndex)


//////////////////////////////////////////////////
//
// Extern TCM/PC Handshake Flags
// Location Definitions
//
//////////////////////////////////////////////////

#define    CAM_FLAG_CLEAR       0x55
#define    CAM_FLAG_SET         0xAA

#define    MEAS_FLAG_CLEAR      0x55
#define    MEAS_FLAG_SET        0xAA

extern BYTE xdata      cameraProcessFlagMirror;
extern BYTE xdata      measurementProcessFlagMirror;

extern BYTE xdata       tcmPcReservedFlag3,
                        tcmPcReservedFlag4,
                        tcmPcReservedFlag5,
                        tcmPcReservedFlag6,
                        tcmPcReservedFlag7,
                        tcmPcReservedFlag8,
                        tcmPcReservedFlag9,
                        tcmPcReservedFlag10,
                        tcmPcReservedFlag11,
                        tcmPcReservedFlag12,
                        tcmPcReservedFlag13,
                        tcmPcReservedFlag14,
                        tcmPcReservedFlag15,
                        tcmPcReservedFlag16;


extern      EncoderMark     cameraFlashEncoderMark,
                            measurementFlashEncoderMark,
                            lastMeasFreeRunEncoderMark;

extern      AdjustMark      measurementFlashAdjustMark;

extern      WORD            measurementPreProcessSetupIndex,
                            cameraPreProcessSetupIndex,
                            lastMeasFreeRunSetupIndex;

extern      BYTE            measurementFlashSetupDistance;

extern      EncoderMark     impressionLength;

extern      EncoderMark     instantaneousCurrentEncoderMark;
extern      EncoderMark     currentEncoderMark;

extern      WORD            measFrameTime;
extern      WORD            camFrameReadoutTime;

extern      DWORD           encoderImpressionCount;
extern      DWORD           instantaneousEncoderImpressionCount;

extern      WORD            measurementFreeRunIntervalTime;

extern      WORD            instantaneousEncoderPeriod;
extern      WORD            encoderPeriod;

extern      BOOL            adjustmentOkFlag, adjustmentNeededFlag, adjustmentLateFlag;

extern      WORD            resetIndexPoint;
extern      BOOL            resetEncoderAtIndexFlag;

extern      BOOL            pressSpeedZero;

extern      BOOL            normalQuadratureEncoderDirection;

extern 		BOOL            useImpressionPulse;
extern 		BOOL            levelImpressionPulse;

extern      BOOL            measurementProcessPendingFlag;
extern      BOOL            cameraProcessPendingFlag;

extern      BOOL            measureCaptureFastPendingFlag;
extern      BOOL            cameraCaptureFastPendingFlag;

extern      BYTE            measurementFreeRunState;
extern      BYTE            cameraState;

extern      WORD            current_PCA_time;

extern      BOOL            synchronizeFlag;
extern      BOOL            holdProcessesFlag;
extern      BOOL            checkForPendingProcess;



//////////////////////////////////////////////////
//
// MR = Measure Read (Free Run) Variables
//
//////////////////////////////////////////////////

extern      WORD     lastMeasReadStartFrameEncoderIdx;
extern      WORD     lastMeasReadEndFrameEncoderIdx;
extern      WORD     lastMeasFreeRunIdx;



#endif







