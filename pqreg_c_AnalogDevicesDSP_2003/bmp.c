/**SDOC*************************************************************************
**
**  bmp.c - functions for handling Windows 3.x bitmap image elements
**
**  Author: Mark Colvin			6/12/01 11:06:10 AM

    	Author	: Manuel Jochem
    	          FiberVision GmbH
    	          Jens-Otto-Krag Str. 11
    	          D-52146 Wuerselen
    	          Tel.: +49 (0) 2405 / 4548-21
    	          Fax : +49 (0) 2405 / 4548-14

		(C) FiberVision GmbH, 2000-2003

**
** $Header:   K:/Projects/PQCamera/PQCamera/GETIMAGE/bmp.c_v   1.3   Dec 30 2002 11:39:40   APeng  $
**
** $Log:   K:/Projects/PQCamera/PQCamera/GETIMAGE/bmp.c_v  $
 * 
 *    Rev 1.3   Dec 30 2002 11:39:40   APeng
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
 *    Rev 1.2   Dec 10 2002 15:59:04   APeng
 * some work has been done to the reduced
 * ROI
 *
 *    Rev 1.0   09 Jul 2001 12:39:28   MARKC
 * Checked in from initial workfile by PVCS Version Manager Project Assistant.
**
**
**********************************************************************SDOCEND**/

/*=======================  I N C L U D E   F I L E S  ========================*/
#include <vclib.h>	/* image */
#include <stdio.h>	/* size_t */
#include "bmp.h"

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/
static DWORD BmpSwapLong (DWORD Dword);
static WORD  BmpSwapWord (WORD  Word);

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

/*===========================  F U N C T I O N S  ============================*/
/*______________________________________________________________________________

    	$Id: BMP.C,v 1.2 2000/09/21 16:58:41 Manuel Exp $

		Module	: Bitmap
        Function: Handle Windows 3.x Bitmaps
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
**  Function:       BmpSetFileHeader
**
**  Author:         Mark Colvin             Date: 6/12/01 11:07:47 AM
**
**  Description:	Initializes the bitmap header element
**
**  Parameters:     FileHeader - pointer to bitmap file struct
**
**  Returns:        none
**
**  Side effects:   initializes struct FileHeader
**
**  History:		6/12/01 11:09:33 AM, mac	added function header
**
**********************************************************************FDOCEND**/
void BmpSetFileHeader(BITMAPFILEHEADER *FileHeader)
{
    FileHeader->bfType      = BmpSwapWord(BMP_ID);
    FileHeader->bfSize      = BmpSwapLong(0L);	/* 0 only for uncompressed images */
    FileHeader->bfReserved1 = BmpSwapWord(0);
    FileHeader->bfReserved2 = BmpSwapWord(0);
    FileHeader->bfOffBits   = BmpSwapLong(SIZEOF(BITMAPFILEHEADER) +
										  SIZEOF(BITMAPINFOHEADER) +
										  sizeof(RGBQUAD) * COLORS);
}


/**FDOC*************************************************************************
**
**  Function:       BmpSetInfoHeader
**
**  Author:         Mark Colvin             Date: 6/12/01 11:12:18 AM
**
**  Description:	Initializes the bitmap information element
**
**  Parameters:     InfoHeader - pointer to bitmap info struct
**					Window - pointer to camera image struct
**					Compression - image compression scheme
**
**  Returns:		none
**
**  Side effects:   initializes struct InfoHeader
**
**  History:		6/12/01 11:16:13 AM, mac - added function header
**
**********************************************************************FDOCEND**/
void BmpSetInfoHeader(BITMAPINFOHEADER *InfoHeader, image *Window, DWORD Compression)
{
    InfoHeader->biSize          = BmpSwapLong((DWORD) SIZEOF(BITMAPINFOHEADER));
    InfoHeader->biWidth			= (LONG) BmpSwapLong((DWORD) Window->dx);
    InfoHeader->biHeight		= (LONG) BmpSwapLong((DWORD) Window->dy);
    InfoHeader->biPlanes        = BmpSwapWord(1);
    InfoHeader->biBitCount		= BmpSwapWord(8);
    InfoHeader->biCompression	= BmpSwapLong((DWORD) Compression);	/* only uncompressed images */
    InfoHeader->biSizeImage		= BmpSwapLong((DWORD) Window->dx * (DWORD) Window->dy);
    InfoHeader->biXPelsPerMeter = (LONG) BmpSwapLong(0L);
    InfoHeader->biYPelsPerMeter = (LONG) BmpSwapLong(0L);
    InfoHeader->biClrUsed		= BmpSwapLong((DWORD) COLORS);
    InfoHeader->biClrImportant  = BmpSwapLong(0L);		/* all colors are important */
}

/**FDOC*************************************************************************
**
**  Function:       BmpSetInfoHeader
**
**  Author:         Mark Colvin             Date: 6/12/01 11:17:17 AM
**
**  Description:	initializes the bitmp color palette table
**
**  Parameters:     ColorTable - pointer to bitmap color table struct
**
**  Returns:        none
**
**  Side effects:   initializes the struct ColorTable
**
**  History:		6/12/01 11:18:52 AM, mac	added function header
**
**********************************************************************FDOCEND**/
void BmpSetPalette(RGBQUAD *ColorTable)
{
	register int i = 0;

	for (i = 0; i < COLORS; i++)		/* 256 colors, grey scale */
	{
	    ColorTable[i].rgbBlue	  = (BYTE) i;	/* same value for each plane */
        ColorTable[i].rgbGreen	  = (BYTE) i;
        ColorTable[i].rgbRed	  = (BYTE) i;
        ColorTable[i].rgbReserved = (BYTE) 0;
	}
}

/**FDOC*************************************************************************
**
**  Function:       BmpCreateHeader
**
**  Author:         Mark Colvin             Date: 6/12/01 11:21:36 AM
**
**  Description:	creates buffer for the bitmap header in the heap, then
**					initializes the 3 elements.
**
**  Parameters:     Window - pointer to camera image struct
**					Compression - image compression scheme
**
**  Returns:        Header - pointer to buffer of bitmap header in the heap
**
**  Side effects:   requires BMPHEADER of the heap, NULL if not enough heap
**
**  History:		6/12/01 11:25:28 AM, mac	added function header
**
**********************************************************************FDOCEND**/
BMPHEADER* BmpCreateHeader(image *Window, DWORD Compression)
{
	BMPHEADER* Header = NULL;

	Header = (BMPHEADER*) vcmalloc (sizeof(BMPHEADER));
	if (Header)
	{
		BmpSetFileHeader(&(Header->bf));
		BmpSetInfoHeader(&(Header->bi), Window, Compression);
		BmpSetPalette(Header->ct);
	}
	return Header;
}

/**FDOC*************************************************************************
**
**  Function:       BmpDestroyHeader
**
**  Author:         Mark Colvin             Date: 6/12/01 11:26:50 AM
**
**  Description:	frees the bitmap header buffer from the heap
**
**  Parameters:     Header - pointer the bitmap header buffer in the heap
**
**  Returns:        none
**
**  Side effects:   returns the buffer area to the heap
**
**  History:		6/12/01 11:28:22 AM, mac	added function header
**
**********************************************************************FDOCEND**/
void BmpDestroyHeader(BMPHEADER* Header)
{
	vcfree((void*) Header);
}

/**FDOC*************************************************************************
**
**  Function:       BmpSwapLong
**
**  Author:         Mark Colvin             Date:  6/12/01 11:29:08 AM
**
**  Description:	swaps upper and lower 16 bits of a 32 value (DWORD)
**
**  Parameters:     Dword - value to be swapped
**
**  Returns:        Dword - swapped value
**
**  Side effects:   none
**
**  History:		6/12/01 11:31:22 AM, mac	added function header
**
**********************************************************************FDOCEND**/
static DWORD BmpSwapLong(DWORD Dword)
{
	WORD HiWord = (WORD)(Dword >> 16);
	WORD LoWord = (WORD)(Dword & 0xFFFF);

	return ((DWORD)BmpSwapWord(LoWord) << 16) + (DWORD)BmpSwapWord(HiWord);
}

/**FDOC*************************************************************************
**
**  Function:       BmpSwapWord
**
**  Author:         Mark Colvin             Date: 6/12/01 11:32:06 AM
**
**  Description:	swaps upper and lower 8 bits of a 16 value (WORD)
**
**  Parameters:     Word - value to be swapped
**
**  Returns:        Word - swapped value
**
**  Side effects:   none
**
**  History:		6/12/01 11:33:14 AM, mac	added function header
**
**********************************************************************FDOCEND**/
static WORD BmpSwapWord(WORD Word)
{
	return ((Word << 8) & 0xFF00) + ((Word >> 8) & 0x00FF);
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/


