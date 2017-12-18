

//
//	#include "timer.h"
//


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    Init_Timers(void)
{
	/* Jump Start the O/C Timer */

	#asm
		LDX		ACNTHI 		; 3
		LDA 	ACNTLO      ; 3
		ADD 	#$1F        ; 2
		BCC 	ST1
		INCX                ; 3
ST1:
		STX     OCMPHI      ; 4
		STA     OCMPLO      ; 4

		BSET	OCIE,TCR
	#endasm
}



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    StartTimer(BYTE  ticks)
{
	sysTimers[currSM_ID] = ticks;
}




//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    CancelTimer(void)
{
	sysTimers[currSM_ID] = 0;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void __TIMER_ISR(void)
{
	if(TSR.OCF)	{

	SET_SYSTEM_TICK_FLAG;

		#asm
			LDA OCMPLO
			ADD #$f4
			TAX
			LDA OCMPHI

			ADC #$01

			STA OCMPHI
			LDA	TSR
			STX	OCMPLO
		#endasm
	}

	// if(TSR.ICF) { }

}


