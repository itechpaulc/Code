
//
// #include "spucomm.h"
//



/////////////////////////////////////////////////////////////////////////////
//
//
//	$Header:   N:/pvcs52/projects/pcu100~1/spucomm.c_v   1.5   May 07 1997 09:01:54   Paul L C  $
//	$Log:   N:/pvcs52/projects/pcu100~1/spucomm.c_v  $
//
//   Rev 1.5   May 07 1997 09:01:54   Paul L C
//Made functions to be expanded "inline" to reduce RAM stack usage.
//Moved some variables into the header files for visibility. Optimized 
//the CRC routine to avoid the use of JSR or stack space.
//
//   Rev 1.4   May 02 1997 13:45:40   Paul L C
//Added error logging, changed some macros to follow
//convention and added comments.
//
//   Rev 1.3   Apr 11 1997 10:29:36   Paul L C
//Implemented processing of the GoIdle message.
//
//   Rev 1.2   Mar 25 1997 09:52:18   Paul L C
//CRC checking was taken out of the Comm ISR. This
//was causing problems with the 1msec timer updates
//since the CRC checking routine takes 1.75 msec to 
//finish.
//
//   Rev 1.1   Mar 18 1997 08:33:38   Paul L C
//CommBuffer indexing was going out of bounds. Added a
//check to prevent this. Also added a check for ID mismatches to
//synchronize the comm machine immediately.
//
//   Rev 1.0   Feb 26 1997 10:54:48   Paul L C
//Initial Revision
//
//
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

#define		INTERBYTE_DELAY			2

//////////////////////////////////////////////////
//
// Note : CRC calculation time is added to this
//        parameter.
//
//////////////////////////////////////////////////

#define		REPLY_DELAY				4


//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

extern	BYTE	CommBuffer[CBUFF_SIZE];

extern	BYTE	PCU_ID;


//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

#define		LvlSftXmitEnable     	(PORT_A.XMIT_ENABLE = 1)
#define		LvlSftXmitDisable		(PORT_A.XMIT_ENABLE = 0)

#define		UartXmitEnable			(SCCR2.TE 	= TRUE)

#define		XmitDoneIrqEnable		(SCCR2.TCIE = TRUE)
#define		XmitDoneIrqDisable		(SCCR2.TCIE = FALSE)





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
// Private Helper Functions
//
//////////////////////////////////////////////////

void	SynchronizeComm(void)
{
	byteCount = 0;
}


//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

void	TransmitMode(void) {

	SCCR2 = 0x00;

	LvlSftXmitEnable;

	UartXmitEnable;
}


//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

void	ReceiveMode(void)
{
	SCCR2 		= 0x00;

	LvlSftXmitDisable;

	SCCR2.RIE 	= TRUE;
	SCCR2.RE  	= TRUE;
}


//////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////

void	SendByte(BYTE byteToSend)
{
	TDR = byteToSend;

	XmitDoneIrqEnable;
}


/////////////////////////////////////////////////////////////
//
// CRC calculation routine :
//
// IMPORTANT : This routine takes 1.75 msec to complete
//				@2mhz cpu speed.
//
/////////////////////////////////////////////////////////////
//
//
// This routine performs the CRC calculation for a given packet.
// This applies for packets to be sent or packets received.
//
// CommBuffer :
//   contains the data from which the CRC (16 bit) value
//	will be derived.
//
// TMPC1 =  PacketLength - 1
//
//
//		TRANSMISSION:
//
//			When this routine is completed
//  			CRC Bytes CRCGX1 and CRCGX2 will contain
//			the CRC word to be used for transmission.
//			Make sure the last 2 bytes of CommBuff are
//			cleared before calling this routine
//
//		CRCGX1 = LO CRC
//		CRCGX2 = HI CRC
//
//
//		RECEPTION:
//
//			After reception of a packet set TMPC1 and call
//			this routine. If CRCGX1 and CRCGX2 are both 0x00
//			after the routine, then the CRC check was ok,
//			the packet is ok.
//
/////////////////////////////////////////////////////////////

BOOL	IsCrcOk(void)
{
	ProcessCRC();

		if((CRCGX1 != 0) || (CRCGX2 != 0)) {

			LogCRCerror;
			return	FALSE;
		}


	return TRUE;
}

/////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////

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
S1A:
		BRSET	7,CRCGX2,S2A

		ROL		CRCGX0              ;CRC ROT
		ROL		CRCGX1
		ROL		CRCGX2

		DEC		CRCNT1
		BNE		S1A
S2A:
		LDA		#CRCPX0				;CRC XOR
		EOR		CRCGX0
		STA		CRCGX0
		LDA		#CRCPX1
		EOR		CRCGX1
		STA		CRCGX1
		LDA		#CRCPX2
		EOR		CRCGX2
		STA		CRCGX2

		LDA		CRCNT1
		BNE		S1A

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
S1B:
		BRSET	7,CRCGX2,S2B

		ROL		CRCGX0              ;CRC ROT
		ROL		CRCGX1
		ROL		CRCGX2

		DEC		CRCNT1
		BNE		S1B

S2B:
		LDA		#CRCPX0				;CRC XOR
		EOR		CRCGX0
		STA		CRCGX0
		LDA		#CRCPX1
		EOR		CRCGX1
		STA		CRCGX1
		LDA		#CRCPX2
		EOR		CRCGX2
		STA		CRCGX2

		LDA		CRCNT1
		BNE		S1B

		INC		CRCNT1

		BRSET	0,TMPD1,CRC1
		BCLR	6,CRCGX0
		BRA		CRC2
CRC1:
		BSET	6,CRCGX0

CRC2:
S1C:
		BRSET	7,CRCGX2,S2C

		ROL		CRCGX0              ;CRC ROT
		ROL		CRCGX1
		ROL		CRCGX2

		DEC		CRCNT1
		BNE		S1C

S2C:
		LDA		#CRCPX0				;CRC XOR
		EOR		CRCGX0
		STA		CRCGX0
		LDA		#CRCPX1
		EOR		CRCGX1
		STA		CRCGX1
		LDA		#CRCPX2
		EOR		CRCGX2
		STA		CRCGX2

		LDA		CRCNT1
		BNE		S1C

CRC3:
		CPX		TMPC1
		BNE		CRC0

		ROL		CRCGX0              ;CRC ROT
		ROL		CRCGX1
		ROL		CRCGX2

CRC_GenDone:

#endasm

} // RTS



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

		if(IsCfgLineLow())
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
		if(IsCfgLineHi()) {

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
		DisableComm();

        CancelTimer();

	return SPUC_IDLE;
}



//////////////////////////////////////////////////
//
// SCI Interrupt Service Routine
//
//////////////////////////////////////////////////

void	__SCI_ISR()
{
	//////////////////////////////////////////////////
	//  Check if Receive Data Register Full
	//////////////////////////////////////////////////

	if (SCSR.RDRF == TRUE)
	{
		ac = SCSR;

		if(byteCount >= MAX_PACKET_LEN)
		{
			// Max Length reached, reset comm

			DisableComm();
			SystemSCIstatus = SCI_INVALID_PACKET;

			return;
		}
			else
		{
			CommBuffer[byteCount] = RDR;
		}

		if(byteCount == 0)
		{
			Length = CommBuffer[LENGTH_SLOT];

			if((Length > MAX_PACKET_LEN) && (Length != COMMAND_41))
			{
				// Invalid Length Byte

				LogLenOutOfRangeErr;

				DisableComm();
				SystemSCIstatus = SCI_INVALID_PACKET;
			}
		}
		else	// byteCount > 0
		{
			if(Length == COMMAND_41)
			{
				DisableComm();
				SystemSCIstatus = SCI_CMD_41_READY;
			}

			if(Length == (byteCount+1))
			{
				// We got the whole packet

				DisableComm();

				if(PCU_ID == CommBuffer[ID_SLOT])
				{
					SystemSCIstatus = SCI_CMD_READY;
				}
				else
				{
					// ID Mismatch, message not for us

					SystemSCIstatus = SCI_INVALID_PACKET;
				}
			}

		}

		byteCount++;
	}

		else

	//////////////////////////////////////////////////
	// Check for Transmit Complete
	//////////////////////////////////////////////////

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






