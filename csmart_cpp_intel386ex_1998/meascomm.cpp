
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




#include "meascomm.h"

#include "80386ex.h"


#include <string.h>

#pragma _builtin_(memcpy)



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  MeasurementCommMachine
//
//      - public interface functions :
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

SCAN_MEASUREMENT_DATA &
MeasurementCommMachine::GetScanMeasurementData(void)
{
    return currScanMeasurementData;
}


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// MeasurementCommMachine - private helper functions
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MeasurementCommMachine::enableMeasurementReception(void)
{
    // Enable the Kernel's SSIO/DMA functions

    EnableMeasurementDataReception();
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MeasurementCommMachine::disableMeasurementReception(void)
{
    // Disable the Kernel's SSIO/DMA functions

    DisableMeasurementDataReception();
}



//////////////////////////////////////////////////
//
// MeasurementCommMachine - RESPONSE ENTRIES
//
//////////////////////////////////////////////////



//////////////////////////////////////////////////
//
// State Transition Matrices
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(MeasurementCommMachine, _MCM_IDLE)
    EV_HANDLER(WaitForMeasurementBlock, MCM_h1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(MeasurementCommMachine, _MCM_WAITING_FOR_MEASUREMENT_BLOCK)
    EV_HANDLER(MeasurementBlockReceived, MCM_h2)
STATE_TRANSITION_MATRIX_END;



//////////////////////////////////////////////////
//
// Matrix Table
//
//////////////////////////////////////////////////

DEFINE_RESPONSE_TABLE_ENTRY(MeasurementCommMachine)
    STATE_MATRIX_ENTRY(_MCM_IDLE),
    STATE_MATRIX_ENTRY(_MCM_WAITING_FOR_MEASUREMENT_BLOCK),
RESPONSE_TABLE_END;



//////////////////////////////////////////////////
//
// Static Member Definitions
//
//////////////////////////////////////////////////

SCAN_MEASUREMENT_DATA
MeasurementCommMachine::currScanMeasurementData;

WORD    MeasurementCommMachine::errorCount = 0;



//////////////////////////////////////////////////
//
// MeasurementCommMachine - Constructors, Destructors
//
//////////////////////////////////////////////////

MeasurementCommMachine::MeasurementCommMachine(STATE_MACHINE_ID sMsysID)
    :StateMachine(sMsysID)
{
    ASSIGN_RESPONSE_TABLE();

    SetCurrState(MCM_IDLE);
}

MeasurementCommMachine::~MeasurementCommMachine(void) { }


WORD    MeasurementCommMachine::GetErrorCount(void) {

    return  MeasurementCommMachine::errorCount;
}



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// MeasurementCommMachine - private EXIT PROCEDURES
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
// Message : Wait For Measurement Block
//
//////////////////////////////////////////////////

WORD
MeasurementCommMachine::MCM_h1(void)
{
        enableMeasurementReception();

    return  MCM_WAITING_FOR_MEASUREMENT_BLOCK;
}

//////////////////////////////////////////////////
//
// Message : Measurement Block Received
//
//////////////////////////////////////////////////

WORD
MeasurementCommMachine::MCM_h2(void)
{
        disableMeasurementReception();

        // Create a new pointer (casted), pointing to
        // the data just received by the Kernel

        SCAN_MEASUREMENT_DATA *ptrKernelMeasurementData =
            (SCAN_MEASUREMENT_DATA *)GetMeasurementBuffer();

        // Update MCM's local copy using the measurement
        // the data received by the Kernel

        memcpy(&currScanMeasurementData, ptrKernelMeasurementData,
               sizeof(SCAN_MEASUREMENT_DATA));

        // Tell HMM that there is new measurement data

        SendHiPrMsg(HeadMachinesManagerID, MeasurementBlockReceived);

    return  MCM_IDLE;
}





