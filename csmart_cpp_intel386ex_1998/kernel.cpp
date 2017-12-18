




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

//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////


StateMachine::StateMachine(BYTE smId)
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

BYTE
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
// Interrupt Flags - Private
//
//////////////////////////////////////////////////

BOOL    TIMER_0_IRQ_SET = FALSE,

        MOTOR_CS_IRQ_SET = FALSE;





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

BYTE    GetStateMachineState(BYTE  smId) {

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

        currStateMachine = sysSMEntries[KernelID];

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

    }


}



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    SendHiPrMsg(BYTE smId, WORD mId) {

    lastHPMsg->smDestId = smId;
    lastHPMsg->senderId = currStateMachine->GetStateMachineId();
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

void    SendLowPrMsg(BYTE smId, WORD mId) {

    lastLPMsg->smDestId = smId;
    lastLPMsg->senderId = currStateMachine->GetStateMachineId();
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

void    SendHiPrMsg(BYTE smId, WORD mId, DWORD d1, DWORD d2) {

    lastHPMsg->smDestId = smId;
    lastHPMsg->senderId = currStateMachine->GetStateMachineId();
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

void    SendLowPrMsg(BYTE smId, WORD mId, DWORD d1, DWORD d2) {

    lastLPMsg->smDestId = smId;
    lastLPMsg->senderId = currStateMachine->GetStateMachineId();
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

void    StartTimer(WORD tmrCount)
{
    systemTimers[currStateMachine->GetStateMachineId()].timerCount
        = tmrCount;

    systemTimers[currStateMachine->GetStateMachineId()].hiPriority
        = FALSE;
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    StartHiPriorityTimer(WORD tmrCount)
{
    systemTimers[currStateMachine->GetStateMachineId()].timerCount
        = tmrCount;

    systemTimers[currStateMachine->GetStateMachineId()].hiPriority
        = TRUE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    CancelTimer(void)
{
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
                    SendHiPrMsg(currSmTimer, TimeOut);
                else
                    SendLowPrMsg(currSmTimer, TimeOut);
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
    WORD    csStatus;

    // Set Axis to the one interrupting

    SendMcsCommand(SET_CURR_AXIS_INTERRUPT);
    SendMcsCommand(GET_STATUS);

    csStatus = ReadMcs();

    if(csStatus & X_AXIS_BIT) // X Axis Interrupt
    {
        // Check for all the possilbe events
        // inform the appropriate machines about the event

        if(csStatus & MOTION_COMPLETE)
            SendHiPrMsg(MotorCommID, XAxisOnTarget);

        if(csStatus & NEGATIVE_LIMIT_SWITCH)
            SendHiPrMsg(MotorCommID, XLimitFound);

        if(csStatus & UPDATE_BREAK_POINT_REACHED)
            SendHiPrMsg(HeadMachinesManagerID, XBreakPointReached);
    }
    else // Y AXIS Interrupt
    {
        if(csStatus & MOTION_COMPLETE)
            SendHiPrMsg(MotorCommID, YAxisOnTarget);

        if(csStatus & NEGATIVE_LIMIT_SWITCH)
            SendHiPrMsg(MotorCommID, YLimitFound);

        if(csStatus & UPDATE_BREAK_POINT_REACHED)
            SendHiPrMsg(HeadMachinesManagerID, YBreakPointReached);
    }
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
    //  Initialize Chip Select Units
    //
    //////////////////////////////////////////////////

    Init_CSU();


    //////////////////////////////////////////////////
    //
    //  Initialize Clock PMU (Power Management)
    //
    //////////////////////////////////////////////////

    Init_ClockPMU(POWER_CONTROL, CLOCK_PRESCALE);


    //////////////////////////////////////////////////
    //
    //  Initialize RCU (Refresh Control unit)
    //
    //////////////////////////////////////////////////



    //////////////////////////////////////////////////
    //
    //  Initialize IO
    //
    //////////////////////////////////////////////////

    InitIO1(PORT_1_LATCH, PORT_1_DIR, PORT_1_CFG);
    InitIO2(PORT_2_LATCH, PORT_2_DIR, PORT_2_CFG);
    InitIO3(PORT_3_LATCH, PORT_3_DIR, PORT_3_CFG);


    //////////////////////////////////////////////////
    //
    //  Initialize Interrupts
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


    //////////////////////////////////////////////////
    //
    //  Initialize Async Serial
    //
    //////////////////////////////////////////////////



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
    // Make sure TX Buff is empty

    while(!(_GetEXRegByte(LSR0) & SIO_TX_BUF_EMPTY))
        ;

    _SetEXRegByte(TBR0, data);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////


#define     TRANSMIT_BUFFER_EMPTY       0x01
#define     RECEIVE_BUFFER_FULL         0x02


static  BYTE    rxBuffer[MAX_HEAD_COMM_PACKET_LEN];
static  BYTE    txBuffer[MAX_HEAD_COMM_PACKET_LEN];

static  BYTE    iir0, commStatus,

                rxByteCount=0, rxDataRcvd=0x00, rxLength,

                txByteCount=0;


#pragma interrupt(irq_4_handler)
void
irq_4_handler(void)
{
        iir0 = _GetEXRegByte(IIR0);

        commStatus = ((iir0 & 0x06) >> 1);

        //////////////////////////////////////////////////
        //
        //
        //
        //////////////////////////////////////////////////

        if(commStatus == RECEIVE_BUFFER_FULL)
        {
            rxDataRcvd = _GetEXRegByte(RBR0);

            if(rxByteCount >= MAX_HEAD_COMM_PACKET_LEN)
            {
                // Out of sync

            }
            else
            {
                rxBuffer[rxByteCount] = rxDataRcvd;
            }

            if(rxByteCount == 0) // Packet Header
            {
                rxLength = rxBuffer[rxByteCount];

                if(rxLength > MAX_HEAD_COMM_PACKET_LEN)
                {
                    // Invalid rxLength

                }
            }
            else // next packet data
            {
                if(rxLength == (rxByteCount+1))
                {
                    // Packet fully received

                }
            }

            rxByteCount++;
        }

        else

        //////////////////////////////////////////////////
        //
        //
        //
        //////////////////////////////////////////////////

        if(commStatus == TRANSMIT_BUFFER_EMPTY)
        {
            if(txByteCount == 0)
            {
                // Uart Just Enabled

                SerialWriteChar(txBuffer[txByteCount]);

                txByteCount++;
            }
            else
            {


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
//  DMA
//
//  Generates an interrupt when the full
//  block of data is received by the SSIO.
//
//      - Interrupt Service Response Handler
//
//////////////////////////////////////////////////

#define     DMA0_TRANSFER_COMPLETE      0x10
#define     DMA1_TRANSFER_COMPLETE      0x20
#define     DMA0_CHAINING_INTERRUPT     0x01
#define     DMA1_CHAINING_INTERRUPT     0x02

#pragma interrupt(irq_12_handler)
void
irq_12_handler(void)
{
    WORD    regDMAIS;

        // Get interrupt status register

        regDMAIS = _GetEXRegByte(DMAIS);

        if(regDMAIS & DMA0_TRANSFER_COMPLETE)
        {
            _SetEXRegByte(DMACLRTC, 0x00);
        }

        if(regDMAIS & DMA1_TRANSFER_COMPLETE)
        {
            _SetEXRegByte(DMACLRTC, 0x00);
        }

        if(regDMAIS & DMA0_CHAINING_INTERRUPT)
        {

        }

        //////////////////////////////////////////////////
        //
        // DMA1 is used with SSIO to receive
        // the measurement data
        //
        //////////////////////////////////////////////////

        if(regDMAIS & DMA1_CHAINING_INTERRUPT)
        {

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






























