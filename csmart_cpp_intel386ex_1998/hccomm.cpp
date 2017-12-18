

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



#include "80386ex.h"

#include "hccomm.h"


#include <string.h>

#pragma _builtin_(memcpy)



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  HeadCommandCommMachine
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  HeadCommandCommMachine
//
//      - public interface functions :
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

void
HeadCommandCommMachine::SetTransmitBuffer(PROBE_HEAD_COMMAND phCmd, BYTE *txData, BYTE dataLength)
{
    txChecksum = 0x00;

        // Set Command Byte

        hccTxBuffer[COMMAND_SLOT] = (BYTE)phCmd;
        txChecksum += hccTxBuffer[COMMAND_SLOT];

        // Set Data Bytes

        for(int d=DATA_START_SLOT; d<dataLength; d++)
        {
            hccTxBuffer[d] = (*txData);

            txChecksum += hccTxBuffer[d];

            txData++; // Next
        }

        // Set Length Byte

        hccTxBuffer[LENGTH_SLOT] = dataLength + HEADER_TRAILER_LENGTH;
        txChecksum += hccTxBuffer[LENGTH_SLOT];

        // Set Checksum Byte

        hccTxBuffer[d] = txChecksum;
}

BYTE    *
HeadCommandCommMachine::GetReceiveBuffer(void)
{
    return hccRxBuffer;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadCommandCommMachine::LinkAll(TableParametersDataManager *pTPDM) {

    LinkTableParametersDataManager(pTPDM);
}



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// HeadCommandCommMachine - private helper functions
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadCommandCommMachine::
LinkTableParametersDataManager(TableParametersDataManager *pTPDM) {

    ptrTPDM = pTPDM;
}


//////////////////////////////////////////////////
//
// HeadCommandCommMachine - RESPONSE ENTRIES
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
// State Transition Matrices
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(HeadCommandCommMachine, _HCC_IDLE)
    EV_HANDLER(SendProbeHeadCommand, HCC_h1),
    EV_HANDLER(StartContinuousScanMode, HCC_h4)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadCommandCommMachine, _HCC_SENDING_PACKET)
    EV_HANDLER(PacketSent, HCC_h2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadCommandCommMachine, _HCC_WAITING_FOR_REPLY)
    EV_HANDLER(PacketReceived, HCC_h3),
    EV_HANDLER(TimeOut, HCC_h3a)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadCommandCommMachine, _HCC_IN_CONTINUOUS_SCAN_MODE)
    EV_HANDLER(EndContinuousScanMode, HCC_h5)
STATE_TRANSITION_MATRIX_END;




//////////////////////////////////////////////////
//
// Matrix Table
//
//////////////////////////////////////////////////

DEFINE_RESPONSE_TABLE_ENTRY(HeadCommandCommMachine)
    STATE_MATRIX_ENTRY(_HCC_IDLE),
    STATE_MATRIX_ENTRY(_HCC_SENDING_PACKET),
    STATE_MATRIX_ENTRY(_HCC_WAITING_FOR_REPLY),
    STATE_MATRIX_ENTRY(_HCC_IN_CONTINUOUS_SCAN_MODE)
RESPONSE_TABLE_END;


//////////////////////////////////////////////////
//
// Static Member Definitions
//
//////////////////////////////////////////////////

BYTE    HeadCommandCommMachine::hccTxBuffer[MAX_PACKET_LENGTH];
BYTE    HeadCommandCommMachine::hccRxBuffer[MAX_PACKET_LENGTH];

BYTE    HeadCommandCommMachine::txChecksum = 0x00;

WORD    HeadCommandCommMachine::errorCount = 0;

TableParametersDataManager  * HeadCommandCommMachine::ptrTPDM = 0;



//////////////////////////////////////////////////
//
// HeadCommandCommMachine - Constructors, Destructors
//
//////////////////////////////////////////////////

HeadCommandCommMachine::HeadCommandCommMachine(STATE_MACHINE_ID sMsysID)
    :StateMachine(sMsysID)
{
    ASSIGN_RESPONSE_TABLE();

    SetCurrState(HCC_IDLE);
}

HeadCommandCommMachine::~HeadCommandCommMachine(void) { }


WORD    HeadCommandCommMachine::GetErrorCount(void) {

    return  HeadCommandCommMachine::errorCount;
}


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// HeadCommandCommMachine - private EXIT PROCEDURES
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
// Message: Send Packet
//
//////////////////////////////////////////////////

WORD
HeadCommandCommMachine::HCC_h1(void)
{
        // Transfer the buffers

        SetProbHeadCommTxBuffer(hccTxBuffer, hccTxBuffer[LENGTH_SLOT]);

        // Enable Async Comm IRQ

        EnableProbeHeadCommunication();

    return  HCC_SENDING_PACKET;
}


//////////////////////////////////////////////////
//
// Message: Packet Sent
//
//////////////////////////////////////////////////

WORD
HeadCommandCommMachine::HCC_h2(void)
{
        // Begin a timer for timeout detection

        StartHiPriorityTimer(ptrTPDM->GetHeadCommTimeOut());

        // Just wait for the reply

    return  HCC_WAITING_FOR_REPLY;
}


//////////////////////////////////////////////////
//
// Message: Packet Received
//
//////////////////////////////////////////////////

WORD
HeadCommandCommMachine::HCC_h3(void)
{
    // Disable Async Comm IRQ

    DisableProbeHeadCommunication();

    // create a pointer to the Kernels ProbHeadCommRxBuffer

    BYTE * ptrRxCommBuff = GetProbHeadCommRxBuffer();

    // The first byte is the length byte

    int rxLength = (* ptrRxCommBuff);

        // Copy Kernel's buffer to local HCC buffer

        memcpy(hccRxBuffer, ptrRxCommBuff, rxLength);

        // Packet came in time

        CancelTimer();

        // Analyze packet, Check sum ok ?

        BYTE    rxChecksum  = 0;

        // sum of all the Data Bytes except the last one

        for(int c=0; c < rxLength-1; c++)
            rxChecksum += hccRxBuffer[c];

        if(rxChecksum != hccRxBuffer[c])
        {
            // Send Error - Checksum;

            goto packetAnalysisDone;
        }

        // Analyze packet, Command match ?

        if(hccRxBuffer[COMMAND_SLOT] != hccTxBuffer[COMMAND_SLOT])
        {
                // Send Error - Command Mismatch;

                SendHiPrMsg(HeadMachinesManagerID, ProbeHeadReplyCommandMismatch);

            goto packetAnalysisDone;
        }

        // Analyze packet, Ack or Nak ?

        if(hccRxBuffer[COMMAND_SLOT] == PROBE_HEAD_NAK)
        {
                // Send Error - Command NAK;

                SendHiPrMsg(HeadMachinesManagerID, ProbeHeadReplyNAK);

            goto packetAnalysisDone;
        }

        // Everything else is OK. Send CommandAcked. Packet
        // possibly has data instead of an ACK byte

        SendHiPrMsg(HeadMachinesManagerID, ProbeHeadCommandAcked);

packetAnalysisDone:

    return  HCC_IDLE;
}

//////////////////////////////////////////////////
//
// Message: TimeOut
//
//////////////////////////////////////////////////

WORD
HeadCommandCommMachine::HCC_h3a(void)
{
        // Send Message ProbeHeadCommTimedOut

        SendHiPrMsg(HeadMachinesManagerID, ProbeHeadCommTimedOut);

    return  HCC_IDLE;
}


//////////////////////////////////////////////////
//
// Start Continuous Scan Mode
//
//////////////////////////////////////////////////

WORD
HeadCommandCommMachine::HCC_h4(void)
{
        // No messages will be processed while in
        // this state execept the EndContinuousScanMode

    return  HCC_IN_CONTINUOUS_SCAN_MODE;
}


//////////////////////////////////////////////////
//
// End Continuous Scan Mode
//
//////////////////////////////////////////////////

WORD
HeadCommandCommMachine::HCC_h5(void)
{
        // Simply return to idle

    return  HCC_IDLE;
}






