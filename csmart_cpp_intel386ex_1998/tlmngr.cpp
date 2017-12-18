




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



#include "tlmngr.h"




//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  TargetLampManager
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  TargetLampManager
//
//      - public interface functions :
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////




//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// TargetLampManager - private helper functions
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////




//////////////////////////////////////////////////
//
// TargetLampManager - RESPONSE ENTRIES
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
// State Transition Matrices
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(TargetLampManager, _TLM_IDLE)
    EV_HANDLER(TargetLampOn, TLM_h1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(TargetLampManager, _TLM_WAIT_FOR_TARGET_LAMP_ON)
    EV_HANDLER(TargetLampIsOn, TLM_h2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(TargetLampManager, _TLM_WAIT_FOR_TARGET_LAMP_TIMER_TO_EXPIRE)
    EV_HANDLER(TargetLampOn, TLM_h3),
    EV_HANDLER(TimeOut, TLM_h4),
    EV_HANDLER(TargetLampOff, TLM_h5)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(TargetLampManager, _TLM_WAIT_FOR_CSM_RESPONSE)
    EV_HANDLER(TargetLampOn, TLM_h3),
    EV_HANDLER(TargetLampOff, TLM_h5)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(TargetLampManager, _TLM_WAIT_FOR_TARGET_LAMP_OFF)
    EV_HANDLER(TargetLampIsOff, TLM_h6)
STATE_TRANSITION_MATRIX_END;



//////////////////////////////////////////////////
//
// Matrix Table
//
//////////////////////////////////////////////////

DEFINE_RESPONSE_TABLE_ENTRY(TargetLampManager)
    STATE_MATRIX_ENTRY(_TLM_IDLE),
    STATE_MATRIX_ENTRY(_TLM_WAIT_FOR_TARGET_LAMP_ON),
    STATE_MATRIX_ENTRY(_TLM_WAIT_FOR_TARGET_LAMP_TIMER_TO_EXPIRE),
    STATE_MATRIX_ENTRY(_TLM_WAIT_FOR_CSM_RESPONSE),
    STATE_MATRIX_ENTRY(_TLM_WAIT_FOR_TARGET_LAMP_OFF)
RESPONSE_TABLE_END;


//////////////////////////////////////////////////
//
// Static Member Definitions
//
//////////////////////////////////////////////////

WORD    TargetLampManager::requestedLampOnTime = 0x0000;



//////////////////////////////////////////////////
//
// TargetLampManager - Constructors, Destructors
//
//////////////////////////////////////////////////

TargetLampManager::TargetLampManager(STATE_MACHINE_ID sMsysID)
    :StateMachine(sMsysID)
{
    ASSIGN_RESPONSE_TABLE();

    SetCurrState(TLM_IDLE);
}

TargetLampManager::~TargetLampManager(void) { }


WORD
TargetLampManager::GetErrorCount(void) {

    return 0;
}


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// TargetLampManager - private EXIT PROCEDURES
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
// Message: Target Lamp On
//
//////////////////////////////////////////////////

WORD
TargetLampManager::TLM_h1(void) {

        // Get data from message

        requestedLampOnTime =
            TARGET_LAMP_TIME_UNITS((WORD)GetCurrEvent().msgData1);

        // relay message to HMM

        SendHiPrMsg(HeadMachinesManagerID, TargetLampOn);

    return  TLM_WAIT_FOR_TARGET_LAMP_ON;
}


//////////////////////////////////////////////////
//
// Message: Target Lamp Is On
//
//////////////////////////////////////////////////

WORD
TargetLampManager::TLM_h2(void) {

        // relay message to CSM

        SendHiPrMsg(ColorSmartManagerID, TargetLampIsOn);

        // Start timer for automatic Target Lamp shut off

        StartHiPriorityTimer(requestedLampOnTime);

    return  TLM_WAIT_FOR_TARGET_LAMP_TIMER_TO_EXPIRE;
}


//////////////////////////////////////////////////
//
// Message: Target Lamp On
//
//////////////////////////////////////////////////

WORD
TargetLampManager::TLM_h3(void) {

        // maybe a new timer value ? Get data from message

        requestedLampOnTime =
            TARGET_LAMP_TIME_UNITS((WORD)GetCurrEvent().msgData1);

        // tell CSM Target Lamp is still On

        SendHiPrMsg(ColorSmartManagerID, TargetLampIsOn);

        // just Re Start timer

        StartHiPriorityTimer(requestedLampOnTime);

    return  TLM_WAIT_FOR_TARGET_LAMP_TIMER_TO_EXPIRE;
}


//////////////////////////////////////////////////
//
//  Message: Time Out
//
//////////////////////////////////////////////////

WORD
TargetLampManager::TLM_h4(void) {

        // Auto shut off tell CSM Target Lamp Time Out has occured

        SendHiPrMsg(ColorSmartManagerID, TargetLampTimedOut);

    return  TLM_WAIT_FOR_CSM_RESPONSE;
}


//////////////////////////////////////////////////
//
//  Message: Target Lamp Off
//
//////////////////////////////////////////////////

WORD
TargetLampManager::TLM_h5(void) {

        // Tell HMM to turn Target Lamp Off

        SendHiPrMsg(HeadMachinesManagerID, TargetLampOff);

    return  TLM_WAIT_FOR_TARGET_LAMP_OFF;
}


//////////////////////////////////////////////////
//
// Message: Target Lamp Is Off
//
//////////////////////////////////////////////////

WORD
TargetLampManager::TLM_h6(void) {

        // Tell CSM that the lamp is now off

        SendHiPrMsg(ColorSmartManagerID, TargetLampIsOff);

    return  TLM_IDLE;
}




