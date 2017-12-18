


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



//////////////////////////////////////////////////
//
//  386EX
//
//////////////////////////////////////////////////


BYTE    _IRQ_SlaveBase_ = 0x30;
BYTE    _IRQ_MstrBase_  = 0x20;
BYTE    _CascadeBits_   = 0x4;


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
//  386EX
//
//      Init_CSU
//
//////////////////////////////////////////////////

void Init_CSU(void)
{
    // MEMORY
    // Configure Upper Chip Select UCS
    // BOOT ROM
    // Region Size 65536 Bytes, 8 Bit Data Port
    // 15 Wait States

    _SetEXRegWord(UCSADL, 0x010f);
    _SetEXRegWord(UCSADH, 0x0000);
    _SetEXRegWord(UCSMSKL, 0xfc01);
    _SetEXRegWord(UCSMSKH, 0x03ff);

    // MEMORY
    // Configure Chip Select CS0, CS1, CS2, CS3
    // 4 LOCAL STAIC RAM BANKS
    // Region Size 128K x 16 - 16 Bit Data Port
    // 1 Wait States

    _SetEXRegWord(CS0ADL, 0x0301);  // CS0 = 0x0 - 0x1ffff
    _SetEXRegWord(CS0ADH, 0x0000);
    _SetEXRegWord(CS0MSKL, 0xfc01);
    _SetEXRegWord(CS0MSKH, 0x0001);

    _SetEXRegWord(CS1ADL, 0x0301);  // CS1 = 0x20000 - 0x3ffff
    _SetEXRegWord(CS1ADH, 0x0002);
    _SetEXRegWord(CS1MSKL, 0xfc01);
    _SetEXRegWord(CS1MSKH, 0x0001);

    _SetEXRegWord(CS2ADL, 0x0301);  // CS2 = 0x40000 - 0x5ffff
    _SetEXRegWord(CS2ADH, 0x0004);
    _SetEXRegWord(CS2MSKL, 0xfc01);
    _SetEXRegWord(CS2MSKH, 0x0001);

    _SetEXRegWord(CS3ADL, 0x0301);  // CS3 = 0x60000 - 0x7ffff
    _SetEXRegWord(CS3ADH, 0x0006);
    _SetEXRegWord(CS3MSKL, 0xfc01);
    _SetEXRegWord(CS3MSKH, 0x0001);

    // IO
    // Configure Chip Select CS4
    // MOTOR CONTROL CHIP SET
    // Region Size 2 Bytes, 8 Bit Data Port
    // 1 Wait States

    _SetEXRegWord(CS4ADL, 0x0402);  // CS4 = 0x300 - 0x301
    _SetEXRegWord(CS4ADH, 0x000c);
    _SetEXRegWord(CS4MSKL, 0x0401);
    _SetEXRegWord(CS4MSKH, 0x0001);

    // MEMORY
    // Configure Chip Select CS5
    // MAIL BOX IN
    // Region Size 2 KBytes - 16 Bit Data Port
    // 2 Wait States

    _SetEXRegWord(CS5ADL, 0x0301);  // 0xA0000 - 0xA03ff
    _SetEXRegWord(CS5ADH, 0x000a);
    _SetEXRegWord(CS5MSKL, 0x0401);
    _SetEXRegWord(CS5MSKH, 0x0000);

    // MEMORY
    // Configure Chip Select CS6
    // MAIL BOX OUT
    // Region Size 2 KBytes - 16 Bit Data Port
    // 1 Wait States

    _SetEXRegWord(CS6ADL, 0x0301);  // 0xB0000 - 0xB03ff
    _SetEXRegWord(CS6ADH, 0x000b);
    _SetEXRegWord(CS6MSKL, 0x0401);
    _SetEXRegWord(CS6MSKH, 0x0000);
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
//  ASSUMPTIONS:
//      Processor is in real mode.
//
//  REAL/PROTECTED MODE:
//      The address calculation from ptMemory
//      assumes the processor is in real mode.
//
//
//////////////////////////////////////////////////

void    SetDMATargMemAddr(BYTE nChannel, void *ptMemory)
{
   BYTE addrDMATar0_1, addrDMATar2, addrDMATar3;

   WORD wSegment, wOffset;

   DWORD lAddress;

       // Set registers to correct channel

        /*
       addrDMATar0_1 = ( nChannel == DMA_Channel0 ? DMA0TAR0_1 : DMA1TAR0_1);
       addrDMATar2 = ( nChannel == DMA_Channel0 ? DMA0TAR2 : DMA1TAR2);
       addrDMATar3 = ( nChannel == DMA_Channel0 ? DMA0TAR3 : DMA1TAR3);
        */


       /*
                                                // If in tiny, small, or medium model,
       #if defined(M_I86TM) || defined(M_I86SM) || defined(M_I86MM)
          _asm
          {                                     // ...then grab our segment from DS
             mov ax, ds
             mov wSegment, ds
          }
          wOffset = (WORD) ptMemory;            // ...and our offset from the pointer
       #else                                    // Else, if in compact, large, or huge memory model
          wSegment = FP_SEG(ptMemory);          // ...grab the segment from the pointer
          wOffset = FP_OFF(ptMemory);           // ...and the offset from the pointer
       #endif                                   // Assuming real mode, compute our physical address

          lAddress = ((DWORD) wSegment << 4) + wOffset;

        */

        _SetEXRegByte(DMACLRBP, 0x0);                                   // Clear the byte pointer flip-flop

        _SetEXRegByte(addrDMATar0_1, (BYTE) (lAddress & 0xFF));         // Write target address, bits 0-7

        _SetEXRegByte(addrDMATar0_1, (BYTE) ((lAddress >> 8) & 0xFF));  // Write target address, bits 8-15

        _SetEXRegByte(addrDMATar2, (BYTE) ((lAddress >> 16) & 0xFF));   // Write target address, bits 16-23

        _SetEXRegByte(addrDMATar3, (BYTE) ((lAddress >> 24) & 0x03));   // Write target address, bits 24-25
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
//  ASSUMPTIONS:
//      Processor is in real mode.
//
//  REAL/PROTECTED MODE:
//      The address calculation from ptMemory
//      assumes the processor is in real mode.
//
//
//////////////////////////////////////////////////

void    SetDMAXferCount(BYTE nChannel, DWORD lCount)
{
   BYTE addrDMAByc0_1;
   BYTE addrDMAByc2;

        // Set registers to correct channel

        /*
        addrDMAByc0_1 = (nChannel == DMA_Channel0 ? DMA0BYC0_1 : DMA1BYC0_1);
        addrDMAByc2 = (nChannel == DMA_Channel0 ? DMA0BYC2 : DMA1BYC2);
        */

        // Clear the byte pointer flip-flop

        _SetEXRegByte(DMACLRBP, 0x0);

        _SetEXRegByte(addrDMAByc0_1, (BYTE) ( lCount & 0xFF));          // Write count, bits 0-7

        _SetEXRegByte(addrDMAByc0_1, (BYTE) ((lCount >> 8)  & 0xFF));   // Write count, bits 8-15

        _SetEXRegByte(addrDMAByc2, (BYTE) ((lCount >> 16) & 0xFF));     // Write count, bits 16-23
}


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
//  Set the requester to an I/O port address, wIO
//  for the DMA channel specified by nChannel.
//
//////////////////////////////////////////////////

void SetDMAReqIOAddr(BYTE nChannel, WORD wIO)
{
   BYTE addrDMAReq0_1;
   BYTE addrDMAReq2_3;

       // Set registers to correct channel

       /*
       addrDMAReq0_1 = ( nChannel == DMA_Channel0 ? DMA0REQ0_1 : DMA1REQ0_1);
       addrDMAReq2_3 = ( nChannel == DMA_Channel0 ? DMA0REQ2_3 : DMA1REQ2_3);
       */

       _SetEXRegByte(DMACLRBP, 0x0);             // Clear the byte pointer flip-flop
                                                 // Write requester I/O address, bits 0-7

       _SetEXRegByte(addrDMAReq0_1, (BYTE) (wIO & 0xFF));   // Write requester I/O address, bits 8-15
       _SetEXRegByte(addrDMAReq0_1, (BYTE) ((wIO >> 8) & 0xFF));

       _SetEXRegByte(addrDMAReq2_3, 0x00);       // Zero requester address bits 16-23
       _SetEXRegByte(addrDMAReq2_3, 0x00);       // Zero requester address bits 24-25
}


//////////////////////////////////////////////////
//
//  386EX
//
//  Initialize DMA for SSIO Interrupt to transfer
//  measurment data directly to memory
//
//////////////////////////////////////////////////

const   BYTE    DMA1_ACK_MASK               = 0x08;
const   BYTE    DMA1_SSIO_RCV_BUFF_FULL     = 0x07;

const   BYTE    CS_DMA1                     = 0x00;
const   BYTE    DMA1_TRANSFER_DIRECTION     = 0x04;

void    InitDMAForSSIOToMemory(void)
{
    BYTE    regDMACfg, regDMAIE, regDMAOvfE;

        /*

        DisableDMAHWRequests(DMA_Channel1);

        regDMACfg =
            (_GetEXRegByte(DMACFG)
                | DMA1_SSIO_RCV_BUFF_FULL
                | DMA1_ACK_MASK);

        _SetEXRegByte(DMACFG, regDMACfg);

        _SetEXRegByte(DMAMOD1,
            (CS_DMA1
                | DMA1_TRANSFER_DIRECTION
                | ));

        _SetEXRegByte(DMAMOD2, );

        _SetEXRegByte(DMABSR, );

        _SetEXRegByte(DMACHR, );


        regDMAIE = (_GetEXRegByte(DMAIEN) & 0x01);

        _SetEXRegByte(DMAIEN, regDMAIE);


        regDMAOvfE = (_GetEXRegByte(DMAOVFE) | 0x0C);

        _SetEXRegByte(DMAOVFE, regDMAOvfE);

        // Set Requester I/O Address to SSIO Receiver

        SetDMAReqIOAddr(DMA_Channel1, SSIORBUF);

        */
}




