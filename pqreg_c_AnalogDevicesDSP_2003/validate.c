


/**SDOC*************************************************************************
 *
 *  valid.c - Validation logic for PrintQuick camera firmware
 *
 *  Author: Paul Calinawan			Nov 22, 2002
 *
 * $Header:   K:/Projects/PQCamera/PQCamera/PQLIB/validate.c_v   1.0   May 02 2003 16:14:44   APeng  $
 *
 * $Log:   K:/Projects/PQCamera/PQCamera/PQLIB/validate.c_v  $
 * 
 *    Rev 1.0   May 02 2003 16:14:44   APeng
 *  
 *
 *    Rev 1.10   Mar 26 2003 10:07:56   APeng
 *
 *
 *    Rev 1.9   Feb 21 2003 16:03:16   APeng
 *
 *
 *    Rev 1.8   Jan 29 2003 15:23:20   APeng
 * Version 0.63.1
 *
 *    Rev 1.7   Jan 20 2003 15:49:38   APeng
 * Version 0.62.2
 *
 *    Rev 1.6   Jan 14 2003 15:13:36   APeng
 * Version0.62.1
 * Brightness, Contrast and ROI space have to fail more than twice to flag failures
 *
 *    Rev 1.5   Jan 13 2003 11:10:12   APeng
 * improved the ROI space validation
 *
 *    Rev 1.4   Jan 07 2003 17:10:34   APeng
 * Version 0.61.2
 * Fix Contrast/Brightness Check in tracking
 *
 *    Rev 1.3   Dec 30 2002 13:52:00   APeng
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
 *    Rev 1.2   Dec 13 2002 11:32:56   APeng
 * Added function to handle stripes and smudges around the registration marks
 *
 *
 *
 *    Rev 1.1   Dec 10 2002 15:38:20   APeng
 * some work has been done on the reduced
 * ROI
 *
 *
 *
 *
 *********************************************************************SDOCEND**/

/*=======================  I N C L U D E   F I L E S  ========================*/


#include <string.h>	/* strlen */
#include <stdio.h>
#include <stdlib.h>
#include <vcdefs.h>
#include <bool.h>
#include <vcrt.h>
#include <vclib.h>
#include <cam0.h>
#include <macros.h>

#include "build.h"
#include "config.h"
#include "command.h"
#include "global.h"
#include "error.h"
#include "image.h"
#include "pqlib.h"

#include "bytes.h"

#include "pqlib.h"

GMI_ASSERTFILE (__FILE__);

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

#define		MAX_UNSTABLE_MEAS_COUNT		(2)

/*===========================  F U N C T I O N S  ============================*/




/**FDOC*************************************************************************
 *
 *  Function:       main
 *
 *  Author:         Paul Calinawan             Date:  Nov 12, 2002
 *
 *  Description:	Entry point for the PQ Validation module
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

void Validate(PQ_CFG* Config, ftr* features, int featcount)
{
#ifdef WITH_TIMING
	time_t elapsed = time(NULL);
#endif
	if(!Config->BrightnessFailure)
		Config->BrightnessFailureCount = 0;

	if(!Config->ContrastFailure)
		Config->ContrastFailureCount = 0;

	if(!Config->RoiSpaceFailure)
		Config->RoiSpaceFailureCount = 0;

	if(Config->BrightnessFailure)
	{
		if (Config->bSearchMode == SEARCH_HUNT)
			UpdateFailureStatus(Config, MARK_FORCED_MISSING);
		else
		{
			Config->BrightnessFailureCount++;

			/* Take more than 3 Brightness Failures to flag the condition */
			if(Config->BrightnessFailureCount >= 3)
				UpdateFailureStatus(Config, MARK_MISSING_BRIGHTNESS_FAILURE);
			else if (Config->BrightnessFailureCount >= 1)
				UpdateFailureStatus(Config, MARK_FORCED_MISSING);
		}

		return;
	}

	if(Config->ContrastFailure)
	{
		if (Config->bSearchMode == SEARCH_HUNT)
			UpdateFailureStatus(Config, MARK_FORCED_MISSING);
		else
		{
			Config->ContrastFailureCount++;

			/* Take more than 3 Contrast Failures to flag the condition */
			if(Config->ContrastFailureCount >= 3)
				UpdateFailureStatus(Config, MARK_MISSING_CONTRAST_FAILURE);
			else if (Config->ContrastFailureCount >= 1)
				UpdateFailureStatus(Config, MARK_FORCED_MISSING);
		}

		return;
	}

	if(Config->pReferenceFound)
	{
		Config->RegMeasUnstable = FALSE;
		ValidateRegMeas(Config);

		if(Config->RegMeasUnstable)
		{
			UpdateFailureStatus(Config, MARK_FOUND_BUT_UNSTABLE);
			return;
		}

		Config->RoiSpaceFailure = FALSE;
		ValidateRoiSpace(Config, features, featcount);

		if(Config->RoiSpaceFailure)
		{
			if (Config->bSearchMode == SEARCH_HUNT)
				UpdateFailureStatus(Config, MARK_FORCED_MISSING);
			else
			{
				Config->RoiSpaceFailureCount++;

				/* Take more than 3 RoiSpace Failures to flag the condition */
				if(Config->RoiSpaceFailureCount >= 3)
					UpdateFailureStatus(Config, MARK_MISSING_ROI_SPACE_VIOLATION);
				else if (Config->RoiSpaceFailureCount >= 1)
					UpdateFailureStatus(Config, MARK_FORCED_MISSING);
			}

			return;
		}

	}

	UpdateForFinalPatternStatus(Config);
#ifdef WITH_TIMING
	cprintf(Console, "  valid net: %ld", time(NULL) - elapsed);
#endif
}


/**FDOC*************************************************************************
 *
 *  Function:
 *
 *  Author:         Paul L C
 *
 *  Description:
 *
 *  Parameters:
 *
 *  Returns:
 *
 *  Side effects:
 *
 *  history:
 *
 *********************************************************************FDOCEND**/


void	ValidateRegMeas(PQ_CFG* Config)
{
	UpdateCurrRegMeas(Config);

	CheckForRegMeasStability(Config);
}

void	UpdateCurrRegMeas(PQ_CFG* Config)
{
	int	pT;
	int refId = Config->bReferenceMarkSelection;

	REG_MEAS	*strcCurrRegMeas = Config->strcCurrRegMeas;

	REG_POS		*strcRegPos = Config->strcRegPos;

	int	refCircum = strcRegPos[refId].nCircum;
	int	refLateral = strcRegPos[refId].nLateral;

	for(pT=0; pT<NREGMARK; pT++)
	{
		if((pT != refId) &&
			(strcRegPos[pT].nMarkStatus == MARK_FOUND))
		{
			strcCurrRegMeas[pT].nCircum =
				(strcRegPos[pT].nCircum - refCircum);

			strcCurrRegMeas[pT].nLateral =
				(strcRegPos[pT].nLateral - refLateral);

			/*
			cprintf(Console, "PT:%d C:%d L:%d", pT+1,
						strcCurrRegMeas[pT].nCircum,
						strcCurrRegMeas[pT].nLateral);
			*/
		}
	}
}


/**FDOC*************************************************************************
 *
 *  Function:       CheckForRegMeasStability
 *
 *  Author:         Paul L C
 *
 *  Description:
 *
 *  Parameters:
 *
 *  Returns:
 *
 *  Side effects:
 *
 *  history:
 *
 *********************************************************************FDOCEND**/

void	CheckForRegMeasStability(PQ_CFG* Config)
{
	int	pT;
	int	circOffsetDeviation = 0;
	int latOffsetDeviation = 0;

	int	unstableMeasCount = 0;
	int maxOffsetDeviation = Config->RegMeasMaxOffsetDeviation;

	int refId = Config->bReferenceMarkSelection;

	REG_POS		*strcRegPos = Config->strcRegPos;

	REG_MEAS	*strcCurrRegMeas = Config->strcCurrRegMeas;
	REG_MEAS	*strcPrevRegMeas = Config->strcPrevRegMeas;

	for(pT=0; pT<NREGMARK; pT++)
	{
		if(pT != refId)
		{
			if(strcRegPos[pT].nMarkStatus == MARK_FOUND) /* Inference detect later */
			{
				circOffsetDeviation = abs(strcCurrRegMeas[pT].nCircum -	strcPrevRegMeas[pT].nCircum);

				latOffsetDeviation = abs(strcCurrRegMeas[pT].nLateral - strcPrevRegMeas[pT].nLateral);

				if((circOffsetDeviation > maxOffsetDeviation) || (latOffsetDeviation > maxOffsetDeviation))
				{
					strcRegPos[pT].nMarkStatus = MARK_FOUND_BUT_UNSTABLE;
					Config->RegMeasUnStableCount[pT] = Config->MinMeasStableCount;

					unstableMeasCount++;
				}
				else
				{
					if(Config->RegMeasUnStableCount[pT])
						--Config->RegMeasUnStableCount[pT];
				}

				strcPrevRegMeas[pT].nCircum = strcCurrRegMeas[pT].nCircum;
				strcPrevRegMeas[pT].nLateral = strcCurrRegMeas[pT].nLateral;
			}
		}
		else
		{
			Config->RegMeasUnStableCount[pT] = 0;
		}
	}

	if(unstableMeasCount >= MAX_UNSTABLE_MEAS_COUNT)
	{
		Config->RegMeasUnstable = TRUE;
		cprintf(Console, "FORCED ALL MARKS UNSTABLE");

		for(pT=0; pT<NREGMARK; pT++)
		{
			if(pT != refId)
			{
				strcRegPos[pT].nMarkStatus = MARK_FOUND_BUT_UNSTABLE;
				Config->RegMeasUnStableCount[pT] = Config->MinMeasStableCount;
			}
		}
	}
}

void	GenerateAvgOffset(PQ_CFG* Config)
{
	int	pT;
	int refId = Config->bReferenceMarkSelection;

	int	avgCircum, avgLateral;

	REG_POS		*strcRegPos = Config->strcRegPos;
	REG_MEAS	*strcCurrRegMeas = Config->strcCurrRegMeas;

	for(pT=0; pT<NREGMARK; pT++)
	{
		if((pT != refId) &&
			(strcRegPos[pT].nMarkStatus == MARK_FOUND))
		{
			avgCircum  = (strcCurrRegMeas[pT].nCircum)/2;
			avgLateral = (strcCurrRegMeas[pT].nLateral)/2;

			strcRegPos[pT].nCircum  =
				strcRegPos[refId].nCircum + avgCircum;

			strcRegPos[pT].nLateral =
				strcRegPos[refId].nLateral + avgLateral;
		}
	}
}

/**FDOC*************************************************************************
 *
 *  Function:       UpdateFailureStatus
 *
 *  Author:         Ann Peng
 *
 *  Description:
 *
 *  Parameters:
 *
 *  Returns:
 *
 *  Side effects:
 *
 *  history: 12/11/2002
 *
 *********************************************************************FDOCEND**/
void	UpdateFailureStatus(PQ_CFG* Config, int failureType)
{
	int	pT;
	char msgConsole[50];

	REG_POS	*strcRegPos = Config->strcRegPos;

	switch (failureType)
	{
	case MARK_MISSING_CONTRAST_FAILURE:
		 strcpy(msgConsole, "CONTRAST FAILURE");
		 break;
	case MARK_MISSING_BRIGHTNESS_FAILURE:
		 strcpy(msgConsole, "BRIGHTNESS FAILURE");
		 break;
	case MARK_MISSING_ROI_SPACE_VIOLATION:
		 strcpy(msgConsole, "ROI SPACE FAILURE");
		 break;
	case MARK_FOUND_BUT_UNSTABLE:
		 strcpy(msgConsole, "UNSTABLE MEASUREMENT");
		 break;
	case MARK_FORCED_MISSING:
		 strcpy(msgConsole, "FORCED MISSING");
		 break;
	}

	cprintf(Console, msgConsole);

	for(pT=0; pT<NREGMARK; pT++)
		if(Config->bMarkEnabled[pT])
			strcRegPos[pT].nMarkStatus = failureType;

	Config->pFound = 0;
	Config->pReferenceFound = FALSE;
	Config->pMarkSearchAllFound = FALSE;
}


/**FDOC*************************************************************************
 *
 *  Function:       ValidateRoiSpace
 *
 *  Author:         Paul L C
 *
 *  Description:
 *
 *  Parameters:
 *
 *  Returns:
 *
 *  Side effects:
 *
 *  history:
 *
 *********************************************************************FDOCEND**/

void ValidateRoiSpace(PQ_CFG* Config, ftr* features, int numBlobs)
{
    long st;
	int i, pId, numPatternElements, numObjects = 0;
	ftr *curFeature;
	unsigned int sumArea = 0, thisArea, maxTotalBlobAreaInReducedROI, maxNumObjectsInReducedROI;
	bool isBlack;

	int	NarrowRoiMargin = Config->NarrowRoiMargin;

	int RoiOriginX = Config->RoILocation[0],
		RoiOriginY = Config->RoILocation[1];

	int	RoiOriginBX = RoiOriginX + TRACK_IMAGE_SIZE,
		RoiOriginBY = RoiOriginY + TRACK_IMAGE_SIZE;

#ifdef USE_SUBPIXELS
#warning subpixeling enabled
	int RoiTx = RoiOriginX + (Config->NarrowRoIminX / (int) SCALE_FACTOR) - NarrowRoiMargin ,
		RoiTy = RoiOriginY + (Config->NarrowRoIminY / (int) SCALE_FACTOR) - NarrowRoiMargin,
		RoiBx = RoiOriginX + (Config->NarrowRoImaxX / (int) SCALE_FACTOR) + NarrowRoiMargin,
		RoiBy = RoiOriginY + (Config->NarrowRoImaxY / (int) SCALE_FACTOR) + NarrowRoiMargin;
#else
	int RoiTx = RoiOriginX + Config->NarrowRoIminX - NarrowRoiMargin ,
		RoiTy = RoiOriginY + Config->NarrowRoIminY - NarrowRoiMargin,
		RoiBx = RoiOriginX + Config->NarrowRoImaxX + NarrowRoiMargin,
		RoiBy = RoiOriginY + Config->NarrowRoImaxY + NarrowRoiMargin;
#endif

		/* Clamp NarrowROI to the original ROI size */
		if(RoiTx < RoiOriginX) RoiTx = RoiOriginX;
		if(RoiTy < RoiOriginY) RoiTy = RoiOriginY;
		if(RoiBx > RoiOriginBX) RoiBx = RoiOriginBX;
		if(RoiBy > RoiOriginBY) RoiBy = RoiOriginBY;

		st = PIXEL_ADDR(RoiTx, RoiTy);

		Config->NarrowRoI.st = st;
		Config->NarrowRoI.dx = (RoiBx - RoiTx);
		Config->NarrowRoI.dy = (RoiBy - RoiTy);
		Config->NarrowRoI.pitch = ScrGetPitch;

		AlignRoi(&Config->NarrowRoI, FALSE);	 /* Align image to words */

		/* Handle stripe and smudge cases */
		/*
		if((numBlobs > MAX_NUM_OBJECTS-1) || (numBlobs < MIN_OBJECT_COUNT))
		{
			Config->RoiSpaceFailure = TRUE;
			DisplayFrame(&(Config->NarrowRoI));
			return;
		}
		*/

		/* Give a margin of 20 for the number of objects */
		numPatternElements = 2 * Config->bMarkSearchSelectCount;
		maxNumObjectsInReducedROI = numPatternElements + 20;

		/* Give a margin of 1 for the total blob area */
		pId = Config->bTargetPatternID;
		maxTotalBlobAreaInReducedROI = (numPatternElements + 2) * (Config->Patterns[pId].bMaxElementSize);

		/*
		cprintf(Console, "In reduced ROI :");
		cprintf(Console, "Max number of objects allowed :%d ", maxNumObjectsInReducedROI);
		cprintf(Console, "Total Area allowed :%d ", maxTotalBlobAreaInReducedROI);
		cprintf(Console, "Number of blobs :%d ", numBlobs);
		*/

		for (i = 0; i < numBlobs; i++)
		{
			curFeature = features + i;
			isBlack = curFeature->color == BLACK;

			if (!isBlack)
				continue;

			if(BlobInRoi(curFeature, &Config->NarrowRoI, &Config->RoI))
			{
				numObjects ++;

				thisArea = curFeature->area;
				sumArea += thisArea;
			}
/*				else
				cprintf(Console, "blob %d outside", i);*/
		}

/*
		cprintf(Console, "Number of objects :%d ", numObjects);
		cprintf(Console, "Total Area :%d ", sumArea);
*/

		if(numObjects > maxNumObjectsInReducedROI)
		{
			Config->RoiSpaceFailure = TRUE;
			cprintf(Console, "Exceeds Max number of objects");
		}
		else if(sumArea > maxTotalBlobAreaInReducedROI)
		{
			Config->RoiSpaceFailure = TRUE;
			cprintf(Console, "Exceeds Max Total Area");
		}

		DisplayFrame(&(Config->NarrowRoI));
}


/**FDOC*************************************************************************
 *
 *  Function:       UpdateForFinalPatternStatus
 *
 *  Author:         Paul L C
 *
 *  Description:
 *
 *  Parameters:
 *
 *  Returns:
 *
 *  Side effects:
 *
 *  history:
 *
 *********************************************************************FDOCEND**/

void	UpdateForFinalPatternStatus(PQ_CFG* Config)
{
	int	pT;

	REG_POS	*strcRegPos = Config->strcRegPos;
	REG_ELEM_INF_DATA *strcElemInfData = Config->strcElemInfData;

	for(pT=0; pT<NREGMARK; pT++)
	{
		if(strcRegPos[pT].nMarkStatus == MARK_FOUND)
		{
			if(Config->RegMeasUnStableCount[pT])
			{
				strcRegPos[pT].nLateral = 0L;
				strcRegPos[pT].nCircum  = 0L;
				strcRegPos[pT].nMarkStatus = MARK_FOUND_BUT_UNSTABLE;

				cprintf(Console, "UNSTABLE PT:%d US:%d",
								pT+1, Config->RegMeasUnStableCount[pT]);
			}
			else
			if(strcElemInfData[pT].nMarkStatusModifier != MARK_MISSING)
			{
				/*
				cprintf(Console, "MOD PT:%d S:0x%x"
							,pT+1, strcElemInfData[pT].nMarkStatusModifier);
				*/

				/* Force Mark Status to the modifier */

				strcRegPos[pT].nMarkStatus =
					strcElemInfData[pT].nMarkStatusModifier;
			}
		}
	}
}

/*______________________________________________________________________________

		Check if blob is inside narrow roi
  ______________________________________________________________________________
*/

bool BlobInRoi(ftr* blob, image* roi, image* parent)
{
	int roiTop    = (int)((roi->st - parent->st) / (long) roi->pitch);
	int roiLeft   = (int)((roi->st - parent->st) % (long) roi->pitch);
	int roiBottom = roiTop  + roi->dy - 1;
	int roiRight  = roiLeft + roi->dx - 1;

	bool isLeft  = blob->x_max < roiLeft;
	bool isRight = blob->x_min > roiRight;
	bool isUpper = blob->y_max < roiTop;
	bool isLower = blob->y_min > roiBottom;

	return !(isLeft || isRight || isUpper || isLower);
}

/*______________________________________________________________________________

		EOF - valid.c
  ______________________________________________________________________________
*/
