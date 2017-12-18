/**SDOC*************************************************************************
 *
 *  statemac.c - main state machine of the Printquick system
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
 * $Header:   K:/Projects/PQCamera/PQCamera/MANAGER/statemac.c_v   1.3   Dec 30 2002 11:52:10   APeng  $
 *
 * $Log:   K:/Projects/PQCamera/PQCamera/MANAGER/statemac.c_v  $
 * 
 *    Rev 1.3   Dec 30 2002 11:52:10   APeng
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
 *    Rev 1.0   09 Jul 2001 12:39:54   MARKC
 * Checked in from initial workfile by PVCS Version Manager Project Assistant.
 *
 *
 *********************************************************************SDOCEND**/

/*=======================  I N C L U D E   F I L E S  ========================*/

#include <string.h>	/* strlen */
#include <stdio.h>
#include <vcdefs.h>
#include <bool.h>
#include <vcrt.h>
#include <vclib.h>
#include <cam0.h>

#include "config.h"
#include "statemac.h"
#include "global.h"
#include "command.h"
#include "error.h"
#include "build.h"
#include "image.h"

#include "pqlib.h"

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

/*===========================  F U N C T I O N S  ============================*/

/*______________________________________________________________________________

    	$Id: statemac.c,v 1.6 2000/09/28 10:30:18 Manuel Exp $

		Module	: PQ-Manager
        Function: state machine for PQ system
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

/*----------------------------------------------------------------------------*/

PRIVATE char* Message = "PQ-manager [build: "__BUILD__"]  "__DATE__"  "__TIME__;
PRIVATE char* Bootup  = "PrintQuick Cam(c)2000 GMI";

/**FDOC*************************************************************************
 *
 *  Function:       PqStateMachine
 *
 *  Author:         Mark Colvin             Date: June 12, 2001
 *
 *  Description:	main processing loop
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
int PqStateMachine(void)
{
	/* input data buffer: factor 2 because of pack/unpack issue */
	BYTE Buffer[2 * MAX_PACKET_LENGTH];

	bool  Exit  = FALSE;
	ERROR Error = ERROR_NONE;
	WORD  Count = 0;
	BYTE RecSeq = 0;
	PQ_CMD  Status = PQ_INIT;
	PQ_CFG* Config = NULL;

	/* send boot message on program start up */
	Error  = SendDatagram (CMD_BOOT_MESSAGE, strlen(Bootup), Bootup, RAW);
	if (Error)
		SendDbgMsg(DBG_ERRORS, "ERROR 11: communication error [code: %02X]", Error);

	SendDbgMsg(DBG_ERRORS, "S/N: %ld", GetSerialNumber());

	/* read configuration on startup */
	Error  = exec(MODULE_CONFIG, OVL_CALL, PQ_INIT, &Config, Buffer, Count);

	/* default to idle after boot message */
	Status = PQ_IDLE ;

	while(!Exit)
	{
		switch(Status)
		{
		  case CMD_EXIT_PROGRAM:   			/* call CONFIG module for clean up */
		    Exit  = TRUE;
		    nobreak;
		  case CMD_REQUEST_VERSION:			/* request firmware version */
		  case CMD_LOAD_SYS_CFG:			/* load system configuration      */
		  case CMD_LOAD_FRM_CFG:			/* load form configuration        */
		  case CMD_REQUEST_STATUS:          /* send status and results 		  */
		  case CMD_SEND_SYS_CFG:			/* send form configuration        */
		  case CMD_UPDATE_FLASH_COUNT:		/* update flash counter           */
		  case CMD_LOAD_PATTERNS:			/* load new target pattern info   */
		  case CMD_LOAD_SHUTTER:			/* load shutter value             */
		  case CMD_START_DEBUG:				/* set debug level                */
		  case CMD_STOP_DEBUG:				/* clear debug level              */
		  case CMD_LOAD_EEPROM:
		  case CMD_SAVE_EEPROM:
		  case CMD_CENTER_ROI:
		    /* all configuration related commands are handled by the CONFIG module */
			Error  = exec(MODULE_CONFIG, OVL_CALL, Status, &Config, Buffer, Count);
			Status = Error ? PQ_RECOVER : PQ_IDLE;
		    break;

		  case CMD_START_HUNTING:
			SendDbgMsg(DBG_INFO, "OK: start scanning (hunter)");
			message_counter = 0;
			Error  = exec(MODULE_HUNTER, OVL_CALL, Config, Buffer);
			Status = Error ? PQ_RECOVER : CMD_STOP_SCANNING;
		    break;

		  case CMD_START_TRACKING:
			SendDbgMsg(DBG_INFO, "OK: start scanning (tracker)");
			message_counter = 0;
			Error  = exec(MODULE_TRACKER, OVL_CALL, Config, Buffer);
			Status = Error ? PQ_RECOVER : CMD_STOP_SCANNING;
		    break;

		  case CMD_STOP_SCANNING:	/* direct command obsolete - only internal state */
			SendDbgMsg(DBG_INFO, "OK: stop scanning");
			Status = CMD_SEND_SYS_CFG;	/* send system config after scanning */
		    break;

		  case CMD_SEND_IMAGE:
			Error  = exec(MODULE_IMAGE, OVL_CALL, Config);
			Status = Error ? PQ_RECOVER : PQ_IDLE;
		    break;

		  case PQ_IDLE:
			SendDbgMsg(DBG_INFO, Message);	/* prompt */
			setPLC0();				/* camera ready: on */
			setRTS();				/* we are ready to communicate */
			while(!kbhit()) 		/* wait here for communicaton event */
				nop;
			resRTS();				/* not ready to communicate */
			resPLC0();				/* camera ready: off */
			Error  = ReadDatagram (&Status, &RecSeq, &Count, Buffer, UNKNOWN);
/*			if(Count > 0)
				SendDbgMsg(DBG_DEBUG, "OK: rcv data [cmd: %02X, seq: %02X, cnt: %02X, buf: %02X]", Status, RecSeq, Count, *Buffer);
			else
				SendDbgMsg(DBG_DEBUG, "OK: rcv data [cmd: %02X, seq: %02X, cnt: %02X, buf: n/a]", Status, RecSeq, Count);
*/			Status = Error ? PQ_RECOVER : Status;
		    break;

		  case PQ_RECOVER:
			SendDbgMsg(DBG_WARNINGS, "WARNING: trying to recover");
			ClearInputBuffer();
			Error  = ERROR_NONE;
			Status = PQ_IDLE;
		    break;

		  default:
			SendDbgMsg(DBG_WARNINGS, "WARNING: unknown token - ignored");
			ClearInputBuffer();
			Status = PQ_IDLE;
		    break;
		}
		if (Error)
			SendDbgMsg(DBG_ERRORS, "ERROR 12: command failed [code: %02X]", Error);
	}

	SendDbgMsg(DBG_WARNINGS, "WARNING: exiting program [error code: %02X]", Error);
	return Error;
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

