




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

#include <avr/interrupt.h>


#include "spidevicemanager.h"

#include "displaysystemmanager.h"

//
//
//

U8	outputShiftRegisterData1, outputShiftRegisterData2;
U8	inputShiftRegisterData1, inputShiftRegisterData2;


////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

#define DECIMAL_POINT_SEGMENT	(0x80)	

U8	ConvertToDisplaySegment(U8 dispData)
{
	switch(dispData)
	{
		case 0x0: return (0x3F);
		case 0x1: return (0x06);
		case 0x2: return (0x5B);
		case 0x3: return (0x4F);
		case 0x4: return (0x66);
		case 0x5: return (0x6D);
		case 0x6: return (0x7D);
		case 0x7: return (0x07);
		case 0x8: return (0x7F);
		case 0x9: return (0x6F);
		
		case 0xA: return (0x00);
		case 0xB: return (0x00);
		case 0xC: return (0x00);
		case 0xD: return (0x00);
		case 0xE: return (0x00);
		case 0xF: return (0x00);							
		
		default: return (0x00);
	}
}


////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

U8	spiLastByteRx;

U8	spiClearByReadSPSR;
U8	spiClearByReadSPDR;

ISR	(SPI_STC_vect)		
{
		// Clear Flag
		
		spiClearByReadSPSR = SPSR;
		spiClearByReadSPDR = SPDR;
		
		spiLastByteRx = spiClearByReadSPDR;

		SetKernelSpiTransferComplete();
}

void SPI_SendByte(U8 dataByteOutput)
{		
		spiClearByReadSPSR = SPSR;
		
		// Write Data Out
		
		SPDR = dataByteOutput;	
		
		//spiLastByteRx = SPDR;
}	

U8	SPI_GetLastByteRx(void)
{
	return spiLastByteRx;
}

//

U8	GetInputShiftRegister1()
{
	return inputShiftRegisterData1;
}

U8	GetInputShiftRegister2()
{
	return inputShiftRegisterData2;
}

//
//
//

void			SetEncoderLatch(void)
{
		PORTC |= ENCODER_LATCH_MASK;
}
			
void			ClearEncoderLatch(void)
{
		PORTC &= (~ENCODER_LATCH_MASK);
}

void			SetDisplayLatch(void)
{
		PORTC |= DISPLAY_LATCH_MASK;
}
			
void			ClearDisplayLatch(void)
{
		PORTC &= (~DISPLAY_LATCH_MASK);	
}
		

////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////

void    Init_SpiDeviceManager(void)
{
	// SPI Resets
	
	SPCR = 0x00;
	SPSR = 0x00;

	//
	// Direction Registers
	// MOSI and SCK are ALL OUTPUT,S others are inputs
	// Slave Select Pin SS as output
	//
	// DD_MOSI		= 0x01		- on Port B3
	// DD_SCK		= 0x01		- on Port B5
	// SS			= 0x01		- on Port B2
	//
	
	DDRB |= ((SET << DDB3) | (SET << DDB5));
	DDRB |= (SET << DDB2);
	
	//
	// Enable SPI and Interrupts
	// SPE = 0x01
	// SPIE = 0x01
	//
	// Data order LSBit / MCU is Master 
	// Clock Polarity normally LOW / Sample Data input on Trailing edge of clock
	// DORD		= 0x00
	// MSTR		= 0x01
	// CPOL		= 0x00
	// CPHA		= 0x01
	//
	// Set Clock Rate 8 Mhz / 128
	// 62.5 Kbits clock
	//
	// SPR0		= 0x01
	// SPR1		= 0x01
	//
		
	SPCR |= ((SET << SPE) | (SET << SPIE) | (SET << MSTR));
	SPCR |= ((SET << SPR0) | (SET << SPR1));
	
	//
	// Disable SPI Clock 2x multiplier
	// SPI2X  = 0x00
	// SPSR	 |= (CLEAR <<SPI2X);
	
	
	SetEncoderLatch();
}


		
								
////////////////////////////////////////////////////////////////////
//
// Exit Procedures
//
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//
// GoActive
//		While in SDM_IDLE
//
////////////////////////////////////////////////////////////////////


NEW_STATE   SDM_exitA(void)
{			
		SendMessage(THIS_MACHINE,SendDisplayData);

	return SDM_ACTIVE_WAIT_CMD;
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SDM_exitB(void)
{
			ClearEncoderLatch();
			
			StartTimer(DEVICE_LATCH_DELAY);
		
	return SDM_ACTIVE_CTRL_ENCODER_LATCH_DELAY_ON;
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SDM_exitC(void)
{
			SetEncoderLatch();
			
			StartTimer(DEVICE_LATCH_DELAY);
		
	return SDM_ACTIVE_CTRL_ENCODER_LATCH_DELAY_OFF;
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

U8	digitOnes;
U8	digitTens;
U8	dispData;

NEW_STATE   SDM_exitD(void)
{
	U8 datConv;
	
		dispData = GetDisplayData();
		
		digitOnes = dispData % 10;
		dispData /= 10;
			
		datConv = ConvertToDisplaySegment(digitOnes);
		
		if(IsSetDecimalPointOnes())
			datConv |= DECIMAL_POINT_SEGMENT;
		
		SPI_SendByte(datConv);
											
	return SDM_ACTIVE_UPDATE_REGISTER1;
}

////////////////////////////////////////////////////////////////////
//
//		SpiTransmitComplete
//		while in SDM_ACTIVE_UPDATE_REGISTER1
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SDM_exitE(void)
{
			StartTimer(SPI_INTERBYTE_DELAY);
			
			inputShiftRegisterData1 = (SPI_GetLastByteRx());
				
	return SDM_ACTIVE_UPDATE_DELAY1;
}

////////////////////////////////////////////////////////////////////
//
// TimeOut
// while in SDM_ACTIVE_UPDATE_DELAY1
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SDM_exitF(void)
{
		U8 datConv;
	
		digitTens = dispData % 10;
		
		if(!IsDisplayTestMode() && digitTens == 0)
		{
			datConv = 0x00; // BLANK Tens display
		}
		else
		{
			datConv = ConvertToDisplaySegment(digitTens);
		}		
		
		if(IsSetDecimalPointTens())
			datConv |= DECIMAL_POINT_SEGMENT;	
		
		SPI_SendByte(datConv);
								
	return SDM_ACTIVE_UPDATE_REGISTER2;
}

////////////////////////////////////////////////////////////////////
//
//		SpiTransmitComplete
//		while in SDM_ACTIVE_UPDATE_REGISTER2
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SDM_exitG(void)
{
			StartTimer(DEVICE_LATCH_DELAY);
				
			inputShiftRegisterData2 = (SPI_GetLastByteRx());
	
	return SDM_ACTIVE_UPDATE_DELAY2;
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SDM_exitH(void)
{
			SetDisplayLatch();
			
			StartTimer(DEVICE_LATCH_DELAY);
						
	return SDM_ACTIVE_CTRL_DISPLAY_LATCH_DELAY;
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SDM_exitI(void)
{
			ClearDisplayLatch();
			
			StartTimer(MILLISECONDS(100));
				
	return SDM_ACTIVE_NEXT_TEST;
}

NEW_STATE   SDM_exitJ(void)
{							
			SendMessage(THIS_MACHINE,SendDisplayData);			
			
	return SDM_ACTIVE_WAIT_CMD;
}


////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_SDM_IDLE)
EV_HANDLER(GoActive, SDM_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SDM_ACTIVE_WAIT_CMD)
EV_HANDLER(SendDisplayData, SDM_exitB),
EV_HANDLER(UpdatEncoderData, SDM_exitB)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SDM_ACTIVE_CTRL_ENCODER_LATCH_DELAY_ON)
EV_HANDLER(TimeOut, SDM_exitC)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SDM_ACTIVE_CTRL_ENCODER_LATCH_DELAY_OFF)
EV_HANDLER(TimeOut, SDM_exitD)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SDM_ACTIVE_UPDATE_REGISTER1)
EV_HANDLER(SpiTransmitComplete, SDM_exitE)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SDM_ACTIVE_UPDATE_DELAY1)
EV_HANDLER(TimeOut, SDM_exitF)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SDM_ACTIVE_UPDATE_REGISTER2)
EV_HANDLER(SpiTransmitComplete, SDM_exitG)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SDM_ACTIVE_UPDATE_DELAY2)
EV_HANDLER(TimeOut, SDM_exitH)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SDM_ACTIVE_CTRL_DISPLAY_LATCH_DELAY)
EV_HANDLER(TimeOut, SDM_exitI)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SDM_ACTIVE_NEXT_TEST)
EV_HANDLER(TimeOut, SDM_exitJ)
STATE_TRANSITION_MATRIX_END;

// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(SDM_Main_Entry)
	STATE(_SDM_IDLE),
	STATE(_SDM_ACTIVE_WAIT_CMD),
	STATE(_SDM_ACTIVE_CTRL_ENCODER_LATCH_DELAY_ON),
	STATE(_SDM_ACTIVE_CTRL_ENCODER_LATCH_DELAY_OFF),
	STATE(_SDM_ACTIVE_UPDATE_REGISTER1),
	STATE(_SDM_ACTIVE_UPDATE_DELAY1),			
	STATE(_SDM_ACTIVE_UPDATE_REGISTER2),			
	STATE(_SDM_ACTIVE_UPDATE_DELAY2),	
	STATE(_SDM_ACTIVE_CTRL_DISPLAY_LATCH_DELAY),
	STATE(_SDM_ACTIVE_NEXT_TEST)
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////


