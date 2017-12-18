

//
//	#include "timer.h"
//



/////////////////////////////////////////////////////////////////////////////
//
//
//	$Header:   N:/pvcs52/projects/pcu100~1/timer.c_v   1.1   May 07 1997 09:08:26   Paul L C  $
//	$Log:   N:/pvcs52/projects/pcu100~1/timer.c_v  $
//
//   Rev 1.1   May 07 1997 09:08:26   Paul L C
//Deleted functions that are expanded "inline" to reduce RAM stack usage.
//These functions are now defined in the header file.
//
//   Rev 1.0   Feb 26 1997 10:54:52   Paul L C
//Initial Revision
//
//
/////////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////
//
// Initiate OC Timer to start immediately
//
//////////////////////////////////////////////////

void    Init_Timers(void)
{
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
// This is the Timer
// Interrupt Service Routine
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

    //
	// if(TSR.ICF) { }
    //

}


