//----------------------------------------------------------------------------
// File : saver_data.c
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
// 1.00 08/08/13 Brandon W. Initial revision.
//                          This is just testing and debug code to
//                          exercise the hardware and not anything
//                          close to the finished implementation.
//----------------------------------------------------------------------------

// Standard includes
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#ifdef GCC_MSP430
#include <legacymsp430.h>
#endif

// RTOS includes
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

// Application includes
#include "board.h"
#include "delay.h"
#include "ticks.h"
#include "saver.h"

#include "config.h"
#include "calibration.h"

#include "saver_dataacquire.h"

#include "saverstatemachine.h"


//
//

// temporarily reversed in Rev C

#define		ACCEL_SCAN_DATA_X_GET()			(ADC12MEM5)
#define		ACCEL_SCAN_DATA_Y_GET()			(ADC12MEM4)

//#define		ACCEL_SCAN_DATA_X_GET()			(ADC12MEM4)
//#define		ACCEL_SCAN_DATA_Y_GET()			(ADC12MEM5)

//

#define		ACCEL_SCAN_DATA_Z_GET()			(ADC12MEM6)

#define		LIGHT_SCAN_DATA_GET()			(ADC12MEM7)


// Makes the ADC reading signed
// Midpoint value at 0

#define		NORMALIZE_ADC_MIDPOINT(a)		(a - 0x800)

// Removes the sign bit, Complement 16 bits

#define		ABS_ADC_READING(a)				((int16_t)(a ^ 0xffff))


// Don't use DMA, it is more or less useless when reading the A/D
// at fixed intervals with a timer (because you can only read one
// sample sequence at a time and manually setup the next sequence).
// #define ADC_DMA						0


// Minimum sample time (timer output for SAMPCON is high this length)
//#define ADC_SAMPLE_TIMER			62500UL		// 62.5KHz is 16 microseconds
#define ADC_SAMPLE_TIMER			20000UL		// 20KHz is 50 microseconds

// Need to keep private global pointer to the Saver interface for ISRs to use
static SaverInterface *saverInstrumentData = NULL;

//
void 	ADCAccelSignalEnable(SaverHandle handle);
void 	ADCDacTestSignalEnable(SaverHandle handle);
void	ADCSampleFreqSet(SaverHandle handle, AdcSampleFreqType sampleFreqType);
void	ADCFilterFreqSet(SaverHandle handle, AdcFilterType filterFreqType);

static	void 	DACTestSignalStart(SaverHandle handle);
static	void 	DACTestSignalStop(SaverHandle handle);

//

#if defined(__MSP430_HAS_ADC12_PLUS__)
//----------------------------------------------------------------------------
// ADCISR
//
// ADC ISR for the A/D controller.
//
// In:
//   nothing
//
// Out:
//   nothing
//
// Returns:
//   nothing
//----------------------------------------------------------------------------

int16_t		lightData;
int			dacTestState=0;
int 		testCount;

#pragma vector=ADC12_VECTOR
interrupt void ADCISR(void)
{
	portBASE_TYPE	wakeHigherPriorityTask	= pdFALSE;

	SaverInterface	*saver		= saverInstrumentData;
	uint16_t		ringBufIndexNext;

	int16_t	accelDataX;
	int16_t	accelDataY;
	int16_t	accelDataZ;

	const Pin		pinTP1		= TP1_PIN;

#ifndef GCC_MSP430
	switch(__even_in_range(ADC12IV,34))
#else
	switch(ADC12IV)
#endif
	{
	case ADC12IV_NONE: break;			// No interrupt
	case ADC12IV_ADC12OVIFG: break;		// ADC overflow
	case ADC12IV_ADC12TOVIFG: break;	// ADC timing overflow
	case ADC12IV_ADC12IFG0: break;
	case ADC12IV_ADC12IFG1: break;
	case ADC12IV_ADC12IFG2: break;
	case ADC12IV_ADC12IFG3: break;
	case ADC12IV_ADC12IFG4: break;
	case ADC12IV_ADC12IFG5: break;
	case ADC12IV_ADC12IFG6: break;
	case ADC12IV_ADC12IFG7:

				if (saver)
				{
					PIOActivateFromISR(&pinTP1);

					// Toggle for next conversion
					ADC12CTL0 |= ADC12ENC;

					// Get next index into ring buffer
					ringBufIndexNext = saver->AccelScanData.RingBufIndex;

					// Note: X axis signal is inverted
					accelDataX = NORMALIZE_ADC_MIDPOINT
									((ACCEL_SCAN_DATA_X_GET() + saver->CalibrationCfg.AccelXOffset));

					accelDataY = NORMALIZE_ADC_MIDPOINT
									((ACCEL_SCAN_DATA_Y_GET() + saver->CalibrationCfg.AccelYOffset));


					accelDataZ = NORMALIZE_ADC_MIDPOINT
									((ACCEL_SCAN_DATA_Z_GET() + saver->CalibrationCfg.AccelZOffset));

					lightData = LIGHT_SCAN_DATA_GET();

					switch(saver->AccelScanCacheData.AscaState)
					{
						case ASC_IDLE:
							break;

						case ASC_PRE_TRIG_COUNT_WAIT:
							if(saver->AccelScanCacheData.AscaScanPreTriggerRemaining)
							{
								saver->AccelScanCacheData.AscaScanPreTriggerRemaining--;
							}
							else
							{
								saver->AccelScanCacheData.AscaState = ASC_SIGNAL_EVENT_DETECT;
							}
							break;

						case ASC_SIGNAL_EVENT_DETECT:
							if((ABS_ADC_READING(accelDataX) > (saver->SetupCfg.AccelXSignalTriggerLevel)) ||
									(ABS_ADC_READING(accelDataY) > (saver->SetupCfg.AccelYSignalTriggerLevel)) ||
									(ABS_ADC_READING(accelDataZ) > (saver->SetupCfg.AccelZSignalTriggerLevel)) )
							{
								saver->AccelScanCacheData.AscaScanPostTriggerRemaining = 50;
								saver->AccelScanCacheData.AscaState = ASC_POST_TRIG_COUNT_WAIT;
							}
							break;

						case ASC_POST_TRIG_COUNT_WAIT:
							if(saver->AccelScanCacheData.AscaScanPostTriggerRemaining)
							{
								saver->AccelScanCacheData.AscaScanPostTriggerRemaining--;
							}
							else
							{
								saver->AccelScanCacheData.AscaScanPreTriggerRemaining = 50;
								saver->AccelScanCacheData.AscaState = ASC_PRE_TRIG_COUNT_WAIT;
							}
							break;
					}


					// Get the A/D reading for each input

					saver->AccelScanData.Ring[ringBufIndexNext++] = accelDataX;
					saver->AccelScanData.Ring[ringBufIndexNext++] = accelDataY;
					saver->AccelScanData.Ring[ringBufIndexNext++] = accelDataZ;


					//saver->AccelScanData.Temperature = ADC12MEM8;


					//
					// Ring buffer management
					//

					// Handle wrap around

					if (ringBufIndexNext >= ADC_CHNL_SAMPLES_MAX_COUNT)
					{
						ringBufIndexNext = 0;
					}

					// Index for data copied to FRAM follows the ring buf index.
					// If the next ring buf equals the data buf index, it is
					// being overrun!  When this happens, increment a count
					// and overwrite the last samples just taken.

					if (ringBufIndexNext == saver->AccelScanData.BufIndex)
					{
						if(saver->AccelScanData.DataOverRun != UINT_MAX)
						{
							++saver->AccelScanData.DataOverRun;	// Overwrite data at current
						}

						// Notify data processing task there is still more data left
						xQueueSendFromISR(saver->TaskDataNotify, NULL, &wakeHigherPriorityTask);
					}
					else
					{
						saver->AccelScanData.RingBufIndex = ringBufIndexNext;
						saver->AccelScanData.DataRemaining--;

						// Data is only processed and written to FRAM in chunks.
						// Once the count remaining reaches 0, a chunk is ready.
						// Notify the data task so it process it.

						if (saver->AccelScanData.DataRemaining == 0)
						{
							saver->AccelScanData.DataRemaining =
									ADC_CHNL_SAMPLES_ACQUIRE_PER_NOTIFY;

							xQueueSendFromISR(saver->TaskDataNotify, NULL, &wakeHigherPriorityTask);
						}
					}

					PIODeactivateFromISR(&pinTP1);
				}

				break;

	case ADC12IV_ADC12IFG8: break;
	case ADC12IV_ADC12IFG9: break;
	case ADC12IV_ADC12IFG10: break;
	case ADC12IV_ADC12IFG11: break;
	case ADC12IV_ADC12IFG12: break;
	case ADC12IV_ADC12IFG13: break;
	case ADC12IV_ADC12IFG14: break;
	default: break;
	}

#ifndef GCC_MSP430
	// Ensure clocks are set as desired on exit from the ISR
	__bic_SR_register_on_exit(configMSP430_CLOCK_FLAGS_CLEAR);
	__bis_SR_register_on_exit(configMSP430_CLOCK_FLAGS_SET);
#endif

	// See if we woke up a task.  Calling portYIELD_FROM_ISR
	// will ensure the interrupt returns to this task if it
	// has a higher priority than the interrupted task.
	if (wakeHigherPriorityTask)
		portYIELD_FROM_ISR();	// must be last instruction in ISR
}
#endif	// defined(__MSP430_HAS_ADC12_PLUS__)

//----------------------------------------------------------------------------
// ADCTimerSetup
//
// Configure timer used to trigger sampling of A/D's.
//
// In:
//   handle    : handle to Saver interface
//   frequency : frequency to set for timer in Hz
//
// Out:
//   nothing
//
// Returns:
//   nothing
//----------------------------------------------------------------------------
static void ADCTimerSetup(SaverHandle handle, uint32_t frequency)
{
	uint32_t	count;
	uint16_t	clockDivider;
	uint32_t	clockFrequency;

		// Ensure the timer is stopped
		TB0CTL = 0;

		// Find the largest clock divider.  This finds the
		// slowest clock that will divide evenly by the
		// requested frequency with a count that is no
		// greater than 16-bits.
		clockFrequency = configSMCLK_FREQUENCY_HZ >> 3;
		count = clockFrequency / frequency;

		if (((count * frequency) != clockFrequency) &&
			(count <= 0x8000))
		{
			clockFrequency = configSMCLK_FREQUENCY_HZ >> 2;
			count = clockFrequency / frequency;
			if (((count * frequency) != clockFrequency) &&
				(count <= 0x8000))
			{
				clockFrequency = configSMCLK_FREQUENCY_HZ >> 1;
				count = clockFrequency / frequency;
				if (((count * frequency) != clockFrequency) &&
					(count <= 0x8000))
				{
					clockFrequency = configSMCLK_FREQUENCY_HZ;
					count = clockFrequency / frequency;
					clockDivider = ID__1;
				}
				else
				{
					clockDivider = ID__2;
				}
			}
			else
			{
				clockDivider = ID__4;
			}
		}
		else
		{
			clockDivider = ID__8;
		}

		// Run the timer from the SMCLK (divided by clockDivider), 16-bit counter
		TB0CTL = TBSSEL__SMCLK | clockDivider | CNTL__16;

		// Clear everything to start with
		TB0CTL |= TBCLR;

		// Set the compare match value according to the frequency we want
		// Timer B0 is using SMCLK. This configuration uses set/reset mode
		// for a PWM style signal with the output high until CCR1 is reached.
		TB0CCR0 = (uint16_t)(count - 1);
		TB0CCR1 = (uint16_t)(clockFrequency / ADC_SAMPLE_TIMER);

		// Set/reset mode
		TB0CCTL1 = OUTMOD_7;
}

//----------------------------------------------------------------------------
// ADCTimerStart
//
// Start timer used to trigger sampling of A/D's.
//
// In:
//   handle : handle to Saver interface
//
// Out:
//   nothing
//
// Returns:
//   nothing
//----------------------------------------------------------------------------
static void ADCTimerStart(SaverHandle handle)
{
	TB0CTL |= MC_1 | TBCLR;			// Up-Mode, start up clean
}

//----------------------------------------------------------------------------
// ADCTimerStop
//
// Stop timer used to trigger sampling of A/D's.
//
// In:
//   handle : handle to Saver interface
//
// Out:
//   nothing
//
// Returns:
//   nothing
//----------------------------------------------------------------------------
static void ADCTimerStop(SaverHandle handle)
{
	TB0CTL &= ~(MC_3);				// Halt timer
}

//
//
//

void ADCAccelSignalEnable(SaverHandle handle)
{
	SaverInterface	*saver	= (SaverInterface*) handle;

	PIODeactivate(&saver->DacTestSignalEnable);
}

//
//
//

void ADCDacTestSignalEnable(SaverHandle handle)
{
	SaverInterface	*saver	= (SaverInterface*) handle;

	PIOActivate(&saver->DacTestSignalEnable);
}

//
//
//

static void ADCAnalogSupplyEnable(SaverHandle handle)
{
	SaverInterface	*saver	= (SaverInterface*) handle;

	PIOActivate(&saver->AnalogSupplyEnable);
}

//
//
//

static void ADCAnalogSupplyDisaable(SaverHandle handle)
{
	SaverInterface	*saver	= (SaverInterface*) handle;

	PIODeactivate(&saver->AnalogSupplyEnable);
}

//

static void DACTestSignalStart(SaverHandle handle)
{
	DAC12_0CTL0 = (DAC12IR + DAC12AMP_5 + DAC12ENC);

	DAC12_0DAT = 0x0000;

	dacTestState = 1;
}

static void	DACTestSignalStop(SaverHandle handle)
{
	DAC12_0CTL0 = 0x0000;

	DAC12_0DAT = 0x0000;

	dacTestState = 0;
}


//----------------------------------------------------------------------------
// ADCDataSetup
//
// Setup DMA for samples read from the A/D's.
//
// In:
//   handle  : handle to Saver interface
//   timeout : maximum time in ticks to wait for A/D's and DMA to be setup
//
// Out:
//   nothing
//
// Returns:
//   pdTRUE if successful
//   pdFALSE otherwise
//----------------------------------------------------------------------------
static portBASE_TYPE ADCDataSetup(
	SaverHandle		handle,
	portTickType	timeout)
{
	portTickType	start	= xTaskGetTickCount();
	SaverInterface	*saver	= (SaverInterface*) handle;
	portBASE_TYPE	success	= pdFALSE;

	// Acquire exclusive access to the Saver
	if (xSemaphoreTake(saver->Mutex, timeout) == pdPASS)
	{
		do
		{
			// Select Accel as the signal source
			ADCAccelSignalEnable(saver);

			// Turn on the external reference voltage for the A/D inputs
			ADCAnalogSupplyEnable(saver);

			// Short delay to give Vref time to ramp up and level off
			vTaskDelay(VREF_ON_DELAY);

			/////////////////////////////////////////////////////////////////
			// TODO: This throws out any samples already in the ring buffer!
			//       We probably want to handle this better...
			/////////////////////////////////////////////////////////////////

			// A/D samples are written to a ring buffer using a DMA block
			// transfer once all of the channels in the sequence are ready.
			saver->AccelScanData.RingBufIndex	= 0;
			saver->AccelScanData.BufIndex		= 0;
			saver->AccelScanData.DataRemaining	= ADC_CHNL_SAMPLES_ACQUIRE_PER_NOTIFY;

			// The data task takes the read samples, applies filtering (if
			// needed), and stores the samples in non-volatile FRAM.  If
			// the data task fails to keep up, a sample overrun count/flag
			// is tracked.  Likewise, data in FRAM must be copied to flash
			// when a triggered/timed event occurs.  Watch for overruns of
			// events as well.  Both should never happen!  This is mostly
			// for debug to see if software is keeping up with higher
			// sample rates.
			saver->AccelScanData.DataOverRun	= 0;

			// ADC12MSC    : multiple sample conversion
			// ADC12SHT0_2 : MEM0-7  sample and hold time = 16 ADC12CLK cycles
			// ADC12SHT1_6 : MEM8-15 sample and hold time = 128 ADC12CLK cycles
			// ADC12ON     : enable A/D
			// Note: See REFCTL0 settings for internal reference controls
			ADC12CTL0 =	ADC12MSC | ADC12SHT0_2 | ADC12SHT1_6 | ADC12ON ; // |


			// ADC12SHP         : use sampling timer
			// ADC12SHS_3       : use timer B0
			// ADC12SSEL_3      : SMCLK clock source
			// ADC12DIV_1       : divide clock by 2
			// ADC12CONSEQ_1    : single sequence of channels
			// ADC12CSTARTADD_4 : start address (first channel in sequence)
			ADC12CTL1 = ADC12SHP | ADC12SHS_3 | ADC12SSEL_3 |
						ADC12DIV_1 | ADC12CONSEQ_1 | ADC12CSTARTADD_4;

			// ADC12RES_2       : 12-bit data
			// ADC12TCOFF       : temperature sensor off
			ADC12CTL2 = ADC12RES_2 | ADC12TCOFF;

			// ADC12INCH_x      : channel number in sequence,
			// ADC12INCH_5 		: for Light Sensor
			// ADC12SREF_1      : V(R+) = VREF+, V(R-) = AVSS // Internal REF
			// ADC12EOS         : last channel in sequence
			ADC12MCTL4 = ADC12INCH_0 | ADC12SREF_1;
			ADC12MCTL5 = ADC12INCH_1 | ADC12SREF_1;
			ADC12MCTL6 = ADC12INCH_2 | ADC12SREF_1;
			ADC12MCTL7 = ADC12INCH_5 | ADC12SREF_1 | ADC12EOS;

			//
			//
			//

			saver->AccelScanCacheData.RingBufIndexStart =
			saver->AccelScanCacheData.RingBufIndex 		=
			saver->AccelScanCacheData.BufIndex 			= UPPERMEM_430X_START_ADDR;

			saver->AccelScanCacheData.RingBufIndexEnd 	= UPPERMEM_430X_END_ADDR;

			saver->AccelScanCacheData.DataOverRun = 0;

			// Setup successful
			success = pdTRUE;

		} while (0);

		xSemaphoreGive(saver->Mutex);
	}

	return success;
}

//----------------------------------------------------------------------------
// ADCDataStart
//
// Enable data conversions for the A/D inputs.
//
// In:
//   handle : handle to Saver interface
//
// Out:
//   nothing
//
// Returns:
//   nothing
//----------------------------------------------------------------------------
static void ADCDataStart(SaverHandle handle)
{
	ADC12IFG	= 0;
	ADC12CTL0	|= ADC12ON;

	ADC12IE		= ADC12IE7;	// Last ADC12MCTLx in sample sequence

	ADC12CTL0	|= ADC12ENC;
}

//----------------------------------------------------------------------------
// ADCDataStop
//
// Disable data conversions for the A/D inputs.
//
// In:
//   handle : handle to Saver interface
//
// Out:
//   nothing
//
// Returns:
//   nothing
//----------------------------------------------------------------------------
static void ADCDataStop(SaverHandle handle)
{
	ADC12IE		= 0;

	ADC12CTL0	&= ~(ADC12ENC);
	ADC12CTL0	&= ~(ADC12ON);
}

//

void	AccelSettingProcessCommData(SaverInterface *saver, uint8_t *ladcStartAddr)
{
	AdcFilterType 		filtFreqType = (AdcFilterType)ladcStartAddr[0];
	AdcSampleFreqType 	sampleFreqType = (AdcSampleFreqType)ladcStartAddr[1];
	uint8_t				dacSignalEnable = (uint8_t)ladcStartAddr[2];

		ADCFilterFreqSet(saver,filtFreqType);
		ADCSampleFreqSet(saver, sampleFreqType);

		if (dacSignalEnable)
		{
				ADCDacTestSignalEnable(saver);
				DACTestSignalStart(saver);
		}
		else
		{
			 ADCAccelSignalEnable(saver);
			 DACTestSignalStop(saver);
		}

		//

		ADCTimerStart((SaverHandle)saver);
		ADCDataStart((SaverHandle)saver);
}

void	ADCSampleFreqSet(SaverHandle handle, AdcSampleFreqType sampleFreqType)
{
	SaverInterface	*saver		= (SaverInterface*) handle;

		switch(sampleFreqType)
		{
			case ADC_SAMPLE_FREQ_50_HZ:
				ADCTimerSetup(saver, ADC_SAMPLE_FREQ_50);
				break;
			case ADC_SAMPLE_FREQ_100_HZ:
				ADCTimerSetup(saver, ADC_SAMPLE_FREQ_100);
				break;
			case ADC_SAMPLE_FREQ_200_HZ:
				ADCTimerSetup(saver, ADC_SAMPLE_FREQ_200);
				break;
			case ADC_SAMPLE_FREQ_250_HZ:
				ADCTimerSetup(saver, ADC_SAMPLE_FREQ_250);
				break;
			case ADC_SAMPLE_FREQ_500_HZ:
				ADCTimerSetup(saver, ADC_SAMPLE_FREQ_500);
				break;
			case ADC_SAMPLE_FREQ_1000_HZ:
				ADCTimerSetup(saver, ADC_SAMPLE_FREQ_1000);
				break;
			case ADC_SAMPLE_FREQ_2500_HZ:
				ADCTimerSetup(saver, ADC_SAMPLE_FREQ_2500);
				break;
		}
}

void	ADCFilterFreqSet(SaverHandle handle, AdcFilterType filterFreqType)
{
	SaverInterface	*saver		= (SaverInterface*) handle;

		saver->SetupCfg.AdcFilterTypeSelect = filterFreqType;

		switch(filterFreqType)
		{
			case ADC_FILTER_50:
				PIODeactivate(&saver->AccelFilterControl[0]);
				PIODeactivate(&saver->AccelFilterControl[1]);
				break;
			case ADC_FILTER_100:
				PIOActivate(&saver->AccelFilterControl[0]);
				PIODeactivate(&saver->AccelFilterControl[1]);
				break;
			case ADC_FILTER_250:
				PIODeactivate(&saver->AccelFilterControl[0]);
				PIOActivate(&saver->AccelFilterControl[1]);
				break;
			case ADC_FILTER_300:
				PIOActivate(&saver->AccelFilterControl[0]);
				PIOActivate(&saver->AccelFilterControl[1]);
				break;
		}
}

static void 	ADCFilterInit(SaverHandle handle)
{
	SaverInterface	*saver		= (SaverInterface*) handle;

	ADCFilterFreqSet(saver, ADC_FILTER_300);
}

//----------------------------------------------------------------------------
// ProcessSaverData
//
// Disable data conversions for the A/D inputs.
//
// In:
//   handle : handle to Saver interface
//
// Out:
//   nothing
//
// Returns:
//   nothing
//----------------------------------------------------------------------------
static void ScanDataCacheRingWrite(SaverHandle handle)
{
	SaverInterface	*saver		= (SaverInterface*) handle;

	uint16_t		accelScanDataIndexNext;
	uint16_t		originalCopyLength, copyLength, dataRemaining;

	uint32_t		accelScanDataCacheEndCopyAddr;
	uint32_t		accelScanDataCacheNextCopyAddr;

		originalCopyLength 	=
			dataRemaining 	= (ADC_CHNL_SAMPLES_ACQUIRE_PER_NOTIFY << 1);

		do
		{
			accelScanDataCacheEndCopyAddr = saver->AccelScanCacheData.RingBufIndex + dataRemaining;

			if(accelScanDataCacheEndCopyAddr >= saver->AccelScanCacheData.RingBufIndexEnd)
			{
				// Handle wrap around, WORD aligned,
				// also leave 1 WORD unused for easier debug viewing of memory boundaries

				copyLength = (saver->AccelScanCacheData.RingBufIndexEnd -
								saver->AccelScanCacheData.RingBufIndex) - 1;

				dataRemaining = originalCopyLength - copyLength;

				accelScanDataCacheNextCopyAddr = saver->AccelScanCacheData.RingBufIndexStart;
			}
			else
			{
				accelScanDataCacheNextCopyAddr = accelScanDataCacheEndCopyAddr;

				copyLength = dataRemaining;
				dataRemaining = 0;
			}

			//

			MSP430XMemCopy((saver->AccelScanCacheData.RingBufIndex),
							(uint32_t)(&saver->AccelScanData.Ring[saver->AccelScanData.BufIndex]),
							copyLength);

			// TODO: Convert regular memcopy to DMA version
			// DMA version need to be debugged
			//
			//				DMAMemCopy(saver->Devices.DMAMemCopy,
			//							0xFBF00,
			//							(uint32_t)(&saver->Data.Ring[saver->Data.DataBufIndex]),
			//							((ADC_CHNL_SAMPLES_PER_PAGE * ADC_CHNL_COUNT) << 1), 100);

			// Update Accel Scan Data CACHE Indexes

			saver->AccelScanCacheData.RingBufIndex = accelScanDataCacheNextCopyAddr;

		} while(dataRemaining > 0);


		// Update Accel Scan Data Indexes

		taskENTER_CRITICAL();
		accelScanDataIndexNext = saver->AccelScanData.BufIndex;
		taskEXIT_CRITICAL();

		accelScanDataIndexNext += ADC_CHNL_SAMPLES_ACQUIRE_PER_NOTIFY;

		if (accelScanDataIndexNext >= ADC_CHNL_SAMPLES_MAX_COUNT)
		{
			taskENTER_CRITICAL();
			saver->AccelScanData.BufIndex = 0;
			taskEXIT_CRITICAL();
		}
		else
		{
			taskENTER_CRITICAL();
			saver->AccelScanData.BufIndex = accelScanDataIndexNext;
			taskEXIT_CRITICAL();
		}
}


//----------------------------------------------------------------------------
// SignalEventDetectProcess
//
// Disable data conversions for the A/D inputs.
//
// In:
//   handle : handle to Saver interface
//
// Out:
//   nothing
//
// Returns:
//   nothing
//----------------------------------------------------------------------------
static void SignalEventDetectProcess(SaverHandle handle)
{
	SaverInterface	*saver		= (SaverInterface*) handle;

	switch(saver->AccelScanCacheData.AscaState)
	{
		case ASC_IDLE:
			saver->AccelScanCacheData.AscaState = ASC_PRE_TRIG_COUNT_WAIT;
			saver->AccelScanCacheData.AscaScanPreTriggerRemaining = 200;
			break;

		case ASC_PRE_TRIG_COUNT_WAIT:
			break;

		case ASC_SIGNAL_EVENT_DETECT:

			break;

		case ASC_POST_TRIG_COUNT_WAIT:
			//saver->AccelScanCacheData.AscaState = ASC_PRE_TRIG_COUNT_WAIT;
			//saver->AccelScanCacheData.AscaScanPreTriggerRemaining = 0;

			break;
	}
}

//----------------------------------------------------------------------------
// ProcessSaverData
//
// Disable data conversions for the A/D inputs.
//
// In:
//   handle : handle to Saver interface
//
// Out:
//   nothing
//
// Returns:
//   nothing
//----------------------------------------------------------------------------
static void SaverDataProcess(SaverHandle handle)
{
		ScanDataCacheRingWrite(handle);

		SignalEventDetectProcess(handle);
}

//----------------------------------------------------------------------------
// SaverDataAcquireTask
//
// This task sleeps indefinitely until it receives notification there
// is A/D data in RAM that needs to be copied and stored in FRAM.  This
// function should not be called directly.  One instance of it should
// be created with the xTaskCreate function.
//
// TODO: this routine will likely also be the place where filtering would
//       be applied and event triggers would be detected and raised
//
// In:
//   parameters : pointer to Saver interface structure (must not be NULL)
//
// Out:
//   nothing
//
// Returns:
//   nothing
//----------------------------------------------------------------------------

uint8_t	 		testStr1[10];
uint8_t	 		testStr2[10];

uint8_t			testFram1	= 0xAA;
uint8_t			testFram2	= 0x55;

float	testPress;
float	testTemp;
float	testTemp2;
float	testHumidity;

uint8_t			testsuccess;

portTASK_FUNCTION(SaverDataAcquireTask, parameters)
{
	SaverInterface	*saver		= (SaverInterface*) parameters;

	// Save private global pointer to Saver interface (needed for ISRs)
	saverInstrumentData = saver;

	saver->AccelScanCacheData.AscaState = ASC_IDLE;

	//FRAM25XSleep(saver->Devices.FRAM, FRAM_WRITE_TIMEOUT);

	FRAM25XWrite(saver->Devices.FRAM, 0x00000000, (uint32_t)&testFram1, 1, FRAM_WRITE_TIMEOUT * 20);
	testFram1 = 0x00;
	FRAM25XRead(saver->Devices.FRAM, 0x00000000, (uint32_t)&testFram1, 1, FRAM_WRITE_TIMEOUT * 20);

	FRAM25XWrite(saver->Devices.FRAM, 0x0003FFFF, (uint32_t)&testFram2, 1, FRAM_WRITE_TIMEOUT * 20);
	testFram2 = 0x00;
	FRAM25XRead(saver->Devices.FRAM, 0x0003FFFF, (uint32_t)&testFram2, 1, FRAM_WRITE_TIMEOUT * 20);



	//testsuccess = FlashMX25EraseSector(saver->Devices.Flash, 0x0000FFFF, pdFALSE, FLASH_CMD_TIMEOUT);

	//testsuccess = FlashMX25EraseBlock32(saver->Devices.Flash, 0x00000000, pdFALSE, FLASH_CMD_TIMEOUT);

	//testsuccess = FlashMX25EraseBlock64(saver->Devices.Flash, 0x00000000, pdFALSE, FLASH_CMD_TIMEOUT);

	testsuccess = FlashMX25EraseChip(saver->Devices.FLASH, pdFALSE, FLASH_CMD_TIMEOUT);

	testsuccess = FlashMX25PageWrite(saver->Devices.FLASH, 0x00000000, (uint32_t)"LANSMONT", 10, pdTRUE, FLASH_WRITE_TIMEOUT);

	testsuccess = FlashMX25Read(saver->Devices.FLASH, 0x00000000, (uint32_t)&testStr1[0], 10, FLASH_READ_TIMEOUT);

	testsuccess = FlashMX25PageWrite(saver->Devices.FLASH, 0x01E84800, (uint32_t)"MONTEREY", 10, pdTRUE, FLASH_WRITE_TIMEOUT);

	testsuccess = FlashMX25Read(saver->Devices.FLASH, 0x01E84800, (uint32_t)&testStr2[0], 10, FLASH_READ_TIMEOUT);


	testsuccess =
		PressureTempRead(saver->Devices.PressureTemp, &testPress, &testTemp, 10000);

	testsuccess =
		SHTHumidityRead(saver->Devices.Humidity, &testHumidity, 1000);

	testsuccess =
		SHTTempRead(saver->Devices.Humidity, &testTemp2, 1000);


	//
	// Todo: Move to Manager

	SaverCalibrationCfgInit((SaverHandle)saver, SAVER_TIMEOUT);

	SaverSetupCfgInit((SaverHandle)saver, SAVER_TIMEOUT);



	// TODO: no longer writing to FRAM
	saver->AccelScanData.DataOffset = FRAM_DATA_OFFSET;

	ADCDataSetup((SaverHandle)saver, portMAX_DELAY);

	ADCTimerSetup((SaverHandle)saver, saver->SetupCfg.AccelSignalSamplesPerSec);
	
	ADCTimerStart((SaverHandle)saver);

	ADCDataStart((SaverHandle)saver);

	//

	ADCFilterInit((SaverHandle)saver);

	// Blocked (DelayUS) and non-blocked (vTaskDelay) 2ms are fine at
	// 5000 A/D samples per second.  This task can still keep up and
	// have no overruns in the ring buffer.  This means there is at
	// least this much processing time available here for triggering,
	// filtering, etc.

		for (;;)
		{
			// Wait for notification of data ready
			if (xQueueReceive(saver->TaskDataNotify, NULL, portMAX_DELAY) == pdPASS)
			{
				switch(saver->TaskDataAcquireState)
				{

				default:
					SaverDataProcess((SaverHandle)saver);
					break;
				}
			}
		}
}


// Todo DAC TEST, was in IRQ section
//
//	if(dacTestState == 1)
//	{
//		if(testCount++ > 10)
//		{
//			DAC12_0DAT = 4000;
//
//			dacTestState = 2;
//			testCount = 0;
//		}
//	}
//	else
//	if(dacTestState == 2)
//	{
//		if(testCount++ > 10)
//		{
//			DAC12_0DAT = 0;
//
//			dacTestState = 1;
//			testCount = 1;
//		}
//	}




