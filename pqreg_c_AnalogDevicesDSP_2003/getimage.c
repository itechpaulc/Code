/**SDOC*************************************************************************
 *
 *  getimage.c - sends camera image to CCU, using JPEG or Windows Bitmap formats
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
 * $Header:   K:/Projects/PQCamera/PQCamera/GETIMAGE/getimage.c_v   1.4   May 02 2003 10:30:42   APeng  $
 *
 * $Log:   K:/Projects/PQCamera/PQCamera/GETIMAGE/getimage.c_v  $
 * 
 *    Rev 1.4   May 02 2003 10:30:42   APeng
 *  
 *
 *    Rev 1.3   Dec 30 2002 11:39:46   APeng
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
 *    Rev 1.2   Dec 10 2002 15:59:06   APeng
 * some work has been done to the reduced
 * ROI
 *
 *    Rev 1.0   09 Jul 2001 12:39:30   MARKC
 * Checked in from initial workfile by PVCS Version Manager Project Assistant.
 *
 *
 *********************************************************************SDOCEND**/

/*=======================  I N C L U D E   F I L E S  ========================*/
#include <stdio.h>
#include <vcdefs.h>
#include <cam0.h>
#include <vcrt.h>
#include <vclib.h>
#include <bool.h>

#include <time.h>

#include "config.h"
#include "error.h"
#include "global.h"
#include "image.h"
#include "pqlib.h"
#include "build.h"

#include "command.h"
#include "bmp.h"
#include "rlc.h"
#include "getimage.h"
#include "datagram.h"

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/
/*==================  E X T E R N A L   V A R I A B L E S  ===================*/
/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/
/*================  I N T E R N A L   D E F I N I T I O N S  =================*/
/*==================  I N T E R N A L   V A R I A B L E S  ===================*/
/*===========================  F U N C T I O N S  ============================*/

/*______________________________________________________________________________

    	$Id: getimage.c,v 1.4 2000/09/28 10:30:17 Manuel Exp $

		Module	: PQ-Image
        Function: routines for image download
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


/**FDOC*************************************************************************
 *
 *  Function:       main
 *
 *  Author:         Mark Colvin             Date: June 12, 2001
 *
 *  Description:	Entry point for the PQ Image module
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
int MAIN(Image)(int dummy, int argc, PQ_CFG* cfg)
{
    /* declare and initialize local variables */
	time_t Elapsed;
	ERROR Error = ERROR_NONE;
	char* BootMsg = "PQ-image   [build: "__BUILD__"]  "__DATE__"  "__TIME__;
    int  SnapPage = 0 * PICTURE;
    int  JpegPage = 3 * PICTURE;
    long Size     = 0L;
	image* source;
    image  target;

	/* show info message on screen */
	SendDbgMsg(DBG_INFO, BootMsg);


	/* react differently according to scanning mode */
	switch(cfg->bSearchMode)
	{
	  case SEARCH_IDLE:		/* in IDLE mode we have time to transfer full image */
	  case SEARCH_HUNT:
		source = &(cfg->FoV);
	    break;
	  case SEARCH_TRACK:
	  default:
		source = &(cfg->RoI);
	    break;
	}
	/* make sure global variables are not overwritten */
	UseGlobals();

	Elapsed = time(NULL);


    SnapPage = getvar (stpage);
    setvar (stpage, JpegPage);
    target   = ImageCreate (0, 0, getvar(hwidth) * 2, getvar(vwidth), IMG_DEFAULT);
    setvar (stpage, SnapPage);


	switch (cfg->bImageType)
	{
	  case IMAGE_JPG:			/* 'jpg'-Image anfordern */
		SendDbgMsg(DBG_INFO, "OK: send JPEG image @ %d", cfg->bImageLevel);

		Size = __jpeg(source, &target, cfg->bImageLevel);
		message_counter = 0;
		Error = SendJPG (&target, Size);

		break;
	  case IMAGE_BMP:         /* 'bmp'-Image anfordern */
		SendDbgMsg(DBG_INFO, "OK: send BMP image");

		message_counter = 0;
		Error = SendBMP (source);

		break;
	  case IMAGE_RLE:      	/* 'rle'-Image anfordern */
		SendDbgMsg(DBG_INFO, "OK: send RLE image");

		message_counter = 0;
		SendRLE (source);

		break;
	  default:                      	/* ignorieren */
		SendDbgMsg(DBG_ERRORS, "ERROR 07: unknown image type [%d]", cfg->bImageType);
		Error = ERROR_UNKNOWN_IMAGE_TYPE;

		break;
	}

	Elapsed = time(NULL) - Elapsed;

	if(!Error)
		SendDbgMsg(DBG_INFO, "OK: image transfered in %ld ms", Elapsed);
	else
		SendDbgMsg(DBG_ERRORS, "ERROR 08: image transfer failed, error %d", Error);


	/* return error code */
	return Error;
}

/**FDOC*************************************************************************
 *
 *  Function:       _jpeg
 *
 *  Author:         Mark Colvin             Date: June 12, 2001
 *
 *  Description:	process the source image with jpeg encode function, write
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
long __jpeg (image* source, image* target, int quality)
{
	long Size = 0L;

    if ((quality > 0) && (quality < 100))
    {
        long MaxSize   = (long) target->dy * (long) target->pitch;
        long StartAddr = target->st;
        long NextAddr  = cjpeg_d(source, quality, StartAddr, MaxSize);

		if(NextAddr > StartAddr)
		{
	        Size = NextAddr - StartAddr;
		}
		else
		{
			SendDbgMsg(DBG_ERRORS, "WARNING: max size (%ld) for jpeg code exceeded!", MaxSize);
		}
    }

	return Size;
}

/**FDOC*************************************************************************
 *
 *  Function:       SendJPG
 *
 *  Author:         Mark Colvin             Date: June 12, 2001
 *
 *  Description:	Transmit current image in JPEG format
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
ERROR SendJPG (image *img, long Length)
{
    long Address = img->st / 2L;
    long Step    = (long)(BUFFER_SIZE / 2);
    int  Chunks  = (int) (Length / (long) BUFFER_SIZE);
    int  Rest    = (int) (Length % (long) BUFFER_SIZE) + 1;    /* +1: Empirie */
    int* RawBuf  = (int*) vcmalloc ((unsigned int) BUFFER_SIZE);
    ERROR Error = ERROR_NONE;

    if (RawBuf == NULL) return ERROR_MEMORY_OVERFLOW;     /* kein Speicher */

    while (Chunks--)
    {
        blrdb ((int) Step, RawBuf, Address);
        Address += Step;
		Error = SendDatagram (CMD_IMAGE_MESSAGE, BUFFER_SIZE, RawBuf, RAW);
        if (Error)
		{
			cprintf(Console,"ERROR: error %d while sending chunks", Error);
			break;
		}
    }

    if (!Error)
	{
		/* Rest senden */
		blrdb ((int) Step, RawBuf, Address);
		Error = SendDatagram (CMD_IMAGE_MESSAGE, Rest, RawBuf, RAW);
        if (Error)
			cprintf(Console,"ERROR: error %d while sending rest", Error);
	}


    if (!Error)
	{
		/* Fertig-Signal senden */
        Error = SendDatagram (CMD_IMAGE_MESSAGE, 0, NULL, RAW);
        if (Error)
			cprintf(Console,"ERROR: error %d while sending ready signal", Error);
	}

    vcfree (RawBuf);
    return Error;
}

/**FDOC*************************************************************************
 *
 *  Function:       SendBMP
 *
 *  Author:         Mark Colvin             Date: June 12, 2001
 *
 *  Description:	Send current image in Windows 3.x bitmap format
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
ERROR SendBMP (image *img)
{
    ERROR  Error = ERROR_NONE;
	BMPHEADER* Bmp   = BmpCreateHeader(img, BI_RGB);
	if (Bmp)
	{
		Error = SendDatagram (CMD_IMAGE_MESSAGE, sizeof(BITMAPFILEHEADER), (BYTE*) &(Bmp->bf), SPLIT);
		Error = SendDatagram (CMD_IMAGE_MESSAGE, sizeof(BITMAPINFOHEADER), (BYTE*) &(Bmp->bi), SPLIT);
		Error = SendDatagram (CMD_IMAGE_MESSAGE, sizeof(RGBQUAD) * COLORS, (BYTE*) &(Bmp->ct), RAW);
		if (!Error)
		{
		    int  Cols    = ((img->dx + 3) / 4) * 4;	/* with padding*/
		    int  Rows    = img->dy;
		    long Step    = (long) img->pitch / 2L;
		    long Address = img->st / 2L + (long) (Rows - 1) * Step;
		    int* Buffer  = (int*) vcmalloc ((unsigned int) Cols);

		    if (Buffer)
			{

				while (Rows--)
				{
					blrdb (Cols / 2, Buffer, Address);
					Address -= Step;
					Error = SendDatagram (CMD_IMAGE_MESSAGE, Cols, Buffer, RAW);

					if (Error)
						break;
				}
				vcfree (Buffer);
			}
		}

		BmpDestroyHeader(Bmp);


		if (!Error)
		{
			/* Fertig-Signal senden */
			Error = SendDatagram (CMD_IMAGE_MESSAGE, 0, NULL, RAW);
		}
	}

	return Error;
}

/**FDOC*************************************************************************
 *
 *  Function:       SendRLE
 *
 *  Author:         Mark Colvin             Date: June 12, 2001
 *
 *  Description:	Send current image in Windows 3.x bitmap (RLE)
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
ERROR SendRLE (image *img)
{
    ERROR  Error = ERROR_NONE;
	BMPHEADER* Bmp   = BmpCreateHeader(img, BI_RLE8);
	if (Bmp)
	{

		Error = SendDatagram (CMD_IMAGE_MESSAGE, sizeof(BITMAPFILEHEADER), (BYTE*) &(Bmp->bf), SPLIT);
		Error = SendDatagram (CMD_IMAGE_MESSAGE, sizeof(BITMAPINFOHEADER), (BYTE*) &(Bmp->bi), SPLIT);
		Error = SendDatagram (CMD_IMAGE_MESSAGE, sizeof(RGBQUAD) * COLORS, (BYTE*) &(Bmp->ct), RAW);
		if (!Error)
		{
		    int  Cols    = ((img->dx + 3) / 4) * 4;	/* with padding*/
		    int  Rows    = img->dy;
			long Step    = (long)(img->pitch) / 2L;
		    long Address = img->st / 2L + (long) (Rows - 1) * Step;

			int  Size = 0;

			int* RawBuf  = (int*) vcmalloc ((unsigned int) img->dx);
			int* RlcBuf  = (int*) vcmalloc ((unsigned int)(Cols * 2));

			if (RawBuf && RlcBuf)
			{

				while (Rows--)
				{
					blrdb (Cols / 2, RawBuf, Address);
					Address -= Step;

					Size = rlcEncode (Cols, 16, RawBuf, RlcBuf, 255);
					Error = SendDatagram (CMD_IMAGE_MESSAGE, Size, RlcBuf, RAW);

					if (Error)
						break;
				}
				RlcBuf[0] = 0; /* ESC */
				RlcBuf[1] = 1; /* EOB */

				Error = SendDatagram (CMD_IMAGE_MESSAGE, 2, RlcBuf, RAW);
			}
			vcfree (RlcBuf);
			vcfree (RawBuf);
		}

		BmpDestroyHeader(Bmp);


		if (!Error)
		{
			/* Fertig-Signal senden */
			Error = SendDatagram (CMD_IMAGE_MESSAGE, 0, NULL, RAW);
		}
	}

	return Error;
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/


