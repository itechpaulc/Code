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
 * $Header:   K:/Projects/PQCamera/PQCamera/TRACKER/tracker.c_v   1.19   Jun 12 2003 17:16:26   APeng  $
 *
 * $Log:   K:/Projects/PQCamera/PQCamera/TRACKER/tracker.c_v  $
 * 
 *    Rev 1.19   Jun 12 2003 17:16:26   APeng
 *  
 * 
 *    Rev 1.17   May 09 2003 11:06:16   APeng
 *  
 * 
 *    Rev 1.16   May 08 2003 10:36:06   APeng
 *  
 * 
 *    Rev 1.15   May 02 2003 10:32:02   APeng
 *  
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
#include "storage.h"
#include "pqlib.h"

#include "patdef.h"

#include "tracker.h"


/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

/*#define USE_SELFTEST*/
#define EXTERNAL_CALL
#define SLOW_CODE

#define	THRESHOLD_RETRY				20	/* Retries before changing Threshold value */

/*
	Tracker performs a retry when any mark is not found.
	The first retry allows the camera to re-threshold based on an ROI that has moved.
	The second retry increases the threshold by 1 step to bring out the low contrast marks.
	The third retry increases the threshold by one more step.
*/

#define	MAX_TRACKER_RETRY_CENTER_ROI	(4)
#define	MAX_TRACKER_RETRY_FIND_ALL		(6)
#define	MIN_TRACKER_ADJUST_STEPS		(2)

#define	RESET_TRACKER_RETRY				(0)

#define		ONEPAIR_MIN_FOUND_ROI_MOVE		1
#define		MIN_FOUND_ROI_MOVE				2


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
int MAIN(Tracker)(int dummy, int argc, PQ_CFG* Config, BYTE* Buffer)
{
	char*  BootMsg  = "PQ-tracker  [build: "__BUILD__"]  "__DATE__"  "__TIME__"";
	bool   Exit = FALSE, patternFoundChange;
	ftr features[MAX_NUM_OBJECTS];
	int featcount = 0;
	time_t Elapsed;
#ifdef WITH_TIMING
	time_t Elapsed2;
#endif
	
	int minFoundRoiMove;
	int i, sumThreshAdjust;
	int frequencyMarkAllFound = 0;
	long j, temp;
	int thresh[8], nMarkFound[8];
	int thresholdLimit;

	HPQ_RLC Rlc = NULL;

	/* reset ROI size before tracking */
	int dx = getvar(hwidth) * 2;
	int dy = getvar(vwidth);
	Config->RoI        = ImageCreate((dx-TRACK_IMAGE_SIZE)/2, (dy-TRACK_IMAGE_SIZE)/2,
						TRACK_IMAGE_SIZE, TRACK_IMAGE_SIZE, IMG_DEFAULT);

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
		vmode(vmOvlStill);
	else
		vmode(vmFreeze);

	if(bDebugLevel >= DBG_INFO)
		ImageClearOverlay();	/* wipe off the text messages */

	/* make sure global variables are not overwritten */
	UseGlobals();

	setvar(intfl, 0);
	setvar(vstat, 0);

	/* we are in tracking mode now */
	Config->bSearchMode = SEARCH_TRACK;
	Config->nCameraStatus = CS_SEEKING;

	/* RS232 */
	resRTS();	/* we do not want to receive anything while tracking */

	/* set PLC */
	setPLC0();	/* camera ready: on */
	setPLC1();	/* camera scanning: on */

	Config->pcf1 = 0;
	Config->pcf2 = 0;

	TrackUpdateThreshold(Config); /* init */

	/* Keep tracking if no stop signal */
	while(!Exit)
	{
		/* make sure Trigger has been reset */
		while (IN0_SET())
			nop;

		tenable();

		/* wait for trigger */
		while(IN0_CLEARED())
		{
			/* check for stop scanning signal */
			if (IN1_SET())
				Exit = TRUE;
		}

		if (Exit)
			break;

		Elapsed = time(NULL);

		/* Camera busy */
		resPLC0();

		if(bDebugLevel >= DBG_ERRORS)
			cprintf(Console, "\f");

		ImageClearOverlay();    /* wipe off the text */
		DisplayFrame(&(Config->RoI));      /* frame the ROI area */


#ifdef USE_SELFTEST
		exec(MODULE_SELFTEST, OVL_CALL, Config);
#endif

		/* ====== Locate the marks and center the ROI ====== */

		/* Initiate search variables */
		InitRegMeasUnstableCount(Config);
		InitSearchFoundResultBackup(Config);
		InitSearchVaribles(Config);

		if ((!Config->BrightnessFailure) && (!Config->ContrastFailure))
		{
			if(!Config->pMarkSearchAllFound)
			{
				/* ====== Track the marks within the ROI ====== */
				Config->TrackRetry = RESET_TRACKER_RETRY;

				/* Re-initiate search variables */
 				InitSearchFoundResultBackup(Config);
				InitSearchVaribles(Config);
				SelectAllMarksForSearch(Config);

				/* Within the centered ROI, keep searching the marks until
				the max number of retries is reached or all the marks are found */
				while((!Config->pMarkSearchAllFound) && (Config->TrackRetry <= MAX_TRACKER_RETRY_FIND_ALL))
				{
					InitTrackVariablesForRetry(Config);

					/*featcount = FindMarks(Rlc, Config, features);*/

#ifdef WITH_TIMING
					Elapsed2 = time(NULL);
#endif
#ifdef EXTERNAL_CALL
					featcount = FindMarks(Rlc, Config, features); /* here is where the external calls are made*/
#else
					featcount = exec(MODULE_FINDMARK, OVL_CALL, Rlc, Config, features);
#endif
#ifdef WITH_TIMING
					cprintf(Console, "findmark gross: %ld", time(NULL) - Elapsed2);
#endif
					   
					StoreMarksFound(Config);
					ShowMarksFound(Config);

                    thresh[Config->TrackRetry] = Config->AdjustedThreshold;
					nMarkFound[Config->TrackRetry] = Config->pFound;

					Config->TrackRetry++;

					thresholdLimit = (int)(0.90 * (float)Config->Brightness);

					if (Config->AdjustedThreshold > thresholdLimit || featcount == TOO_MANY_OBJECTS_DETECTED)
						break;
				}

				UpdatePatternFound(Config);
			}
		}

		if(!Config->pReferenceFound)
			cprintf(Console, "REF NOT FOUND");

		if(Config->pFound)
		{
			Config->nCameraStatus = CS_READING;
			Config->PatternMatchQuality = 50;
			/*TrackUpdateThreshold(Config);*/

			/* cprintf(Console, "Adjust State:%d", Config->thresholdAdjustState);*/

			Config->FinalThreshold = Config->AdjustedThreshold;
			if(Config->pMarkSearchAllFound) {
				Config->TrackerAdjustSteps = (Config->FinalThreshold - Config->BaseThreshold) / 2;
			}
			else 
			{
				for (i = Config->TrackRetry - 1; i > 0; i--)
				{
					if (nMarkFound[i] != nMarkFound[i-1]) 
					{
						Config->FinalThreshold = thresh[i];
						patternFoundChange = 1;
						break;
					}
				}

				if (!patternFoundChange)
					Config->TrackerAdjustSteps *= 2;
				else 
					if ((featcount == TOO_MANY_OBJECTS_DETECTED) || (Config->AdjustedThreshold > Config->Brightness))
						Config->TrackerAdjustSteps /= 2;
					else
						Config->TrackerAdjustSteps = (Config->FinalThreshold - Config->BaseThreshold) / 3;					
			}

			if(Config->TrackerAdjustSteps < MIN_TRACKER_ADJUST_STEPS)
				Config->TrackerAdjustSteps = MIN_TRACKER_ADJUST_STEPS;

			cprintf(Console, "Threshold:%d", Config->BaseThreshold);
			cprintf(Console, "Final threshold:%d", Config->FinalThreshold);
 			cprintf(Console, "Adjusted Step:%d", Config->TrackerAdjustSteps);

			if(Config->pMarkSearchAllFound)
				Config->pcf1++;
		}
		else
		{
			Config->nCameraStatus = CS_SEEKING;
			Config->PatternMatchQuality = 0;
		}

		/* Data Validation Routines */
#ifdef WITH_TIMING
		Elapsed2 = time(NULL);
#endif
#ifdef EXTERNAL_CALL
		exec(MODULE_VALID, OVL_CALL, Config, features, featcount);
#else
		Validate(Config, features, featcount);
#endif
#ifdef WITH_TIMING
		cprintf(Console, "valid gross: %ld", time(NULL) - Elapsed2);
#endif

		Config->pcf2++;
		Config->pcfCurr = (long)(Config->pcf1*100)/(long)(Config->pcf2);

		/* cprintf(Console, "CAM STATUS:%d", Config->nCameraStatus);*/

		/* Center ROI around marks found at the end of tracking */
		/*
		if(Config->bMarkSearchSelectCount == 1)
			minFoundRoiMove = ONEPAIR_MIN_FOUND_ROI_MOVE;
		else
			minFoundRoiMove = MIN_FOUND_ROI_MOVE;
		*/

		/*if(Config->pReferenceFound && Config->pFound >= minFoundRoiMove)*/
		if(Config->pReferenceFound)
		{
			CenterROI(Config);

			/* Frame the ROI area */
			DisplayFrame(&(Config->RoI));
		}

		/* cprintf(Console, "CAM STATUS:%d", Config->nCameraStatus);*/

		SendDbgMsg(DBG_ERRORS, "T%:%d:%d (F:%d/%d) (C:%d B:%d) [PCF:%ld]"
					, Config->Threshold, Config->AdjustedThreshold
					, Config->pFound, Config->bMarkSearchSelectCount
					, Config->Contrast, Config->Brightness, Config->pcfCurr);

		/* Send Data and Status to CCU */
#ifdef WITH_TIMING
		Elapsed2 = time(NULL);
#endif
#ifdef EXTERNAL_CALL
		exec(MODULE_CONFIG, OVL_CALL, CMD_REQUEST_STATUS, &Config, Buffer, 200);
#else
		RequestStatus(Config, Buffer);
#endif
#ifdef WITH_TIMING
		cprintf(Console, "CMD_REQUEST_STATUS: %ld", time(NULL) - Elapsed2);
#endif

		/* at the end of 'SendDatagram' there is 'setRTS()',
		therefore allow no calls to camera ...	*/
		resRTS();

		/* keep track of flash cycles */
		Config->FlashCounter++;

		switch (bDebugLevel)
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

		/* every SAVE_COUNT_INTERVAL times we want to save the FlashCounter */
		if ((Config->FlashCounter % SAVE_COUNT_INTERVAL) == 0)
			storage(IO_SAVE, Config, sizeof(PQ_CFG));

		/* Camera ready */
		setPLC0();
		Elapsed = time(NULL) - Elapsed;
		SendDbgMsg(DBG_ERRORS, "OK: [%ld] %lu ms", Config->FlashCounter, Elapsed);
	}

	fast_rlc_clean(Rlc);

	/* reset PLC */
	resPLC0();	/* camera ready: off */
	resPLC1();	/* camera scanning: off */

	/* RS232 */
	setRTS();	/* we are ready to receive transmissions */

	/* we are back in idle mode */
	Config->bSearchMode = SEARCH_IDLE;

	vmode(vmOvlLive);

	ImageClearOverlay();

	cprintf(Console, "\f");

	return ERROR_NONE;
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
#ifdef EXTERNAL_CALL
int FindMarks(HPQ_RLC Rlc, PQ_CFG* Config, ftr* features)
{
#ifdef WITH_TIMING
	time_t elapsed2 = time(NULL);
#endif
	int origThreshold;

	int	minObjectCount, numberObjects = 0;

	origThreshold = Config->Threshold;

#ifdef WITH_TIMING
	{
		time_t elapsed = time(NULL);
		exec(MODULE_THRESH, OVL_CALL, Config);
		cprintf(Console, "  threshold gross: %ld", time(NULL) - elapsed);
	}
#else
	exec(MODULE_THRESH, OVL_CALL, Config);
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
		exec(MODULE_CHKBLOB, OVL_CALL, numberObjects, features, Config);
		cprintf(Console, "  chkblob gross: %ld", time(NULL) - elapsed);
#else
		exec(MODULE_CHKBLOB, OVL_CALL, numberObjects, features, Config);
#endif

		/* display blob data and mark the image */
		if(bDebugLevel >= DBG_ERRORS_GREY )
			mark_objects(&(Config->RoI), features, numberObjects);
	}

	DisplayFrame(&(Config->RoI));      /* frame the ROI area */

	Config->Threshold = origThreshold;

#ifdef WITH_TIMING
	cprintf(Console, " findmark net: %ld", time(NULL) - elapsed2);
#endif
	return numberObjects;
}
#endif
/**FDOC*************************************************************************
 *
 *  Function:       TrackUpdateThreshold
 *
 *  Author:         Paul L C
 *
 *  Description:    Updates Threshold value base on Pattern Match Results
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

void	TrackUpdateThreshold(PQ_CFG* Config)
{
	if(Config->thresholdAdjustState == THRESHOLD_ADJUST_INIT)
	{
		Config->TrackerAdjustSteps = Config->HuntStepThreshold >> 1 ;

		if(Config->TrackerAdjustSteps < MIN_TRACKER_ADJUST_STEPS)
			Config->TrackerAdjustSteps = MIN_TRACKER_ADJUST_STEPS;

		Config->AvgMidpointThreshold = Config->Threshold;

		Config->thresholdAdjustFailCount = 0;
		Config->thresholdAdjustState = THRESHOLD_ADJUST_ALL_FOUND;
	}
	else
	if(Config->thresholdAdjustState == THRESHOLD_ADJUST_ALL_FOUND)
	{
			if(Config->pcfCurr < Config->MinTrackingPcfLevel)
			{
				if(Config->pcfCurr > Config->pcfPrev)
				{
					Config->pcfPrev = Config->pcfCurr;
					Config->thresholdAdjustFailCount = 0;
				}
				else
					Config->thresholdAdjustFailCount++;

				if(Config->thresholdAdjustFailCount > THRESHOLD_RETRY)
					Config->thresholdAdjustState = THRESHOLD_ADJUST_SEARCH_BEGIN;
				else
					Config->thresholdAdjustState = THRESHOLD_ADJUST_ALL_FOUND;
			}
			else
			{
				Config->thresholdAdjustFailCount = 0;
				Config->thresholdAdjustState = THRESHOLD_ADJUST_ALL_FOUND;
			}
	}
	else
	{
		exec(MODULE_FINETUNE, OVL_CALL, Config);
	}
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

void	ShowMarksFound(PQ_CFG* Config)
{
	int		pT;
	int		markStat[NREGMARK], refStat, allFound;

	for(pT=0; pT<NREGMARK; pT++)
	{
		if((Config->strcRegPos[pT].nMarkStatus == MARK_FOUND) ||
			(Config->regFoundResultBackup[pT] == TRUE))
				markStat[pT] = pT+1;
		else
			markStat[pT] = 0;
	}

	refStat = (Config->pReferenceFound) ? 1 : 0;
	allFound = Config->pMarkSearchAllFound ? 1 : 0;

	cprintf(Console, " [T%d A%d R%d] [%d %d %d %d][%d %d %d %d]",
				Config->AdjustedThreshold, allFound, refStat,
						markStat[0], markStat[1],
						markStat[2], markStat[3],
						markStat[4], markStat[5],
						markStat[6], markStat[7]);
}


void	CenterROI(PQ_CFG* Config)
{
#ifdef SLOW_CODE
	int x, y;

	cprintf(Console, "Center ROI ...");

	MoveRoINew(Config);

	x = ScrGetX((Config->RoI).st);
	y = ScrGetY((Config->RoI).st);

	/* update the location of the RoI */

	Config->RoILocation[0] = x;
	Config->RoILocation[1] = y;
#else
	long ByteAdrr = (Config->RoI).st - (Config->FoV).st;
	long Pitch    = (long) ScrGetPitch;

	cprintf(Console, "Center ROI (new)...");

	MoveRoINew(Config);

	/* update the location of the RoI */
	Config->RoILocation[0] = (WORD) (ByteAdrr % Pitch);
	Config->RoILocation[1] = (WORD) (ByteAdrr / Pitch);
#endif
}

void	InitRegMeasUnstableCount(PQ_CFG* Config)
{
#ifdef SLOW_CODE
	int pT;

	for(pT=0; pT<NREGMARK; pT++)
	{
		Config->RegMeasUnStableCount[pT] = 0;
	}
#else
	memset(Config->RegMeasUnStableCount, 0, NREGMARK * sizeof(BYTE));
#endif
}


/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

