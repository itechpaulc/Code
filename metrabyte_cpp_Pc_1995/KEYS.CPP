





// -------------------------------------------------------------
// Designed by  :       Paul Calinawan
// Date         :       March 10, 1995
// -------------------------------------------------------------
// Compiler     : BORLAND 4.0 c/c++
// -------------------------------------------------------------
// Rev 00.05    : First QRT library release
// -------------------------------------------------------------
// Copyright (c)    :   by Radionics Inc., 1995
// -------------------------------------------------------------


#include "keyclass.h"

#include <assert.h>
#include <dos.h>


							
// -------------------------------------------------------------
//  Class - Key Parameters
// -------------------------------------------------------------

	keyParam::keyParam(void) 
		: key_id(NO_ID), 
		  param_1(NO_PARAM), param_2(NO_PARAM) { }
		  
	keyParam::~keyParam(void) { }
									
	keyParam::keyParam( char kId, int P1, int P2 )
		: key_id(kId), 
		  param_1(P1), param_2(P2) { }

	// -------------------------------------------------------------
	// Overloaded assignment operator
	
	keyParam & keyParam::operator = (const keyParam &kp) {

		key_id  = kp.key_id;
		param_1 = kp.param_1;
		param_2 = kp.param_2;

		return (*this);
	}
	
	// -------------------------------------------------------------
	// Compare each key parameters. The Borland TSet template	
	// class will not allow duplicates to be added to the set.

	BOOL keyParam::operator == (const keyParam &kp) {

		return (BOOL)(   key_id == kp.key_id
					  && param_1 == kp.param_1
					  && param_2 == kp.param_2 ); 
	}
	

// -------------------------------------------------------------
// Class - Keypad (Abstract Base)
// -------------------------------------------------------------

	// -------------------------------------------------------------
	// Static variables initialization

	int	keypad::onTime	= KEY_ON; 
	int	keypad::offTime	= KEY_OFF;

	
	// -------------------------------------------------------------
	// Constructors 

	keypad::keypad(void)
		: keypadID(NO_ID),
		  kset() { }

		
	keypad::keypad(char kId)
		: keypadID(kId),
		  kset() { }
		
	keypad::~keypad(void) { }								// Destructor

	
	// -------------------------------------------------------------
	//  Add a key to the Set Container. NO keyParam Duplicates
	//  are allowed.
			
	void	keypad::AddKey(keyParam kp) {

		kset.Add(kp);
	}

	// -------------------------------------------------------------
	//  Change keypress timing parameters
			
	void	keypad::SetTimer(int OnTm, int OffTm) {

		keypad::onTime	= OnTm; 
		keypad::offTime	= OffTm;
	}

	// -------------------------------------------------------------
	// Retrieve keypress timing parameters

	void	keypad::GetTimer(int &OnTm, int &OffTm) {
	
		OnTm  = keypad::onTime; 
		OffTm = keypad::offTime;			
	}		

	
// -------------------------------------------------------------
// Class - Keypad using an Mem32 as the controller board
// -------------------------------------------------------------

	keypadMem32::keypadMem32(void)
		: keypad(),
		  cntrl(0) { }
			
	keypadMem32::keypadMem32(mem32 *controller, char kId) 
		: keypad(kId),
		  cntrl(controller) { }
		
	keypadMem32::~keypadMem32(void) { }


	
	// -------------------------------------------------------------
	//	Single character keypress

	void 	keypadMem32::Keypress(const char keySelect, 
									const char kpID, KPRESS_TYPE kpt) {

		keyParam				key;            		
				
		keySetIterator			ksetIterator(kset);

		// -------------------------------------------------------------
		// If id is available do an id check to verify if there is 
		// a match on this keypad's id before performing the keypress.
		// If no id perform the keypress anyway.
		
		if ( (kpID == NO_ID) || (kpID == keypadID) )							 
		{		                                          						
			while( ksetIterator )												// Search for the matching
			{                                                                 	// key from the set
				key	= ksetIterator++;
				
				if (key.key_id == keySelect)               						// Is this the keyParam
			
				{			
				switch (kpt) {													// perform the keypress

					/*
				  case MOMENTARY // Latch Key, wait, Unlatch Key, wait
			  
							:  	cntrl->CloseRelay(key.param_2, key.param_1);
								delay(keypad::onTime);                          
								cntrl->OpenRelay(key.param_2, key.param_1);        	
								delay(keypad::offTime);                            	
								break;
					*/

				  case HOLD  	 // Latch key
			  
							 :  cntrl->CloseRelay(key.param_2, key.param_1);
								break;

				  case RELEASE	 // Un Latch Key
			  
							 :  cntrl->OpenRelay(key.param_2, key.param_1);
								break;

					default	 :  assert(0);	return;
				  }// endswitch 	
				}// endif
			}// endwhile
		}// endif			
	}// endkeypress


	// -------------------------------------------------------------
	// c string keypress
	// Perform keypress for each of the character in the c string
						
	void 	keypadMem32::Keypress(const char *keySelect, 
									const char kpID, KPRESS_TYPE kpt) {

		while(*keySelect) {                										// while there is char
												
			Keypress(*keySelect, kpID, kpt);									// call single char keypress
			keySelect++;														// for each character
		}
	}
	
	
	// -------------------------------------------------------------
	// Perform keypress for each of the character in the string class
	/*	
	void 	keypadMem32::Keypress(const string keySelect, 
									const char kpID, KPRESS_TYPE kpt) {

		Keypress(keySelect.c_str(), kpID, kpt);									// call c string keypress
	}
	*/

	// -------------------------------------------------------------
	//	Perform keypress for each of the number in the long int argument

	void 	keypadMem32::Keypress(const long keySelect, 
									const char kpID, KPRESS_TYPE kpt) {

		const int 	base = 10;
		char		c_string[15];

		ltoa(keySelect, c_string, base);										// convert to c string

		Keypress(c_string, kpID, kpt);	                 						// call c string keypress
	}


	// -------------------------------------------------------------
	// Keypress with no id. Provide parameter for keypress type
					
	void 	keypadMem32::Keypress( const char key, KPRESS_TYPE kpt ) {

				Keypress(key, NO_ID, kpt);
	}
					
	void 	keypadMem32::Keypress( const char *keys, KPRESS_TYPE kpt ) {

				Keypress(keys, NO_ID, kpt);
	}

	/*
	void 	keypadMem32::Keypress( const string keys, KPRESS_TYPE kpt ) {

				Keypress(keys, NO_ID, kpt);
	}
	*/
							
	void 	keypadMem32::Keypress( const long keys, KPRESS_TYPE kpt ) {

				Keypress(keys, NO_ID, kpt);
	}

	
			
	// -------------------------------------------------------------
	// Change controller ?

	void	keypadMem32::Config(mem32 *newController) {

		cntrl = newController;
	}
	



	
// -------------------------------------------------------------
// Class - Keypad using an Mcpt 8x8 as the controller board
//			
// Note: 	Since this is a mostly similar to keypad mem32 class
//			Refer to the keypadmem32 class for documentation
//
// -------------------------------------------------------------

	keypadMcpt88::keypadMcpt88(void)
		: keypad(),
		  cntrl(0) { }
			
	keypadMcpt88::keypadMcpt88(mcpt88 *controller, char kId) 
		: keypad(kId),
		  cntrl(controller) { }
		
	keypadMcpt88::~keypadMcpt88(void) { }

	// -------------------------------------------------------------
	//	Single character keypress
	
	void 	keypadMcpt88::Keypress(const char keySelect, 
									const char kpID, KPRESS_TYPE kpt) {

		keyParam 			key;                           		
		keySetIterator		ksetIterator(kset);

		
		if ( (kpID == NO_ID) || (keypadID == kpID) )
		{
		
			while( ksetIterator ) 
			{
				key = ksetIterator++;
				
				if (key.key_id == keySelect)
				{			
				switch (kpt) {

						/*
				  case MOMENTARY // Latch Key, wait, Unlatch Key, wait

							 :  cntrl->CloseRelayX(key.param_1);
								cntrl->CloseRelayY(key.param_2);
								delay(keypad::onTime);
								cntrl->OpenRelayX(key.param_1);
								cntrl->OpenRelayY(key.param_2);								
								delay(keypad::offTime);
								break;
						*/

				  case HOLD 	// Latch Key

							:   cntrl->CloseRelayX(key.param_1);
								cntrl->CloseRelayY(key.param_2);
								break;

				  case RELEASE	// Un Latch Key

							: 	cntrl->OpenRelayX(key.param_1);
								cntrl->OpenRelayY(key.param_2);
								break;

					default	:  assert(0);	return;

				  }// endswitch
				}// endif
			}// endwhile
		}// endif			
	}// endkeypress


	// -------------------------------------------------------------
	//
						
	void 	keypadMcpt88::Keypress(const char *keySelect, 
									const char kpID, KPRESS_TYPE kpt) {

		while(*keySelect) {
		
			Keypress(*keySelect, kpID, kpt);
			keySelect++;
		}
	}
	
	// -------------------------------------------------------------
	//
	/*
	void 	keypadMcpt88::Keypress(const string keySelect, 
									const char kpID, KPRESS_TYPE kpt) {

		Keypress(keySelect.c_str(), kpID, kpt);
	}
	*/

	// -------------------------------------------------------------
	//						

	void 	keypadMcpt88::Keypress(const long keySelect, 
									const char kpID, KPRESS_TYPE kpt) {

		const int 	base = 10;
		char		buffer[15];

		ltoa(keySelect, buffer, base);

		Keypress(buffer, kpID, kpt);	
	}
	
	// -------------------------------------------------------------
	// 
					
	void 	keypadMcpt88::Keypress( const char key, KPRESS_TYPE kpt ) {

				Keypress(key, NO_ID, kpt);
	}
					
	void 	keypadMcpt88::Keypress( const char *keys, KPRESS_TYPE kpt ) {

				Keypress(keys, NO_ID, kpt);
	}

	/*
	void 	keypadMcpt88::Keypress( const string keys, KPRESS_TYPE kpt ) {

				Keypress(keys, NO_ID, kpt);
	}
	*/
								
	void 	keypadMcpt88::Keypress( const long keys, KPRESS_TYPE kpt ) {

				Keypress(keys, NO_ID, kpt);
	}

		
	// -------------------------------------------------------------
	//

	void	keypadMcpt88::Config(mcpt88 *newController) {

		cntrl = newController;
	}
	

