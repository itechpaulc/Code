



// -----------------------------------------------------
// Designed By  :	Paul Calinawan
// Date 		:	April 5, 1995
// -----------------------------------------------------
// Compiler     : Borland 4.0 c/c++
// -----------------------------------------------------
// Purpose		: A class object for the
//		  		  metrabyte Pio 12 board.
// -----------------------------------------------------        
// Rev          : 00.50 : First QRT library release
// -----------------------------------------------------


// -------------------------------------------------------------
// Copyright (c)    :   by Radionics Inc., 1993,1994, 1995
// -------------------------------------------------------------


#include <conio.h>
#include <stdio.h>


#include "metcard.h"


		Ppio12Interrupt		pio12Interrupt::curISR = NULL;
		
		
		// -------------------------------------------------------------
		//		
		pio12Interrupt::pio12Interrupt( int ba, int iNum )
			:  base_address(ba),
			   interrupt_number(iNum)	 { }
		
		pio12Interrupt::~pio12Interrupt() {

			curISR = NULL;
		} 

		// -------------------------------------------------------------
		//
		
		void	pio12Interrupt::ConfigPort( int setting ) {

				WritePort(setting, CONTROL);
		}
						
		// -------------------------------------------------------------
		//	 
		
		int	pio12Interrupt::ReadPort( PIO_PORT portSelect ) {

				return inp(base_address + portSelect);
		}

		// -------------------------------------------------------------
		//
		
		void	pio12Interrupt::WritePort( int portByte, PIO_PORT portSel ) {

				outp(base_address + portSel, portByte);
		}		

		
		#pragma option -O2-b-e                      // no global reg allocation
													// ..or dead code elimination
													
		void	interrupt 	pio12Interrupt::IntRoutine(...) {
				
				if (pio12Interrupt::curISR)
					curISR->IntHandler();
		}


 // -------------------------------------------------------------
 //
		
		pio12IO::pio12IO(int base_adr, int iNum) 
				: pio12Interrupt(base_adr, iNum) { };
		
		pio12IO::~pio12IO(void) { };
		
		// -------------------------------------------------------------
		//																				
		void	pio12IO::IntRoutine(void) { }; 									// define virtual functions		                  		
																				// to do nothing
		void    pio12IO::InstallInt(void) { };                          		
		void	pio12IO::DeInstallInt(void) { };                        		
	
