
/**SDOC*************************************************************************
 *
 *  config.c - PQ camera firmware - CONFIG module
 *
 *  Author: Mark Colvin - GMI

	Author	: Manuel Jochem
   	          FiberVision GmbH
			  Jens-Otto-Krag-Strasse 11
			  D-52146 Wuerselen
   	       	  Tel.: +49 (0)2405 / 4548-21
   	       	  Fax : +49 (0)2405 / 4548-14

			(C)	FiberVision GmbH, 2000-2003

 *
 * $Header:   K:/Projects/PQCamera/PQCamera/PQLIB/reqstatus.c_v   1.0   May 02 2003 16:07:18   APeng  $
 *
 * $Log:   K:/Projects/PQCamera/PQCamera/PQLIB/reqstatus.c_v  $
 * 
 *    Rev 1.0   May 02 2003 16:07:18   APeng
 * new file in PQLIB
 *
 *    Rev 1.5   Feb 21 2003 15:26:08   APeng
 *
 *
 *    Rev 1.4   Jan 14 2003 15:12:26   APeng
 *
 *
 *    Rev 1.3   Dec 30 2002 11:37:58   APeng
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
 *    Rev 1.2   Dec 10 2002 15:45:36   APeng
 * some work has been done to the reduced
 * ROI
 *
 *    Rev 1.3   20 Sep 2001 15:31:10   MARKC
 * See SSI09.19.2001-3-A
 *
 *    Rev 1.2   30 Aug 2001 13:04:20   MARKC
 * See SSI08.28.2001-1-A
 * Chg'd ConfigInit() to use macros for camera and pattern status
 *
 *    Rev 1.1   10 Aug 2001 10:05:44   MARKC
 * chg'd to default target patterns to Paul's arrangement (Cal Offset testing)
 *
 *    Rev 1.0   09 Jul 2001 12:39:24   MARKC
 * Checked in from initial workfile by PVCS Version Manager Project Assistant.
 *
 *
 *********************************************************************SDOCEND**/

/*=======================  I N C L U D E   F I L E S  ========================*/
#include <stdio.h>
#include <string.h>
#include <vcdefs.h>
#include <cam0.h>
#include <vc65.h>
#include <vcrt.h>
#include <vclib.h>
#include <macros.h>


#include "build.h"
#include "config.h"
#include "command.h"
#include "error.h"
#include "global.h"
#include "image.h"
#include "pqlib.h"

GMI_ASSERTFILE (__FILE__);

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

/*===========================  F U N C T I O N S  ============================*/

ERROR RequestStatus(PQ_CFG* Config, BYTE* Buffer)
{
    /* declare and initialize local variables */
	ERROR Error    = ERROR_NONE;
	REG_POS SwapStatus;
	WORD swap[2];
	int i = 0;
	int j = 0;

	Buffer[j++] = Config->nCameraStatus;

	for (i = 0; i < NREGMARK; i++)
	{
		SwapStatus.nCircum  = SwapLong(Config->strcRegPos[i].nCircum);
		SwapStatus.nLateral = SwapLong(Config->strcRegPos[i].nLateral);
		SwapStatus.nMarkStatus  = SwapWord(Config->strcRegPos[i].nMarkStatus);
		BufferSplit((unsigned int*) &SwapStatus, (unsigned int*) Buffer + j, sizeof(REG_POS));
		j += BYTESIZE(REG_POS);
	}

	Buffer[j++] = Config->Threshold;
	Buffer[j++] = Config->BaseThreshold;
	swap[0] = SwapWord(Config->RoILocation[0]);
	swap[1] = SwapWord(Config->RoILocation[1]);
	BufferSplit((unsigned int*) &swap, (unsigned int*) Buffer + j, 4);
	j += 4;
	Buffer[j++] = Config->PatternMatchQuality;

	Error = SendDatagram (CMD_SEARCH_STATUS, (BYTESIZE(REG_POS) * NREGMARK) + 8, Buffer, RAW);

	if (Error)
		SendDbgMsg(DBG_ERRORS, "ERROR 03: send status failed (%d)", Error);
	else
		SendDbgMsg(DBG_INFO, "OK: send status");

	/* exit module */
	return Error;
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

