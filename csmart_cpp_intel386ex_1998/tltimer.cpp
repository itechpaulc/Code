




/////////////////////////////////////////////////////////////////////////////
//
//
//    $Header:      $
//    $Log:         $
//
//
//    Author : Paul Calinawan        January 1998
//
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//    NOTE:
//
//    This document contains CONFIDENTIAL and proprietary information
//    which is the property of Graphics Microsystems, Inc. It may not
//    be copied or transmitted in whole or in part by any means to any
//    media without Graphics Microsystems Inc's prior written permission.
//
/////////////////////////////////////////////////////////////////////////////



#include "tltimer.h"



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  TargetLampTimer
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  TargetLampTimer
//
//      - public interface functions :
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
TargetLampTimer::SetTargetLampTimeOut(WORD tLampTimeOut)
{
    targetLampTimeOut = tLampTimeOut;
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL
TargetLampTimer::IsTargetLampTimerExpired(void)
{
    return  targetLampTimerExpired;
}


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// TargetLampTimer - private helper functions
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////




//////////////////////////////////////////////////
//
// TargetLampTimer - RESPONSE ENTRIES
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
// State Transition Matrices
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(TargetLampTimer, _TLT_IDLE)
    EV_HANDLER(StartTargetLampTimer, TLT_h1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(TargetLampTimer, _TLT_WAITING_FOR_TARGET_LAMP_TIMER_TO_EXPIRE)
    EV_HANDLER(StartTargetLampTimer, TLT_h1),
    EV_HANDLER(TimeOut, TLT_h2),
    EV_HANDLER(CancelTargetLampTimer, TLT_h3)
STATE_TRANSITION_MATRIX_END;



//////////////////////////////////////////////////
//
// Matrix Table
//
//////////////////////////////////////////////////

DEFINE_RESPONSE_TABLE_ENTRY(TargetLampTimer)
    STATE_MATRIX_ENTRY(_TLT_IDLE),
    STATE_MATRIX_ENTRY(_TLT_WAITING_FOR_TARGET_LAMP_TIMER_TO_EXPIRE)
RESPONSE_TABLE_END;


//////////////////////////////////////////////////
//
// Static Member Definitions
//
//////////////////////////////////////////////////

WORD    TargetLampTimer::targetLampTimeOut = 0x0000;

BOOL    TargetLampTimer::targetLampTimerExpired = TRUE;



//////////////////////////////////////////////////
//
// TargetLampTimer - Constructors, Destructors
//
//////////////////////////////////////////////////

TargetLampTimer::TargetLampTimer(BYTE sMsysID)
    :StateMachine(sMsysID)
{
    ASSIGN_RESPONSE_TABLE();

    SetCurrState(TLT_IDLE);
}

TargetLampTimer::~TargetLampTimer(void) { }


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// TargetLampTimer - private EXIT PROCEDURES
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

WORD
TargetLampTimer::TLT_h1(void) {

        // Make sure timeout has value

        if(targetLampTimeOut != 0) {

                StartTimer(SECONDS(targetLampTimeOut));

                targetLampTimerExpired = FALSE;

            return  TLT_WAITING_FOR_TARGET_LAMP_TIMER_TO_EXPIRE;
        }

        targetLampTimerExpired = TRUE;

        //
        // Send Expired message to CSM
        //

    return  TLT_IDLE;
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TargetLampTimer::TLT_h2(void) {

        targetLampTimerExpired = TRUE;

        //
        // Send Expired message to CSM
        //

    return  TLT_IDLE;
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TargetLampTimer::TLT_h3(void) {

        targetLampTimerExpired = TRUE;

        CancelTimer();

    return  TLT_IDLE;
}

