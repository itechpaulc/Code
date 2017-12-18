


// -------------------------------------------------------------
// Designed by  :       Paul Calinawan
// Date         :       March 7, 1995
// -------------------------------------------------------------
// Compiler     : BORLAND 4.0 c/c++
// -------------------------------------------------------------
// Rev 00.05    : First QRT library release
// -------------------------------------------------------------
// Copyright (c)    :   by Radionics Inc., 1995
// -------------------------------------------------------------



#include "kpdclass.h"      


// -------------------------------------------------------------
// Several functions have been disabled to allow class to
// be used under Windows
// -------------------------------------------------------------


	Mcpt88KeypadModule::Mcpt88KeypadModule(void) 
		: keypadMcpt88(0) { }		
	
	// -------------------------------------------------------------
	//

	Mcpt88KeypadModule::Mcpt88KeypadModule( mcpt88 *cntrllr,  char kpdid ) 
		: keypadMcpt88(cntrllr, kpdid) { }
		
		
	Mcpt88KeypadModule::~Mcpt88KeypadModule(void) { }

			
	// -------------------------------------------------------------
	//
					
	void Mcpt88KeypadModule::
	Keypress(const char key, const char id, KPRESS_TYPE kpt) {

		Parent::Keypress(key, id, kpt);
	}
					
	void Mcpt88KeypadModule::
	Keypress(const char *keys, const char id, KPRESS_TYPE kpt) {

		Parent::Keypress(keys, id, kpt);
	}

	/*
	void Mcpt88KeypadModule::
	Keypress(const string keys, const char id, KPRESS_TYPE kpt) {

		Parent::Keypress(keys, id, kpt);
	}
	*/
								
	void Mcpt88KeypadModule::
	Keypress(const long keys, const char id, KPRESS_TYPE kpt) {

		Parent::Keypress(keys, id, kpt);	
	}				
	
	// -------------------------------------------------------------
	//
					
	void Mcpt88KeypadModule::
	Keypress(const char key, KPRESS_TYPE kpt) {

		Parent::Keypress(key, NO_ID, kpt);
	}
					
	void Mcpt88KeypadModule::
	Keypress(const char *keys, KPRESS_TYPE kpt) {

		Parent::Keypress(keys, NO_ID, kpt);
	}

	/*
	void Mcpt88KeypadModule::
	Keypress(const string keys, KPRESS_TYPE kpt) {

		Parent::Keypress(keys, NO_ID, kpt);
	}
	*/
								
	void Mcpt88KeypadModule::
	Keypress(const long keys, KPRESS_TYPE kpt) {

		Parent::Keypress(keys, NO_ID, kpt);	
	}




	
// -------------------------------------------------------------
//

  
	D1255LowerMcpt88::D1255LowerMcpt88(void)
		: Mcpt88KeypadModule() { }

	D1255LowerMcpt88::D1255LowerMcpt88( mcpt88 *cntrllr, char kpid )
		: Mcpt88KeypadModule(cntrllr, kpid)
	{ 
		Parent::AddKey(keyParam('1', 0, 1 ));
		Parent::AddKey(keyParam('2', 1, 0 ));
		Parent::AddKey(keyParam('3', 0, 0 ));
		Parent::AddKey(keyParam('4', 1, 1 ));
		Parent::AddKey(keyParam('5', 1, 4 ));
		Parent::AddKey(keyParam('6', 2, 0 ));
		Parent::AddKey(keyParam('7', 2, 1 ));
		Parent::AddKey(keyParam('8', 2, 2 ));
		Parent::AddKey(keyParam('9', 0, 3 ));		
		Parent::AddKey(keyParam('0', 1, 2 ));
		
		Parent::AddKey(keyParam('E', 0, 2 ));
		Parent::AddKey(keyParam('e', 0, 2 ));
		Parent::AddKey(keyParam('C', 1, 3 ));
		Parent::AddKey(keyParam('c', 1, 3 ));
		Parent::AddKey(keyParam('P', 2, 3 ));
		Parent::AddKey(keyParam('p', 2, 3 ));
		Parent::AddKey(keyParam('N', 0, 4 ));
		Parent::AddKey(keyParam('n', 0, 4 ));
		Parent::AddKey(keyParam('K', 2, 4 ));
		Parent::AddKey(keyParam('k', 2, 4 ));
	}	
	
	D1255LowerMcpt88::~D1255LowerMcpt88(void) { }

	

// -------------------------------------------------------------
//

  
	D1255UpperMcpt88::D1255UpperMcpt88(void) 
		: Mcpt88KeypadModule() { }

	D1255UpperMcpt88::D1255UpperMcpt88( mcpt88 *cntrllr, char kpid ) 
		: Mcpt88KeypadModule(cntrllr, kpid)
	{ 
		Parent::AddKey(keyParam('1', 4, 5 ));
		Parent::AddKey(keyParam('2', 3, 6 ));
		Parent::AddKey(keyParam('3', 3, 6 ));
		Parent::AddKey(keyParam('4', 4, 6 ));
		Parent::AddKey(keyParam('5', 7, 6 ));
		Parent::AddKey(keyParam('6', 3, 7 ));
		Parent::AddKey(keyParam('7', 4, 7 ));
		Parent::AddKey(keyParam('8', 5, 7 ));
		Parent::AddKey(keyParam('9', 6, 5 ));		
		Parent::AddKey(keyParam('0', 5, 6 ));
		
		Parent::AddKey(keyParam('E', 5, 5 ));
		Parent::AddKey(keyParam('e', 5, 5 ));
		Parent::AddKey(keyParam('C', 6, 6 ));
		Parent::AddKey(keyParam('c', 6, 6 ));
		Parent::AddKey(keyParam('P', 6, 7 ));
		Parent::AddKey(keyParam('p', 6, 7 ));
		Parent::AddKey(keyParam('N', 7, 5 ));
		Parent::AddKey(keyParam('n', 7, 5 ));
		Parent::AddKey(keyParam('K', 7, 7 ));
		Parent::AddKey(keyParam('k', 7, 7 ));
	}
	
	D1255UpperMcpt88::~D1255UpperMcpt88(void) { }



// -------------------------------------------------------------
//
// -------------------------------------------------------------


	Mem32KeypadModule::Mem32KeypadModule(void) 
		: keypadMem32(0) { }		
	
	// -------------------------------------------------------------
	//

	Mem32KeypadModule::Mem32KeypadModule( mem32 *cntrllr,  char kpdid ) 
		: keypadMem32(cntrllr, kpdid) { }
		
		
	Mem32KeypadModule::~Mem32KeypadModule(void) { }

			
	// -------------------------------------------------------------
	//
					
	void Mem32KeypadModule::
	Keypress(const char key, const char id, KPRESS_TYPE kpt) {

		Parent::Keypress(key, id, kpt);
	}
					
	void Mem32KeypadModule::
	Keypress(const char *keys, const char id, KPRESS_TYPE kpt) {

		Parent::Keypress(keys, id, kpt);
	}

	/*
	void Mem32KeypadModule::
	Keypress(const string keys, const char id, KPRESS_TYPE kpt) {

		Parent::Keypress(keys, id, kpt);
	}
	*/
								
	void Mem32KeypadModule::
	Keypress(const long keys, const char id,	KPRESS_TYPE kpt) {

		Parent::Keypress(keys, id, kpt);	
	}


	// -------------------------------------------------------------
	//
					
	void Mem32KeypadModule::
	Keypress(const char key, KPRESS_TYPE kpt) {

		Parent::Keypress(key, NO_ID, kpt);
	}
					
	void Mem32KeypadModule::
	Keypress(const char *keys, KPRESS_TYPE kpt) {

		Parent::Keypress(keys, NO_ID, kpt);
	}

	/*
	void Mem32KeypadModule::
	Keypress(const string keys, KPRESS_TYPE kpt) {

		Parent::Keypress(keys, NO_ID, kpt);
	}
	*/
								
	void Mem32KeypadModule::
	Keypress(const long keys, KPRESS_TYPE kpt) {

		Parent::Keypress(keys, NO_ID, kpt);	
	}

		
	
// -------------------------------------------------------------
//

  
	D1255LowerMem32::D1255LowerMem32(void)
		: Mem32KeypadModule() { }

	D1255LowerMem32::D1255LowerMem32( mem32 *cntrllr, char kpid )
		: Mem32KeypadModule(cntrllr, kpid)
	{ 
		Parent::AddKey(keyParam('1', 0, 0 ));
		Parent::AddKey(keyParam('2', 1, 0 ));
		Parent::AddKey(keyParam('3', 2, 0 ));
		Parent::AddKey(keyParam('4', 3, 0 ));
		Parent::AddKey(keyParam('5', 4, 0 ));
		Parent::AddKey(keyParam('6', 5, 0 ));
		Parent::AddKey(keyParam('7', 6, 0 ));
		Parent::AddKey(keyParam('8', 7, 0 ));
		Parent::AddKey(keyParam('9', 0, 1 ));		
		Parent::AddKey(keyParam('0', 1, 1 ));
		
		Parent::AddKey(keyParam('E', 2, 1 ));
		Parent::AddKey(keyParam('e', 2, 1 ));
		Parent::AddKey(keyParam('C', 3, 1 ));
		Parent::AddKey(keyParam('c', 3, 1 ));
		Parent::AddKey(keyParam('P', 4, 1 ));
		Parent::AddKey(keyParam('p', 4, 1 ));
		Parent::AddKey(keyParam('N', 5, 1 ));
		Parent::AddKey(keyParam('n', 5, 1 ));
		Parent::AddKey(keyParam('K', 6, 1 ));
		Parent::AddKey(keyParam('k', 6, 1 ));

		// Relay 7, Bank 1 Not used
	}	
	
	D1255LowerMem32::~D1255LowerMem32(void) { }

	

// -------------------------------------------------------------
//

  
	D1255UpperMem32::D1255UpperMem32(void) 
		: Mem32KeypadModule() { }

	D1255UpperMem32::D1255UpperMem32( mem32 *cntrllr, char kpid ) 
		: Mem32KeypadModule(cntrllr, kpid)
	{ 
		Parent::AddKey(keyParam('1', 0, 2 ));
		Parent::AddKey(keyParam('2', 1, 2 ));
		Parent::AddKey(keyParam('3', 2, 2 ));
		Parent::AddKey(keyParam('4', 3, 2 ));
		Parent::AddKey(keyParam('5', 4, 2 ));
		Parent::AddKey(keyParam('6', 5, 2 ));
		Parent::AddKey(keyParam('7', 6, 2 ));
		Parent::AddKey(keyParam('8', 7, 2 ));
		Parent::AddKey(keyParam('9', 0, 3 ));		
		Parent::AddKey(keyParam('0', 1, 3 ));
		
		Parent::AddKey(keyParam('E', 2, 3 ));
		Parent::AddKey(keyParam('e', 2, 3 ));
		Parent::AddKey(keyParam('C', 3, 3 ));
		Parent::AddKey(keyParam('c', 3, 3 ));
		Parent::AddKey(keyParam('P', 4, 3 ));
		Parent::AddKey(keyParam('p', 4, 3 ));
		Parent::AddKey(keyParam('N', 5, 3 ));
		Parent::AddKey(keyParam('n', 5, 3 ));
		Parent::AddKey(keyParam('K', 6, 3 ));
		Parent::AddKey(keyParam('k', 6, 3 ));

		// Relay 7, Bank 3 Not used
	}		
	
	D1255UpperMem32::~D1255UpperMem32(void) { }

		
// -------------------------------------------------------------
//

  
	D202LowerMem32::D202LowerMem32(void)
		: Mem32KeypadModule() { }

	D202LowerMem32::D202LowerMem32( mem32 *cntrllr, char kpid )
		: Mem32KeypadModule(cntrllr, kpid)
	{		
		Parent::AddKey(keyParam('0', 0, 0 ));
		Parent::AddKey(keyParam('1', 1, 0 ));
		Parent::AddKey(keyParam('2', 2, 0 ));
		Parent::AddKey(keyParam('3', 3, 0 ));
		Parent::AddKey(keyParam('4', 4, 0 ));
		Parent::AddKey(keyParam('5', 5, 0 ));
		Parent::AddKey(keyParam('6', 6, 0 ));
		Parent::AddKey(keyParam('7', 7, 0 ));
		Parent::AddKey(keyParam('8', 0, 1 ));
		Parent::AddKey(keyParam('9', 1, 1 ));		
												
		Parent::AddKey(keyParam('K', 2, 1 ));
		Parent::AddKey(keyParam('k', 2, 1 ));
		Parent::AddKey(keyParam('A', 3, 1 ));
		Parent::AddKey(keyParam('a', 3, 1 ));
		Parent::AddKey(keyParam('B', 4, 1 ));
		Parent::AddKey(keyParam('b', 4, 1 ));
		Parent::AddKey(keyParam('C', 5, 1 ));
		Parent::AddKey(keyParam('c', 5, 1 ));

		// Relay 6, Bank 1 Not used
		// Relay 7, Bank 1 Not used
	}
	
	D202LowerMem32::~D202LowerMem32(void) { }

	

// -------------------------------------------------------------
//

  
	D202UpperMem32::D202UpperMem32(void) 
		: Mem32KeypadModule() { }

	D202UpperMem32::D202UpperMem32( mem32 *cntrllr, char kpid ) 
		: Mem32KeypadModule(cntrllr, kpid)
	{ 
		Parent::AddKey(keyParam('0', 0, 2 ));
		Parent::AddKey(keyParam('1', 1, 2 ));
		Parent::AddKey(keyParam('2', 2, 2 ));
		Parent::AddKey(keyParam('3', 3, 2 ));
		Parent::AddKey(keyParam('4', 4, 2 ));
		Parent::AddKey(keyParam('5', 5, 2 ));
		Parent::AddKey(keyParam('6', 6, 2 ));
		Parent::AddKey(keyParam('7', 7, 2 ));
		Parent::AddKey(keyParam('8', 0, 3 ));
		Parent::AddKey(keyParam('9', 1, 3 ));		
												
		Parent::AddKey(keyParam('K', 2, 3 ));
		Parent::AddKey(keyParam('k', 2, 3 ));
		Parent::AddKey(keyParam('A', 3, 3 ));
		Parent::AddKey(keyParam('a', 3, 3 ));
		Parent::AddKey(keyParam('B', 4, 3 ));
		Parent::AddKey(keyParam('b', 4, 3 ));
		Parent::AddKey(keyParam('C', 5, 3 ));
		Parent::AddKey(keyParam('c', 5, 3 ));

		// Relay 6, Bank 1 Not used
		// Relay 7, Bank 1 Not used
	}		
	
	D202UpperMem32::~D202UpperMem32(void) { }


	
// -------------------------------------------------------------
//              
  
	D222LowerMem32::D222LowerMem32(void)
		: Mem32KeypadModule() { }

	D222LowerMem32::D222LowerMem32( mem32 *cntrllr, char kpid )
		: Mem32KeypadModule(cntrllr, kpid)
	{ 
		Parent::AddKey(keyParam('1', 0, 0 ));
		Parent::AddKey(keyParam('2', 1, 0 ));
		Parent::AddKey(keyParam('3', 2, 0 ));
		Parent::AddKey(keyParam('4', 3, 0 ));
		Parent::AddKey(keyParam('5', 4, 0 ));
		Parent::AddKey(keyParam('6', 5, 0 ));
		Parent::AddKey(keyParam('7', 6, 0 ));
		Parent::AddKey(keyParam('8', 7, 0 ));
		Parent::AddKey(keyParam('9', 0, 1 ));		
		Parent::AddKey(keyParam('0', 1, 1 ));
		
		Parent::AddKey(keyParam('*', 2, 1 ));
		Parent::AddKey(keyParam('K', 3, 1 ));
		Parent::AddKey(keyParam('k', 3, 1 ));
		Parent::AddKey(keyParam('B', 4, 1 ));
		Parent::AddKey(keyParam('b', 4, 1 ));
		Parent::AddKey(keyParam('C', 5, 1 ));
		Parent::AddKey(keyParam('c', 5, 1 ));
		Parent::AddKey(keyParam('A', 6, 1 ));
		Parent::AddKey(keyParam('a', 6, 1 ));

		// Relay 7, Bank 1 Not used
	}	                          	
	
	D222LowerMem32::~D222LowerMem32(void) { }

	

// -------------------------------------------------------------
//

  
	D222UpperMem32::D222UpperMem32(void) 
		: Mem32KeypadModule() { }

	D222UpperMem32::D222UpperMem32( mem32 *cntrllr, char kpid ) 
		: Mem32KeypadModule(cntrllr, kpid)
	{ 
		Parent::AddKey(keyParam('1', 0, 2 ));
		Parent::AddKey(keyParam('2', 1, 2 ));
		Parent::AddKey(keyParam('3', 2, 2 ));
		Parent::AddKey(keyParam('4', 3, 2 ));
		Parent::AddKey(keyParam('5', 4, 2 ));
		Parent::AddKey(keyParam('6', 5, 2 ));
		Parent::AddKey(keyParam('7', 6, 2 ));
		Parent::AddKey(keyParam('8', 7, 2 ));
		Parent::AddKey(keyParam('9', 0, 3 ));		
		Parent::AddKey(keyParam('0', 1, 3 ));
		
		Parent::AddKey(keyParam('*', 2, 3 ));	// Bottom Right Key
		Parent::AddKey(keyParam('K', 3, 3 )); 	// Clear Key
		Parent::AddKey(keyParam('k', 3, 3 ));
		Parent::AddKey(keyParam('B', 4, 3 ));	// Bottom Left Key + 1
		Parent::AddKey(keyParam('b', 4, 3 ));
		Parent::AddKey(keyParam('C', 5, 3 ));  	// Bottom Left Key + 2
		Parent::AddKey(keyParam('c', 5, 3 ));
		Parent::AddKey(keyParam('A', 6, 3 ));	// Bottom Left Key
		Parent::AddKey(keyParam('a', 6, 3 ));

		// Relay 7, Bank 3 Not used
	}
		
	
	D222UpperMem32::~D222UpperMem32(void) { }


