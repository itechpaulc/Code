
//
// #include "spucomm.h"
//


#define		INTERBYTE_DELAY			2

#define		REPLY_DELAY				4



extern	BYTE	CommBuffer[CBUFF_SIZE];

extern	BYTE	PCU_ID;



#define		LvlSftXmitEnable     	(PORT_A.XMIT_ENABLE = 1)
#define		LvlSftXmitDisable		(PORT_A.XMIT_ENABLE = 0)

#define		UartXmitEnable			(SCCR2.TE 	= TRUE)

#define		XmitDoneIrqEnable		(SCCR2.TCIE = TRUE)
#define		XmitDoneIrqDisable		(SCCR2.TCIE = FALSE)





//////////////////////////////////////////////////
//
// Private Members
//
//////////////////////////////////////////////////


BYTE	byteCount;

BYTE	Length;



//////////////////////////////////////////////////
//
// Virtual Message Interface
//
//////////////////////////////////////////////////

void	SetBaudRate(BYTE baudRate)
{
	BAUD  = baudRate;

	SCCR1 = 0x00;	// 1 start, 8 data, 1 stop
}



//////////////////////////////////////////////////
//

void	ClearCommBuffer(void) {

	for(Length=0; Length<CBUFF_SIZE ;Length++)
		CommBuffer[Length] = 0x00;
}




//////////////////////////////////////////////////
//
// Private Helper Function
//
//////////////////////////////////////////////////

void	SynchronizeComm(void)
{
	byteCount = 0;
}


void	TransmitMode(void) {

	SCCR2 = 0x00;

	LvlSftXmitEnable;

	UartXmitEnable;
}


void	ReceiveMode(void)
{
	SCCR2 		= 0x00;

	LvlSftXmitDisable;

	SCCR2.RIE 	= TRUE;
	SCCR2.RE  	= TRUE;
}


void	DisableComm(void)
{
	SCCR2 = 0x00;

	LvlSftXmitDisable;
}


void	SendByte(BYTE byteToSend)
{
	TDR = byteToSend;

	XmitDoneIrqEnable;
}



//////////////////////////////////////////////////
//
// CRC calculation routine
//
//////////////////////////////////////////////////

/*

; This routine performs the CRC calculation for a given packet.
; This applies for packets to be sent or packets received.
;
; CommBuffer :
;   contains the data from which the CRC (16 bit) value
;	will be derived.
;
; TMPC1 =  PacketLength - 1
;
;
;		TRANSMISSION:
;
;			When this routine is completed
;  			CRC Bytes CRCGX1 and CRCGX2 will contain
;			the CRC word to be used for transmission.
;			Make sure the last 2 bytes of CommBuff are
;			cleared before calling this routine
;
;		CRCGX1 = LO CRC
;		CRCGX2 = HI CRC
;
;
;		RECEPTION:
;
;			After reception of a packet set TMPC1 and call
;			this routine. If CRCGX1 and CRCGX2 are both 0x00
;			after the routine, then the CRC check was ok,
;			the packet is ok.
;

*/


BOOL	IsCrcOk(void)
{
	ProcessCRC();

	return ((CRCGX1 == 0) && (CRCGX2 == 0)) ?
		TRUE : FALSE ;
}

void	ProcessCRC(void)
{
#asm

CRCPX2   EQU      $C0               ; Constants For CRC
CRCPX1   EQU      $02               ; calculations
CRCPX0   EQU      $80               ;

		LDA		CommBuffer			; Length Slot
		DECA
		STA		TMPC1

		CLRX
		LDA		CommBuffer,x
		STA		CRCGX2
		INCX
		LDA		CommBuffer,x
		STA		CRCGX1
		INCX
		LDA		CommBuffer,x
		STA		CRCGX0
		LDA		#$7
		STA		CRCNT1
		JSR		S1
CRC0:
		INCX
		LDA		CommBuffer,x
		STA		TMPD1
		CLC
		RORA
		ORA		CRCGX0
		STA		CRCGX0
		LDA		#$7
		STA		CRCNT1
		JSR		S1

		INC		CRCNT1

		BRSET	0,TMPD1,CRC1
		BCLR	6,CRCGX0
		BRA		CRC2
CRC1:
		BSET	6,CRCGX0

CRC2:	JSR		S1

CRC3:
		CPX		TMPC1
		BNE		CRC0
		JSR		CRCROT
		BRA		CRC_GenDone
S1:
		BRSET	7,CRCGX2,S2

		JSR		CRCROT
		DEC		CRCNT1
		BNE		S1
S2:
		JSR		CRCXOR
		LDA		CRCNT1
		BNE		S1
		RTS
CRCROT:
		ROL		CRCGX0
		ROL		CRCGX1
		ROL		CRCGX2
		RTS
CRCXOR:
		LDA		#CRCPX0
		EOR		CRCGX0
		STA		CRCGX0
		LDA		#CRCPX1
		EOR		CRCGX1
		STA		CRCGX1
		LDA		#CRCPX2
		EOR		CRCGX2
		STA		CRCGX2

		RTS

CRC_GenDone:

		RTS

#endasm
}



//////////////////////////////////////////////////
//
// State Machine Initialization
//
//////////////////////////////////////////////////

BYTE	Construct_SPUComm(void) {

		SynchronizeComm();

		DisableComm();

	return	SPUC_IDLE;
}



//////////////////////////////////////////////////
//
// SendCommBuff message, while in Idle
//
//////////////////////////////////////////////////

BYTE	SPUC_exitA(void)
{
		StartTimer(REPLY_DELAY);

	return SPUC_REPLY_WAIT;
}


//////////////////////////////////////////////////
//
// TimeOut, while in Reply Wait
//
//////////////////////////////////////////////////

BYTE	SPUC_exitB(void)
{
		SynchronizeComm();

		Length = CommBuffer[0];

		TransmitMode();

		XmitDoneIrqEnable;

	return SPUC_SENDING_PACKET;
}


//////////////////////////////////////////////////
//
// Byte Sent, while in Sending
//
//////////////////////////////////////////////////

BYTE	SPUC_exitC(void)
{
		if(byteCount == 0)	// Uart just enabled
		{
				// Send the first byte immediately

				if(CommBuffer[LENGTH_SLOT] == COMMAND_41)
					SendByte(ACK_COMMAND_41);
				else
				if(CommBuffer[LENGTH_SLOT] == PCU_TYPE_ID)
					SendByte(PCU_TYPE_ID);
				else
					SendByte(CommBuffer[byteCount]);

				byteCount++;

				return SAME_STATE;
		}

		// ByteCount > 0

		if(	(CommBuffer[LENGTH_SLOT] == COMMAND_41)
		   || (CommBuffer[LENGTH_SLOT] == PCU_TYPE_ID))
		{
			// One Byte Packet

			DisableComm();

			SendMessage(CommBuffSent,PCUMNGR_SM_ID);

			return SPUC_IDLE;
		}


	StartTimer(INTERBYTE_DELAY);

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// TimeOut, while in Sending
//
//////////////////////////////////////////////////

BYTE	SPUC_exitH(void)
{
		if((byteCount == Length))
		{
			DisableComm();

			SendMessage(CommBuffSent,PCUMNGR_SM_ID);

			return SPUC_IDLE;
		}

		SendByte(CommBuffer[byteCount]);

		byteCount++;

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// WaitForCommand message while in Idle
//
//////////////////////////////////////////////////

BYTE	SPUC_exitD(void)
{
		StartTimer(CFG_MEASURE_RATE);

	return SPUC_SYNCLO;
}


//////////////////////////////////////////////////
//
// TimeOut, while in Config Sync Low
//
//////////////////////////////////////////////////

BYTE	SPUC_exitE(void)
{
	StartTimer(CFG_MEASURE_RATE);

	if(CfgLineIsLow)
		return	SPUC_SYNCHI;

	return SAME_STATE;
}


//////////////////////////////////////////////////
//
// TimeOut, while in Config Sync Hi
//
//////////////////////////////////////////////////

BYTE	SPUC_exitF(void)
{
	if(CfgLineIsHi) {

		SynchronizeComm();

		ReceiveMode();

		return SPUC_CMD_WAIT;
	}

	StartTimer(CFG_MEASURE_RATE);

	return SAME_STATE;
}



//////////////////////////////////////////////////
//
// Go Idle while in Receiving
//
//////////////////////////////////////////////////

BYTE	SPUC_exitG(void)
{
	return SPUC_IDLE;
}




//////////////////////////////////////////////////
//
// SCI Interrupt Service Routine
//
//////////////////////////////////////////////////

void	__SCI_ISR()
{
	//  Check if Receive Data Register Full

	if (SCSR.RDRF == TRUE)
	{
		ac = SCSR;

		CommBuffer[byteCount] = RDR;

		if(byteCount == 0)
		{
			Length = CommBuffer[LENGTH_SLOT];

			if((Length > MAX_PACKET_LEN) && (Length != COMMAND_41))
			{
				DisableComm();
				SystemSCIstatus = SCI_INVALID_PACKET;
			}
		}
		else	/* byteCount > 0 */
		{
			if(Length == COMMAND_41)
			{
				DisableComm();
				SystemSCIstatus = SCI_CMD_READY;
			}

			if(Length == (byteCount+1))
			{
				if(PCU_ID == CommBuffer[ID_SLOT])
				{
					DisableComm();

					if(IsCrcOk())
						SystemSCIstatus = SCI_CMD_READY;
					else
						SystemSCIstatus = SCI_INVALID_PACKET;
				}
			}
		}

		byteCount++;
	}

		else

	// Check for Transmit Complete

	if (SCSR.TC == TRUE)
	{
		SystemSCIstatus = SCI_BYTE_SENT;

		XmitDoneIrqDisable;
	}
}


//////////////////////////////////////////////////
//
// State Matrix Table
//
//////////////////////////////////////////////////

#asm
_SPUCOMM:
	fdb 	xSPUC_IDLE_MATRIX
	fdb		xSPUC_REPLY_WAIT_MATRIX
	fdb		xSPUC_SENDING_MATRIX
	fdb		xSPUC_SYNCLO_MATRIX
	fdb		xSPUC_SYNCHI_MATRIX
	fdb		xSPUC_CMD_WAIT_MATRIX
#endasm


//////////////////////////////////////////////////
//
// Message/Exit Function Matrix Table
//
//////////////////////////////////////////////////

#asm
xSPUC_IDLE_MATRIX:
	fcb     SendCommBuff
	fdb		SPUC_exitA
	fcb		WaitForCommand
	fdb		SPUC_exitD
	fcb		NULL_MESSAGE
#endasm

#asm
xSPUC_REPLY_WAIT_MATRIX:
	fcb		TimeOut
	fdb		SPUC_exitB
	fcb     NULL_MESSAGE
#endasm

#asm
xSPUC_SENDING_MATRIX:
	fcb		ByteSent
	fdb		SPUC_exitC
	fcb		TimeOut
	fdb		SPUC_exitH
	fcb     NULL_MESSAGE
#endasm

#asm
xSPUC_SYNCLO_MATRIX:
	fcb		TimeOut
	fdb		SPUC_exitE
	fcb     NULL_MESSAGE
#endasm

#asm
xSPUC_SYNCHI_MATRIX:
	fcb		TimeOut
	fdb		SPUC_exitF
	fcb     NULL_MESSAGE
#endasm

#asm
xSPUC_CMD_WAIT_MATRIX:
	fcb     CommGoIdle
	fdb     SPUC_exitG
	fcb     NULL_MESSAGE
#endasm






