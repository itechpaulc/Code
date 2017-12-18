
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
 * $Header:   K:/Projects/PQCamera/PQCamera/CONFIG/config.c_v   1.8   May 02 2003 10:58:52   APeng  $
 *
 * $Log:   K:/Projects/PQCamera/PQCamera/CONFIG/config.c_v  $
 * 
 *    Rev 1.8   May 02 2003 10:58:52   APeng
 *  
 * 
 *    Rev 1.7   May 02 2003 10:32:46   APeng
 *  
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
#include "storage.h"
#include "global.h"
#include "image.h"
#include "pqlib.h"

GMI_ASSERTFILE (__FILE__);

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

static void ConfigInit(PQ_CFG* Config);

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

/* software version */
PRIVATE long nVersion = (MAJOR_VERSION << 16) + MINOR_VERSION;

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

#define	INIT_MIN_BRIGHTNESS				(50)
#define	INIT_MIN_CONTRAST				(26)
#define	INIT_MAX_CONTRAST_DEVIATION		(4)

/* Max Measurement offset deviation in pixels */

#define	INIT_MAX_OFFSET_DEVIATION		(30)
#define	INIT_TRACKING_PCF				(90)


/* Measurement offset deviation has to be		*/
/* stable for n counts before it becomes valid	*/

#define	INIT_MIN_MEAS_STABLE_COUNT		(3)


/*===========================  F U N C T I O N S  ============================*/

/*______________________________________________________________________________

    	$Id: config.c,v 1.16 2000/11/30 17:15:42 Manuel Exp $

		Module	: PQ-Config
        Function: handling of configuration data
    	Language: ISO C
	    System	: VC/RT

    	Author	: Manuel Jochem
    	          FiberVision GmbH
				  Jens-Otto-Krag-Strasse 11
				  D-52146 Wuerselen
    	       	  Tel.: +49 (0)2405 / 4548-21
    	       	  Fax : +49 (0)2405 / 4548-14

		(C)	FiberVision GmbH, 2000-2003
  ______________________________________________________________________________
*/




/*______________________________________________________________________________

	Entry point for the PQ configuration module
  ______________________________________________________________________________
*/

/**FDOC*************************************************************************
 *
 *  Function:       main
 *
 *  Author:         Mark Colvin             Date: June 11, 2001
 *
 *  Description:	main function of the Config module
 *
 *  Parameters:     dummy	module name
 *					argc	overlay call
 *					Cmd		operation requested
 *					CfgPtr	pointer to config structure in heap
 *					Buffer	pointer to spare buffer in heap
 *					Size	buffer size of Buffer
 *
 *  Returns:        none
 *
 *  Side effects:   none
 *
 *  History:		6/11/01 10:37:49 PM, mac	added function header
 *
 *********************************************************************FDOCEND**/
int MAIN(Config)(int dummy, int argc, PQ_CMD Cmd, PQ_CFG** CfgPtr, BYTE* Buffer, int Size)
{
    /* declare and initialize local variables */
	ERROR Error    = ERROR_NONE;
	char* BootMsg  = "PQ-config  [build: "__BUILD__"]  "__DATE__"  "__TIME__;
	PQ_CFG* Config = *CfgPtr;
	int i,j,k;

	/* show info message on screen */
	SendDbgMsg(DBG_INFO, BootMsg);

	GMI_ASSERT(CfgPtr);

	/* make sure global variables are not overwritten */
	UseGlobals();

	if(Size > 0)
		SendDbgMsg(DBG_DEBUG, "OK: cfg data [cmd: %02X, cnt: %02X, buf: %02X]", Cmd, Size, *Buffer);
	else
		SendDbgMsg(DBG_DEBUG, "OK: cfg data [cmd: %02X, cnt: %02X, buf: n/a]", Cmd, Size);

	/* process command */
	switch(Cmd)
	{
	  case PQ_INIT:	/* init configuration (prog startup)*/
	  	/* create CONFIG object */
		Config = (PQ_CFG*) vcmalloc(sizeof(PQ_CFG));
		if(!Config)
		{
			Error = ERROR_MEMORY_OVERFLOW;
			SendDbgMsg(DBG_ERRORS, "ERROR 01: memory overflow");
		}
		else
		{
			/* give pointer back to calling program */
			*CfgPtr = Config;

			/* initialize configuration data with compile-time defaults */
			ConfigInit(Config);

			/* try to load data from EEPROM (may fail) */
			Error = storage(IO_LOAD, Config, sizeof(PQ_CFG));
			if (Error)
				SendDbgMsg(DBG_WARNINGS, "WARNING: no configuration file found");
			else
			{
				/* reset search mode to idle */
				Config->bSearchMode = SEARCH_IDLE;
				SendDbgMsg(DBG_INFO, "OK: load config file");
			}
		}
		break;

	  case CMD_EXIT_PROGRAM:		/* clean up configuration (prog exit)*/
	  	/* release CONFIG object */
	  	vcfree((void*) Config);
		SendDbgMsg(DBG_DEBUG, "OK: clean up CONFIG");
		break;

	  case CMD_REQUEST_VERSION:			/* Request firmware version */
	  {
	    long Version = SwapLong(nVersion);

		Error = SendDatagram (CMD_SEND_VERSION, sizeof(long), (BYTE*) &Version, SPLIT);
		if (Error)
			SendDbgMsg(DBG_ERRORS, "ERROR 02: send version failed");
		else
			SendDbgMsg(DBG_INFO, "OK: send version [V. %08lX]", nVersion);
		break;
	  }

	  case CMD_LOAD_SYS_CFG:			/* Load system configuration      */
		Config->NumReadsToAverage = Buffer[0];
		Config->MinTrackingPcfLevel = Buffer[1];
		SendDbgMsg(DBG_INFO, "OK: load system config");
		break;

	  case CMD_LOAD_FRM_CFG:			/* Load form configuration        */
		for (i = 0, j = 0; j < NREGMARK; i++, j++)
			Config->bMarkEnabled[j] = Buffer[i];

		Config->bReferenceMarkSelection = Buffer[i++];

		for ( j=0; j < NREGMARK; i++,j++ )
				Config->bMarkContrast[j] = Buffer[i];

		Config->bTargetPatternID = Buffer[i];
		/*SendDbgMsg(DBG_INFO, "OK: load form config");*/

		SendDbgMsg(DBG_ERRORS, "OK: load form config PId:%d", Config->bTargetPatternID);

		break;

	  case CMD_UPDATE_FLASH_COUNT:		/* Update flash counter             */
		BufferPack((unsigned int*) Buffer, (unsigned int*) Buffer, sizeof(long));
		Config->FlashCounter = SwapLong(* (long*) Buffer);
		SendDbgMsg(DBG_INFO, "OK: set flash counter to %ld", Config->FlashCounter);
		break;

	  case CMD_REQUEST_STATUS:
	  	RequestStatus(Config, Buffer);
		break;

	  case CMD_SEND_SYS_CFG:			/* send form configuration        */
	  {
	  	DWORD SwapCounter = 0L;
		j = 0;

		Buffer[j++] = Config->bSearchMode;
		Buffer[j++] = bDebugLevel;
		Buffer[j++] = Config->bTargetPatternID;

		for (i = 0; i < NREGMARK; i++, j++ )
			Buffer[j] = Config->bMarkEnabled[i];

		Buffer[j++] = Config->bReferenceMarkSelection;

		for (i = 0; i < NREGMARK; i++, j++ )
			Buffer[j] = Config->bMarkContrast[i];

		Buffer[j++] = Config->bImageLevel;
		Buffer[j++] = Config->bUnderSample;
		Buffer[j++] = Config->bImageType;
	  	SwapCounter = SwapLong(Config->FlashCounter);
		BufferSplit((unsigned int*) &SwapCounter, (unsigned int*) Buffer + j, sizeof(DWORD));
 		j += BYTESIZE(DWORD);

		Error = SendDatagram (CMD_SYSTEM_CONFIG, sizeof(BYTE) * j, Buffer, RAW);
		if (Error)
			SendDbgMsg(DBG_ERRORS, "ERROR 04: send form config failed");
		else
			SendDbgMsg(DBG_INFO, "OK: send form config");
		break;
	  }

	  case CMD_LOAD_PATTERNS:		/*  Load new target pattern info  */
	  {
		int pId;
		int  Index  = Buffer[0];					  /* adjust ptr */
		int *PatPtr = (int*) Config->Patterns + (Index * sizeof(PATTERN_CONFIG));
		PATTERN_CONFIG	*Patterns   = Config->Patterns;

		SendDbgMsg(DBG_ERRORS, "OK: load new target pattern info (%d)", Index);

		BufferPack((unsigned int*) Buffer+18, (unsigned int*) Buffer+18,
													(sizeof(PATTERN_CONFIG)-16));

		/* move in the pattern name */
		for (i = 1, j = 0; j < MAX_PATTERN_NAME_LTH; i++, j++ )
		{
			PatPtr[j] = Buffer[i];
		}

		/* move in the pattern vectors arrays */
		for (k = 0, ++i; k < (4 * NREGMARK); i += 2, j+=2, k++ )
		{
			PatPtr[j+1] = SwapWord((WORD) Buffer[i]);
			PatPtr[j] = SwapWord((WORD) Buffer[i+1]);
		}

		/* move in the pattern limits */
		for (k = 0; k < 3; i++, j++, k++ )
		{
			PatPtr[j] = SwapWord((WORD) Buffer[i]);
		}

		pId = Config->bTargetPatternID;
		Config->LineFilterSize = Patterns[pId].bLineFilterSize;

		Config->RegMeasMaxOffsetDeviation =
			(SCALE_FACTOR * INIT_MAX_OFFSET_DEVIATION);

/*		for( i = 0; i < 100; i+=8)
		{
			SendDbgMsg(DBG_DEBUG, "pack %2d - %4X %4X %4X %4X %4X %4X %4X %4X ",
				i, Buffer[i], Buffer[i+1], Buffer[i+2], Buffer[i+3], Buffer[i+4],
				Buffer[i+5], Buffer[i+6], Buffer[i+7] );
		}
*/

		/* Display the pattern information from the pattern structures */
/*		for( i = 0; i < MAX_NUM_PATTERNS; i++)  */
		i = Index;
		{
			SendDbgMsg(DBG_DEBUG, "Pattern %d width %d  min area %d max area %d", i,
				Patterns[i].bLineFilterSize, Patterns[i].bMinElementSize,
				Patterns[i].bMaxElementSize);
			for( j=0; j < NREGMARK; j++ )
			{
				SendDbgMsg(DBG_DEBUG, "   %4ld, %4ld  -  %4ld, %4ld",
					Patterns[i].iPattern1X[j], Patterns[i].iPattern1Y[j],
					Patterns[i].iPattern2X[j], Patterns[i].iPattern2Y[j] );
			}
		}
		break;
	  }
	  case CMD_LOAD_SHUTTER:		/* Load shutter value             */
	  {
	  	long Shutter;

		BufferPack((unsigned int*) Buffer, (unsigned int*) Buffer, sizeof(long));

		Shutter = SwapLong(* (long*) Buffer);

		if ((Shutter >= 100) && (Shutter < 10000000))
		{
			Config->Shutter = Shutter;
			shutter(Config->Shutter);	/* set integration time */
			SendDbgMsg(DBG_INFO, "OK: set shutter to %ld", Config->Shutter);
		}
		else
		{
			SendDbgMsg(DBG_WARNINGS, "WARNING: unknown shutter [%ld] - ignored", Shutter);
		}
		break;
	  }
	  case CMD_START_DEBUG:				/* Set debug level                */
	    if (Buffer[0] < DBG_MAX)
		{
			bDebugLevel = Buffer[0];
			SendDbgMsg(DBG_INFO, "OK: set debug level to %d", bDebugLevel);
		}
		else
		{
			SendDbgMsg(DBG_WARNINGS, "WARNING: unknown debug level [%d] - ignored", Buffer[0]);
		}

	    if ( (Buffer[1] < 100))
		{
			Config->bImageLevel = Buffer[1];
			SendDbgMsg(DBG_INFO, "OK: set image level to %d", Config->bImageLevel);
		}
		else
		{
			SendDbgMsg(DBG_WARNINGS, "WARNING: unknown image level [%d] - ignored", Buffer[1]);
		}

	    if ((Buffer[2] <= 8) && ((Buffer[2] % 2) == 0))/* has to be even! */
		{
			Config->bUnderSample = Buffer[2];
			SendDbgMsg(DBG_INFO, "OK: set undersampling to %d", Config->bUnderSample);
		}
		else
		{
			SendDbgMsg(DBG_WARNINGS, "WARNING: unknown undersampling [%d] - ignored", Buffer[2]);
		}

	    if ((Buffer[3] <= IMAGE_RLE))
		{
			Config->bImageType = Buffer[3];
			SendDbgMsg(DBG_INFO, "OK: set image type to %d", Config->bImageType);
		}
		else
		{
			SendDbgMsg(DBG_WARNINGS, "WARNING: unknown image type [%d] - ignored", Buffer[3]);
		}

		break;

	  case CMD_STOP_DEBUG:				/* clear debug level - obsolete       */
		SendDbgMsg(DBG_WARNINGS, "WARNING: obsolete command - ignored");
		break;

	  case CMD_LOAD_EEPROM: /* load configuration */
		Error = storage(IO_LOAD, Config, sizeof(PQ_CFG));
		if (Error)
			SendDbgMsg(DBG_ERRORS, "ERROR 05: no config file found");
		else
		{
			/* reset search mode to idle */
			Config->bSearchMode = SEARCH_IDLE;
			SendDbgMsg(DBG_INFO, "OK: load config file");
		}
		break;

	  case CMD_SAVE_EEPROM:  /* save configuration */
		Error = storage(IO_SAVE, Config, sizeof(PQ_CFG));
		if (Error)
			SendDbgMsg(DBG_ERRORS, "ERROR 06: flash eeprom write error");
		else
			SendDbgMsg(DBG_INFO, "OK: save config file");
		break;

	  case CMD_CENTER_ROI:  /* make the RoI image located the center of the FoV */
	    Config->RoI.st = PIXEL_ADDR((IMAGE_WIDTH-TRACK_IMAGE_SIZE)/2,
                                (IMAGE_HEIGHT-TRACK_IMAGE_SIZE)/2 );
		SendDbgMsg(DBG_INFO, "OK: RoI centered" );
		break;

	  default:	/* unknown command */
	    Error = ERROR_UNKNOWN_CMD;
		SendDbgMsg(DBG_WARNINGS, "WARNING: unknown command - ignored");
	    break;
	}

	/* exit module */
	return Error;
}

/**FDOC*************************************************************************
**
**  Function:       ConfigInit
**
**  Author:         Mark Colvin             Date:  June 12, 2001
**
**  Description:	Preset configuration data with compile-time defaults
**
**  Parameters:     Config - pointer to PQ main configuration structure (on heap)
**
**  Returns:        none
**
**  Side effects:   initializes structure Config
**
**  History:		6/12/01 10:37:21 AM, mac	added function header
**
**********************************************************************FDOCEND**/

static void ConfigInit(PQ_CFG* Config)
{
	/* get screen size */
	int dx = getvar(hwidth) * 2;
	int dy = getvar(vwidth);
	int i;

	/* Pattern Configuration */
	PATTERN_CONFIG Patterns[ MAX_NUM_PATTERNS ]=
	{
		{
			/* Caloffset STD */

			"Cal Offset test ",
			{-1,-1,-26,-26,-13,-13,-1,-1},
			{-1, 1,-10, 10,-10, 10,-1, 1},
			{ 1, 1, 26, 26, 13, 13, 1, 1},
			{ 1,-1, 10,-10, 10,-10, 1,-1},
			8, 30, 120

			/* Calculated Caloffset 125% */

			/*
			{-1,-1,-33,-33,-17,-17,-1,-1},
			{-1, 1,-13, 13,-13, 13,-1, 1},
			{ 1, 1, 33, 33, 17, 17, 1, 1},
			{ 1,-1, 13,-13, 13,-13, 1,-1},
			10, 38, 150
			*/
		},
	 	{
			"Cal Offset test ",
			{-1,-1,-26,-26,-13,-13,-1,-1},
			{-1, 1,-10, 10,-10, 10,-1, 1},
			{ 1, 1, 26, 26, 13, 13, 1, 1},
			{ 1,-1, 10,-10, 10,-10, 1,-1},
			8, 30, 120
		},
	 	{
			"Cal Offset test ",
			{-1,-1,-26,-26,-13,-13,-1,-1},
			{-1, 1,-10, 10,-10, 10,-1, 1},
			{ 1, 1, 26, 26, 13, 13, 1, 1},
			{ 1,-1, 10,-10, 10,-10, 1,-1},
			8, 30, 120
		}
	};

	memcpy(Config->Patterns, Patterns, MAX_NUM_PATTERNS * sizeof(PATTERN_CONFIG));

	/* set variables */
	Config->FoV        = ImageCreate( 0,  0, dx, dy, IMG_DEFAULT);
	Config->RoI        = ImageCreate((dx-TRACK_IMAGE_SIZE)/2, (dy-TRACK_IMAGE_SIZE)/2,
							TRACK_IMAGE_SIZE, TRACK_IMAGE_SIZE, IMG_DEFAULT);
	Config->Version    = 0x01;

	Config->MinBrightness	= INIT_MIN_BRIGHTNESS;
	Config->MinContrast		= INIT_MIN_CONTRAST;
	Config->MaxContrastDeviation = INIT_MAX_CONTRAST_DEVIATION;

	Config->ContrastFailureCount = 0;
	Config->BrightnessFailureCount = 0;
	Config->RoiSpaceFailureCount = 0;

	Config->Threshold  =  80;
	Config->RLC        = 0x80000L;
	Config->MaxRLC     = 0x20000L;

	Config->LineFilterSize = 5;

	/* Form Configuration */
	memset(Config->bMarkEnabled, 1, NREGMARK);/* for each mark:				 */
		 							/*   TRUE if enabled,					 */
									/*   FALSE if not.						 */
	Config->bReferenceMarkSelection=4;/* index of Pattern mark to locate	*/
									/* during Hunt mode (0 to NREGMARK)		 */
	memset(Config->bMarkContrast, 0 , NREGMARK);/* for each mark:			 */
		       						/* 	0 - High contrast ink				 */
									/*	1 - Medium contrast ink				 */
									/*	2 - Low contrast ink				 */
									/*	3 - Reflective ink					 */
	Config->bTargetPatternID=0;     /* ID of Target Pattern to use        	 */
									/* (0 to use default target pattern)	 */
	/* Status fields */
	Config->nCameraStatus = CS_NOTREADING;  /* camera status				*/
	Config->bSearchMode = SEARCH_IDLE;		/* camera search mode			*/
	Config->bImageType = 0;			/* type of image to be send		*/
									/* 0 - JPEG						*/
									/* 1 - BMP						*/
									/* 2 - RLE						*/

	Config->bImageLevel = 30;		/* quality of image to be send	*/
									/* BMP: ignored					*/
									/* JPG: 1 - 99 %				*/
									/* RLE: ignored (opt: 0-255 GV)	*/

	Config->bUnderSample = 0;		/* XY pixel subsampling			*/
									/* 0: no subsampling			*/
									/* 2: every 2nd pixel 			*/
									/* 8: every 8th pixel 			*/

	Config->RegMeasMaxOffsetDeviation =
		(SCALE_FACTOR * INIT_MAX_OFFSET_DEVIATION);

	/* Press Configuration */

	Config->NumReadsToAverage	= 1;
	Config->MinTrackingPcfLevel = INIT_TRACKING_PCF;

	Config->MinMeasStableCount = INIT_MIN_MEAS_STABLE_COUNT;

	Config->Shutter    = 500L;
	shutter(Config->Shutter);

	for (i = 0; i < NREGMARK; i++)
	{
		Config->strcRegPos[i].nCircum  = 0L;
		Config->strcRegPos[i].nLateral = 0L;
		Config->strcRegPos[i].nMarkStatus  = MARK_MISSING;
	}

	Config->FlashCounter = 0L;

	Config->NarrowRoiMargin = 30;

	return;
}



/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

