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

#include "systeminit.h"

// Direction Register Data - Port B
//
//		Port 0 - OUTPUT		- LINK LED


//
// Direction Register Data - Port C
//
//		Port 0 - OUTPUT		- Heart Beat LED
//
//		Port 1 - INPUT		- LINK Push Button Switch
//
//		Port 2 - OUTPUT		- Latch Data to 7 Segment  Display - 2 digits
//		Port 3 - OUTPUT		- RX / TX Enable Line
//
//		Port 4 - OUTPUT		- Latch Data from 16 Bit P-Serial Input Register
//
//		Port 5 - OUTPUT		- Unused
//
//		Port 6 - RESET LINE
//
//		Port 7 - N/A
//

//
// Direction Register Data - Port D
//
//		Port 0 - 
//		Port 1 - 
//		Port 2 - 
//		Port 3 - 
//		Port 4 - 
//
//		Port 5 - OUTPUT		- Display LED BLANKING
//
//		Port 6 - OUTPUT		- Comm RX LED
//		Port 7 - OUTPUT		- Comm TX LED
//



void SystemInitialize(void)
{
		// Port B
		
		DDRB = ((1<<PB0));
	
		// Port C Direction Register and Pull up resistor setup
		
		PORTC = ((1<<PC1));		
		DDRC = ((1<<PC4) | (1<<PC3) | (1<<PC2) | (1<<PC0));
	
		//
		
		DDRD = ((1<<PD7) | (1<<PD6) | (1<<PD5) | (1<<PD1));
		
		
		
}