/**SDOC*************************************************************************
 *
 *  storage.c - stores the configuration data for camera in the EEPROM
 *
 *  Author: Mark Colvin				June 12, 2001
    	Author	: Manuel Jochem
    	          FiberVision GmbH
				  Jens-Otto-Krag-Strasse 11
				  D-52146 Wuerselen
    	       	  Tel.: +49 (0)2405 / 4548-21
    	       	  Fax : +49 (0)2405 / 4548-14

		(C)	FiberVision GmbH, 2000-2003
 *
 * $Header:   K:/Projects/PQCamera/PQCamera/STORAGE/storage.c_v   1.3   Dec 30 2002 13:49:28   APeng  $
 *
 * $Log:   K:/Projects/PQCamera/PQCamera/STORAGE/storage.c_v  $
 * 
 *    Rev 1.3   Dec 30 2002 13:49:28   APeng
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
 *    Rev 1.2   Dec 10 2002 15:59:26   APeng
 * some work has been done to the reduced
 * ROI
 *
 *    Rev 1.0   09 Jul 2001 12:41:24   MARKC
 * Checked in from initial workfile by PVCS Version Manager Project Assistant.
 *
 *
 *********************************************************************SDOCEND**/

/*=======================  I N C L U D E   F I L E S  ========================*/

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

/*===========================  F U N C T I O N S  ============================*/

/*______________________________________________________________________________

    	$Id: storage.c,v 1.4 2000/09/28 10:30:17 Manuel Exp $

		Module	: PQ-Storage
        Function: read/write data from/to EEPROM file
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

#include <stdio.h>
#include <vcdefs.h>
#include <vcrt.h>
#include <vclib.h>


#include "config.h"
#include "error.h"
#include "global.h"
#include "pqlib.h"
#include "build.h"

#include "sectpad.h"
#include "storage.h"
#include "image.h"


GMI_ASSERTFILE (__FILE__);

/**FDOC*************************************************************************
 *
 *  Function:       main
 *
 *  Author:         Mark Colvin             Date: June 12, 2001
 *
 *  Description:	Entry point for the PQ storage module
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
int MAIN(Storage)(int dummy, int argc, IO_CMD cmd, void* buffer, int size)
{
    /* declare and initialize local variables */
	char* BootMsg      = "PQ-storage [build: "__BUILD__"]  "__DATE__"  "__TIME__"";
	ERROR Error        = ERROR_NONE;
	FILE *Settings     = NULL;
	void *doublebuffer = NULL;
	int   doublesize   = size * 2;

	/* show info message on screen */
	SendDbgMsg(DBG_INFO, BootMsg);

	/* debug checks */
	GMI_ASSERT (buffer != NULL);
	GMI_ASSERT (size   > 0);

	/* make sure global variables are not overwritten */
	UseGlobals();

	/* get memory for working buffer */
	doublebuffer = (void*) vcmalloc (doublesize);
	if (!doublebuffer)	/* no memory? */
	{
		/* exit with error code*/
		return ERROR_MEMORY_OVERFLOW;
	}

	/* process command: */
	switch(cmd)
	{
	  case IO_LOAD:	/* read EEPROM file: */
		/* open file for reading */
		Settings = fopen (DATAFILE, "rb");
		if (Settings)	/* file open? */
		{
			/* reading data? */
			if (1 == fread (doublebuffer, doublesize, 1, Settings))
			{
				/* put data into correct binary format */
				BufferPack (doublebuffer, buffer, size);
		   	    SendDbgMsg(DBG_DEBUG,"OK: loading %d bytes", size * 2);
			}
			else
			{
				/* read error! */
				Error = ERROR_FILE_CORRUPT;
			}
			/* close file */
			fclose (Settings);
		}
		else
		{
			/* file not found! */
			Error = ERROR_FILE_NOT_FOUND;
		}
		break;
	  case IO_SAVE:	/* write EEPROM file: */
	  	/* file on sector boundary? */
		if(!SectorPad(DATAFILE))
		{
			/* open file for writing */
			Settings = fopen (DATAFILE, "wb");
			if (Settings)	/* file open? */
			{
				/* put data into correct binary format */
				BufferSplit (buffer, doublebuffer, size);

				/* writing data? */
				if (1 == fwrite (doublebuffer, doublesize, 1, Settings))
				{
					/* ok! */
			   	    SendDbgMsg(DBG_DEBUG,"OK: saving %d bytes", size * 2);
				}
				else
				{
					/* write error! */
					Error = ERROR_FLASH_WRITE_ERROR;
				}

				/* close file */
				if (EOF == fclose (Settings))
				{
					/* write error! */
					Error = ERROR_FLASH_WRITE_ERROR;
				}
			}
			else
			{
				/* write error! */
				Error = ERROR_FLASH_WRITE_ERROR;
			}
		}
		else
		{
			/* unknown error! */
			Error = ERROR_UNKNOWN_ERROR;
		}
	    break;
	  default:	/* unknown command: */
	  	Error = ERROR_UNKNOWN_CMD;	/* error! */
		GMI_ASSERT(NOT_REACHED);
	    break;
	}
	/* free memory */
	vcfree(doublebuffer);

	/* return error code */
	return Error;
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

