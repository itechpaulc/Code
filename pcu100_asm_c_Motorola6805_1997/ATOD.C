


//
//	#include "atod.h"
//


/////////////////////////////////////////////////////////////////////////////
//
//
//	$Header:   N:/pvcs52/projects/pcu100~1/atod.c_v   1.4   Apr 25 1997 09:57:38   Paul L C  $
//	$Log:   N:/pvcs52/projects/pcu100~1/atod.c_v  $
//
//   Rev 1.4   Apr 25 1997 09:57:38   Paul L C
//Made changes to accomodate CPU port remapping.
//
//   Rev 1.3   Mar 25 1997 09:47:58   Paul L C
//Optimized filtering to use bit shifting instead of long divides.
//Added message processing for GoFastActive. This allow the AtoD
//machine to go from polling mode into Sample As Fast As Possible
//mode immediately. 
//
//   Rev 1.2   Mar 18 1997 08:29:16   Paul L C
//Union needed to specify BothBytes (bb). These errors were
//flagged by the new compiler.
//
//   Rev 1.1   Mar 04 1997 15:20:56   Paul L C
//Created 2 Filter types, one for fast reading when motor is active 
//and one for atod value refresh when motor is idle. Added a 
//settling state to make sure atod values are stable before any
//moves are executed. Made readingMSB a bit test, AtoDIsSettled
//a bit test.
//
//
//
//   Rev 1.0   Feb 26 1997 10:54:28   Paul L C
//Initial Revision
//
//
/////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////
//
// Private Members
//
//////////////////////////////////////////////////


union		WORDtype	instantAtoDvalue;


WORD		prevAtoDvalue;

BYTE		samplingRate,
			sampleCount;


#define		EnableAtoDchipSelect	(PORT_A.AD_CSEL_NOT = CLEAR)

#define		DisableAtoDchipSelect	(PORT_A.AD_CSEL_NOT = SET)


#define		SPIdoneIrqEnable		(SPCR.SPIE = TRUE)

#define		SPIdoneIrqDisable		(SPCR.SPIE = FALSE)


#define		SPIsystemEnable			(SPCR.SPE = TRUE)

#define		SPIsystemDisable		(SPCR.SPE = FALSE)




//////////////////////////////////////////////////
//
// Virtual Message Interface
//
//////////////////////////////////////////////////

void	SetAtoDsamplingRate(BYTE sRate) {

	samplingRate = sRate;

		if(sRate == FAST_SAMPLING_RATE)
			DO_FAST_FILTER;
		else
			DO_SLOW_FILTER;	// for :
							// SETTLE_SAMPLING_RATE or
							// NORMAL_SAMPLING_RATE
}



//////////////////////////////////////////////////
//
// Private Helper Functions
//
//////////////////////////////////////////////////

void	FilterReadings(void) {


		// currAtoDvalue = 93% prevAtoDvalue + 7% instantAtoDvalue.Word

		currAtoDvalue = (prevAtoDvalue * 15 + instantAtoDvalue.Word) >> 4;


		/*
		if(FILTER_FAST) {

			// currAtoDvalue = 50% prevAtoDvalue + 50% instantAtoDvalue.Word

			currAtoDvalue = (prevAtoDvalue + instantAtoDvalue.Word) >> 1;

		} else {

			// currAtoDvalue = 75% prevAtoDvalue + 25% instantAtoDvalue.Word

			currAtoDvalue = (prevAtoDvalue * 3 + instantAtoDvalue.Word) >> 2;
		}
        */

	instantAtoDvalue.Word = 0x0000;

	prevAtoDvalue = currAtoDvalue;
}


//////////////////////////////////////////////////
//
// Start the AtoD reading process
//
//////////////////////////////////////////////////

void	SampleAtoD(void)
{
		SET_READING_MSB();

		EnableAtoDchipSelect;

		SPDR = 0x00;				// Put dummy data to initiate
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

		sampleCount = 0;
		CLEAR_ATOD_SETTLED();

			SPIsystemEnable;

			// (2 Mhz / 16) = 125 Khz

			SPCR.MSTR = SET;
			SPCR.SPR1 = SET;
			SPCR.SPR0 = CLEAR;

		SampleAtoD();

	return ATDM_SETTLE;
}


//////////////////////////////////////////////////
//
// TimeOut Message, While in ACTIVE
//
//////////////////////////////////////////////////

BYTE	ATDM_exitB(void) {

		SampleAtoD();

	return SAME_STATE;	// ACTIVE
}


//////////////////////////////////////////////////
//
// SPI Data Ready Message, While in ACTIVE
//
//////////////////////////////////////////////////

BYTE	ATDM_exitC(void) {

	FilterReadings();

		if(samplingRate == NORMAL_SAMPLING_RATE)
			StartTimer(samplingRate);	// Sample in Time intervals
		else
			SampleAtoD();				// Sample as fast as possible

	return SAME_STATE;	// ACTIVE
}


//////////////////////////////////////////////////
//
// SPI Data Ready Message, While in SETTLE
//
//////////////////////////////////////////////////

BYTE	ATDM_exitD(void) {

	FilterReadings();

			sampleCount++;

			// Check if we have enough samples

			if(sampleCount != ATOD_SETTLE_SAMPLES) {

					SampleAtoD();

				return SAME_STATE;
			}

		if(samplingRate == NORMAL_SAMPLING_RATE)
			StartTimer(samplingRate);	// Sample in Time intervals
		else
			SampleAtoD();				// Sample as fast as possible

		SET_ATOD_SETTLED();

	return ATDM_ACTIVE;	// SETTLE DONE
}


//////////////////////////////////////////////////
//
//  GoAtoDfastActive Message, While in ACTIVE
//
//////////////////////////////////////////////////


BYTE	ATDM_exitE(void) {

		SetAtoDsamplingRate(FAST_SAMPLING_RATE);

		CancelTimer();

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
	fdb 	xATDM_SETTLE_MATRIX
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
xATDM_SETTLE_MATRIX:
	fcb		SPIdataReady
	fdb		ATDM_exitD
	fcb		NULL_MESSAGE
#endasm

#asm
xATDM_ACTIVE_MATRIX:
	fcb 	TimeOut
	fdb		ATDM_exitB
	fcb		SPIdataReady
	fdb		ATDM_exitC
	fcb		GoAtoDfastActive
	fdb		ATDM_exitE
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
		if(IS_READING_MSB())
		{
			CLEAR_READING_MSB();

			instantAtoDvalue.bb.HiByte = SPDR;

			instantAtoDvalue.Word >>= 1;

			SPDR = 0x00;	// Initiate LSB Transfer
		}
			else
		{
			instantAtoDvalue.bb.LowByte |= (SPDR >> 1);

			SPIdoneIrqDisable;

			SET_SYS_SPI_FLAG;

			DisableAtoDchipSelect;
		}
	}
}
