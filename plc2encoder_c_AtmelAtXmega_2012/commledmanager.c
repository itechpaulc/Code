





//
//
//
//  Author :    Paul Calinawan
//
//  Date:       Jan 14 , 2012
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




////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////

#include "itechsys.h"


////////////////////////////////////////////////////////////////////


#include "kernel.h"


////////////////////////////////////////////////////////////////////


#include <avr/io.h>

#include "commledmanager.h"


////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

//
//

void CommTxON(void)
{
	PORTD |= COMM_TX_LED_MASK;
}

void CommTxOFF(void)
{
	PORTD &= ~(COMM_TX_LED_MASK);
}

void CommRxON(void)
{
	PORTD |= COMM_RX_LED_MASK;
}

void CommRxOFF(void)
{
	PORTD &= ~(COMM_RX_LED_MASK);
}



////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////

void    Init_CommLedManager(void)
{
		CommTxOFF();
		CommRxOFF();
}

////////////////////////////////////////////////////////////////////
//
// Exit Procedures
//
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//
// GoActive
//
////////////////////////////////////////////////////////////////////

#define		COMM_RECEIVE_MSG_CAD			(0x5555)
#define		COMM_TRANSMIT_MSG_CAD			(0x5555)

#define		COMM_RECEIVE_MSG_ACTIVITY_CAD	(0x0001)

#define		COMM_LED_IDLE					(0x0000)

U16		rxLedCurrCadence;
U16		txLedCurrCadence;

#define		COMM_LED_INIT_MASK				(0x01)

U16		rxLedCurrMask;
U16		txLedCurrMask;

NEW_STATE   CLM_exitA(void)
{
		StartTimer(COMM_LED_UPDATE_RATE);		
		
		rxLedCurrCadence = COMM_LED_IDLE;
		txLedCurrCadence = COMM_LED_IDLE;
			
		rxLedCurrMask = COMM_LED_INIT_MASK;
		txLedCurrMask = COMM_LED_INIT_MASK;
			
    return CLM_ACTIVE;
}


////////////////////////////////////////////////////////////////////
//
// TimeOut while in CLM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   CLM_exitB(void)
{
		StartTimer(COMM_LED_UPDATE_RATE);
		
		(rxLedCurrCadence & rxLedCurrMask) ? CommRxON() : CommRxOFF();			
		(txLedCurrCadence & txLedCurrMask) ? CommTxON() : CommTxOFF();
		
		rxLedCurrMask = (rxLedCurrMask << 1);
		txLedCurrMask = (txLedCurrMask << 1);
		
		if(rxLedCurrMask == 0)
		{
			rxLedCurrMask = COMM_LED_INIT_MASK;
			rxLedCurrCadence = COMM_LED_IDLE;
		}
				
		if(txLedCurrMask == 0)
		{
			txLedCurrMask = COMM_LED_INIT_MASK;
			txLedCurrCadence = COMM_LED_IDLE;
		}
								
    return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
// PulseTxLed 
//		while in CLM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   CLM_exitC(void)
{
		StartTimer(COMM_LED_UPDATE_RATE);
		
		if(txLedCurrCadence == COMM_LED_IDLE)
		{
			txLedCurrCadence = COMM_RECEIVE_MSG_ACTIVITY_CAD;

			txLedCurrMask = COMM_LED_INIT_MASK;				
		}		
				
    return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
// PulseRxLed 
//		while in CLM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   CLM_exitD(void)
{
		StartTimer(COMM_LED_UPDATE_RATE);
		
		if(rxLedCurrCadence == COMM_LED_IDLE)
		{
			rxLedCurrCadence = COMM_RECEIVE_MSG_ACTIVITY_CAD;

			rxLedCurrMask = COMM_LED_INIT_MASK;
		}
				
    return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
// FlashTxLed while in CLM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   CLM_exitE(void)
{
		StartTimer(COMM_LED_UPDATE_RATE);

		if(txLedCurrCadence == COMM_LED_IDLE)
		{				
			txLedCurrCadence = COMM_TRANSMIT_MSG_CAD;
		
			txLedCurrMask = COMM_LED_INIT_MASK;					
		}
				
    return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
// FlashRxLed while in CLM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   CLM_exitF(void)
{
		StartTimer(COMM_LED_UPDATE_RATE);
		
		if(rxLedCurrCadence == COMM_LED_IDLE)
		{				
			rxLedCurrCadence = COMM_RECEIVE_MSG_CAD;
		
			rxLedCurrMask = COMM_LED_INIT_MASK;
		}
							
    return SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_CLM_IDLE)
EV_HANDLER(GoActive, CLM_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_CLM_ACTIVE)
EV_HANDLER(TimeOut, CLM_exitB),
EV_HANDLER(PulseTxLed, CLM_exitC),
EV_HANDLER(PulseRxLed, CLM_exitD),
EV_HANDLER(FlashTxLed, CLM_exitE),
EV_HANDLER(FlashRxLed, CLM_exitF)
STATE_TRANSITION_MATRIX_END;

// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(CLM_Main_Entry)
	STATE(_CLM_IDLE)						,
	STATE(_CLM_ACTIVE)     
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////


