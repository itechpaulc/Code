




/////////////////////////////////////////////////////////////////////////////
//
//
//    $Header:      $
//    $Log:         $
//
//    Author : Paul Calinawan       December 1997
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



#include "kernel.h"

#include "80386ex.h"

#include "mtrcomm.h"

#include "meascomm.h"

#include "hccomm.h"


#include <string.h>

#pragma _builtin_(memcpy)



//////////////////////////////////////////////////
//
// Local variables of the Kernel
//
//////////////////////////////////////////////////


#define     TRANSMIT_BUFFER_EMPTY       0x01
#define     RECEIVE_BUFFER_FULL         0x02


static  BYTE    probeHeadRxBuffer[MAX_PACKET_LENGTH];
static  BYTE    probeHeadTxBuffer[MAX_PACKET_LENGTH];

static  BYTE    iir0, commStatus,

                rxByteCount=0,  rxDataRcvd=0x00,    rxLength,
                txByteCount=0,  txLength;


static  SCAN_MEASUREMENT_MODE   asynchronousCommMode = NORMAL_SCAN_MEASUREMENT;
static  SCAN_MEASUREMENT_MODE   motorchipSetCommMode = NORMAL_SCAN_MEASUREMENT;


//////////////////////////////////////////////////
//
// IACM - machine and
// IMCSC - machine
//
//////////////////////////////////////////////////

#define MEASURE_COMMAND_LENGTH      0x03

#define MEASURE_COMMAND_CHECKSUM    (PHC_MeasureWithFlash + MEASURE_COMMAND_LENGTH)

BYTE    MEASURE_COMMAND[] = {

    MEASURE_COMMAND_LENGTH,
    PHC_MeasureWithFlash,
    MEASURE_COMMAND_CHECKSUM
};


void    SetMeasurementMode(SCAN_MEASUREMENT_MODE smM) {

    asynchronousCommMode = smM;
    motorchipSetCommMode = smM;

    // Set up Tx Buffer for the probe head
    // "Measure" command only

    SetProbHeadCommTxBuffer(MEASURE_COMMAND, MEASURE_COMMAND_LENGTH);
}


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  Deferred Interrupt Response Kernel Handlers
//  Forward Declaration
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

void    CheckMotorCsIrq(void);
void    UpdateSystemTimers(void);
void    HandlePciBusIrq(void);



//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////


StateMachine::StateMachine(STATE_MACHINE_ID smId)
    : currState(NULL_STATE),
      smID(smId),
      responseEntry(0)
{ }

StateMachine::~StateMachine(void) { }


//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

BYTE
StateMachine::GetCurrState(void) {

    return currState;
}


//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

void
StateMachine::SetCurrState(BYTE cState) {

    currState = cState;
}


//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

STATE_MACHINE_ID
StateMachine::GetStateMachineId(void) {

    return smID;
}


//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

void
StateMachine::AssignResponseEntry(GENERIC_TABLE_PTR re) {

    responseEntry = re;
}


//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

void
StateMachine::ProcessEvent(SYSTEM_EVENT *stateMMsg) {

    stateEntrySelected = responseEntry + currState;

    while(1)
    {
        // Search for an Event or MessageID Match

        msgEntrySelected = (ENTRY_MAP *)(* stateEntrySelected);

        if(msgEntrySelected->msgId == stateMMsg->msgId)
        {
            // Execute exit procedure and update the Current State

            currState =
                msgEntrySelected->exitProc();

            break;
        }
        else
        if(msgEntrySelected->msgId == NULL_MESSAGE_ID)
        {
            doUnhandledMsg();
            break;
        }

        stateEntrySelected++;
    }
}


//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

void
StateMachine::doUnhandledMsg(void)
{
    // Send NAK to Original Sender

}


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// PCI Message Structures, Constructors
// Destructors and  Member access
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
// constructor used to extract
// data from mailbox memory
//
//////////////////////////////////////////////////

MAILBOX_MESSAGE::MAILBOX_MESSAGE(void) {

    Clear();
}

MAILBOX_MESSAGE::MAILBOX_MESSAGE(DWORD mboxData) {

    commandAndExtension = LOWORD(mboxData);

    param1     = (BYTE)(HIWORD(mboxData));
    param2     = (BYTE)(HIWORD(mboxData) >> 8);
}


MAILBOX_MESSAGE::~MAILBOX_MESSAGE(void) { }


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MAILBOX_MESSAGE::Clear(void) {

    commandAndExtension = 0x0000;
    param1 = param2     = 0x00;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MAILBOX_MESSAGE::SetParam1(BYTE p1) {

    param1 = p1;
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MAILBOX_MESSAGE::SetParam2(BYTE p2) {

    param2 = p2;
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BYTE
MAILBOX_MESSAGE::GetParam1(void) {

    return  param1;
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BYTE
MAILBOX_MESSAGE::GetParam2(void) {

    return  param2;
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MAILBOX_MESSAGE::SetCommand(WORD cmd) {

    commandAndExtension &= ~PCI_MESSAGE_COMMAND_MASK; // clear

    commandAndExtension |= cmd;
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
MAILBOX_MESSAGE::GetCommand(void) {

    return (commandAndExtension & PCI_MESSAGE_COMMAND_MASK);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MAILBOX_MESSAGE::SetExtension(WORD ext) {

    commandAndExtension &= ~PCI_MESSAGE_EXT_MASK; // clear

    commandAndExtension |= ((ext << 12) & PCI_MESSAGE_EXT_MASK);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
MAILBOX_MESSAGE::GetExtension(void) {

    return ((commandAndExtension & PCI_MESSAGE_EXT_MASK) >> 12);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MAILBOX_MESSAGE::SetCommandAndExtension(WORD cmdExt) {

    commandAndExtension = cmdExt;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
MAILBOX_MESSAGE::GetCommandAndExtension(void) {

    return commandAndExtension;
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
MAILBOX_MESSAGE::GetParam1and2(void) {

    return ((param2 << 8) | param1);
}



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// PCI Message Header
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
PCI_MESSAGE_HEADER::GetCommandAndExtension(void) {

    return mboxMessageCopy.GetCommandAndExtension();
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
PCI_MESSAGE_HEADER::GetParam1and2(void) {

    return mboxMessageCopy.GetParam1and2();
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
PCI_MESSAGE_HEADER::GetDataLength(void) {

    return length;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
PCI_MESSAGE_HEADER::SetMailboxCopy(MAILBOX_MESSAGE mboxCopy) {

    mboxMessageCopy = mboxCopy;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
PCI_MESSAGE_HEADER::SetDataLength(WORD dataLength) {

    length = dataLength;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
PCI_MESSAGE_HEADER::GetCellsUsed(void) {

        if(length == 0) {

            // Only one cell used, param 1 and param 2
            // may contain data

            return 1;
        }


        // param 1 is Start Cell and param 2 is End Cell
        // by protocol definition

    return  (mboxMessageCopy.GetParam2() -
             mboxMessageCopy.GetParam1());
}




//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  Kernel - Section
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////



//////////////////////////////////////////////////
//
//  Kernel - private data :
//
//////////////////////////////////////////////////


// System State Machine's Pointer Table

static  StateMachine    *sysSMEntries[MAX_STATE_MACHINES];

static  StateMachine    *currStateMachine = 0;

static  BYTE            smCount=0;

static  SYSTEM_EVENT    currEvent;



//////////////////////////////////////////////////
//
// Interrupt Flags - Kernel, Private
//
//////////////////////////////////////////////////

BOOL    TIMER_0_IRQ_SET     = FALSE,
        MOTOR_CS_IRQ_SET    = FALSE,
        PCI_BUS_IRQ_SET     = FALSE,
        PACKET_SENT         = FALSE,
        PACKET_RECEIVED     = FALSE,

        MOTOR_CS_IRQ_X_MOTION_COMPLETE              = FALSE,
        MOTOR_CS_IRQ_X_NEGATIVE_LIMIT_SWITCH        = FALSE,
        MOTOR_CS_IRQ_X_UPDATE_BREAK_POINT_REACHED   = FALSE,

        MOTOR_CS_IRQ_Y_MOTION_COMPLETE              = FALSE,
        MOTOR_CS_IRQ_Y_NEGATIVE_LIMIT_SWITCH        = FALSE,
        MOTOR_CS_IRQ_Y_UPDATE_BREAK_POINT_REACHED   = FALSE,

        MEASUREMENT_BLOCK_RECEIVED = FALSE;


//////////////////////////////////////////////////
//
// Dedicated State Mahcine Timers
//
//////////////////////////////////////////////////

TIMER_DATA  systemTimers[MAX_STATE_MACHINES];




//////////////////////////////////////////////////
//
// Ring Buffer Position calculation constants
//
//////////////////////////////////////////////////

static  SYSTEM_EVENT    *LastHpMsgSlot=0,
                        *LastLpMsgSlot=0;

static  SYSTEM_EVENT    *currHPMsg=0, *lastHPMsg=0,
                        *currLPMsg=0, *lastLPMsg=0;

static  SYSTEM_EVENT    highPriorityQueue[MAX_MESSAGE_COUNT],
                        lowPriorityQueue[MAX_MESSAGE_COUNT];



//////////////////////////////////////////////////
//
//  Kernel - public interface :
//
//////////////////////////////////////////////////


#define    MESSAGE_QUEUE_SIZE     (MAX_MESSAGE_COUNT-1)


void    InitKernel(void)
{
    // Point to the 1st Message Queue Slot

    currHPMsg = lastHPMsg = highPriorityQueue;
    currLPMsg = lastLPMsg = lowPriorityQueue;

    for(int smIdx=0; smIdx<MAX_STATE_MACHINES; smIdx++) {

            systemTimers[smIdx].timerCount = 0x0000;
            systemTimers[smIdx].hiPriority = FALSE;

            sysSMEntries[smIdx] = 0;
    }

    // Message Queue Wrap around pointer init

    LastHpMsgSlot = &highPriorityQueue[MESSAGE_QUEUE_SIZE];
    LastLpMsgSlot = &lowPriorityQueue[MESSAGE_QUEUE_SIZE];
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    AddStateMachineToKernel(StateMachine *stateMPtr) {

    sysSMEntries[stateMPtr->GetStateMachineId()] = stateMPtr;

    smCount++;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

StateMachine *  GetStateMachine(BYTE smId) {

    return sysSMEntries[smId];
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD    GetStateMachineState(BYTE  smId) {

    if(sysSMEntries[smId] != 0)
        return  sysSMEntries[smId]->GetCurrState();

    return NULL_STATE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

SYSTEM_EVENT    GetCurrEvent(void) {

    return  currEvent;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL    StateMachineErrorsLogged(void) {

    for(int smIdx=0; smIdx<MAX_STATE_MACHINES; smIdx++) {

        if(sysSMEntries[smIdx] != 0)
            if(sysSMEntries[smIdx]->GetErrorCount())
                return TRUE;
    }

    return FALSE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD    GetStateMachineErrorCount(BYTE  smId) {

    if(sysSMEntries[smId] != 0)
        return  sysSMEntries[smId]->GetErrorCount();

    return 0;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    RunKernel(void)
{
    while(1)
    {

        if(currHPMsg != lastHPMsg)    // Check if a HIGH priority message is queued
        {
            currEvent = (*currHPMsg);

            currStateMachine = sysSMEntries[currHPMsg->smDestId];

            currStateMachine->ProcessEvent(currHPMsg);

            if(currHPMsg == LastHpMsgSlot)
                currHPMsg = highPriorityQueue;  // Point to head
            else
                currHPMsg++;                    // Point to next
        }
        else
        if(currLPMsg != lastLPMsg)    // Check if a LOW priority message is queued
        {
            currEvent = (*currHPMsg);

            currStateMachine = sysSMEntries[currLPMsg->smDestId];

            currStateMachine->ProcessEvent(currLPMsg);

            if(currLPMsg == LastLpMsgSlot)
                currLPMsg = lowPriorityQueue;
            else
                currLPMsg++;
        }

        //
        // Check for interrupt flags
        //

        if(TIMER_0_IRQ_SET)
        {
            TIMER_0_IRQ_SET = FALSE;
            UpdateSystemTimers();
        }

        if(MOTOR_CS_IRQ_SET)
        {
            MOTOR_CS_IRQ_SET = FALSE;
            CheckMotorCsIrq();
        }

        if(PACKET_SENT)
        {
            PACKET_SENT = FALSE;
            SendHiPrKernelMsg(HeadCommandCommID, PacketSent);
        }

        if(PACKET_RECEIVED)
        {
            PACKET_RECEIVED = FALSE;
            SendHiPrKernelMsg(HeadCommandCommID, PacketReceived);
        }

        if(PCI_BUS_IRQ_SET)
        {
            PCI_BUS_IRQ_SET = FALSE;
            HandlePciBusIrq();
        }

        if(MEASUREMENT_BLOCK_RECEIVED)
        {
            MEASUREMENT_BLOCK_RECEIVED = FALSE;
            SendHiPrKernelMsg(MeasurementCommMachineID, MeasurementBlockReceived);
        }

    }
}



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    SendHiPrMsg(STATE_MACHINE_ID smId, SYSTEM_MESSAGE_ID mId) {

    lastHPMsg->smDestId = smId;
    lastHPMsg->senderId = currEvent.smDestId;
    lastHPMsg->msgId = mId;

    if(lastHPMsg == LastHpMsgSlot)
        lastHPMsg = highPriorityQueue;  // Point to head
    else
        lastHPMsg++;                    // Point to next
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    SendLowPrMsg(STATE_MACHINE_ID smId, SYSTEM_MESSAGE_ID mId) {

    lastLPMsg->smDestId = smId;
    lastLPMsg->senderId = currEvent.smDestId;
    lastLPMsg->msgId = mId;

    if(lastLPMsg == LastLpMsgSlot)
        lastLPMsg = lowPriorityQueue;
    else
        lastLPMsg++;
}



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    SendHiPrMsg(STATE_MACHINE_ID smId, SYSTEM_MESSAGE_ID mId, DWORD d1, DWORD d2) {

    lastHPMsg->smDestId = smId;
    lastHPMsg->senderId = currEvent.smDestId;
    lastHPMsg->msgId = mId;
    lastHPMsg->msgData1 = d1;
    lastHPMsg->msgData2 = d2;

    if(lastHPMsg == LastHpMsgSlot)
        lastHPMsg = highPriorityQueue;  // Point to head
    else
        lastHPMsg++;                    // Point to next
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    SendLowPrMsg(STATE_MACHINE_ID smId, SYSTEM_MESSAGE_ID mId, DWORD d1, DWORD d2) {

    lastLPMsg->smDestId = smId;
    lastLPMsg->senderId = currEvent.smDestId;
    lastLPMsg->msgId = mId;
    lastLPMsg->msgData1 = d1;
    lastLPMsg->msgData2 = d2;

    if(lastLPMsg == LastLpMsgSlot)
        lastLPMsg = lowPriorityQueue;
    else
        lastLPMsg++;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    SendHiPrKernelMsg(STATE_MACHINE_ID smId, SYSTEM_MESSAGE_ID mId) {

    lastHPMsg->smDestId = smId;
    lastHPMsg->senderId = KernelID;
    lastHPMsg->msgId = mId;

    if(lastHPMsg == LastHpMsgSlot)
        lastHPMsg = highPriorityQueue;  // Point to head
    else
        lastHPMsg++;                    // Point to next
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    SendLowPrKernelMsg(STATE_MACHINE_ID smId, SYSTEM_MESSAGE_ID mId) {

    lastLPMsg->smDestId = smId;
    lastLPMsg->senderId = KernelID;
    lastLPMsg->msgId = mId;

    if(lastLPMsg == LastLpMsgSlot)
        lastLPMsg = lowPriorityQueue;
    else
        lastLPMsg++;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    StartTimer(WORD tmrCount) {

    if(tmrCount == 0)
    {
        SendLowPrKernelMsg
            (currStateMachine->GetStateMachineId(), TimeOut);
    }
    else
    {
        systemTimers[currStateMachine->GetStateMachineId()].timerCount
            = tmrCount;

        systemTimers[currStateMachine->GetStateMachineId()].hiPriority
            = FALSE;
    }
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    StartHiPriorityTimer(WORD tmrCount) {

    if(tmrCount == 0)
    {
        SendHiPrKernelMsg
            (currStateMachine->GetStateMachineId(), TimeOut);
    }
    else
    {
        systemTimers[currStateMachine->GetStateMachineId()].timerCount
            = tmrCount;

        systemTimers[currStateMachine->GetStateMachineId()].hiPriority
            = TRUE;
    }
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    CancelTimer(void) {

    // Expire the timer of the current machine

    systemTimers[currStateMachine->GetStateMachineId()].timerCount
        = 0x0000;
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BYTE    GetCurrSmKernelId(void)
{
    return currStateMachine->GetStateMachineId();
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

static  WORD    irqErrorCount = 0;

WORD    GetIrqErrorCount(void)
{
    return irqErrorCount;
}

//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  Deferred Interrupt Response Kernel Handlers
//  Definition
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

static  void    UpdateSystemTimers(void)
{
    for(int currSmTimer=0; currSmTimer<MAX_STATE_MACHINES ;currSmTimer++)
    {
        if(systemTimers[currSmTimer].timerCount != 0)
        {
            --systemTimers[currSmTimer].timerCount;

            // Send Message - Timer Expiration to the
            // appropriate state machine - indexed

            if(systemTimers[currSmTimer].timerCount == 0)
            {
                if(systemTimers[currSmTimer].hiPriority == TRUE)
                    SendHiPrKernelMsg(STATE_MACHINE_ID(currSmTimer), TimeOut);
                else
                    SendLowPrKernelMsg(STATE_MACHINE_ID(currSmTimer), TimeOut);
            }
        }
    }
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

static  void    CheckMotorCsIrq(void)
{

        //////////////////////////////////////////////////
        // X AXIS
        //////////////////////////////////////////////////

        if(MOTOR_CS_IRQ_X_MOTION_COMPLETE) {

            MOTOR_CS_IRQ_X_MOTION_COMPLETE = FALSE;
            SendHiPrKernelMsg(MotorCommID, XAxisOnTarget);
        }

        if(MOTOR_CS_IRQ_X_NEGATIVE_LIMIT_SWITCH) {

            MOTOR_CS_IRQ_X_NEGATIVE_LIMIT_SWITCH = FALSE;
            SendHiPrKernelMsg(MotorCommID, XLimitFound);
        }

        if(MOTOR_CS_IRQ_X_UPDATE_BREAK_POINT_REACHED) {

            MOTOR_CS_IRQ_X_UPDATE_BREAK_POINT_REACHED = FALSE;
            SendHiPrKernelMsg(HeadMachinesManagerID, XBreakPointReached);
        }


        //////////////////////////////////////////////////
        // Y AXIS
        //////////////////////////////////////////////////

        if(MOTOR_CS_IRQ_Y_MOTION_COMPLETE) {

            MOTOR_CS_IRQ_Y_MOTION_COMPLETE = FALSE;
            SendHiPrKernelMsg(MotorCommID, YAxisOnTarget);
        }

        if(MOTOR_CS_IRQ_Y_NEGATIVE_LIMIT_SWITCH) {

            MOTOR_CS_IRQ_Y_NEGATIVE_LIMIT_SWITCH = FALSE;
            SendHiPrKernelMsg(MotorCommID, YLimitFound);
        }

        if(MOTOR_CS_IRQ_Y_UPDATE_BREAK_POINT_REACHED) {

            MOTOR_CS_IRQ_Y_UPDATE_BREAK_POINT_REACHED = FALSE;
            SendHiPrKernelMsg(HeadMachinesManagerID, YBreakPointReached);
        }
}



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    HandlePciBusIrq(void) {

    // Inform PCI RX Comm about the event

    SendHiPrKernelMsg(PciRxCommID, PciMessageReceived);
}


//////////////////////////////////////////////////
//
// Kernel's Mailbox Access Functions
//
//////////////////////////////////////////////////

MAILBOX_MESSAGE     ReadMailboxMessage(void) {

    // Extract the mailbox data from the mailbox
    // memory, store in a DWORD

    DWORD   mboxData = (*((WORD *) PCI_MAIL_BOX_IN_ADDRESS_HI_WORD) << 16);

            mboxData &= *((WORD *) PCI_MAIL_BOX_IN_ADDRESS_LO_WORD);

        // Create a message intialized with the DWORD

        MAILBOX_MESSAGE mboxMsg(mboxData);

    return mboxMsg;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
WriteMailboxMessage(MAILBOX_MESSAGE & mboxMsg) {

    WORD    * ptrCmdExtAddr = (WORD  *)PCI_MAIL_BOX_IN_ADDRESS_HI_WORD;
    WORD    * ptrPar1Par2   = (WORD  *)PCI_MAIL_BOX_IN_ADDRESS_LO_WORD;

        // combine p1 and p2 to form a word

        WORD p1andp2 = ((mboxMsg.GetParam1() << 8) | mboxMsg.GetParam2());

        // write to the mbox hi word address

        *ptrPar1Par2  =  p1andp2;

        // write to the mbox hi word address

        *ptrCmdExtAddr = mboxMsg.GetCommandAndExtension();
}




//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  386EX Perhiperal Initialization Routine
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

void    InitPerhiperals(void)
{

    //////////////////////////////////////////////////
    //
    //  Initialize Clock PMU (Power Management)
    //
    //////////////////////////////////////////////////

    Init_ClockPMU(POWER_CONTROL, CLOCK_PRESCALE);


    //////////////////////////////////////////////////
    //
    //  Initialize Interrupts
    //
    //  Note:   Dont Care about configuration settings of
    //          MASTER_EXT_PINS, SLAVE_EXT_PINS
    //          at this point (both are = 0x00)
    //
    //////////////////////////////////////////////////

    InitICU(ICU_TRIGGER_EDGE, ICU_MASTER_VECT_BASE, MCAS_IR1,
            ICU_TRIGGER_EDGE, ICU_SLAVE_VECT_BASE,
            MASTER_EXT_PINS, SLAVE_EXT_PINS);


    //////////////////////////////////////////////////
    //
    //  Initialize Timers
    //
    //////////////////////////////////////////////////

    InitTimer(TMR_0, TMR_RATEGEN, TMR_0_INPUTS, TMR_0_OUTPUTS,
                SYSTEM_TICK, TMR_ENABLE);


    //////////////////////////////////////////////////
    //
    //  Initialize DMA for SSIO Interrupt to transfer
    //  measurment data directly to memory
    //
    //////////////////////////////////////////////////

    InitDMAForSSIOToMemory();


    //////////////////////////////////////////////////
    //
    //  Initialize Sync Serial
    //
    //////////////////////////////////////////////////

    InitSSIO();


    //////////////////////////////////////////////////
    //
    //  Initialize Async Serial
    //
    //////////////////////////////////////////////////

    InitSIO(SIO_0, SIO_8N1, PROBE_HEAD_COMM_BAUD_RATE, BCLKIN);



    //////////////////////////////////////////////////
    //
    // Finalize IO Configuration Settings
    //
    //////////////////////////////////////////////////

    InitIO1(PORT_1_LATCH, PORT_1_DIR, PORT_1_CFG);
    InitIO2(PORT_2_LATCH, PORT_2_DIR, PORT_2_CFG);
    InitIO3(PORT_3_LATCH, PORT_3_DIR, PORT_3_CFG);



    //////////////////////////////////////////////////
    //
    //  Enable the interrupts
    //
    //////////////////////////////////////////////////

    Enable8259Interrupt(MASTER_MASK, SLAVE_MASK);

}



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  Kernel - Interrupt Service Response Handler
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

extern "C" {

    void   irq_0_handler(void);
    void   irq_1_handler(void);
    void   irq_2_handler(void);
    void   irq_3_handler(void);
    void   irq_4_handler(void);
    void   irq_5_handler(void);
    void   irq_6_handler(void);
    void   irq_7_handler(void);
    void   irq_8_handler(void);
    void   irq_9_handler(void);
    void   irq_10_handler(void);
    void   irq_11_handler(void);
    void   irq_12_handler(void);
    void   irq_13_handler(void);
    void   irq_14_handler(void);
    void   irq_15_handler(void);
}



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// From MASTER INTERRUPT Controller
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
//  TIMER 0
//
//      - Interrupt Service Response Handler
//
//////////////////////////////////////////////////

#pragma interrupt(irq_0_handler)
void
irq_0_handler(void)
{
        TIMER_0_IRQ_SET = TRUE;

    NonSpecificEOI();
}

//////////////////////////////////////////////////
//
//  Motor Chip Set
//
//      - Interrupt Service Response Handler
//
//////////////////////////////////////////////////

#pragma interrupt(irq_1_handler)
void
irq_1_handler(void)
{
    WORD    mcsStatus;

    // Set Axis to the one interrupting

    SendMcsCommand(SET_CURR_AXIS_INTERRUPT);
    SendMcsCommand(GET_STATUS);

    mcsStatus = ReadMcs();

        if(mcsStatus & X_AXIS_BIT) // X Axis Interrupt
        {
            // Check for all the possilbe events
            // inform the appropriate machines about the event

            if(mcsStatus & MOTION_COMPLETE)
                MOTOR_CS_IRQ_X_MOTION_COMPLETE = TRUE;

            if(mcsStatus & NEGATIVE_LIMIT_SWITCH)
                MOTOR_CS_IRQ_X_NEGATIVE_LIMIT_SWITCH = TRUE;

            if(mcsStatus & UPDATE_BREAK_POINT_REACHED)
            {
                if(motorchipSetCommMode == CONTINOUS_SCAN_MEASUREMENT)
                {
                    // Immediately send the measurement command just by
                    // Enabling the probe head comm

                    EnableProbeHeadCommunication();
                }

                MOTOR_CS_IRQ_X_UPDATE_BREAK_POINT_REACHED = TRUE;
            }
        }
        else // Y AXIS Interrupt
        {
            if(mcsStatus & MOTION_COMPLETE)
                MOTOR_CS_IRQ_Y_MOTION_COMPLETE = TRUE;

            if(mcsStatus & NEGATIVE_LIMIT_SWITCH)
                MOTOR_CS_IRQ_Y_NEGATIVE_LIMIT_SWITCH = TRUE;

            if(mcsStatus & UPDATE_BREAK_POINT_REACHED)
                MOTOR_CS_IRQ_Y_UPDATE_BREAK_POINT_REACHED = TRUE;
        }

        MOTOR_CS_IRQ_SET = TRUE;

    NonSpecificEOI();
}

//////////////////////////////////////////////////
//
//  UNUSED
//
//  Interrupt Cascade
//
//      - Interrupt Service Response Handler
//
//////////////////////////////////////////////////

#pragma interrupt(irq_2_handler)
void
irq_2_handler(void)
{
    ++irqErrorCount;
}

//////////////////////////////////////////////////
//
//  UNUSED
//
//  Asynchronous Serial Communication
//  Port 1
//
//      - Interrupt Service Response Handler
//
//////////////////////////////////////////////////

#pragma interrupt(irq_3_handler)
void
irq_3_handler(void)
{
    ++irqErrorCount;
}


//////////////////////////////////////////////////
//
//  Asynchronous Serial Communication
//  Port 0
//
//      - Interrupt Service Response Handler
//
//////////////////////////////////////////////////

void    SerialWriteChar(BYTE data)
{
    _SetEXRegByte(TBR0, data);
}


//////////////////////////////////////////////////
//
// Kernel's Probe Head Communication
// Access Functions
//
//////////////////////////////////////////////////

void    EnableProbeHeadCommunication(void) {

    // Enable TBE and RBF Interrupts

    _SetEXRegByte(IER0, (SIO_INTR_TBE | SIO_INTR_RBF));

    txByteCount = 0; // reset counters, index
    rxByteCount = 0;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DisableProbeHeadCommunication(void) {

    // Disable TBE and RBF Interrupts

    _SetEXRegByte(IER0, 0x00);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    SetProbHeadCommTxBuffer(BYTE *txBuff, BYTE txLength) {

    memcpy(probeHeadTxBuffer, txBuff, txLength);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BYTE    * GetProbHeadCommRxBuffer(void) {

    return probeHeadRxBuffer;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

#pragma interrupt(irq_4_handler)
void
irq_4_handler(void)
{
        iir0 = _GetEXRegByte(IIR0);

        commStatus = ((iir0 & 0x06) >> 1);

        //////////////////////////////////////////////////
        // Check if BYTE was received
        //////////////////////////////////////////////////

        if(commStatus == RECEIVE_BUFFER_FULL)
        {
            rxDataRcvd = _GetEXRegByte(RBR0);

            // Store data byte

            probeHeadRxBuffer[rxByteCount] = rxDataRcvd;

            if(rxByteCount == 0)
            {
                // Packet Header, the First Byte
                // is the length byte

                rxLength = probeHeadRxBuffer[rxByteCount];

                if(rxLength > MAX_PACKET_LENGTH)
                {
                    // Debug
                    // Invalid rxLength

                }
            }
            else // next packet data
            {
                if(rxLength == (rxByteCount+1))
                {
                    if(asynchronousCommMode == CONTINOUS_SCAN_MEASUREMENT)
                    {
                        // Disable the probe head communication that was automatically
                        // enalbled by the motor IRQ while in this mode

                        DisableProbeHeadCommunication();
                    }
                    else
                    {
                        // NORMAL_SCAN_MEASUREMENT
                        // Packet fully received, tell Kernel

                        PACKET_RECEIVED = TRUE;
                    }
                }
            }

            rxByteCount++;
        }

        else

        //////////////////////////////////////////////////
        // Check if BYTE was transmitted
        //////////////////////////////////////////////////

        if(commStatus == TRANSMIT_BUFFER_EMPTY)
        {
            if(txByteCount == 0)
            {
                // UART was Just Enabled, the First Byte
                // is the length byte

                txLength = probeHeadTxBuffer[txByteCount];

                SerialWriteChar(txLength);

                txByteCount++;
            }
            else
            {
                if(txByteCount < txLength)
                {
                    // Transmission has started, Transmit the
                    // rest of the packet

                    SerialWriteChar(probeHeadTxBuffer[txByteCount]);

                    txByteCount++;
                }
                else
                {
                    if(asynchronousCommMode == NORMAL_SCAN_MEASUREMENT)
                    {
                        // Packet Transmitted, tell Kernel

                        PACKET_SENT = TRUE;
                    }
                }
            }
        }

    NonSpecificEOI();
}

//////////////////////////////////////////////////
//
//  PCI Bridge, new data received in the
//  mail box
//
//      - Interrupt Service Response Handler
//
//////////////////////////////////////////////////

#pragma interrupt(irq_5_handler)
void
irq_5_handler(void)
{
        PCI_BUS_IRQ_SET = TRUE;

    NonSpecificEOI();
}

//////////////////////////////////////////////////
//
//  UNUSED
//
//  Pin Multiplexed and used as
//  a general purpose output.
//
//      - Interrupt Service Response Handler
//
//////////////////////////////////////////////////

#pragma interrupt(irq_6_handler)
void
irq_6_handler(void)
{
    ++irqErrorCount;

    NonSpecificEOI();
}

//////////////////////////////////////////////////
//
//  UNUSED
//
//  Pin Multiplexed and used as
//  a general purpose output.
//
//      - Interrupt Service Response Handler
//
//////////////////////////////////////////////////

#pragma interrupt(irq_7_handler)
void
irq_7_handler(void)
{
    ++irqErrorCount;

    NonSpecificEOI();
}


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// From SLAVE INTERRUPT Controller
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
//  UNUSED
//
//  External Interrupt INT4. Port is not used
//  Pin is multiplexed.
//
//      - Interrupt Service Response Handler
//
//////////////////////////////////////////////////

#pragma interrupt(irq_8_handler)
void
irq_8_handler(void)
{
    ++irqErrorCount;

    NonSpecificEOI();
}


//////////////////////////////////////////////////
//
//  UNUSED
//
//  Synchronous Serial Communication (SSIO).
//
//  Does not generate the interrupt. The DMA is
//  the one that generates the interrupt for the
//  data block received by SSIO.
//
//      - Interrupt Service Response Handler
//
//////////////////////////////////////////////////

#pragma interrupt(irq_9_handler)
void
irq_9_handler(void)
{
    ++irqErrorCount;

    NonSpecificEOI();
}

//////////////////////////////////////////////////
//
//  UNUSED
//
//  Timer 1. Not used.
//
//      - Interrupt Service Response Handler
//
//////////////////////////////////////////////////

#pragma interrupt(irq_10_handler)
void
irq_10_handler(void)
{
    ++irqErrorCount;

    NonSpecificEOI();
}


//////////////////////////////////////////////////
//
//  UNUSED
//
//  Timer 2. Not used.
//
//      - Interrupt Service Response Handler
//
//////////////////////////////////////////////////

#pragma interrupt(irq_11_handler)
void
irq_11_handler(void)
{
    ++irqErrorCount;

    NonSpecificEOI();
}


//////////////////////////////////////////////////
//
// Kernel's Probe Head
// Meaurement Data Communication Access Functions
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    EnableMeasurementDataReception(void) {

    // Call Driver Level to enable DMA

    EnableDMAHWRequests(DMA_Channel1);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DisableMeasurementDataReception(void) {

    // Call Driver Level to disable DMA

    DisableDMAHWRequests(DMA_Channel1);
}


//////////////////////////////////////////////////
//
//  DMA
//
//  Generates an interrupt when the full
//  block of data is received by the SSIO.
//
//      - Interrupt Service Response Handler
//
//////////////////////////////////////////////////

#define     DMA1_TRANSFER_COMPLETE      0x20

#pragma interrupt(irq_12_handler)
void
irq_12_handler(void)
{
    WORD    regDMAIS;

        // Get Interrupt Status Register

        regDMAIS = _GetEXRegByte(DMAIS);

        //////////////////////////////////////////////////
        //
        // DMA1 is used with SSIO to receive
        // the measurement data
        //
        //////////////////////////////////////////////////

        if(regDMAIS & DMA1_TRANSFER_COMPLETE)
        {
            // Clear the Transfer Complete Signal

            _SetEXRegByte(DMACLRTC, 0x00);

            MEASUREMENT_BLOCK_RECEIVED = TRUE;
        }

    NonSpecificEOI();
}

//////////////////////////////////////////////////
//
//  UNUSED
//
//  External Interrupt INT6. Port is not used
//  Pin is multiplexed.
//
//      - Interrupt Service Response Handler
//
//////////////////////////////////////////////////

#pragma interrupt(irq_13_handler)
void
irq_13_handler(void)
{
    ++irqErrorCount;

    NonSpecificEOI();
}


//////////////////////////////////////////////////
//
//  UNUSED
//
//  External Interrupt INT7. Port is not used
//  Pin is multiplexed.
//
//      - Interrupt Service Response Handler
//
//////////////////////////////////////////////////

#pragma interrupt(irq_14_handler)
void
irq_14_handler(void)
{
    ++irqErrorCount;

    NonSpecificEOI();
}


//////////////////////////////////////////////////
//
//  WATCH DOG
//
//      - Interrupt Service Response Handler
//
//////////////////////////////////////////////////

#pragma interrupt(irq_15_handler)
void
irq_15_handler(void)
{


    NonSpecificEOI();
}


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  Redefine exit functions to reduce code size
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

extern "C" {

    void    exit(int i) { }
    int     atexit(void) { return 0; }
}






























