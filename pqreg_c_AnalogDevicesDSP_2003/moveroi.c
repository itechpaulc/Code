/**SDOC*************************************************************************
**
**  moveroi.c	- adjusts the center of RoI location based on location of objects
**
**  Author: Mark Colvin			June 23, 2001
    	Author	: Manuel Jochem
    	          FiberVision GmbH
    	          Jens-Otto-Krag Str. 11
    	          D-52146 Wuerselen
    	          Tel.: +49 (0) 2405 / 4548-21
    	          Fax : +49 (0) 2405 / 4548-14

		(C) FiberVision GmbH, 2000-2003
**
** $Header:   K:/Projects/PQCamera/PQCamera/PQLIB/moveroi.c_v   1.8   Jun 12 2003 17:16:24   APeng  $
**
** $Log:   K:/Projects/PQCamera/PQCamera/PQLIB/moveroi.c_v  $
 * 
 *    Rev 1.8   Jun 12 2003 17:16:24   APeng
 *  
 * 
 *    Rev 1.7   Jun 05 2003 15:18:32   APeng
 *  
 * 
 *    Rev 1.6   Jan 29 2003 15:23:12   APeng
 * Version 0.63.1
 * 
 *    Rev 1.5   Jan 21 2003 11:07:16   APeng
 * revert to previous version
 * 
 *    Rev 1.3   Dec 30 2002 13:43:12   APeng
 * 2002-12-20: Version 0.60.6:
 * 
 * added code for grayscale average made roi size mult
 le of 8
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
 *    Rev 1.2   Dec 10 2002 15:59:22   APeng
 * some work has been done to the reduced
 * ROI
 *
 *    Rev 1.2   30 Aug 2001 13:02:06   MARKC
 * See SSI08.28.2001-1-A
 * Chg'd MoveRoINew() to use macros for pattern status
 *
 *    Rev 1.1   10 Aug 2001 09:52:20   MARKC
 * See SSI08.09.01-2-A
 *
 *    Rev 1.0   09 Jul 2001 12:40:58   MARKC
 * Checked in from initial workfile by PVCS Version Manager Project Assistant.
**
**
**********************************************************************SDOCEND**/

/*=======================  I N C L U D E   F I L E S  ========================*/
#include <vcrt.h>
#include <vclib.h>
#include <cam0.h>
#include <vcdefs.h>
#include <bool.h>

#include "config.h"
#include "pqlib.h"
#include "global.h"

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

/*===========================  F U N C T I O N S  ============================*/
/*______________________________________________________________________________

    	$Id: moveroi.c,v 1.1 2000/09/21 17:04:44 Manuel Exp $

		Module	: PQ-RLC
        Function: create run-length code
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
**
**  Function:       MoveRoINew
**
**  Author:         Mark Colvin             Date: June 23, 2001
**
**  Description:	move the center of the RoI relative to the objects found
**
**  Parameters:     cfg - PQ configuration
**					f - feature structure of the objects found
**					num - number of objects found
**
**  Returns:        none
**
**  Side effects:   none
**
**  History:		6/26/01 11:30:46 AM mac, added function header
**
**********************************************************************FDOCEND**/
void MoveRoINew(PQ_CFG* cfg)
{
	/* NOTE: marks are relative to RoI: we need info about current RoI */
	image *RoI  = &(cfg->RoI);	/* we want to save some dereferencing...*/
	long offset = (long) RoI->pitch;
	long right  = (long)(cfg->FoV.dx - RoI->dx - 3);
	long bottom = (long)(cfg->FoV.dy - RoI->dy - 3);
	long left = 0L;
	long top  = 0L;
	long cur_left = RoI->st % offset;
	long cur_top  = RoI->st / offset;
	long left_offset, top_offset;
	int pT;
	WORD markStatus;

	REG_POS	*strcRegPos = cfg->strcRegPos;

	int matchCount = 0;

	for(pT=0; pT<NREGMARK; pT++)
	{
		markStatus = strcRegPos[pT].nMarkStatus;

		if(cfg->bMarkEnabled[pT] && (markStatus == MARK_FOUND  || markStatus == MARK_FOUND_WITH_ONE_INFERENCE 
			|| markStatus == MARK_FOUND_WITH_TWO_INFERENCES || markStatus == MARK_FOUND_WITH_TWO_INFERENCES_LARGE))
		{
			matchCount++;

			left_offset = (strcRegPos[pT].nLateral + SCALE_FACTOR2) / SCALE_FACTOR;
			top_offset = (strcRegPos[pT].nCircum + SCALE_FACTOR2)  / SCALE_FACTOR;

			left += left_offset;
			top  += top_offset;
		}
	}

	left /= (long) matchCount;
	top  /= (long) matchCount;

	left -= RoI->dx / 2;
	top  -= RoI->dy / 2;

	left += cur_left;
	top  += cur_top;

	if      (left < 0L)     left = 0L;
	else if (left > right)  left = right;

	if      (top  < 0L)     top  = 0L;
	else if (top  > bottom) top  = bottom;

	RoI->st = left + top * offset;

	AlignRoi(RoI, FALSE);	/* Align image to words */

	return;
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

