/**SDOC*************************************************************************
**
**  dbgsend.c - sends text messages to video and serial output
**
**  Author: Mark Colvin				6/12/01 3:46:53 PM
    	Author	: Manuel Jochem
    	          FiberVision GmbH
    	          Jens-Otto-Krag Str. 11
    	          D-52146 Wuerselen
    	          Tel.: +49 (0) 2405 / 4548-21
    	          Fax : +49 (0) 2405 / 4548-14

		(C) FiberVision GmbH, 2000-2003
**
** $Header:   K:/Projects/PQCamera/PQCamera/PQLIB/dbgsend.c_v   1.4   May 02 2003 10:31:50   APeng  $
**
** $Log:   K:/Projects/PQCamera/PQCamera/PQLIB/dbgsend.c_v  $
 * 
 *    Rev 1.4   May 02 2003 10:31:50   APeng
 *  
 *
 *    Rev 1.3   Dec 30 2002 13:43:06   APeng
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
 *    Rev 1.2   Dec 10 2002 15:59:18   APeng
 * some work has been done to the reduced
 * ROI
 *
 *    Rev 1.0   09 Jul 2001 12:40:10   MARKC
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
#include "pqlib.h"
#include "datagram.h"
#include "error.h"


/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

#define MAX_STR_LEN 75

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

/*===========================  F U N C T I O N S  ============================*/
/*______________________________________________________________________________

    	$Id: dbgsend.c,v 1.1 2000/09/28 10:30:19 Manuel Exp $

		Module	: cprintf
        Function: console cprintf
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
**  Function:       SendDbgMsg
**
**  Author:         Mark Colvin             Date: 6/12/01 12:40:55 PM
**
**  Description:	output text on the camera video using overlay graphics, send
**					to the serial port
**
**  Parameters:     level - level of debug message types are allowed to be displayed
**					format - printf format string
**					VARIABLE args allowed here
**
**  Returns:        ERROR
**
**  Side effects:   uses 75 bytes of stack (limit of output text)
**
**  History:		6/12/01 1:24:27 PM, mac		added function header
**
**********************************************************************FDOCEND**/
ERROR SendDbgMsg (DBG_LEVEL level, const char *format,...)
{
  	ERROR Error = ERROR_NONE;
    int lvl=bDebugLevel;

    if( (lvl == DBG_ERRORS_GREY) || (lvl ==	DBG_ERRORS_BIN))
        lvl =	DBG_ERRORS;

    if( (lvl == DBG_DEBUG_GREY_MARK) || (lvl == DBG_DEBUG_GREY) ||
        (lvl ==	DBG_DEBUG_BIN) )
	    lvl = DBG_DEBUG;

	if(level <= lvl)
	{
		char buffer[MAX_STR_LEN + 1];
		va_list arg_ptr;         	/* argument pointer                     */

		va_start(arg_ptr,format);  	/* init argument pointer                */

		if(intvsprintf (buffer, format, arg_ptr) > 0)
		{
			/* clamp string length to maximum value */
			int len = strlen(buffer);

			if (len > MAX_STR_LEN)
				len = MAX_STR_LEN;

			/* print to screen first (in case of SIO error) */
			cprintf(Console, buffer);

			/* send to PC */
			Error = SendDatagram (CMD_DEBUG_MESSAGE, len, buffer, RAW);
		}

		va_end(arg_ptr);
	}

	return Error;
}


/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

