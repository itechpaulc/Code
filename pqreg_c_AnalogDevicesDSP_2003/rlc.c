/**SDOC*************************************************************************
 *
 *  rlc.c - Handle Windows 3.x Run-Length-Encoded Bitmaps
 *
 *  Author: Mark Colvin					June 12, 2001 
    	Author	: Manuel Jochem
    	          FiberVision GmbH
    	          Jens-Otto-Krag Str. 11
    	          D-52146 Wuerselen
    	          Tel.: +49 (0) 2405 / 4548-21
    	          Fax : +49 (0) 2405 / 4548-14

		(C) FiberVision GmbH, 2000-2003
 *
 * $Header:   K:/Projects/PQCamera/PQCamera/GETIMAGE/rlc.c_v   1.3   Dec 30 2002 11:39:56   APeng  $
 *
 * $Log:   K:/Projects/PQCamera/PQCamera/GETIMAGE/rlc.c_v  $
 * 
 *    Rev 1.3   Dec 30 2002 11:39:56   APeng
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
 *    Rev 1.0   09 Jul 2001 12:39:32   MARKC
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

    	$Id: RLC.C,v 1.1 2000/09/12 14:39:52 Manuel Exp $

		Module	: RLE
        Function: Handle Windows 3.x Run-Length-Encoded Bitmaps
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

#include <vcrt.h>

#include "rlc.h"

const int ESC   = 0x00;
const int EOL   = 0x00;
const int EOB   = 0x01;
const int DELTA = 0x02;

/**FDOC*************************************************************************
 *
 *  Function:       rlcEncode
 *
 *  Author:         Mark Colvin             Date: June 12, 2001
 *
 *  Description:	run length encoding for one row of image buffer
 *
 *  Parameters:     Count - number of pixels to encode
 *					Thres - binarization threshold value
 *					*In - pointer to input buffer
 *					*Out - pointer to output buffer (has to be at least 2 * Count in size)
 *					Max - max number of repetitions
 *
 *  Returns:        number of encoded values
 *
 *  Side effects:   
 *
 *  History:
 *
 *********************************************************************FDOCEND**/
int rlcEncode (int Count, int Thres, int* In, int* Out, int Max)
{
    int Index   = 0;
    int Counter = 1;
    int Size    = 0;
    int This    = 0;
    int Last    = 0;

    if (Count < 2) return 0;

    Last = (*In > Thres ? *In : 0);
    In++;

    for (Index = 1; Index < Count; Index++)
    {
        This = (*In > Thres ? *In : 0);
        In++;

        if (This == Last)
        {
            if (Counter < Max)
            {
                Counter++;
            }
            else
            {
                *Out++  = Counter;
                *Out++  = Last;

                Counter = 1;
                Size++;
            }
        }
        else
        {
            *Out++  = Counter;
            *Out++  = Last;

            Counter = 1;
            Size++;
        }

        Last = This;
    }
    *Out++  = Counter;
    *Out++  = Last;
    Size++;

    *Out++  = ESC;
    *Out++  = EOL;
    Size++;

    return 2 * Size;
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

