
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



#include <avr/pgmspace.h>

#include <avr/wdt.h>

#include <avr/interrupt.h>

//

#include "systeminit.h"

#include "kernel.h"

//
//

#define F_CPU			8000000 

#define BAUD			115200UL

 #include <util/setbaud.h>


//
//
//

#ifdef ATMEGA_32K_FLASH
char  ITECH_PROPERTY[] PROGMEM =
			"A property of Imaging Technologies Inc.							\
            This document contains CONFIDENTIAL and proprietary information		\
            which is the property of Imaging Technologies, Inc. It may not be	\
            copied or transmitted in whole or in part by any means to any		\
            media without prior written permission from							\
            Imaging Technologies, Inc., Copyright 2012" ;
#endif
 

//
//
//

void	DisableWatchDog(void)
{			
		wdt_disable();
}

void	EnableWatchDog(void)
{			
		wdt_enable(WDTO_120MS);
}

void	ActivateGlobalInterrupt(void)
{			
		sei();
}

void	DeActivateGlobalInterrupt(void)
{			
		cli();
}

//
//
//


int main(void)
{		
		EnableWatchDog(); 
		
		SystemInitialize();
	
		InitializeKernel();		
	
		
		ActivateGlobalInterrupt();
	
			RunKernel();
	
	return (0);	
}