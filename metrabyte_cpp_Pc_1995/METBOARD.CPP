
		  

// -----------------------------------------------------
// Designed By  :	Paul Calinawan
// Date 		:	January 15, 1995
// -----------------------------------------------------
// Compiler     : Borland 4.0 c/c++
// -----------------------------------------------------
// Purpose		: A virtual base class object for the
//		  		  metrabyte "m" series board.
// -----------------------------------------------------        
// Rev          : 00.50 : First QRT library release
// -----------------------------------------------------


// -------------------------------------------------------------
// Copyright (c)    :   by Radionics Inc., 1993,1994, 1995
// -------------------------------------------------------------
							   

#include <dos.h>
#include <assert.h>

#include "metclass.h"    


	const int MAX_TRIES = 1000;

	// ------------------------------------------------------------
	// Metrabyte Controller Parameters
	
	metCntrlParam::metCntrlParam(void)
		: adrptr(0), bus(0) { };
		
	metCntrlParam::~metCntrlParam(void){ }; 

	metCntrlParam::metCntrlParam( RADX_CNTRL cntrlSel ){                    	// Select RADX_CNTRL_A 
																				// or RADX_CNTRL_B 
		if (cntrlSel == RADX_CNTRL_A) {
				adrptr = ADRPTR_A; 
				bus = BUS_A;
		 }   
			else
		if (cntrlSel == RADX_CNTRL_B) {
				adrptr = ADRPTR_B; 
				bus = BUS_B;
		 }        
	}
			
	metCntrlParam::metCntrlParam( int adr_ptr, int bus_ptr)
		:   adrptr(adr_ptr),
			bus(bus_ptr) { }    
	

	BOOL metCntrlParam::operator == ( const metCntrlParam &ref ) {
	
		return (BOOL) (   this->adrptr == ref.adrptr
						&& this->bus == ref.bus );
	}                                        
									 
	BOOL metCntrlParam::operator != ( const metCntrlParam &ref ) {                                
	
		return ( (*this) == ref ) ? FALSE : TRUE;
	}               

	
	// ------------------------------------------------------------	
	// Metrabyte Board Attributes
	 
	metboardParam::metboardParam(void) 
		:   banks(0),
			board_adr(0),
			mcParam() { }
			
	metboardParam::~metboardParam(void) {  }
	
	metboardParam::metboardParam( int bn, int ba, RADX_CNTRL cntrlSel)
		:   banks(bn), 
			board_adr(ba), 
			mcParam(cntrlSel) { } 

	metboardParam::metboardParam( int bn, int ba, metCntrlParam cntrlParam)
		:   banks(bn), 
			board_adr(ba), 
			mcParam(cntrlParam) { }                             				// Copy Constructor


	// ------------------------------------------------------------        
	// Overloaded operators
			
	BOOL metboardParam::operator == ( const metboardParam &ref ) {
	
		int this_last_adr = this->board_adr + this->banks,
			ref_last_adr = ref.board_adr + ref.banks;
		
		for (int i=this->board_adr; i <= this_last_adr; i++)    				// look at all the addresses
		   for (int j=ref.board_adr; j <= ref_last_adr; j++)    				// occupied by the two metboards

				if ( i == j && this->mcParam.adrptr == ref.mcParam.adrptr
							&& this->mcParam.bus == ref.mcParam.bus) 

							return TRUE;                        				// any of the assigned 
																				// board addresses conflicting?
		return  FALSE;  // no conflicts
	}                      
							 
	BOOL metboardParam::operator != ( const metboardParam &ref ) {
	
		return ( (*this) == ref ) ? FALSE : TRUE ;
	}                      

	
	// ------------------------------------------------------------
	// Class Metboard    
	// ------------------------------------------------------------
	// This function returns an int value of 0x00..0xFF. The 
	// byte value of the bit pattern read from bank. This is a 
	// fast version of the read_byte procedure for the metrabyte. 
	// Valid bank no 0..3
	// ------------------------------------------------------------

	int 	metboard::ReadBankByte(int bank) const                             
	{
		assert ( 3 >= bank >= 0 );
											  
		int   adrptr_pt, read_write_status;

		outp(mParam.mcParam.adrptr, mParam.board_adr + bank);           		// Latch to port

			do{	adrptr_pt = inp(mParam.mcParam.adrptr);

				read_write_status = adrptr_pt & READ_WRITE_STATUS;
			}
			while (read_write_status != 0x00);      							// Wait for r/w bus
																				// to be low
		return(inp(mParam.mcParam.bus));                      
	}


	// ------------------------------------------------------------
	// This function sends a byte value of 0x00..0xFF, The byte 
    // value of the bit pattern written to the bank. This is a 
    // fast version of the write_byte procedure for the metrabyte.
    // Valid bank no 0..3
	// ------------------------------------------------------------

	void    metboard::WriteBankByte(int bank, int byte) const 
	{
        assert ( 3 >= bank >= 0 );

        int   adrptr_pt, read_write_status;
                    
		outp(mParam.mcParam.adrptr, mParam.board_adr + bank);           		// latch to controller      						
                                                                        
			do{	adrptr_pt = inp(mParam.mcParam.adrptr);
            
				read_write_status = adrptr_pt & READ_WRITE_STATUS;
			}                                                           		// Wait for r/w bus
			while (read_write_status != 0x00);      							// to be low

		outp(mParam.mcParam.bus, byte);                                 		// send signal to controller

            do{	adrptr_pt = inp(mParam.mcParam.adrptr);

				read_write_status = adrptr_pt & READ_WRITE_STATUS;
			}                                                           		// Wait for r/w bus
			while (read_write_status != 0x00);      							// to be low         
    }                           


	// -------------------------------------------------------------
	// This is an access function which returns all the values of
	// the virtual base class metboard attributes. 
	// -------------------------------------------------------------

    metboardParam   metboard::Retrieve (void) const {			

            return mParam;
    }                


	// -----------------------------------------------------------
    // Reconfigures metboard with a new parameter
	// -----------------------------------------------------------

	void    metboard::Config( metboardParam newParam ) {
    
		    mParam = newParam;
	}

    // -----------------------------------------------------------
    // Call to Check if controller is responding
    
    BOOL   metboard::initialize(void) 
	{
        int   adrptr_pt, read_write_status, tries = 0;

		outp(mParam.mcParam.adrptr, mParam.board_adr);                  		// Latch to port

			do{	adrptr_pt = inp(mParam.mcParam.adrptr);
            
                if ( tries++ > MAX_TRIES)
					return FALSE;                                       		// not responding
                    
				read_write_status = adrptr_pt & READ_WRITE_STATUS;
			}
			while (read_write_status != 0x00);      				
            
		return(TRUE);                                                   		// responding            
    }                    
   

 	// -----------------------------------------------------------   
    // Overloded equality operator to compare base attributes.
    // Allows address conflicts to be checked. Derived classes
    // will be able to use this operator through implicit 
    // derived - base conversion
   	// -----------------------------------------------------------
							
    BOOL   metboard::operator == (const metboard   &m_comp){

             return (BOOL)
                 (this->mParam ==  m_comp.mParam);
    }

    BOOL   metboard::operator != (const metboard   &m_comp){

             return (BOOL)
                 (this->mParam !=  m_comp.mParam);
    }
        
	// -----------------------------------------------------------
	// Constructors for the class metboard. Note: Since this is a
	// virtual base class, objects of type metboard may not be
	// created
	// -----------------------------------------------------------

	metboard::metboard(void) 
		: mParam() {  }  														// Default Constructor

	metboard::metboard(metboardParam mp) 
		: mParam(mp){ }
		
	metboard::~metboard(void){ }                                        		// destructor
	
	



   
