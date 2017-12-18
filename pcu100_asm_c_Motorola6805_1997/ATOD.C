


//
//	#include "atod.h"
//

//////////////////////////////////////////////////
//
// Private Members
//
//////////////////////////////////////////////////




WORD	instantAtoDvalue,
		prevAtoDvalue;

BOOL	ReadingMSB;


#define		EnableAtoDchipSelect	(PORT_B.AD_CSEL_NOT = CLEAR)

#define		DisableAtoDchipSelect	(PORT_B.AD_CSEL_NOT = SET)


#define		SPIdoneIrqEnable		(SPCR.SPIE = TRUE)

#define		SPIdoneIrqDisable		(SPCR.SPIE = FALSE)


#define		SPIsystemEnable			(SPCR.SPE = TRUE)

#define		SPIsystemDisable		(SPCR.SPE = FALSE)



//////////////////////////////////////////////////
//
// Virtual Message Interface
//
//////////////////////////////////////////////////




//////////////////////////////////////////////////
//
// Private Helper Function
//
//////////////////////////////////////////////////

void	FilterReadings(void) {

	WORD t1;

		t1 = (prevAtoDvalue * 4/5);

		currAtoDvalue = t1 + (instantAtoDvalue /5);
}



void	SampleAtoD(void)
{
			ReadingMSB = TRUE;

			EnableAtoDchipSelect;

			SPDR = 0x00;			// Put dummy data to initiate
									// transaction for MSB
			SPIdoneIrqEnable;
}



//////////////////////////////////////////////////
//
// State Machine Initialization
//
//	Linear Tech's AtoD maximum clock = 200 Khz
//
//	 This SPI is set @ 125 Khz
//
//////////////////////////////////////////////////

BYTE	Construct_ATODdriver(void) {

		SPCR = 0x00;

		DisableAtoDchipSelect;

		samplingRate = NORMAL_SAMPLING_RATE;

	return	ATDM_IDLE;
}


//////////////////////////////////////////////////
//
// GoActive Message, While in Idle
//
//////////////////////////////////////////////////

BYTE	ATDM_exitA(void) {

		SPIsystemEnable;

		// (2 Mhz / 16) = 125 Khz

		SPCR.MSTR = SET;
		SPCR.SPR1 = SET;
		SPCR.SPR0 = CLEAR;

		if(samplingRate == NORMAL_SAMPLING_RATE)
			StartTimer(samplingRate);
		else
			SampleAtoD();

	return ATDM_ACTIVE;
}


//////////////////////////////////////////////////
//
// TimeOut Message, While in ACTIVE
//
//////////////////////////////////////////////////

BYTE	ATDM_exitB(void) {

		SampleAtoD();

	return SAME_STATE;
}

//////////////////////////////////////////////////
//
// SPI Data Ready Message, While in ACTIVE
//
//////////////////////////////////////////////////

BYTE	ATDM_exitC(void) {

		FilterReadings();

		instantAtoDvalue = 0x0000;

		prevAtoDvalue = currAtoDvalue;

		if(samplingRate == NORMAL_SAMPLING_RATE)
			StartTimer(samplingRate);
		else
			SampleAtoD();

	return SAME_STATE;
}



//////////////////////////////////////////////////
//
// State Matrix Table
//
//////////////////////////////////////////////////

#asm
_ATODDRIVER:
	fdb 	xATDM_IDLE_MATRIX
	fdb		xATDM_ACTIVE_MATRIX
#endasm



//////////////////////////////////////////////////
//
// Message/Exit Function Matrix Table
//
//////////////////////////////////////////////////

#asm
xATDM_IDLE_MATRIX:
	fcb		AtoDGoActive
	fdb		ATDM_exitA
	fcb		NULL_MESSAGE
#endasm

#asm
xATDM_ACTIVE_MATRIX:
	fcb 	TimeOut
	fdb		ATDM_exitB
	fcb		SPIdataReady
	fdb		ATDM_exitC
	fcb		NULL_MESSAGE
#endasm


//////////////////////////////////////////////////
//
// SPI Interrupt Service Routine
//
//////////////////////////////////////////////////
//
// The first 3 clock cycles are used to setup
// the AtoD chip. The 4th cycle is the start
// of the Most Significant Bit (12 Bits total)
//
//////////////////////////////////////////////////

void	__SPI_ISR(void)
{
	if(SPSR.SPIF == TRUE)
	{
		if(ReadingMSB)
		{
			ReadingMSB = FALSE;

			instantAtoDvalue = SPDR;

			instantAtoDvalue <<= 7;

			SPDR = 0x00;	// Initiate LSB Transfer
		}
			else
		{
			instantAtoDvalue |= (SPDR >> 1);

			SPIdoneIrqDisable;

			SET_SYS_SPI_FLAG;

			DisableAtoDchipSelect;
		}
	}
}
