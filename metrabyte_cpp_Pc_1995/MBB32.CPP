



// -----------------------------------------------------
// Designed By  :       Paul Calinawan
// Date         :       January 19, 1995
// -----------------------------------------------------
// Compiler     : BORLAND 4.0 C/C++
// -----------------------------------------------------
// Purpose      : A class object for the metrabyte 
//                MBB 32 TTL I/O board.
// -----------------------------------------------------        
// Rev          : 00.50 : First QRT library release
// -----------------------------------------------------

  
// -------------------------------------------------------------
// Copyright (c)    :   by Radionics Inc., 1993,1994,1995
// -------------------------------------------------------------


#include <stdlib.h> 
#include <assert.h>

							     
#include "metclass.h"  
							
    const int mbb32_BANKS = 4;

	// -----------------------------------------------------------
	// Constructor code. 
	// -----------------------------------------------------------
	
	mbb32::mbb32 (int ba, RADX_CNTRL cntrlSel)                        
			:mii32(ba, cntrlSel) { }

	mbb32::mbb32 (int ba, metCntrlParam cntrlPa)                        
			:mii32(ba, cntrlPa) { }
	
	// ------------------------------------------------------------                                                 
    // Default Constructor
    
    mbb32::mbb32 (void) 
            :mii32( ) { }
    
	// ------------------------------------------------------------
	// This function activates the specified output in given bank
	// of the mbb32 metrabyte board and leaves the rest
	// of the outputs to their original state. 
    // Valid output no 0 to 7.  Valid bank 0 to 3. 
	// ------------------------------------------------------------

	void    mbb32::SetOutput(int bank, int output)
    {    
		assert ( (7 >= output >= 0) && (3 >= bank >= 0) );     					// arg check 
				
		    int     rmask = 0x01, pattern, bank_byte;
                
			rmask = rmask << output;                 							// Bit shift to Select a mask
																				// for the specified Output.
			bank_byte = Retrieve(bank);       					        		// get current Output states
																				// of the selected bank
			pattern = bank_byte | rmask;               							// set the selected bit
            WriteBankByte(bank, pattern);

			update();   
	}


	// ------------------------------------------------------------
	// This function deactivates the specified Output in given bank
	// of the mbb32 metrabyte board and leaves the rest of the
	// Outputs to their original state. Valid Output no 0 to 7
	// Valid bank 0 to 3
	// ------------------------------------------------------------

	void    mbb32::ClearOutput(int bank, int output)
	{
		assert ( (7 >= output >= 0) && (3 >= bank >= 0) );              		// arg check

			int     rmask = 0x01, pattern, bank_byte;

			rmask = rmask << output;                 							// Bit shift to Select a mask
																				// for the specified Output.
			bank_byte = Retrieve(bank);      					        		// get current Output states
																				// of the selected bank
			rmask = ~rmask;                 									// invert mask
			pattern = bank_byte & rmask;            							// select bit to reset
			WriteBankByte(bank, pattern);           

			update();            
	}


	// ------------------------------------------------------------
	// This function activates all Outputs of a given bank
	// Valid bank 0 to 3
	// ------------------------------------------------------------

	void   mbb32::SetBank(int bank)
	{           
		assert(3 >= bank >= 0);                                          		// arg check                             
			
			WriteBankByte(bank, ALL_ON);    
										
			update();
	}


	// ------------------------------------------------------------
	// This function deactivates all Outputs of a given bank
	// Valid bank 0 to 3
	// ------------------------------------------------------------

	void   mbb32::ClearBank(int bank) 
	{        
		assert(3 >= bank >= 0);                          	            		// arg check 
            
        	WriteBankByte(bank, ALL_OFF);   

            update();
	}


	// ------------------------------------------------------------
	// This function resets all banks 
	// ------------------------------------------------------------

	void    mbb32::Reset(void) 
    {
			for (int bank=0; bank<mbb32_BANKS; bank++)
    	    	    ClearBank(bank);
	}


	
