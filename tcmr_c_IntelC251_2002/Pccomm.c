



/*
 *
 *
 *		$Header:   K:/Projects/Tcmr/Source/Pccomm.c_v   1.3   11 Jul 2003 14:48:26   PaulLC  $
 *		$Log:   K:/Projects/Tcmr/Source/Pccomm.c_v  $
 * 
 *    Rev 1.3   11 Jul 2003 14:48:26   PaulLC
 * Incorporated all changes since 0.60.B; Camera Trigger jumping fixes; 
 * Encoder noise filtering; Changes to supporte latest TCMRC HW; Limit switch configure.
 * 
 *    Rev 1.2   Apr 08 2002 14:31:42   PaulLC
 * Contains all of the changes made during beta development.
 * 
 *    Rev 1.1   Aug 27 2001 15:48:24   PaulLC
 * Changes made up to Engineering Version 00.25.A.
 * 
 *    Rev 1.0   Oct 06 2000 14:27:24   PaulLC
 * Checked in from initial workfile by PVCS Version Manager Project Assistant.
 * 
 *
 *		Author : Paul Calinawan
 *
 *			May 2000
 *
 *			Graphics Microsystems Inc
 *			1284 Forgewood Ave
 *			Sunnyvale, CA 94089
 *
 *			(408) 745-7745
 *
 *
 *		Print Quick Camera Control Module
 *	-------------------------------------------
 *
 *
*/

/////////////////////////////////////////////////////////////////////////////
//
//    NOTE:
//
//    This document contains CONFIDENTIAL and proprietary information
//    which is the property of Graphics Microsystems, Inc. It may not
//    be copied or transmitted in whole or in part by any means to any
//    media without Graphics Microsystems Inc's prior written permission.
//
/////////////////////////////////////////////////////////////////////////////



#include "kernel.h"

#include "pccomm.h"

#include "webenc.h"

#include "camsync.h"

#include "transp.h"

#include "pevmon.h"

#include "motorcs.h"



//////////////////////////////////////////////////
//
// Private Data
//
//////////////////////////////////////////////////

BYTE		pcMessageErrorLog;

#define		GET_PC_COMM_ERROR_LOG()			(pcMessageErrorLog)

#define		LENGTH_ERROR					0x01
#define		CHECK_SUM_ERROR					0x02
#define		UNKNOWN_COMMAND_ERROR			0x03
#define		DATA_VALIDATION_ERROR			0x04

#define		TRANSPORT_UNCALIBRATED			0xE0
#define		TRANSPORT_IN_LIMIT				0xE1
#define		TRANSPORT_IN_POSITION_ERROR		0xE2
#define		TRANSPORT_IN_NOT_READY_STATE	0xE3

#define		DUAL_PORT_MEM_TEST_FAILED		0xF0
#define		SYS_PARAMETERS_NOT_LOADED		0xF1

#define		LOG_LENGTH_ERROR()				(pcMessageErrorLog = LENGTH_ERROR)
#define		LOG_CHECK_SUM_ERROR()			(pcMessageErrorLog = CHECK_SUM_ERROR)
#define		LOG_UNKNOWN_COMMAND_ERROR()		(pcMessageErrorLog = UNKNOWN_COMMAND_ERROR)
#define		LOG_DATA_VALIDATION_ERROR()		(pcMessageErrorLog = DATA_VALIDATION_ERROR)

#define		CLEAR_PC_COMM_ERROR_LOG()		(pcMessageErrorLog = 0x00)


BYTE	bdata	SYSTEM_PARAMETERS_SET_FLAGS = 0x00;

sbit	CAMERA_PARAMETERS_SET		= SYSTEM_PARAMETERS_SET_FLAGS ^ 0;
sbit	ENCODER_PARAMETERS_SET		= SYSTEM_PARAMETERS_SET_FLAGS ^ 1;
sbit	TRANSPORT_PARAMETERS_SET	= SYSTEM_PARAMETERS_SET_FLAGS ^ 2;
sbit	PRESS_STATUS_PARAMETERS_SET	= SYSTEM_PARAMETERS_SET_FLAGS ^ 3;

#define		CAMERA_PARAMETERS_LOADED()			(CAMERA_PARAMETERS_SET			= CLEAR)
#define		ENCODER_PARAMETERS_LOADED()			(ENCODER_PARAMETERS_SET			= CLEAR)
#define		TRANSPORT_PARAMETERS_LOADED()		(TRANSPORT_PARAMETERS_SET		= CLEAR)
#define		PRESS_STATUS_PARAMETERS_LOADED()	(PRESS_STATUS_PARAMETERS_SET	= CLEAR)

#define		CAMERA_PARAMETERS_BLANK()			(CAMERA_PARAMETERS_SET			= SET)
#define		ENCODER_PARAMETERS_BLANK()			(ENCODER_PARAMETERS_SET			= SET)
#define		TRANSPORT_PARAMETERS_BLANK()		(TRANSPORT_PARAMETERS_SET		= SET)
#define		PRESS_STATUS_PARAMETERS_BLANK()		(PRESS_STATUS_PARAMETERS_SET	= SET)

#define		ALL_SYS_PARAMS_LOADED()	(SYSTEM_PARAMETERS_SET_FLAGS == 0x00)

//
//
//

BYTE	xdata	tcmCommBufferStart				_at_	0x800;

// Sratch Pad area
// for Temporary Debug

BYTE	xdata	debug0							_at_	0x801;
BYTE	xdata	debug1							_at_	0x802;
BYTE	xdata	debug2							_at_	0x803;
BYTE	xdata	debug3							_at_	0x804;
BYTE	xdata	debug4							_at_	0x805;
BYTE	xdata	debug5							_at_	0x806;
BYTE	xdata	debug6							_at_	0x807;
BYTE	xdata	debug7							_at_	0x808;


BYTE	xdata	tcmTxBuffer[MAX_PACKET_LENGTH]	_at_	0xB80;
BYTE	xdata	tcmRxBuffer[MAX_PACKET_LENGTH]	_at_	0xBBF;


void	xdata *ptrPcRxMessageBuffer;
void	xdata *ptrPcTxMessageBuffer;

// Receive BUFFER

#define		RX_MSG_BUFFER_SLOT_0	(ptrPcRxMessageBuffer)
#define		RX_MSG_BUFFER_SLOT_1	(ptrPcRxMessageBuffer+1)
#define		RX_MSG_BUFFER_SLOT_2	(ptrPcRxMessageBuffer+2)
#define		RX_MSG_BUFFER_SLOT_3	(ptrPcRxMessageBuffer+3)
#define		RX_MSG_BUFFER_SLOT_4	(ptrPcRxMessageBuffer+4)
#define		RX_MSG_BUFFER_SLOT_5	(ptrPcRxMessageBuffer+5)
#define		RX_MSG_BUFFER_SLOT_6	(ptrPcRxMessageBuffer+6)
#define		RX_MSG_BUFFER_SLOT_7	(ptrPcRxMessageBuffer+7)
#define		RX_MSG_BUFFER_SLOT_8	(ptrPcRxMessageBuffer+8)
#define		RX_MSG_BUFFER_SLOT_9	(ptrPcRxMessageBuffer+9)
#define		RX_MSG_BUFFER_SLOT_10	(ptrPcRxMessageBuffer+10)
#define		RX_MSG_BUFFER_SLOT_11	(ptrPcRxMessageBuffer+11)
#define		RX_MSG_BUFFER_SLOT_12	(ptrPcRxMessageBuffer+12)
#define		RX_MSG_BUFFER_SLOT_13	(ptrPcRxMessageBuffer+13)
#define		RX_MSG_BUFFER_SLOT_14	(ptrPcRxMessageBuffer+14)
#define		RX_MSG_BUFFER_SLOT_15	(ptrPcRxMessageBuffer+15)
#define		RX_MSG_BUFFER_SLOT_16	(ptrPcRxMessageBuffer+16)
#define		RX_MSG_BUFFER_SLOT_17	(ptrPcRxMessageBuffer+17)
#define		RX_MSG_BUFFER_SLOT_18	(ptrPcRxMessageBuffer+18)
#define		RX_MSG_BUFFER_SLOT_19	(ptrPcRxMessageBuffer+19)
#define		RX_MSG_BUFFER_SLOT_20	(ptrPcRxMessageBuffer+20)
#define		RX_MSG_BUFFER_SLOT_21	(ptrPcRxMessageBuffer+21)
#define		RX_MSG_BUFFER_SLOT_22	(ptrPcRxMessageBuffer+22)
#define		RX_MSG_BUFFER_SLOT_23	(ptrPcRxMessageBuffer+23)
#define		RX_MSG_BUFFER_SLOT_24	(ptrPcRxMessageBuffer+24)
#define		RX_MSG_BUFFER_SLOT_25	(ptrPcRxMessageBuffer+25)
#define		RX_MSG_BUFFER_SLOT_26	(ptrPcRxMessageBuffer+26)
#define		RX_MSG_BUFFER_SLOT_27	(ptrPcRxMessageBuffer+27)
#define		RX_MSG_BUFFER_SLOT_28	(ptrPcRxMessageBuffer+28)
#define		RX_MSG_BUFFER_SLOT_29	(ptrPcRxMessageBuffer+29)
#define		RX_MSG_BUFFER_SLOT_30	(ptrPcRxMessageBuffer+30)
#define		RX_MSG_BUFFER_SLOT_31	(ptrPcRxMessageBuffer+31)
#define		RX_MSG_BUFFER_SLOT_32	(ptrPcRxMessageBuffer+32)
#define		RX_MSG_BUFFER_SLOT_33	(ptrPcRxMessageBuffer+33)
#define		RX_MSG_BUFFER_SLOT_34	(ptrPcRxMessageBuffer+34)
#define		RX_MSG_BUFFER_SLOT_35	(ptrPcRxMessageBuffer+35)
#define		RX_MSG_BUFFER_SLOT_36	(ptrPcRxMessageBuffer+36)
#define		RX_MSG_BUFFER_SLOT_37	(ptrPcRxMessageBuffer+37)
#define		RX_MSG_BUFFER_SLOT_38	(ptrPcRxMessageBuffer+38)

// Transmit BUFFER

#define		TX_MSG_BUFFER_SLOT_0	(ptrPcTxMessageBuffer)
#define		TX_MSG_BUFFER_SLOT_1	(ptrPcTxMessageBuffer+1)
#define		TX_MSG_BUFFER_SLOT_2	(ptrPcTxMessageBuffer+2)
#define		TX_MSG_BUFFER_SLOT_3	(ptrPcTxMessageBuffer+3)
#define		TX_MSG_BUFFER_SLOT_4	(ptrPcTxMessageBuffer+4)
#define		TX_MSG_BUFFER_SLOT_5	(ptrPcTxMessageBuffer+5)
#define		TX_MSG_BUFFER_SLOT_6	(ptrPcTxMessageBuffer+6)
#define		TX_MSG_BUFFER_SLOT_7	(ptrPcTxMessageBuffer+7)
#define		TX_MSG_BUFFER_SLOT_8	(ptrPcTxMessageBuffer+8)
#define		TX_MSG_BUFFER_SLOT_9	(ptrPcTxMessageBuffer+9)
#define		TX_MSG_BUFFER_SLOT_10	(ptrPcTxMessageBuffer+10)
#define		TX_MSG_BUFFER_SLOT_11	(ptrPcTxMessageBuffer+11)
#define		TX_MSG_BUFFER_SLOT_12	(ptrPcTxMessageBuffer+12)
#define		TX_MSG_BUFFER_SLOT_13	(ptrPcTxMessageBuffer+13)
#define		TX_MSG_BUFFER_SLOT_14	(ptrPcTxMessageBuffer+14)
#define		TX_MSG_BUFFER_SLOT_15	(ptrPcTxMessageBuffer+15)
#define		TX_MSG_BUFFER_SLOT_16	(ptrPcTxMessageBuffer+16)
#define		TX_MSG_BUFFER_SLOT_17	(ptrPcTxMessageBuffer+17)
#define		TX_MSG_BUFFER_SLOT_18	(ptrPcTxMessageBuffer+18)
#define		TX_MSG_BUFFER_SLOT_19	(ptrPcTxMessageBuffer+19)
#define		TX_MSG_BUFFER_SLOT_20	(ptrPcTxMessageBuffer+20)
#define		TX_MSG_BUFFER_SLOT_21	(ptrPcTxMessageBuffer+21)
#define		TX_MSG_BUFFER_SLOT_22	(ptrPcTxMessageBuffer+22)
#define		TX_MSG_BUFFER_SLOT_23	(ptrPcTxMessageBuffer+23)
#define		TX_MSG_BUFFER_SLOT_24	(ptrPcTxMessageBuffer+24)
#define		TX_MSG_BUFFER_SLOT_25	(ptrPcTxMessageBuffer+25)
#define		TX_MSG_BUFFER_SLOT_26	(ptrPcTxMessageBuffer+26)
#define		TX_MSG_BUFFER_SLOT_27	(ptrPcTxMessageBuffer+27)
#define		TX_MSG_BUFFER_SLOT_28	(ptrPcTxMessageBuffer+28)
#define		TX_MSG_BUFFER_SLOT_29	(ptrPcTxMessageBuffer+29)
#define		TX_MSG_BUFFER_SLOT_30	(ptrPcTxMessageBuffer+30)
#define		TX_MSG_BUFFER_SLOT_31	(ptrPcTxMessageBuffer+31)
#define		TX_MSG_BUFFER_SLOT_32	(ptrPcTxMessageBuffer+32)
#define		TX_MSG_BUFFER_SLOT_33	(ptrPcTxMessageBuffer+33)
#define		TX_MSG_BUFFER_SLOT_34	(ptrPcTxMessageBuffer+34)
#define		TX_MSG_BUFFER_SLOT_35	(ptrPcTxMessageBuffer+35)
#define		TX_MSG_BUFFER_SLOT_36	(ptrPcTxMessageBuffer+36)
#define		TX_MSG_BUFFER_SLOT_37	(ptrPcTxMessageBuffer+37)
#define		TX_MSG_BUFFER_SLOT_38	(ptrPcTxMessageBuffer+38)


//////////////////////////////////////////////////
//
// TCMR/PC Handshake Flags
// Location Definitions
//
//////////////////////////////////////////////////

BYTE	xdata	topCameraProcessDoneFlag		_at_	0xB70;
BYTE	xdata	bottomCameraProcessDoneFlag		_at_	0xB71;

BYTE	xdata	topTransportProcessDoneFlag		_at_	0xB72;
BYTE	xdata	bottomTransportProcessDoneFlag	_at_	0xB73;

BYTE	xdata	topCameraResetDoneFlag			_at_	0xB74;
BYTE	xdata	bottomCameraResetDoneFlag		_at_	0xB75;

BYTE	xdata	topFlashRechargeDoneFlag		_at_	0xB76;
BYTE	xdata	bottomFlashRechargeDoneFlag		_at_	0xB77;

BYTE	xdata	topTransportResetDoneFlag		_at_	0xB78;
BYTE	xdata	bottomTransportResetDoneFlag	_at_	0xB79;

// reserved

BYTE	xdata	tcmPcReservedFlag10		_at_	0xB7a;
BYTE	xdata	tcmPcReservedFlag11		_at_	0xB7b;
BYTE	xdata	tcmPcReservedFlag12		_at_	0xB7c;
BYTE	xdata	tcmPcReservedFlag13		_at_	0xB7d;
BYTE	xdata	tcmPcReservedFlag14		_at_	0xB7e;
BYTE	xdata	tcmPcReservedFlag15		_at_	0xB7f;



//////////////////////////////////////////////////
//
// TCMR	Error Logging, Location Definitions
//
//////////////////////////////////////////////////

BYTE	xdata	mCSchecksumErrorCountLog		_at_	0xB60;
BYTE	xdata	currMCSmsgTypeErrorCountLog		_at_	0xB61;

// reserved
				
BYTE	xdata	tcmPcReservedVar2		_at_	0xB62;
BYTE	xdata	tcmPcReservedVar3		_at_	0xB63;
BYTE	xdata	tcmPcReservedVar4		_at_	0xB64;
BYTE	xdata	tcmPcReservedVar5		_at_	0xB65;
BYTE	xdata	tcmPcReservedVar6		_at_	0xB66;
BYTE	xdata	tcmPcReservedVar7		_at_	0xB67;
BYTE	xdata	tcmPcReservedVar8		_at_	0xB68;
BYTE	xdata	tcmPcReservedVar9		_at_	0xB69;
BYTE	xdata	tcmPcReservedVar10		_at_	0xB6a;
BYTE	xdata	tcmPcReservedVar11		_at_	0xB6b;
BYTE	xdata	tcmPcReservedVar12		_at_	0xB6c;
BYTE	xdata	tcmPcReservedVar13		_at_	0xB6d;
BYTE	xdata	tcmPcReservedVar14		_at_	0xB6e;
BYTE	xdata	tcmPcReservedVar15		_at_	0xB6f;


//////////////////////////////////////////////////
//
//////////////////////////////////////////////////

typedef	struct	
{
	BYTE	pcCommandId;

	void	(*pcCommandHandler)(void);

}PC_COMMAND_HANDLER;


BYTE	xdata		*ptrPcMessage;

BYTE				currCommandId;

void				(*currCommandHandler)(void);

PC_COMMAND_HANDLER	*currCommandHandlerEntry;


//////////////////////////////////////////////////
//
// WEH Private Functions Forward Delcare
//
//////////////////////////////////////////////////

BOOL		IsPcMessageOk(void);

void		HandlePcMessageError(void);

void		HandlePcMessageConfig(void);

void		HandlePcMessageActive(void);


void		ClearDebugMemory(void)
{
	debug0 = 0x00;		debug1 = 0x00;			debug2 = 0x00;
	debug3 = 0x00;		debug4 = 0x00;			debug5 = 0x00;
	debug6 = 0x00;		debug7 = 0x00;			
}

//////////////////////////////////////////////////
//
// PC Command Handlers, Forward declarations
//
//////////////////////////////////////////////////

void	DoDebug_0(void);
void	DoDualPortMemoryTest(void);

void	DoQueryTransportSystemStatus(void);
void	DoSubsystemPowerSaveControl(void);
void    DoHeadTransportReset(void);
void    DoCameraReset(void);

void	DoGetVersion(void);
void	DoGetMotorChipSetVersion(void);
void	DoGoActive(void);

void	DoSetEncoderSystemParameters(void);
void	DoQueryEncoderSystemParameters(void);

void	DoSetCameraSystemParameters(void);
void	DoQueryCameraSystemParameters(void);

void	DoSetTransportSystemParameters(void);
void	DoQueryTransportSystemParameters(void);

void	DoSetPressStatusSystemParameters(void);
void	DoQueryPressStatusSystemParameters(void);

void	DoSetImpressionCount(void);

void	DoSetCameraTriggerPosition(void);
void	DoSynchronizeCameraProcess(void);
void	DoCameraCaptureFast(void);
void	DoSetCameraInputControl(void);
void	DoQueryCameraOutputStatus(void);

void	DoSetCameraSetupIndex(void);

void	DoSetWebEncDivideBy(void);
void	DoSetWebEncFilter(void);
void	DoQueryWebEncIndex(void);
void	DoQueryWebEncPeriod(void);
void	DoQueryWebEncDirectionStatusFlag(void);
void	DoQueryWebEncImpressionCount(void);

void	DoTransportFindHome(void);
void	DoTransportGoHome(void);
void	DoSetTransportPosition(void);
void	DoJogTransport(void);
void	DoQueryTransportPosition(void);
void	DoQueryTransportState(void);


void	DoQueryPressStatus(void);





//////////////////////////////////////////////////
//
//	Special Functions to Control the 
//	Task Done Flags. 
//
//////////////////////////////////////////////////

#define		MAX_FLAG_CHANGE_RETRY	10

//
// Camera Process Flags
//

void	ClearTopCameraProcessDoneFlag(void)
{
	topCameraProcessDoneFlag = PROCESS_IS_DONE;
}

void	ClearBottomCameraProcessDoneFlag(void)
{
	bottomCameraProcessDoneFlag = PROCESS_IS_DONE;
}

void	ClearCameraProcessDoneFlag(int cameraTaskStateMachineId)
{
	BYTE	r,	flagForceRead;

	for(r=0; r < MAX_FLAG_CHANGE_RETRY; r++)
	{
		if(cameraTaskStateMachineId == 
			TopCameraSynchronizerHandlerID)
		{
			ClearTopCameraProcessDoneFlag();

			// Check to make sure Dual Port Ram 
			// accepted data change

			flagForceRead = topCameraProcessDoneFlag;
		}
		else
		{
			ClearBottomCameraProcessDoneFlag();

			// Check to make sure Dual Port Ram 
			// accepted data change

			flagForceRead = bottomCameraProcessDoneFlag;
		}
		
		if(flagForceRead == PROCESS_IS_DONE)
			break;	
	}
}


//
// Transport Process Flags
//

void	ClearTopTransportProcessDoneFlag(void)
{
	topTransportProcessDoneFlag = PROCESS_IS_DONE;
}

void	ClearBottomTransportProcessDoneFlag(void)
{
	bottomTransportProcessDoneFlag = PROCESS_IS_DONE;
}

void	ClearTransportProcessDoneFlag(int transportTaskStateMachineId)
{
	BYTE	r,	flagForceRead;

	for(r=0; r < MAX_FLAG_CHANGE_RETRY; r++)
	{
		if(transportTaskStateMachineId == TopTransportHandlerID)
		{
			ClearTopTransportProcessDoneFlag();

			// Check to make sure Dual Port Ram 
			// accepted data change

			flagForceRead = topTransportProcessDoneFlag;
		}
		else
		{
			ClearBottomTransportProcessDoneFlag();

			// Check to make sure Dual Port Ram 
			// accepted data change

			flagForceRead = bottomTransportProcessDoneFlag;
		}
		
		if(flagForceRead == PROCESS_IS_DONE)
			break;	
	}
}


//
// Camera System Reset Flags
//

void	ClearTopCameraResetDoneFlag(void)
{
	topCameraResetDoneFlag = PROCESS_IS_DONE;
}

void	ClearBottomCameraResetDoneFlag(void)
{
	bottomCameraResetDoneFlag = PROCESS_IS_DONE;
}

void	ClearCameraResetDoneFlag(int cameraSystemTaskId)
{
	BYTE	r,	flagForceRead;

	for(r=0; r < MAX_FLAG_CHANGE_RETRY; r++)
	{
		if(cameraSystemTaskId == TopCameraSynchronizerHandlerID)
		{
			ClearTopCameraResetDoneFlag();

			// Check to make sure Dual Port Ram 
			// accepted data change

			flagForceRead = topCameraResetDoneFlag;
		}
		else
		{
			ClearBottomCameraResetDoneFlag();

			// Check to make sure Dual Port Ram 
			// accepted data change

			flagForceRead = bottomCameraResetDoneFlag;
		}
		
		if(flagForceRead == PROCESS_IS_DONE)
			break;	
	}
}

//
// Flash Recharge Flags
//

void	ClearTopFlashRechargeDoneFlag(void)
{
	topFlashRechargeDoneFlag = PROCESS_IS_DONE;
}

void	ClearBottomFlashRechargeDoneFlag(void)
{
	bottomFlashRechargeDoneFlag = PROCESS_IS_DONE;
}

void	ClearFlashRechargeDoneFlag(int cameraTaskStateMachineId)
{
	BYTE	r,	flagForceRead;

	for(r=0; r < MAX_FLAG_CHANGE_RETRY; r++)
	{
		if(cameraTaskStateMachineId == TopCameraSynchronizerHandlerID)
		{
			ClearTopFlashRechargeDoneFlag();

			// Check to make sure Dual Port Ram 
			// accepted data change

			flagForceRead = topFlashRechargeDoneFlag;
		}
		else
		{
			ClearBottomFlashRechargeDoneFlag();

			// Check to make sure Dual Port Ram 
			// accepted data change

			flagForceRead = bottomFlashRechargeDoneFlag;
		}
		
		if(flagForceRead == PROCESS_IS_DONE)
			break;	
	}
}

//
// Transport System Reset Flags
//

void	ClearTopTransportResetDoneFlag(void)
{
	topTransportResetDoneFlag = PROCESS_IS_DONE;
}

void	ClearBottomTransportResetDoneFlag(void)
{
	bottomTransportResetDoneFlag = PROCESS_IS_DONE;
}

void	ClearTransportResetDoneFlag(int transportSystemTaskId)
{
	BYTE	r,	flagForceRead;

	for(r=0; r < MAX_FLAG_CHANGE_RETRY; r++)
	{
		if(transportSystemTaskId == TopTransportHandlerID)
		{
			ClearTopTransportResetDoneFlag();

			// Check to make sure Dual Port Ram 
			// accepted data change

			flagForceRead = topTransportResetDoneFlag;
		}
		else
		{
			ClearBottomTransportResetDoneFlag();

			// Check to make sure Dual Port Ram 
			// accepted data change

			flagForceRead = bottomTransportResetDoneFlag;
		}
		
		if(flagForceRead == PROCESS_IS_DONE)
			break;	
	}
}


//////////////////////////////////////////////////
//
// PC Communication, interface
//
//////////////////////////////////////////////////

void	StartReplyTransmission(void)
{
	BYTE	xdata *ptrTxBuff = GetTcmTxBuffer();

	BYTE	len = *(ptrTxBuff);		// First Byte

	BYTE	l;

	BYTE	checkSum = len;

			--len;		// 1 Less for the checksum data slot

			// Calculate to Generate Checksum

			for(l=1; l < len; l++)
			{
				++ptrTxBuff;
				checkSum += *(ptrTxBuff);
			}

		WriteToPcInterruptByte(checkSum);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	StartAckTransmission(BYTE cmdId)
{
		*(BYTE *)(TX_MSG_BUFFER_SLOT_0)	= 4;		// Length
		*(BYTE *)(TX_MSG_BUFFER_SLOT_1)	= cmdId;	// Cmd
		*(BYTE *)(TX_MSG_BUFFER_SLOT_2)	= ACK;		// Ack

	StartReplyTransmission();
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	StartNakTransmission(BYTE errorRegister)
{
		*(BYTE *)(TX_MSG_BUFFER_SLOT_0) = 4;				// Length
		*(BYTE *)(TX_MSG_BUFFER_SLOT_1) = NAK;				// Nak
		*(BYTE *)(TX_MSG_BUFFER_SLOT_2) = errorRegister;	// Data1

	StartReplyTransmission();
}


//////////////////////////////////////////////////
//
// Exit Procedures
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
//	GoSystemParameterConfig while in IDLE
//
//////////////////////////////////////////////////

int	PCH_exitA(void)
{
	ClearDebugMemory();

	ClearCameraProcessDoneFlag(TopCameraSynchronizerHandlerID);
	ClearTransportProcessDoneFlag(TopTransportHandlerID);
	ClearFlashRechargeDoneFlag(TopCameraSynchronizerHandlerID);
	SetTopCameraResetDoneFlag();
	SetTopTransportResetDoneFlag();

	ClearCameraProcessDoneFlag(BottomCameraSynchronizerHandlerID);
	ClearTransportProcessDoneFlag(BottomCameraSynchronizerHandlerID);
	ClearFlashRechargeDoneFlag(BottomCameraSynchronizerHandlerID);
	SetBottomCameraResetDoneFlag();
	SetBottomTransportResetDoneFlag();

		// INIT PC Comm
	
		ptrPcRxMessageBuffer = GetTcmRxBuffer();

		ptrPcTxMessageBuffer = GetTcmTxBuffer();

		// No Parameters Loaded

		CAMERA_PARAMETERS_BLANK();
		ENCODER_PARAMETERS_BLANK();
		TRANSPORT_PARAMETERS_BLANK();
		PRESS_STATUS_PARAMETERS_BLANK();

		ENABLE_PC_TO_TCMR_RECEPTION();
		
	return PCH_ACTIVE_CONFIGURING;
}


//////////////////////////////////////////////////
//
//	PcCommandReceived while in 
//	ACTIVE_CONFIGURING
//
//////////////////////////////////////////////////

void	HandlePcMessageConfig(void)
{
		ptrPcMessage	=	GetTcmRxBuffer();

		currCommandId	=	*(ptrPcMessage + COMMAND_SLOT);

		switch(currCommandId)
		{
			case	GET_VERSION:
						DoGetVersion();
						break;

			case	GO_ACTIVE:
						DoGoActive();
						if(ALL_SYS_PARAMS_LOADED()) 
							SendMessage(THIS_SM, GoActive);
						break;

			case	SET_ENCODER_SYS_PARAMS:
						DoSetEncoderSystemParameters();
						break;

			case	QUERY_ENCODER_SYS_PARAMS:
						DoQueryEncoderSystemParameters();
						break;

			case	SET_CAMERA_SYS_PARAMS:
						DoSetCameraSystemParameters();
						break;
						
			case	QUERY_CAMERA_SYS_PARAMS:
						DoQueryCameraSystemParameters();
						break;

			case	SET_TRANSPORT_SYS_PARAMS:
						DoSetTransportSystemParameters();
						break;

			case	QUERY_TRANSPORT_SYS_PARAMS:
						DoQueryTransportSystemParameters();
						break;

			case	SET_PRESS_STAT_SYS_PARAMS:
						DoSetPressStatusSystemParameters();
						break;

			case	QUERY_PRESS_STAT_SYS_PARAMS:
						DoQueryPressStatusSystemParameters();
						break;

			case	DEBUG_0_COMMAND:
						DoDebug_0();
						break;

			case	DUAL_PORT_MEMORY_TEST:
						DoDualPortMemoryTest();
						break;

			default:
				LOG_UNKNOWN_COMMAND_ERROR();
				HandlePcMessageError();
				break;
		}
}


int	PCH_exitB(void)
{
		if(IsPcMessageOk())
			HandlePcMessageConfig();
				else
			HandlePcMessageError();
	
	return	SAME_STATE;
}

//////////////////////////////////////////////////
//
// Go Active while in CONFIGURING
//
//////////////////////////////////////////////////

int	PCH_exitC(void)
{		
		SendMessage(TCMR_SystemManagerID, SystemParameterConfigDone);

		// Stop replying to any CCU request
		// Temporarily until GoActive is complete

	return PCH_WAIT_GOACTIVE_DONE;
}

//////////////////////////////////////////////////
//
// GoActiveDone while in PCH_WAIT_GOACTIVE_DONE
//
//////////////////////////////////////////////////

int	PCH_exitC1(void)
{		
	return PCH_ACTIVE;
}

//////////////////////////////////////////////////
//
//	PcCommandReceived while in ACTIVE
//	
//	Note the searching of commands using the CASE
//	statement takes too much time. This technique
//	allows the search to be scheduled giving time
//	for other tasks to execute.
//
//////////////////////////////////////////////////


PC_COMMAND_HANDLER	code	pcCommandHandlerEntries[] =
{
	{ SYNCHRONIZE_CAMERA_PROCESS,	DoSynchronizeCameraProcess			},
	{ SET_CAMERA_TRIGGER_POSITION,	DoSetCameraTriggerPosition			},
	{ SET_CAMERA_TRIGGER_POSITION,	DoSetCameraTriggerPosition			},
	{ QUERY_CAMERA_OUTPUT_STATUS,	DoQueryCameraOutputStatus			},
	{ QUERY_PRESS_STATUS,			DoQueryPressStatus					},
	{ QUERY_WEB_ENC_PERIOD,			DoQueryWebEncPeriod					},
	{ QUERY_WEB_IMPRESSION_COUNT,	DoQueryWebEncImpressionCount		},
	{ SET_CAMERA_INPUT_CONTROL,		DoSetCameraInputControl				},
	{ JOG_TRANSPORT,				DoJogTransport						},
	{ QUERY_WEB_ENC_INDEX,			DoQueryWebEncIndex					},
	{ SET_TRANSPORT_POSITION,		DoSetTransportPosition				},
	{ SET_CAMERA_SETUP_INDEX,		DoSetCameraSetupIndex				},
	{ QUERY_TRANSPORT_POSITION,		DoQueryTransportPosition			},
	{ CAMERA_CAPTURE_FAST,			DoCameraCaptureFast					},
	{ SET_WEB_ENC_DIVIDE_BY,		DoSetWebEncDivideBy					},
	{ SET_WEB_ENC_FILTER,			DoSetWebEncFilter					},
	{ QUERY_WEB_ENC_DIR_STAT_FLAG,	DoQueryWebEncDirectionStatusFlag	},
	{ TRANSPORT_FIND_HOME,			DoTransportFindHome					},
	{ TRANSPORT_GO_HOME,			DoTransportGoHome					},
	{ QUERY_TRANSPORT_STATE,		DoQueryTransportState				},
	{ SET_IMPRESSION_COUNT,			DoSetImpressionCount				},
	{ TRANSPORT_RESET,				DoHeadTransportReset				},
	{ CAMERA_RESET,					DoCameraReset						},
	{ QUERY_TRANSPORT_STATUS,		DoQueryTransportSystemStatus		},
	{ SUBSYSTEM_POWERSAVE_CONTROL,	DoSubsystemPowerSaveControl			},
	{ QUERY_MOTOR_CHIPSET_VERSION,	DoGetMotorChipSetVersion			},

	{ SET_ENCODER_SYS_PARAMS,		DoSetEncoderSystemParameters		},
	{ QUERY_ENCODER_SYS_PARAMS,		DoQueryEncoderSystemParameters		},
	{ SET_CAMERA_SYS_PARAMS,		DoSetCameraSystemParameters			},
	{ QUERY_CAMERA_SYS_PARAMS,		DoQueryCameraSystemParameters		},
	{ SET_TRANSPORT_SYS_PARAMS,		DoSetTransportSystemParameters		},
	{ QUERY_TRANSPORT_SYS_PARAMS,	DoQueryTransportSystemParameters	},
	{ SET_PRESS_STAT_SYS_PARAMS,	DoSetPressStatusSystemParameters	},
	{ QUERY_PRESS_STAT_SYS_PARAMS,	DoQueryPressStatusSystemParameters	},

	{ NULL_PC_COMMAND_ID,			0									}
};


//////////////////////////////////////////////////
//
// Active State 
//	PC Comm message handler
//
//////////////////////////////////////////////////

int	PCH_exitD(void)
{
	ptrPcMessage =	GetTcmRxBuffer();

		if(IsPcMessageOk())
		{
			currCommandId = *(ptrPcMessage + COMMAND_SLOT);
			currCommandHandlerEntry = &pcCommandHandlerEntries;

			SendLowPriorityMessage(THIS_SM, SearchNextCommandEntry);

			return PCH_COMMAND_SEARCH;
		}
		else
		{
			HandlePcMessageError();		

			return SAME_STATE;
		}
}

//////////////////////////////////////////////////
//
// SearchNextCommandEntry
//		while in PCH_COMMAND_SEARCH
//
//////////////////////////////////////////////////

int	PCH_exitE(void)
{
	if(currCommandId == currCommandHandlerEntry->pcCommandId)
	{
		// Command Id Match Found, execute Handler

		currCommandHandlerEntry->pcCommandHandler();

		return PCH_ACTIVE;
	}
	else
	if(currCommandHandlerEntry->pcCommandId == NULL_PC_COMMAND_ID)
	{
		LOG_UNKNOWN_COMMAND_ERROR(); 
		HandlePcMessageError();	

		return PCH_ACTIVE;
	}
	else
	{
		++currCommandHandlerEntry;

		SendLowPriorityMessage(THIS_SM, SearchNextCommandEntry);

		return SAME_STATE;
	}
}


//////////////////////////////////////////////////
//
// Validate the message
//
//////////////////////////////////////////////////

BOOL    IsPcMessageOk(void)
{
    BYTE    xdata *ptrRxBuffer = GetTcmRxBuffer();
    BYTE    len = *(ptrRxBuffer);           // First Byte
    BYTE    checkSum = len;
    BYTE    l;

        if(len > MAX_PACKET_LENGTH)         // is Length OK ?
        {
            LOG_LENGTH_ERROR();
            return FALSE;
        }

        --len;                              // 1 Less for the
                                            // checksum data slot

        // Calculate to Generate Checksum

        for(l=1; l < len; l++)
        {
            ++ptrRxBuffer;
            checkSum += *(ptrRxBuffer);
        }

        if(checkSum != GetTcmRxCheckSum())  // is Checksum OK ?
        {
            LOG_CHECK_SUM_ERROR();
            return FALSE;
        }

        CLEAR_PC_COMM_ERROR_LOG();
        return TRUE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    HandlePcMessageError(void)
{
    StartNakTransmission(GET_PC_COMM_ERROR_LOG());
}

//////////////////////////////////////////////////
//
// WEH Private Functions Implementation
//
//////////////////////////////////////////////////

void	DoDebug_0(void)
{
		*(BYTE  *)(TX_MSG_BUFFER_SLOT_0) = 0x07;	// Length
		*(BYTE  *)(TX_MSG_BUFFER_SLOT_1) = DEBUG_0_COMMAND;
		*(DWORD *)(TX_MSG_BUFFER_SLOT_2) = 0x11223344;

	StartReplyTransmission();
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoDualPortMemoryTest(void)
{	
	WORD m;

	BYTE *mPtr;

		// Write 0xAA
		mPtr = &tcmCommBufferStart;
		for(m=0; m<0x03FE; m++)
		{
			*(mPtr) = 0xAA;
			++mPtr;
		}

		// Read 0xAA
		mPtr = &tcmCommBufferStart;
		for(m=0; m<0x03FE; m++)
		{
			if(*mPtr == 0xAA)
				++mPtr;
			else
			{
				StartNakTransmission(DUAL_PORT_MEM_TEST_FAILED);
				return;
			}
		}

		// Write 0x55
		mPtr = &tcmCommBufferStart;
		for(m=0; m<0x03FE; m++)
		{
			*(mPtr) = 0x55;
			++mPtr;
		}

		// Read 0x55
		mPtr = &tcmCommBufferStart;
		for(m=0; m<0x03FE; m++)
		{
			if(*mPtr == 0x55)
				++mPtr;
			else
			{
				StartNakTransmission(DUAL_PORT_MEM_TEST_FAILED);
				return;
			}
		}

		// Write 0x00
		mPtr = &tcmCommBufferStart;
		for(m=0; m<0x03FE; m++)
		{
			*(mPtr) = 0x00;
			++mPtr;
		}

		// Read 0x00
		mPtr = &tcmCommBufferStart;
		for(m=0; m<0x03FE; m++)
		{
			if(*mPtr == 0x00)
				++mPtr;
			else
			{
				StartNakTransmission(DUAL_PORT_MEM_TEST_FAILED);
				return;
			}
		}

	StartAckTransmission(DUAL_PORT_MEMORY_TEST);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoQueryTransportSystemStatus(void)
{
		*(BYTE *)(TX_MSG_BUFFER_SLOT_0)	= 0x06;	// Length
		*(BYTE *)(TX_MSG_BUFFER_SLOT_1)	= 
			QUERY_TRANSPORT_STATUS;				// Cmd

		*(BOOL *)(TX_MSG_BUFFER_SLOT_2)	=
			GetCurrTopTransportStatus();

		*(BOOL *)(TX_MSG_BUFFER_SLOT_3)	=
			GetCurrBottomTransportStatus();

		*(BYTE *)(TX_MSG_BUFFER_SLOT_4)	=
			GetCurrTransportLimitStatus();

	StartReplyTransmission();
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoSubSystemPowerSaveControl(void)
{
	BOOL	topSubsystemSelect = 
				*(BOOL *)(RX_MSG_BUFFER_SLOT_2);
	
	BOOL	bottomSubsystemSelect = 
				*(BOOL *)(RX_MSG_BUFFER_SLOT_3);
    
		if(topSubsystemSelect == TRUE)
		{
			// Immediately Shut down power of the
			// Camera and Transport on the Junction Box

			HOLD_TOP_CAMERA_SYSTEM_RESET();	// CAMERA + ILLUM

			HOLD_TOP_TRANSPORT_RESET();		// TRANSPORT 
		}

			
		if(bottomSubsystemSelect == TRUE)
		{
			HOLD_BOTTOM_CAMERA_SYSTEM_RESET();
			
			HOLD_BOTTOM_TRANSPORT_RESET();	
		}

	StartAckTransmission(SUBSYSTEM_POWERSAVE_CONTROL);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoHeadTransportReset(void)
{
	BYTE	transportSystemSelect = 
				*(BYTE *)(RX_MSG_BUFFER_SLOT_2);
    
		if(transportSystemSelect == TOP_SYSTEM_SELECT)
		{
			HOLD_TOP_TRANSPORT_RESET();

			SetTopTransportResetDoneFlag();
		}
		else
		{
			HOLD_BOTTOM_TRANSPORT_RESET();

			SetBottomTransportResetDoneFlag();
		}

		SendMessageAndData
			(TCMR_SystemManagerID ,TransportReset, transportSystemSelect);


	StartAckTransmission(TRANSPORT_RESET);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    DoCameraReset(void)
{
	BYTE	cameraSystemSelect = 
				*(BYTE *)(RX_MSG_BUFFER_SLOT_2);
    
		if(cameraSystemSelect == TOP_SYSTEM_SELECT)
		{
			// Immediately Shut down all power of the
			// Camera and Transport on the Junction Box

			HOLD_TOP_CAMERA_SYSTEM_RESET();	// CAMERA + ILLUM

			SetTopCameraResetDoneFlag();
		}
		else
		{
			HOLD_BOTTOM_CAMERA_SYSTEM_RESET();

			SetBottomCameraResetDoneFlag();
		}
		
		SendMessageAndData
			(TCMR_SystemManagerID ,CameraReset, cameraSystemSelect);

	StartAckTransmission(CAMERA_RESET);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoGetVersion(void)
{
		*(BYTE *)(TX_MSG_BUFFER_SLOT_0)	= 0x06;			// Length
		*(BYTE *)(TX_MSG_BUFFER_SLOT_1)	= GET_VERSION;	// Cmd

		// Add VERSION Data into reply buffer

		*(BYTE *)(TX_MSG_BUFFER_SLOT_2)	= MAJOR_VERSION;
		*(BYTE *)(TX_MSG_BUFFER_SLOT_3)	= MINOR_VERSION;
		*(BYTE *)(TX_MSG_BUFFER_SLOT_4)	= INTERMEDIATE_VERSION;

	StartReplyTransmission();
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoGetMotorChipSetVersion(void)
{
		*(BYTE *)(TX_MSG_BUFFER_SLOT_0)	= 0x05;							// Length
		*(BYTE *)(TX_MSG_BUFFER_SLOT_1)	= QUERY_MOTOR_CHIPSET_VERSION;	// Cmd

		*(WORD *)(TX_MSG_BUFFER_SLOT_2)	= GetMotorChipSetVersion();

	StartReplyTransmission();
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoGoActive(void)
{
	if(ALL_SYS_PARAMS_LOADED())
		StartAckTransmission(GO_ACTIVE);
	else
		StartNakTransmission(SYS_PARAMETERS_NOT_LOADED);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoSetEncoderSystemParameters(void)
{
			SetImpressionCount
				(*(DWORD *)(RX_MSG_BUFFER_SLOT_2));

			SetEncoderCountPerImpression
				(*(WORD *)(RX_MSG_BUFFER_SLOT_6));

			// These 2 parameters require 
			// Control ports and Registers to be updated

			SetTdcPulseEdge
				(*(BOOL *)(RX_MSG_BUFFER_SLOT_8));

			SetWebEncoderDivideBy
				(*(BYTE *)(RX_MSG_BUFFER_SLOT_9));
			
			SetWebEncoderFilter
				(*(BYTE *)(RX_MSG_BUFFER_SLOT_10));

			/* DEBUG
			SetWebEncoderPeriodFilterType
				(*(BYTE *)(RX_MSG_BUFFER_SLOT_11));
			*/

		ENCODER_PARAMETERS_LOADED();

	StartAckTransmission(SET_ENCODER_SYS_PARAMS);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoQueryEncoderSystemParameters(void)
{
		*(BYTE *)(TX_MSG_BUFFER_SLOT_0) = 0x0C;	// Length
		*(BYTE *)(TX_MSG_BUFFER_SLOT_1) = 
			QUERY_ENCODER_SYS_PARAMS;	// Cmd

		*(DWORD *)(TX_MSG_BUFFER_SLOT_2)	=
			GetImpressionCount();

		*(WORD  *)(TX_MSG_BUFFER_SLOT_6)	=
			GetEncoderCountPerImpression();

		*(BOOL  *)(TX_MSG_BUFFER_SLOT_8)	=
			GetTdcPulseEdge();

		*(BYTE  *)(TX_MSG_BUFFER_SLOT_9)	=
			GetWebEncoderDivideBy();
		
		*(BYTE  *)(TX_MSG_BUFFER_SLOT_10)	=
			GetWebEncoderFilter();

		/* DEBUG
		*(BYTE  *)(TX_MSG_BUFFER_SLOT_11)	=
			GetWebEncoderPeriodFilterType();
		*/

	StartReplyTransmission();
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoSetCameraSystemParameters(void)
{
			SetCameraPreTriggerSetupIndexDistance
				(*(WORD *)(RX_MSG_BUFFER_SLOT_2));

			SetCameraTriggerRetardTime
				(*(WORD *)(RX_MSG_BUFFER_SLOT_4));

			SetCameraShutterTime
				(*(WORD *)(RX_MSG_BUFFER_SLOT_6));
			
			SetCameraFlashRechargeTime
				(*(WORD *)(RX_MSG_BUFFER_SLOT_8));

		CAMERA_PARAMETERS_LOADED();

	StartAckTransmission(SET_CAMERA_SYS_PARAMS);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoQueryCameraSystemParameters(void)
{
		*(BYTE *)(TX_MSG_BUFFER_SLOT_0) = 0x0B;	// Length
		*(BYTE *)(TX_MSG_BUFFER_SLOT_1) = 
			QUERY_CAMERA_SYS_PARAMS;		// Cmd

		*(WORD *)(TX_MSG_BUFFER_SLOT_2)	=
			GetCameraPreTriggerSetupIndexDistance();

		*(WORD *)(TX_MSG_BUFFER_SLOT_4)	=
			GetCameraTriggerRetardTime();
		
		*(WORD *)(TX_MSG_BUFFER_SLOT_6)	=
			GetCameraShutterTime();

		*(WORD *)(TX_MSG_BUFFER_SLOT_8)	=
			GetCameraFlashRechargeTime();

	StartReplyTransmission();
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoSetTransportSystemParameters(void)
{
			SetMoveSettleWait(*(BYTE *)(RX_MSG_BUFFER_SLOT_2));
			SetFindHomeSpeed(*(DWORD *)(RX_MSG_BUFFER_SLOT_3));
			SetLocateSpeed(*(DWORD *)(RX_MSG_BUFFER_SLOT_7));
			SetBacklashSpeed(*(DWORD *)(RX_MSG_BUFFER_SLOT_11));
			SetBacklashCount(*(WORD *)(RX_MSG_BUFFER_SLOT_15));
			SetJogSpeed(*(DWORD *)(RX_MSG_BUFFER_SLOT_17));
			SetHitHomeLimitSpeed(*(DWORD *)(RX_MSG_BUFFER_SLOT_21));
			SetHomeDistanceFromNearLimit(*(WORD *)(RX_MSG_BUFFER_SLOT_25));
			SetStartingVelocity(*(DWORD *)(RX_MSG_BUFFER_SLOT_27));
			SetAcceleration(*(DWORD *)(RX_MSG_BUFFER_SLOT_31));
			
			SetTransportOrientation(*(BYTE *)(RX_MSG_BUFFER_SLOT_35));
			SetTransportEncoderAvailable(*(BYTE *)(RX_MSG_BUFFER_SLOT_36));
			SetLimitConfig(*(BYTE *)(RX_MSG_BUFFER_SLOT_37));

		TRANSPORT_PARAMETERS_LOADED();

	StartAckTransmission(SET_TRANSPORT_SYS_PARAMS);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoQueryTransportSystemParameters(void)
{
		*(BYTE *)(TX_MSG_BUFFER_SLOT_0) = 0x27;	// Length
		*(BYTE *)(TX_MSG_BUFFER_SLOT_1) = 
			QUERY_TRANSPORT_SYS_PARAMS;	// Cmd

		*(BYTE *)(TX_MSG_BUFFER_SLOT_2)		= GetMoveSettleWait();
		*(DWORD *)(TX_MSG_BUFFER_SLOT_3)	= GetFindHomeSpeed();
		*(DWORD *)(TX_MSG_BUFFER_SLOT_7)	= GetLocateSpeed();
		*(DWORD *)(TX_MSG_BUFFER_SLOT_11)	= GetBacklashSpeed();
		*(WORD *)(TX_MSG_BUFFER_SLOT_15)	= GetBacklashCount();
		*(DWORD *)(TX_MSG_BUFFER_SLOT_17)	= GetJogSpeed();
		*(DWORD *)(TX_MSG_BUFFER_SLOT_21)	= GetHitHomeLimitSpeed();
		*(WORD *)(TX_MSG_BUFFER_SLOT_25)	= GetHomeDistanceFromNearLimit();
		*(DWORD *)(TX_MSG_BUFFER_SLOT_27)	= GetStartingVelocity();
		*(DWORD *)(TX_MSG_BUFFER_SLOT_31)	= GetAcceleration();
		
		*(BYTE *)(TX_MSG_BUFFER_SLOT_35)	= GetTransportOrientation();
		*(BYTE *)(TX_MSG_BUFFER_SLOT_36)	= GetTransportEncoderAvailable();
		*(BYTE *)(TX_MSG_BUFFER_SLOT_37)	= GetLimitConfig();

	StartReplyTransmission();
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoSetPressStatusSystemParameters(void)
{
			SetPressStatusSystemParameter
				(*(BYTE *)(RX_MSG_BUFFER_SLOT_2));

		PRESS_STATUS_PARAMETERS_LOADED();

	StartAckTransmission(SET_PRESS_STAT_SYS_PARAMS);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoQueryPressStatusSystemParameters(void)
{
		*(BYTE *)(TX_MSG_BUFFER_SLOT_0) = 0x4;		// Length
		*(BYTE *)(TX_MSG_BUFFER_SLOT_1) = 
			QUERY_PRESS_STAT_SYS_PARAMS;	// Cmd

		*(BYTE *)(TX_MSG_BUFFER_SLOT_2)	= 
			GetPressStatusSystemParameter();

	StartReplyTransmission();
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoSetCameraTriggerPosition(void)
{
	BYTE	cameraSelect = *(BYTE *)(RX_MSG_BUFFER_SLOT_2);

	WORD	camIdx	= *(WORD *)(RX_MSG_BUFFER_SLOT_3);
	WORD	camRem	= *(WORD *)(RX_MSG_BUFFER_SLOT_5);

		if(cameraSelect == TOP_SYSTEM_SELECT)
		{
			SetTopCameraFlashEncoderMark(camIdx, camRem);
		}
		else
		{
			SetBottomCameraFlashEncoderMark(camIdx, camRem);
		}

	StartAckTransmission(SET_CAMERA_TRIGGER_POSITION);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoSynchronizeCameraProcess(void)
{
	BOOL	synchronizeTopCam	 = *(BOOL *)(RX_MSG_BUFFER_SLOT_2);
	BOOL	synchronizeBottomCam = *(BOOL *)(RX_MSG_BUFFER_SLOT_3);

		if(synchronizeTopCam)
		{
			SetTopCameraProcessDoneFlag();
			SetTopFlashRechargeDoneFlag();

			SendMessageAndData
				(TCMR_SystemManagerID, SyncCameraTrigger, TOP_SYSTEM_SELECT);
		}

		if(synchronizeBottomCam)
		{
			SetBottomCameraProcessDoneFlag();
			SetBottomFlashRechargeDoneFlag();

			SendMessageAndData
				(TCMR_SystemManagerID, SyncCameraTrigger, BOTTOM_SYSTEM_SELECT);
		}

	StartAckTransmission(SYNCHRONIZE_CAMERA_PROCESS);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoCameraCaptureFast(void)
{
	BOOL	captureFastTopCam	 = *(BOOL *)(RX_MSG_BUFFER_SLOT_2);
	BOOL	captureFastBottomCam = *(BOOL *)(RX_MSG_BUFFER_SLOT_3);

		if(captureFastTopCam)
		{
			SetTopCameraProcessDoneFlag();
			SetTopFlashRechargeDoneFlag();

			SendMessageAndData
				(TCMR_SystemManagerID, CameraCaptureFast, TOP_SYSTEM_SELECT);
		}

		if(captureFastBottomCam)
		{
			SetBottomCameraProcessDoneFlag();
			SetBottomFlashRechargeDoneFlag();

			SendMessageAndData
				(TCMR_SystemManagerID, CameraCaptureFast, BOTTOM_SYSTEM_SELECT);
		}

	StartAckTransmission(CAMERA_CAPTURE_FAST);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoSetCameraInputControl(void)
{		
	BYTE	topCamInCtrl	= *(BYTE *)(RX_MSG_BUFFER_SLOT_2);
	BYTE	bottomCamInCtrl	= *(BYTE *)(RX_MSG_BUFFER_SLOT_3);
		
		(topCamInCtrl & 0x01) ?
			SET_TOP_CAMERA_IN_0() : CLEAR_TOP_CAMERA_IN_0();

		(topCamInCtrl & 0x02) ?
			SET_TOP_CAMERA_IN_1() : CLEAR_TOP_CAMERA_IN_1();

		(bottomCamInCtrl & 0x01) ?
			SET_BOTTOM_CAMERA_IN_0() : CLEAR_BOTTOM_CAMERA_IN_0();

		(bottomCamInCtrl & 0x02) ?
			SET_BOTTOM_CAMERA_IN_1() : CLEAR_BOTTOM_CAMERA_IN_1();

	StartAckTransmission(SET_CAMERA_INPUT_CONTROL);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

#define		CAMERA_OUT_0	0x01
#define		CAMERA_OUT_1	0x02

void	DoQueryCameraOutputStatus(void)
{
	BYTE	topCamIoStat	= 0x00,
			bottomCamIoStat = 0x00;
		
		if(GET_TOP_CAMERA_OUT_0())
			topCamIoStat |= CAMERA_OUT_0;

		if(GET_TOP_CAMERA_OUT_1())
			topCamIoStat |= CAMERA_OUT_1;

		if(GET_BOTTOM_CAMERA_OUT_0())
			bottomCamIoStat |= CAMERA_OUT_0;

		if(GET_BOTTOM_CAMERA_OUT_1())
			bottomCamIoStat |= CAMERA_OUT_1;

		*(BYTE *)(TX_MSG_BUFFER_SLOT_0) = 0x05;	// Length
		*(BYTE *)(TX_MSG_BUFFER_SLOT_1) = QUERY_CAMERA_OUTPUT_STATUS; // Cmd

		*(BYTE *)(TX_MSG_BUFFER_SLOT_2)	= topCamIoStat;
		*(BYTE *)(TX_MSG_BUFFER_SLOT_3)	= bottomCamIoStat;

	StartReplyTransmission();
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoSetCameraSetupIndex(void)
{
		SetCameraPreTriggerSetupIndexDistance
			(*(WORD *)(RX_MSG_BUFFER_SLOT_2));

	StartAckTransmission(SET_CAMERA_SETUP_INDEX);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoSetWebEncDivideBy(void)
{
	// IMPORTANT:
	// CCU Has to WAIT at least 2 IMPRESSION counts
	// before proceeding, when this command is used

		SetWebEncoderDivideBy
			(*(BYTE *)(RX_MSG_BUFFER_SLOT_2));

	StartAckTransmission(SET_WEB_ENC_DIVIDE_BY);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoSetWebEncFilter(void)
{
	// IMPORTANT:
	// CCU Has to WAIT at least 2 IMPRESSION counts
	// before proceeding, when this command is used

		SetWebEncoderFilter
			(*(BYTE *)(RX_MSG_BUFFER_SLOT_2));

	StartAckTransmission(SET_WEB_ENC_FILTER);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoQueryWebEncIndex(void)
{
		*(BYTE *)(TX_MSG_BUFFER_SLOT_0) = 0x05;	// Length
		*(BYTE *)(TX_MSG_BUFFER_SLOT_1) = 
			QUERY_WEB_ENC_INDEX;			// Cmd

		*(WORD *)(TX_MSG_BUFFER_SLOT_2) = GetCurrEncoderIndex();

	StartReplyTransmission();
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoQueryWebEncPeriod(void)
{
		*(BYTE *)(TX_MSG_BUFFER_SLOT_0) = 0x05;	// Length
		*(BYTE *)(TX_MSG_BUFFER_SLOT_1) = 
			QUERY_WEB_ENC_PERIOD;			// Cmd

		*(WORD *)(TX_MSG_BUFFER_SLOT_2) = GetEncoderPeriod();

		if(GetEncoderPeriod() == 0x0000)
			++debug0;

	StartReplyTransmission();
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoQueryWebEncDirectionStatusFlag(void)
{
		*(BYTE *)(TX_MSG_BUFFER_SLOT_0) = 0x04;			// Length
		*(BYTE *)(TX_MSG_BUFFER_SLOT_1) = 
			QUERY_WEB_ENC_DIR_STAT_FLAG;	// Cmd

		if(IS_WEB_DIRECTION_SET())
			*(BOOL *)(TX_MSG_BUFFER_SLOT_2) = WEB_DIRECTION_FORWARD;
		else
			*(BOOL *)(TX_MSG_BUFFER_SLOT_2) = WEB_DIRECTION_REVERSED;

	StartReplyTransmission();
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoSetImpressionCount(void)
{	
		SetImpressionCount
			(*(DWORD *)(RX_MSG_BUFFER_SLOT_2));

	StartAckTransmission(SET_IMPRESSION_COUNT);
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoQueryWebEncImpressionCount(void)
{
		*(BYTE *)(TX_MSG_BUFFER_SLOT_0) = 0x07;	// Length
		*(BYTE *)(TX_MSG_BUFFER_SLOT_1) = 
			QUERY_WEB_IMPRESSION_COUNT;			// Cmd

		*(DWORD *)(TX_MSG_BUFFER_SLOT_2) =
			GetImpressionCount();

	StartReplyTransmission();
}



//////////////////////////////////////////////////
//
// Transport Handler Functions
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

near	BYTE	transportSystemSelect;

void	DoTransportFindHome(void)
{
	transportSystemSelect = 
		*(BYTE *)(RX_MSG_BUFFER_SLOT_2);
    
		SetTransportProcessDoneFlag(transportSystemSelect);

		SendMessageAndData
			(TCMR_SystemManagerID, TransportFindHome, transportSystemSelect);

	StartAckTransmission(TRANSPORT_FIND_HOME);
}

//////////////////////////////////////////////////
//
// NOTE: Command is ignored if the transport is
//			actively moving
//
//////////////////////////////////////////////////

void	DoTransportGoHome(void)
{
	transportSystemSelect = 
		*(BYTE *)(RX_MSG_BUFFER_SLOT_2);
    
		if(IS_TRANSPORT_IN_ERROR(transportSystemSelect))
		{
			if(!IS_TRANSPORT_CALIBRATED(transportSystemSelect))
				StartNakTransmission(TRANSPORT_UNCALIBRATED);
			else
			if(IS_TRANSPORT_IN_LIMIT(transportSystemSelect))
				StartNakTransmission(TRANSPORT_IN_LIMIT);
			else
			if(IS_TRANSPORT_IN_POSITION_ERROR(transportSystemSelect))
				StartNakTransmission(TRANSPORT_IN_POSITION_ERROR);
			else
				StartNakTransmission(TRANSPORT_IN_NOT_READY_STATE);
		}
		else
		{
			SetTransportProcessDoneFlag(transportSystemSelect);
			
			SendMessageAndData
				(TCMR_SystemManagerID, TransportGoHome, transportSystemSelect);
				
			StartAckTransmission(TRANSPORT_GO_HOME);
		}
}

//////////////////////////////////////////////////
//
// NOTE: Command is ignored if the transport is
//			actively moving
//
//////////////////////////////////////////////////
	
DWORD	positionRequested;

void	DoSetTransportPosition(void)
{
	transportSystemSelect = 
		*(BYTE *)(RX_MSG_BUFFER_SLOT_2);
    	
		positionRequested = *(DWORD *)(RX_MSG_BUFFER_SLOT_3);

		if(IS_TRANSPORT_IN_ERROR(transportSystemSelect))
		{
			if(!IS_TRANSPORT_CALIBRATED(transportSystemSelect))
				StartNakTransmission(TRANSPORT_UNCALIBRATED);
			else
			if(IS_TRANSPORT_IN_LIMIT(transportSystemSelect))
				StartNakTransmission(TRANSPORT_IN_LIMIT);
			else
			if(IS_TRANSPORT_IN_POSITION_ERROR(transportSystemSelect))
				StartNakTransmission(TRANSPORT_IN_POSITION_ERROR);
			else
				StartNakTransmission(TRANSPORT_IN_NOT_READY_STATE);
		}
		else
		{
			SetTransportProcessDoneFlag(transportSystemSelect);

			SetPositionRequest(positionRequested, transportSystemSelect);

			SendMessageAndData
				(TCMR_SystemManagerID, TransportGotoPosition, transportSystemSelect);
				
			StartAckTransmission(SET_TRANSPORT_POSITION);
		}
}

//////////////////////////////////////////////////
//
// NOTE: Command is ignored if the transport is
//			actively moving
//
//////////////////////////////////////////////////

void	DoJogTransport(void)
{
	signed	int	jogDistance;

	transportSystemSelect = 
		*(BYTE *)(RX_MSG_BUFFER_SLOT_2);

		jogDistance = *(signed int *)(RX_MSG_BUFFER_SLOT_3);

		if(IS_TRANSPORT_IN_ERROR(transportSystemSelect))
		{
			if(!IS_TRANSPORT_CALIBRATED(transportSystemSelect))
				StartNakTransmission(TRANSPORT_UNCALIBRATED);
			else
			if(IS_TRANSPORT_IN_LIMIT(transportSystemSelect))
				StartNakTransmission(TRANSPORT_IN_LIMIT);
			else
			if(IS_TRANSPORT_IN_POSITION_ERROR(transportSystemSelect))
				StartNakTransmission(TRANSPORT_IN_POSITION_ERROR);
			else
				StartNakTransmission(TRANSPORT_IN_NOT_READY_STATE);
		}
		else
		{
			SetTransportProcessDoneFlag(transportSystemSelect);

			SetPositionRequest(jogDistance, transportSystemSelect);

			SendMessageAndData
				(TCMR_SystemManagerID, TransportJog, transportSystemSelect);
				
			StartAckTransmission(JOG_TRANSPORT);
		}
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoQueryTransportPosition(void)
{
	transportSystemSelect = 
		*(BYTE *)(RX_MSG_BUFFER_SLOT_2);
    
		*(BYTE *)(TX_MSG_BUFFER_SLOT_0) = 0x07;	// Length
		*(BYTE *)(TX_MSG_BUFFER_SLOT_1) = 
			QUERY_TRANSPORT_POSITION;			// Cmd

		if(transportSystemSelect == TOP_TRANSPORT_SEL)
		{
			if(IS_TRANSPORT_CALIBRATED(TOP_TRANSPORT_SEL)&&
				!IS_TRANSPORT_IN_LIMIT(TOP_TRANSPORT_SEL))
			{
				*(DWORD *)(TX_MSG_BUFFER_SLOT_2) =
					GetCurrentPosition(TOP_TRANSPORT_SEL);

				StartReplyTransmission();
			}
			else
			{
				if(!IS_TRANSPORT_CALIBRATED(TOP_TRANSPORT_SEL))
					StartNakTransmission(TRANSPORT_UNCALIBRATED);
				else
					StartNakTransmission(TRANSPORT_IN_LIMIT);
			}
		}
		else
		{
			if(IS_TRANSPORT_CALIBRATED(BOTTOM_TRANSPORT_SEL)&&
				!IS_TRANSPORT_IN_LIMIT(BOTTOM_TRANSPORT_SEL))
			{
				*(DWORD *)(TX_MSG_BUFFER_SLOT_2) =
					GetCurrentPosition(BOTTOM_TRANSPORT_SEL);

				StartReplyTransmission();
			}
			else
			{
				if(!IS_TRANSPORT_CALIBRATED(BOTTOM_TRANSPORT_SEL))
					StartNakTransmission(TRANSPORT_UNCALIBRATED);
				else
					StartNakTransmission(TRANSPORT_IN_LIMIT);
			}
		}
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoQueryTransportState(void)
{
	transportSystemSelect = 
		*(BYTE *)(RX_MSG_BUFFER_SLOT_2);	
	
	*(BYTE *)(TX_MSG_BUFFER_SLOT_0) = 0x05;	// Length
	*(BYTE *)(TX_MSG_BUFFER_SLOT_1) = 
		QUERY_TRANSPORT_STATE;				// Cmd

		if(transportSystemSelect == TOP_TRANSPORT_SEL)
		{
			*(int *)(TX_MSG_BUFFER_SLOT_2) = 
				GetMachineState(TopTransportHandlerID);

			*(BOOL *)(TX_MSG_BUFFER_SLOT_3) = 
				GetCurrTopTransportStatus();
		}
		else
		{		
			*(int *)(TX_MSG_BUFFER_SLOT_2) = 
				GetMachineState(BottomTransportHandlerID);

			*(BOOL *)(TX_MSG_BUFFER_SLOT_3) = 
				GetCurrBottomTransportStatus();
		}

	StartReplyTransmission();
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoQueryPressStatus(void)
{
	*(BYTE *)(TX_MSG_BUFFER_SLOT_0) = 0x05;	// Length
	*(BYTE *)(TX_MSG_BUFFER_SLOT_1) = 
		QUERY_PRESS_STATUS;					// Cmd

		*(BYTE *)(TX_MSG_BUFFER_SLOT_2) = 
			GetCurrPressStatus();
		*(BYTE *)(TX_MSG_BUFFER_SLOT_3) = 
			GetLatchedPressStatus();

		ResetLatchedPressStatus();

	StartReplyTransmission();
}


//////////////////////////////////////////////////
//
// STATE MACHINE MATRIX DEFINITIONS
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_PCH_IDLE)
	EV_HANDLER(GoSystemParameterConfig, PCH_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_PCH_ACTIVE_CONFIGURING)
	EV_HANDLER(PcCommandReceived, PCH_exitB),
	EV_HANDLER(GoActive, PCH_exitC)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_PCH_WAIT_GOACTIVE_DONE)
	EV_HANDLER(GoActiveDone, PCH_exitC1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_PCH_ACTIVE)
	EV_HANDLER(PcCommandReceived, PCH_exitD)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_PCH_COMMAND_SEARCH)
	EV_HANDLER(SearchNextCommandEntry, PCH_exitE)
STATE_TRANSITION_MATRIX_END;

// 
// VERY IMPORTANT : 
//		State Entry definition order MUST match the 
//		order of the state definition in the .H File 
//

SM_RESPONSE_ENTRY(PCH_Entry)
	STATE(_PCH_IDLE)				,
	STATE(_PCH_ACTIVE_CONFIGURING)	,
	STATE(_PCH_WAIT_GOACTIVE_DONE)	,
	STATE(_PCH_ACTIVE)				,
	STATE(_PCH_COMMAND_SEARCH)
SM_RESPONSE_END




