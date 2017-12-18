


// -----------------------------------------------------
// Designed By  :       Paul Calinawan
// Date         :       January 25, 1995
// -----------------------------------------------------
// Compiler     : BORLAND 4.0 C/C++
// -----------------------------------------------------
// Purpose      : A class object for the metrabyte 
//                Mem 8 Power relay board.
// -----------------------------------------------------        
// Rev          : 00.50 : First QRT library release
// -----------------------------------------------------

  
// -------------------------------------------------------------
// Copyright (c)    :   by Radionics Inc., 1993,1994, 1995
// -------------------------------------------------------------
                               

#include <assert.h>

#include "metclass.h"            


	const   int mem8_BANKS  = 1,
	
				Mem8BaseBank = 0;
				
	// -----------------------------------------------------------
	// Constructor code. 
	// -----------------------------------------------------------
	   
	mem8::mem8 (int ba, RADX_CNTRL cntrlSel)                        
			:	metboard() {
			
		metboardParam newParam(mem8_BANKS, ba, cntrlSel);
		Config(newParam);
	}

	mem8::mem8 (int ba, metCntrlParam cntrlPa)                        
			:metboard() {
			 
		metboardParam newParam(mem8_BANKS, ba, cntrlPa);
		Config(newParam);
	}       
	
	// -----------------------------------------------------------
    // Default Constructor
    
 	mem8::mem8 (void)
            :metboard( ) { }
							
    
	// ------------------------------------------------------------
	// This function activates the specified relay 
	// of the mem 8 metrabyte board and leaves the rest of
	// the relays with their original states. Valid relay no (0..7)
	// ------------------------------------------------------------

	void   mem8::CloseRelay(int relay) 
    {    
		assert( 7 >= relay >= 0 );                      	            		// arg check 

		    int     rmask = 0x01, pattern;

			rmask = rmask << relay;                 							// Bit shift to Select a mask
																				// for the specified relay.
            update();                                                                        
			pattern = bank_byte | rmask;          					    		// select bit to set

            WriteBankByte(Mem8BaseBank, pattern);
	}



	// ------------------------------------------------------------
	// This function de-activates the specified relay 
	// of the mem 8 metrabyte board and leaves the rest of 
	// the relays with their original states. Valid relay no (0..7)
	// ------------------------------------------------------------

	void   mem8::OpenRelay(int relay) 
    {    
		assert( 7 >= relay >= 0 );                     	                		// arg check 

    		int     rmask = 0x01, pattern;

			rmask = rmask << relay;                 							// Bit shift to Select a mask
																				// for the specified relay.
			rmask = ~rmask;                         							// Invert mask,

            update();
			pattern = bank_byte & rmask;          					    		// select bit to reset

			WriteBankByte(Mem8BaseBank, pattern);
	}
    
	
	// ------------------------------------------------------------
	// This function deactivates all relays 
	// ------------------------------------------------------------

	void   mem8::Reset(void) { 
	
			    WriteBankByte(Mem8BaseBank, ALL_OFF); 
	}


	// ------------------------------------------------------------
	// This is an access function which returns the current
	// byte value of mem8 bank 
	// ------------------------------------------------------------

	int     mem8::RetrieveBank(void){

                update();
			    return(bank_byte);
	}

    
	// -----------------------------------------------------------
	// This function updates the private object of the class
	// mem8 by reading then assigning the current byte values
	// of the mem 8 bank
	// -----------------------------------------------------------

	void   mem8::update(void){ 

			bank_byte = ReadBankByte(Mem8BaseBank);     		
	}
      
	 