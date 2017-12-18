/**SDOC*************************************************************************
**
**  bufpack.c - functions for 'packing' 8 bit buffer to 16bit buffer
**					(msb, lsb -> 16bit)
**
**  Author: Mark Colvin		6/12/01 11:56:22 AM

   	Author	:	Manuel Jochem
    	        FiberVision GmbH
    	        Jens-Otto-Krag Str. 11
    	        D-52146 Wuerselen
    	        Tel.: +49 (0) 2405 / 4548-21
    	        Fax : +49 (0) 2405 / 4548-14

		(C) FiberVision GmbH, 2000-2003

**
** $Header:   K:/Projects/PQCamera/PQCamera/PQLIB/bufpack.c_v   1.3   Dec 30 2002 13:43:04   APeng  $
**
** $Log:   K:/Projects/PQCamera/PQCamera/PQLIB/bufpack.c_v  $
 * 
 *    Rev 1.3   Dec 30 2002 13:43:04   APeng
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
 *    Rev 1.2   Dec 10 2002 15:59:14   APeng
 * some work has been done to the reduced
 * ROI
 *
 *    Rev 1.0   09 Jul 2001 12:40:02   MARKC
 * Checked in from initial workfile by PVCS Version Manager Project Assistant.
**
**
**********************************************************************SDOCEND**/

/*=======================  I N C L U D E   F I L E S  ========================*/

#include "gmiassert.h"

GMI_ASSERTFILE (__FILE__);

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

/*===========================  F U N C T I O N S  ============================*/
/*______________________________________________________________________________

    	$Id: bufpack.c,v 1.4 2000/09/21 17:04:44 Manuel Exp $

		Module	: buffer pack
        Function: pack buffer contens
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
**  Function:       BufferPack
**
**  Author:         Mark Colvin             Date:  6/12/01 11:57:51 AM
**
**  Description:	pack the lowbytes of a buffer into low and highbytes of
**					another buffer (msb, lsb -> 16bit)
**
**  Parameters:     source - pointer to source buffer
**					target - pointer to target buffer
**					size - buffer element count of target
**
**  Returns:        none
**
**  Side effects:   target is changed by contents of source,
**							target may be the same as source
**
**  History:		6/12/01 12:01:55 PM, mac	added function header
**
**********************************************************************FDOCEND**/
void BufferPack (unsigned int *source, unsigned int *target, size_t size)
{
	/* intermediate variable -> source and target may be the same */
	unsigned int value;

	/* debug checks */
	GMI_ASSERT(source != NULL);
	GMI_ASSERT(target != NULL);
	GMI_ASSERT(size > 0);

	/* 'pack' contents of source buffer to target buffer */
	while (size-- > 0)
	{
		/* first copy highbyte, */
		value  = *source++ << 8;
		/* then lowbyte */
		value += *source++ & 0xff;

		*target++ = value;
	}

	return;
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

