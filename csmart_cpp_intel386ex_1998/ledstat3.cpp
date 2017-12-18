



/////////////////////////////////////////////////////////////////////////////
//
//
//    $Header:      $
//    $Log:         $
//
//
//    Author : Paul Calinawan        December 1997
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



#include "80386ex.h"

#include "ledstat.h"




//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  MotorCommLedStatusMachine
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// MotorCommLedStatusMachine - private helper functions
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

void
MotorCommLedStatusMachine::LedOn(void)
{
    p3Status = GetIO3Latch();           // Read Port 3

    p3Status |= MTRC_STATUS_LED_PORT;   // Set LED Port

    SetIO3Latch(p3Status);              // Write to Port
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MotorCommLedStatusMachine::LedOff(void)
{
    p3Status = GetIO3Latch();           // Read Port 3

    p3Status &= ~MTRC_STATUS_LED_PORT;  // Clear LED Port

    SetIO3Latch(p3Status);              // Write to Port
}


//////////////////////////////////////////////////
//
// MotorCommLedStatusMachine - RESPONSE ENTRIES
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
// State Transition Matrices
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(MotorCommLedStatusMachine, _MTRC_SLM_IDLE)
    EV_HANDLER(TimeOut, MTRC_SLM_h1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(MotorCommLedStatusMachine, _MTRC_SLM_ON)
    EV_HANDLER(TimeOut, MTRC_SLM_h1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(MotorCommLedStatusMachine, _MTRC_SLM_OFF)
    EV_HANDLER(TimeOut, MTRC_SLM_h2)
STATE_TRANSITION_MATRIX_END;


//////////////////////////////////////////////////
//
// Matrix Table
//
//////////////////////////////////////////////////

DEFINE_RESPONSE_TABLE_ENTRY(MotorCommLedStatusMachine)
    STATE_MATRIX_ENTRY(_MTRC_SLM_IDLE),
    STATE_MATRIX_ENTRY(_MTRC_SLM_ON),
    STATE_MATRIX_ENTRY(_MTRC_SLM_OFF)
RESPONSE_TABLE_END;


//////////////////////////////////////////////////
//
// Static Member Definitions
//
//////////////////////////////////////////////////

BYTE    MotorCommLedStatusMachine::p3Status;


//////////////////////////////////////////////////
//
// MotorCommLedStatusMachine - Constructors, Destructors
//
//////////////////////////////////////////////////

MotorCommLedStatusMachine::MotorCommLedStatusMachine(BYTE sMsysID)
    :StateMachine(sMsysID)
{
    LedOff();

    ASSIGN_RESPONSE_TABLE();

    SetCurrState(MTRC_SLM_IDLE);
}

MotorCommLedStatusMachine::~MotorCommLedStatusMachine(void) { }


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// MotorCommLedStatusMachine - private EXIT PROCEDURES
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

WORD
MotorCommLedStatusMachine::MTRC_SLM_h1(void)
{
        LedOff();

        StartTimer(MSEC(50));

    return  MTRC_SLM_OFF;
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
MotorCommLedStatusMachine::MTRC_SLM_h2(void)
{
        LedOn();

        StartTimer(MSEC(50));

    return  MTRC_SLM_ON;
}

