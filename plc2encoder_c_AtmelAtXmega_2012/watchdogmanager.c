





//
//
//
//  Author :    Paul Calinawan
//
//  Date:       Feb 4 , 2012
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

#include <avr/wdt.h>

#include "itechsys.h"


////////////////////////////////////////////////////////////////////


#include "kernel.h"

#include "watchdogmanager.h"


////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////

U8  commOK;
U8  encoderMoving;

U8	monitorWatchdog;

U8	watchDogCycle;

//
//
//

U8  IsCommOK(void)
{
	return commOK;
}

U8  IsEncoderMoving(void)
{
	return encoderMoving;
}

////////////////////////////////////////////////////////////////////
//
// Force Watchdog Reset to Kick and restart MCU
//
////////////////////////////////////////////////////////////////////

void	DoForcedWatchDogReset(void)
{
	for(;;);
}

///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////

void	ResetWatchDog(void)
{
		wdt_reset();
}

////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////

void    Init_WatchdogManager(void)
{
		ResetWatchDog();
		
		commOK = TRUE;
		encoderMoving = TRUE;	
		
		monitorWatchdog = FALSE;
		
		watchDogCycle = 0;
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

NEW_STATE   WDM_exitA(void)
{
		ResetWatchDog();
	
		StartTimer(WATCHDOG_CLEAR_FREQUENCY);

    return WDM_ACTIVE;
}


////////////////////////////////////////////////////////////////////
//
// TimeOut while in WDM_ACTIVE
//
////////////////////////////////////////////////////////////////////

#define MAX_WATCHDOG_CYCLE_WAIT		(50)

NEW_STATE   WDM_exitB(void)
{
	watchDogCycle++;
	
		if(watchDogCycle > 100)
		{
			watchDogCycle = 0;
			
			if(monitorWatchdog)
				if(IsCommOK() || IsEncoderMoving())
				{
					commOK = FALSE;
					encoderMoving = FALSE;
						
					ResetWatchDog();		
				}	
				else
				{
					DoForcedWatchDogReset();
				}		
		}
		else
		{				
			ResetWatchDog();		
		}
		
		StartTimer(WATCHDOG_CLEAR_FREQUENCY);	
			
	return SAME_STATE;			
}


////////////////////////////////////////////////////////////////////
//
// CommOkWatchDogReset while in WDM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   WDM_exitC(void)
{
		commOK = TRUE;

    return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
// EncMovingWatchdogReset while in WDM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   WDM_exitD(void)
{
		encoderMoving = TRUE;

    return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
// BeginWatchdogMonitor while in WDM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   WDM_exitE(void)
{
		monitorWatchdog = TRUE;

    return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
// EndWatchdogMonitor while in WDM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   WDM_exitF(void)
{
		monitorWatchdog = FALSE;

    return SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_WDM_IDLE)
EV_HANDLER(GoActive, WDM_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_WDM_ACTIVE)
EV_HANDLER(TimeOut, WDM_exitB),
EV_HANDLER(CommOkWatchDogReset, WDM_exitC),
EV_HANDLER(EncMovingWatchdogReset, WDM_exitD),
EV_HANDLER(BeginWatchdogMonitor, WDM_exitE),
EV_HANDLER(EndWatchDogMonitor, WDM_exitF)
STATE_TRANSITION_MATRIX_END;

// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(WDM_Main_Entry)
	STATE(_WDM_IDLE)						,
	STATE(_WDM_ACTIVE)     
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////

