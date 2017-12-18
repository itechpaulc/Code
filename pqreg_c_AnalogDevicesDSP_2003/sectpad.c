/**SDOC*************************************************************************
 *
 *  sectpad.c - creates padding space in EEPROM for the config data file
 *
 *  Author: Mark Colvin				June 12, 2001
 *
 * $Header:   K:/Projects/PQCamera/PQCamera/STORAGE/sectpad.c_v   1.3   Dec 30 2002 13:49:24   APeng  $
 *
 * $Log:   K:/Projects/PQCamera/PQCamera/STORAGE/sectpad.c_v  $
 * 
 *    Rev 1.3   Dec 30 2002 13:49:24   APeng
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

    	$Id: sectpad.c,v 1.2 2000/09/28 10:30:17 Manuel Exp $

		Module	: sector padding
        Function: checks if padding for sector boundaries is necessary,
		          performs padding
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

#include <cam0.h> 	/* SECSIZE */
#include <stdio.h>

#include "pqlib.h"
#include "error.h"
#include "sectpad.h"	/* PADDING */

GMI_ASSERTFILE (__FILE__);

/**FDOC*************************************************************************
 *
 *  Function:       SectorPad
 *
 *  Author:         Mark Colvin             Date: June 12, 2001
 *
 *  Description:	handle padding for EEPROM files
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
int SectorPad (char* filename)
{
    /* declare and initialize local variables */
	long  TestAddr  = 0L;
	long  EpromAddr = 0L;
	ERROR Error     = ERROR_NONE;
	FILE* Ptr       = NULL;

    /* debug checks */
	GMI_ASSERT(filename != NULL);

	/* file exists? (any type) */
	if((EpromAddr = search (-1, filename)) > 0L)
	{
		/* show info message */
		SendDbgMsg(DBG_DEBUG, "OK: file '%s' found at %lX", filename, EpromAddr);
		if((EpromAddr % SECSIZE) == 0L)	/* file on sector boundary? */
		{
			/*
			   make sure this is the last file in the filesytem:
			   we do this by checking if the next address and the next FREE
			   address are identical
			*/
			TestAddr = fnaddr(EpromAddr);
			if(TestAddr == snext())
			{
				/* erase sector */
				SendDbgMsg(DBG_DEBUG, "OK: erasing sector %d", (int)(EpromAddr / SECSIZE));
				Error = erase((int)(EpromAddr / SECSIZE)) == 0 ? ERROR_NONE : ERROR_COULD_NOT_ERASE;
			}
			else
			{
				/* filesystem error */
				SendDbgMsg(DBG_ERRORS, "ERROR 17: file '%s' not the last in filesystem! (0x%lX)", filename, TestAddr);
				Error = ERROR_NOT_LAST_FILE;
                SendErrorMsg(ERROR_TYPE_FATAL_ERROR, Error );
			}
		}
		else
		{
			/* address error */
			SendDbgMsg(DBG_ERRORS, "ERROR 17a: file '%s' not on sector boundary! (%lX)", filename, EpromAddr % SECSIZE);
			Error = ERROR_NOT_SECTOR_ALIGNED;
            SendErrorMsg(ERROR_TYPE_FATAL_ERROR, Error );
		}
	}
	else
	{
		/* get address for new file */
		EpromAddr = snext();
		if((EpromAddr % SECSIZE) == 0L)	/* file on sector boundary? */
		{
			/* show info message */
			SendDbgMsg(DBG_DEBUG, "OK: next file possible at %lX", EpromAddr);
		}
		else							/* padding necessary! */
		{
			/* show info message */
			SendDbgMsg(DBG_DEBUG, "OK: padding needed (%lX)", EpromAddr);
			if(fopen(PADDING, "ra") == NULL)	/* no padfile present? */
			{
				/* open padfile for writing */
				Ptr = fopen(PADDING, "wb");
				if(Ptr)/* file open? */
				{
					/* calculate file length */
					unsigned int ByteCount   = (unsigned int)(Ptr->_data % SECSIZE);
					unsigned int BytesToFill = (unsigned int)(SECSIZE - (long) ByteCount - 1L);
                    /* shoe info message */
					SendDbgMsg(DBG_DEBUG, "OK: creating padfile at %lX, length: %x", EpromAddr, BytesToFill);
					/* create padfile */
					fseek(Ptr, BytesToFill, SEEK_SET);
					fclose(Ptr);
				}
				else
                {
					Error = ERROR_COULD_NOT_OPEN_FILE;	/* write error! */
                    SendErrorMsg(ERROR_TYPE_FATAL_ERROR, Error );
                }
			}
			else
			{
				/* corrupt padfile! */
				SendDbgMsg(DBG_ERRORS, "ERROR 18: padfile found but not on sector boundary!");
				Error = ERROR_FILE_CORRUPT;
                SendErrorMsg(ERROR_TYPE_FATAL_ERROR, Error );
			}
		}
	}
	return Error;
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

