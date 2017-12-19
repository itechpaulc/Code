//----------------------------------------------------------------------------
// File : saver_cfg.c
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
// 1.00 06/24/13 Brandon W. Initial revision.
//----------------------------------------------------------------------------

// Standard includes
#include <stdint.h>
#include <string.h>

// RTOS includes
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

// Application includes
#include "board.h"
#include "saver.h"
#include "config.h"
#include "saverinterface.h"
#include "ticks.h"

//

#define SIGNAL_SAMPLES_PER_SEC_DFLT			ADC_SAMPLE_FREQ_1000
#define	SIGNAL_SAMPLES_PER_RECORD_DFLT		1000

#define TIMER_SAMPLES_PER_SEC_DFLT			SIGNAL_SAMPLES_PER_SEC_DFLT
#define	TIMER_SAMPLES_PER_RECORD_DFLT		0


//----------------------------------------------------------------------------
// SaverSetupCfgRead
//
// Read the Saver setup from non-volatile memory.
//
// In:
//   handle  : handle to Saver interface
//   timeout : max time to wait for settings to be read timer ticks
//
// Out:
//   setupCfg     : contains configuration read (or defaults if invalid)
//
// Returns:
//   pdTRUE if successful
//   pdFALSE on error
//----------------------------------------------------------------------------
static portBASE_TYPE SaverSetupCfgRead(
	SaverHandle		handle,
	SaverSetupCfg	*setupCfg,
	portTickType	timeout)
{
	SaverInterface	*saver	= (SaverInterface*) handle;
	SaverSetupCfg	cfgRead;
	portBASE_TYPE	success	= pdFALSE;

	if (saver && setupCfg)
	{
		// Read settings, use defaults if this fails
		if (SettingsRead(saver->Devices.Settings,
			FRAM_SETUP_CFG_OFFSET, (uint32_t)(&cfgRead),
			sizeof(SaverSetupCfg), pdTRUE, timeout))
		{
			// Setup is valid in addition to being read successfully
			if (setupCfg->AccelSignalSamplesPerSec != 0)
			{
				memcpy(&saver->SetupCfg, &cfgRead, sizeof(SaverSetupCfg));

				success = pdTRUE;
			}
		}

		// Load defaults if read failed
		if (success == pdFALSE)
		{
			SaverSetupCfgDefaults(handle, setupCfg);
		}
	}

	return success;
}

//----------------------------------------------------------------------------
// SaverSetupCfgWrite
//
// Write the Saver setup to non-volatile memory.
//
// In:
//   handle  	: handle to Saver interface
//   setupCfg   : contains the Saver setup to write
//   timeout 	: max time to wait for settings to be written timer ticks
//
// Out:
//   nothing
//
// Returns:
//   pdTRUE if successful
//   pdFALSE on error
//----------------------------------------------------------------------------
static portBASE_TYPE SaverSetupCfgWrite(
	SaverHandle		handle,
	SaverSetupCfg	*setupCfg,
	portTickType	timeout)
{
	portTickType	start	= xTaskGetTickCount();
	SaverInterface	*saver	= (SaverInterface*) handle;
	portBASE_TYPE	success	= pdFALSE;
	SaverSetupCfg		cfgOld;

	if (saver)
	{
		do
		{
			// Check existing settings
			if (SaverSetupCfgRead(handle, &cfgOld, timeout))
			{
				// Do nothing if stored settings already match
				if (memcmp(&cfgOld, setupCfg, sizeof(SaverSetupCfg)) == 0)
				{
					success = pdTRUE;
					break;
				}
			}

			// Save the setup (settings don't match or read failed)
			success = SettingsWrite(saver->Devices.Settings,
				FRAM_SETUP_CFG_OFFSET, (uint32_t)setupCfg, sizeof(SaverSetupCfg),
				pdTRUE, TicksRemaining(start, timeout));

		} while(0);
	}

	return success;
}

//----------------------------------------------------------------------------
// SaverSetupCfgInit
//
// Initialize the Saver configuration.
//
// In:
//   handle  : handle to Saver interface
//   timeout : max time to wait in ticks for settings to be read and
//             initialized
//
// Out:
//   nothing
//
// Returns:
//   pdTRUE if successful
//   pdFALSE on error
//----------------------------------------------------------------------------
portBASE_TYPE SaverSetupCfgInit(SaverHandle handle, portTickType timeout)
{
	SaverInterface	*saver	= (SaverInterface*) handle;
	portBASE_TYPE	success	= pdFALSE;

	if (saver)
	{
		// Acquire exclusive access to the Saver
		if (xSemaphoreTake(saver->Mutex, timeout) == pdPASS)
		{
			// Read setupCfg, but don't return false here on any error from the read
			// (the CRC may simply be invalid and defaults had to be used)
			SaverSetupCfgRead(handle, &saver->SetupCfg, timeout);

			// Release the semaphore
			xSemaphoreGive(saver->Mutex);

			success = pdTRUE;
		}
	}

	return success;
}

//----------------------------------------------------------------------------
// SaverSetupCfgGet
//
// Get a copy of the current system configuration.
//
// In:
//   handle  : handle to Saver interface
//   timeout : maximum time to wait for access to the configuration in ticks
//
// Out:
//   setupCfg     : if successful, contains the current system configuration
//
// Returns:
//   pdTRUE if successful
//   pdFALSE on error
//----------------------------------------------------------------------------
portBASE_TYPE SaverSetupCfgGet(
	SaverHandle		handle,
	SaverSetupCfg	*setupCfg,
	portTickType	timeout)
{
	SaverInterface	*saver	= (SaverInterface*) handle;
	portBASE_TYPE	success	= pdFALSE;

	// Return a copy of the current settings
	if (saver && setupCfg)
	{
		// Acquire exclusive access to the Saver
		if (xSemaphoreTake(saver->Mutex, timeout) == pdPASS)
		{
			memcpy(setupCfg, &saver->SetupCfg, sizeof(saver->SetupCfg));

			// Release the semaphore
			xSemaphoreGive(saver->Mutex);

			success = pdTRUE;
		}
	}

	return success;
}

//----------------------------------------------------------------------------
// SaverSetupCfgSet
//
// Sets the current Saver setup.
//
// In:
//   handle  	: handle to Saver interface
//   setupCfg   : contains the Saver setup to set
//   timeout 	: maximum time to wait for access to the current setup in ticks
//
// Out:
//   nothing
//
// Returns:
//   pdTRUE if successful
//   pdFALSE on error
//----------------------------------------------------------------------------
portBASE_TYPE SaverSetupCfgSet(
	SaverHandle		handle,
	SaverSetupCfg	*setupCfg,
	portTickType	timeout)
{
	SaverInterface	*saver	= (SaverInterface*) handle;
	portBASE_TYPE	success	= pdFALSE;

	if (saver && setupCfg)
	{
		// Acquire exclusive access to the Saver
		if (xSemaphoreTake(saver->Mutex, timeout) == pdPASS)
		{
			memcpy(&saver->SetupCfg, setupCfg, sizeof(saver->SetupCfg));

			// Release the semaphore
			xSemaphoreGive(saver->Mutex);

			success = pdTRUE;
		}
	}

	return success;
}

//----------------------------------------------------------------------------
// SaverSetupCfgDefaults
//
// Get a copy of the default Saver setup.
//
// In:
//   handle : handle to Saver interface
//
// Out:
//   setupCfg    : if successful, contains the default Saver setup
//
// Returns:
//   pdTRUE if successful
//   pdFALSE on error
//----------------------------------------------------------------------------
portBASE_TYPE SaverSetupCfgDefaults(SaverHandle handle, SaverSetupCfg *setupCfg)
{
	// Return the default setup
	if (handle && setupCfg)
	{
		setupCfg->AccelSignalSamplesPerSec		= SIGNAL_SAMPLES_PER_SEC_DFLT;
		setupCfg->AccelSignalSamplesPerRecord	= SIGNAL_SAMPLES_PER_RECORD_DFLT;

		setupCfg->AccelXSignalRecordEnabled	= pdTRUE;
		setupCfg->AccelYSignalRecordEnabled	= pdTRUE;
		setupCfg->AccelZSignalRecordEnabled	= pdTRUE;

		setupCfg->AccelSignalPreTrigCount = 250;
		setupCfg->AccelSignalPostTrigCount = 250;

		setupCfg->AccelExtTriggerEnabled = pdFALSE;

		setupCfg->SingalRecordRetentionMode = FILL_STOP;

		setupCfg->AccelSignalMaxRecordCount = 50;

		setupCfg->AccelXSignalDescription[0] = 'X';
		setupCfg->AccelYSignalDescription[0] = 'Y';
		setupCfg->AccelZSignalDescription[0] = 'Z';

		setupCfg->AccelXSignalFullScale = 200;
		setupCfg->AccelYSignalFullScale = 200;
		setupCfg->AccelZSignalFullScale = 200;

		setupCfg->AccelXSignalTriggerEnabled = pdTRUE;
		setupCfg->AccelYSignalTriggerEnabled = pdTRUE;
		setupCfg->AccelZSignalTriggerEnabled = pdTRUE;

		setupCfg->AccelXSignalTriggerLevel = 150;
		setupCfg->AccelYSignalTriggerLevel = 150;
		setupCfg->AccelZSignalTriggerLevel = 150;

		// TIMER Configuration, scheduled events

		setupCfg->AccelTimerWakeUpIntervalSeconds = 10;

		setupCfg->AccelTimerSamplesPerSec		= TIMER_SAMPLES_PER_SEC_DFLT;
		setupCfg->AccelTimerSamplesPerRecord	= TIMER_SAMPLES_PER_RECORD_DFLT;

		setupCfg->AccelXTimerRecordEnabled	= pdTRUE;
		setupCfg->AccelYTimerRecordEnabled	= pdTRUE;
		setupCfg->AccelZTimerRecordEnabled	= pdTRUE;

		setupCfg->TimerRecordRetentionMode = FILL_STOP;

		setupCfg->AccelTimerMaxRecordCount = 50;

		// LED / Alarms control flags

		setupCfg->AccelAlarmEnabled 	= pdTRUE;
		setupCfg->AccelAlarmThreshold 	= 2400;

		setupCfg->HeartBeatAlarmEnabled = pdTRUE;
		setupCfg->TempAlarmEnabled		= pdTRUE;
		setupCfg->HumidityAlarmEnabled	= pdTRUE;
		setupCfg->PressureAlarmEnabled	= pdTRUE;
		setupCfg->TiltAlarmEnabled 		= pdTRUE;

		//

		setupCfg->PushButtonEnabled = pdTRUE;

		// Communications

		setupCfg->CellPhoneEnabled 			= pdFALSE;
		setupCfg->CellPhoneNumber[0] 		= '5';
		setupCfg->CellPhoneSmsAlarmEnabled 	= pdFALSE;

		setupCfg->GpsEnabled  = pdFALSE;

		return pdTRUE;
	}
	else
	{
		return pdFALSE;
	}
}


//----------------------------------------------------------------------------
// SaverSetupCfgSave
//
// Save the current setup to non-volatile memory.
//
// In:
//   handle  : handle to Saver interface
//   timeout : max time to wait for settings to be saved timer ticks
//
// Out:
//   nothing
//
// Returns:
//   pdTRUE if successful
//   pdFALSE on error
//----------------------------------------------------------------------------
portBASE_TYPE SaverSetupCfgSave(SaverHandle handle, portTickType timeout)
{
	portTickType	start	= xTaskGetTickCount();
	SaverInterface	*saver	= (SaverInterface*) handle;
	portBASE_TYPE	success	= pdFALSE;

	if (saver)
	{
		// Acquire exclusive access to the Saver
		if (xSemaphoreTake(saver->Mutex, timeout) == pdPASS)
		{
			success = SaverSetupCfgWrite(handle, &saver->SetupCfg,
				TicksRemaining(start, timeout));

			// Release the semaphore
			xSemaphoreGive(saver->Mutex);
		}
	}

	return success;
}
