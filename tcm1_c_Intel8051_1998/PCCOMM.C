



/*
 *
 *
 *    $Header:   L:/RR/PROJ829/tcm2/source/PCCOMM.C_v   1.6   05 Aug 1998 09:09:16   PaulLC  $
 *    $Log:   L:/RR/PROJ829/tcm2/source/PCCOMM.C_v  $
 * 
 *    Rev 1.6   05 Aug 1998 09:09:16   PaulLC
 * Camera Frame White Streak fixed in this version. A new message was added to allow the OP to query the 
 * TCM system parameters. The Camera/Measure capture fast, now controls the Camera/Measure Process Flags.
 * 
 *    Rev 1.5   31 Jul 1998 09:12:20   PaulLC
 * Fixed issue where the measurement flash could be delayed by as much as 65msec. 
 * The Measurement Flash setup index distance is now a parameter downloaded by the OP. 
 * 
 *    Rev 1.4   24 Jul 1998 11:19:16   PaulLC
 * Reset Measure FIFO was moved from the "Measure Flash Setup" section over to the 
 * "Synchronize Command" section. A Mirror flag was added for the Measurement Process
 * to allow the OP to check its completion. Relative Encoder Index is now updated on each
 * Encoder Pulse Task. The Last Free Run pulse is now triggered for the Measure Capture 
 * Fast Process.
 * 
 *    Rev 1.3   21 Jul 1998 16:42:34   PaulLC
 * Optimized floating point calculations to be done in the OP. The Synchronize message was
 * revised to accomodate the new OP Data. Divide by 64K were optimized for speed. Redesigned
 * Measure Free run and Synchronize Message task to be encoder-based processes.
 * 
 *    Rev 1.2   07 Jul 1998 13:39:56   PaulLC
 * Fixed ResetAtEncoderIndex feature. Added a new message to allow the Encoder
 * Directio bit to be querried. Added new defines for this message. Made an explicit 
 * call to Reset Measure FIFO when initialized on TCM reset.
 * 
 *    Rev 1.1   17 Jun 1998 13:59:02   PaulLC
 * Fixed Reverse Encoder test problem by taking out debug sections.  Fixed Set Impression command
 * to update both background and foreground variables. Made TCM 2 version numbering to be 5 digits.
 * 
 *    Rev 1.0   11 Jun 1998 13:12:04   Paul L C
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
// PC Command Handlers, Forward declarations
//
//////////////////////////////////////////////////

void    DoGetVersion(void);

void    DoSetAdjustmentParameters(void);
void    DoSetCameraParameters(void);
void    DoSetMeasurementParameters(void);
void    DoSynchronizeCameraAndMeasurementProcess(void);

void    DoMeasureCaptureFast(void);
void    DoCameraCaptureFast(void);

void    DoMeasurementFifoResetCommand(void);
void    DoProbeHeadResetCommand(void);

void    DoQueryEncoderIndexCommand(void);
void    DoQueryEncoderPeriodCommand(void);
void    DoQueryImpressionCountCommand(void);
void    DoQueryAdjustmentOkFlagCommand(void);
void    DoQueryProcessStatusFlagCommand(void);
void    DoQueryEncoderDirectionStatusCommand(void);
void    DoGetSystemParameters(void);

void    DoResetEncoderAtEncoderIndex(void);

void    DoSetSystemParameters(void);
void    DoSetImpressionCount(void);

void    DoDebug_0(void);


//////////////////////////////////////////////////
//
// Private Data
//
//////////////////////////////////////////////////

BYTE    xdata   tcmTxBuffer[MAX_PACKET_LENGTH]  _at_    0xB80;
BYTE    xdata   tcmRxBuffer[MAX_PACKET_LENGTH]  _at_    0xBBF;

BYTE xdata toPcInterruptByte    _at_    0xBfe;
BYTE xdata fromPcInterruptByte  _at_    0xBff;

WORD    resetIndexPoint;
BOOL    resetEncoderAtIndexFlag;


BYTE    pcMessageErrorLog;

#define     GET_PC_COMM_ERROR_LOG()  (pcMessageErrorLog)

#define     LENGTH_ERROR            1
#define     CHECK_SUM_ERROR         2
#define     UNKNOWN_COMMAND_ERROR   3
#define     DATA_VALIDATION_ERROR   4

#define     LOG_LENGTH_ERROR()          (pcMessageErrorLog = LENGTH_ERROR)
#define     LOG_CHECK_SUM_ERROR()       (pcMessageErrorLog = CHECK_SUM_ERROR)
#define     LOG_UNKNOWN_COMMAND_ERROR() (pcMessageErrorLog = UNKNOWN_COMMAND_ERROR)
#define     LOG_DATA_VALIDATION_ERROR() (pcMessageErrorLog = DATA_VALIDATION_ERROR)

#define     CLEAR_PC_COMM_ERROR_LOG()   (pcMessageErrorLog = 0x00)



//////////////////////////////////////////////////
//
// TCM/PC Handshake Flags
// Location Definitions
//
//////////////////////////////////////////////////

BYTE xdata      cameraProcessFlagMirror         _at_    0xB70;
BYTE xdata      measurementProcessFlagMirror    _at_    0xB71;

BYTE xdata      tcmPcReservedFlag3          _at_    0xB72;
BYTE xdata      tcmPcReservedFlag4          _at_    0xB73;
BYTE xdata      tcmPcReservedFlag5          _at_    0xB74;
BYTE xdata      tcmPcReservedFlag6          _at_    0xB75;
BYTE xdata      tcmPcReservedFlag7          _at_    0xB76;
BYTE xdata      tcmPcReservedFlag8          _at_    0xB77;
BYTE xdata      tcmPcReservedFlag9          _at_    0xB78;
BYTE xdata      tcmPcReservedFlag10         _at_    0xB79;
BYTE xdata      tcmPcReservedFlag11         _at_    0xB7a;
BYTE xdata      tcmPcReservedFlag12         _at_    0xB7b;
BYTE xdata      tcmPcReservedFlag13         _at_    0xB7c;
BYTE xdata      tcmPcReservedFlag14         _at_    0xB7d;
BYTE xdata      tcmPcReservedFlag15         _at_    0xB7e;
BYTE xdata      tcmPcReservedFlag16         _at_    0xB7f;


//////////////////////////////////////////////////
//
// Relative Indexing Data and Functions
//
//////////////////////////////////////////////////

int     relCameraSetupIndex,        relHoldOffIndex,
        relMeasFreeRunStartIndex,   relMeasFreeRunEndIndex;

int CalculateRelativeIndexBasedOnMeasureFlashIndex(WORD encIdx)
{
    int relIdxVal;

        relIdxVal = ((int)encIdx - GetMeasurementFlashEncoderIndex());

        if(relIdxVal > 0)
            relIdxVal = -((int)(GetImpressionLengthIndex() - encIdx) +
                          GetMeasurementFlashEncoderIndex());

        return relIdxVal;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    InitPcComm(void)
{


}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    HandlePcMessageReceived(void)
{
    BYTE    pcCommand;
    BYTE    xdata * ptrPcMessage;

        ptrPcMessage = GetTcmRxBuffer();

        pcCommand =  *(ptrPcMessage + COMMAND_SLOT);

    switch(pcCommand)
    {
        case    SET_ADJUSTMENT_PARAMETERS:
                        DoSetAdjustmentParameters();
                        break;

        case    SET_CAMERA_PARAMETERS:
                        DoSetCameraParameters();
                        break;

        case    SET_MEASUREMENT_PARAMETERS:
                        DoSetMeasurementParameters();
                        break ;

        case    QUERY_ENCODER_INDEX:
                        DoQueryEncoderIndexCommand();
                        break ;

        case    SYNCHRONIZE_CAM_MEAS_PROCESS:
                        DoSynchronizeCameraAndMeasurementProcess();
                        break ;

        case    QUERY_ENCODER_PERIOD:
                        DoQueryEncoderPeriodCommand();
                        break;

        case    QUERY_IMPRESSION_COUNT:
                        DoQueryImpressionCountCommand();
                        break ;

        case    QUERY_ADJUSTMENT_OK_FLAG:
                        DoQueryAdjustmentOkFlagCommand();
                        break ;

        case    QUERY_PROCESS_STATUS_FLAG:
                        DoQueryProcessStatusFlagCommand();
                        break ;

        case    RESET_ENCODER_AT_ENCODER_INDEX:
                        DoResetEncoderAtEncoderIndex();
                        break ;

        case    MEASURE_CAPTURE_FAST:
                        DoMeasureCaptureFast();
                        break ;

        case    CAMERA_CAPTURE_FAST:
                        DoCameraCaptureFast();
                        break ;

        case    GET_VERSION:
                        DoGetVersion();
                        break ;

        case    SET_SYSTEM_PARAMETERS:
                        DoSetSystemParameters();
                        break;

        case    QUERY_SYSTEM_PARAMETERS:
                        DoGetSystemParameters();
                        break;

        case    SET_IMPRESSION_COUNT:
                        DoSetImpressionCount();
                        break;

        case    QUERY_ENCODER_DIRECTION_STATUS_FLAG:
                        DoQueryEncoderDirectionStatusCommand();
                        break;

        case    MEASUREMENT_FIFO_RESET:
                        DoMeasurementFifoResetCommand();
                        break;

        case    PROBE_HEAD_RESET:
                        DoProbeHeadResetCommand();
                        break;

        case    DEBUG_0_COMMAND :
                        DoDebug_0();
                        break;

        default:
            LOG_UNKNOWN_COMMAND_ERROR();
            HandlePcMessageError();
            break;
    }

    CLEAR_PC_MESSAGE_RECEIVED_FLAG();
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL    IsPcMessageOk(void)
{
    BYTE    xdata *ptrRxBuffer = GetTcmRxBuffer();

    BYTE    len = (*ptrRxBuffer);           // First Byte

    BYTE    checkSum = len;

    BYTE    l;

        if(len > MAX_PACKET_LENGTH)         // is Length OK ?
        {
            LOG_LENGTH_ERROR();
            return FALSE;
        }

        --len;                              // 1 Less for the
                                            // checksum data slot

        // Calculate to Generate Checksum

        for(l=1; l < len; l++)
        {
            ++ptrRxBuffer;
            checkSum += (*ptrRxBuffer);
        }

        if(checkSum != GetTcmRxCheckSum())  // is Checksum OK ?
        {
            LOG_CHECK_SUM_ERROR();
            return FALSE;
        }

        CLEAR_PC_COMM_ERROR_LOG();

        return TRUE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    HandlePcMessageError(void)
{
    StartNakTransmission(GET_PC_COMM_ERROR_LOG());
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoSetAdjustmentParameters(void)
{
    xdata void * ptrPcMessageBuffer = GetTcmRxBuffer();

    signed int ptrMeasIndex
                = *(signed int *)(ptrPcMessageBuffer+2); //encoder index adjustment

    WORD    ptrMeasRemainder
                = *(WORD *)(ptrPcMessageBuffer+4);       //replacement remainder

		if(!IS_ADJUSTMENT_LATE())
		{
	        SetMeasurementFlashAdjustMark(ptrMeasIndex, ptrMeasRemainder);

			SET_ADJUSTMENT_OK_FLAG();
            SET_ADJUSTMENT_NEEDED_FLAG();
		}
		else
		{
			CLEAR_ADJUSTMENT_OK_FLAG();		//flash already setup, too late now
		}

	    StartAckTransmission(SET_ADJUSTMENT_PARAMETERS);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoSetCameraParameters(void)
{
    signed  int camPreProcSetupIdx;

    xdata void * ptrPcMessageBuffer = GetTcmRxBuffer();

    WORD    ptrCamIndex     = *(WORD *)(ptrPcMessageBuffer+2);
    WORD    ptrCamRemainder = *(WORD *)(ptrPcMessageBuffer+4);

        SetCameraFlashEncoderMark(ptrCamIndex, ptrCamRemainder);

        camPreProcSetupIdx =
            ptrCamIndex - CAMERA_PREPROCESS_SETUP_INDEX_DISTANCE;

        // Check for underflow, Normalize as necessary

        NormalizeEncoderIndex(&camPreProcSetupIdx);

        SetCameraPreProcessSetupIndex(camPreProcSetupIdx);

	    StartAckTransmission(SET_CAMERA_PARAMETERS);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoSetMeasurementParameters(void)
{
    signed  int measPreProcSetupIdx;

    xdata void * ptrPcMessageBuffer = GetTcmRxBuffer();

    WORD    ptrMeasIndex       = *(WORD *)(ptrPcMessageBuffer+2);
    WORD    ptrMeasRemainder   = *(WORD *)(ptrPcMessageBuffer+4);
    BOOL    measureWithFlash   = *(BOOL *)(ptrPcMessageBuffer+6);

        SetMeasurementFlashEncoderMark(ptrMeasIndex, ptrMeasRemainder);

        measPreProcSetupIdx =
            GetMeasurementFlashEncoderIndex() - GetMeasurementFlashSetupDistance();

        // Check for underflow, Normalize as necessary

        NormalizeEncoderIndex(&measPreProcSetupIdx);

        SetMeasurementPreProcessSetupIndex(measPreProcSetupIdx);

        if(measureWithFlash == SET)
           MEASURE_WITH_FLASH();
		else
		   MEASURE_WITHOUT_FLASH();

    StartAckTransmission(SET_MEASUREMENT_PARAMETERS);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

#define HOLD_OFF_THRESHOLD  1

void    DoSynchronizeCameraAndMeasurementProcess(void)
{
    signed int  holdoffThresholdIdx;

    xdata void * ptrPcMessageBuffer = GetTcmRxBuffer();

    BOOL    runCameraProcess        = *(BOOL *)(ptrPcMessageBuffer+2);
    BOOL    runMeasurementProcess   = *(BOOL *)(ptrPcMessageBuffer+3);
    WORD    startIdx                = *(WORD *)(ptrPcMessageBuffer+4);
    WORD    endIdx                  = *(WORD *)(ptrPcMessageBuffer+6);
    WORD    lastFreeRunIndex        = *(WORD *)(ptrPcMessageBuffer+8);
    WORD    lastFreeRunRemainder    = *(WORD *)(ptrPcMessageBuffer+10);

        CLEAR_HOLD_PROCESSES_FLAG();

        if(runCameraProcess == TRUE)
        {
            POST_CAMERA_PROCESS_PENDING_FLAG();

            cameraProcessFlagMirror = CAM_FLAG_SET;
        }

        if(runMeasurementProcess == TRUE)
        {
            POST_MEASUREMENT_PROCESS_PENDING_FLAG();

            CLEAR_ADJUSTMENT_OK_FLAG();
            CLEAR_ADJUSTMENT_NEEDED_FLAG();
            CLEAR_ADJUSTMENT_LATE_FLAG();

            RESET_MEASURE_FIFO();

            SetLastMeasReadStartFrameEncoderIdx(startIdx);
            SetLastMeasReadEndFrameEncoderIdx(endIdx);

            SetLastMeasFreeRunIndex(lastFreeRunIndex);
            SetLastMeasFreeRunRemainder(lastFreeRunRemainder);

            measurementProcessFlagMirror = MEAS_FLAG_SET;
        }

        if(IS_MEASUREMENT_PROCESS_POSTED() && IS_CAMERA_PROCESS_POSTED())
        {
            SET_SYNCHRONIZE_PROCESSES_FLAG();

            holdoffThresholdIdx  = endIdx - HOLD_OFF_THRESHOLD; // Restrict Window
            NormalizeEncoderIndex(&holdoffThresholdIdx);        // Check if End Encoder Index
                                                                // will have to be normalized
            //
            // Set Up Relative Indexes
            //

            RelSetMeasFreeRunStartIndex
            (CalculateRelativeIndexBasedOnMeasureFlashIndex
            (GetLastMeasReadStartFrameEncoderIdx()));

            RelSetMeasFreeRunEndIndex
            (CalculateRelativeIndexBasedOnMeasureFlashIndex
            (GetLastMeasReadEndFrameEncoderIdx()));

            RelSetCameraSetupIndex
            (CalculateRelativeIndexBasedOnMeasureFlashIndex
            (GetCameraPreProcessSetupIndex())
            - HOLD_OFF_THRESHOLD);

            RelSetHoldOffIndex
            (CalculateRelativeIndexBasedOnMeasureFlashIndex
            (holdoffThresholdIdx));
        }
        else
        {
            if(IS_CAMERA_PROCESS_POSTED())
                SET_CAMERA_PROCESS_PENDING_FLAG();

            if(IS_MEASUREMENT_PROCESS_POSTED())
                SET_MEASUREMENT_PROCESS_PENDING_FLAG();
        }

    StartAckTransmission(SYNCHRONIZE_CAM_MEAS_PROCESS);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoMeasureCaptureFast(void)
{
    xdata void * ptrPcMessageBuffer = GetTcmRxBuffer();

    BOOL    measureWithFlash    = *(BOOL *)(ptrPcMessageBuffer+2);

        if(measureWithFlash == SET)
            MEASURE_WITH_FLASH();
        else
            MEASURE_WITHOUT_FLASH();

        SET_MEASURE_CAPTURE_FAST_PENDING_FLAG();

        measurementProcessFlagMirror = MEAS_FLAG_SET;

	    StartAckTransmission(MEASURE_CAPTURE_FAST);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoCameraCaptureFast(void)
{
	WORD    cameraFlashOutputCompareTime;

        cameraState = WAITING_FOR_LAST_CAMERA_FREE_RUN;

        cameraFlashOutputCompareTime = GET_CPU_CURRENT_TIMER_TIME();

        // - Camera Frame time

        cameraFlashOutputCompareTime += camFrameReadoutTime;

	    CCAP1L = LOBYTE(cameraFlashOutputCompareTime);
	    CCAP1H = HIBYTE(cameraFlashOutputCompareTime);

        ENABLE_OUTPUT_COMPARE_1();    //just interrupt needed, not bit toggle

        SET_CAMERA_CAPTURE_FAST_PENDING_FLAG();

        cameraState = WAITING_FOR_LAST_CAMERA_FREE_RUN;

        cameraProcessFlagMirror = CAM_FLAG_SET;

        StartAckTransmission(CAMERA_CAPTURE_FAST);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoMeasurementFifoResetCommand(void)
{
        RESET_MEASURE_FIFO();

    	StartAckTransmission(MEASUREMENT_FIFO_RESET);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoProbeHeadResetCommand(void)
{
        RESET_PROBE_HEAD();

    	StartAckTransmission(PROBE_HEAD_RESET);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoGetVersion(void)
{
    BYTE    xdata * ptrReplyBuffer = GetTcmTxBuffer();

        *(ptrReplyBuffer)   = 6;                        // Length
        *(ptrReplyBuffer+1) = GET_VERSION;              // Cmd

        // Add VERSION Data into reply buffer

        *(ptrReplyBuffer+2) = MAJOR_VERSION;
        *(ptrReplyBuffer+3) = MINOR_VERSION;
        *(ptrReplyBuffer+4) = INTERMEDIATE_VERSION;

    	StartReplyTransmission();
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoQueryEncoderIndexCommand(void)
{
    BYTE    xdata * ptrReplyBuffer = GetTcmTxBuffer();

        *(ptrReplyBuffer)   = 5;                        // Length
        *(ptrReplyBuffer+1) = QUERY_ENCODER_INDEX;      // Cmd

        // Add DWORD Data into reply buffer

        (WORD)(*(ptrReplyBuffer+2)) = GetCurrentEncoderIndex();

    	StartReplyTransmission();
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoQueryEncoderPeriodCommand(void)
{
    BYTE    xdata * ptrReplyBuffer = GetTcmTxBuffer();

        *(ptrReplyBuffer)   = 5;                        // Length
        *(ptrReplyBuffer+1) = QUERY_ENCODER_PERIOD;     // Cmd

        // Add WORD Data into reply buffer

        (WORD)(*(ptrReplyBuffer+2)) = GetEncoderPeriod();

    	StartReplyTransmission();
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoQueryImpressionCountCommand(void)
{
    BYTE    xdata * ptrReplyBuffer = GetTcmTxBuffer();

        *(ptrReplyBuffer)   = 7;                        // Length
        *(ptrReplyBuffer+1) = QUERY_IMPRESSION_COUNT;   // Cmd

        // Add DWORD Data into reply buffer

        (DWORD)(*(ptrReplyBuffer+2)) = GetEncoderImpressionCount();

    StartReplyTransmission();
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoQueryAdjustmentOkFlagCommand(void)
{
    BYTE    xdata * ptrReplyBuffer = GetTcmTxBuffer();

        *(ptrReplyBuffer)   = 4;                            // Length
        *(ptrReplyBuffer+1) = QUERY_ADJUSTMENT_OK_FLAG;     // Cmd

        // Add Data into reply buffer

        *(ptrReplyBuffer+2) = GET_ADJUSTMENT_OK_FLAG();

        CLEAR_ADJUSTMENT_OK_FLAG();                         // Reset Flag

    StartReplyTransmission();
}


//////////////////////////////////////////////////
//
//	This command indicates whether there is a
//  camera command setup or measurement setup that
//  has not been completed yet.
//  The camera process Flag is set at camera
//  setup and cleared at end of frame grab
//	signal event.
//
//////////////////////////////////////////////////

void    DoQueryProcessStatusFlagCommand(void)
{
    BYTE    xdata * ptrReplyBuffer = GetTcmTxBuffer();

        *(ptrReplyBuffer)   = 5;                            // Length
        *(ptrReplyBuffer+1) = QUERY_PROCESS_STATUS_FLAG;    // Cmd

        // Add Data into reply buffer

        *(ptrReplyBuffer+2) = IS_ANY_CAMERA_PROCESS_PENDING();
        *(ptrReplyBuffer+3) = IS_MEASUREMENT_PROCESS_PENDING();

    	StartReplyTransmission();
}


//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

void    DoQueryEncoderDirectionStatusCommand(void)
{
    BYTE    xdata * ptrReplyBuffer = GetTcmTxBuffer();

        *(ptrReplyBuffer)   = 4;                                    // Length
        *(ptrReplyBuffer+1) = QUERY_ENCODER_DIRECTION_STATUS_FLAG;  // Cmd

        // Add Data into reply buffer

        *(ptrReplyBuffer+2) = IS_ENCODER_RUNNING_FORWARD();

    	StartReplyTransmission();
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoResetEncoderAtEncoderIndex(void)
{
    xdata void * ptrPcMessageBuffer = GetTcmRxBuffer();

    WORD    ptrResetIdxPt = *(WORD *)(ptrPcMessageBuffer+2);

        SetEncoderResetIndexPoint(ptrResetIdxPt);

        SET_RESET_ENCODER_AT_INDEX_FLAG();

    	StartAckTransmission(RESET_ENCODER_AT_ENCODER_INDEX);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoSetSystemParameters(void)
{
    xdata void * ptrPcMessageBuffer = GetTcmRxBuffer();

    WORD    ptrMeasFrameTime            = *(WORD *)(ptrPcMessageBuffer+2);
    WORD    ptrCameraFrameReadTime      = *(WORD *)( ptrPcMessageBuffer+4);

    WORD    ptrEncIdxPerImpression      = *(WORD *)(ptrPcMessageBuffer+6);
    WORD    ptrEncRemainPerImpression   = *(WORD *)(ptrPcMessageBuffer+8);

    WORD    ptrMeasFreeRunTime          = *(WORD *)(ptrPcMessageBuffer+10);
    DWORD   ptrImpressionCnt            = *(DWORD *)(ptrPcMessageBuffer+12);

    BOOL    encoderDirFlagNormal        = *(BOOL *)(ptrPcMessageBuffer+16);
    BOOL    useImpressionPulseFlag      = *(BOOL *)(ptrPcMessageBuffer+17);
    BOOL    impressionPulseLevelHi      = *(BOOL *)(ptrPcMessageBuffer+18);
    BYTE    measFlashSetupDistance      = *(BYTE *)(ptrPcMessageBuffer+19);

        SetMeasurementFrameTime(ptrMeasFrameTime);
        SetCameraFrameReadoutTime(ptrCameraFrameReadTime);
        SetImpressionLength(ptrEncIdxPerImpression, ptrEncRemainPerImpression);

        SetMeasurementFreeRunIntervalTime(ptrMeasFreeRunTime);

        SetInstantaneousEncoderImpressionCount(ptrImpressionCnt);
        SetEncoderImpressionCount(ptrImpressionCnt);

        if(encoderDirFlagNormal == TRUE)
            SetNormalQuadratureEncoderDirection();
        else
            SetReverseQuadratureEncoderDirection();

        if(useImpressionPulseFlag == TRUE)
        {
            SetUseImpressionPulseFlag();

            if(impressionPulseLevelHi == TRUE)
                SetHighImpressionPulseLevel();
            else
                ClearHighImpressionPulseLevel();
        }
        else
        {
            ClearUseImpressionPulseFlag();
        }

        SetMeasurementFlashSetupDistance(measFlashSetupDistance);

    StartAckTransmission(SET_SYSTEM_PARAMETERS);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////


void    DoGetSystemParameters(void)
{
    BYTE    xdata * ptrReplyBuffer = GetTcmTxBuffer();

        *(ptrReplyBuffer)   = 0x14;                         // Length
        *(ptrReplyBuffer+1) = QUERY_SYSTEM_PARAMETERS;      // Cmd

        // Add Data into reply buffer

        (WORD)(*(ptrReplyBuffer+2))    = GetMeasurementFrameTime();
        (WORD)(*(ptrReplyBuffer+4))    = GetCameraFrameReadoutTime();
        (WORD)(*(ptrReplyBuffer+6))    = GetImpressionLengthIndex();
        (WORD)(*(ptrReplyBuffer+8))    = GetImpressionLengthRemainder();
        (WORD)(*(ptrReplyBuffer+10))   = GetMeasurementFreeRunIntervalTime();
        (DWORD)(*(ptrReplyBuffer+12))  = GetEncoderImpressionCount();
        (BOOL)(*(ptrReplyBuffer+16))   = IsNormalQuadratureEncoderDirection();
        (BOOL)(*(ptrReplyBuffer+17))   = IsUseImpressionPulseFlagSet();
        (BOOL)(*(ptrReplyBuffer+18))   = IsHighImpressionPulseLevelSet();
        (BYTE)(*(ptrReplyBuffer+19))   = GetMeasurementFlashSetupDistance();

    StartReplyTransmission();
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoSetImpressionCount(void)
{
    xdata void * ptrPcMessageBuffer = GetTcmRxBuffer();

    DWORD   ptrImpressionCount =  *(DWORD *)(ptrPcMessageBuffer+2);

        SetInstantaneousEncoderImpressionCount(ptrImpressionCount);
        SetEncoderImpressionCount(ptrImpressionCount);

    StartAckTransmission(SET_IMPRESSION_COUNT);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoDebug_0(void)
{
    BYTE    xdata * ptrReplyBuffer = GetTcmTxBuffer();

        *(ptrReplyBuffer) = 0x07;
        *(ptrReplyBuffer+1) = DEBUG_0_COMMAND;
        (DWORD)*(ptrReplyBuffer+2) = 0x11223344;

    StartReplyTransmission();
}


