//----------------------------------------------------------------------------
// File : init.c
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
// 1.00 08/17/13 Brandon W. Initial revision.
//----------------------------------------------------------------------------

// Standard includes
#include <string.h>

// RTOS includes
#include "FreeRTOS.h"
#include "task.h"

// Application includes

#include "board.h"

#include "saver.h"

//

#include "battery_controller.h"

#include "saver_overlayhandler.h"

//

#include "saver_cmdhandler.h"
#include "saver_dataacquire.h"

#include "saver_eventstore.h"

#include "saver_manager.h"

#include "init.h"

#include "saverstatemachine.h"

#include "ticks.h"


#include "usbcomm.h"


//----------------------------------------------------------------------------
// SaverProcessorInit
//
// Perform any remaining processor and hardware initialization required
// before setting up peripheral devices.
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
void SaverProcessorInit(void)
{
	//const Pin pinsXT1[] = {XT1_PINS};
	const Pin pinsXT2[] = {XT2_PINS};

	// Stop the watchdog
	WDTCTL = WDTPASSWD | WDTHOLD;

	//PIOConfigure(pinsXT1, PIO_PIN_LIST_SIZE(pinsXT1));
	PIOConfigure(pinsXT2, PIO_PIN_LIST_SIZE(pinsXT2));

	// Unlock XT1 pins for operation
	while(BAKCTL & LOCKBAK)
	{
		BAKCTL &= ~(LOCKBAK);
	}

	// Enable XT1 & XT2
	UCSCTL6 &= ~(XT2OFF + XT1OFF);
	do
	{
		// Clear XT2, XT1, DCO fault flags
		UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG);
		// Clear fault flags
		SFRIFG1 &= ~OFIFG;
		// Delay for oscillator to stabilize
		__delay_cycles(10000);
	} while ((SFRIFG1 & OFIFG) || (UCSCTL7 & XT2OFFG));	// Test osc fault flags

	// XTAL and XTAL2 stable, reduce drive strength
	UCSCTL6 &= ~(XT1DRIVE_3 + XT2DRIVE_3);

	// ACLK=XT1, SMCLK=XT2, MCLK=XT2 (all are divided by 1)
	UCSCTL4 = SELA__XT1CLK + SELS__XT2CLK + SELM__XT2CLK;
	UCSCTL5 = DIVA__1 + DIVS__1 + DIVM__1;
	// FLL Reference Clock = XT2 (divided by 1)
	UCSCTL3 = SELREF__XT2CLK | FLLREFDIV__1;

	// Ensure MCLK and SMCLK are on
	_BIC_SR(CPUOFF);

	// Turn off DCOCLK and FLL, DCO is not used for MCLK or SMCLK
	_BIS_SR(SCG0 | SCG1);
}

//----------------------------------------------------------------------------
// ProcessorMemInit
//
// Perform any remaining processor and hardware initialization required
// before setting up peripheral devices.
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
void ProcessorMemInit(void)
{
	MSP430XUpperMemClear();
}


//----------------------------------------------------------------------------
// SaverDevInitEarly
//
// Initialize shared devices that will be used by the application tasks.
// Only devices that do not require the task scheduler during initialization
// can be configured by this routine.
//
// In:
//   handle : handle to Saver interface
//
// Out:
//   nothing
//
// Returns:
//   pdTRUE if successful, pdFALSE if not
//----------------------------------------------------------------------------
static portBASE_TYPE SaverDevInitEarly(SaverHandle handle)
{
	SaverInterface	*saver				= (SaverInterface*)handle;

	const Pin		pinSDA				= I2C_B2_SDA_PIN;
	const Pin		pinSCL				= I2C_B2_SCL_PIN;

//	const Pin		pinUART0TX			= UART_A0_TX_PIN;
//	const Pin		pinUART0RX			= UART_A0_RX_PIN;

	const Pin		pinUART1TX			= UART_A1_TX_PIN;
	const Pin		pinUART1RX			= UART_A1_RX_PIN;

//	const Pin		pinUART2TX			= UART_A2_TX_PIN;
//	const Pin		pinUART2RX			= UART_A2_RX_PIN;

	const Pin		pinSPI0SCLK			= SPI_B0_SCLK_PIN;
	const Pin		pinSPI0SIMO			= SPI_B0_MOSI_PIN;
	const Pin		pinSPI0SOMI			= SPI_B0_MISO_PIN;
	const Pin		pinSPI1SCLK			= SPI_B1_SCLK_PIN;
	const Pin		pinSPI1SIMO			= SPI_B1_MOSI_PIN;
	const Pin		pinSPI1SOMI			= SPI_B1_MISO_PIN;
	const Pin		pinsTest[]			= {TEST_POINT_PINS};
	const Pin		pinsNoConnect[]		= {NO_CONNECT_PINS};

	const Pin		pinChargeEnable 	= CHARGE_CE_PIN;
	const Pin		pinChargeEn1 		= CHARGE_EN1_PIN;
	const Pin		pinChargeEn2 		= CHARGE_EN2_PIN;

	const Pin		pinBatteryDischarge = BATTERY_DISCHARGE_PIN;
	const Pin 		pinBatteryConnect	= BATTERY_CONNECT_PIN;

	const Pin		pinLightPower		= LIGHT_POWER_PIN;
	const Pin		pinDacTestSignal	= DAC0_PIN;

	portBASE_TYPE	success				= pdTRUE;


	if (saver == NULL)
		return pdFALSE;

	// Initialize the GPIO PORTs that can have interrupts
	PIOInitInterrupts(MSP430_ID_PORT1);
	PIOInitInterrupts(MSP430_ID_PORT2);
	PIOInitInterrupts(MSP430_ID_PORT3);
	PIOInitInterrupts(MSP430_ID_PORT4);


	// Enable Internal Reference as the master, not the A/D
	// Disable Temp Sensor
	// Allow Internal Reference to be available on the output pin
	// Turn On Internal Reference
	// Use 2.5V selection

	REFCTL0 = REFMSTR | REFTCOFF | REFOUT | REFON | REFVSEL_2 ;

	// Configure all test points (as low level outputs)
	PIOConfigure(pinsTest, PIO_PIN_LIST_SIZE(pinsTest));

	// Configure all unconnected pins (as low level outputs)
	PIOConfigure(pinsNoConnect, PIO_PIN_LIST_SIZE(pinsNoConnect));

	// Configure Battery Charger
	PIOConfigure(&pinChargeEnable, 1);
	PIOConfigure(&pinChargeEn1, 1);
	PIOConfigure(&pinChargeEn2, 1);

	PIODeactivate(&pinChargeEnable);
	PIODeactivate(&pinChargeEn1);
	PIODeactivate(&pinChargeEn2);

	// Configure Battery Control Pins
	PIOConfigure(&pinBatteryDischarge, 1);
	PIOConfigure(&pinBatteryConnect, 1);

	PIODeactivate(&pinBatteryDischarge);
	PIOActivate(&pinBatteryConnect);

	PIOConfigure(&pinLightPower, 1);
	PIOActivate(&pinLightPower);

	PIOConfigure(&pinDacTestSignal, 1);



	/////////////////////////////////////////////////////////////////////////
	// TODO: clean this up, initialize all pins on the processor properly
	/////////////////////////////////////////////////////////////////////////


	saver->Devices.CRC = CRCInit();
	if (saver->Devices.CRC == NULL)
		success = pdFALSE;

	saver->Devices.DMA = DMAInit(pdFALSE, pdTRUE, pdFALSE);
	if (saver->Devices.DMA == NULL)
		success = pdFALSE;

	saver->Devices.I2C = I2CInit(I2CBusB2, &pinSDA, &pinSCL,
		I2C_CLOCK_FREQ_HZ);
	if (saver->Devices.I2C == NULL)
		success = pdFALSE;

//	saver->Devices.UART0 = UARTInit(UART0, UARTBaud9600, UARTParityNone,
//		UARTDataLength8, UARTStopBits1, UART_EXT_RX_BUF_SIZE,
//		UART_EXT_TX_BUF_SIZE, &pinUART0TX, &pinUART0RX, NULL,
//		DMAChannel0, UART_TASK_PRIORITY);
//	if (saver->Devices.UART0 == NULL)
//		success = pdFALSE;

	saver->Devices.UART1 = UARTInit(UART1, UARTBaud115200, UARTParityNone,
		UARTDataLength8, UARTStopBits1, UART_CELL_RX_BUF_SIZE,
		UART_CELL_TX_BUF_SIZE, &pinUART1TX, &pinUART1RX, NULL,
		DMAChannel0, UART_TASK_PRIORITY);
	if (saver->Devices.UART1 == NULL)
		success = pdFALSE;

//	saver->Devices.UART2 = UARTInit(UART2, UARTBaud9600, UARTParityNone,
//		UARTDataLength8, UARTStopBits1, UART_CELL_RX_BUF_SIZE,
//		UART_GPS_TX_BUF_SIZE, &pinUART2TX, &pinUART2RX, NULL,
//		DMAChannel0, UART_TASK_PRIORITY);
//	if (saver->Devices.UART2 == NULL)
//		success = pdFALSE;


	saver->Devices.SPI0 = SPIBusInit(SPIBusB0, &pinSPI0SCLK, &pinSPI0SIMO,
			&pinSPI0SOMI, NULL, saver->Devices.DMA, DMAChannel2, DMAChannel3);

	if (saver->Devices.SPI0 == NULL)
		success = pdFALSE;


		saver->Devices.SPI1 = SPIBusInit(SPIBusB1, &pinSPI1SCLK, &pinSPI1SIMO,
			&pinSPI1SOMI, NULL, saver->Devices.DMA, DMAChannel4, DMAChannel5);

	if (saver->Devices.SPI1 == NULL)
		success = pdFALSE;

	//

	saver->Mode = RUN;

	return success;
}

//----------------------------------------------------------------------------
// SaverDevInitLate
//
// Perform any late initialization required after starting the task scheduler.
// This function should be called by the main task when it starts.
//
// In:
//   handle : handle to Saver interface
//
// Out:
//   nothing
//
// Returns:
//   pdTRUE if successful, pdFALSE if not
//----------------------------------------------------------------------------
portBASE_TYPE SaverDevInitLate(SaverHandle handle)
{
	SaverInterface	*saver			= (SaverInterface*)handle;

	const Pin		pinTiltGyroCS 		= GYRO_CS_PIN;
	const Pin		pinTiltGyroPower	= GYRO_POWER_PIN;

	const Pin		pinFRAMCS		= FRAM_CS_PIN;
	const Pin		pinFlashCS		= FLASH_CS_PIN;
	const Pin		pinFlashReset	= FLASH_RESET_PIN;

	const Pin		pinTrackerPower 	= TRACKER_MAIN_POWER_PIN;
	const Pin		pinTrackerPowerOn 	= TRACKER_POWER_ON_PIN;
	const Pin		pinTrackerReset 	= TRACKER_RESET_PIN;
	const Pin		pinTrackerGpsOn 	= TRACKER_GPS_ON_PIN;

	const Pin		pinButtonInt	= BUTTON_PIN;

	const Pin		pinBatteryAlert	= BATTERY_GAUGE_ALERT_PIN;

	portBASE_TYPE	success			= pdTRUE;


	if (saver == NULL)
		return pdFALSE;

	saver->Devices.PMM = PMMInit();
	if (saver->Devices.PMM == NULL)
	{
		success = pdFALSE;
	}
	else if (PMMVCoreLevelSet(saver->Devices.PMM, PMM_VCORE_LEVEL,
		PMM_TIMEOUT) == pdFALSE)
	{
		success = pdFALSE;
	}

	saver->Devices.PressureTemp =
		PressureTempInit(saver->Devices.I2C, PRESSURE_TEMP_I2C_ADDR);
	if (saver->Devices.PressureTemp == NULL)
		success = pdFALSE;

	saver->Devices.Humidity = SHTSensorInit(saver->Devices.I2C,
		HUMIDITY_TEMP_I2C_ADDR, SHTHumidityTempBits10and13);
	if (saver->Devices.Humidity == NULL)
		success = pdFALSE;


	saver->Devices.BatteryController = BatteryControllerInit();
	if (saver->Devices.BatteryController == NULL)
		success = pdFALSE;

	saver->Devices.BatteryGauge = BatteryGaugeInit(saver->Devices.I2C,
		BATTERY_GAUGE_I2C_ADDR, &pinBatteryAlert);
	if (saver->Devices.BatteryGauge == NULL)
		success = pdFALSE;

	//

	saver->Devices.TILTGYRO = TILTGYROMPU6KInit(saver->Devices.SPI0, &pinTiltGyroCS,
		GYRO_SPI_CLOCK_FREQ_HZ, &pinTiltGyroPower);
	if (saver->Devices.TILTGYRO == NULL)
		success = pdFALSE;

	saver->Devices.FRAM = FRAM25XInit(saver->Devices.SPI0, &pinFRAMCS,
		FRAM_SPI_CLOCK_FREQ_HZ);
	if (saver->Devices.FRAM == NULL)
		success = pdFALSE;

	saver->Devices.FLASH = FlashMX25Init(saver->Devices.SPI1, &pinFlashCS,
		FLASH_SPI_CLOCK_FREQ_HZ, &pinFlashReset);
	if (saver->Devices.FLASH == NULL)
		success = pdFALSE;

	//

	// Todo: Create task and do reboot sequence within task, not inside init
//	saver->Devices.TRACKER = TRACKERInit(saver->Devices.UART1,
//			&pinTrackerPower, &pinTrackerPowerOn, &pinTrackerReset, &pinTrackerGpsOn);
//	if (saver->Devices.TRACKER == NULL)
//		success = pdFALSE;

	//

	saver->Devices.Settings = SettingsInit(saver->Devices.FRAM,
		saver->Devices.CRC);
	if (saver->Devices.Settings == NULL)
		success = pdFALSE;

	saver->Devices.DMAMemCopy = DMAMemCopyInit(saver->Devices.DMA, DMAChannel0);
	if (saver->Devices.DMAMemCopy == NULL)
		success = pdFALSE;



#if 0
	saver->Devices.USB = USBInit(devices);
	if (saver->Devices.USB == NULL)
		success = pdFALSE;
#endif

	saver->Devices.RTC = RTCInit();
	if (saver->Devices.RTC == NULL)
		success = pdFALSE;

	//

	ButtonInit(&pinButtonInt);

	return success;
}

//----------------------------------------------------------------------------
// SaverInit
//
// Initialize the Saver interface and all supporting tasks.  This routine
// should be called once before starting the task scheduler.
//
// In:
//   nothing
//
// Out:
//   nothing
//
// Returns:
//   Handle to Saver interface or NULL if an error occurred
//----------------------------------------------------------------------------
SaverHandle SaverInit(void)
{
	SaverInterface	*saver					= NULL;
	portBASE_TYPE	success					= pdFALSE;
	const Pin		pinDacTestSignalEnable	= DAC_TEST_SIGNAL_ENABLE_PIN;
	const Pin		pinAnalogSupplyEnable	= ADC_ANALOG_SUPPLY_ENABLE_PIN;
	const Pin		pinsADC[]				= {ADC_PINS};

	const Pin		accelFilter[]		= {ACCEL_FILTER_SEL_PINS};

	do
	{
		// Allocate Saver interface
		saver = (SaverInterface*)pvPortMalloc(sizeof(SaverInterface));

		if (! saver)
			break;

		// Clear and let the control task initialize most fields
		memset(saver, 0, sizeof(SaverInterface));

		// Create semaphore for shared access protection
		saver->Mutex = xSemaphoreCreateMutex();

		if (! saver->Mutex)
			break;

		// Save the task interval
		saver->TaskCmdInterval	= CMD_TASK_INTERVAL;

		// Create simple notification queue (depth is equal to the
		// number of groups (chunks) of samples in the ring buffer,
		// and each queue item is empty)
		saver->TaskDataNotify = xQueueCreate(
			ADC_CHNL_SAMPLES_RING / ADC_CHNL_SAMPLES_ACQUIRE_PER_NOTIFY,
			0);

		//saver->TaskDataNotify = xQueueCreate(1, 0);

		if (! saver->TaskDataNotify)
			break;

		// Create notification queue (depth is equal to the maximum
		// number of pending events, and each queue item contains
		// information about the event)
		saver->TaskEventNotify = xQueueCreate(EVENTS_PENDING_MAX,
			sizeof(SaverEvent));

		if (! saver->TaskEventNotify)
			break;

		//
		//

		saver->TaskCmdNotify =
				xQueueCreate(SAVER_COMMAND_QUEUE, sizeof(SaverSystemMessage));

		if (! saver->TaskCmdNotify)
			break;

		//
		//

		saver->TaskManagerNotify =
				xQueueCreate(SAVER_MANAGER_QUEUE, sizeof(SaverSystemMessage));

		if (! saver->TaskManagerNotify)
			break;

		//
		//

		saver->TaskOverlayNotify =
				xQueueCreate(SAVER_OVERLAY_QUEUE, sizeof(SaverSystemMessage));

		if (! saver->TaskOverlayNotify)
			break;


		//----------------------------------------------------------------------------
		//
		// The Tasks - State Machines
		//
		//----------------------------------------------------------------------------

		//
		// Create task to handle external commands and messages (pc etc)
		if (xTaskCreate(SaverCmdHndlrTask,
			(signed char *) "SvrCmdHndlr", CMD_TASK_STACK_SIZE,
			saver, CMD_TASK_PRIORITY, &saver->TaskCmdHndlr) != pdPASS)
		{
			break;
		}

		saver->TaskCmdHndlrState = SM_STATE_UNDEFINED;

		// Start the task suspended
		//vTaskSuspend(saver->TaskCmdHndlr);

		//----------------------------------------------------------------------------
		//
		// Create task to filter, check for trigger,
		// and save A/D data into UPPER48K MCU RAM
		if (xTaskCreate(SaverDataAcquireTask,
			(signed char *) "SvrDataAcquire",
			DATA_TASK_STACK_SIZE, saver,
			DATA_TASK_PRIORITY, &saver->TaskDataAcquire) != pdPASS)
		{
			break;
		}

		saver->TaskDataAcquireState = SM_STATE_UNDEFINED;

		// Start the task suspended
		vTaskSuspend(saver->TaskDataAcquire);


		//----------------------------------------------------------------------------
		//
		// Create task to transfer events from FRAM to flash
		if (xTaskCreate(SaverEventStoreTask,
			(signed char *) "SvrEvntStore",
			EVENT_TASK_STACK_SIZE, saver,
			EVENT_TASK_PRIORITY, &saver->TaskEventStore) != pdPASS)
		{
			break;
		}

		saver->TaskEventStoreState = SM_STATE_UNDEFINED;

		// Start the task suspended
		vTaskSuspend(saver->TaskEventStore);


		//----------------------------------------------------------------------------
		//
		// Create task to manage the device
		if (xTaskCreate(SaverManagerTask,
			(signed char *) "SvrMngr",
			MNGR_TASK_STACK_SIZE, saver,
			MNGR_TASK_PRIORITY, &saver->TaskManager) != pdPASS)
		{
			break;
		}
		saver->TaskManagerState = SM_STATE_UNDEFINED;

		// Start the task suspended
		vTaskSuspend(saver->TaskManager);

		//----------------------------------------------------------------------------
		//
		// Create task to manage the LED, and Overlay
		if (xTaskCreate(SaverOverlayTask,
			(signed char *) "SvrOvrly",
			OVERLAY_TASK_STACK_SIZE, saver,
			OVERLAY_TASK_PRIORITY, &saver->TaskOverlay) != pdPASS)
		{
			break;
		}

		saver->TaskOverlayState = SM_STATE_UNDEFINED;

		// Start the task suspended
		vTaskSuspend(saver->TaskOverlay);

		//
		//

		// Configure pins for A/D inputs
		PIOConfigure(pinsADC, PIO_PIN_LIST_SIZE(pinsADC));

		// Configure pin for the DAC Test Signal Enable control
		PIOConfigure(&pinDacTestSignalEnable, 1);
		// Save a copy of the pin, so the reference voltage can be turned
		// on or off as needed
		memcpy(&saver->DacTestSignalEnable, &pinDacTestSignalEnable,
			sizeof(pinDacTestSignalEnable));

		// Configure pin for the Analog Supply control
		PIOConfigure(&pinAnalogSupplyEnable, 1);
		// Save a copy of the pin, so the reference voltage can be turned
		// on or off as needed
		memcpy(&saver->AnalogSupplyEnable, &pinAnalogSupplyEnable,
			sizeof(pinAnalogSupplyEnable));


		// Configure all Accel Filter pins (as low level outputs)
		PIOConfigure(accelFilter, PIO_PIN_LIST_SIZE(accelFilter));

		memcpy(&saver->AccelFilterControl, &accelFilter,
			sizeof(accelFilter));

		//
		//
		// Made it through initialization successfully

		success = pdTRUE;

	} while (0);

	if (success)
	{
		// Do the early device initialization (for devices that
		// can be initialized before the task scheduler starts)
		SaverDevInitEarly((SaverHandle)saver);
	}
	else
	{
		// Initialization failed, free resources
		if (saver)
		{
			if (saver->TaskEventStore)
				vTaskDelete(saver->TaskEventStore);

			if (saver->TaskDataAcquire)
				vTaskDelete(saver->TaskDataAcquire);

			if (saver->TaskCmdHndlr)
				vTaskDelete(saver->TaskCmdHndlr);

			if (saver->TaskEventNotify)
				vQueueDelete(saver->TaskEventNotify);

			if (saver->TaskDataNotify)
				vQueueDelete(saver->TaskDataNotify);

			if (saver->Mutex)
				vSemaphoreDelete(saver->Mutex);

			vPortFree(saver);
			saver = NULL;
		}
	}

	return (SaverHandle)saver;
}
