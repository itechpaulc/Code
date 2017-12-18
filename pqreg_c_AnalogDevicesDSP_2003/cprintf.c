/**SDOC*************************************************************************
**
**  cprintf.c - functions for handling image overlay console text output
**
**  Author: Mark Colvin				6/12/01 12:37:39 PM

		Author	: Manuel Jochem
    	          FiberVision GmbH
    	          Jens-Otto-Krag Str. 11
    	          D-52146 Wuerselen
    	          Tel.: +49 (0) 2405 / 4548-21
    	          Fax : +49 (0) 2405 / 4548-14

		(C) FiberVision GmbH, 2000-2003

**
** $Header:   K:/Projects/PQCamera/PQCamera/PQLIB/cprintf.c_v   1.4   May 02 2003 10:31:48   APeng  $
**
** $Log:   K:/Projects/PQCamera/PQCamera/PQLIB/cprintf.c_v  $
 * 
 *    Rev 1.4   May 02 2003 10:31:48   APeng
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
 *    Rev 1.2   Dec 10 2002 15:59:16   APeng
 * some work has been done to the reduced
 * ROI
 *
 *    Rev 1.0   09 Jul 2001 12:40:06   MARKC
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

#include "image.h"

#include "pqlib.h"

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/
typedef struct tagCONSOLE
{
	image Console;
	image Text;
	int CurRow;
	int CurCol;
	int MaxRow;
	int MaxCol;
} CONSOLE;

static void cout(HCONSOLE Con, char* Buffer);
static void ConsoleClear(HCONSOLE Con);

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/
#define FONTSIZE_X 	1
#define FONTSIZE_Y 	2
#define PITCH_ROW 	(FONTSIZE_Y * 8 + 2)
#define PITCH_COL 	(FONTSIZE_X * 8)
#define MARGIN		8

/*#define FRAME_CONSOLE*/

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

/*===========================  F U N C T I O N S  ============================*/
/*______________________________________________________________________________

    	$Id: cprintf.c,v 1.5 2000/09/29 14:54:08 Manuel Exp $

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
**  Function:       cprintf
**
**  Author:         Mark Colvin             Date: 6/12/01 12:40:55 PM
**
**  Description:	output text on the camera video using overlay graphics
**
**  Parameters:     console - struct with console image area information
**					format - printf format string
**					VARIABLE args allowed here
**
**  Returns:        number of characters output to console
**
**  Side effects:   uses 81 bytes of stack (limit of output text)
**
**  History:		6/12/01 1:24:27 PM, mac		added function header
**
**********************************************************************FDOCEND**/
int cprintf (HCONSOLE console, const char *format,...)
{
  	int result = -1;

	if(console)
	{
		char buffer[81];
		va_list arg_ptr;         	/* argument pointer                     */

		va_start(arg_ptr,format);  	/* init argument pointer                */
		result = intvsprintf (buffer, format, arg_ptr);
											/* fill buffer with formatted text */
		cout(console, buffer);				/* output this text to overlay area */
		va_end(arg_ptr);			/* clean up the variable arguments */
	}

	return result;
}

/**FDOC*************************************************************************
**
**  Function:       copen
**
**  Author:         Mark Colvin             Date: 6/12/01 1:32:33 PM
**
**  Description:	initialize the console image area structures
**
**  Parameters:     x,y	- upper left corner of overlay area
**					Cols - number of columns of text
**					Rows - number of rows of text
**
**  Returns:        pointer to console structure in heap
**
**  Side effects:   requires CONSOLE bytes of heap
**
**  History:		6/12/01 1:35:41 PM, mac	added function header
**
**********************************************************************FDOCEND**/
HCONSOLE copen (int X, int Y, int Cols, int Rows)
{
	/* create new console object */
	HCONSOLE New = (HCONSOLE) vcmalloc(sizeof(CONSOLE));

	if(New)
	{
		/* define position and size of console window */
		int Left   = (X / 16) * 16;	/* overlay should be accessed in multiples of 16 */
		int Top    = Y;
		int Width  = Cols * PITCH_COL + 2 * MARGIN;
		int Height = Rows * PITCH_ROW + 2 * MARGIN;

		New->Console = ImageCreate (Left - MARGIN, Top - MARGIN, Width, Height, PAGE_OVERLAY);
		/* init position and size of first text line */
		New->Text    = ImageCreate (Left, Top, Width, PITCH_ROW, PAGE_OVERLAY);

		New->CurRow = 0;
		New->CurCol = 0;
		New->MaxRow = Rows;
		New->MaxCol = Cols;

#ifdef FRAME_CONSOLE
		ImageFrameo(&(New->Console));
#endif
	}
	return New;
}

/**FDOC*************************************************************************
**
**  Function:       cclose
**
**  Author:         Mark Colvin             Date: 6/12/01 1:36:50 PM
**
**  Description:	closes the overlay console area and free console structure
**
**  Parameters:     Con - pointer to console struct buffer in heap
**
**  Returns:        none
**
**  Side effects:   frees HCONSOLE of heap
**
**  History:		6/12/01 1:38:24 PM, mac	added function header
**
**********************************************************************FDOCEND**/
void cclose(HCONSOLE Con)
{
	vcfree((void*)Con);		/* console struct is freed from heap */
}

/**FDOC*************************************************************************
**
**  Function:       cout
**
**  Author:         Mark Colvin             Date: 6/12/01 1:38:56 PM
**
**  Description:	generates text in overlay graphics for the console area
**
**  Parameters:     Con - pointer to console struct
**					buffer - pointer to buffer holding the text
**
**  Returns:        none
**
**  Side effects:   none
**
**  History:		6/12/01 1:41:39 PM, mac	added function header
**
**********************************************************************FDOCEND**/
static void cout(HCONSOLE Con, char* Buffer)
{
	char* CRPos = NULL;
	char* FFPos = NULL;

	/* Check for newline */
	CRPos = strchr(Buffer, '\n');
	if (CRPos != NULL)
		*(CRPos - 1) = '\0';			/* replace newline with NULL */

	/* Check for formfeed */
	FFPos = strchr(Buffer, '\f');
	if (FFPos != NULL)
	{
		ConsoleClear(Con);				/* clear console area if formfeed */
		return;
	}

	/* Check for empty string */
	if (strcmp(Buffer, ""))				/* no empty strings */
	{
		if (Con->CurRow >= Con->MaxRow)		/* if go past bottom, clear overlay */
			ConsoleClear(Con);
												 /* output text here */
		chprint_ov (Buffer, &(Con->Text), FONTSIZE_X, FONTSIZE_Y);
		Con->Text.st += (ADDRESS) PITCH_ROW * (ADDRESS) Con->Text.pitch;
		Con->CurRow++;			/* next addr for text output */
	}
	return;
}

/**FDOC*************************************************************************
**
**  Function:       ConsoleClear
**
**  Author:         Mark Colvin             Date: 6/12/01 1:53:20 PM
**
**  Description:	Clears the console area
**
**  Parameters:     Con - pointer to console struct buffer
**
**  Returns:        none
**
**  Side effects:   change the overlay graphics
**
**  History:		6/12/01 1:55:32 PM, mac		added function header
**
**********************************************************************FDOCEND**/
static void ConsoleClear(HCONSOLE Con)
{
	ImageOverlaySet (&(Con->Console), 0);	/* fills the overlay area with 0 */
#ifdef FRAME_CONSOLE
	ImageFrameo(&(Con->Console));				/* draw the frame */
#endif

	Con->CurRow  = 0;						/* reset row index */
	Con->Text.st = Con->Console.st			/* reset the address for next text */
				 + (ADDRESS) MARGIN * (ADDRESS) Con->Text.pitch + MARGIN;
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

