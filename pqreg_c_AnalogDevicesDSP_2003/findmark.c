/**SDOC*************************************************************************
 *
 *  tracker.c - tracks the targets in the RoI
 *
 *  Author: Mark Colvin			June 12, 2001
    	Author	: Manuel Jochem
    	          FiberVision GmbH
    	          Jens-Otto-Krag Str. 11
    	          D-52146 Wuerselen
    	          Tel.: +49 (0) 2405 / 4548-21
    	          Fax : +49 (0) 2405 / 4548-14

		(C) FiberVision GmbH, 2000-2003
 *
 * $Header:   K:/Projects/PQCamera/PQCamera/Findmark/findmark.c_v   1.1   May 08 2003 10:36:04   APeng  $
 *
 * $Log:   K:/Projects/PQCamera/PQCamera/Findmark/findmark.c_v  $
 * 
 *    Rev 1.1   May 08 2003 10:36:04   APeng
 *  
 * 
 *    Rev 1.0   May 02 2003 14:39:40   APeng
 * Files in new directory Findmark
 *
 *    Rev 1.12   Mar 07 2003 13:57:20   APeng
 *
 *
 *    Rev 1.11   Feb 21 2003 16:03:16   APeng
 *
 *
 *    Rev 1.10   Feb 13 2003 16:38:14   APeng
 * Version 0.63.2
 * Increased the number of threshold tries to find yellow
 *
 *    Rev 1.9   Jan 29 2003 15:23:20   APeng
 * Version 0.63.1
 *
 *    Rev 1.6   Jan 14 2003 15:13:12   APeng
 * Version0.62.1
 *
 *    Rev 1.4   Dec 30 2002 13:51:28   APeng
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
 *    Rev 1.3   Dec 13 2002 11:32:36   APeng
 * Added function to handle stripes and smudges around the registration marks
 *
 *
 *
 *    Rev 1.2   Dec 10 2002 15:59:26   APeng
 * some work has been done to the reduced
 * ROI
 *
 *    Rev 1.5   20 Sep 2001 15:32:20   MARKC
 * See SSI09.19.2001-3-A
 *
 *    Rev 1.4   30 Aug 2001 12:55:12   MARKC
 * See SSI08.28.2001-1-A
 * Chg'd Tracking() and TrackBlobs() to use macros for camera and pattern status.
 *
 *    Rev 1.3   10 Aug 2001 09:50:58   MARKC
 * See SSI07.18.01-2-B, SSI07.22.01-1-A, SSI08.09.01-2-A
 *
 *    Rev 1.2   17 Jul 2001 15:23:12   MARKC
 * added vcfree( features) at the end of the function
 *
 *    Rev 1.1   16 Jul 2001 19:26:38   MARKC
 * moved Track_blobs function to pattern module
 *
 *    Rev 1.0   09 Jul 2001 12:41:36   MARKC
 * Checked in from initial workfile by PVCS Version Manager Project Assistant.
 *
 *
 *********************************************************************SDOCEND**/

/*=======================  I N C L U D E   F I L E S  ========================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vcdefs.h>
#include <vcrt.h>
#include <vclib.h>
#include <cam0.h>
#include <vc65.h>
#include <macros.h>
#include <time.h>

#include "build.h"
#include "config.h"
#include "global.h"
#include "command.h"
#include "error.h"
#include "image.h"
#include "pqlib.h"

#include "patdef.h"

#include "findmark.h"


/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

/*===========================  F U N C T I O N S  ============================*/

/*______________________________________________________________________________

    	$Id: tracker.c,v 1.6 2000/10/06 17:46:23 Manuel Exp $

		Module	: PQ-Runtime
        Function: keep track of ROI within FOV
    	Language: ISO C
	    System	: VC/RT

    	Author	: Manuel Jochem
    	          FiberVision GmbH
    	          Jens-Otto-Krag Str. 11
    	          D-52146 Wuerselen
    	          Tel.: +49 (0) 2405 / 4548-21
    	          Fax : +49 (0) 2405 / 4548-14

		(C) FiberVision GmbH, 2000-2003
  ______________________________________________________________________________
*/

/**FDOC*************************************************************************
 *
 *  Function:       main
 *
 *  Author:         Mark Colvin             Date: June 12, 2001
 *
 *  Description:	main entry of tracking module
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

int MAIN(findmark)(int dummy, int argc, HPQ_RLC Rlc, PQ_CFG* Config, ftr* features)
{
	char*  BootMsg  = "PQ-findmark [build: "__BUILD__"]  "__DATE__"  "__TIME__"";
	int numberObjects = 0;
#ifdef WITH_TIMING
	time_t elapsed = time(NULL);
#endif

	/* show info message on screen */
	SendDbgMsg(DBG_INFO, BootMsg);
	/* make sure global variables are not overwritten */
	UseGlobals();

	numberObjects = FindMarks(Rlc, Config, features);

#ifdef WITH_TIMING
	cprintf(Console, " findmark net: %ld", time(NULL) - elapsed);
#endif
	return numberObjects;
}

/**FDOC*************************************************************************
 *
 *  Function:       Find Registration Marks
 *
 *  Author:         Mark Colvin             Date: June 29, 2000
 *
 *  Description:    Analyze image for tracking mode
 *
 *  Parameters:
 *
 *  Returns:
 *
 *  Side effects:
 *
 *  History:        7/20/01 plc - small rearrangements, restored call to TrackBlobs
 *
 *********************************************************************FDOCEND**/

int FindMarks(HPQ_RLC Rlc, PQ_CFG* Config, ftr* features)
{
	int	minObjectCount, numberObjects = 0;

	int origThreshold = Config->Threshold;

#ifdef WITH_TIMING
	{
		time_t elapsed = time(NULL);
		CalculateMidpointThreshold(Config);
		cprintf(Console, "  threshold gross: %ld", time(NULL) - elapsed);
	}
#else
		CalculateMidpointThreshold(Config);
#endif

	/* feature extraction */
	numberObjects = find_objects2(Rlc, Config, features);

	if(Config->bMarkSearchSelectCount == 1)
		minObjectCount = ONEPAIR_OBJECT_COUNT;
	else
		minObjectCount = MIN_OBJECT_COUNT;

	/* do the blob pairing function if */
	/* have a reasonable number of blobs */

	if((numberObjects < MAX_NUM_OBJECTS-1) && (numberObjects >= minObjectCount))
	{
#ifdef WITH_TIMING
		time_t elapsed = time(NULL);
		Chkblob(numberObjects, features, Config);
		cprintf(Console, "  chkblob gross: %ld", time(NULL) - elapsed);
#else
		Chkblob(numberObjects, features, Config);
#endif

		/* display blob data and mark the image */
		if(bDebugLevel >= DBG_ERRORS_GREY )
			mark_objects(&(Config->RoI), features, numberObjects);
	}

	DisplayFrame(&(Config->RoI));      /* frame the ROI area */

	Config->Threshold = origThreshold;

	return numberObjects;
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

