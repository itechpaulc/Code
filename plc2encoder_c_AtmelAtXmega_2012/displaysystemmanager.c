




//
//
//
//  Author :    Paul Calinawan
//
//  Date:       April 4 , 2011
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


////////////////////////////////////////////////////////////////////


#include <avr/io.h>

#include "spidevicemanager.h"

#include "displaysystemmanager.h"

#include "displayintensitycontroller.h"

#include "encoderhandler.h"

#include "linkbuttonmanager.h"

#include "systemmanager.h"



////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////

U8		displayData;

U8		displayDataModeHexOrDec;

U8		displayDecimalPointOnes;
U8		displayDecimalPointTens;

U8		displayTestMode;



///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////

//
//
//

void	SetDisplayData(U8 dispDat)
{
	displayData = dispDat;	
}

U8	GetDisplayData(void)
{
	return displayData;	
}

//
//
//

#define			DECIMAL_DISPLAY_MODE			(0x00)
#define			HEX_DISPLAY_MODE				(0x01)

void	SetDisplayDataModeDecimal(void)
{
	displayDataModeHexOrDec = DECIMAL_DISPLAY_MODE;	
}

void	SetDisplayDataModeHex(void)
{
	displayDataModeHexOrDec = HEX_DISPLAY_MODE;	
}

U8	IsDisplayDataDecimal(void)
{
		if(displayDataModeHexOrDec == DECIMAL_DISPLAY_MODE)
			return TRUE;
		
		return FALSE;	
}


//
// Decimal Point Handling Functions
//

void	SetDecimalPointOnes(void)
{
	displayDecimalPointOnes = TRUE;
}

void	ClearDecimalPointOnes(void)
{
	displayDecimalPointOnes = FALSE;
}

U8	IsSetDecimalPointOnes(void)
{
	return displayDecimalPointOnes;	
}

//

void	SetDecimalPointTens(void)
{
	displayDecimalPointTens = TRUE;
}

void	ClearDecimalPointTens(void)
{
	displayDecimalPointTens = FALSE;
}

U8	IsSetDecimalPointTens(void)
{
	return displayDecimalPointTens;	
}

//
//
//

void	SetDisplayTestMode(void)
{
	displayTestMode = TRUE;	
}

void	ClearDisplayTestMode(void)
{
	displayTestMode = FALSE;	
}

U8	IsDisplayTestMode(void)
{
		return displayTestMode;	
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////

void    Init_DisplaySystemManager(void)
{
		SetDisplayData(0x00);
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


NEW_STATE   DSM_exitA(void)
{
		SetDisplayData(0);
		
		SetDisplayDataModeDecimal();
	
		ClearDecimalPointOnes();
		ClearDecimalPointTens();
		
		ClearDisplayTestMode();
		
	return DSM_ACTIVE;
}


////////////////////////////////////////////////////////////////////
//
// GoDisplayTestMode
//		while in DSM_ACTIVE
//
////////////////////////////////////////////////////////////////////

U8	testDisplayData;

U8	intensityCycle;

U8	decimalPointToggle;

NEW_STATE   DSM_exitB(void)
{
		StartTimer(TEST_SEGMETNT_DELAY);
		
		SetDisplayDataModeDecimal();
						
		ClearDecimalPointOnes();
		ClearDecimalPointTens();
		
		testDisplayData = 0;
		intensityCycle = DISPLAY_LOW_INTENSITY;
		
		decimalPointToggle = TRUE;
		
		SetDisplayData(testDisplayData);
	
		SetDisplayTestMode();
		
	return DSM_TEST_MODE;
}


////////////////////////////////////////////////////////////////////
//
// Timeout
//		while in DSM_TEST_MODE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   DSM_exitC(void)
{
		StartTimer(TEST_SEGMETNT_DELAY);
	
		testDisplayData += 11;
		
		if(testDisplayData > 99)
		{
			testDisplayData = 0;	
			
			intensityCycle++;
				
			if(intensityCycle > DISPLAY_MAX_INTENSITY)
			{				
				intensityCycle = DISPLAY_LOW_INTENSITY;							
			}
			
			if(decimalPointToggle == TRUE)
			{
				decimalPointToggle = FALSE;
				
				ClearDecimalPointOnes();
				ClearDecimalPointTens();
				
				LinkLEDON();				
			}				
			else
			{
				decimalPointToggle = TRUE;
				
				SetDecimalPointOnes();
				SetDecimalPointTens();
				
				LinkLEDOFF();
			}
								
			SetDisplayIntensity(intensityCycle);
			
			//
			//
			
			SendMessage(CommLedManager, PulseRxLed);
			SendMessage(CommLedManager, FlashTxLed);
		}
							
		SetDisplayData(testDisplayData);
		SetDisplayTestMode();			
	
	return SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
// GoDisplayAddressMode
//		while in DSM_ACTIVE
//		or	in DSM_TEST_MODE
//		or	in DSM_DISPLAY_ADDRESS_MODE
//
////////////////////////////////////////////////////////////////////

U8	displayAddressDurationSteady;
U8	displayAddressDurationFlashing;

#define		DISPLAY_ADDRESS_DURATION_STEADY_INIT	(15)
#define		DISPLAY_ADDRESS_DURATION_FLASHING_INIT	(25)

NEW_STATE   DSM_exitD(void)
{
		StartTimer(DISPLAY_ADDRESS_RATE);	
				
		displayAddressDurationSteady = 
			DISPLAY_ADDRESS_DURATION_STEADY_INIT;
		displayAddressDurationFlashing = 
			DISPLAY_ADDRESS_DURATION_FLASHING_INIT;
		
		SetDisplayDataModeHex();
		
		ClearDecimalPointOnes();
		ClearDecimalPointTens();
				
		SetDisplayData(GetDeviceAddress());
	
		intensityCycle = DISPLAY_MAX_INTENSITY;
		SetDisplayIntensity(intensityCycle);
		
		ClearDisplayTestMode();
	
	return DSM_DISPLAY_ADDRESS_MODE;
}


////////////////////////////////////////////////////////////////////
//
// Timeout
//		while in DSM_DISPLAY_ADDRESS_MODE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   DSM_exitE(void)
{
		StartTimer(DISPLAY_ADDRESS_RATE);		
			
		--displayAddressDurationSteady;
		
		ClearDisplayTestMode();
		ClearDecimalPointOnes();
		ClearDecimalPointTens();
			
		if(displayAddressDurationSteady == 0)						
		{
			displayAddressDurationSteady = 1;			
				
			SetDisplayData(GetDeviceAddress());
				
			if(intensityCycle == DISPLAY_MAX_INTENSITY)
				intensityCycle = DISPLAY_MED_INTENSITY;
			else
				intensityCycle = DISPLAY_MAX_INTENSITY;
		
			SetDisplayIntensity(intensityCycle);
				
			--displayAddressDurationFlashing;
		
			if(displayAddressDurationFlashing == 0)
			{
				SendMessage(SystemManager, DisplayAddressDone);
			
				return DSM_ACTIVE;
			}
		}	
	
	return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
// GoDisplayAddressUpdateMode
//		while in DSM_ACTIVE
//		or in DSM_TEST_MODE
//
////////////////////////////////////////////////////////////////////
	
NEW_STATE   DSM_exitF(void)
{
		StartTimer(DISPLAY_ADDRESS_RATE);
		
		SetDisplayDataModeHex();
		
		ClearDecimalPointOnes();
		ClearDecimalPointTens();
				
		SetDisplayData(GetDeviceAddress());
	
		SetDisplayIntensity(DISPLAY_MAX_INTENSITY);
		
		ClearDisplayTestMode();
	
	return DSM_DISPLAY_ADDRESS_UPDATE_MODE;
}


////////////////////////////////////////////////////////////////////
//
// Timeout
//		while in DSM_DISPLAY_ADDRESS_MODE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   DSM_exitG(void)
{
		StartTimer(DISPLAY_ADDRESS_RATE);
		
						
		ClearDecimalPointOnes();
		ClearDecimalPointTens();
				
		SetDisplayData(GetDeviceAddress());
		
		ClearDisplayTestMode();
	
	return SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
//	GoDisplayEncoderMode
//	EncoderDataUpdated
//		while in DSM_ACTIVE
//
////////////////////////////////////////////////////////////////////

U8			intensityFade;
U8			displayEncoderDurationSteady;

#define		DISPLAY_ENCODER_DURATION_STEADY_INIT	(5)

NEW_STATE   DSM_exitH(void)
{
		StartTimer(DISPLAY_DECAY_ENCODER_RATE);
		
		displayEncoderDurationSteady =
			DISPLAY_ENCODER_DURATION_STEADY_INIT;
					
		SetDisplayData(GetEncoderScaledValue());
		
		intensityFade = DISPLAY_MAX_INTENSITY;
		SetDisplayIntensity(intensityFade);			
	
	return DSM_DISPLAY_ENCODER_MODE;
}

////////////////////////////////////////////////////////////////////
//
// Timeout
//		while in DSM_DISPLAY_ENCODER_MODE
//
////////////////////////////////////////////////////////////////////

#define		DISPLAY_ENCODER_MIN_INTENSITY		(2)

NEW_STATE   DSM_exitI(void)
{
		StartTimer(DISPLAY_DECAY_ENCODER_RATE);

		SetDisplayDataModeDecimal();
		ClearDisplayTestMode();
				
		SetDisplayData(GetEncoderScaledValue());
				
		--displayEncoderDurationSteady;
				
		if(displayEncoderDurationSteady == 0)
		{
			displayEncoderDurationSteady = 1;
			
			--intensityFade;
		
			if(intensityFade < DISPLAY_ENCODER_MIN_INTENSITY)
				intensityFade = DISPLAY_ENCODER_MIN_INTENSITY;
			
			SetDisplayIntensity(intensityFade);
		}		

	return SAME_STATE;
}




////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_DSM_IDLE)
EV_HANDLER(GoActive, DSM_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_DSM_ACTIVE)
EV_HANDLER(GoDisplayTestMode, DSM_exitB),
EV_HANDLER(GoDisplayAddressMode, DSM_exitD),
EV_HANDLER(GoDisplayEncoderMode, DSM_exitH)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_DSM_TEST_MODE)
EV_HANDLER(TimeOut, DSM_exitC),
EV_HANDLER(GoDisplayAddressMode, DSM_exitD),
EV_HANDLER(GoDisplayAddressUpdateMode, DSM_exitF)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_DSM_DISPLAY_ADDRESS_MODE)
EV_HANDLER(TimeOut, DSM_exitE),
EV_HANDLER(GoDisplayAddressMode, DSM_exitD),
EV_HANDLER(GoDisplayAddressUpdateMode, DSM_exitF)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_DSM_DISPLAY_ADDRESS_UPDATE_MODE)
EV_HANDLER(TimeOut, DSM_exitG),
EV_HANDLER(GoDisplayAddressMode, DSM_exitD),
EV_HANDLER(GoDisplayEncoderMode, DSM_exitH)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_DSM_DISPLAY_ENCODER_MODE)
EV_HANDLER(TimeOut, DSM_exitI),
EV_HANDLER(GoDisplayAddressMode, DSM_exitD),
EV_HANDLER(GoDisplayAddressUpdateMode, DSM_exitF),
EV_HANDLER(GoDisplayEncoderMode, DSM_exitH)
STATE_TRANSITION_MATRIX_END;

// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(DSM_Main_Entry)
		STATE(_DSM_IDLE),
		STATE(_DSM_ACTIVE),
		STATE(_DSM_TEST_MODE),
		STATE(_DSM_DISPLAY_ADDRESS_MODE),
		STATE(_DSM_DISPLAY_ADDRESS_UPDATE_MODE),
		STATE(_DSM_DISPLAY_ENCODER_MODE)
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////


