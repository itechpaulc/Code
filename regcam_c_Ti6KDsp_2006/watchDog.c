




//
//
//
//  Author :    Paul Calinawan
//
//  Date:       February 2, 2006
//
//  Copyrights: Imaging Technologies Inc.
//
//  Product:    Intelli-Ribbon Control
//  
//  Subsystem:  Camera System
//  -------------------------------------------
//
//
//      CONFIDENTIAL DOCUMENT
//
//      Property of Imaging Technologies Inc.
//
//



////////////////////////////////////////////////////////////////////

#include <vcrt.h>
#include <vclib.h>
#include <macros.h>
#include <sysvar.h>


////////////////////////////////////////////////////////////////////


#include "itechsys.h"


////////////////////////////////////////////////////////////////////


#include "kernel.h"


////////////////////////////////////////////////////////////////////


#include "watchdog.h"

#include "spicomm.h"



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
// Initialize Machine
//
////////////////////////////////////////////////////////////////////

void    Init_WDTHandlerMachine(void)
{
	SendMessage(WatchDog_Manager, GoActive );
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

NEW_STATE   WDT_exitA(void)
{
 //    print("WDT_exitA !!!!!!!!!!!!!!!!\n");
    StartTimer(SECONDS(2));
    return WDT_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// Watch Dog Reset
//
////////////////////////////////////////////////////////////////////

NEW_STATE   WDT_exitB(void)
{
	SPI_ISSUE_RESET_WATCHDOG();
//    print("WDT_exitB !!!!!!!!!!!!!!!!\n");
    StartTimer(SECONDS(2));
    return SAME_STATE;
}

NEW_STATE   WDT_exitC(void)
{
    DWORD   extendTO = GetMessageData1();

    CancelTimer();
//    print("WDT_exitC !!!!!!!!!!!!!!!!\n");

    if ( extendTO )
        SPI_ISSUE_RESET_WATCHDOG();

    return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_WDT_IDLE)
    EV_HANDLER(GoActive, WDT_exitA)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_WDT_ACTIVE)
    EV_HANDLER(TimeOut,  WDT_exitB),
    EV_HANDLER(ResetWDT, WDT_exitC)
STATE_TRANSITION_MATRIX_END;


// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(WDT_Main_Entry)
    STATE(_WDT_IDLE)            ,           
    STATE(_WDT_ACTIVE)   
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////


