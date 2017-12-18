/*______________________________________________________________________________

    	$Id: checkblobs.c,v 1.6 2000/10/06 17:46:23 Manuel Exp $

		Module	: Check Blobs
        Function: filter blobs and preprare for pattern matching
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

/*=======================  I N C L U D E   F I L E S  ========================*/

/* standard C headers*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* standard vision components headers*/
#include <vcrt.h>
#include <vclib.h>
#include <cam0.h>
#include <vc65.h>
#include <macros.h>
/* standard fibervision headers*/
#include <bool.h>
#include <vcdefs.h>

/* project library headers*/
#include "image.h"
#include "pqlib.h"
/* misc project headers*/
#include "config.h"
#include "global.h"
#include "error.h"
#include "command.h"
#include "patdef.h"
/* module headers*/
#include "build.h"
#include "pqlib.h"

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/
/*==================  E X T E R N A L   V A R I A B L E S  ===================*/
/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/
/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

#define		MIN_CANDIDATE_COUNT			2
#define		MIN_SINGLE_BLOB_COUNT		2
#define		MAX_LARGE_BLOB_COUNT		4

/* #define GRAYVALUE_STUFF */

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/
/*===========================  F U N C T I O N S  ============================*/

int Chkblob(int numobjects, ftr *f, PQ_CFG* Config)
{
#ifdef WITH_TIMING
	time_t elapsed = time(NULL);
#endif
	CANDIDATE_BLOBS		*CandidateBlobs;

	long maxElementArea, maxCompoundBlobArea, minBlobArea, minElementArea, thisArea;
	int maxElementWidth, maxElementHeight;

	int nb, dx, dy;
	int nextValidBlob;
	int pId;

	ftr *curFeature;
	bool isBlack;
	int skipped1 = 0;
	int skipped2 = 0;  /* number of filtered objects*/

	int maxCompoundWidth, maxCompoundHeight;

	int numElementX, numElementY;

	pId = Config->bTargetPatternID;

	minElementArea = Config->Patterns[pId].bMinElementSize;
	maxElementArea = Config->Patterns[pId].bMaxElementSize;
	maxElementWidth = isqrt(maxElementArea);
	maxElementHeight = maxElementWidth;

	/* maximum size (x,y) for large compounds */
	maxCompoundBlobArea = maxElementArea * (Config->bMarkSearchSelectCount - 1);
	maxCompoundWidth = Config->bMarkSearchSelectCount * maxElementWidth ;
	maxCompoundHeight = Config->bMarkSearchSelectCount * maxElementHeight ;

	CandidateBlobs = (CANDIDATE_BLOBS *) vcmalloc(sizeof(CANDIDATE_BLOBS));

	if( !CandidateBlobs )
		SendDbgMsg(DBG_ERRORS,"CAND BLOB FAILED MALLOC");

	Config->singleBlobCount = 0;
	Config->largeBlobCount = 0;

	CandidateBlobs->blobCount = 0;
	nextValidBlob = 0;

	for(nb = 0; nb < numobjects; nb++)
	{
		curFeature = &(f[nb]);

		isBlack = curFeature->color == BLACK;

		if (!isBlack)
			continue;

		#ifdef GRAYVALUE_STUFF
		{
			/* create image structure based on blob bounding box */
			image thisBlob = ImageCreate(curFeature->x_min, curFeature->y_min,
				curFeature->x_max - curFeature->x_min + 1,
				curFeature->y_max - curFeature->y_min + 1, IMG_DEFAULT);

			/* correct start address by ROI offset */
			thisBlob.st += Config->RoI.st;

			/* make roi word aligned */
			AlignRoi(&thisBlob, TRUE);

			/* calculate the mean gray value of the blob */
			meanGrayvalue = calculateMeanGrayvalue(&thisBlob);
			cprintf(Console, "average gray value :%d", meanGrayvalue);
		}
		#endif

		thisArea = curFeature->area;

		dx = curFeature->x_max - curFeature->x_min;
		dy = curFeature->y_max - curFeature->y_min;

		/* FILTER 1 : bounding box size */
		/* The upper limit of the bounding box size is the max compound blob size */
		/* The lower limit of the bounding box height is the line filter size */
		/* Line filter is used for horizontal direction during RLC check.
		   We are immitating the same filter for vertical direction */

		if((dy < Config->LineFilterSize) || (dx > maxCompoundWidth) || (dy > maxCompoundHeight))
		{
			skipped1++;
			continue;
		}

		/* FILTER 2 : blob area size */
		/* check if the blob is plain square */
		/* the blob area should not be less than the product of the min element size and the number of element in the blob */

 		numElementX = 1 + (dx / maxElementWidth);
		numElementY = 1 + (dy / maxElementHeight);

		minBlobArea = (numElementX > numElementY) ? (numElementX * minElementArea) : (numElementY * minElementArea);

		/*
					cprintf(Console, "min blob area  :%ld", minBlobArea);
		*/

		if ((thisArea < minBlobArea) || (thisArea > maxCompoundBlobArea))
		{
			skipped2++;
			continue;
		}

		/* cprintf(Console, "blob area  :%ld", thisArea);*/

		if (numElementX > 1 || numElementY >1)
		{
            Config->largeBlobCount++;
			CandidateBlobs->smallBlob[nextValidBlob] = FALSE;
		}
		else
		{
			Config->singleBlobCount++;
			CandidateBlobs->smallBlob[nextValidBlob] = TRUE;
		}

#ifdef GRAYSCALE_CENTROIDS
#	ifdef TEST_GRAYSCALE_CENTROIDS
		/* get centroids from grayscale image */
        if(nb==1)
			FindCOG(&(Config->RoI), curFeature, maxElementWidth);
#	else
		FindCOG(&(Config->RoI), curFeature, maxElementWidth);
#	endif
#endif

		CandidateBlobs->blobRect[nextValidBlob].center.x =
			(signed int)(curFeature->x_center);
		CandidateBlobs->blobRect[nextValidBlob].center.y =
			(signed int)(curFeature->y_center);

		CandidateBlobs->blobRect[nextValidBlob].top.x = curFeature->x_min;
		CandidateBlobs->blobRect[nextValidBlob].top.y = curFeature->y_min;

		CandidateBlobs->blobRect[nextValidBlob].bottom.x = curFeature->x_max;
		CandidateBlobs->blobRect[nextValidBlob].bottom.y = curFeature->y_max;

#ifdef USE_SUBPIXELS
#warning subpixeling enabled
		CandidateBlobs->blobRect[nextValidBlob].top.x    *= (int) SCALE_FACTOR;
		CandidateBlobs->blobRect[nextValidBlob].top.y    *= (int) SCALE_FACTOR;
		CandidateBlobs->blobRect[nextValidBlob].bottom.x *= (int) SCALE_FACTOR;
		CandidateBlobs->blobRect[nextValidBlob].bottom.y *= (int) SCALE_FACTOR;
#endif

		CandidateBlobs->blobArea[nextValidBlob] = thisArea;

		CandidateBlobs->inferenceCount[nextValidBlob] = 0;

		CandidateBlobs->blobCount++;
		nextValidBlob++;

		if(nextValidBlob == MAX_NUM_CANDIDATE_BLOBS)
		{
			cprintf(Console, "MAX CANDIDATE Count Reached");
			break;
		}
	}

	if(bDebugLevel >= DBG_ERRORS)
	{
/*		ImageClearOverlay();*/

/*		cprintf(Console, "SINGLE:%d LARGE:%d",
					Config->singleBlobCount, Config->largeBlobCount);*/
/*
		if(Config->bSearchMode == SEARCH_TRACK)
			cprintf(Console, "Filter[%d]: %d %d Cand: %d", numobjects-1,
					skipped1, skipped2, CandidateBlobs->blobCount);
*/

		mark_candidates(&(Config->RoI), CandidateBlobs);
	}

	if((CandidateBlobs->blobCount >= MIN_CANDIDATE_COUNT) &&
		(Config->singleBlobCount >= MIN_SINGLE_BLOB_COUNT) )
#ifdef WITH_TIMING
	{
		time_t elapsed2 = time(NULL);
		exec(MODULE_PATTERN, OVL_CALL, Config, CandidateBlobs);
		cprintf(Console, "    pattern gross: %ld", time(NULL) - elapsed2);
	}
#else
	exec(MODULE_PATTERN, OVL_CALL, Config, CandidateBlobs);
#endif

	if( CandidateBlobs )
		vcfree( CandidateBlobs );

#ifdef WITH_TIMING
	cprintf(Console, "   chkblob net: %ld", time(NULL) - elapsed);
#endif
	return nextValidBlob;
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

