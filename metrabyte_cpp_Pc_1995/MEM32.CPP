

// -----------------------------------------------------
// Designed By  :       Paul Calinawan
// Date         :       January 19, 1995
// -----------------------------------------------------
// Compiler     : BORLAND 4.0 C/C++
// -----------------------------------------------------
// Purpose      : A class object for the metrabyte 
//                MEM 32 SPST relay board.
// -----------------------------------------------------        
// Rev          : 00.50 : First QRT library release
// -----------------------------------------------------

  
// -------------------------------------------------------------
// Copyright (c)    :   by Radionics Inc., 1993,1994,1995
// -------------------------------------------------------------


#include <assert.h>
#include <iostream.h>

							     
#include "metclass.h"   

	// -----------------------------------------------------------
	// Constructor code. 
	// -----------------------------------------------------------
	
	mem32::mem32 (int ba, RADX_CNTRL cntrlSel)                        
			:	mbb32(ba, cntrlSel) { }

	mem32::mem32 (int ba, metCntrlParam cntrlPa)                        
			:mbb32(ba, cntrlPa) { }

    
    // ------------------------------------------------------------                                                 
    // Default Constructor
    
    mem32::mem32 (void) 
			:mbb32( ) { }
    
   
	// ------------------------------------------------------------
	// This function activates the specified relay in given bank
	// of the mem32 metrabyte board and leaves the rest
	// of the relays to their original state. Valid relay no 0 to 7
	// Valid bank 0 to 3
	// ------------------------------------------------------------

	void    mem32::CloseRelay(int bank, int relay) {
    
        SetOutput(bank, relay);                 
	}


	// ------------------------------------------------------------
	// This function deactivates the specified relay in given bank
	// of the mem32 metrabyte board and leaves the rest of the
	// relays to their original state. Valid relay no 0 to 7
	// Valid bank 0 to 3
	// ------------------------------------------------------------

	void    mem32::OpenRelay(int bank, int relay) {

        ClearOutput(bank, relay);   
	}


	
    
