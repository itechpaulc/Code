/**SDOC*************************************************************************
**
**  globdef.c - global variables defined at start of static variable area
**
**  Author: Mark Colvin - June 23, 2001
    	Author	: Manuel Jochem
    	          FiberVision GmbH
				  Jens-Otto-Krag-Strasse 11
				  D-52146 Wuerselen
    	       	  Tel.: +49 (0)2405 / 4548-21
    	       	  Fax : +49 (0)2405 / 4548-14

		(C)	FiberVision GmbH, 2000-2003
**
** $Header:   K:/Projects/PQCamera/PQCamera/PQLIB/globdef.c_v   1.3   Dec 30 2002 13:43:10   APeng  $
**
** $Log:   K:/Projects/PQCamera/PQCamera/PQLIB/globdef.c_v  $
 * 
 *    Rev 1.3   Dec 30 2002 13:43:10   APeng
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
 *    Rev 1.2   Dec 10 2002 15:59:22   APeng
 * some work has been done to the reduced
 * ROI
 *
 *    Rev 1.0   09 Jul 2001 12:40:16   MARKC
 * Checked in from initial workfile by PVCS Version Manager Project Assistant.
**
**
**********************************************************************SDOCEND**/

/*=======================  I N C L U D E   F I L E S  ========================*/
/*
#include <stdio.h>
#include <vclib.h>
*/
#include "config.h"

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/
#define ASM_DEFINE(a)			ASM_PARA(## a ##, a)
#define ASM_PARA(a, b) 			asm("#define " #a " " #b)
#define GLOBALVAR(a, b, c)  	asm(".VAR/DM/RAM/ABS=0x40+" #a " " #c "_;\t\t!" #b "\n\t.GLOBAL " #c "_;")
#define GLOBALARRAY(a, b, c, d)	asm(".VAR/DM/RAM/ABS=0x40+" #a " " #c "_[" #d "];\t\t!" #b "\n\t.GLOBAL " #c "_;")

/*----------------------------------------------------------------------------*/

#ifndef __DA_C__

/* single word types */
GLOBALVAR(0, HCONSOLE,	Console);
GLOBALVAR(1, BYTE,	bDebugLevel);
GLOBALVAR(2, BYTE,	message_counter);
GLOBALVAR(3, BOOL,	UngetFlag);
GLOBALVAR(4, BYTE,	UngetChar);

#else

/* make development assitant for C shut up */
#define extern
#include "global.h"

#endif /* __DA_C__*/

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

/*===========================  F U N C T I O N S  ============================*/
/*______________________________________________________________________________

    	$Id: globdef.c,v 1.7 2001/04/20 11:49:33 Manuel Exp $

		Module	: PQ-Globaldef
        Function: definition global variables
    	Language: ISO C / ADSP assembler
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


/**FDOC*************************************************************************
**
**  Function:       UseGlobals
**
**  Author:         Mark Colvin             Date: June 23, 2001
**
**  Description:	Dummy function to force the linking of this module in every
**					program module of PrintQuick firmware
**
**  Parameters:     none
**
**  Returns:        none
**
**  Side effects:   none
**
**  History:		6/26/01 1:43:00 PM, mac - added function header
**
**********************************************************************FDOCEND**/
void UseGlobals(void)
{
/*______________________________________________________________________________

	Call this funtion in each program to force linking of this module
  ______________________________________________________________________________
*/

	return;
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

