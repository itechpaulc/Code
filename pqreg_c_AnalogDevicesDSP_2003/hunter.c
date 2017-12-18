/**SDOC*************************************************************************
 *
 *  hunter.c - find the RoI within camera's FoV
 *
 *  Author: Mark Colvin				June 12, 2001
    	Author	: Manuel Jochem
    	          FiberVision GmbH
    	          Jens-Otto-Krag Str. 11
    	          D-52146 Wuerselen
    	          Tel.: +49 (0) 2405 / 4548-21
    	          Fax : +49 (0) 2405 / 4548-14

		(C) FiberVision GmbH, 2000-2003
 *
 * $Header:   K:/Projects/PQCamera/PQCamera/HUNTER/hunter.c_v   1.10   May 02 2003 10:30:42   APeng  $
 *
 * $Log:   K:/Projects/PQCamera/PQCamera/HUNTER/hunter.c_v  $
 * 
 *    Rev 1.10   May 02 2003 10:30:42   APeng
 *  
 *
 *    Rev 1.9   Mar 26 2003 10:06:56   APeng
 *
 *
 *    Rev 1.8   Feb 21 2003 15:26:44   APeng
 *
 *
 *    Rev 1.7   Jan 29 2003 15:23:10   APeng
 * Version 0.63.1
 *
 *    Rev 1.6   Jan 20 2003 15:47:28   APeng
 * Version 0.62.2
 *
 *    Rev 1.5   Jan 14 2003 15:12:38   APeng
 *
 *
 *    Rev 1.4   Dec 30 2002 11:44:02   APeng
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
 *    Rev 1.2   Dec 10 2002 15:59:06   APeng
 * some work has been done to the reduced
 * ROI
 *
 *    Rev 1.2   20 Sep 2001 15:31:52   MARKC
 * See SSI09.19.2001-3-A
 *
 *    Rev 1.1   30 Aug 2001 12:58:58   MARKC
 * See SSI08.28.2001-1-A
 * Chg'd FinalHunting() and FindPrimaryPattern() to use macros for camera and pattern status.
 *
 *    Rev 1.0   09 Jul 2001 12:39:34   MARKC
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
#include "error.h"
#include "command.h"
#include "image.h"
#include "pqlib.h"

#include "patdef.h"

#include "hunter.h"

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/
/*==================  E X T E R N A L   V A R I A B L E S  ===================*/
/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/
/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

/*#define USE_SELFTEST*/
#define EXTERNAL_CALL

#define ONLY_FOUND_FRAME

#define	MAX_THRESH_SEARCH_COUNT		(5)

#define	REGION_WIDTH				(HUNT_IMAGE_SIZE)
#define	REGION_HEIGHT				(HUNT_IMAGE_SIZE)

#define	REF_PAIR_COUNT				(1)
#define	REF_PAIR_SCORE				(10)
#define	ELEMENT_PAIR_SCORE			(2)

#define	REF_FOUND_SCORE				(REF_PAIR_SCORE + (REF_PAIR_COUNT * ELEMENT_PAIR_SCORE))

/* TEST!!! To become parametrized */

#define	EXTRA_ELEMNENT_MATCH_REQUIRED	(1)
#define	REGION_PASSING_SCORE			(REF_FOUND_SCORE + (EXTRA_ELEMNENT_MATCH_REQUIRED * ELEMENT_PAIR_SCORE))

#define	REGION_NO_SCORE					(0)


/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

/*===========================  F U N C T I O N S  ============================*/

/*______________________________________________________________________________

    	$Id: hunter.c,v 1.6 2000/10/06 17:46:23 Manuel Exp $

		Module	: PQ-Hunter
        Function: find ROI within FOV
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
 *  Description:	main of hunter module
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
int MAIN(Hunter)(int dummy, int argc, PQ_CFG* Config, BYTE* Buffer)
{
	char*  BootMsg  = "PQ-hunter  [build: "__BUILD__"]  "__DATE__"  "__TIME__;
	bool   Exit = FALSE;
	time_t Elapsed;
	HPQ_RLC Rlc = NULL;
	int featcount;
	ftr features[MAX_NUM_OBJECTS];

	/* show info message on screen */
	SendDbgMsg(DBG_INFO, BootMsg);

	Rlc = fast_rlc_init(&(Config->RoI));
	if(!Rlc)
	{
		SendDbgMsg(DBG_ERRORS, "ERROR 25: memory overflow");
		return ERROR_MEMORY_OVERFLOW;
	}

	/* set video mode observe sync */
	SYNC();

	if (bDebugLevel >= DBG_ERRORS)
		vmode(vmOvlStill);      /* show the current image, update immediate */
	else
		vmode(vmOvlFreeze);     /* show the last image taken, always 1 behind */

	if(bDebugLevel >= DBG_INFO)
		ImageClearOverlay();	/* wipe off the text messages */

	/* make sure global variables are not overwritten */
	UseGlobals();

	setvar(intfl, 0);
	setvar(vstat, 0);

	/* we are in Hunting mode now */
	Config->bSearchMode = SEARCH_HUNT;
	Config->nCameraStatus = CS_SEEKING;

	/* RS232 */
	resRTS();	/* we do not want to receive anything while FinalHunting */

	/* set PLC */
	setPLC0();	/* camera ready: on */
	setPLC1();	/* camera scanning: on */

	while(!Exit)
	{
		/* make sure Trigger has been reset */
		while (IN0_SET())
			nop;

		tenable();

		/* wait for trigger */
		while (IN0_CLEARED())
		{
			/* check for stop scanning signal */
			if (IN1_SET())
			{
				Exit = TRUE;
			}
		}

		if(Exit)
			break;

		Elapsed = time(NULL);   /* note the time */

		resPLC0();			    /* Camera is busy */

		if(bDebugLevel >= DBG_ERRORS)
			cprintf(Console, "\f");

		ImageClearOverlay();   /* wipe off the text */

		InitSearchFoundResultBackup(Config);

		SelectAllMarksForSearch(Config);

		featcount = PrimaryHunting(Rlc, Config, features);

		if(Config->pFound)
		{
			Config->PatternMatchQuality = 50;
			Config->nCameraStatus = CS_READING;
		}
		else
		{
			Config->PatternMatchQuality = 0;
			Config->nCameraStatus = CS_SEEKING;
		}

		/* Data Validation Routines */
#ifdef EXTERNAL_CALL
		exec(MODULE_VALID, OVL_CALL, Config, features, featcount);
#else
		Validate(Config, features, featcount);
#endif

		Elapsed = time(NULL) - Elapsed;     /* how fast was the whole process? */

		/* ShowHeap(); */

		if(Config->pReferenceFound)
		{
			SendDbgMsg(DBG_ERRORS, "OK: %lu ms T:%d F:%d PId:%d", Elapsed,
						Config->Threshold, Config->pFound, Config->bTargetPatternID);
			SendDbgMsg(DBG_ERRORS, "REF FOUND");
		}
		else
		{
			SendDbgMsg(DBG_ERRORS, "OK: %lu ms - REF SEARCH FAIL", Elapsed);
		}

#ifdef EXTERNAL_CALL
		exec(MODULE_CONFIG, OVL_CALL, CMD_REQUEST_STATUS, &Config, Buffer, 0);
#else
		RequestStatus(Config, Buffer);
#endif

		/* at the end of 'SendDatagram' there is 'setRTS()',
		therefore allow no calls to camera ...	*/
		resRTS();

		/* keep track of flash cycles */
		Config->FlashCounter++;


		switch (bDebugLevel)	/* check if camera image should be sent to computer */
		{
			case DBG_ERRORS_GREY:
			case DBG_ERRORS_BIN:
			case DBG_DEBUG_GREY_MARK:
			case DBG_DEBUG_GREY:
			case DBG_DEBUG_BIN:
			{
			    /* send the image */
				exec(MODULE_IMAGE, OVL_CALL, Config);
				resRTS();
				break;
			}
		}

		/* Camera ready */
		setPLC0();
	}

	fast_rlc_clean(Rlc);

	/* reset PLC */
	resPLC0();	/* camera ready: off */
	resPLC1();	/* camera scanning: off */

	/* RS232 */
	setRTS();	/* we are ready to receive transmissions */

	/* we are back in idle mode */
	Config->bSearchMode = SEARCH_IDLE;
	Config->thresholdAdjustState = THRESHOLD_ADJUST_INIT;

	vmode(vmOvlLive);

	ImageClearOverlay();

	cprintf(Console, "\f");

	return ERROR_NONE;
}

/**FDOC*************************************************************************
 *
 *  Function:
 *
 *  Author:
 *
 *  Author:
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

/* #define		SHOW_THRESH_DEBUG	TRUE */

#define		ONEPAIR_OBJECT_COUNT	2

/* Three out of five region brightness fail detect */

#define		MAX_REGION_BRIGHTNESS_FAIL_COUNT	(4)

int	PrimaryHunting( HPQ_RLC Rlc, PQ_CFG* Config, ftr* features)
{
	int regionIdx,
		regionBrightnessFailCount,regionContrastFailCount,
		startThreshold, threshSearchCount,
		currScore, passingScore, extraMatchRequired;

	image regionImage;

    int minObjectCount;
	int	numberObjects = 0;

	BOOL	regionBrightnessOK, regionContrastOK,
			terminateRegionSearch;

	BOOL regionIsFound = FALSE;


/* #define ONE_REGION	TRUE */

#if defined(ONE_REGION)
	#define	MAX_REGION_COUNT	(1)

	POINT		regionStartPoints[MAX_REGION_COUNT] =
	{
		{205, 125}	/* Region 1 */
	};
#else
	#define	MAX_REGION_COUNT				(15)
	#define	MAX_REGION_CONTRAST_FAIL_COUNT	(MAX_REGION_COUNT)

	POINT		regionStartPoints[MAX_REGION_COUNT] =
	{
		{205, 125}, {205,  10},	{205, 240},		/* Region 1 - 3 */
		{303, 125},	{303,  10},	{303, 240},		/* Region 4 - 6 */
		{107, 125},	{107,  10},	{107, 240},		/* Region 7 - 9 */

		{400, 125},	{400,  10},	{400, 240},		/* Region 10 - 12 */
		{ 10, 125},	{ 10,  10},	{ 10, 240}		/* Region 13 - 15 */
	};
#endif

	regionImage.dx = REGION_WIDTH;
	regionImage.dy = REGION_HEIGHT;
	regionImage.pitch = ScrGetPitch;

	regionIdx = 0;
	regionBrightnessFailCount = 0;
	regionContrastFailCount = 0;

	/* (REF_FOUND_SCORE + (EXTRA_ELEMNENT_MATCH_REQUIRED * ELEMENT_PAIR_SCORE)) */

	/* extraMatchRequired = Config->bMarkSearchSelectCount - 2; */

	extraMatchRequired = 0;

	if(Config->bMarkSearchSelectCount > 1)
		extraMatchRequired = 1;

	passingScore =  (REF_FOUND_SCORE + (extraMatchRequired * ELEMENT_PAIR_SCORE));

	InitSearchVaribles(Config);

	while((regionIdx < MAX_REGION_COUNT) && (!regionIsFound))
	{
		regionImage.st =
			PIXEL_ADDR(regionStartPoints[regionIdx].x,
						regionStartPoints[regionIdx].y);

		Config->RoILocation[0] = regionStartPoints[regionIdx].x;
		Config->RoILocation[1] = regionStartPoints[regionIdx].y;

		AlignRoi(&regionImage, FALSE);  /* Align image to words */

		regionBrightnessOK			=
		regionContrastOK			=
		Config->BrightnessFailure	=
		Config->ContrastFailure		= FALSE;

		CalculateStartThreshold(Config, &regionImage);

		regionBrightnessOK	= (!Config->BrightnessFailure);
		regionContrastOK	= (!Config->ContrastFailure);

		if(regionBrightnessOK && regionContrastOK)
		{
			InitSearchVaribles(Config);

#if SHOW_THRESH_DEBUG
			cprintf(Console, "REGION BRIGHTNESS & CONTRAST OK");
#endif
			Config->RoI.st = regionImage.st;
			Config->RoI.dx = regionImage.dx;
			Config->RoI.dy = regionImage.dy;

#ifdef USE_SELFTEST
			if(regionIdx == 0)
				exec(MODULE_SELFTEST, OVL_CALL, Config);
#endif

			startThreshold = Config->Threshold;

			Config->prevScore = 0;
			threshSearchCount = 0;
			currScore = 0;

			terminateRegionSearch = FALSE;

			while((threshSearchCount < MAX_THRESH_SEARCH_COUNT) &&
					(!regionIsFound) && (!terminateRegionSearch))
			{
				InitSearchVaribles(Config);

#if SHOW_THRESH_DEBUG
				cprintf(Console, "Search %d THR %d", threshSearchCount, Config->Threshold);
#endif
				numberObjects = find_objects2(Rlc, Config, features);

				if(Config->bMarkSearchSelectCount == 1)
					minObjectCount = ONEPAIR_OBJECT_COUNT;
				else
					minObjectCount = MIN_OBJECT_COUNT;

				if((numberObjects < MAX_NUM_OBJECTS-1) &&
					(numberObjects >= minObjectCount))
				{
					/*CheckBlobs(numberObjects, features, Config); */
					exec(MODULE_CHKBLOB, OVL_CALL, numberObjects, features, Config);

					currScore = GetRegionScore(Config);

					if(currScore == REGION_NO_SCORE)
						terminateRegionSearch = TRUE;
					else
					if(currScore >= passingScore)
						regionIsFound = TRUE;
					else
					{
						InitSearchVaribles(Config);
						Config->Threshold += Config->HuntStepThreshold;
					}
				}
				else
				{
					if(numberObjects == TOO_MANY_OBJECTS_DETECTED)
					{
#if SHOW_THRESH_DEBUG
						cprintf(Console, "TOO MANY OBJECTS");
#endif
						terminateRegionSearch = TRUE;
					}
					else
					{
#if SHOW_THRESH_DEBUG
						cprintf(Console, "NOT ENOUGH OBJECTS");
#endif
						terminateRegionSearch = TRUE;
					}
				}

				if(currScore > Config->prevScore)
				{
#if SHOW_THRESH_DEBUG
					cprintf(Console, "RESET SEARCH");
#endif
					Config->prevScore = currScore;
					threshSearchCount = 0;
				}

				threshSearchCount++;
			}
		}
		else
		{
			if(!regionBrightnessOK)
			{
				/* cprintf(Console, "R:%d BRIGHTNESS FAILURE", regionIdx); */

				regionBrightnessFailCount++;

				if(regionBrightnessFailCount >= MAX_REGION_BRIGHTNESS_FAIL_COUNT)
					break;
			}
			else
			if(!regionContrastOK)
			{
				/* cprintf(Console, "R:%d CONTRAST FAILURE", regionIdx); */

				regionContrastFailCount++;

				if(regionContrastFailCount >=
					MAX_REGION_CONTRAST_FAIL_COUNT)
				{
					Config->ContrastFailure = TRUE;
					break;
				}
			}
		}

		regionIdx++;
	}

	if(regionBrightnessFailCount >= MAX_REGION_BRIGHTNESS_FAIL_COUNT)
		Config->BrightnessFailure = TRUE;
	else
		Config->BrightnessFailure = FALSE;

	if(regionContrastFailCount >= MAX_REGION_CONTRAST_FAIL_COUNT)
		Config->ContrastFailure = TRUE;
	else
		Config->ContrastFailure = FALSE;

	if(!Config->ContrastFailure && !Config->BrightnessFailure)
	{
		if(regionIsFound)
		{
#ifdef ONLY_FOUND_FRAME
			if(bDebugLevel >= DBG_ERRORS_GREY )
			{
				if(numberObjects > 0)
					DisplayFrame(&regionImage);

				mark_objects(&regionImage, features, numberObjects);
			}
#endif

#if SHOW_THRESH_DEBUG
			cprintf(Console, "REGION FOUND !!!");
#endif
		}
		else
		{
#if SHOW_THRESH_DEBUG
			cprintf(Console, "REGION NOT FOUND ???");
#endif
		}
	}

#ifndef ONLY_FOUND_FRAME
	if(bDebugLevel >= DBG_ERRORS_GREY )
	{
		if(numberObjects > 0)
			DisplayFrame(&regionImage);

		mark_objects(&regionImage, features, numberObjects);
	}
#endif

	/* return regionIsFound; */
	return numberObjects;

}

int		GetRegionScore(PQ_CFG* Config)
{
	int regScore = 0;

		if(Config->pFound)
		{
			if(Config->pReferenceFound)
				regScore = REF_PAIR_SCORE;

			regScore +=
				(Config->pFound * ELEMENT_PAIR_SCORE);
		}

	return regScore;
}

/* TEST!!! Histogram Start is at 4 because hito returns strange values
at the start of the array */

#define		MAX_HISTOGRAM_INDEX				256
#define		HISTOGRAM_START					2
#define		HISTOGRAM_END					(MAX_HISTOGRAM_INDEX - HISTOGRAM_START)

#define		MIN_REGISTER_ELEMENT_COUNT		2
#define		MIN_SUBSTRATE_ELEMENT_COUNT		36

#define		MIN_HUNT_STEP_THRESHOLD			2


void	CalculateStartThreshold(PQ_CFG* Config, image *imageRegion)
{
	unsigned long regionHistogram[MAX_HISTOGRAM_INDEX];

	int	i, biasAdjust;
	int substrateThreshold = 255;
	int elemThreshold = 0;
	int rangeThreshold;

	unsigned long pixAreaCount, histogramData;

	BOOL	regionBrightnessOK, regionContrastOK;

	int pId = Config->bTargetPatternID;
	PATTERN_CONFIG	*Patterns = Config->Patterns;

	unsigned long minElemArea =
			MIN_REGISTER_ELEMENT_COUNT * Patterns[pId].bMinElementSize;

	unsigned long minSubstrateArea =
			MIN_SUBSTRATE_ELEMENT_COUNT * Patterns[pId].bMinElementSize;


	histogram(imageRegion, regionHistogram);

	pixAreaCount = 0;

	for(i=HISTOGRAM_START; i<HISTOGRAM_END; i++)
	{
		histogramData = regionHistogram[i];

		pixAreaCount += histogramData;

		if(pixAreaCount > minElemArea)
		{
			elemThreshold = i;
			break;
		}
	}

	pixAreaCount = 0;

	for(i=HISTOGRAM_END-1; i>elemThreshold; i--)
	{
		histogramData = regionHistogram[i];

		pixAreaCount += histogramData;

		if(pixAreaCount > minSubstrateArea)
		{
			substrateThreshold = i;
			break;
		}
	}

	rangeThreshold = substrateThreshold - elemThreshold;

	Config->Contrast = (100 * rangeThreshold) >> 8;
	Config->Brightness = substrateThreshold;

	Config->ContrastFailure = (Config->Contrast < Config->MinContrast);
	Config->BrightnessFailure = (Config->Brightness < Config->MinBrightness);

	regionBrightnessOK	= (!Config->BrightnessFailure);
	regionContrastOK	= (!Config->ContrastFailure);

	if(regionBrightnessOK && regionContrastOK)
	{
#if SHOW_THRESH_DEBUG
	SendDbgMsg(DBG_ERRORS, "ElemTHR:%d  SubsTHR:%d  RANGE:%d TCONTR:%d",
				elemThreshold, substrateThreshold,
				rangeThreshold, Config->Contrast);
#endif

		Config->BaseThreshold =
			(elemThreshold + substrateThreshold) >> 1 ;

		/* Lower Starting Threshold , Slope = 1/4	Intercept = +15 */

		biasAdjust = ((rangeThreshold >> 2) - 15 );

		Config->BaseThreshold += biasAdjust;
		Config->Threshold = Config->BaseThreshold;

		Config->HuntStepThreshold = (biasAdjust >> 1);  /* Step is half biasAdjust */

		if(	Config->HuntStepThreshold < MIN_HUNT_STEP_THRESHOLD) /* but no less than min */
				Config->HuntStepThreshold = MIN_HUNT_STEP_THRESHOLD;

		/* retard start threshold by 2 steps */
		Config->Threshold -= (Config->HuntStepThreshold * 2);

#if SHOW_THRESH_DEBUG
		cprintf(Console, "BASE:%d  BIAS:%d STEP:%d",
					Config->BaseThreshold, biasAdjust, Config->HuntStepThreshold);
#endif
	}
}


void	UpdateForBrightnessFailure(PQ_CFG* Config)
{
	int	pT;

	REG_POS	*strcRegPos = Config->strcRegPos;

	for(pT=0; pT<NREGMARK; pT++)
	{
		if(Config->bMarkEnabled[pT])
		{
			strcRegPos[pT].nMarkStatus =
				MARK_MISSING_BRIGHTNESS_FAILURE;
		}
	}
}

void	UpdateForContrastFailure(PQ_CFG* Config)
{
	int	pT;

	REG_POS	*strcRegPos = Config->strcRegPos;

	for(pT=0; pT<NREGMARK; pT++)
	{
		if(Config->bMarkEnabled[pT])
		{
			strcRegPos[pT].nMarkStatus =
				MARK_MISSING_CONTRAST_FAILURE;
		}
	}
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

