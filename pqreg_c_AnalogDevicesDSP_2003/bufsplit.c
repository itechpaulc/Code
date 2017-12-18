/**SDOC*************************************************************************
**
**  bufsplit.c - functions for expanding 16bit buffer to 8bit buffers
**					(16bit -> msb, lsb)
**
**  Author: Mark Colvin			6/12/01 12:09:44 PM

    	Author	: Manuel Jochem
    	          FiberVision GmbH
    	          Jens-Otto-Krag Str. 11
    	          D-52146 Wuerselen
    	          Tel.: +49 (0) 2405 / 4548-21
    	          Fax : +49 (0) 2405 / 4548-14

		(C) FiberVision GmbH, 2000-2003

**
** $Header:   K:/Projects/PQCamera/PQCamera/PQLIB/bufsplit.c_v   1.3   Dec 30 2002 13:43:04   APeng  $
**
** $Log:   K:/Projects/PQCamera/PQCamera/PQLIB/bufsplit.c_v  $
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
 *    Rev 1.2   Dec 10 2002 15:59:16   APeng
 * some work has been done to the reduced
 * ROI
 *
 *    Rev 1.0   09 Jul 2001 12:40:04   MARKC
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

    	$Id: bufsplit.c,v 1.3 2000/09/15 17:23:43 Manuel Exp $

		Module	: buffer split
        Function: expand buffer comntens
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
**  Function:       BufferSplit
**
**  Author:         Mark Colvin             Date: 6/12/01 12:10:41 PM
**
**  Description:	expand the low- and highbytes of a buffer into lowbytes of
**					another buffer  (16bit -> msb, lsb)
**
**  Parameters:     source - pointer to source buffer
**					target - pointer to target buffer
**					size - buffer element count of the source buffer
**
**  Returns:        none
**
**  Side effects:   target is changed by the contents of the source buffer
**
**  History:
**
**********************************************************************FDOCEND**/
void BufferSplit (unsigned int *source, unsigned int *target, size_t size)
{
	/* debug checks */
	GMI_ASSERT(source != NULL);
	GMI_ASSERT(target != NULL);
	GMI_ASSERT(size > 0);

	/* expand contents of source buffer to target buffer */
	while (size-- > 0)
	{
		/* first copy highbyte, */
		*target++ = *source >> 8;
		/* then lowbyte */
		*target++ = *source++ & 0xff;
	}

	return;
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

