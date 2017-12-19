//----------------------------------------------------------------------------
// File : settings.c
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
// 1.01 08/08/13 Brandon W. CRC checked read/write routines have been
//                          removed from the F-RAM driver. CRCs are now
//                          handled at this level.
//                          The last two bytes of all settings data passed
//                          to this interface are expected to be reserved
//                          for the CRC.
// 1.00 06/24/13 Brandon W. Initial revision.
//                          These routines provide an interface for
//                          accessing settings in non-volatile F-RAM.
//----------------------------------------------------------------------------

// Standard includes
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// RTOS includes
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

// Application includes
#include "crc.h"
#include "fram_fm25x.h"
#include "settings.h"
#include "ticks.h"

typedef struct
{
	FRAM25XHandle		FRAM;
	CRCHandle			CRC;
	xSemaphoreHandle	Mutex;
} SettingsInterface;

//----------------------------------------------------------------------------
// SettingsCheckedRead
//
// Read settings stored in non-volatile memory.  The data read is expected
// to contain a 16-bit CRC in the last two bytes which is calculated for
// all bytes preceding it.  If the CRC is valid, success is returned.
//
// NOTE: on any an error or timeout, the previous contents of the
//       provided data buffer may be destroyed
//
// In:
//   handle  : handle to interface for reading and writing settings
//   offset  : offset in non-volatile memory where the settings are stored
//   length  : length of the data to read (including CRC)
//   timeout : maximum time in ticks to wait for read to complete
//
// Out:
//   data    : contains the settings read from non-volatile memory
//
// Returns:
//   pdTRUE if successful
//   pdFALSE on error
//----------------------------------------------------------------------------
static portBASE_TYPE SettingsCheckedRead(
	SettingsHandle		handle,
	uint32_t			offset,
	uint32_t			dataAddress,
	uint32_t			length,
	portTickType		timeout)
{
	portTickType		start		= xTaskGetTickCount();
	SettingsInterface	*settings	= (SettingsInterface*)handle;
	portBASE_TYPE		success		= pdFALSE;
	uint16_t			crc			= ~0;

	// Check parameters
	if (settings && dataAddress && (length > sizeof(crc)))
	{
		// Read settings and CRC from non-volatile memory
		if (FRAM25XRead(settings->FRAM, offset, dataAddress, length, timeout))
		{
			// Todo fix pointer types
			// Compute CRC and compare with CRC from the data read
//			if (CRCCalc(settings->CRC, dataAddress, length - sizeof(crc), &crc,
//				TicksRemaining(start, timeout)))
//			{
//				if (crc == *((uint16_t*)(&dataAddress[length - sizeof(crc)])))
//				{
//					success = pdTRUE;
//				}
//			}
		}
	}

	return success;
}

//----------------------------------------------------------------------------
// SettingsCheckedWrite
//
// Write settings into non-volatile memory.  A 16-bit CRC is expected to
// be stored in the last two bytes.  The CRC is re-calculated and updated
// before the data is saved.
//
// In:
//   handle  : handle to interface for reading and writing settings
//   offset  : offset in non-volatile memory where the settings are stored
//   length  : length of the data to read (including CRC)
//   timeout : maximum time in ticks to wait for write to complete
//
// Out:
//   data    : the 16-bit CRC for all bytes preceding the CRC itself is
//             recalculated and updated in the last two bytes of this buffer
//
// Returns:
//   pdTRUE if successful
//   pdFALSE on error
//----------------------------------------------------------------------------
static portBASE_TYPE SettingsCheckedWrite(
	SettingsHandle	handle,
	uint32_t		offset,
	uint32_t		dataAddress,
	uint32_t		length,
	portTickType	timeout)
{
	portTickType		start		= xTaskGetTickCount();
	SettingsInterface	*settings	= (SettingsInterface*)handle;
	uint16_t			crc			= ~0;

	// Check parameters
	if (settings && dataAddress && (length > sizeof(crc)))
	{
		// Todo fix pointer types
		// Compute CRC and set it at the end of the data
//		if (CRCCalc(settings->CRC, dataAddress, length - sizeof(crc), &crc, timeout))
//		{
//			*((uint16_t*)(&dataAddress[length - sizeof(crc)])) = crc;
//
//			// Write settings and CRC into non-volatile memory and return result
//			return FRAM25XWrite(settings->FRAM, offset, dataAddress, length,
//				TicksRemaining(start, timeout));
//		}
	}

	return pdFALSE;
}

//----------------------------------------------------------------------------
// SettingsInit
//
// Initialize interface for reading/writing settings stored in a SPI F-RAM
// device.
//
// In:
//   fram : handle to a Ramtron FM25x SPI F-RAM memory device
//
// Out:
//   nothing
//
// Returns:
//   Handle to settings interface or NULL if an error occurred.
//----------------------------------------------------------------------------
SettingsHandle SettingsInit(FRAM25XHandle fram, CRCHandle crc)
{
	SettingsInterface	*settings = NULL;

	if (fram && crc)
	{
		// Allocate settings interface structure
		settings = (SettingsInterface*)pvPortMalloc(sizeof(SettingsInterface));

		// Save handle to F-RAM if allocation was successful
		if (settings)
		{
			// Save handle to F-RAM
			settings->FRAM = fram;

			// Save handle to CRC interface
			settings->CRC = crc;

			// Create semaphore for shared access protection
			settings->Mutex = xSemaphoreCreateMutex();

			if (settings->Mutex == NULL)
			{
				// Failed to create mutex, free resources
				vPortFree(settings);
				settings = NULL;
			}
		}
	}

	return ((SettingsHandle)settings);
}

//----------------------------------------------------------------------------
// SettingsRead
//
// Read settings stored in non-volatile memory.  The block of data read
// is expected to contain a 16-bit CRC in the last two bytes.  If the CRC
// is invalid and there is a backup, the secondary settings are read and
// also checked.  The backup is assumed to start immediately after the
// primary settings.  This function only returns success if the primary or
// secondary settings are read successfully and one of them contains a
// valid CRC.
//
// NOTE: on any an error or timeout, the previous contents of the
//       provided data buffer may be destroyed.
//
// In:
//   handle  : handle to interface for reading and writing settings
//   offset  : offset in non-volatile memory where the settings are stored
//   length  : length of the data to read (including CRC)
//   backup  : pdTRUE if a redundant backup is available and should
//             be read when the CRC is invalid for the primary settings,
//             pdFALSE if there is no backup image in memory after the
//             primary settings
//   timeout : maximum time in ticks to wait for all read(s) to complete
//
// Out:
//   data    : contains the settings read from non-volatile memory
//
// Returns:
//   pdTRUE if successful
//   pdFALSE on error
//----------------------------------------------------------------------------
portBASE_TYPE SettingsRead(
	SettingsHandle	handle,
	uint32_t		offset,
	uint32_t		dataAddress,
	uint32_t		length,
	portBASE_TYPE	backup,
	portTickType	timeout)
{
	portTickType		start		= xTaskGetTickCount();
	SettingsInterface	*settings	= (SettingsInterface*)handle;
	portBASE_TYPE		success		= pdFALSE;

	// Check parameters
	if (settings && dataAddress && (length > 0))
	{
		// Acquire exclusive to the settings interface
		if (xSemaphoreTake(settings->Mutex, timeout) == pdPASS)
		{
			// Read primary settings and check CRC
			if (SettingsCheckedRead(handle, offset, dataAddress, length,
				TicksRemaining(start, timeout)))
			{
				success = pdTRUE;
			}
			else if (backup)
			{
				// Read backup settings and check CRC
				if (SettingsCheckedRead(handle, offset + length, dataAddress, length,
					TicksRemaining(start, timeout)))
				{
					success = pdTRUE;
				}
			}

			// Release settings interface
			xSemaphoreGive(settings->Mutex);
		}
	}

	return success;
}

//----------------------------------------------------------------------------
// SettingsWrite
//
// Write settings into non-volatile memory.  A 16-bit CRC is expected to
// be contained in the last two bytes.  The CRC is re-calculated and updated
// before the data is saved.  If there is a backup copy, it is assumed to
// start immediately after the primary settings.  The backup will only be
// written if the primary settings are stored successfully.
//
// CAUTION: an error is NOT returned if the primary settings are written
//          successfully and only the backup fails
//
// In:
//   handle  : handle to interface for reading and writing settings
//   offset  : offset in non-volatile memory where the settings are stored
//   length  : length of the data to read (including CRC)
//   backup  : pdTRUE if a redundant backup should also be written
//   timeout : maximum time in ticks to wait for all write(s) to complete
//
// Out:
//   data    : the 16-bit CRC for all data preceding the CRC itself is
//             recalculated and updated in the last two bytes of this buffer
//
// Returns:
//   pdTRUE if successful
//   pdFALSE on error
//----------------------------------------------------------------------------
portBASE_TYPE SettingsWrite(
	SettingsHandle	handle,
	uint32_t		offset,
	uint32_t		dataAddress,
	uint32_t		length,
	portBASE_TYPE	backup,
	portTickType	timeout)
{
	portTickType		start		= xTaskGetTickCount();
	SettingsInterface	*settings	= (SettingsInterface*)handle;
	portBASE_TYPE		success		= pdFALSE;

	// Check parameters
	if (settings && dataAddress && (length > 0))
	{
		// Acquire exclusive to the settings interface
		if (xSemaphoreTake(settings->Mutex, timeout) == pdPASS)
		{
			// Write primary settings and CRC into non-volatile memory
			if (SettingsCheckedWrite(handle, offset, dataAddress, length,
				TicksRemaining(start, timeout)))
			{
				success = pdTRUE;

				if (backup)
				{
					// Write backup copy to non-volatile memory (and store CRC).
					// Don't check return. If this fails, it is not considered
					// an error.
					SettingsCheckedWrite(handle, offset + length,
							dataAddress, length, TicksRemaining(start, timeout));
				}
			}

			// Release settings interface
			xSemaphoreGive(settings->Mutex);
		}
	}

	return success;
}
