



/////////////////////////////////////////////////////////////////////////////
//
//
//	$Header:   N:/pvcs52/projects/pcu100~1/pcu100.c_v   1.3   May 02 1997 13:38:46   Paul L C  $
//	$Log:   N:/pvcs52/projects/pcu100~1/pcu100.c_v  $
//
//   Rev 1.3   May 02 1997 13:38:46   Paul L C
//Took out debug sections.
//
//   Rev 1.2   Mar 06 1997 11:07:02   Paul L C
//SWI_ISR function was added.
//
//   Rev 1.1   Mar 04 1997 15:26:46   Paul L C
//Added an ISR for the IRQ to simply just RTI.
//
//   Rev 1.0   Feb 26 1997 10:54:42   Paul L C
//Initial Revision
//
//
/////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////
//
// Emulator option to generate the
// symbol table
//
//////////////////////////////////////////////////

#pragma option s3;



//////////////////////////////////////////////////
//
// Header Files
//
//////////////////////////////////////////////////

#include "system.h"


#include "kernel.h"
#include "timer.h"
#include "ledstat.h"
#include "pcumngr.h"
#include "spucomm.h"
#include "motor.h"
#include "atod.h"
#include "digin.h"
#include "digout.h"
#include "updownsw.h"
#include "shcountr.h"


//////////////////////////////////////////////////
//
// C Source Files
//
//////////////////////////////////////////////////

#include "kernel.c"
#include "timer.c"
#include "ledstat.c"
#include "pcumngr.c"
#include "spucomm.c"
#include "motor.c"
#include "atod.c"
#include "digin.c"
#include "digout.c"
#include "updownsw.c"
#include "shcountr.c"



//////////////////////////////////////////////////
//
// State Machine Intialization
//
//////////////////////////////////////////////////

const BYTE (*ConstructProcedures[SM_COUNT])() = {

	Construct_Kernel,
	Construct_LEDstatus,
	Construct_SPUComm,
	Construct_PCUManager,
	Construct_MotorController,
	Construct_ATODdriver,
	Construct_DigitalInMonitor,
	Construct_DOutputController,
	Construct_UpDownSwMonitor,
	Construct_SheetCounter
};


//////////////////////////////////////////////////
//
// Call the constructor for each of the state
// machines.
//
//////////////////////////////////////////////////

void	Init_Machines()
{
	for(currSM_ID=0; currSM_ID<SM_COUNT; currSM_ID++)
		SM_States[currSM_ID] = (*ConstructProcedures[currSM_ID])();
}


//////////////////////////////////////////////////
//
// Initialize the I/O ports
//
//////////////////////////////////////////////////

void	Init_Ports()
{
	PORT_A = PORT_A_INIT;
	PORT_B = PORT_B_INIT;
	PORT_C = PORT_C_INIT;

	DDR_PA = DDR_PA_INIT;
	DDR_PB = DDR_PB_INIT;
	DDR_PC = DDR_PC_INIT;
}


//////////////////////////////////////////////////
//
// This is the main executive loop
//
//////////////////////////////////////////////////

void main(void)
{
_STARTUP:

   #asm
	  LDX      #$50
ClearNextByte:
	  CLR      ,x
	  INCX
	  BNE      ClearNextByte
   #endasm

	Init_Ports();
	Init_Timers();
	Init_Machines();

	CLI();

	while(1)
		RunKernel();
}


//////////////////////////////////////////////////
//
// Unused Reset Vectors
//
//////////////////////////////////////////////////

void	__IRQ_ISR() { }

void	__SWI_ISR() { }
