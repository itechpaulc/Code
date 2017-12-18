

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

#pragma option v;
#pragma option s3;

//////////////////////////////////////////////////
//
//
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


//////////////////////////////////////////////////
//
//
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


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

const int (*ConstructProcedures[SM_COUNT])() = {

	Construct_Kernel,
	Construct_LEDstatus,
	Construct_SPUComm,
	Construct_PCUManager,
	Construct_MotorController,
	Construct_ATODdriver,
	Construct_DigitalInMonitor,
	Construct_DOutputController
};


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	Init_Machines()
{
	for(currSM_ID=0; currSM_ID<SM_COUNT; currSM_ID++)
		SM_States[currSM_ID] = (*ConstructProcedures[currSM_ID])();
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	Init_Ports()
{
	PORT_A = PORT_A_INIT;

	DDR_PA = DDR_PA_INIT;
	DDR_PB = DDR_PB_INIT;
	DDR_PC = DDR_PC_INIT;


	PassConfigLine;	/* Pass CFG Line 	*/
					/* Turn Off Motor	*/
}


//////////////////////////////////////////////////
//
//
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

	// debug motor control

	SetMinPosition(0x0000);
	SetMaxPosition(0x0FFF);

	CLI();

	for(;;)
	{
		RunKernel();
	}
}



