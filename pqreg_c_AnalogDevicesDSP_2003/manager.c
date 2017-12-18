/**SDOC*************************************************************************
 *
 *  manager.c - entry point of the Printquick firmware
 *
 *  Author:  Mark Colvin			June 12, 2001
    	Author	: Manuel Jochem
    	          FiberVision GmbH
				  Jens-Otto-Krag-Strasse 11
				  D-52146 Wuerselen
    	       	  Tel.: +49 (0)2405 / 4548-21
    	       	  Fax : +49 (0)2405 / 4548-14

		(C)	FiberVision GmbH, 2000-2003
 *
 * $Header:   K:/Projects/PQCamera/PQCamera/MANAGER/manager.c_v   1.4   May 02 2003 10:31:12   APeng  $
 *
 * $Log:   K:/Projects/PQCamera/PQCamera/MANAGER/manager.c_v  $
 * 
 *    Rev 1.4   May 02 2003 10:31:12   APeng
 *  
 *
 *    Rev 1.3   Dec 30 2002 11:52:10   APeng
 * 2002-12-20: Version 0.60.6:
 *
 * added code for grayscale average made roi size multible of 8
 * added new module for loading of test images
 * added mechanism for converting images
 * added passing of features to narrowroi
 * changed 'bigger'-parameter of most AlignRoi-calls to FALSE
 * added alignment of ROIs,
 * added template function for pixel access more speed optimizations
 * NarrowRoi-bug seemly fixed, some speed optimizations
 * turned 'CheckBlobs' into external module 'chkblob'
 * small changes to reduce code size
 *
 *
 *
 *    Rev 1.2   Dec 10 2002 15:59:12   APeng
 * some work has been done to the reduced
 * ROI
 *
 *    Rev 1.1   16 Jul 2001 19:25:06   MARKC
 * Added pattern module
 *
 *    Rev 1.0   09 Jul 2001 12:39:52   MARKC
 * Checked in from initial workfile by PVCS Version Manager Project Assistant.
 *
 *
 *********************************************************************SDOCEND**/

/*=======================  I N C L U D E   F I L E S  ========================*/
#include <stdio.h>
#include <vcdefs.h>
#include <bool.h>
#include <vcrt.h>
#include <vclib.h>
#include <macros.h>

#include "config.h"
#include "statemac.h"
#include "comm.h"
#include "image.h"
#include "global.h"
#include "pqlib.h"
#include "build.h"

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

/*===========================  F U N C T I O N S  ============================*/

/*______________________________________________________________________________

    	$Id: manager.c,v 1.6 2000/09/28 10:30:18 Manuel Exp $

		Module	: PQ-Manager
        Function: administration of PQ modules
    	Language: ISO C
	    System	: VC/RT

    	Author	: Manuel Jochem
    	          FiberVision GmbH
				  Jens-Otto-Krag-Strasse 11
				  D-52146 Wuerselen
    	       	  Tel.: +49 (0)2405 / 4548-21
    	       	  Fax : +49 (0)2405 / 4548-14

		(C)	FiberVision GmbH, 2000-2003
  ______________________________________________________________________________
*/


/*----------------------------------------------------------------------------*/
/*
	POSIX function:

	#include <unistd.h>
	int access (const char *filename, int how);
    existence: how = F_OK
*/
#define access(f,h) (search(0x00, f) <= 0L)

/**FDOC*************************************************************************
 *
 *  Function:       main
 *
 *  Author:         Mark Colvin             Date: June 12, 2001
 *
 *  Description:	This is the main entrance point to the PrintQuick system
 *
 *  Parameters:
 *
 *  Returns:
 *
 *  Side effects:
 *
 *  History:
 *
 *********************************************************************FDOCEND**/
int main(int dummy, int argc, char* argv)
{
	/* declare and initialize variables */
	ERROR Error = ERROR_NONE;
	int   NewOv = (DRAMSIZ - (SizeOfOverlay / 2)) / PGSIZE;
			   /* 400000h words                    1024 words */
	/* initialize heap, DRAM, video and CTS */
	vcsetup();									/* start heap */
	setvar( ovpage, NewOv);						/* setup overlay start memory page */
	setvar( ScrLogPage, getvar(stpage) );		/* setup main image start memory page */
	ImageClearOverlay();						/* clear overlay */
    SetOvRed();									/* set overlay color to red */
	vmode(vmOvlLive);							/* set video mode to live w/overlay */
	UseGlobals();								/* placeholder for the PQ global variables */
	InitGlobals();								/* default values for PQ globals */

	/* init PLC */
	resPLC0();			/* camera ready: off during init phase */
	resPLC1();			/* camera scanning: off */
	resPLC2();			/* not used */
	resPLC3();			/* not used */

	/* RS232 */
	ClearInputBuffer();	/* clear serial input buffer */
	CommInitCts();		/* enable listening for CTS */
	resRTS();			/* not yet ready to communicate */

	/* open console windows for messages, located at 20,20 with 60 columns, 24 lines*/
	Console = copen(20, 20, 70,  24);

	/* display info messages */
	SendDbgMsg(DBG_INFO, "PrintQuick v%ld.%ld.%d - Graphics Microsystems, Inc (c)2000-2003",
		MAJOR_VERSION, MINOR_VERSION, TEST_VERSION);

	/* check if all runtime modules are present */
	if (access(MODULE_CONFIG,  F_OK) ||
		access(MODULE_STORAGE, F_OK) ||
		access(MODULE_HUNTER,  F_OK) ||
		access(MODULE_TRACKER, F_OK) ||
		access(MODULE_PATTERN, F_OK) ||
		access(MODULE_FINETUNE,F_OK) ||
		access(MODULE_THRESH,  F_OK) ||
		access(MODULE_SELFTEST,F_OK) ||
		access(MODULE_CHKBLOB, F_OK) ||
		access(MODULE_VALID,   F_OK) ||
		access(MODULE_FINDMARK,F_OK) ||
		access(MODULE_IMAGE,   F_OK)
	   )
	{
		/* fatal error */
		SendDbgMsg(DBG_ERRORS, "ERROR 10: system files not found!");
		Error = ERROR_FILE_NOT_FOUND;
        SendErrorMsg(ERROR_TYPE_FATAL_ERROR, Error );
	}
	else
	{
		/* start state machine */
		Error = PqStateMachine();
	}

	/* display info messages */
	SendDbgMsg(DBG_INFO, "PQ camera exiting" );

	/* clean up*/
	cclose(Console);

	/* reset PLC */
	resPLC0();			/* camera ready: off */
	resPLC1();			/* camera scanning: off */
	resPLC2();			/* not used */
	resPLC3();			/* not used */

	/* RS232 */
	setRTS();			/* ready to communicate*/

	/* exit to VC/RT*/
	return (int) Error;
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

