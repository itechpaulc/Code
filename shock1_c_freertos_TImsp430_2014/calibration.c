//----------------------------------------------------------------------------
// File : calibration.c
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
#include "calibration.h"
#include "saverinterface.h"
#include "ticks.h"

#include "config.h"

//

//----------------------------------------------------------------------------
// SaverCalibrationCfgRead
//
// Read the Saver Calibration from non-volatile memory.
//
// In:
//   handle  : handle to Saver interface
//   timeout : max time to wait for settings to be read timer ticks
//
// Out:
//   calCfg     : contains configuration read (or defaults if invalid)
//
// Returns:
//   pdTRUE if successful
//   pdFALSE on error
//----------------------------------------------------------------------------
static portBASE_TYPE SaverCalibrationCfgRead(
	SaverHandle			handle,
	SaverCalibrationCfg	*calCfg,
	portTickType		timeout)
{
	SaverInterface		*saver	= (SaverInterface*) handle;
	SaverCalibrationCfg	cfgRead;
	portBASE_TYPE		success	= pdFALSE;

	if (saver && calCfg)
	{
		// Read settings, use defaults if this fails
		if (SettingsRead(saver->Devices.Settings,
			FRAM_CALIB_CFG_OFFSET, (uint32_t)(&cfgRead),
			sizeof(SaverCalibrationCfg), pdTRUE, timeout))
		{
			// Setup is valid in addition to being read successfully
			// TODO:
			if (1)
			{
				success = pdTRUE;
				memcpy(&saver->CalibrationCfg, &cfgRead, sizeof(SaverCalibrationCfg));
			}
		}

		// Load defaults if read failed
		if (success == pdFALSE)
		{
			SaverCalibrationCfgDefaults(handle, calCfg);
		}
	}

	return success;
}

//----------------------------------------------------------------------------
// SaverCalibrationCfgWrite
//
// Write the Saver Calibration to non-volatile memory.
//
// In:
//   handle  	: handle to Saver interface
//   calCfg   	: contains the Saver Calibration to write
//   timeout 	: max time to wait for settings to be written timer ticks
//
// Out:
//   nothing
//
// Returns:
//   pdTRUE if successful
//   pdFALSE on error
//----------------------------------------------------------------------------
static portBASE_TYPE SaverCalibrationCfgWrite(
	SaverHandle			handle,
	SaverCalibrationCfg	*calCfg,
	portTickType		timeout)
{
	portTickType		start	= xTaskGetTickCount();
	SaverInterface		*saver	= (SaverInterface*) handle;
	portBASE_TYPE		success	= pdFALSE;
	SaverCalibrationCfg	cfgOld;

	if (saver)
	{
		do
		{
			// Check existing settings
			if (SaverCalibrationCfgRead(handle, &cfgOld, timeout))
			{
				// Do nothing if stored settings already match
				if (memcmp(&cfgOld, calCfg, sizeof(SaverCalibrationCfg)) == 0)
				{
					success = pdTRUE;
					break;
				}
			}

			// Save the Calibration (settings don't match or read failed)
			success = SettingsWrite(saver->Devices.Settings,
				FRAM_CALIB_CFG_OFFSET, (uint32_t)calCfg, sizeof(SaverCalibrationCfg),
				pdTRUE, TicksRemaining(start, timeout));

		} while(0);
	}

	return success;
}

//----------------------------------------------------------------------------
// SaverCalibrationCfgInit
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
portBASE_TYPE SaverCalibrationCfgInit(SaverHandle handle, portTickType timeout)
{
	SaverInterface	*saver	= (SaverInterface*) handle;
	portBASE_TYPE	success	= pdFALSE;

	if (saver)
	{
		// Acquire exclusive access to the Saver
		if (xSemaphoreTake(saver->Mutex, timeout) == pdPASS)
		{
			// Read calCfg, but don't return false here on any error from the read
			// (the CRC may simply be invalid and defaults had to be used)
			SaverCalibrationCfgRead(handle, &saver->CalibrationCfg, timeout);

			// Release the semaphore
			xSemaphoreGive(saver->Mutex);

			success = pdTRUE;
		}
	}

	return success;
}

//----------------------------------------------------------------------------
// SaverCalibrationCfgGet
//
// Get a copy of the current system configuration.
//
// In:
//   handle  : handle to Saver interface
//   timeout : maximum time to wait for access to the configuration in ticks
//
// Out:
//   calCfg     : if successful, contains the current system configuration
//
// Returns:
//   pdTRUE if successful
//   pdFALSE on error
//----------------------------------------------------------------------------
portBASE_TYPE SaverCalibrationCfgGet(
	SaverHandle			handle,
	SaverCalibrationCfg	*calCfg,
	portTickType		timeout)
{
	SaverInterface	*saver	= (SaverInterface*) handle;
	portBASE_TYPE	success	= pdFALSE;

	// Return a copy of the current settings
	if (saver && calCfg)
	{
		// Acquire exclusive access to the Saver
		if (xSemaphoreTake(saver->Mutex, timeout) == pdPASS)
		{
			memcpy(calCfg, &saver->CalibrationCfg, sizeof(saver->CalibrationCfg));

			// Release the semaphore
			xSemaphoreGive(saver->Mutex);

			success = pdTRUE;
		}
	}

	return success;
}

//----------------------------------------------------------------------------
// SaverCalibrationCfgSet
//
// Sets the current Saver Calibration.
//
// In:
//   handle  	: handle to Saver interface
//   calCfg   	: contains the Saver Calibration to set
//   timeout 	: maximum time to wait for access to the current setup in ticks
//
// Out:
//   nothing
//
// Returns:
//   pdTRUE if successful
//   pdFALSE on error
//----------------------------------------------------------------------------
portBASE_TYPE SaverCalibrationCfgSet(
	SaverHandle			handle,
	SaverCalibrationCfg	*calCfg,
	portTickType		timeout)
{
	SaverInterface	*saver	= (SaverInterface*) handle;
	portBASE_TYPE	success	= pdFALSE;

	if (saver && calCfg)
	{
		// Acquire exclusive access to the Saver
		if (xSemaphoreTake(saver->Mutex, timeout) == pdPASS)
		{
			memcpy(&saver->CalibrationCfg, calCfg, sizeof(saver->CalibrationCfg));

			// Release the semaphore
			xSemaphoreGive(saver->Mutex);

			success = pdTRUE;
		}
	}

	return success;
}

//----------------------------------------------------------------------------
// SaverCalibrationCfgDefaults
//
// Get a copy of the default Calibration Saver Calibration.
//
// In:
//   handle : handle to Saver interface
//
// Out:
//   calCfg	: if successful, contains the default Saver Calibration
//
// Returns:
//   pdTRUE if successful
//   pdFALSE on error
//----------------------------------------------------------------------------
portBASE_TYPE SaverCalibrationCfgDefaults(SaverHandle handle, SaverCalibrationCfg *calCfg)
{
	// Return the default calibration
	if (handle && calCfg)
	{
//		calCfg->AccelXOffset 	= (uint16_t)(+8);
//		calCfg->AccelYOffset 	= (uint16_t)(-4);
//		calCfg->AccelZOffset 	= (uint16_t)(+24);

		calCfg->AccelXOffset 	= 0x0000;
		calCfg->AccelYOffset 	= 0x0000;
		calCfg->AccelZOffset 	= 0x0000;


		calCfg->AccelXSlope 	= 1.0f;
		calCfg->AccelYSlope 	= 1.0f;
		calCfg->AccelZSlope 	= 1.0f;

		calCfg->IsCalibrated 	= pdFALSE;
		calCfg->CalDateTime 	= 0;

		return pdTRUE;
	}
	else
	{
		return pdFALSE;
	}
}

//----------------------------------------------------------------------------
// SaverCalibrationCfgSave
//
// Save the current Calibration to non-volatile memory.
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
portBASE_TYPE SaverCalibrationCfgSave(SaverHandle handle, portTickType timeout)
{
	portTickType	start	= xTaskGetTickCount();
	SaverInterface	*saver	= (SaverInterface*) handle;
	portBASE_TYPE	success	= pdFALSE;

	if (saver)
	{
		// Acquire exclusive access to the Saver
		if (xSemaphoreTake(saver->Mutex, timeout) == pdPASS)
		{
			success = SaverCalibrationCfgWrite(handle, &saver->CalibrationCfg,
				TicksRemaining(start, timeout));

			// Release the semaphore
			xSemaphoreGive(saver->Mutex);
		}
	}

	return success;
}
