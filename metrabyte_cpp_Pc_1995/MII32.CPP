		  

// -----------------------------------------------------
// Designed By  :       Paul Calinawan
// Date         :       January 9, 1995
// -----------------------------------------------------
// Compiler     : BORLAND 4.0 C/C++
// -----------------------------------------------------
// Purpose      : A class object for the metrabyte MII32
//                digital input board.
// -----------------------------------------------------        
// Rev          : 00.50 : First QRT library release
// -----------------------------------------------------
     

// -------------------------------------------------------------
// Copyright (c)    :   by Radionics Inc., 1993,1994,1995
// -------------------------------------------------------------
                               
#include <conio.h>
#include <assert.h>
            
#include "metclass.h"           
        
	const int   FOUR_BANKS  = 4;      							
    
	// -----------------------------------------------------------
	// Constructor code. 
	// -----------------------------------------------------------
       
	mii32::mii32 (int ba, RADX_CNTRL cntrlSel)                        
			:	metboard() {
			
		metboardParam newParam(FOUR_BANKS, ba, cntrlSel);
		Config(newParam);
	}

	mii32::mii32 (int ba, metCntrlParam cntrlPa)                        
			:metboard() {
			 
		metboardParam newParam(FOUR_BANKS, ba, cntrlPa);
		Config(newParam);
	}       
	
	// -----------------------------------------------------------
	// Default Constructor
	
	mii32::mii32(void)
			:metboard( ){ }
						  
	// ------------------------------------------------------------
	// This is an access function. It uses c++ referencing to
	// pass the current byte values of the four banks through
	// the fuction arguments. It also updates the objects of mii32.
	// ------------------------------------------------------------

	void   mii32::Retrieve ( int &b1, int &b2, int &b3, int &b4 ) 
	{
			update();
			
			b1 = bank_byte1;        b2 = bank_byte2;
			b3 = bank_byte3;        b4 = bank_byte4;
	}

	// -----------------------------------------------------------
	// This is an access function. It returns the current byte
	// value of a selected bank. Valid bank (0..3)
	// -----------------------------------------------------------

	int    mii32::Retrieve(int bank)
	{
			assert ( 3 >= bank >= 0 );               // arg check                                  
				
				return(ReadBankByte(bank));
	}

	// -----------------------------------------------------------
    // Function returns true if the selected input on the
    // selected bank is high     
	// -----------------------------------------------------------

    BOOL    mii32::IsSet(int bank, int input) {

		assert ( (7 >= input >= 0) && (3 >= bank >= 0) );               		// arg check

			int     rmask = 0x01, bank_byte;

			rmask = rmask << input;                 							// Bit shift to Select a mask
																				// for the specified Output.
			bank_byte = Retrieve(bank);      					        		// get current Output states
																				// of the selected bank

			return (BOOL) ( bank_byte & rmask );       							// select   
    }
    
	// -----------------------------------------------------------
	// This function updates the objects of the class mii32
	// by reading the current byte values of all the four banks
	// -----------------------------------------------------------

	void    mii32::update(void){

		const int base_board_addr = 0;

		bank_byte1 = ReadBankByte(base_board_addr);                             
		bank_byte2 = ReadBankByte(base_board_addr + 1);
		bank_byte3 = ReadBankByte(base_board_addr + 2);
		bank_byte4 = ReadBankByte(base_board_addr + 3);
	}
