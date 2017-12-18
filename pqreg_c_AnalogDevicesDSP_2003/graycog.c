/*______________________________________________________________________________

    	$Id: isqrt.c,v 1.1 2002/04/24 08:11:43 manuel Exp $

		Module	: graycog
        Function: center of gravity calculation from grayscale image

    	Language: ISO C
	    System	: VC/RT

    	Author	: Manuel Jochem
    	          FiberVision GmbH
    	          Jens-Otto-Krag Str. 11
    	          D-52146 Wuerselen
    	          Tel.: +49 (0) 2405 / 4548-21
    	          Fax : +49 (0) 2405 / 4548-14

		(C) FiberVision GmbH, 2003
  ______________________________________________________________________________
*/

#include <stdlib.h>	/* abs */
#include <vcrt.h>
#include <vclib.h>
#include <vcdefs.h>

#include "global.h"
#include "config.h"
#include "pqlib.h"

#ifndef MAXGRAY
#	define MAXGRAY 255
#endif

/*______________________________________________________________________________

		RectToImage
  ______________________________________________________________________________
*/

void RectToImage(image* pParent, ftr* pRectangle, image* pTarget)
{
	pTarget->st    = pParent->st + (long) pRectangle->y_min * (long) pParent->pitch
	                             + (long) pRectangle->x_min;
	pTarget->dx    = pRectangle->x_max - pRectangle->x_min + 1;
	pTarget->dy    = pRectangle->y_max - pRectangle->y_min + 1;
	pTarget->pitch = pParent->pitch;
}

/*______________________________________________________________________________

		RectOffsetFromImage
  ______________________________________________________________________________
*/

void RectOffsetFromImage(image* pParent, image* pSource, int* Xoffset, int* Yoffset)
{
	long AddressOffset = pSource->st - pParent->st;
	long PitchAsLong   = (long) pSource->pitch;

	*Xoffset = (int)(AddressOffset % PitchAsLong);
	*Yoffset = (int)(AddressOffset / PitchAsLong);
}

/*______________________________________________________________________________

		GetXweight
  ______________________________________________________________________________
*/

static long GetXweight(int count, int* buffer)
{
	int i;
	long Xweight = 0L;

	for(i = 1; i <= count; i++)
	{
		Xweight += (long)(i * (MAXGRAY - *buffer++));
	}

	return Xweight;
}

/*______________________________________________________________________________

		Myaddlf
  ______________________________________________________________________________
*/

static unsigned int Myaddlf(int count, int* buffer)
{
	int i;
	unsigned int sum = 0u;

	for(i = 0; i < count; i++)
	{
		sum += (unsigned int)(MAXGRAY - *buffer++);
	}

	return sum;
}

/*______________________________________________________________________________

		GetGrayscaleCentroid
  ______________________________________________________________________________
*/

void GetGrayscaleCentroid (image* roi, int* x, int* y, int scale)
{
	long ThisArea= 0L;
	long Area    = 0L;
	long Area2   = 0L;
	long Xweight = 0L;
	long Yweight = 0L;

	long addr  = roi->st >> 1;
	long pitch = (long) (roi->pitch / 2);
	int  dx    = roi->dx;
	int  dy    = roi->dy;
	int  dx2   = dx / 2;

	int* buffer = (int*) vcmalloc(dx * sizeof(int));/* internal line buffer  */
	if (buffer)
	{
		int i;

		for(i = 1; i <= dy; i++)
		{
			blrdb(dx2,buffer,addr);
			addr  += pitch;

			ThisArea = (long) Myaddlf(dx, buffer);
			Area    += ThisArea;
			Yweight += ThisArea * (long) i;
			Xweight += GetXweight(dx, buffer);
		}
		vcfree(buffer);

		Area2 = Area >> 1;		/* for rounding purposes */

		Xweight *= (long) scale;
		Yweight *= (long) scale;

		*x = (int)((Xweight + Area2) / Area);
		*y = (int)((Yweight + Area2) / Area);
		*y -= scale; 			/* empiric value */
		*x -= scale; 			/* empiric value */
	}
}

/*______________________________________________________________________________

		FindCOG
  ______________________________________________________________________________
*/

void FindCOG(image* parent, ftr* curFeature, int size)
{
	int x = 0;
	int y = 0;
	int Xoffset = 0;
	int Yoffset = 0;
	image obj;
	long scale  = SCALE_FACTOR;
	long scale2 = scale >> 1;
	long size2  = ((long) size * scale) >> 1;
	bool align  = FALSE;

	/* make a copy of the current candidate */
	ftr thisFeature = *curFeature;

	/* set up new bounding box */
	long x_long = thisFeature.x_center - size2;
	long y_long = thisFeature.y_center - size2;
	thisFeature.x_min = (int) ((x_long + scale2) / scale);
	thisFeature.y_min = (int) ((y_long + scale2) / scale);
	thisFeature.x_max = thisFeature.x_min + size;
	thisFeature.y_max = thisFeature.y_min + size;

	/* convert bounding box to image variable and check alignment */
	RectToImage(parent, &thisFeature, &obj);
	align = AlignRoi(&obj, TRUE);
	RectOffsetFromImage(parent, &obj, &Xoffset, &Yoffset);

	/* do the grayscale centroid calculation */
	GetGrayscaleCentroid (&obj, &x, &y, (int) scale);

	/* add scaled offset to ROI */
	x += (Xoffset * (int) scale);
	y += (Yoffset * (int) scale);

	SendDbgMsg(DBG_ERRORS, "COG %04d %04d %04d %04d %d %d %d %d",
		(int) curFeature->x_center, x, (int) curFeature->y_center, y,
		thisFeature.x_min, Xoffset, thisFeature.y_min, Yoffset);

	/* The grayscale centroid is very stable, but also error prone in
	   case of overlap/near overlap of marks. As a safeguard, we check,
	   if the new (grayscale) value varies too much from the old (binary)
	   one. If it does, we stay with the binary value */

	if(abs((int)curFeature->x_center - x) < 2 * scale)
	{
		curFeature->x_center = (long) x;
	}
	else
	{
		cprintf(Console, "COG: new x value ignored!");
	}

	if(abs((int)curFeature->y_center - y) < 2 * scale)
	{
		curFeature->y_center = (long) y;
	}
	else
	{
		cprintf(Console, "COG: new y value ignored!");
	}
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/


