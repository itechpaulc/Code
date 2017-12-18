





//
//
//
//  Author :    Paul Calinawan
//
//  Date:       Jan 24 , 2012
//
//  Copyrights: Imaging Technologies Inc.
//
//  Product:    ITECH PLC2
//  
//  Subsystem:  Absolute Encoder Monitor Board
//
//  -------------------------------------------
//
//
//      CONFIDENTIAL DOCUMENT
//
//      Property of Imaging Technologies Inc.
//
//




////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////

#include "itechsys.h"


////////////////////////////////////////////////////////////////////


#include "kernel.h"

#include "spidevicemanager.h"




////////////////////////////////////////////////////////////////////


#include <avr/io.h>


#include "encoderhandler.h"

#include "displaysystemmanager.h"



////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////

//
// Programmable Parameters
//

U16		encoderTypeCount;

BOOL	enoderReversed;

U8		encoderMovingTolerance;

//
// Local Variables
//

U16		encoderRawGrayBitData;

//

U16		encoderTrueBinaryValueCurr,
		encoderTrueBinaryValuePrev;

#define MAX_ENCODER_VAL_HISTORY		8

U16		encoderTrueBinaryValueHistory[MAX_ENCODER_VAL_HISTORY];

U8		encoderValHistIdx;
U32		encoderValHistTotal;
U16		encoderValHistAverage;

U8		encoderFilter;

U8		encoderSamplingRate;


//

U8		encoderScaledValue;

BOOL	encoderIsAtMidPoint;

BOOL	encoderValueChanged;


///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////

void	SetEncoderTypeCount(U16 encTypeCount)
{
	encoderTypeCount = encTypeCount;
}

U16	GetEncoderTypeCount(void)
{
	return encoderTypeCount;
}

//
// Note:
// From the shaft end, looking into the encoder body
//
// Rotating the shaft Clockwise is NORMAL
// Rotating the shaft Clockwise is REVERSED
//

void	SetEncoderReversed(void)
{
	enoderReversed = TRUE;
}

void	ClearEncoderReversed(void)
{
	enoderReversed = FALSE;
}

BOOL	IsEncoderReversed(void)
{
	return enoderReversed;
}

//
//
//

void	SetEncoderFilter(U8 encFilter)
{
	encoderFilter = encFilter;
}

U16	GetEncoderFilter(void)
{
	return encoderFilter;
}

//
//
//

void	SetEncoderSamplingRate(U8 encSamplRate)
{
	encoderSamplingRate = encSamplRate;
}

U8		GetEncoderSamplingRate(void)
{
	return encoderSamplingRate;
}


//
//
//

void	SetEncoderRawGrayBitData(U16 encRawGrayBitDat)
{
	encoderRawGrayBitData = encRawGrayBitDat;
}

U16	GetEncoderRawGrayBitData(void)
{
	return encoderRawGrayBitData;
}

//

void	SetEncoderTrueBinaryValueCurr(U16 encTrueBinVal)
{
	encoderTrueBinaryValueCurr = encTrueBinVal;
}

U16	GetEncoderTrueBinaryValueCurr(void)
{
	return encoderTrueBinaryValueCurr;
}

//

void	SetEncoderScaledValue(U16 encScaledVal)
{
	encoderScaledValue = encScaledVal;
}

U8	GetEncoderScaledValue(void)
{
	return encoderScaledValue;
}


#define VAL_SCALE_OVERFLOW_1024		(655)

#define MAX_SCALE_VALUE				(99)

void	UpdatEncoderScaledValue(void)
{
	U32 currValScaled100;
	
	U8	scaledVal;	
	
	U16	encTrueBinValCurr = GetEncoderTrueBinaryValueCurr();
	
		currValScaled100 = encTrueBinValCurr * 100;
		
		scaledVal = currValScaled100 / GetEncoderTypeCount();
					
		if((GetEncoderTypeCount() == ENCODER_TYPE_COUNT_1024) &&
			(encTrueBinValCurr > VAL_SCALE_OVERFLOW_1024))
				scaledVal += 64;
		
		if(IsEncoderReversed())
			scaledVal = MAX_SCALE_VALUE - scaledVal;			
			
	SetEncoderScaledValue(scaledVal);	
}

//

void	SetEncoderMovingTolerance(U16 encMovingTol)
{
	encoderMovingTolerance = encMovingTol;
}

U8	GetEncoderMovingTolerance(void)
{
	return encoderMovingTolerance;
}

//

void	SetEncoderIsAtMidPoint(void)
{
	encoderIsAtMidPoint = TRUE;
}

void	ClearEncoderIsAtMidPoint(void)
{
	encoderIsAtMidPoint = FALSE;
}

BOOL	IsEncoderAtMidPoint(void)
{
	return encoderIsAtMidPoint;
}

//
//
//

void	CheckEncoderIsAtMidPoint(void)
{
	U16	encMidCount;
	
	signed int	countDiff;
	
		encMidCount = GetEncoderTypeCount() >> 1;
		
		countDiff = (GetEncoderTrueBinaryValueCurr() - encMidCount);
		
		countDiff = abs(countDiff);
		
		if(countDiff == GetEncoderMovingTolerance())
		{
			SetEncoderIsAtMidPoint();
			
			SetDecimalPointOnes();
			SetDecimalPointTens();
		}		
		else
		{
			ClearEncoderIsAtMidPoint();
			
			ClearDecimalPointOnes();
			ClearDecimalPointTens();
		}				
}

//
//
//

U16	GetEncoderTrueBinaryValuePrev(void)
{
	return encoderTrueBinaryValuePrev;
}
 
void	IsCheckEncoderValueChange(void)
{
	int encChangeDiff;
	
		encChangeDiff = 
			abs (GetEncoderTrueBinaryValueCurr() - GetEncoderTrueBinaryValuePrev());
		
		if(encChangeDiff > 1)
		{
			encoderValueChanged = TRUE;
		
			encoderTrueBinaryValuePrev = GetEncoderTrueBinaryValueCurr();
			
			SendMessage(WatchDogManager, EncMovingWatchdogReset);
		}
		else
		{
			encoderValueChanged = FALSE;
		}
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

//
// Thanks to WIKIPEDIA
//

void	ConvertEncoderGrayDataToDecimal(void)
{
	U16 etbv = 
		encoderRawGrayBitData ^ (encoderRawGrayBitData>>16);
        
		etbv ^= (etbv>>8);
		
		etbv ^= (etbv>>4);
        etbv ^= (etbv>>2);
        etbv ^= (etbv>>1);
	
		SetEncoderTrueBinaryValueCurr(etbv);		
}

U16		GetEncoderHistoryAverage(void)
{
	U8 eIdx, shiftDiv;
	
	encoderValHistTotal = 0x0000;
	
		for(eIdx=0; eIdx < encoderFilter; eIdx++)
		{
			encoderValHistTotal += 
				encoderTrueBinaryValueHistory[eIdx];
		}
			
		switch(encoderFilter)
		{
			case	ENCODER_FILTER_0: shiftDiv = 0; break;
			case	ENCODER_FILTER_2: shiftDiv = 1; break;
			case	ENCODER_FILTER_4: shiftDiv = 2; break;
			case	ENCODER_FILTER_8: shiftDiv = 3; break;									
			
			default: break;
		}		
		
		encoderValHistAverage = 
			(encoderValHistTotal >> shiftDiv);
		
	return encoderValHistAverage;
}

void	FilterEncoderData(void)
{		
		switch(encoderFilter)
		{
			case ENCODER_FILTER_0:
				// No Filter
				break;
				
			case ENCODER_FILTER_2:
				if(encoderValHistIdx == ENCODER_FILTER_2)
					encoderValHistIdx = 0;
					
				encoderTrueBinaryValueHistory[encoderValHistIdx]
					= encoderTrueBinaryValueCurr;
					
				SetEncoderTrueBinaryValueCurr
					(GetEncoderHistoryAverage());
				break;
				
			case ENCODER_FILTER_4:
				if(encoderValHistIdx == ENCODER_FILTER_4)
					encoderValHistIdx = 0;	
					
				encoderTrueBinaryValueHistory[encoderValHistIdx]
					= encoderTrueBinaryValueCurr;						
				SetEncoderTrueBinaryValueCurr
					(GetEncoderHistoryAverage());				
				break;		
					
			case ENCODER_FILTER_8:
				if(encoderValHistIdx == ENCODER_FILTER_8)
					encoderValHistIdx = 0;		
					
				encoderTrueBinaryValueHistory[encoderValHistIdx]
					= encoderTrueBinaryValueCurr;					
				SetEncoderTrueBinaryValueCurr
					(GetEncoderHistoryAverage());					
				break;
				
			default:
		
				break;						
		}
		
		encoderValHistIdx++;
}

////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////

#define ENCODER_TYPE_COUNT_IS_1024
//#define ENCODER_TYPE_COUNT_IS_256

#define ENCODER_TYPE_NORMAL
//#define ENCODER_TYPE_REVERSED

void    Init_EncoderHandler(void)
{
	#ifdef ENCODER_TYPE_COUNT_IS_1024
	SetEncoderTypeCount(ENCODER_TYPE_COUNT_1024);	
	SetEncoderMovingTolerance(ENCODER_TYPE_COUNT_1024_MOVING_TOL);
	#endif	

	#ifdef ENCODER_TYPE_COUNT_IS_256
	SetEncoderTypeCount(ENCODER_TYPE_COUNT_256);	
	SetEncoderMovingTolerance(ENCODER_TYPE_COUNT_256_MOVING_TOL);
	#endif
	
	#ifdef ENCODER_TYPE_NORMAL
	SetEncoderSamplingRate(ENCODER_HANDLER_SAMPLING_RATE);
	#endif

	#ifdef ENCODER_TYPE_NORMAL	
	ClearEncoderReversed();	
	#endif
	
	#ifdef ENCODER_TYPE_REVERSED
	SetEncoderReversed();
	#endif
			
	SetEncoderFilter(ENCODER_FILTER_2);
	
	encoderValHistIdx = 0;
}

////////////////////////////////////////////////////////////////////
//
// Exit Procedures
//
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//
// GoActive
//
////////////////////////////////////////////////////////////////////

NEW_STATE   EH_exitA(void)
{		
		StartTimer(MILLISECONDS(encoderSamplingRate));

    return EH_ACTIVE;
}


////////////////////////////////////////////////////////////////////
//
// TimeOut while in EH_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   EH_exitB(void)
{
	U16	inputShiftDataWordRegister = 0x0000;
	
		StartTimer(ENCODER_HANDLER_SAMPLING_RATE);
				
		// Get Data from SPI manager.
		
		inputShiftDataWordRegister = GetInputShiftRegister1() << 8;
		inputShiftDataWordRegister |= GetInputShiftRegister2();				
				
		SetEncoderRawGrayBitData(inputShiftDataWordRegister);	
			
		
		ConvertEncoderGrayDataToDecimal();
		FilterEncoderData();
			
		UpdatEncoderScaledValue();
		
		CheckEncoderIsAtMidPoint();
		
			if(IsEncoderAtMidPoint())
			{			
				SetDecimalPointOnes();
				SetDecimalPointTens();
			}		
			else
			{			
				ClearDecimalPointOnes();
				ClearDecimalPointTens();
			}		
		
		IsCheckEncoderValueChange();
		
		if(encoderValueChanged)
		{
			SendMessage(DisplaySystemManager, GoDisplayEncoderMode);			
		
			encoderValueChanged = FALSE;
		}
				
    return SAME_STATE;
}



////////////////////////////////////////////////////////////////////
//
// GoIdle while in EH_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   EH_exitC(void)
{
		CancelTimer();
	
	return EH_IDLE;
}

////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_EH_IDLE)
EV_HANDLER(GoActive, EH_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_EH_ACTIVE)
EV_HANDLER(TimeOut, EH_exitB),
EV_HANDLER(GoIdle, EH_exitC)
STATE_TRANSITION_MATRIX_END;

// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(EH_Main_Entry)
	STATE(_EH_IDLE)					,
	STATE(_EH_ACTIVE)       
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////


