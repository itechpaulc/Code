/**SDOC*************************************************************************
 *
 *  finetune.c - finetune threshold
 *
 *  Author: Mark Colvin			July 12, 2001
 *
 * $Header:   K:/Projects/PQCamera/PQCamera/Finetune/finetune.c_v   1.5   May 02 2003 10:30:22   APeng  $
 *
 * $Log:   K:/Projects/PQCamera/PQCamera/Finetune/finetune.c_v  $
 * 
 *    Rev 1.5   May 02 2003 10:30:22   APeng
 *  
 *
 *    Rev 1.4   Dec 30 2002 11:38:42   APeng
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
 *    Rev 1.3   Dec 13 2002 11:32:32   APeng
 * Added function to handle stripes and smudges around the registration marks
 *
 *
 *
 *    Rev 1.2   Dec 10 2002 15:59:04   APeng
 * some work has been done to the reduced
 * ROI
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


#include <string.h>	/* strlen */
#include <stdio.h>
#include <stdlib.h>
#include <vcdefs.h>
#include <bool.h>
#include <vcrt.h>
#include <vclib.h>
#include <cam0.h>
#include <macros.h>


#include "config.h"
#include "command.h"
#include "global.h"
#include "error.h"
#include "image.h"
#include "pqlib.h"
#include "build.h"

#include "bytes.h"

#include "finetune.h"

GMI_ASSERTFILE (__FILE__);



/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

#define	THRESHOLD_SAMPLE_COUNT			8

#define	SETTLE_SAMPLE_COUNT				16


/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

/*===========================  F U N C T I O N S  ============================*/




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

int MAIN(FineTune)(int dummy, int argc, PQ_CFG* Config)
{
    /* declare and initialize local variables */
	int err = ERROR_NONE;

	char* BootMsg  = "PQ-finetune [build: "__BUILD__"]  "__DATE__"  "__TIME__;

	/* show info message on screen */
	SendDbgMsg(DBG_INFO, BootMsg);

	GMI_ASSERT(Config);

	/* make sure global variables are not overwritten */
	UseGlobals();

	FineTuneThreshold(Config);

	return err;
}

void	FineTuneThreshold(PQ_CFG* Config)
{
		if(Config->thresholdAdjustState == THRESHOLD_ADJUST_SEARCH_BEGIN)
		{
			Config->patFoundSampleCount = 0;
			Config->patFoundLowerThreshold = 0;
			Config->patFoundCenterThreshold = 0;
			Config->patFoundHigherThreshold = 0;

			Config->centerThreshold = Config->Threshold;
			Config->lowerThreshold = Config->Threshold - Config->TrackerAdjustSteps;
			Config->higherThreshold = Config->Threshold + Config->TrackerAdjustSteps;

			Config->pcf1 = 1;
			Config->pcf2 = 1;
			Config->Threshold = Config->higherThreshold;

			Config->patFoundMax =
				Config->bMarkSearchSelectCount * THRESHOLD_SAMPLE_COUNT;

			Config->thresholdAdjustState = THRESHOLD_ADJUST_SEARCH_UP;
		}
		else
		if(Config->thresholdAdjustState == THRESHOLD_ADJUST_SEARCH_UP)
		{
				cprintf(Console, "ADJUST UP %d", Config->patFoundHigherThreshold);

				if(Config->patFoundSampleCount == THRESHOLD_SAMPLE_COUNT)
				{
					Config->patFoundSampleCount = 0;

					Config->pcf1 = 1;
					Config->pcf2 = 1;
					Config->Threshold = Config->centerThreshold;

					if(Config->patFoundHigherThreshold == Config->bMarkSearchSelectCount)
					{
						cprintf(Console, "SET HIGHER THRESHOLD NOW");
						Config->Threshold = Config->higherThreshold;

						Config->thresholdAdjustState = THRESHOLD_ADJUST_SETTLE;
					}
					else
					{
						Config->thresholdAdjustState = THRESHOLD_ADJUST_SEARCH_CENTER;
					}
				}
				else
				{
					Config->patFoundSampleCount++;
					Config->patFoundHigherThreshold += Config->pFound;
				}
		}
		else
		if(Config->thresholdAdjustState == THRESHOLD_ADJUST_SEARCH_CENTER)
		{
				cprintf(Console, "ADJUST CENTER %d", Config->patFoundCenterThreshold);

				if(Config->patFoundSampleCount == THRESHOLD_SAMPLE_COUNT)
				{
					Config->patFoundSampleCount = 0;

					Config->pcf1 = 1;
					Config->pcf2 = 1;
					Config->Threshold = Config->lowerThreshold;

					if(Config->patFoundCenterThreshold == Config->bMarkSearchSelectCount)
					{
						cprintf(Console, "SET CENTER THRESHOLD NOW");
						Config->Threshold = Config->centerThreshold;

						Config->thresholdAdjustState = THRESHOLD_ADJUST_SETTLE;
					}
					else
					{
						Config->thresholdAdjustState = THRESHOLD_ADJUST_SEARCH_DOWN;
					}
				}
				else
				{
					Config->patFoundSampleCount++;
					Config->patFoundCenterThreshold += Config->pFound;
				}
		}
		else
		if(Config->thresholdAdjustState == THRESHOLD_ADJUST_SEARCH_DOWN)
		{
				cprintf(Console, "ADJUST DOWN %d", Config->patFoundLowerThreshold);

				if(Config->patFoundSampleCount == THRESHOLD_SAMPLE_COUNT)
				{
					/* Validate that there is "Found" data recorded */

					if(Config->patFoundLowerThreshold ||
						Config->patFoundCenterThreshold ||
						Config->patFoundHigherThreshold)

					if(Config->patFoundHigherThreshold >= Config->patFoundCenterThreshold)
					{
						if(Config->patFoundHigherThreshold >= Config->patFoundLowerThreshold)
						{
							cprintf(Console, "SET HIGHER THRESHOLD");
							Config->Threshold = Config->higherThreshold;
						}
						else
						{
							cprintf(Console, "SET CENTER THRESHOLD");
							Config->Threshold = Config->centerThreshold;
						}
					}
					else
					{
						if(Config->patFoundCenterThreshold >= Config->patFoundLowerThreshold)
						{
							cprintf(Console, "SET CENTER THRESHOLD");
							Config->Threshold = Config->centerThreshold;
						}
						else
						{
							cprintf(Console, "SET LOWER THRESHOLD");
							Config->Threshold = Config->lowerThreshold;
						}
					}

					Config->pcf1 = 1;
					Config->pcf2 = 1;
					Config->patFoundSampleCount = 0;
					Config->thresholdAdjustFailCount = 0;
					Config->thresholdAdjustState = THRESHOLD_ADJUST_SETTLE;

				}
				else
				{
					Config->patFoundSampleCount++;
					Config->patFoundLowerThreshold += Config->pFound;
				}
		}
		else
		if(Config->thresholdAdjustState == THRESHOLD_ADJUST_SETTLE)
		{
			Config->patFoundSampleCount++;

			if(Config->patFoundSampleCount == SETTLE_SAMPLE_COUNT)
			{
				Config->thresholdAdjustState = THRESHOLD_ADJUST_ALL_FOUND;
			}
		}
}



/*______________________________________________________________________________

		EOF - finetune.c
  ______________________________________________________________________________
*/
