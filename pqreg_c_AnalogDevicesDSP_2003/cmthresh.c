


/**SDOC*************************************************************************
 *
 *  thresh.c - Threshold Calculation logic for PrintQuick camera firmware
 *
 *  Author: Paul Calinawan			Nov 12, 2002
 *
 * $Header:   K:/Projects/PQCamera/PQCamera/PQLIB/cmthresh.c_v   1.1   May 08 2003 10:27:22   APeng  $
 *
 * $Log:   K:/Projects/PQCamera/PQCamera/PQLIB/cmthresh.c_v  $
 * 
 *    Rev 1.1   May 08 2003 10:27:22   APeng
 *  
 * 
 *    Rev 1.0   May 02 2003 13:44:40   APeng
 *  
 *
 *    Rev 1.6   Mar 07 2003 13:57:22   APeng
 *
 *
 *    Rev 1.5   Feb 21 2003 16:03:16   APeng
 *
 *
 *    Rev 1.4   Jan 29 2003 15:23:20   APeng
 * Version 0.63.1
 *
 *    Rev 1.3   Jan 07 2003 17:10:34   APeng
 * Version 0.61.2
 * Fix Contrast/Brightness Check in tracking
 *
 *    Rev 1.2   Dec 30 2002 13:50:02   APeng
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
 *    Rev 1.1   Dec 10 2002 15:59:28   APeng
 * some work has been done to the reduced
 * ROI
 *
 *
 *
 *
 *********************************************************************SDOCEND**/

/*=======================  I N C L U D E   F I L E S  ========================*/


/* standard C headers*/

#include <string.h>	/* strlen */
#include <stdio.h>
#include <stdlib.h>
/* standard vision components headers*/
#include <vcrt.h>
#include <vclib.h>
#include <cam0.h>
#include <macros.h>
/* standard fibervision headers*/
#include <vcdefs.h>
#include <bool.h>

/* project library headers*/
#include "image.h"
#include "pqlib.h"
/* misc project headers*/
#include "config.h"
#include "command.h"
#include "global.h"
#include "error.h"
#include "bytes.h"
/* module headers*/
#include "pqlib.h"


GMI_ASSERTFILE (__FILE__);



/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

#define		ONEPAIR_MIN_FOUND_AVG_THRESH	(1)
#define		MIN_FOUND_AVG_THRESH			(2)

#define		FIRST_TRACKER_TRY				(0)

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

/*===========================  F U N C T I O N S  ============================*/


/**FDOC*************************************************************************
 *
 *  Function:       CalculateMidpointThreshold
 *
 *  Author:         Paul L C
 *
 *  Description:    Threshold determination base on ROI Histogram
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

#define		MAX_HISTOGRAM_INDEX				256
#define		HISTOGRAM_START					2
#define		HISTOGRAM_END					(MAX_HISTOGRAM_INDEX - HISTOGRAM_START)

#define		MIN_REGISTER_ELEMENT_COUNT		2
#define		MIN_SUBSTRATE_ELEMENT_COUNT		36

void	CalculateMidpointThreshold(PQ_CFG* Config)
{
#ifdef WITH_TIMING
	time_t elapsed = time(NULL);
#endif

	Config->ContrastFailure = FALSE;
	Config->BrightnessFailure = FALSE;

	/* analyze ROI histogram only at the first try */
	if(Config->TrackRetry == FIRST_TRACKER_TRY)
		ThresholdingByHistogram(Config);

	if(!Config->ContrastFailure && !Config->BrightnessFailure)
		AdjustThreshold(Config);

#ifdef WITH_TIMING
	cprintf(Console, "   threshold net: %ld", time(NULL) - elapsed);
#endif
}


void	ThresholdingByHistogram(PQ_CFG* Config)
{
	int substrateThreshold = 255;
	int elemThreshold = 0;
	int rangeThreshold;
	int i, minFoundAvgThresh;
	unsigned long regionHistogram[MAX_HISTOGRAM_INDEX];
	unsigned long pixAreaCount;

	int pId = Config->bTargetPatternID;
	PATTERN_CONFIG	*Patterns = Config->Patterns;

	unsigned long minElemArea =
		MIN_REGISTER_ELEMENT_COUNT * Patterns[pId].bMinElementSize;

	unsigned long minSubstrateArea =
		MIN_SUBSTRATE_ELEMENT_COUNT * Patterns[pId].bMinElementSize;

	histogram(&(Config->RoI), regionHistogram);

	pixAreaCount = 0;

	for(i=HISTOGRAM_START; i<HISTOGRAM_END; i++)
	{
		pixAreaCount += *(regionHistogram + i);

		if(pixAreaCount > minElemArea)
		{
			elemThreshold = i;
			break;
		}
	}

	pixAreaCount = 0;

	for(i=HISTOGRAM_END-1; i>elemThreshold; i--)
	{
		pixAreaCount += *(regionHistogram + i);

		if(pixAreaCount > minSubstrateArea)
		{
			substrateThreshold = i;
			break;
		}
	}

	rangeThreshold = substrateThreshold - elemThreshold;

	Config->Contrast = (100 * rangeThreshold) >> 8;
	Config->Brightness = substrateThreshold;

	Config->ContrastFailure		= (Config->Contrast < Config->MinContrast);
	Config->BrightnessFailure	= (Config->Brightness < Config->MinBrightness);

	if( (!Config->ContrastFailure) && (!Config->BrightnessFailure))
	{
		Config->MidpointThreshold =
			(elemThreshold + substrateThreshold) >> 1 ;

		/*
		cprintf(Console, "MIDPOINT: %d", Config->MidpointThreshold);
		*/

		if(Config->bMarkSearchSelectCount == 1)
			minFoundAvgThresh = ONEPAIR_MIN_FOUND_AVG_THRESH;
		else
			minFoundAvgThresh = MIN_FOUND_AVG_THRESH;

		if(Config->pFound >= minFoundAvgThresh)
		{
			Config->AvgMidpointCount++;

			Config->TotalAvgMidpointThreshold += Config->MidpointThreshold;

			if(Config->AvgMidpointCount == 16)
			{
				Config->AvgMidpointThreshold =
					(Config->TotalAvgMidpointThreshold) >> 4;

				Config->TotalAvgMidpointThreshold = 0;
				Config->AvgMidpointCount = 0;
			}
		}

	}
}


void	AdjustThreshold(PQ_CFG* Config)
{
	if(Config->TrackRetry == 0)
	{	
		Config->AdjustedThreshold = Config->Threshold +
			(Config->MidpointThreshold - Config->AvgMidpointThreshold);

		/* new base threshold adjusted by brightness variation */
		Config->BaseThreshold = Config->AdjustedThreshold;
	}
	else 
		Config->AdjustedThreshold += Config->TrackerAdjustSteps;

	Config->Threshold = Config->AdjustedThreshold;
}


/*______________________________________________________________________________

		EOF - thresh.c
  ______________________________________________________________________________
*/
