 /**SDOC*************************************************************************
 *
 *  pattern.c - pattern matching logic for PrintQuick camera firmware
 *
 *  Author: Mark Colvin			July 12, 2001
 *
 * $Header:   K:/Projects/PQCamera/PQCamera/pattern/pattern.c_v   1.9   Jun 17 2003 11:41:36   APeng  $
 *
 * $Log:   K:/Projects/PQCamera/PQCamera/pattern/pattern.c_v  $
 * 
 *    Rev 1.9   Jun 17 2003 11:41:36   APeng
 *  
 * 
 *    Rev 1.8   Jun 05 2003 15:18:32   APeng
 *  
 * 
 *    Rev 1.7   May 02 2003 10:32:08   APeng
 *  
 *
 *    Rev 1.6   Mar 07 2003 13:57:22   APeng
 *
 *
 *    Rev 1.5   Feb 21 2003 15:27:38   APeng
 *
 *
 *    Rev 1.4   Dec 30 2002 11:57:52   APeng
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
 *    Rev 1.3   Dec 13 2002 11:33:12   APeng
 * Added function to handle stripes and smudges around the registration marks
 *
 *
 *
 *    Rev 1.1   10 Aug 2001 10:08:58   MARKC
 * added Paul's code for new Pattern Match (see SSI07.22.01-1-A)
 *
 *    Rev 1.0   17 Jul 2001 11:28:18   MARKC
 * Initial code to work with tracker module
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
#include "build.h"
#include "pattern.h"

#define USE_GRAYVALUE_FOR_PATTERN_MATCHING
#define CORNER_SIZE		5

GMI_ASSERTFILE (__FILE__);

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

#define min(a,b)	((a)<(b)?(a):(b))
#define max(a,b)	((a)>(b)?(a):(b))

#define SCALED_IMAGE_WIDTH  (IMAGE_WIDTH  * (int) SCALE_FACTOR)
#define SCALED_IMAGE_HEIGHT (IMAGE_HEIGHT * (int) SCALE_FACTOR)

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/


PRIVATE		PATTERN_SEARCH_VECTORS		PQ_SearchVectors;
PRIVATE		PATTERN_SEARCH_VECTORS		*SearchVectors;

PRIVATE		PATTERN_SEARCH_BOUNDARY		PQ_PatternSearchBoundary;
PRIVATE		PATTERN_SEARCH_BOUNDARY		*PatternSearchBoundary;

PRIVATE		COMPARISON_DONE_MATRIX		PQ_ComparisonDoneMatrix;
PRIVATE		COMPARISON_DONE_MATRIX		*ComparisonDoneMatrix;

PRIVATE		PAIRING_TABLE				PQ_PairingTable[MAX_NUM_CANDIDATE_BLOBS];
/*PRIVATE		PAIRING_TABLE				*PairingTable;*/

PRIVATE		int		currTolerance;

/* Printing Tolerance and Skew Tolerance */

#ifdef USE_SUBPIXELS
#	warning subpixeling enabled
#	define		INIT_BOUNDARY_TOLERANCE		(2 * (int) SCALE_FACTOR)
#else
#	define		INIT_BOUNDARY_TOLERANCE		2
#endif

PRIVATE		RECT	MinSearchBoundary;

/* Four corners test */

#define		CORNER_COUNT	4
PRIVATE		RECT			largeBlobRect[MAX_NUM_CANDIDATE_BLOBS - CORNER_COUNT];

/*===========================  F U N C T I O N S  ============================*/

/* Member Functions */

#ifndef INLINE
/*#	define INLINE static inline*/
#	define INLINE
#endif

INLINE void	PARTNER_LINK_SetIsActive(PARTNER_LINK *partnerLink)
{
	partnerLink->partnerLinkData |= IS_ACTIVE_MASK;
}

INLINE void	PARTNER_LINK_ClearIsActive(PARTNER_LINK *partnerLink)
{
	partnerLink->partnerLinkData &= ~IS_ACTIVE_MASK;
}

INLINE BOOL	PARTNER_LINK_IsActive(PARTNER_LINK *partnerLink)
{
	if(partnerLink->partnerLinkData & IS_ACTIVE_MASK)
		return TRUE;

	return FALSE;
}

INLINE void	PARTNER_LINK_SetIsTopElement(PARTNER_LINK *partnerLink)
{
	partnerLink->partnerLinkData |= IS_TOPELEMENT_MASK;
}

INLINE void	PARTNER_LINK_ClearIsTopElement(PARTNER_LINK *partnerLink)
{
	partnerLink->partnerLinkData &= ~IS_TOPELEMENT_MASK;
}

INLINE BOOL	PARTNER_LINK_IsTopElement(PARTNER_LINK *partnerLink)
{
	if(partnerLink->partnerLinkData & IS_TOPELEMENT_MASK)
		return TRUE;

	return FALSE;
}

INLINE void PARTNER_LINK_SetElementTypeMatched
(PARTNER_LINK *partnerLink, BYTE partnerEtm)
{
	partnerLink->partnerLinkData &= ~ELEMENTTYPEMATCHED_MASK;

	partnerLink->partnerLinkData |=	(partnerEtm << 2);
}

INLINE BYTE PARTNER_LINK_GetElementTypeMatched(PARTNER_LINK *partnerLink)
{
	return
		(partnerLink->partnerLinkData & ELEMENTTYPEMATCHED_MASK) >> 2;
}

INLINE void PARTNER_LINK_SetPartnerBlobIdMatched
(PARTNER_LINK *partnerLink, BYTE partnerBim)
{
	ByteSetHi(partnerLink->partnerLinkData, partnerBim);
}

INLINE BYTE PARTNER_LINK_GetPartnerBlobIdMatched(PARTNER_LINK *partnerLink)
{
	BYTE partnerBim;

		partnerBim = ByteGetHi(partnerLink->partnerLinkData);

	return	partnerBim;
}


/*===========================  F U N C T I O N S  ============================*/

INLINE BOOL IsPatternEnabled( int patternType, PQ_CFG* Config)
{
	return Config->bMarkSearchSelect[patternType];
}

INLINE BOOL IsPatternFound( int patternType, PQ_CFG* Config)
{
	if(Config->regFoundResultBackup[patternType] == TRUE)
		return TRUE;

	return FALSE;
}

INLINE BOOL IsPatternMissing( int patternType, PQ_CFG* Config)
{
	return (!IsPatternFound(patternType, Config));
}

INLINE BOOL AllElementsResolved( CANDIDATE_BLOBS *CandidateBlobs)
{
	int bA, blobCount;

		blobCount = CandidateBlobs->blobCount;

		for(bA=0; bA<blobCount; bA++)
		{
			if(PQ_PairingTable[bA].numActiveLink > 0)
				return FALSE;
		}

	return TRUE;
}

INLINE BOOL IsPointInBound( POINT *point, RECT *bound)
{
	if(((point->x >= bound->top.x) && (point->y >= bound->top.y)) &&
		((point->x <= bound->bottom.x) && (point->y <= bound->bottom.y)))
			return TRUE;

	return FALSE;
}

INLINE BOOL IsRectInBound( RECT *rect, RECT *bound)
{
	if(((rect->top.x >= bound->top.x) && (rect->top.y >= bound->top.y)) &&
		((rect->bottom.x <= bound->bottom.x) &&	(rect->bottom.y <= bound->bottom.y)))
			return TRUE;

	return FALSE;
}

INLINE BOOL MoveRect( POINT *ptOffset, RECT *rect)
{
		rect->top.x += ptOffset->x;
		rect->top.y += ptOffset->y;

		if((rect->top.x < 0) || (rect->top.y < 0))
		{
			return FALSE;
		}

		rect->bottom.x += ptOffset->x;
		rect->bottom.y += ptOffset->y;

#ifdef USE_SUBPIXELS
		if((rect->bottom.x > SCALED_IMAGE_WIDTH) || (rect->bottom.y > SCALED_IMAGE_HEIGHT))
		{
			return FALSE;
		}
#else
		if((rect->bottom.x > IMAGE_WIDTH) || (rect->bottom.y > IMAGE_HEIGHT))
		{
			return FALSE;
		}
#endif

	rect->center.x += ptOffset->x;
	rect->center.y += ptOffset->y;

	return TRUE;
}

INLINE void CenterRect( POINT *ptCenter, RECT *rect)
{
	rect->top.x = rect->top.x - rect->center.x + ptCenter->x;
	rect->top.y = rect->top.y - rect->center.y + ptCenter->y;

	rect->bottom.x = rect->bottom.x - rect->center.x + ptCenter->x;
	rect->bottom.y = rect->bottom.y - rect->center.y + ptCenter->y;

	rect->center = *ptCenter;
}

INLINE void AssignRect( RECT *sourceRect, RECT *destRect)
{
	memcpy(destRect, sourceRect, sizeof(RECT));
}

void InitPatternSearchVectors( int searchPos, PQ_CFG* Config)
{
	int v;

	/* Build Search Vector */

	for(v=0; v<NREGMARK; v++)
	{
		if((Config->Patterns[searchPos].iPattern1X[v] < 0) &&
			(Config->Patterns[searchPos].iPattern1Y[v] < 0))
		{
			/* Orientation is 'Left to Right' */

			PQ_SearchVectors.searchVectorOffsetTB[v].x =
				abs(Config->Patterns[searchPos].iPattern1X[v]) +
				abs(Config->Patterns[searchPos].iPattern2X[v]);

			PQ_SearchVectors.searchVectorOffsetTB[v].y =
				abs(Config->Patterns[searchPos].iPattern1Y[v]) +
				abs(Config->Patterns[searchPos].iPattern2Y[v]);

			PQ_SearchVectors.searchVectorOffsetBT[v].x =
				(-PQ_SearchVectors.searchVectorOffsetTB[v].x);

			PQ_SearchVectors.searchVectorOffsetBT[v].y =
				(-PQ_SearchVectors.searchVectorOffsetTB[v].y);
		}
		else
		{
			/* Orientation is 'Right to Left' */

			PQ_SearchVectors.searchVectorOffsetBT[v].x =
				abs(Config->Patterns[searchPos].iPattern1X[v]) +
				abs(Config->Patterns[searchPos].iPattern2X[v]);

			PQ_SearchVectors.searchVectorOffsetTB[v].y =
				abs(Config->Patterns[searchPos].iPattern1Y[v]) +
				abs(Config->Patterns[searchPos].iPattern2Y[v]);

			PQ_SearchVectors.searchVectorOffsetBT[v].y =
				(-PQ_SearchVectors.searchVectorOffsetTB[v].y);

			PQ_SearchVectors.searchVectorOffsetTB[v].x =
				(-PQ_SearchVectors.searchVectorOffsetBT[v].x);
		}
#ifdef USE_SUBPIXELS
		PQ_SearchVectors.searchVectorOffsetTB[v].x *= (int) SCALE_FACTOR;
		PQ_SearchVectors.searchVectorOffsetTB[v].y *= (int) SCALE_FACTOR;
		PQ_SearchVectors.searchVectorOffsetBT[v].x *= (int) SCALE_FACTOR;
		PQ_SearchVectors.searchVectorOffsetBT[v].y *= (int) SCALE_FACTOR;
#endif
	}
}

void ScaleCandidateBlobs(CANDIDATE_BLOBS *CandidateBlobs)
{
	int bA, blobCount;

	blobCount = CandidateBlobs->blobCount;

	for(bA=0; bA<blobCount; bA++)
	{
		CandidateBlobs->blobRect[bA].center.x /= SCALE_FACTOR;
		CandidateBlobs->blobRect[bA].center.y /= SCALE_FACTOR;
	}
}

void CreateMinSearchBoundary( int boundaryTolerance)
{
	currTolerance = boundaryTolerance;

	MinSearchBoundary.center.x = 0;
	MinSearchBoundary.center.y = 0;

	MinSearchBoundary.top.x = 0 - boundaryTolerance;
	MinSearchBoundary.top.y = 0 - boundaryTolerance;

	MinSearchBoundary.bottom.x = 0 + boundaryTolerance;
	MinSearchBoundary.bottom.y = 0 + boundaryTolerance;
}


void CreatePatternSearchBoundaries( POINT *blobCenter, PQ_CFG* Config)
{
	int pT;

	for(pT=0; pT<NREGMARK; pT++)
	{
		if(IsPatternEnabled(pT, Config) && IsPatternMissing(pT, Config))
		{
			CenterRect(blobCenter, &MinSearchBoundary);

			AssignRect(&MinSearchBoundary, &PQ_PatternSearchBoundary.aBottomBoundary[pT]);
			AssignRect(&MinSearchBoundary, &PQ_PatternSearchBoundary.aTopBoundary[pT]);

			/* "TOP TO BOTTOM" SEARCH BOUNDARY */

			PQ_PatternSearchBoundary.aBottomValid[pT] =
				MoveRect(&SearchVectors->searchVectorOffsetTB[pT],
							&PQ_PatternSearchBoundary.aBottomBoundary[pT]);

			/* "BOTTOM TO TOP" SEARCH BOUNDARY */

			PQ_PatternSearchBoundary.aTopValid[pT] =
				MoveRect(&SearchVectors->searchVectorOffsetBT[pT],
							&PQ_PatternSearchBoundary.aTopBoundary[pT]);
		}
		else
		{
			PQ_PatternSearchBoundary.aTopValid[pT] = FALSE;
			PQ_PatternSearchBoundary.aBottomValid[pT] = FALSE;
		}
	}
}

INLINE BOOL IsComparisonDone( int thisBlobIdx, int partnerBlobIdx)
{
	if(PQ_ComparisonDoneMatrix.doneMatrix[thisBlobIdx][partnerBlobIdx] ||
		 PQ_ComparisonDoneMatrix.doneMatrix[partnerBlobIdx][thisBlobIdx])
			return TRUE;

	return FALSE;
}

INLINE void SetComparisonDone( int thisBlobIdx, int partnerBlobIdx)
{
		PQ_ComparisonDoneMatrix.doneMatrix[thisBlobIdx][partnerBlobIdx] = TRUE;
}

INLINE void InitComparisonDoneMatrix( void )
{
	memset((void*)&PQ_ComparisonDoneMatrix,0,MAX_NUM_CANDIDATE_BLOBS * MAX_NUM_CANDIDATE_BLOBS );
}

void InitPairingTable( void )
{
/*
	const unsigned int InitValue1 = (NO_BLOB << 8);
	const unsigned int InitValue2 = (ELEMENT_TYPE_NONE << 2);
	const unsigned int InitValue3 = 0x00;

	unsigned int InitValue = InitValue1 | InitValue2 | InitValue3;
*/
	unsigned int InitValue = 0xFF3C;
	int pTIdx;
	PAIRING_TABLE* curPairTab;

	for(pTIdx=0; pTIdx<MAX_NUM_CANDIDATE_BLOBS; pTIdx++)
	{
		curPairTab = &PQ_PairingTable[pTIdx];

		curPairTab->positiveMatchFound = FALSE;

		curPairTab->numLink = 0;
		curPairTab->numActiveLink = 0;

		memset(&(curPairTab->partnerLinks[0]), InitValue, MAX_NUM_CANDIDATE_BLOBS);
	}

}

void InitPatternMatchTable( PATTERN_MATCH_TABLE *PatternMatchTable)
{
	int pT;

	for(pT=0; pT<NREGMARK; pT++)
	{
		(PatternMatchTable + pT)->centerTopElement.x = SCREEN_OUT_OF_RANGE;
		(PatternMatchTable + pT)->centerTopElement.y = SCREEN_OUT_OF_RANGE;
		(PatternMatchTable + pT)->centerBottomElement.x = SCREEN_OUT_OF_RANGE;
		(PatternMatchTable + pT)->centerBottomElement.y = SCREEN_OUT_OF_RANGE;

		(PatternMatchTable + pT)->found = FALSE;

		(PatternMatchTable + pT)->matchCount = 0;
		(PatternMatchTable + pT)->unResolvedCount = 0;
		(PatternMatchTable + pT)->tolerance = 0;
	}
}


void BuildPotentialPairs( PQ_CFG* Config, CANDIDATE_BLOBS *CandidateBlobs,
				                    PATTERN_MATCH_TABLE *PatternMatchTable)
{
	int typeMatchNextSlot;

	int bA, bB, pT;

	int blobCount = CandidateBlobs->blobCount;

	for(bA=0; bA<blobCount; bA++)
	{
		if(CandidateBlobs->smallBlob[bA])
		{
			CreatePatternSearchBoundaries(&CandidateBlobs->blobRect[bA].center, Config);

			for(bB=0; bB<blobCount; bB++)
			{
				if((!IsComparisonDone(bA,bB)) && (bA != bB))
				{
					for(pT=0; pT<NREGMARK; pT++)
					{
						if(IsPatternEnabled(pT, Config) && IsPatternMissing(pT, Config))
						{
							if((PQ_PatternSearchBoundary.aBottomValid[pT] == TRUE) &&
								(IsRectInBound(&PQ_PatternSearchBoundary.aBottomBoundary[pT],
												&CandidateBlobs->blobRect[bB]) ||
								 IsPointInBound(&CandidateBlobs->blobRect[bB].center,
												&PQ_PatternSearchBoundary.aBottomBoundary[pT])))
							{
								SetComparisonDone(bA, bB);
								(PatternMatchTable + pT)->unResolvedCount++;

								/* Update Pairing Table For this blob */
								typeMatchNextSlot = PQ_PairingTable[bA].numActiveLink;

								PQ_PairingTable[bA].numActiveLink++;
								PQ_PairingTable[bA].numLink++;

								PARTNER_LINK_SetElementTypeMatched
									(&(PQ_PairingTable[bA].partnerLinks[typeMatchNextSlot]), pT);

								/*if(Config->referenceSearchType != SEARCH_INFER_TYPE_NONE)
								cprintf(Console, "sETM1:%d eT:%d", bA, pT);*/

								PARTNER_LINK_ClearIsTopElement
									(&(PQ_PairingTable[bA].partnerLinks[typeMatchNextSlot]));

								PARTNER_LINK_SetPartnerBlobIdMatched
									(&(PQ_PairingTable[bA].partnerLinks[typeMatchNextSlot]), bB);

								PARTNER_LINK_SetIsActive
									(&(PQ_PairingTable[bA].partnerLinks[typeMatchNextSlot]));


								/* Update Pairing Table For potential partner blob */
								typeMatchNextSlot = PQ_PairingTable[bB].numActiveLink;

								PQ_PairingTable[bB].numActiveLink++;
								PQ_PairingTable[bB].numLink++;

								PARTNER_LINK_SetElementTypeMatched
									(&(PQ_PairingTable[bB].partnerLinks[typeMatchNextSlot]), pT);

								/*if(Config->referenceSearchType != SEARCH_INFER_TYPE_NONE)
								cprintf(Console, "sETM2:%d eT:%d", bB, pT);*/

								PARTNER_LINK_SetIsTopElement
									(&(PQ_PairingTable[bB].partnerLinks[typeMatchNextSlot]));

								PARTNER_LINK_SetPartnerBlobIdMatched
									(&(PQ_PairingTable[bB].partnerLinks[typeMatchNextSlot]), bA);

								PARTNER_LINK_SetIsActive
									(&(PQ_PairingTable[bB].partnerLinks[typeMatchNextSlot]));

								/*if(Config->referenceSearchType != SEARCH_INFER_TYPE_NONE)
								cprintf(Console, "pTBOT %d bA %d, bB %d", pT, bA, bB);*/
							}

							if((PQ_PatternSearchBoundary.aTopValid[pT] == TRUE) &&
								(IsRectInBound(&PQ_PatternSearchBoundary.aTopBoundary[pT],
												&CandidateBlobs->blobRect[bB]) ||
								IsPointInBound(&CandidateBlobs->blobRect[bB].center,
												&PQ_PatternSearchBoundary.aTopBoundary[pT])))
							{
								SetComparisonDone(bA, bB);
								(PatternMatchTable + pT)->unResolvedCount++;

								/* Update Pairing Table For this blob */
								typeMatchNextSlot = PQ_PairingTable[bA].numActiveLink;

								PQ_PairingTable[bA].numActiveLink++;
								PQ_PairingTable[bA].numLink++;

								PARTNER_LINK_SetElementTypeMatched
									(&(PQ_PairingTable[bA].partnerLinks[typeMatchNextSlot]), pT);

								/*if(Config->referenceSearchType != SEARCH_INFER_TYPE_NONE)
								cprintf(Console, "sETM3:%d eT:%d", bA, pT);*/

								PARTNER_LINK_SetIsTopElement
									(&(PQ_PairingTable[bA].partnerLinks[typeMatchNextSlot]));

								PARTNER_LINK_SetPartnerBlobIdMatched
									(&(PQ_PairingTable[bA].partnerLinks[typeMatchNextSlot]), bB);

								PARTNER_LINK_SetIsActive
									(&(PQ_PairingTable[bA].partnerLinks[typeMatchNextSlot]));


								/* Update Pairing Table For potential partner blob */
								typeMatchNextSlot = PQ_PairingTable[bB].numActiveLink;

								PQ_PairingTable[bB].numActiveLink++;
								PQ_PairingTable[bB].numLink++;

								PARTNER_LINK_SetElementTypeMatched
									(&(PQ_PairingTable[bB].partnerLinks[typeMatchNextSlot]), pT);

								/*if(Config->referenceSearchType != SEARCH_INFER_TYPE_NONE)
								cprintf(Console, "sETM4:%d eT:%d", bB, pT);*/

								PARTNER_LINK_ClearIsTopElement
									(&(PQ_PairingTable[bB].partnerLinks[typeMatchNextSlot]));

								PARTNER_LINK_SetPartnerBlobIdMatched
									(&(PQ_PairingTable[bB].partnerLinks[typeMatchNextSlot]), bA);

								PARTNER_LINK_SetIsActive
									(&(PQ_PairingTable[bB].partnerLinks[typeMatchNextSlot]));

								/*if(Config->referenceSearchType != SEARCH_INFER_TYPE_NONE)
								cprintf(Console, "pTTOP %d bA %d, bB %d", pT, bA, bB);*/
							}
						}
					}
				}
			}
		}
	}
}


/* the ONE active Link */

#define		ERROR_LINK	-1

int	GetTheActiveLink( int pairingTableIdx)
{
	int linkIdx;

	int numLink = PQ_PairingTable[pairingTableIdx].numLink;

		for(linkIdx=0; linkIdx<numLink; linkIdx++)
		{
			if(PARTNER_LINK_IsActive
				(&(PQ_PairingTable[pairingTableIdx].partnerLinks[linkIdx])))
				return linkIdx;
		}

	return ERROR_LINK;
}

void GetPartnersElementCenter( BOOL searchForTop, POINT *centerRef,
                                    int elemTypeMatched, POINT *pointUpdated)
{
	if(searchForTop)
	{
		pointUpdated->x = centerRef->x +
							PQ_SearchVectors.searchVectorOffsetBT[elemTypeMatched].x;
		pointUpdated->y = centerRef->y +
							PQ_SearchVectors.searchVectorOffsetBT[elemTypeMatched].y;
	}
	else
	{
		pointUpdated->x = centerRef->x +
							PQ_SearchVectors.searchVectorOffsetTB[elemTypeMatched].x;
		pointUpdated->y = centerRef->y +
							PQ_SearchVectors.searchVectorOffsetTB[elemTypeMatched].y;
	}
}


void UpdatePatternMatchTable( int elemTypeMatched, int headElement, int partnerElement,
			CANDIDATE_BLOBS *CandidateBlobs, PATTERN_MATCH_TABLE *PatternMatchTable)
{
	(PatternMatchTable + elemTypeMatched)->found = TRUE;

	(PatternMatchTable + elemTypeMatched)->tolerance = currTolerance;

		/* Determine the correct Element Centers */

		if((CandidateBlobs->smallBlob[headElement] == TRUE) &&
			(CandidateBlobs->smallBlob[partnerElement] == TRUE))
		{
			/* Is top or bottom */

			if(PQ_PairingTable[headElement].positiveTopElement)
			{
				(PatternMatchTable + elemTypeMatched)->centerTopElement =
					CandidateBlobs->blobRect[headElement].center;

				(PatternMatchTable + elemTypeMatched)->centerBottomElement =
					CandidateBlobs->blobRect[partnerElement].center;
			}
			else
			{
				(PatternMatchTable + elemTypeMatched)->centerTopElement =
					CandidateBlobs->blobRect[partnerElement].center;

				(PatternMatchTable + elemTypeMatched)->centerBottomElement =
					CandidateBlobs->blobRect[headElement].center;
			}
		}
		else
		if(CandidateBlobs->smallBlob[headElement] == TRUE)
		{
			if(PQ_PairingTable[headElement].positiveTopElement)
			{
				(PatternMatchTable + elemTypeMatched)->centerTopElement =
					CandidateBlobs->blobRect[headElement].center;

				/* approximate the other's blob center */

				GetPartnersElementCenter
					(FALSE, &CandidateBlobs->blobRect[headElement].center,
						elemTypeMatched,
						&(PatternMatchTable + elemTypeMatched)->centerBottomElement);
			}
			else
			{
				(PatternMatchTable + elemTypeMatched)->centerBottomElement =
					CandidateBlobs->blobRect[headElement].center;

				GetPartnersElementCenter
					(TRUE, &CandidateBlobs->blobRect[headElement].center,
						elemTypeMatched,
						&(PatternMatchTable + elemTypeMatched)->centerTopElement);
			}
		}
}

void DeleteDuplicateElementMatch(int elemTypeToDelete, CANDIDATE_BLOBS *CandidateBlobs,
				                    PATTERN_MATCH_TABLE *PatternMatchTable)
{
	int		bA, bB, bC;
	int		elementTypeA, numLinkB, numLinkC, partnerBlobIdx;
	int		blobCount = CandidateBlobs->blobCount;

	PAIRING_TABLE* pCurBlob;
	PAIRING_TABLE* pPartBlob;
	PARTNER_LINK* pPartLink;

	/* Delete Other Element Duplicate Link Matches that consists of TWO COMPOUND Blobs */

	for(bA=0; bA<blobCount; bA++)
	{
		pCurBlob = &(PQ_PairingTable[bA]);

		if((pCurBlob->positiveMatchFound == FALSE) &&
			(pCurBlob->numLink > 1))
		{
			numLinkB = pCurBlob->numLink;

			for(bB=0; bB<numLinkB; bB++)
			{
				pPartLink = &(pCurBlob->partnerLinks[bB]);

				elementTypeA = PARTNER_LINK_GetElementTypeMatched(pPartLink);

				if(PARTNER_LINK_IsActive(pPartLink) && (elementTypeA == elemTypeToDelete))
				{
					partnerBlobIdx = PARTNER_LINK_GetPartnerBlobIdMatched(pPartLink);

					pPartBlob = &(PQ_PairingTable[partnerBlobIdx]);

					if((pPartBlob->positiveMatchFound == FALSE) &&
						(pPartBlob->numLink > 1))
					{
						numLinkC = pPartBlob->numLink;

						for(bC=0; bC<numLinkC; bC++)
						{
							if(bA == PARTNER_LINK_GetPartnerBlobIdMatched
										(&(pPartBlob->partnerLinks[bC])))
							{
								PARTNER_LINK_ClearIsActive(pPartLink);

								pCurBlob->numActiveLink--;

								PARTNER_LINK_ClearIsActive
									(&(pPartBlob->partnerLinks[bC]));

								pPartBlob->numActiveLink--;

								(PatternMatchTable + elemTypeToDelete)->unResolvedCount--;
							}
						}
					}
				}
			}
		}
	}
}

void UpdatePairLinks ( int elemTypeToDelete, int headBlobIdx, int partnerBlobIdx,
			CANDIDATE_BLOBS *CandidateBlobs, PATTERN_MATCH_TABLE *PatternMatchTable)
{
	int bB;

	int numLink;

		/* Delete Head Element to Partner Link */

			PARTNER_LINK_ClearIsActive
				(&(PQ_PairingTable[headBlobIdx].partnerLinks[partnerBlobIdx]));

			PQ_PairingTable[headBlobIdx].numActiveLink--;

			/* Search Partner for Head Link */

			numLink = PQ_PairingTable[partnerBlobIdx].numLink;

			for(bB=0; bB<numLink; bB++)
			{
				if(PARTNER_LINK_GetElementTypeMatched
					(&(PQ_PairingTable[partnerBlobIdx].partnerLinks[bB])) == elemTypeToDelete)
				{
					PARTNER_LINK_ClearIsActive
						(&(PQ_PairingTable[partnerBlobIdx].partnerLinks[bB]));

					PQ_PairingTable[partnerBlobIdx].numActiveLink--;
					break;
				}
			}

		(PatternMatchTable + elemTypeToDelete)->unResolvedCount--;

		if((PatternMatchTable + elemTypeToDelete)->unResolvedCount > 0)
			DeleteDuplicateElementMatch(elemTypeToDelete, CandidateBlobs, PatternMatchTable);
}

/* Search for the Target Pair match, based on the potential pairs */

void SearchForTargetPairs( CANDIDATE_BLOBS *CandidateBlobs,
                                    PATTERN_MATCH_TABLE *PatternMatchTable)
{
	int		currActiveLinkA;
	int		elementTypeA, elementTypeB;
	int		bA, bB;

	int		partnerBlobIdx, numLink;

	BOOL	searchDone;

	int		blobCount = CandidateBlobs->blobCount;

	do
	{
		searchDone = TRUE;

		for(bA=0; bA<blobCount; bA++)
		{
			/* Search for ONE Head Element and its Partner Element */

			/* Find HEAD */

			if((PQ_PairingTable[bA].positiveMatchFound == FALSE) &&
				(PQ_PairingTable[bA].numActiveLink == 1) &&
				(CandidateBlobs->smallBlob[bA] == TRUE))
			{
				PQ_PairingTable[bA].positiveMatchFound = TRUE;

				currActiveLinkA = GetTheActiveLink(bA);

				elementTypeA = PARTNER_LINK_GetElementTypeMatched
								(&(PQ_PairingTable[bA].partnerLinks[currActiveLinkA]));

				/* cprintf(Console, "PMF:%d  eTA:%d cAL:%d", bA, elementTypeA, currActiveLinkA); */

				if(PARTNER_LINK_IsTopElement
					(&(PQ_PairingTable[bA].partnerLinks[currActiveLinkA])))
					PQ_PairingTable[bA].positiveTopElement = FALSE;
				else
					PQ_PairingTable[bA].positiveTopElement = TRUE;

				partnerBlobIdx =
					PARTNER_LINK_GetPartnerBlobIdMatched
						(&(PQ_PairingTable[bA].partnerLinks[currActiveLinkA]));

				if(PQ_PairingTable[partnerBlobIdx].numLink == 1)
				{
					(PatternMatchTable + elementTypeA)->matchCount++;

					PQ_PairingTable[partnerBlobIdx].positiveMatchFound = TRUE;

					if(PQ_PairingTable[bA].positiveTopElement == TRUE)
						PQ_PairingTable[partnerBlobIdx].positiveTopElement = FALSE;
					else
						PQ_PairingTable[partnerBlobIdx].positiveTopElement = TRUE;
				}
				else
				{
					numLink = PQ_PairingTable[partnerBlobIdx].numLink;

					for(bB=0; bB<numLink; bB++)
					{
						elementTypeB = PARTNER_LINK_GetElementTypeMatched
										(&(PQ_PairingTable[partnerBlobIdx].partnerLinks[bB]));

						if(elementTypeA == elementTypeB)
						{
							(PatternMatchTable + elementTypeA)->matchCount++;

							if(PQ_PairingTable[partnerBlobIdx].numActiveLink == 1)
							{
								PQ_PairingTable[partnerBlobIdx].positiveMatchFound = TRUE;

								if(PQ_PairingTable[bA].positiveTopElement == TRUE)
									PQ_PairingTable[partnerBlobIdx].positiveTopElement = FALSE;
								else
									PQ_PairingTable[partnerBlobIdx].positiveTopElement = TRUE;

								break;
							}
						}
					}
				}

				UpdatePatternMatchTable
					(elementTypeA, bA, partnerBlobIdx, CandidateBlobs, PatternMatchTable);

				UpdatePairLinks
					(elementTypeA, bA, partnerBlobIdx, CandidateBlobs, PatternMatchTable);

				searchDone = FALSE;

				break;
			}
		}
	}while(!searchDone);
}

void	StoreLargeBlobArray(CANDIDATE_BLOBS *CandidateBlobs)
{
	int	b, l;
	int	blobCount = CandidateBlobs->blobCount;

	for(b=0, l=0; b<blobCount; b++)
		if(CandidateBlobs->smallBlob[b] == FALSE)
		{
			largeBlobRect[l] = CandidateBlobs->blobRect[b];
			l++;
		}
}

bool	BuildOneofFourCorners(int mainBlobId, int cornerIdx, CANDIDATE_BLOBS *CandidateBlobs, PQ_CFG* Config)
{
	int tX, tY, bX, bY, topX, topY, botX, botY;
	int meanGrayvalue;
	image thisBlob;
	bool cornerValid = TRUE;
	int nextValidBlob;

	/* Clean up candidate blob array */

	nextValidBlob = CandidateBlobs->blobCount = 0;

	/* Create small blobs out of 4 corners */

		tX = largeBlobRect[mainBlobId].top.x;
		tY = largeBlobRect[mainBlobId].top.y;
		bX = largeBlobRect[mainBlobId].bottom.x;
		bY = largeBlobRect[mainBlobId].bottom.y;

#ifdef USE_SUBPIXELS
		topX = tX /(int) SCALE_FACTOR;
		topY = tY /(int) SCALE_FACTOR;
		botX = bX / (int) SCALE_FACTOR;
		botY = bY / (int) SCALE_FACTOR;
#endif

		switch(cornerIdx)
		{
		case 0:/* Top Left Corner */
			{
				CandidateBlobs->blobRect[nextValidBlob].center.x	=
				CandidateBlobs->blobRect[nextValidBlob].top.x		=
				CandidateBlobs->blobRect[nextValidBlob].bottom.x	= tX;
				CandidateBlobs->blobRect[nextValidBlob].center.y	=
				CandidateBlobs->blobRect[nextValidBlob].top.y		=
				CandidateBlobs->blobRect[nextValidBlob].bottom.y	= tY;

				/* cprintf(Console, "lB %d  x:%d y:%d", nextValidBlob, tX, tY); */

				/* create image structure based on blob bounding box of size CORNER_SIZE at the corner */
#ifdef USE_GRAYVALUE_FOR_PATTERN_MATCHING
				thisBlob = ImageCreate(topX, topY, CORNER_SIZE, CORNER_SIZE, IMG_DEFAULT);
				/*cprintf(Console, "Top left corner");  */
				/*SendDbgMsg(DBG_ERRORS, "Top left corner");*/
#endif
			}

			break;

		case 1:/* Bottom Right Corner */
			{
				CandidateBlobs->blobRect[nextValidBlob].center.x	=
				CandidateBlobs->blobRect[nextValidBlob].top.x		=
				CandidateBlobs->blobRect[nextValidBlob].bottom.x	= bX;
				CandidateBlobs->blobRect[nextValidBlob].center.y	=
				CandidateBlobs->blobRect[nextValidBlob].top.y		=
				CandidateBlobs->blobRect[nextValidBlob].bottom.y	= bY;
				/*cprintf(Console, "lB %d  x:%d y:%d", nextValidBlob, bX, bY); */

#ifdef USE_GRAYVALUE_FOR_PATTERN_MATCHING
				thisBlob = ImageCreate(botX-CORNER_SIZE, botY-CORNER_SIZE, CORNER_SIZE, CORNER_SIZE, IMG_DEFAULT);
				/*cprintf(Console, "Bottom right corner");  */
				/*SendDbgMsg(DBG_ERRORS, "Bottom right corner");*/
#endif
			}

			break;

		case 2:/* Top Right */
			{
				CandidateBlobs->blobRect[nextValidBlob].center.x	=
				CandidateBlobs->blobRect[nextValidBlob].top.x		=
				CandidateBlobs->blobRect[nextValidBlob].bottom.x	= bX;
				CandidateBlobs->blobRect[nextValidBlob].center.y	=
				CandidateBlobs->blobRect[nextValidBlob].top.y		=
				CandidateBlobs->blobRect[nextValidBlob].bottom.y	= tY;
				/*cprintf(Console, "lB %d  x:%d y:%d", nextValidBlob, bX, tY); */

#ifdef USE_GRAYVALUE_FOR_PATTERN_MATCHING
				thisBlob = ImageCreate(botX-CORNER_SIZE, topY, CORNER_SIZE, CORNER_SIZE, IMG_DEFAULT);
				/*cprintf(Console, "Top right corner");  */
				/*SendDbgMsg(DBG_ERRORS, "Top right corner");*/
#endif
			}

			break;

		case 3:/* Bottom Left */
			{
				CandidateBlobs->blobRect[nextValidBlob].center.x	=
				CandidateBlobs->blobRect[nextValidBlob].top.x		=
				CandidateBlobs->blobRect[nextValidBlob].bottom.x	= tX;
				CandidateBlobs->blobRect[nextValidBlob].center.y	=
				CandidateBlobs->blobRect[nextValidBlob].top.y		=
				CandidateBlobs->blobRect[nextValidBlob].bottom.y	= bY;
				/*cprintf(Console, "lB %d  x:%d y:%d", nextValidBlob, tX, bY); */

#ifdef USE_GRAYVALUE_FOR_PATTERN_MATCHING
				thisBlob = ImageCreate(topX, botY-CORNER_SIZE, CORNER_SIZE, CORNER_SIZE, IMG_DEFAULT);
				/*cprintf(Console, "Bottom left corner");  */
				/*SendDbgMsg(DBG_ERRORS, "Bottom left corner");*/
#endif
			}

			break;

		default:
			{
			}

			break;
		}


		CandidateBlobs->smallBlob[nextValidBlob] = TRUE;
		CandidateBlobs->blobCount ++;
		

#ifdef USE_GRAYVALUE_FOR_PATTERN_MATCHING
				/* correct start address by ROI offset */
				thisBlob.st += Config->RoI.st;

				/* make roi word aligned */
				AlignRoi(&thisBlob, TRUE);

				/* calculate the mean gray value of the blob */
				meanGrayvalue = calculateMeanGrayvalue(&thisBlob);
				/*cprintf(Console, "average gray value :%d", meanGrayvalue);*/
				/*SendDbgMsg(DBG_ERRORS, "average gray value :%d", meanGrayvalue);*/

				if (meanGrayvalue > Config->AdjustedThreshold) 
					cornerValid = FALSE;

				/*SendDbgMsg(DBG_ERRORS, "is this corner valid??? : %d", cornerValid);*/

				return cornerValid;
#endif		
				
}


void DoForcedSearchForRefMarks( PQ_CFG* Config, CANDIDATE_BLOBS *CandidateBlobs,
                                    PATTERN_MATCH_TABLE *PatternMatchTable)
{
	int a, b, c;
	int nextValidBlob;
	int lBlobCount = Config->largeBlobCount;
	RECT tempBlob;
	bool cornerValid;

	StoreLargeBlobArray(CandidateBlobs);

	SelectReferenceMarkForSearch(Config);

	for(a=0; a<lBlobCount; a++)
	{
		/*SendDbgMsg(DBG_ERRORS, "Large Blob number : %d", a);*/

		for(c=0; c<CORNER_COUNT; c++)
		{
			/*SendDbgMsg(DBG_ERRORS, "         Corner number : %d", c);*/

			cornerValid = BuildOneofFourCorners(a, c, CandidateBlobs, Config);

			nextValidBlob = CandidateBlobs->blobCount;

			/*
			if (!cornerValid && Config->bSearchMode == SEARCH_TRACK)
				continue;
				*/

			/* Fill up Candidate blob array with the rest of the Large Blobs */

			for(b=0; b<lBlobCount; b++)
			{
				if(b != a)
				{
					/*-- Enlarge the blob size by the boudary tolerance --*/
					tempBlob = largeBlobRect[b];

					tempBlob.top.x -= 2 * INIT_BOUNDARY_TOLERANCE;
					tempBlob.top.y -= 2 * INIT_BOUNDARY_TOLERANCE;

					tempBlob.bottom.x += 2 * INIT_BOUNDARY_TOLERANCE;
					tempBlob.bottom.y += 2 * INIT_BOUNDARY_TOLERANCE;
					/*--------------------------------------------------*/

					CandidateBlobs->blobRect[nextValidBlob] = tempBlob;
					CandidateBlobs->smallBlob[nextValidBlob] = FALSE;

					CandidateBlobs->blobCount++;
					nextValidBlob++;
				}
			}

			DoPatternRecogition(Config, CandidateBlobs, PatternMatchTable);

			StoreMarksFound(Config);

			CalculatePairCenters(Config, PatternMatchTable);

			if(Config->pReferenceFound)
			{
				cprintf(Console, "REF INFERRED");
				break;
			}
		}

		if(Config->pReferenceFound)
			break;
	}
}

void DoForcedSearchForOtherMarks( PQ_CFG* Config, CANDIDATE_BLOBS *CandidateBlobs,
                                    PATTERN_MATCH_TABLE *PatternMatchTable)
{
	int i, a, b, c;
	int nextValidBlob;
	int lBlobCount = Config->largeBlobCount;
	RECT tempBlob;

	StoreLargeBlobArray(CandidateBlobs);

	for(a=0; a<lBlobCount; a++)
	{
		for(c=0; c<CORNER_COUNT; c++)
		{
			nextValidBlob = CandidateBlobs->blobCount;

			BuildOneofFourCorners(a, c, CandidateBlobs, Config);

			/* Fill up Candidate blob array with the rest of the Large Blobs */

			for(b=0; b<lBlobCount; b++)
			{
				if(b != a)
				{
					for(i = 0; i < NREGMARK; i++)
						if (Config->regFoundResultBackup[i] == FALSE)
							Config->bMarkSearchSelect[i] = TRUE;
						else
							Config->bMarkSearchSelect[i] = FALSE;

					/*-- Enlarge the blob size by the boudary tolerance --*/
					tempBlob = largeBlobRect[b];

					tempBlob.top.x -= 2 * INIT_BOUNDARY_TOLERANCE;
					tempBlob.top.y -= 2 * INIT_BOUNDARY_TOLERANCE;

					tempBlob.bottom.x += 2 * INIT_BOUNDARY_TOLERANCE;
					tempBlob.bottom.y += 2 * INIT_BOUNDARY_TOLERANCE;
					/*--------------------------------------------------*/

					CandidateBlobs->blobRect[nextValidBlob] = tempBlob;
					CandidateBlobs->smallBlob[nextValidBlob] = FALSE;

					CandidateBlobs->blobCount++;
					nextValidBlob++;

					DoPatternRecogition(Config, CandidateBlobs, PatternMatchTable);

					StoreMarksFound(Config);

					CalculatePairCenters(Config, PatternMatchTable);

					if(Config->pMarkSearchAllFound)
					{
						cprintf(Console, "OTHER MARK INFERRED");
						break;
					}
				}
			}

			if(Config->pMarkSearchAllFound)
				break;
		}

		if(Config->pMarkSearchAllFound)
			break;
	}
}


void DoPatternRecogition( PQ_CFG* Config, CANDIDATE_BLOBS *CandidateBlobs,
				                    PATTERN_MATCH_TABLE *PatternMatchTable)
{
	InitComparisonDoneMatrix();
	InitPairingTable();
	InitPatternMatchTable(PatternMatchTable);
	BuildPotentialPairs(Config, CandidateBlobs, PatternMatchTable);
	SearchForTargetPairs(CandidateBlobs, PatternMatchTable);
}

/* PATTERN RECOGNITION END */



/**FDOC*************************************************************************
 *
 *  Function:       main
 *
 *  Author:         Mark Colvin             Date:  June 12, 2001
 *
 *  Description:	Entry point for the PQ pattern match module
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

int MAIN(Pattern)(int dummy, int argc, PQ_CFG* Config, CANDIDATE_BLOBS *CandidateBlobs)
{
    /* declare and initialize local variables */
	PATTERN_MATCH_TABLE	*PatternMatchTable;
#ifdef WITH_TIMING
	time_t elapsed = time(NULL);
#endif
	int err = ERROR_NONE;

	char* BootMsg  = "PQ-pattern [build: "__BUILD__"]  "__DATE__"  "__TIME__;

	/* show info message on screen */
	SendDbgMsg(DBG_INFO, BootMsg);

	GMI_ASSERT(Config);

	/* make sure global variables are not overwritten */
	UseGlobals();

	PatternMatchTable =
		(PATTERN_MATCH_TABLE *) vcmalloc(sizeof(PATTERN_MATCH_TABLE) * NREGMARK);

	if( !PatternMatchTable )
		SendDbgMsg(DBG_ERRORS,"PM TABLE FAILED MALLOC");

	SearchVectors = &PQ_SearchVectors;
	PatternSearchBoundary = &PQ_PatternSearchBoundary;
	ComparisonDoneMatrix = &PQ_ComparisonDoneMatrix;
	/*PairingTable = &PQ_PairingTable[0];*/

	InitPatternSearchVectors(Config->bTargetPatternID, Config);

	CreateMinSearchBoundary(INIT_BOUNDARY_TOLERANCE);

#ifndef USE_SUBPIXELS
	ScaleCandidateBlobs(CandidateBlobs);
#endif

	DoPatternRecogition(Config, CandidateBlobs, PatternMatchTable);

	CalculatePairCenters(Config, PatternMatchTable);

	if (!Config->pReferenceFound && (Config->largeBlobCount >= 2))
	{
		Config->referenceSearchType = SEARCH_INFER_TYPE_3;
		DoForcedSearchForRefMarks(Config, CandidateBlobs, PatternMatchTable);

		/* if reference marks are found, while other marks are not found, force search other marks */
		if (Config->pReferenceFound && (!Config->pMarkSearchAllFound) && (Config->largeBlobCount >= 3))
			DoForcedSearchForOtherMarks(Config, CandidateBlobs, PatternMatchTable);
	}
	else 
		if (Config->pReferenceFound && (!Config->pMarkSearchAllFound) && (Config->largeBlobCount >= 2))
			DoForcedSearchForOtherMarks(Config, CandidateBlobs, PatternMatchTable);

	if(Config->bSearchMode == SEARCH_TRACK)
		DetectInferences(Config, CandidateBlobs);

	if( PatternMatchTable )
		vcfree( PatternMatchTable );

#ifdef WITH_TIMING
	cprintf(Console, "     pattern net: %ld", time(NULL) - elapsed);
#endif
	return err;
}

void CalculatePairCenters(PQ_CFG* Config, PATTERN_MATCH_TABLE *PatternMatchTable)
{
	bool Hunting = (Config->bSearchMode == SEARCH_HUNT);

	long topCenter, center, botCenter, Tx, Ty, Bx, By;
	int	 minX, minY, maxX, maxY;

	int unResolvedCount, pT;

	REG_POS	*strcRegPos = Config->strcRegPos;
	REG_ELEM_INF_DATA *strcElemInfData = Config->strcElemInfData;

	for(pT=0; pT<NREGMARK; pT++)
	{
		unResolvedCount = (PatternMatchTable + pT)->unResolvedCount;

		if(IsPatternEnabled(pT, Config))
		{
			if(IsPatternFound(pT, Config)) /* Previously Found */
			{
				Config->pFound++;

				if(pT == Config->bReferenceMarkSelection)
						Config->pReferenceFound = TRUE;

				/* cprintf(Console, "PREV FOUND %d", pT+1); */
			}
			else
			if((PatternMatchTable + pT)->found)
			{
				if(unResolvedCount >= 1)
				{
					/* Found but not resolved */
					strcRegPos[pT].nMarkStatus  = MARK_FOUND_BUT_UNRESOLVED;
				}
				else
				{
				    /* Pattern is Found */

				    /* Lateral Offset Calculation */

					Tx = topCenter = (PatternMatchTable + pT)->centerTopElement.x;
					Bx = botCenter = (PatternMatchTable + pT)->centerBottomElement.x;

				    if(topCenter < botCenter)
					{
						center = (botCenter - topCenter) >> 1;
						strcRegPos[pT].nLateral = (topCenter + center);
					}
					else
					{
						center = (topCenter - botCenter) >> 1;
						strcRegPos[pT].nLateral = (botCenter + center);
					}

					/* Circum Offset Calculation */

					Ty = topCenter = (PatternMatchTable + pT)->centerTopElement.y;
					By = botCenter = (PatternMatchTable + pT)->centerBottomElement.y;

				    if(topCenter < botCenter)
					{
						center = (botCenter - topCenter) >> 1;
						strcRegPos[pT].nCircum = (topCenter + center);
					}
					else
					{
						center = (topCenter - botCenter) >> 1;
						strcRegPos[pT].nCircum = (botCenter + center);
					}

					/* Important Hunter uses the center of the FOV
					   as the point of origin 0,0 */
					if(Hunting)
					{
#ifdef SLOW_CODE
						int roiOriginX = ScrGetX((Config->RoI).st);
						int roiOriginY = ScrGetY((Config->RoI).st);
#else
						long ByteAdrr = (Config->RoI).st - (Config->FoV).st;
						long Pitch    = (long) ScrGetPitch;

						int roiOriginX = (int) (ByteAdrr % Pitch);
						int roiOriginY = (int) (ByteAdrr / Pitch);
#endif
#ifdef USE_SUBPIXELS
						strcRegPos[pT].nCircum  += (roiOriginY - IMAGE_CENTERY) * SCALE_FACTOR;
						strcRegPos[pT].nLateral += (roiOriginX - IMAGE_CENTERX) * SCALE_FACTOR;
#else
						strcRegPos[pT].nCircum  += (roiOriginY - IMAGE_CENTERY);
						strcRegPos[pT].nLateral += (roiOriginX - IMAGE_CENTERX);
#endif
					}
#ifndef USE_SUBPIXELS
					strcRegPos[pT].nLateral *= SCALE_FACTOR;
					strcRegPos[pT].nCircum  *= SCALE_FACTOR;
#endif
					strcRegPos[pT].nMarkStatus  = MARK_FOUND;

					/* Remember location of the elements */

					strcElemInfData[pT].topElem.x = Tx;
					strcElemInfData[pT].topElem.y = Ty;
					strcElemInfData[pT].bottomElem.x = Bx;
					strcElemInfData[pT].bottomElem.y = By;

					minX = min(Tx, Bx);
					maxX = max(Tx, Bx);
					minY = min(Ty, By);
					maxY = max(Ty, By);

					if(Config->FirstPatternIsFound)
					{
						Config->NarrowRoIminX = minX;
						Config->NarrowRoIminY = minY;
						Config->NarrowRoImaxX = maxX;
						Config->NarrowRoImaxY = maxY;

						Config->FirstPatternIsFound = FALSE;

						/*
						cprintf(Console, "FNROI MinX:%d Y:%d MaxX:%d Y:%d",
											Config->NarrowRoIminX,
											Config->NarrowRoIminY,
											Config->NarrowRoImaxX,
											Config->NarrowRoImaxY);
											*/
					}
					else
					{
						/*
						cprintf(Console, "NEXTA MinX:%d Y:%d MaxX:%d Y:%d",
											Config->NarrowRoIminX,
											Config->NarrowRoIminY,
											Config->NarrowRoImaxX,
											Config->NarrowRoImaxY);
											*/

						if(minX < Config->NarrowRoIminX)	Config->NarrowRoIminX = minX;
						if(minY < Config->NarrowRoIminY)	Config->NarrowRoIminY = minY;
						if(maxX > Config->NarrowRoImaxX)	Config->NarrowRoImaxX = maxX;
						if(maxY > Config->NarrowRoImaxY)	Config->NarrowRoImaxY = maxY;

						/*
						cprintf(Console, "NEXTB MinX:%d Y:%d MaxX:%d Y:%d",
											Config->NarrowRoIminX,
											Config->NarrowRoIminY,
											Config->NarrowRoImaxX,
											Config->NarrowRoImaxY);
											*/
					}

					/* CCU NEEDS TO CHANGE
					if(Config->referenceSearchType == SEARCH_INFER_TYPE_NONE)
					{
						strcRegPos[pT].nMarkStatus  = MARK_FOUND;
					}
					else
					if(Config->referenceSearchType == SEARCH_INFER_TYPE_3)
					{
						strcRegPos[pT].nMarkStatus = MARK_FOUND_INFER_TYPE_3;
					}
					*/


					if(pT == Config->bReferenceMarkSelection)
					{
						Config->pReferenceFound = TRUE;

						/* SendDbgMsg(DBG_ERRORS, "REF Txy %ld %ld Bxy %ld %ld", Tx, Ty, Bx, By); */
					}

                    Config->pFound++;
			    }
            }
			else
			{
				/* Missing */
				strcRegPos[pT].nMarkStatus  = MARK_MISSING;

				/* cprintf(Console, "MISSIING"); */
			}

			if((PatternMatchTable + pT)->matchCount > 1)
			{
				/* Has duplicates */

				strcRegPos[pT].nMarkStatus  = MARK_FOUND_WITH_DUPLICATES;

				/* cprintf(Console, "HAS DUPLICATES"); */
			}
		}
	}

	if(Config->pFound == Config->bMarkSearchSelectCount)
		Config->pMarkSearchAllFound = TRUE;
	else
		Config->pMarkSearchAllFound = FALSE;
}

void	DetectInferences(PQ_CFG* Config, CANDIDATE_BLOBS *CandidateBlobs)
{
	BOOL	topInferred, bottomInferred;

	int		pT, bC;

	BYTE	tinfCount, binfCount;
	BYTE	tElemId, bElemId;

	REG_POS	*strcRegPos = Config->strcRegPos;
	REG_ELEM_INF_DATA *strcElemInfData = Config->strcElemInfData;

	int blobCount = CandidateBlobs->blobCount;

	for(pT=0; pT<NREGMARK; pT++)
	{
		if(strcRegPos[pT].nMarkStatus == MARK_FOUND)
		{
			for(bC=0; bC<blobCount; bC++)
			{
				if(IsPointInBound
					(&strcElemInfData[pT].topElem, &CandidateBlobs->blobRect[bC]))
				{
					CandidateBlobs->inferenceCount[bC]++;
					strcElemInfData[pT].topElemId = bC;

					/* cprintf(Console, "P:%d ID:%d TOP INC INFER BC:%d",
								pT+1, bC, blobCount); */
				}
			}

			for(bC=0; bC<blobCount; bC++)
			{
				if(IsPointInBound
					(&strcElemInfData[pT].bottomElem, &CandidateBlobs->blobRect[bC]))
				{
					CandidateBlobs->inferenceCount[bC]++;
					strcElemInfData[pT].bottomElemId = bC;

					/* cprintf(Console, "P:%d ID:%d BOT INC INFER BC:%d",
								pT+1, bC, blobCount); */
				}
			}
		}
	}

	for(pT=0; pT<NREGMARK; pT++)
	{
		if(strcRegPos[pT].nMarkStatus == MARK_FOUND)
		{
			tElemId = strcElemInfData[pT].topElemId;
			bElemId = strcElemInfData[pT].bottomElemId;

			tinfCount = CandidateBlobs->inferenceCount[tElemId];
			binfCount = CandidateBlobs->inferenceCount[bElemId];

			topInferred = (tinfCount > 1) ? TRUE : FALSE;
			bottomInferred = (binfCount > 1) ? TRUE : FALSE;

			if(topInferred || bottomInferred)
			{
				if(topInferred && bottomInferred)
				{
					strcElemInfData[pT].nMarkStatusModifier =
						MARK_FOUND_WITH_TWO_INFERENCES;
				}
				else
				{
					strcElemInfData[pT].nMarkStatusModifier =
						MARK_FOUND_WITH_ONE_INFERENCE;

					/*
					if(topInferred)
						cprintf(Console, "P:%d TOP INFERRED", pT+1);
					if(bottomInferred)
						cprintf(Console, "P:%d BOTTOM INFERRED", pT+1);
					*/
				}
			}
		}
	}
}

/*______________________________________________________________________________

		EOF - pattern.c
  ______________________________________________________________________________
*/


