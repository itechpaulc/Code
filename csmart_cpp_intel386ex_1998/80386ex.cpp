


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



#pragma _builtin_(saveglobaltable)
#pragma _builtin_(restoreglobaltable)

void    saveglobaltable(void *ptrTable);
void    restoreglobaltable(void *ptrTable);


//////////////////////////////////////////////////
//
//  386EX
//
//  Private Varibles and defines
//
//////////////////////////////////////////////////


static      BYTE     _IRQ_SlaveBase_     = 0x30;
static      BYTE     _IRQ_MstrBase_      = 0x20;
static      BYTE     _CascadeBits_       = 0x04;


static      WORD    SSIO_MeasurementBuffer[ATOD_MEASUREMENT_SIZE];


/////////////////////////////////////////////////////////
//
//  ACCESS to the buffer used by the
//  SSIO and the DMA
//
/////////////////////////////////////////////////////////

WORD    * GetMeasurementBuffer(void) {

    return SSIO_MeasurementBuffer;
}


//////////////////////////////////////////////////
//
//  386EX
//
//
//    InitICU
//      Description:
//      Initialization for both the master and slave Interrupt Control
//      Units (ICU).  This routine only initializes the internal interrupt
//      controllers, external ICUs must be initialized separatly. These
//      should be initialized before interrupts are enabled (i.e. enable()).
//
//   Parameters:
//      MstrMode        Mode of operation for Master ICU
//      MstrBase        Specifies the base interrupt vector number for the
//                      Master interrupts. For example, if IR1 of the master
//                      goes active and the MstrBase = 0x20 the processor
//                      will use interrupt vector table entry 0x21.
//      MstrCascade     Which Master IRQs are used for Slave ICUs.
//      SlaveMode       Mode of operation for Slave ICU
//      SlaveBase       Specifies the base interrupt vector number for the
//                      Slave interrupts. For example, if IR1 of the slave
//                      goes active and the SlaveBase = 0x40 the processor
//                      will use interrupt vector table entry 0x41.
//       MstrPins       Defines what EX pins are avaliable externally
//                      to the chip for the Master.
//       SlavePins      Defines what EX pins are avaliable externally
//                      to the chip for the Slave.
//
//   Returns: Error Code
//          E_OK  -- Initialized OK, No error.
//
//   Assumptions:
//      REMAPCFG register has Expanded I/O space access enabled (ESE bit set).
//
//   Real/Protected Mode
//      No changes required.
//
//////////////////////////////////////////////////

int
InitICU(BYTE MstrMode, BYTE MstrBase, BYTE MstrCascade,
        BYTE SlaveMode, BYTE SlaveBase,
        BYTE MstrPins, BYTE SlavePins)
{
    BYTE   icw, cfg_pins;

        //  Program Slave ICU

        _IRQ_SlaveBase_ = SlaveBase & 0xf8;
        _SetEXRegByte(ICW1S, 0x11 | SlaveMode);         // Set slave triggering
        _SetEXRegByte(ICW2S, _IRQ_SlaveBase_);          // Set slave base interrupt type, least 3-bit must be 0
        _SetEXRegByte(ICW3S, 0x2);                      // Set slave ID
        _SetEXRegByte(ICW4S, 0x1);                      // Set bit 0 to guarantee operation


        // Program Master ICU

        _IRQ_MstrBase_ = MstrBase & 0xf8;
        _CascadeBits_  = MstrCascade | 0x4;
        icw = (MstrMode & ICU_TRIGGER_LEVEL) ?
            0x19 : 0x11;

        _SetEXRegByte(ICW1M, icw);                      // Set master triggering
        _SetEXRegByte(ICW2M, _IRQ_MstrBase_);           // Set master base interrupt type, least 3-bit must be 0
        _SetEXRegByte(ICW3M, _CascadeBits_);            // Set master cascade pins, Make sure IR2 set for Cascade
        icw = (MstrMode & ~ICU_TRIGGER_LEVEL) | 1;      // Set bit 0 and remove Trigger_level bit (part of ICW1)
        _SetEXRegByte(ICW4M, icw);                      // Set slave IDs in master


        // Program chip configuration registers

        cfg_pins = _GetEXRegByte(INTCFG);

        if( (MstrCascade & 0xfb) != 0 )                 // bit 2 (IR2) is internal, external signals not required for just IR2
            cfg_pins |= 0x80;                           // Using external slaves, therefore enable Cascade signals

        cfg_pins |= SlavePins;
        _SetEXRegByte(INTCFG, SlavePins);               // Set Slave external interrupt pins
        cfg_pins = _GetEXRegByte(P3CFG);                // Preserve other set bits
        _SetEXRegByte(P3CFG, cfg_pins | MstrPins);      // Set Master external interrupt pins

  return E_OK;
}


//////////////////////////////////////////////////
//
//  386EX
//
//  InitICUSlave
//      Description:
//      Initialization only the internal slave Interrupt Control
//      Units (ICU).  This routine only initializes the internal
//      interrupt controller, external ICUs must be
//      initialized separately.
//
//   Parameters:
//      SlaveMode       Mode of operation for Slave ICU
//      SlaveBase       Specifies the base interrupt vector number
//                      for the Slave interrupts. For example, if
//                      IR1 of the slave goes active and the
//                      SlaveBase = 0x40 the processor will use
//                      interrupt vector table entry 0x41.
//       SlavePins     Defines what EX pins are avaliable externally to the chip for the Slave.
//
//   Returns: Error Code
//      E_OK  -- Initialized OK, No error.
//
//   Assumptions:
//      REMAPCFG register has Expanded I/O space access enabled (ESE bit set).
//
//   Real/Protected Mode
//      No changes required.
//
//////////////////////////////////////////////////

int InitICUSlave(BYTE SlaveMode, BYTE SlaveBase, BYTE SlavePins)
{
    BYTE   cfg_pins;

    // Program Slave ICU

        _IRQ_SlaveBase_ = SlaveBase & 0xf8;
        _SetEXRegByte(ICW1S, 0x11 | SlaveMode);         // Set slave triggering
        _SetEXRegByte(ICW2S, _IRQ_SlaveBase_);          // Set slave base interrupt type, least 3-bit must be 0
        _SetEXRegByte(ICW3S, 0x2);                      // Set slave ID
        _SetEXRegByte(ICW4S, 0x1);                      // Set bit 0 to guarantee operation
        cfg_pins = _GetEXRegByte(INTCFG);
        cfg_pins |= SlavePins;
        _SetEXRegByte(INTCFG, SlavePins);               // Set Slave external interrupt pins

  return E_OK;
}



//////////////////////////////////////////////////
//
//  386EX
//
//
// Disable8259Interrupt
//   Description:
//      Disables 8259a interrupts for the master and the slave.
//
//   Parameters:
//      MstrMask        Mask value for mster ICU
//      SlaveMask       Mask value for slave ICU
//
//      Each bit location that is set will disable the corresponding
//      interrupt (by setting the bit in the interrupt control
//      register).  For example, to disable master IR3 and IR5 set
//      MstrMask = 0x28 (bits 3 and 5 are set).
//
//   Returns: None
//
//   Assumptions:
//      REMAPCFG register has Expanded I/O space access enabled (ESE bit set).
//
//   Real/Protected Mode
//      No changes required.
//
//////////////////////////////////////////////////

void Disable8259Interrupt(BYTE MstrMask, BYTE SlaveMask)
{
    BYTE Mask;

    if(MstrMask != 0)
    {
        Mask = _GetEXRegByte(OCW1M);
        _SetEXRegByte(OCW1M, Mask | MstrMask);
    }

    if(SlaveMask != 0)
    {
        Mask = _GetEXRegByte(OCW1S);
        _SetEXRegByte(OCW1S, Mask | SlaveMask);
    }
}


//////////////////////////////////////////////////
//
//  386EX
//
//
//  Enable8259Interrupt
//      Description:
//      Enables 8259a interrupts for the master and the slave.
//
//   Parameters:
//      MstrMask        Enable mask value for mster ICU
//      SlaveMask       Enable mask value for slave ICU
//
//      Each bit location that is set will enable the corresponding
//      interrupt (by clearing the bit in the interrupt control
//      register).  For example, to enable master IR3 and IR5 set
//      MstrMask = 0x28 (bits 3 and 5 are set).
//
//   Returns: None
//
//   Assumptions:
//      REMAPCFG register has Expanded I/O space access enabled (ESE bit set).
//
//   Real/Protected Mode
//      No changes required.
//
//////////////////////////////////////////////////

void Enable8259Interrupt(BYTE MstrMask, BYTE SlaveMask)
{
    BYTE Mask;

    if(MstrMask != 0)
    {
        Mask = _GetEXRegByte(OCW1M);
        _SetEXRegByte(OCW1M, Mask & (~MstrMask));
    }

    if(SlaveMask != 0)
    {
        Mask = _GetEXRegByte(OCW1S);
        _SetEXRegByte(OCW1S, Mask & (~SlaveMask));
    }
}


//////////////////////////////////////////////////
//
//  386EX
//
//
//  InitTimer:
//      Description:
//
//	    Parameters:
//		Unit 		Unit number of the timer.  EX supports 0, 1 or 2
//		Mode		Defines Counter Mode and Count value type
//		Inputs 		Specifies Input sources
//		Output		Specifies Which Output to drive
//		InitCount	Value to be loaded into count register
//		Enable		Enable (1) or disable (0) Timer
//
//	Returns:	Error Codes
//		E_INVAILD_DEVICE 	-- Unit number specifies a non-existing device
//		E_OK				-- Initialized OK, No error.
//
//	Assumptions:
//		REMAPCFG register has Expanded I/O space access enabled (ESE bit set).
//
//	Real/Protected Mode
//		No changes required.
//
//////////////////////////////////////////////////

int InitTimer(int Unit, WORD Mode, BYTE Inputs, BYTE Output,
                WORD InitCount, int Enable)
{
    BYTE    TmpByte;
    WORD    TmrCntPort;

        if(Unit > 2)
            return E_INVALID_DEVICE;

        TmrCntPort = 0xf040 + Unit;             // Set depending on which timer

        // Set Pin configuration

        if(Unit < 2)
        {
            TmpByte = _GetEXRegByte(P3CFG) | (Output << Unit);      // Bit 0 or 1
            _SetEXRegByte(P3CFG,TmpByte);
        }
        else
        {
            TmpByte = _GetEXRegByte(PINCFG) | (Output << 5);        // Bit 5
            _SetEXRegByte(PINCFG,TmpByte);
        }

        // Set Timer Config

        TmpByte = _GetEXRegByte(TMRCFG);        // All Timers share this register,
                                                // Keep previous settings
        if(!Enable)
            TmpByte |= 0x80;                    // Set Timer Disable Bit

        TmpByte |= (Inputs << (Unit*2));		// Set CK?CON and GT?CON bits
        _SetEXRegByte(TMRCFG,TmpByte);

        // Set Timer Control Register

        TmpByte = Unit << 6;		            // Set counter select
        TmpByte |= (0x30 | Mode);	            // Set R/W low then high
                                                // byte and Mode bits
        _SetEXRegByte(TMRCON,TmpByte);

        // Set Initial Counter Value

        TmpByte = HIBYTE(InitCount);
        _SetEXRegByte(TmrCntPort, LOBYTE(InitCount));
        _SetEXRegByte(TmrCntPort, TmpByte);

	return E_OK;
}


//////////////////////////////////////////////////
//
//  386EX
//
//      SetupWDT
//
//////////////////////////////////////////////////

/*
void SetupWDT(BYTE Status)
{
   _SetEXRegByte(WDSTATUS, Status);
}
*/


//////////////////////////////////////////////////
//
//  386EX
//
//      InitIO1
//
//      Parameters:
//
//      Pn_LATCH    - Port Data Latch Output Value
//      Pn_DIR      - Direction (0 - OUTPUT) (1 - INPUT)
//      Pn_CFG      - Mode (0 - IO Mode) (1 - Perhiperal Mode)
//
//////////////////////////////////////////////////

void InitIO1(BYTE P1_LATCH, BYTE P1_DIR, BYTE P1_CFG)
{
    // port 1 init

    _SetEXRegByte(P1LTC, P1_LATCH);
    _SetEXRegByte(P1DIR, P1_DIR);
    _SetEXRegByte(P1CFG, P1_CFG);
}


//////////////////////////////////////////////////
//
//  386EX
//
//      InitIO2
//
//      Pn_LATCH    - Port Data Latch Output Value
//      Pn_DIR      - Direction (0 - OUTPUT) (1 - INPUT)
//      Pn_CFG      - Mode (0 - IO Mode) (1 - Perhiperal Mode)
//
//////////////////////////////////////////////////

void InitIO2(BYTE P2_LATCH, BYTE P2_DIR, BYTE P2_CFG)
{
    /// port 2 init

    _SetEXRegByte(P2LTC, P2_LATCH);
    _SetEXRegByte(P2DIR, P2_DIR);
    _SetEXRegByte(P2CFG, P2_CFG);
}


//////////////////////////////////////////////////
//
//  386EX
//
//      InitIO3
//
//      Pn_LATCH    - Port Data Latch Output Value
//      Pn_DIR      - Direction (0 - OUTPUT) (1 - INPUT)
//      Pn_CFG      - Mode (0 - IO Mode) (1 - Perhiperal Mode)
//
//////////////////////////////////////////////////

void InitIO3(BYTE P3_LATCH, BYTE P3_DIR, BYTE P3_CFG)
{
    // port 3 init

    _SetEXRegByte(P3LTC, P3_LATCH);
    _SetEXRegByte(P3DIR, P3_DIR);
    _SetEXRegByte(P3CFG, P3_CFG);
}


//////////////////////////////////////////////////
//
//  386EX
//
//      Init_RCU
//
//////////////////////////////////////////////////

void Init_RCU(BYTE Value1, BYTE Value2, BYTE Value3, BYTE Value4)
{
    _SetEXRegWord(RFSCIR, Value1);    // Program refresh interval.
    _SetEXRegWord(RFSBAD, Value2);    // Program base address.
    _SetEXRegWord(RFSADD, Value3);    // Program address register.
    _SetEXRegWord(RFSCON, Value4);    // Program the enable bit.
}


//////////////////////////////////////////////////
//
//  386EX
//
//      Init_ClockPMU
//
//////////////////////////////////////////////////

void Init_ClockPMU(BYTE PowerCont, BYTE ClockPrescale)
{
    _SetEXRegByte(PWRCON, PowerCont);
    _SetEXRegWord(CLKPRS, ClockPrescale);
}


//////////////////////////////////////////////////
//
// 386EX DMA CONTROL CODE
//
// As shown in the : Intel 386 EX
// Pin Multiplexing Schematic Diagram
// RBF (Receive Buffer Full) of SSIO is Connected
// to DMA1. This is why DMA 1 was selected
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
//  386EX
//
//  This function enables the DMA and initializes
//  settings independent of the  two channels:
//
//         bus arbitration - set to no rotation, external
//                           bus master request(HOLD) assigned
//                           to lowest priority level
//
//         EOP# sampling - set to asynch. (no effect when DMA
//                         is used with internal peripherals)
//
//         DRQn sampling - set to synch. (no effect when DMA
//                         is used with internal peripherals)
//
//
//////////////////////////////////////////////////

void InitDMA(void)
{
   _SetEXRegByte(DMACLR, 0x0);               // Resets DMA peripheral
   _SetEXRegByte(DMACMD1, 0x0);              // DMACMD1[7:5]=0: reserved
                                             // DMACMD1[4]=0: disable priority rotation enable
                                             // DMACMD1[2]=0: enable channel's 0 and 1
                                             // DMACMD1[1:0]=0: reserved

   _SetEXRegByte(DMACMD2, 0x8);              // DMACMD2[7:4]=0: reserved
                                             // DMACMD2[3:2]=2: assign HOLD to the lowest
                                             //    priority level
                                             // DMACMD2[1]=0:  EOP# samples input async.
                                             // DMACMD2[0]=0:  DRQn samples input async.
}


//////////////////////////////////////////////////
//
//  386EX
//
//  This function disables channel hardware
//  requests for the given DMA channel.
//  The channel, however, can still receive
//  software requests.
//
//////////////////////////////////////////////////

void    DisableDMAHWRequests(BYTE nChannel)
{
    BYTE regDMAMSK = 0;

        regDMAMSK = nChannel;               // Set regDMAMSK[CS] to channel
        regDMAMSK &= 0x04;                  // Set regDMAMSK[HRM]

        _SetEXRegByte(DMAMSK, regDMAMSK);   // Set hw request mask for given
                                            // channel
}


//////////////////////////////////////////////////
//
//  386EX
//
//  This function enables channel hardware
//  requests for the given DMA channel.
//
//////////////////////////////////////////////////

void    EnableDMAHWRequests(BYTE nChannel)
{
    BYTE regDMAMSK = 0;                     // Clear regDMAMSK[HRM]

        regDMAMSK = nChannel;               // Set regDMAMSK[CS] to channel
        _SetEXRegByte(DMAMSK, regDMAMSK);   // Clear hardware request mask for
                                            // given channel
}


//////////////////////////////////////////////////
//
//  386EX
//
//  Initialize DMA for SSIO Interrupt to transfer
//  the Probe Heaad's Measurment Data
//  directly to a memory buffer
//
//////////////////////////////////////////////////

#define     DMA1_ACK_MASK                   (BYTE)0x80
#define     DMA1_SSIO_RCV_BUFF_FULL         (BYTE)0x30

#define     CS_DMA1                         (BYTE)0x01
#define     DMA1_DIR_REQUESTER_TO_TARGET    (BYTE)0x04
#define     DMA1_AUTO_INITIALIZE            (BYTE)0x08

#define     DMA1_REQUESTER_ADDRESS_HOLD     (BYTE)0x08
#define     DMA1_REQUESTER_IN_IO_SPACE      (BYTE)0x40

#define     DMA0_CFG_MASK                   (BYTE)0x0F

#define     DMA1_DMAIEN_RESERVED_MASK       (BYTE)0xFC
#define     DMA1_TRANSFER_COMPLETE_IRQ      (BYTE)0x02

#define     DMA1_DMAOVFE_RESERVED_MASK      (BYTE)0xF0
#define     DMA1_TOV1                       (BYTE)0x04


void    InitDMAForSSIOToMemory(void)
{
    BYTE    regDMACfg, regDMAIE, regDMAOvfE;

        InitDMA();

        DisableDMAHWRequests(DMA_Channel1);

        //////////////////////////////////////////////////
        // SET DMA Config Register
        // Dont Touch DMA 0, clear DMA 1 config reading
        // Set DMA 1 config to : SSIO(SSRBF)
        // : ACK MASK since DREQ1 is connected to an internal peripheral
        //////////////////////////////////////////////////

        regDMACfg = ( (_GetEXRegByte(DMACFG) & DMA0_CFG_MASK)
                        | DMA1_SSIO_RCV_BUFF_FULL | DMA1_ACK_MASK);

        _SetEXRegByte(DMACFG, regDMACfg);


        //////////////////////////////////////////////////
        // Set CHANNEL SELECT (CS) = Channel 1, DMA1
        // Set TRANSFER DIRECTTION = REQUESTER_TO_TARGET
        // Set to Enable Auto Initialize, DMAMOD1[BIT 4] = 1
        // Increment Target's Address, DMAMOD1[BIT 5] = 0
        // Set Transfer Mode to DEMAND MODE, DMAMOD1[BIT 6,7] = 0
        // Change Mode1 Register
        //////////////////////////////////////////////////

        _SetEXRegByte(DMAMOD1,
            (CS_DMA1 | DMA1_DIR_REQUESTER_TO_TARGET | DMA1_AUTO_INITIALIZE));


        //////////////////////////////////////////////////
        // SET Mode2 Register
        // Set CHANNEL SELECT (CS) = Channel 1, DMA1
        // Do Not HOLD Target Address, DMAMOD2[2] = 0
        // HOLD The requester Address = DMA1_REQUESTER_ADDRESS_HOLD, the
        //      SSIO address remains the same as the requester
        // Target Device Type is in Memory Space, DMAMOD2[5] = 0
        // Requester Device Type = DMA1_REQUESTER_IN_IO_SPACE
        // Bus Cycle Option used is the Fly-By Mode, DMAMOD2[7] = 0
        //////////////////////////////////////////////////

        _SetEXRegByte(DMAMOD2,
            (CS_DMA1 | DMA1_REQUESTER_ADDRESS_HOLD | DMA1_REQUESTER_IN_IO_SPACE));


        //////////////////////////////////////////////////
        // SET Bus Size Register
        // Set CHANNEL SELECT (CS) = Channel 1, DMA1
        // Set Target Bus Size 16 Bit, DMABSR[4] = 0
        // Set Target Bus Size 16 Bit, DMABSR[6] = 0
        //      In other words Clear the whole Register
        //////////////////////////////////////////////////

        _SetEXRegByte(DMABSR, (CS_DMA1));


        //////////////////////////////////////////////////
        // SET DMA Chaining Register
        // Set CHANNEL SELECT (CS) = Channel 1, DMA1
        // Disable Chaining buffer transfers (CE),
        //      DMACHR[2] = 0
        //      In other words Clear the whole Register
        //////////////////////////////////////////////////

        _SetEXRegByte(DMACHR, (CS_DMA1));


        //////////////////////////////////////////////////
        // SET Interrupt Enable Register
        // Do not touch the reserved bits, DMAIEN[2..7]
        // Clear DMA Channel 0, Transfer Complete
        //      TC0, DMAIEN[0] = 0
        // Set DMA Channel 1, Transfer Complete TC1,
        //      DMAIEN[1] = DMA1_TRANSFER_COMPLETE_IRQ
        //////////////////////////////////////////////////

        regDMAIE = ((_GetEXRegByte(DMAIEN) & DMA1_DMAIEN_RESERVED_MASK)
                        | DMA1_TRANSFER_COMPLETE_IRQ);

        _SetEXRegByte(DMAIEN, regDMAIE);


        //////////////////////////////////////////////////
        // SET Overflow Enable Register
        // Do not touch the reserved bits, DMAOVFE[4..7]
        // Set Target Over Flow Enable to use 26 Bits,
        //      to allow the Measurement Buffer to be
        //      placed outside of the 64 K boundary
        //      DMAOVFE[2] = DMA1_TOV1
        // Does not care for the requester's register.
        //      The requester will always be the
        //      SSIO peripheral
        //////////////////////////////////////////////////

        regDMAOvfE = ((_GetEXRegByte(DMAOVFE) & DMA1_DMAOVFE_RESERVED_MASK) | DMA1_TOV1 );

        _SetEXRegByte(DMAOVFE, regDMAOvfE);


        //////////////////////////////////////////////////
        // Set Requester I/O Address to SSIO Receiver
        //////////////////////////////////////////////////

        SetDMAReqIOAddr(DMA_Channel1, SSIORBUF);


        //////////////////////////////////////////////////
        // Set Target Memory Address
        //////////////////////////////////////////////////

        SetDMATargetMemAddr(DMA_Channel1, SSIO_MeasurementBuffer);


        //////////////////////////////////////////////////
        // Set Transfer Byte Count
        //////////////////////////////////////////////////

        SetDMAXferCount(DMA_Channel1, SSIO_DMA_TRANSFER_BYTE_COUNT);
}


//////////////////////////////////////////////////
//
//  386EX
//
//  Set the requester to an I/O port address, wIO
//  for the DMA channel specified by nChannel.
//
//////////////////////////////////////////////////

void SetDMAReqIOAddr(BYTE nChannel, WORD wIO)
{
   WORD addrDMAReq0_1;
   WORD addrDMAReq2_3;

        // Set registers to correct channel

        addrDMAReq0_1 = ( nChannel == DMA_Channel0 ? DMA0REQ0_1 : DMA1REQ0_1);
        addrDMAReq2_3 = ( nChannel == DMA_Channel0 ? DMA0REQ2_3 : DMA1REQ2_3);

        // Clear the byte pointer flip-flop. Reset BP to Known
        // State (0) before writing to the Channnel Registers, Requester's Address

        _SetEXRegByte(DMACLRBP, 0x0);


        // Write requester I/O address, bits 8-15

        _SetEXRegByte(addrDMAReq0_1, (BYTE) (wIO & 0xFF));
        _SetEXRegByte(addrDMAReq0_1, (BYTE) ((wIO >> 8) & 0xFF));

        _SetEXRegByte(addrDMAReq2_3, 0x00);       // Zero requester address bits 16-23
        _SetEXRegByte(addrDMAReq2_3, 0x00);       // Zero requester address bits 24-25
}


//////////////////////////////////////////////////
//
//  386EX
//
//  This function sets the target memory address
//  for the DMA channel specified by nChannel.
//
//  PARAMETERS:
//
//      nChannel - channel for which to set target address
//      ptMemory - pointer to target memory location
//
//////////////////////////////////////////////////

#define     SIX_BYTE_BUFFER         6
#define     EIGHT_BYTE_BUFFER       8

#define     FP_OFF(fp)  (*((DWORD far *) & (fp) + 1))

void    SetDMATargetMemAddr(BYTE nChannel, void far * ptMemory)
{
    /*

    WORD    addrDMATar0_1, addrDMATar2, addrDMATar3;

        // Determine the physical location as pointed by "ptMemory"
        // while the processor is already in Protected Mode

        WORD     wOffset  = FP_OFF(ptMemory);   // : 16 bit


        BYTE    globalTable[SIX_BYTE_BUFFER];

                saveglobaltable(globalTable);   // Fill up the 6-byte buffer


        // Build the GDT base address, extracting data from the 6-byte buffer

        DWORD   globalTableBaseAddress = (DWORD)
                    ((globalTable[2]) | (globalTable[3] << 8) |
                     (globalTable[4] << 16) | (globalTable[5] << 24));


        // Determine which table among the GDT is used, through the offset

        BYTE    * descriptorTableBaseAddress = (BYTE *)(globalTableBaseAddress + wOffset);


        // Copy the descriptor table

        BYTE    dataSegmentDescriptor[EIGHT_BYTE_BUFFER];

                dataSegmentDescriptor[0] = *(descriptorTableBaseAddress);
                dataSegmentDescriptor[1] = *(descriptorTableBaseAddress+1);
                dataSegmentDescriptor[2] = *(descriptorTableBaseAddress+2);
                dataSegmentDescriptor[3] = *(descriptorTableBaseAddress+3);
                dataSegmentDescriptor[4] = *(descriptorTableBaseAddress+4);
                dataSegmentDescriptor[5] = *(descriptorTableBaseAddress+5);
                dataSegmentDescriptor[6] = *(descriptorTableBaseAddress+6);
                dataSegmentDescriptor[7] = *(descriptorTableBaseAddress+7);

        // Build the Base Address, extracting data from dataSegmentDescriptor

        DWORD   dataSegmentBaseAddress;

        DWORD   longAddress;


            longAddress =

            // Set registers to correct channel

            addrDMATar0_1   = ( nChannel == DMA_Channel0 ? DMA0TAR0_1 : DMA1TAR0_1);
            addrDMATar2     = ( nChannel == DMA_Channel0 ? DMA0TAR2 : DMA1TAR2);
            addrDMATar3     = ( nChannel == DMA_Channel0 ? DMA0TAR3 : DMA1TAR3);


            // Clear the byte pointer flip-flop. Reset BP to Known
            // State (0) before writing to the Channnel Registers, Target Address

            _SetEXRegByte(DMACLRBP, 0x0);


            // Write target address, bits 0-7, BYTE 0,
            // NOTE: BP is automatically incremented to the next

            _SetEXRegByte(addrDMATar0_1, (BYTE)(longAddress & 0xFF));

            // Write target address, bits 8-15, BYTE 1

            _SetEXRegByte(addrDMATar0_1, (BYTE)((longAddress >> 8) & 0xFF));

            // Write target address, bits 16-23, BYTE 2

            _SetEXRegByte(addrDMATar2, (BYTE)((longAddress >> 16) & 0xFF));

            // Write target address, bits 24-25, BYTE 3

            _SetEXRegByte(addrDMATar3, (BYTE)((longAddress >> 24) & 0x03));

    */
}



//////////////////////////////////////////////////
//
//  386EX
//
//  This function sets the target memory
//  device for the DMA channel specified
//  by nChannel.
//
//  PARAMETERS:
//
//      nChannel - channel for which to set target address
//      ptMemory - pointer to target memory location
//
//////////////////////////////////////////////////

void    SetDMAXferCount(BYTE nChannel, DWORD lCount)
{
    WORD addrDMAByc0_1;
    WORD addrDMAByc2;

        // Set registers to correct channel

        addrDMAByc0_1 = (nChannel == DMA_Channel0 ? DMA0BYC0_1 : DMA1BYC0_1);
        addrDMAByc2 = (nChannel == DMA_Channel0 ? DMA0BYC2 : DMA1BYC2);


        // Clear the byte pointer flip-flop. Reset BP to Known
        // State (0) before writing to the Channnel Registers, Byte Counter

        _SetEXRegByte(DMACLRBP, 0x0);


        // Write count, bits 0-7
        // NOTE: BP is automatically incremented to the next

        _SetEXRegByte(addrDMAByc0_1, (BYTE)(lCount & 0xFF));

        // Write count, bits 8-15

        _SetEXRegByte(addrDMAByc0_1, (BYTE)((lCount >> 8)  & 0xFF));

        // Write count, bits 16-23

        _SetEXRegByte(addrDMAByc2, (BYTE)((lCount >> 16) & 0xFF));
}



//////////////////////////////////////////////////
//
// 386EX ASYNC SERIAL CONTROL CODE
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
//  386EX
//
//      Initialization routine for Asynchronous Serial I/O Port.
//
//  Parameters:
//      Unit    Unit number of the serial port.  0 for SIO port 0, 1 for SIO port 1.
//      Mode    Defines Parity, number of data bits, number of stop bits... Reference
//              Serial Line Control register for various options
//
//      BaudRate    Specifies baud rate.  The baud divisor value is calculated based
//                  on clocking source and clock frequency.  The clocking frequency
//                  is set by calling the InitializeLibrary function.
//      ClockRate   Specifies the serial port clocking rate, for
//                  internal clocking = CLK2 for external = COMCLK
//
//  Returns: Error Codes
//      E_INVAILD_DEVICE    -- Unit number specifies a non-existing device
//      E_OK                -- Initialized OK, No error.
//
//  Assumptions:
//      SIOCFG  Has already been configured for Clocking source and Modem control source
//
//      REMAPCFG register has Expanded I/O space access enabled (ESE bit set).
//      The processor Port pin are initialized separately.
//
//////////////////////////////////////////////////

int InitSIO(int Unit, BYTE Mode, DWORD BaudRate, DWORD BaudClkIn)
{
   WORD SIOPortBase;
   WORD BaudDivisor;

        // Check for valid unit

        if(Unit > 1)
          return E_INVALID_DEVICE;

        // Set Port base based on serial port used

        SIOPortBase = (Unit ? SIO1_BASE : SIO0_BASE);

        // Initialized Serial Port registers
        // Calculate the baud divisor value, based on baud clocking

        BaudDivisor = (WORD)(BaudClkIn / (16*BaudRate));

        // Turn on access to baud divisor register

        _SetEXRegByte(SIOPortBase + LCR, 0x80);

        // Set the baud rate divisor register, High byte first

        _SetEXRegByte(SIOPortBase + DLH, HIBYTE(BaudDivisor));
        _SetEXRegByte(SIOPortBase + DLL, LOBYTE(BaudDivisor));

        // Set Serial Line control register
        // Sets Mode and reset the Divisor latch

        _SetEXRegByte(SIOPortBase + LCR, Mode);

   return E_OK;
}



//////////////////////////////////////////////////
//
// 386EX SYNCHRONOUS SERIAL CONTROL CODE
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
//  386EX
//
//      Must have expanded I/O space enabled
//      to initialize this peripheral.
//
//      Do Not Care about the Transmitter functions
//
//////////////////////////////////////////////////

void InitSSIO(void)
{
    WORD   temp;

        _SetEXRegByte(SSIOBAUD, 0x0);       // Configure the BRG, Disable
                                            // Baud Rate Generator

        _SetEXRegByte(SSIOCON2, 0x0);       // Control 2 Register, Set the
                                            // Receiver in slave mode

        _ReadEXRegWord(temp, SSIORBUF);     // Clear any receive buffer interrupt

        _SetEXRegByte(SSIOCON1, 0xC1);      // Configure SSIO Control 1 Register
                                            //
                                            // The Receiver is Enabled.
                                            //
                                            // The receiver interrupt is Disabled.
                                            // The transmitter is Disabled.
                                            // The transmitter interrupt is Disabled.
}





