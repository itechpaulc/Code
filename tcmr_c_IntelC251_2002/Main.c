



/*
 *
 *
 *		$Header:   K:/Projects/Tcmr/Source/Main.c_v   1.1   11 Jul 2003 14:48:26   PaulLC  $
 *		$Log:   K:/Projects/Tcmr/Source/Main.c_v  $
 * 
 *    Rev 1.1   11 Jul 2003 14:48:26   PaulLC
 * Incorporated all changes since 0.60.B; Camera Trigger jumping fixes; 
 * Encoder noise filtering; Changes to supporte latest TCMRC HW; Limit switch configure.
 * 
 *    Rev 1.0   Oct 06 2000 14:27:22   PaulLC
 * Checked in from initial workfile by PVCS Version Manager Project Assistant.
 * 
 *
 *		Author : Paul Calinawan
 *
 *			May 2000
 *
 *			Graphics Microsystems Inc
 *			1284 Forgewood Ave
 *			Sunnyvale, CA 94089
 *
 *			(408) 745-7745
 *
 *
 *		Print Quick Camera Control Module
 *	-------------------------------------------
 *
 *
*/

/////////////////////////////////////////////////////////////////////////////
//
//    NOTE:
//
//    This document contains CONFIDENTIAL and proprietary information
//    which is the property of Graphics Microsystems, Inc. It may not
//    be copied or transmitted in whole or in part by any means to any
//    media without Graphics Microsystems Inc's prior written permission.
//
/////////////////////////////////////////////////////////////////////////////


char	code	GMI_PROPERTY[]	=

"	Author : Paul L Calinawan		\
	May 2000						\
	Graphics Microsystems Inc		\
	1284 Forgewood Ave				\
	Sunnyvale, CA 94089				\
	(408) 745-7745					\
	-----							\
	This document contains CONFIDENTIAL and proprietary information   \
	which is the property of Graphics Microsystems Inc It may not     \
	be copied or transmitted in whole or in part by any means to any  \
	media without Graphics Microsystems Inc's prior written permission	";



#include "tcmr.h"

#include "kernel.h"


//////////////////////////////////////////////////
//
// PSD Register Configuration
//
//////////////////////////////////////////////////

BYTE	xdata	psdCsiopRegisters[PSD_CSIOP_REG_LEN]		_at_	0x0500;


//////////////////////////////////////////////////
//
// PSD System Memories Available
//
//////////////////////////////////////////////////

#define		EXTRA_PSD_RAM_LEN			0x0800

BYTE	xdata	psdExtraRam[EXTRA_PSD_RAM_LEN]				_at_	0x0C00;


#define		PSD_MAIN_FLASH_BLOCK_0		0x4000

BYTE	xdata	psdMainFlashBlock_0[PSD_MAIN_FLASH_BLOCK_0]	_at_	0x4000;


#define		PSD_MAIN_FLASH_BLOCK_1		0x4000

BYTE	xdata	psdMainFlashBlock_1[PSD_MAIN_FLASH_BLOCK_1]	_at_	0x8000;


#define		PSD_MAIN_FLASH_BLOCK_2		0x4000

BYTE	xdata	psdMainFlashBlock_2[PSD_MAIN_FLASH_BLOCK_2]	_at_	0xC000;


//////////////////////////////////////////////////
//
// Expanded
// External Output Ports
//
// Important: These ports have no read capability
//				a variable is assigned to buffer
//				port data for each
//
//////////////////////////////////////////////////

BYTE	xdata	externalOutputPortA							_at_	0x0610;

BYTE	near	externalOutputPortAData;

BYTE	xdata	externalOutputPortB							_at_	0x0612;

BYTE	near	externalOutputPortBData;



//////////////////////////////////////////////////
//
// System Initialization Routines
//
//////////////////////////////////////////////////

void	InitPSD(void);

void	InitPorts(void);

void	InitPCAModules(void);

void	InitTimers(void);

void	InitSerial(void);

void	InitInterrupts(void);

void	InitKernel(void);


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void main(void)
{
	InitPSD();

    InitPorts();

    InitPCAModules();

    InitTimers();

    InitSerial();

    InitInterrupts();


    //////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////

	InitKernel();

	SendMessage(TCMR_SystemManagerID, GoActive);


	//////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////

	RunKernel();
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	InitPSD(void)
{
	// PORT A Configuration
	
	psdCsiopRegisters[PSD_PORT_A_CONTROL]	= 
		PSD_PORT_A_CONTROL_DATA;

	psdCsiopRegisters[PSD_PORT_A_DIR]		= 
		PSD_PORT_A_DIR_DATA;
	
	psdCsiopRegisters[PSD_PORT_A_DRIVE_SEL]	= 
		PSD_PORT_A_DRIVE_SEL_DATA;


	// PORT B Configuration
	
	psdCsiopRegisters[PSD_PORT_B_CONTROL]	= 
		PSD_PORT_B_CONTROL_DATA;

	psdCsiopRegisters[PSD_PORT_B_DIR]		= 
		PSD_PORT_B_DIR_DATA;
	
	psdCsiopRegisters[PSD_PORT_B_DRIVE_SEL]	= 
		PSD_PORT_B_DRIVE_SEL_DATA;


	// PORT C Configuration

	psdCsiopRegisters[PSD_PORT_C_DIR]		= 
		PSD_PORT_C_DIR_DATA;
	
	psdCsiopRegisters[PSD_PORT_C_DRIVE_SEL]	= 
		PSD_PORT_C_DRIVE_SEL_DATA;
	

	// PORT D Configuration
	
	psdCsiopRegisters[PSD_PORT_D_DIR]		= 
		PSD_PORT_D_DIR_DATA;
	
	psdCsiopRegisters[PSD_PORT_D_DRIVE_SEL]	= 
		PSD_PORT_D_DRIVE_SEL_DATA;

}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    InitPorts(void)
{
    // Port 0 - Dual Port Address Lines
    // Port 2 - Dual Port Address Lines

    // Port 1
	
	HEART_BEAT_OFF();

    RESET_TOP_CAMERA_TRIGGER();

    RESET_BOTTOM_CAMERA_TRIGGER();


    // Port 3
	// TURN OFF UNUSED PORTS

	PORT3_4_OFF();
	PORT3_5_OFF();


	// PSD Ports

	HOLD_TOP_TRANSPORT_RESET();
	HOLD_BOTTOM_TRANSPORT_RESET();

	CLEAR_TOP_CAMERA_IN_0();
	CLEAR_TOP_CAMERA_IN_1();

	CLEAR_BOTTOM_CAMERA_IN_0();
	CLEAR_BOTTOM_CAMERA_IN_1();

	// External Outputs

	CLEAR_EXT_OUT_PORT_A();
	CLEAR_EXT_OUT_PORT_B();	

	HOLD_TOP_CAMERA_SYSTEM_RESET();
	HOLD_BOTTOM_CAMERA_SYSTEM_RESET();

	CLEAR_ENCODER_DIGITAL_FILTER();
	CLEAR_WEB_ENCODER_DIVIDE_BY();

	if(PRINT_QUICK)
		SET_PCA_CLOCK_10_MICROSEC();
	else
		SET_PCA_CLOCK_1_MICROSEC();
}


//////////////////////////////////////////////////
//
//  Compare 0   - Web TDC Pulse Capture
//					(falling edge)
//
//  Compare 1   - Toggles the 
//					Top Camera Trigger
//
//  Compare 2   - Toggles the 
//					Bottom Camera Trigger
//
//  Capture 3   - Web Encoder Pulse Capture
//					(falling edge)
//
//  Compare 4   - Timer used for Kernel
//					System Timing
//
//////////////////////////////////////////////////

void InitPCAModules(void)
{
	//// Fosc /12 = PCA Tick = 0.5 micro second  @24 Mhz

	// PCA Tick = 10 microsecond 
	// Using 100 Khz Clock Reference on ECI Port

	CMOD    = 0x06;

	// Module 0, Compare 0 - Start with No Operation
	
	CCAPM0 = 0x00;

	// Module 1, Compare 1 - Start with No Operation

	DISABLE_OUTPUT_COMPARE_1();

	// Module 2, Compare 2 - Start with No Operation

	DISABLE_OUTPUT_COMPARE_2();

	// Module 3, Capture on Falling edge

	CCAPM3 = 0x10;

	// Module 4, Compare 4 - Start with No Operation

	CCAPM4 = 0x00;

	// Begin Running the PCA Timers

	CCON    = 0x40;     
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    InitTimers(void)
{
	// Timer 0 and 1 Settings - All are unused

	TCON = 0x00;
	TMOD = 0x00; 


	// Timer 2 and 3 Settings - All are unused

	T2CON = 0x00;
	T2MOD = 0x00;
}



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    InitSerial(void)
{
	// Serial Channel Unused

	SCON = 0x00;
}


//////////////////////////////////////////////////
//
// Interrupt Priority Settings:
//
//		PCA								- Highest
//		PC COMM (Ext0 Interrupts)		- Medium
//		MOTOR COMM (Ext1 Interrupts)	- Low
//
//		others							- Lowest
//
//		Others Disabled:
//			TIMER 0
//			SERIAL
//			TIMER1
//			TIMER2
//
//////////////////////////////////////////////////

void    InitInterrupts(void)
{
	// Set Interrupt Priority

	IPH0 = 0x41;
	IPL0 = 0x44;

	// Enable PCA, EXT INTR 0,1 and TIMER0
	// Disable Other Interrupts

	// Set Register IE0

		EA  = SET;
		EC  = SET;

	//
	// Individually set below
	//
	// ET2, ES
	// ET1, EX1
	// ET0, EX0
	//

	DISABLE_OUTPUT_COMPARE_1();
	DISABLE_OUTPUT_COMPARE_2();

	DISABLE_WEB_ENCODER_TDC_CAPTURE();

	DISABLE_WEB_ENCODER_CAPTURE();

	DISABLE_MOTOR_CHIPSET_INTERRUPT();

	DISABLE_PC_TO_TCMR_RECEPTION();

	DISABLE_OUTPUT_COMPARE_4();
}





