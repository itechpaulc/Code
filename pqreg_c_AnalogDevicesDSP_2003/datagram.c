/**SDOC*************************************************************************
**
**  datagram.c - send and recieve of PQ camera data packets
**
**  Author: Mark Colvin				6/12/01 2:10:41 PM

    	Author	: Manuel Jochem
    	          FiberVision GmbH
    	          Jens-Otto-Krag Str. 11
    	          D-52146 Wuerselen
    	          Tel.: +49 (0) 2405 / 4548-21
    	          Fax : +49 (0) 2405 / 4548-14

		(C) FiberVision GmbH, 2000-2003

**
** $Header:   K:/Projects/PQCamera/PQCamera/PQLIB/datagram.c_v   1.5   May 02 2003 10:31:48   APeng  $
**
** $Log:   K:/Projects/PQCamera/PQCamera/PQLIB/datagram.c_v  $
 * 
 *    Rev 1.5   May 02 2003 10:31:48   APeng
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

/* standard C headers*/

#include <stdio.h>
#include <time.h>
/* standard fibervision headers*/
#include <vcdefs.h>
#include <bool.h>

/* project library headers*/
#include "image.h"
#include "pqlib.h"
/* misc project headers*/
#include "config.h"
#include "global.h"
#include "error.h"
#include "bytes.h"
#include "comm.h"
/* module headers*/
#include "datagram.h"

GMI_ASSERTFILE(__FILE__);

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/
/*------ disable the check for CTS on sending serial data ------*/
/*#define NO_CTS
*/

/* definition of checksum update macros */
#define UpdateChecksum(a,b)	((b) + (a))
#define INIT_CHECKSUM 	0x0000u
#define MSG_OVERHEAD	8
typedef unsigned int chksum;

/* definitions for little endianess: */
#define MSB(a) ((unsigned char)((a) >> 8))
#define LSB(a) ((unsigned char)((a) & 0x00FF))

#define SET_MSB(b,a) (b |= (unsigned int)(a) << 8)
#define SET_LSB(b,a) (b |= (unsigned int)(a) & 0x00FF)

/*----------------------------------------------------------------------------*/
#define MAX_TRIALS 			3
#define TIME_OUT		2000L
#define HEADER_SIZE	    	5
#define USE_OVERLAY_CALLS

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/
PRIVATE BYTE Buffer[MSG_OVERHEAD];

/*===========================  F U N C T I O N S  ============================*/
/*______________________________________________________________________________

    	$Id: datagram.c,v 1.8 2000/09/28 10:30:19 Manuel Exp $

		Module	: Datagram
        Function: sending and receiving of data packages
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
**  Function:       SendDatagram
**
**  Author:         Mark Colvin             Date: 6/12/01 2:17:08 PM
**
**  Description:	Send data to serial port, handshake with other device.
**
**  Parameters:     token - message type
**					count - message length
**					data - pointer to message buffer
**					split - data buffer has to be split (16bit -> msb, lsb)
**
**  Returns:        SysErr -ERROR_NONE - no error, success
**							ERROR_SIO_RCV_TIMEOUT_ERROR - timeout waiting for response
**							ERROR_SIO_CTS_TIMEOUT_ERROR -
**							ERROR_SIO_FRAMING_ERROR -
**							ERROR_SIO_CHECKSUM_ERROR -
**							ERROR_SIO_MAXTRIALS_ERROR - retries exhausted, not working
**							ERROR_SIO_UNKNWON_TYPE_ERROR -
**							ERROR_SIO_MEMORY_ERROR -
**							ERROR_SIO_ESCAPE_ERROR -
**							ERROR_SIO_RCV_IN_PROGRESS_ERROR - got a char while xmit start
**							ERROR_SIO_SND_IN_PROGRESS_ERROR -
**							ERROR_SIO_MORE_THAN_ONE_UNREAD -
**							ERROR_SIO_ACK_EXPECTED - got a char other than ACK
**
**
**  Side effects:   none
**
**  History:		6/12/01 3:16:21 PM, mac added function header
**
**********************************************************************FDOCEND**/
ERROR SendDatagram (BYTE Token, WORD Count, BYTE* Data, bool Split)
{
#ifdef USE_OVERLAY_CALLS
	GLOBAL BYTE message_counter;
#else
	PRIVATE BYTE message_counter = 0;
#endif
	/* set up variables */
	ERROR   SysErr = ERROR_NONE;
	chksum  Check  = INIT_CHECKSUM;
	WORD FullCount = MSG_OVERHEAD + Count;
	BYTE Response  = NAK;
	BYTE Value     = 0;
	int TrialsLeft = MAX_TRIALS;
	int i;


	/* check for ongoing receive event */
	if(kbhit())
	{
		/* ERROR: useless to continue */
		return ERROR_SIO_RCV_IN_PROGRESS_ERROR;
	}
	else
	{
		/* reset RTS to prevent future receive events */
		resRTS();
		ClearInputBuffer()
	}

	/* are we sending packed data? */
	if(Split)
	{
		/* correct buffer size*/
		FullCount += Count;
	}

	/* set header data */
	Buffer[0] = STX;
	Buffer[1] = Token;
	Buffer[2] = message_counter;
	Buffer[3] = LSB(FullCount);
	Buffer[4] = MSB(FullCount);

	/* try sending packet MAX_TRIALS times */
	while(TrialsLeft-- && (Response == NAK) && !SysErr)
	{
		/* init checksum for each trial */
		Check = INIT_CHECKSUM;

        /* send header data */
		for(i = 0; (i < HEADER_SIZE) && (!SysErr); i++)
		{
			/* send each byte updating checksum */
			Value  = Buffer[i] & 0xFF;
			SysErr = SendByte(Value);
			Check  = UpdateChecksum(Value, Check);
		}

		if(SysErr)	/* CTS timeout? */
			break;	/* ERROR: useless to continue */


		/* send buffer data */
		for(i = 0; (i < Count) && (!SysErr); i++)
		{
			if (Split)	/* packed data? */
			{
				/* send high byte first */
				Value  = Data[i]  >> 8;
				SysErr = SendByte(Value);
				Check  = UpdateChecksum(Value, Check);
			}
			/* send low byte / only byte */
			Value  = Data[i] & 0xFF;
			SysErr = SendByte(Value);
			Check  = UpdateChecksum(Value, Check);
		}

		if(SysErr)	/* CTS timeout? */
			break;	/* ERROR: useless to continue */


		/* send checksum */
		SysErr = SendWord(Check);

		if(SysErr)	/* CTS timeout? */
			break;	/* ERROR: useless to continue */

		/* send trailer */
		SysErr = SendByte(ETX);
		if(SysErr)	/* CTS timeout? */
			break;	/* ERROR: useless to continue */

		/* allow receiving of ACK/NAK */
		setRTS();


		/* wait for answer from PC */
		SysErr = ReadByte(&Response);

		/* response? */
		if(!SysErr)
		{
			if(Response == ACK)/* ACK? */
			{
				/* done: exit */
				break;
			}
			else
			{
				/* unkown data */
				UnreadByte(Response);
				SysErr = ERROR_SIO_ACK_EXPECTED;
				/* no use trying to continue */
				break;
			}
		}
		else
		{
			/* read timeout error - assume NAK and try again */
			Response = NAK;
		}

		/* stop receiving */
		resRTS();
	}


	if(!SysErr)	/* everything fine? */
	{
		/* inkrement message count */
		if( message_counter < 0xff )
			message_counter++;
		else
			message_counter = 0;
	}
	else
	{
		/* still receive timeout? */
		if(SysErr == ERROR_SIO_RCV_TIMEOUT_ERROR)
		{
			/* no ACK after MAX_TRIALS attempts */
			SysErr = ERROR_SIO_MAXTRIALS_ERROR;
		}
		else
		{
			/* unkown error: ignore */
		}
	}

	setRTS();	/* ready to receive transmissions again */

	return SysErr;	/* return error code */
}

/**FDOC*************************************************************************
**
**  Function:       ReadDatagram
**
**  Author:         Mark Colvin             Date: 6/12/01 2:17:08 PM
**
**  Description:	Read data from serial port, handshake with other device.
**
**  Parameters:     token - message type
**					Seqnum - sequence number of the packet
**					count - message length
**					data - pointer to message buffer
**					Join - data buffer has to be packed (msb, lsb ->16bit)
**
**  Returns:        SysErr -ERROR_NONE - no error, success
**							ERROR_SIO_RCV_TIMEOUT_ERROR - timeout waiting for response
**							ERROR_SIO_CTS_TIMEOUT_ERROR - device timed out during char send
**							ERROR_SIO_FRAMING_ERROR - not a STX at msg start or ETX at end
**							ERROR_SIO_CHECKSUM_ERROR - calc'd chksum did not agree w/msg
**							ERROR_SIO_MAXTRIALS_ERROR - retries exhausted, not working
**							ERROR_SIO_UNKNWON_TYPE_ERROR -
**							ERROR_SIO_MEMORY_ERROR -
**							ERROR_SIO_ESCAPE_ERROR - got a ESC char at msg start
**							ERROR_SIO_RCV_IN_PROGRESS_ERROR - got a char while xmit start
**							ERROR_SIO_SND_IN_PROGRESS_ERROR - a char was sent during recv
**							ERROR_SIO_MORE_THAN_ONE_UNREAD - trying to unget empty buffer
**							ERROR_SIO_ACK_EXPECTED - got a char other than ACK
**
**
**  Side effects:   none
**
**  History:		6/12/01 3:16:21 PM, mac added function header
**
**********************************************************************FDOCEND**/
ERROR ReadDatagram (BYTE* Token, BYTE* SeqNum, WORD* Count, BYTE* Data, bool Join)
{
	ERROR SysErr = ERROR_NONE;
	chksum Check = INIT_CHECKSUM;
	chksum Ref   = INIT_CHECKSUM;
	BYTE Value   = 0;
	int i;

	GMI_ASSERT(Data);



	/* check for unwanted RS232 send event */
	if(nosend())
		return ERROR_SIO_SND_IN_PROGRESS_ERROR;

	for(i = 0; i < HEADER_SIZE; i++)
	{
		SysErr = ReadByte(Buffer + i);
		Check  = UpdateChecksum(Buffer[i], Check);
		if(SysErr) return SysErr;
	}


	SendDbgMsg(DBG_INFO, "REC: %02x %02x %02x %02x %02x",
		Buffer[0], Buffer[1], Buffer[2], Buffer[3], Buffer[4] );

	if(Buffer[0] == ESC) return ERROR_SIO_ESCAPE_ERROR;
	if(Buffer[0] != STX) return ERROR_SIO_FRAMING_ERROR;
	*Token  = Buffer[1];
	*SeqNum = Buffer[2];
	*Count  = 0;
	SET_LSB(*Count, Buffer[3]);
	SET_MSB(*Count, Buffer[4]);
	*Count -= MSG_OVERHEAD;


	if (Join)
		*Count /= 2;

	for(i = 0; i < *Count; i++)
	{
		SysErr = ReadByte(&Value);
		if(SysErr) return SysErr;
		Check  = UpdateChecksum(Value, Check);


		if (Join)
		{
			/* first copy highbyte, */
			Data[i] = Value << 8;

			SysErr = ReadByte(&Value);
			if(SysErr) return SysErr;
			Check  = UpdateChecksum(Value, Check);

			/* then lowbyte */
			Data[i] += Value & 0xff;
		}
		else
		{
			Data[i] = Value;
		}


	}

	SysErr = ReadWord(&Ref);
	if(SysErr) return SysErr;

	SysErr = ReadByte(Buffer + MSG_OVERHEAD - 1);
	if(SysErr) return SysErr;

	if (Buffer[MSG_OVERHEAD - 1] != ETX) return ERROR_SIO_FRAMING_ERROR;


	if (Ref != Check)
	{
		SysErr = SendByte(NAK);
		return ERROR_SIO_CHECKSUM_ERROR;
	}
	else
	{
		SysErr = SendByte(ACK);
		return SysErr;
	}
}

/**FDOC*************************************************************************
**
**  Function:       SendWord
**
**  Author:         Mark Colvin             Date: 6/12/01 3:31:14 PM
**
**  Description:	Sends 16bit data value, msw, lsw
**
**  Parameters:     Data - message data
**
**  Returns:        SysErr - (see above)
**
**  Side effects:   none
**
**  History:		6/12/01 3:33:36 PM, mac added function header
**
**********************************************************************FDOCEND**/
ERROR SendWord(unsigned int Data)
{
	ERROR SysErr;

	SysErr = SendByte(LSB(Data));
	if(!SysErr)
		SysErr = SendByte(MSB(Data));

	return SysErr;
}

/**FDOC*************************************************************************
**
**  Function:       SendByte
**
**  Author:         Mark Colvin             Date: 6/12/01 3:31:14 PM
**
**  Description:	Sends 8bit data value
**
**  Parameters:     Data - message data
**
**  Returns:        SysErr - (see above)
**
**  Side effects:   none
**
**  History:		6/12/01 3:33:36 PM, mac added function header
**
**********************************************************************FDOCEND**/
ERROR SendByte(BYTE Data)
{
#ifndef NO_CTS
	/* preset timeout */
	time_t Timeout = time(NULL) + TIME_OUT;

	/* check for CTS -reversed- */
	while(CommCtsSet())

	/* check for CTS */
/*	while(CommCtsCleared())
*/
		if(time(NULL) > Timeout)
			return ERROR_SIO_CTS_TIMEOUT_ERROR;
#endif
	/* send data */
	putchar((int) Data);

	return ERROR_NONE;
}

/**FDOC*************************************************************************
**
**  Function:       ReadWord
**
**  Author:         Mark Colvin             Date: 6/12/01 3:31:14 PM
**
**  Description:	Receives bit data value, msw, lsw
**
**  Parameters:     Data - pointer to message data
**
**  Returns:        SysErr - (see above)
**
**  Side effects:   none
**
**  History:		6/12/01 3:33:36 PM, mac added function header
**
**********************************************************************FDOCEND**/
ERROR ReadWord(unsigned int* Data)
{
	int Byte;
	ERROR SysErr;
	*Data = 0;

	SysErr = ReadByte(&Byte);
	if(SysErr) return SysErr;
	SET_LSB(*Data, Byte);

	SysErr = ReadByte(&Byte);
	if(SysErr) return SysErr;
	SET_MSB(*Data, Byte);

	return SysErr;
}

/**FDOC*************************************************************************
**
**  Function:       ReadByte
**
**  Author:         Mark Colvin             Date: 6/12/01 3:31:14 PM
**
**  Description:	Receives 8bit data value
**
**  Parameters:     Data - pointer to message data
**
**  Returns:        SysErr - (see above)
**
**  Side effects:   none
**
**  History:		6/12/01 3:33:36 PM, mac added function header
**
**********************************************************************FDOCEND**/
ERROR ReadByte(BYTE* Data)
{
	if(UngetFlag)
	{
		/* if there is an "unread" character waiting, we take that */
		*Data = UngetChar;
		UngetFlag = FALSE;
	}
	else
	{
		/* preset timeout */
		time_t Timeout = time(NULL) + TIME_OUT;

		/* check for incomming data */
		while(!kbhit())
			if(time(NULL) > Timeout)
				return ERROR_SIO_RCV_TIMEOUT_ERROR;

		/* read data */
		*Data = (BYTE) getchar();
	}
	return ERROR_NONE;
}

/**FDOC*************************************************************************
**
**  Function:       UnreadByte
**
**  Author:         Mark Colvin             Date: 6/12/01 3:31:14 PM
**
**  Description:	Pushes 8bit data value back into recieve queue.
**
**  Parameters:     Data - message data
**
**  Returns:        SysErr - (see above)
**
**  Side effects:   none
**
**  History:		6/12/01 3:33:36 PM, mac added function header
**
**********************************************************************FDOCEND**/
ERROR UnreadByte(BYTE Data)
{
	if(!UngetFlag)
	{
		/* save character to buffer and make a note you did */
		UngetChar = Data;
		UngetFlag = TRUE;
		return ERROR_NONE;
	}
	else
	{
		/*  there is already an "unread" character in the buffer */
		return ERROR_SIO_MORE_THAN_ONE_UNREAD;
	}
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

