


// -----------------------------------------------------
// Designed By  :       Paul Calinawan
// Date         :       January 25, 1995
// -----------------------------------------------------
// Compiler     : BORLAND 4.0 C/C++
// -----------------------------------------------------
// Purpose      : A class object for the metrabyte 
//                MCPT 8X8 matrix board.
// -----------------------------------------------------        
// Rev          : 00.50 : First QRT library release
// -----------------------------------------------------

  
// -------------------------------------------------------------
// Copyright (c)    :   by Radionics Inc., 1993,1994, 1995
// -------------------------------------------------------------

                               

#include <assert.h>

#include "metclass.h"            


	const   int mcpt88_BANKS  = 2,
   	            BANK_Y = 1,
  	            BANK_X = 0;

	// -----------------------------------------------------------
	// Constructor code. 
	// -----------------------------------------------------------
	   
	mcpt88::mcpt88 (int ba, RADX_CNTRL cntrlSel)                        
			:	metboard() {
			
		metboardParam newParam(mcpt88_BANKS, ba, cntrlSel);
		Config(newParam);
	}

	mcpt88::mcpt88 (int ba, metCntrlParam cntrlPa)                        
			:metboard() {
			 
		metboardParam newParam(mcpt88_BANKS, ba, cntrlPa);
		Config(newParam);
	}       
	
	// -----------------------------------------------------------
	// Default Constructor
	
	mcpt88::mcpt88 (void)
			:metboard( ) { }
							
	
	// ------------------------------------------------------------
	// This function activates the specified relay in Bank_X
	// of the mcpt 8x8 metrabyte board and leaves the rest of
	// the relays with their original states. Valid relay no (0..7)
	// ------------------------------------------------------------

	void   mcpt88::CloseRelayX(int relay) 
	{    
		assert( 7 >= relay >= 0 );                      	            		// arg check 

		    int     rmask = 0x01, pattern;

			rmask = rmask << relay;                 							// Bit shift to Select a mask
																				// for the specified relay.
			update();                                                                        
			pattern = bank_byte_x | rmask;          							// select bit to set

			WriteBankX(pattern);
	}


	// ------------------------------------------------------------
	// This function activates the specified relay in Bank_Y
	// of the mcpt 8x8 metrabyte board and leaves the rest of
	// the relays with their original states. Valid relay no (0..7)
	// ------------------------------------------------------------

	void   mcpt88::CloseRelayY(int relay)
    {    
		assert( 7 >= relay >= 0);                      	                		// arg check 
   
		    int     rmask = 0x01, pattern;

			rmask = rmask << relay;                 							// Bit shift to Select a mask
																				// for the specified relay.
            update();                                                                        
			pattern = bank_byte_y | rmask;          							// select bit to set

            WriteBankY(pattern);
	}


	// ------------------------------------------------------------
	// This function de-activates the specified relay in Bank_X
	// of the mcpt 8x8 metrabyte board and leaves the rest of the
	// relays with their original states. Valid relay no (0..7)
	// ------------------------------------------------------------

	void   mcpt88::OpenRelayX(int relay) 
    {    
		assert( 7 >= relay >= 0 );                     	                		// arg check 

    		int     rmask = 0x01, pattern;

			rmask = rmask << relay;                 							// Bit shift to Select a mask
																				// for the specified relay.
			rmask = ~rmask;                         							// Invert mask,

            update();
			pattern = bank_byte_x & rmask;          							// select bit to reset

			WriteBankX(pattern);
	}


	// ------------------------------------------------------------
	// This function de-activates the specified relay in Bank_Y
	// of the mcpt 8x8 metrabyte board and leaves the rest of the
	// relays with their original states. Valid relay no (0..7)
	// ------------------------------------------------------------

	void   mcpt88::OpenRelayY(int relay) 
    {
		assert( 7 >= relay >= 0 );                     	                		// arg check 
            
		    int     rmask = 0x01, pattern;

			rmask = rmask << relay;                 							// Bit shift to Select a mask
																				// for the specified relay.
			rmask = ~rmask;                         							// Invert mask,

            update();            
			pattern = bank_byte_y & rmask;          							// select bit to reset

			WriteBankY(pattern);
	}


	// ------------------------------------------------------------
	// This function deactivates ALL relays of Bank_X
	// ------------------------------------------------------------

	void   mcpt88::OpenBankX(void) { 
	
			    WriteBankX(ALL_OFF); 
	}


	// ------------------------------------------------------------
	// This function deactivates ALL relays of Bank_Y
	// ------------------------------------------------------------

	void   mcpt88::OpenBankY(void) { 
	
			    WriteBankY(ALL_OFF);
	}


	// ------------------------------------------------------------
	// This function resets ALL relays on both Bank_X and Bank_Y
	// ------------------------------------------------------------

	void   mcpt88::Reset(void) {

			    OpenBankY();
                OpenBankX();
	}


	// ------------------------------------------------------------
	// This is an access function which returns the current
	// byte value of Bank_X on an mcpt8x8 board
	// ------------------------------------------------------------

	int     mcpt88::RetrieveBankX(void){

                update();
			    return(bank_byte_x);
	}


	// ------------------------------------------------------------
	// This is an access function which returns the current
	// byte value of Bank_Y on an mcpt8x8 board
	// ------------------------------------------------------------

	int     mcpt88::RetrieveBankY(void) {

                update();
	    		return(bank_byte_y);
	}

	
	// ------------------------------------------------------------
	// This function calls the function write_bank_byte from the
	// metboard base class. The bit pattern of the byte value is
	// written to Bank_X.
	// ------------------------------------------------------------

	void   mcpt88::WriteBankX(int bit_pattern) {
                
			    WriteBankByte(BANK_X, bit_pattern);
			    update();        									                     
    }

    
	// ------------------------------------------------------------
	// This function calls the function write_bank_byte from the
	// metboard base class. The bit pattern of the byte value is
	// written to Bank_Y.
	// ------------------------------------------------------------

	void   mcpt88::WriteBankY(int bit_pattern) {
        
			    WriteBankByte(BANK_Y, bit_pattern);
			    update();           								
    }

    
	// -----------------------------------------------------------
	// This function updates the private objects of the class
	// mcpt88 by reading then assigning the current byte values
	// of both Bank_X and Bank_Y
	// -----------------------------------------------------------

	void   mcpt88::update(void){

			int base_board_adr = 0;
    	
			bank_byte_x = ReadBankByte(base_board_adr);
			bank_byte_y = ReadBankByte(base_board_adr + 1);
	}
      
	 
