/**SDOC*************************************************************************
**
**  error.c - sends error message serial output
**
**  Author: Mark Colvin				6/25/01 3:46:53 PM
    	Author	: Manuel Jochem
    	          FiberVision GmbH
    	          Jens-Otto-Krag Str. 11
    	          D-52146 Wuerselen
    	          Tel.: +49 (0) 2405 / 4548-21
    	          Fax : +49 (0) 2405 / 4548-14

		(C) FiberVision GmbH, 2000-2003
**
** $Header:   K:/Projects/PQCamera/PQCamera/PQLIB/error.c_v   1.3   Dec 30 2002 13:43:08   APeng  $
**
** $Log:   K:/Projects/PQCamera/PQCamera/PQLIB/error.c_v  $
 * 
 *    Rev 1.3   Dec 30 2002 13:43:08   APeng
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
 *    Rev 1.2   Dec 10 2002 15:59:20   APeng
 * some work has been done to the reduced
 * ROI
 * 
 *    Rev 1.0   09 Jul 2001 12:40:30   MARKC
 * Checked in from initial workfile by PVCS Version Manager Project Assistant.
**
**
**********************************************************************SDOCEND**/

/*=======================  I N C L U D E   F I L E S  ========================*/
#include <vcrt.h>		/* ADDRESS */
#include <vclib.h>		/* image */
#include <cam0.h>		/* PGSIZE */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "command.h"	/* CMD_DEBUG_MESSAGE */
#include "global.h"		/* bDebugLevel */
#include "datagram.h"
#include "error.h"
#include "pqlib.h"

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

/*===========================  F U N C T I O N S  ============================*/
/*______________________________________________________________________________

    	$Id: error.c,v 1.1 2000/09/28 10:30:19 Manuel Exp $

		Module	: 
        Function: 
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
**
**  Function:       SendErrorMsg
**
**  Author:         Mark Colvin             Date: 6/25/01 1:40:55 PM
**
**  Description:	output error message to CCU, send
**					to the serial port
**
**  Parameters:     type - type of error message 
**					error - error number
**
**  Returns:        ERROR
**
**  Side effects:   
**
**  History:		6/12/01 1:24:27 PM, mac		added function header
**
**********************************************************************FDOCEND**/
ERROR SendErrorMsg (BYTE type, WORD error )
{
    ERROR fError = 0;
	char buffer[4];
    WORD swap;
    buffer[0] = type;
	buffer[3] = 0;

	swap  = SwapWord(error);
	buffer[1] = swap >>8;		   /* split the error number */
	buffer[2] = swap & 0xff;


	/* send to PC */
	fError = SendDatagram (CMD_ERROR_MESSAGE, 3, buffer, RAW);
	return fError;
}


/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/
