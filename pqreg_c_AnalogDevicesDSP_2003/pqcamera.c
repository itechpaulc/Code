/**SDOC*************************************************************************
 *
 *  pqcamera.c - image processing for PQ camera
 *
 *  Author: Mark Colvin				June 12, 2001
 *
 * $Header:   K:/Projects/PQCamera/PQCamera/PQLIB/pqcamera.c_v   1.7   Mar 07 2003 13:57:20   APeng  $
 *
 * $Log:   K:/Projects/PQCamera/PQCamera/PQLIB/pqcamera.c_v  $
 * 
 *    Rev 1.7   Mar 07 2003 13:57:20   APeng
 *  
 * 
 *    Rev 1.6   Feb 21 2003 16:03:14   APeng
 *  
 * 
 *    Rev 1.5   Jan 29 2003 15:23:20   APeng
 * Version 0.63.1
 * 
 *    Rev 1.4   Dec 30 2002 13:43:12   APeng
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
 *    Rev 1.3   Dec 13 2002 11:32:34   APeng
 * Added function to handle stripes and smudges around the registration marks
 * 
 * 
 * 
 *    Rev 1.2   Dec 10 2002 15:59:24   APeng
 * some work has been done to the reduced
 * ROI
 *
 *    Rev 1.0   09 Jul 2001 12:40:22   MARKC
 * Checked in from initial workfile by PVCS Version Manager Project Assistant.
 *
 *
 *********************************************************************SDOCEND**/

/*=======================  I N C L U D E   F I L E S  ========================*/
#include <cam0.h>
#include <vcrt.h>
#include <vc65.h>
#include <vclib.h>
#include <macros.h>
#include <string.h>
#include <limits.h>
#include <bool.h>

#include "config.h"
#include "global.h"
#include "comm.h"

#include "pqlib.h"

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

/*===========================  F U N C T I O N S  ============================*/

/*=======================  I N C L U D E   F I L E S  ========================*/

/**FDOC*************************************************************************
 *
 *  Function:       InitSearchVaribles
 *
 *  Author:         Mark Colvin             Date: June 12, 2001
 *
 *  Description:
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
void	InitSearchVaribles(PQ_CFG* Config)
{
	int i;

	Config->pFound = 0;
	Config->pReferenceFound = FALSE;
	Config->pMarkSearchAllFound = FALSE;
	Config->FirstPatternIsFound = TRUE;

	Config->referenceSearchType = SEARCH_INFER_TYPE_NONE;

	for( i=0; i < NREGMARK; i++ )
	{
		if( Config->bMarkEnabled[i] )    /* if enabled, mark not found */
		{
			Config->strcRegPos[i].nLateral = 0L;
			Config->strcRegPos[i].nCircum  = 0L;
			Config->strcRegPos[i].nMarkStatus  = MARK_MISSING;

			Config->strcElemInfData[i].topElem.x = 0;
			Config->strcElemInfData[i].topElem.y = 0;
			Config->strcElemInfData[i].bottomElem.x = 0;
			Config->strcElemInfData[i].bottomElem.y = 0;
			Config->strcElemInfData[i].nMarkStatusModifier = MARK_MISSING;

			Config->strcElemInfData[i].topElemId = 0;
			Config->strcElemInfData[i].bottomElemId = 0;
		}
		else
		{
			Config->strcRegPos[i].nMarkStatus  = MARK_DISABLED;
		}
	}
}


/**FDOC*************************************************************************
 *
 *  Function:       InitSearchVaribles
 *
 *  Author:         Mark Colvin             Date: June 12, 2001
 *
 *  Description:
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
void	InitTrackVariablesForRetry(PQ_CFG* Config)
{
	int i;

		Config->pFound = 0;
		Config->pMarkSearchAllFound = FALSE;					
		
		for( i=0; i < NREGMARK; i++ )
		{
			Config->strcElemInfData[i].topElemId = 0;
			Config->strcElemInfData[i].bottomElemId = 0;
		}
}

/**FDOC*************************************************************************
 *
 *  Function:       InitSearchFoundResultBackup
 *
 *  Author:         Paul Calinawan             Date: November 12, 2002
 *
 *  Description:
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
void	InitSearchFoundResultBackup(PQ_CFG* Config)
{
	int i;

	for( i=0; i < NREGMARK; i++ )
	{
		Config->regFoundResultBackup[i] = FALSE;
	}
}

/**FDOC*************************************************************************
 *
 *  Function:       SelectAllMarksForSearch
 *
 *  Author:         Mark Colvin             Date: June 12, 2001
 *
 *  Description:
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
void	SelectAllMarksForSearch(PQ_CFG* Config)
{
	int i;

	/* For Hunting, SearchSelect Marks that are Enabled*/

	Config->bMarkSearchSelectCount = 0;

	for( i=0; i < NREGMARK; i++ )
	{
		if(Config->bMarkEnabled[i] == MARK_HOST_DISABLED)
		{
			Config->bMarkSearchSelect[i] = FALSE;
		}
		else
		{
			Config->bMarkSearchSelect[i] = TRUE;
			Config->bMarkSearchSelectCount++;
		}
	}
}

/**FDOC*************************************************************************
 *
 *  Function:       SelectRefMarkForSearch
 *
 *  Author:         Paul Calinawan             Date: May 20, 2001
 *
 *  Description:
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
void	SelectReferenceMarkForSearch(PQ_CFG* Config)
{
	int i;

	for( i=0; i < NREGMARK; i++ )
	{
		if(i == Config->bReferenceMarkSelection)
		{
			Config->bMarkSearchSelect[i] = TRUE;
		}
		else
		{
			Config->bMarkSearchSelect[i] = FALSE;
		}
	}
}



void	UpdatePatternFound(PQ_CFG* Config)
{
	int pT;
	Config->pFound = 0;

	/* Update Pattern Found */
	for(pT=0; pT<NREGMARK; pT++)
		if (Config->regFoundResultBackup[pT] == TRUE)
			Config->pFound ++;
}

void	StoreMarksFound(PQ_CFG* Config)
{
	int pT;

	/* Remember Marks Found */
	for(pT=0; pT<NREGMARK; pT++)
		if(Config->strcRegPos[pT].nMarkStatus == MARK_FOUND)
			Config->regFoundResultBackup[pT] = TRUE;

	UpdatePatternFound(Config);

	if(Config->pFound == Config->bMarkSearchSelectCount)
		Config->pMarkSearchAllFound = TRUE;
	else
		Config->pMarkSearchAllFound = FALSE;
}



/* end of file PQCAMERA.C */
