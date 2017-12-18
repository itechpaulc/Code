

/////////////////////////////////////////////////////////////////////////////
//
//
//    $Header:      $
//    $Log:         $
//
//
//    Author : Paul Calinawan        February 1998
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



#include "ptxcomm.h"


#include "80386ex.h"


#include <string.h>

#pragma _builtin_(memcpy)




//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  PciTxCommMachine
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  PciTxCommMachine
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
PciTxCommMachine::QueueNextPciMessage
(MAILBOX_MESSAGE mboxMsg, WORD dataLengthWord, void * dataPtr)
{
    // Cast dataPtr to WORD * type

    WORD * dataSourcePtr = (WORD *)dataPtr;

    // Intilize Byte Count equivalent of dataLengthWord

    int dataLengthBytes = dataLengthWord * 2;

    // Pci Message Cell start is the Header start pointer.
    // Cast void * to PCI_MESSAGE_HEADER *

    PCI_MESSAGE_HEADER * headerStartPtr = (PCI_MESSAGE_HEADER *)nextPciMessageCellPtr;

    // Data start pointer. Cast void * to WORD *

    WORD * dataStartPtr = (WORD *)nextPciMessageCellPtr + MAILBOX_HEADER_WORD_SIZE;

    // initialize indexes

    WORD cellStart  = currCellIndex;
    WORD cellEnd    = currCellIndex;

        ++messageCount;

        // Copy data into the next cell(s), data section only

        if(dataLengthWord != 0)
            memcpy(dataStartPtr, dataSourcePtr, dataLengthBytes);

            WORD dataWordUsed = MAILBOX_HEADER_WORD_SIZE + dataLengthWord;

            WORD extraCellsUsed = dataWordUsed / PCI_MESSAGE_CELL_WORD_SIZE;


            // Adjust and move the Next cell pointer.
            // Keep track of the End cell index

            WORD * nextCellPtr = (WORD *)nextPciMessageCellPtr;

            for(WORD c=0; c < (extraCellsUsed+1); c++)
            {
                ++cellEnd;

                // Check if past over last cell boundary

                if(cellEnd > CELL_END_INDEX)
                {
                    // reset

                    cellEnd = 0;
                    nextCellPtr = (WORD *)firstPciMessageCellPtr;
                }
                else
                {
                    nextCellPtr += PCI_MESSAGE_CELL_WORD_SIZE;
                }
            }

            // update the Next Cell pointer

            nextPciMessageCellPtr = (void *)nextCellPtr;


        // Update indexes p1 and p2 of the mailbox message

        mboxMsg.SetParam1(cellStart);
        mboxMsg.SetParam2(cellEnd);

        // Rebuild and Update mailbox header

        nextPciMessageHeader.SetMailboxCopy(mboxMsg);
        nextPciMessageHeader.SetDataLength(dataLengthBytes);

        // Copy mboxMsg into the queue, pointer de-referenced

        * headerStartPtr = nextPciMessageHeader;
}


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// PciTxCommMachine - private helper functions
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL
PciTxCommMachine::isMailboxReady(void) {

    BYTE    p2Status = GetIO2Latch();   // Read Port 2

        return (p2Status & MAIL_BOX_OUT_READY) ?
            TRUE : FALSE ;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
PciTxCommMachine::latchNextPciMessage(void) {

    currPciMessageHeader = *(PCI_MESSAGE_HEADER *)currPciMessageCellPtr;

        // Write to Pci Bridge, Mailbox out LOW first then HI WORD

        *pciMailboxOutLoWordPtr = currPciMessageHeader.GetParam1and2();
        *pciMailboxOutHiWordPtr = currPciMessageHeader.GetCommandAndExtension();

        // Adjust curr Pci Cell Pointer, depending on length and
        // numbers of cells used by the message that was just sent

        WORD cellsUsed = currPciMessageHeader.GetCellsUsed();

        WORD * currCellPtr = (WORD *)currPciMessageCellPtr;

            // Calculate Cell offset

            currCellPtr += (cellsUsed * PCI_MESSAGE_CELL_BYTE_SIZE);

            // update the Current Cell pointer

            nextPciMessageCellPtr = (void *)currCellPtr;

        // update count

        --messageCount;
}



//////////////////////////////////////////////////
//
// PciTxCommMachine - RESPONSE ENTRIES
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
// State Transition Matrices
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(PciTxCommMachine, _PTC_IDLE)
    EV_HANDLER(TransmitPciMessage, PTC_h1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(PciTxCommMachine, _PTC_EMPTYING_PCI_MESSAGE_QUEUE)
    EV_HANDLER(TransmitPciMessage, PTC_h2),
    EV_HANDLER(TimeOut, PTC_h3)
STATE_TRANSITION_MATRIX_END;


//////////////////////////////////////////////////
//
// Matrix Table
//
//////////////////////////////////////////////////

DEFINE_RESPONSE_TABLE_ENTRY(PciTxCommMachine)
    STATE_MATRIX_ENTRY(_PTC_IDLE),
    STATE_MATRIX_ENTRY(_PTC_EMPTYING_PCI_MESSAGE_QUEUE)
RESPONSE_TABLE_END;



//////////////////////////////////////////////////
//
// Static Member Definitions
//
//////////////////////////////////////////////////

WORD    PciTxCommMachine::errorCount = 0;
WORD    PciTxCommMachine::messageCount = 0;

void    const *
PciTxCommMachine::firstPciMessageCellPtr = PCI_TX_BUFFER_ADDRESS;

void    *
PciTxCommMachine::currPciMessageCellPtr  = (void *)PCI_TX_BUFFER_ADDRESS;

void    *
PciTxCommMachine::nextPciMessageCellPtr  = currPciMessageCellPtr;

void    const *
PciTxCommMachine::lastPciMessageCellPtr = (void const *)PCI_LAST_TX_MESSAGE_SLOT_ADDRESS;


WORD    PciTxCommMachine::currCellIndex = 0;


PCI_MESSAGE_HEADER  PciTxCommMachine::nextPciMessageHeader;
PCI_MESSAGE_HEADER  PciTxCommMachine::currPciMessageHeader;


WORD *
PciTxCommMachine::pciMailboxOutHiWordPtr = (WORD *)PCI_MAIL_BOX_OUT_ADDRESS_HI_WORD;

WORD *
PciTxCommMachine::pciMailboxOutLoWordPtr = (WORD *)PCI_MAIL_BOX_OUT_ADDRESS_LO_WORD;



//////////////////////////////////////////////////
//
//  PciTxCommMachine - Constructors, Destructors
//
//////////////////////////////////////////////////

PciTxCommMachine::PciTxCommMachine(STATE_MACHINE_ID sMsysID)
    :StateMachine(sMsysID)
{
    ASSIGN_RESPONSE_TABLE();

    SetCurrState(PTC_IDLE);
}

PciTxCommMachine::~PciTxCommMachine(void) { }


WORD    PciTxCommMachine::GetErrorCount(void) {

    return  PciTxCommMachine::errorCount;
}


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// PciTxCommMachine - private EXIT PROCEDURES
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////


#define     TX_BUFFER_MONITOR_TIME      MIN_SYSTEM_TICK

//////////////////////////////////////////////////
//
// Message : Transmit Pci Message
//
//////////////////////////////////////////////////

WORD
PciTxCommMachine::PTC_h1(void) {

        latchNextPciMessage();

        StartTimer(TX_BUFFER_MONITOR_TIME);

    return PTC_EMPTYING_PCI_MESSAGE_QUEUE;
}


//////////////////////////////////////////////////
//
// Message : Transmit Pci Message
//
//////////////////////////////////////////////////

WORD
PciTxCommMachine::PTC_h2(void) {

        // just keep track of how many
        // messages are in the queue

        ++messageCount;

    return PTC_EMPTYING_PCI_MESSAGE_QUEUE;
}


//////////////////////////////////////////////////
//
// Message : TimeOut
//
//////////////////////////////////////////////////

WORD
PciTxCommMachine::PTC_h3(void) {

        if(isMailboxReady())
        {
            if(messageCount==0)
            {
                // isLastPciMessageSent == TRUE

                return PTC_IDLE;
            }

            // else more messages to send

            latchNextPciMessage();
        }

        StartTimer(TX_BUFFER_MONITOR_TIME);

    return PTC_EMPTYING_PCI_MESSAGE_QUEUE;
}


