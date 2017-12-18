


//
// #include "pcumngr.h"
//



/////////////////////////////////////////////////////////////////////////////
//
//
//	$Header:   N:/pvcs52/projects/pcu100~1/pcumngr.c_v   1.10   Jun 23 1997 10:26:16   Paul L C  $
//	$Log:   N:/pvcs52/projects/pcu100~1/pcumngr.c_v  $
//
//   Rev 1.10   Jun 23 1997 10:26:16   Paul L C
//Corrected an error where the PCU does not  move if ANY of
//the limits are asserted. This is now change so that the PCU
//can get out of the limit position.
//
//   Rev 1.9   May 16 1997 15:02:20   Paul L C
//A correction was made where the Staus LED should only starts 
//SLOW PULSING after the PCU ID has been assigned. Changed a 
//function name IsLocalControlActive fore consistentency.
//
//   Rev 1.8   May 07 1997 09:15:42   Paul L C
//Made functions to be expanded "inline" to reduce RAM stack usage.
//These are now defined in the header file. Adjusted STACK BOUND.
//
//   Rev 1.7   May 02 1997 13:36:08   Paul L C
//Implemented Error Logging macros. Deleted functions that
//supported PCU Mode. PCU Mode is now used as a flag for local
//control activation only. Made changes to make macros follow
//the same general convention. Deleted references to command 0x4A.
//The status LED will go back to normal from an error cadence after
//the SPU request to clear PCU error command is processed. Clear
//CommBuffer was integrated with Build7ByteHeader. Added checks
//to Activate State Machines when associated commands are received
//from the SPU. Reset Servo command will not be supported. Implemented 
//support for the Hardware Limit Input Checking. Added stack checking 
//functions. Command 0x55 now also contains the DistanceToNudge 
//parameter. Command 0x59 now also contains data of the OutputStates.
//
//
//   Rev 1.6   Apr 25 1997 09:58:08   Paul L C
//Made changes to accomodate CPU port remapping.
//
//   Rev 1.5   Apr 11 1997 10:06:52   Paul L C
//Cleaned up duplicate procedure calls.
//
//   Rev 1.4   Mar 25 1997 09:43:28   Paul L C
//Added : If config measurement error occurs, then DO NOT pass the
//config line.
//
//   Rev 1.3   Mar 18 1997 08:27:34   Paul L C
//Union requires .BothBytes (bb) to be specified. This error was
//flagged by the new compiler. LED stat machine is now activated
//after the PCU ID has been assigned. Corrected an error where the 
//PCU ID was continously changed as the SPU kept sending the
//ID assignment sequences. 
//
//
//   Rev 1.2   Mar 10 1997 11:09:52   Paul L C
//Implemented system check rate to update led based on
//the system status. Improved PMM_exitF structure. Added 
//command62 to GetPCUMode. Added Command 61 support.
//
//   Rev 1.1   Mar 04 1997 15:35:48   Paul L C
//Added the request PCU RevNo feature. Test for AtoD settle and
//MotorLimitsSet are implemented. This is to check if motor moves
//should be done. Fixed an error to tell the comm machine to just get 
//the next command when motor is not yet ready to move (AtoD settle
//failed or MotorLimits not set). 
//
//   Rev 1.0   Feb 26 1997 10:54:44   Paul L C
//Initial Revision
//
//
/////////////////////////////////////////////////////////////////////////////



extern	BYTE	CommBuffer[CBUFF_SIZE];

extern	BYTE	CRCGX1;
extern	BYTE	CRCGX2;


//////////////////////////////////////////////////
//
// Private Members
//
//////////////////////////////////////////////////

BYTE	ConfigTimer;


//////////////////////////////////////////////////
//
// State Machine Initialization
//
//////////////////////////////////////////////////

BYTE	Construct_PCUManager(void) {

		ConfigTimer = 0;

		PCU_ID = NO_ID_ASSIGNED;

		StartTimer(POWERUP_DELAY);

		ClearMotorActiveLED;

		ClearAllErrorRegisters;

	return	PCUM_IDLE;
}


//////////////////////////////////////////////////
//
// TimeOut message, while in Idle
//
//////////////////////////////////////////////////

BYTE	PMM_exitA(void) {

		StartTimer(CFG_MEASURE_RATE);

	return 	PCUM_CFGHI_WAIT;
}


//////////////////////////////////////////////////
//
// TimeOut message, while in ConfigHi Wait
//
//////////////////////////////////////////////////

BYTE	PMM_exitB(void)
{
	StartTimer(CFG_MEASURE_RATE);

	if(IsCfgLineHi()) {

		ConfigTimer++;

		return	PCUM_CFG_MEASURE;
	}

	return 	SAME_STATE;
}


//////////////////////////////////////////////////
//
// CommandReceived message
//
//////////////////////////////////////////////////

BYTE	PMM_exitC(void)
{
	if(CommBuffer[LENGTH_SLOT] == COMMAND_41)
	{
		// A 2 byte packet was received for id setting

		if(PCU_ID == NO_ID_ASSIGNED) {

			SendMessage(LedGoActive,LED_SM_ID);
			PCU_ID = CommBuffer[ID_SLOT];
			PassConfigLine;
		}

		SendMessage(WaitForCommand,SPUCOMM_SM_ID);
	}
		else

	switch(CommBuffer[CMD_SLOT])
	{
		// OLD servo command support

		case	0x41 : DoCommand41();	break;
		case	0x42 : DoCommand42();	break;
		case	0x43 : DoCommand43();	break;
		case	0x44 : DoCommand44();	break;
		case	0x48 : DoCommand48();	break;
		case	0x49 : DoCommand49();	break;
		case	0x4B : DoCommand4B();	break;
		case	0x4E : DoCommand4E();	break;
		case	0x51 : DoCommand51();	break;

		// PCU specific commands

		case	0x52 : DoCommand52();	break;
		case	0x53 : DoCommand53();	break;
		case	0x54 : DoCommand54();	break;
		case	0x55 : DoCommand55();  	break;
		case	0x56 : DoCommand56();	break;
		case	0x57 : DoCommand57();	break;
		case	0x58 : DoCommand58();	break;
		case	0x59 : DoCommand59();	break;
		case	0x5A : DoCommand5A();	break;
		case	0x5B : DoCommand5B();	break;
		case	0x5C : DoCommand5C();	break;
		case	0x5D : DoCommand5D();	break;
		case	0x5E : DoCommand5E();	break;
		case	0x5F : DoCommand5F();	break;
		case	0x60 : DoCommand60();	break;
		case	0x61 : DoCommand61();	break;
		case	0x62 : DoCommand62();	break;

		default : UnknownCommand();
	}

	return	PCUM_ACTIVE;
}



//////////////////////////////////////////////////
//
// TimeOut message for system check.
//
//////////////////////////////////////////////////

BYTE	PMM_exitD(void) {

		// Make sure we are not displaying transmit

		if(GetState(LED_SM_ID) != LED_TRANSMIT)
			if(SystemErrors || ErrorRegister) {

				// Check if there are any errors,
				// Update status LED to indicate error

				SetCadence(STROBED);

			} else {

				// cadence before changing it to NORMAL

				SetCadence(SLOWPULSE);
			}

		// Make sure STACK RAM is still ok

		if(!IsStackOk())
			TrapStackError();

		StartTimer(SYSTEM_CHECK_RATE);

	return	PCUM_ACTIVE;
}


//////////////////////////////////////////////////
//
// InvalidPacket message
//
//////////////////////////////////////////////////

BYTE	PMM_exitE(void) {

		SendMessage(WaitForCommand,SPUCOMM_SM_ID);

	return	PCUM_ACTIVE;
}


//////////////////////////////////////////////////
//
// TimeOut while in Config Line Measure
//
//////////////////////////////////////////////////

BYTE	PMM_exitF(void)
{
		StartTimer(CFG_MEASURE_RATE);

		ConfigTimer++;

		if(IsCfgLineLow())
		{
			// One complete config pulse
			// has been detected

			DoNotPassConfigLine;

			if(ConfigTimer > BAUD_48_LO && ConfigTimer < BAUD_48_HI) {

				SetBaudRate(BAUD_4800);
				goto ConfigMeasureOK;
			}

			if(ConfigTimer > BAUD_96_LO && ConfigTimer < BAUD_96_HI) {

				SetBaudRate(BAUD_9600);
				goto ConfigMeasureOK;
			}

				// Error : Config Not OK

				LogCFGmeasureErr;

			return	PCUM_ACTIVE;
		}

		return	SAME_STATE;

ConfigMeasureOK:

		SystemStatus.SYS_CONFIGURED = TRUE;

	return	PCUM_2CFGHI_WAIT;
}


//////////////////////////////////////////////////
//
// TimeOut while in 2nd Config Hi Wait
//
//////////////////////////////////////////////////

BYTE	PMM_exitG(void)
{
		if(IsCfgLineHi()) {

			if(SystemStatus.SYS_CONFIGURED == TRUE)
				SendMessage(WaitForCommand,SPUCOMM_SM_ID);

			StartTimer(SYSTEM_CHECK_RATE);

			return	PCUM_ACTIVE;
		}

		StartTimer(CFG_MEASURE_RATE);

	return	SAME_STATE;
}


//////////////////////////////////////////////////
//
// MotorMove Done and CommBuff Sent
// message while in Active
//
//////////////////////////////////////////////////

BYTE	PMM_exitH(void)
{
		ClearMotorActiveLED;

		SendMessage(WaitForCommand,SPUCOMM_SM_ID);

	return	SAME_STATE;
}


//////////////////////////////////////////////////
//
// UpDownSWdetected message while in Active
//
//////////////////////////////////////////////////

BYTE	PMM_exitI(void)
{
	// Make Sure the Motor is not running before
	// allowing local control to make changes

	if(GetState(MOTOR_CTRLR_SM_ID) != MCM_IDLE)
		SendMessage(UpDnSwGoActive,UPDOWN_MONITOR_ID);

	return	SAME_STATE;
}


//////////////////////////////////////////////////
//
// Motor Nudge Up message while in Active
//
//////////////////////////////////////////////////

BYTE	PMM_exitJ(void)
{
		if(IsInMaxHardwareLimit()) {

			LogHWlimitStall;

			SendMessage(WaitForCommand,SPUCOMM_SM_ID);

		} else {

			LATCH_LOCAL_MOVE;

			// Check motor limits

			if(!IsInMaxPosition())
				SendMessage(MotorNudgeUp,MOTOR_CTRLR_SM_ID);
		}

	return	SAME_STATE;
}


//////////////////////////////////////////////////
//
//  Motor Nudge Down message while in Active
//
//////////////////////////////////////////////////

BYTE	PMM_exitK(void)
{
		if(IsInMinHardwarelimit()) {

			LogHWlimitStall;

			SendMessage(WaitForCommand,SPUCOMM_SM_ID);

		} else {

			LATCH_LOCAL_MOVE;

			// Check motor limits

			if(!IsInMinPosition())
				SendMessage(MotorNudgeDown,MOTOR_CTRLR_SM_ID);
		}

	return	SAME_STATE;
}


//////////////////////////////////////////////////
//
// From UpDown Switch
// Motor Stop Nudging, message while in Active
//
//////////////////////////////////////////////////

BYTE	PMM_exitL(void)
{
		SendMessage(MotorGoIdle,MOTOR_CTRLR_SM_ID);

	return	SAME_STATE;
}



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// Private Helper Functions	Definition
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
// SPU Requested for a reply from the first
// servo on the string
//
//////////////////////////////////////////////////

void	DoCommand41(void) {

	// Send SPECIAL ack

	CommBuffer[LENGTH_SLOT] = COMMAND_41;

	SendCBuff();
}


//////////////////////////////////////////////////
//
// SPU Requested to change the PCU ID
// (unit number)
//
//////////////////////////////////////////////////

void	DoCommand42(void) {

		PCU_ID = CommBuffer[DATA_1_SLOT];

	SendMessage(WaitForCommand,SPUCOMM_SM_ID);
}


//////////////////////////////////////////////////
//
// SPU Requested to Close the Config Line
// ( to allow the next servo in
//   the string to respond )
//
//////////////////////////////////////////////////

void	DoCommand43(void) {

		PassConfigLine;

	SendMessage(WaitForCommand,SPUCOMM_SM_ID);
}


//////////////////////////////////////////////////
//
// SPU Requested to Open the Config Line
// ( to prevent the next servo in
//   the string from responding )
//
//////////////////////////////////////////////////

void	DoCommand44(void) {

		DoNotPassConfigLine;

	SendMessage(WaitForCommand,SPUCOMM_SM_ID);
}


//////////////////////////////////////////////////
//
// SPU Requested a reply from the last
// servo;
//
// The PCU also sends back the ErrorRegister
// which indicates the type of problems that the
// PCU just encountered.
//
//////////////////////////////////////////////////

void	DoCommand48(void) {

	Build7ByteHeader();

		CommBuffer[ERROR_REGISTER_SLOT] = ErrorRegister;

	BuildCRC();

	SendCBuff();
}


//////////////////////////////////////////////////
//
// SPU Requested for the "Servo Type"
//
//////////////////////////////////////////////////

void	DoCommand49(void) {

	// Send SPECIAL ack

	CommBuffer[LENGTH_SLOT] = PCU_TYPE_ID;

	SendCBuff();
}



//////////////////////////////////////////////////
//
// Set Moving Timeout command from SPU
//
//////////////////////////////////////////////////

void	DoCommand4B(void) {

	// Activate AtoD Driver if not yet

	if(GetState(ATOD_DRIVER_SM_ID) == ATDM_IDLE)
		SendMessage(AtoDGoActive,ATOD_DRIVER_SM_ID);

		SetMovingTimeOut(CommBuffer[DATA_1_SLOT]);

	SendMessage(WaitForCommand,SPUCOMM_SM_ID);
}


//////////////////////////////////////////////////
//
// SPU Requested PCU to clear its status
// 	registers, error registers...
//
//////////////////////////////////////////////////

void	DoCommand4E(void) {

		ClearAllErrorRegisters;

	SendMessage(WaitForCommand,SPUCOMM_SM_ID);
}


//////////////////////////////////////////////////
//
// Set Coasting Timeout command from SPU
//
//////////////////////////////////////////////////

void	DoCommand51(void) {

	// Activate AtoD Driver if not yet

	if(GetState(ATOD_DRIVER_SM_ID) == ATDM_IDLE)
			SendMessage(AtoDGoActive,ATOD_DRIVER_SM_ID);

		SetCoastTimeOut(CommBuffer[DATA_1_SLOT]);

	SendMessage(WaitForCommand,SPUCOMM_SM_ID);
}



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// PCU specific commands
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
// Get PCU Revision
//
//////////////////////////////////////////////////

void	DoCommand52(void) {

	Build7ByteHeader();

		CommBuffer[DATA_1_SLOT] = PCU_REV_HI;
		CommBuffer[DATA_2_SLOT] = PCU_REV_LO;

	BuildCRC();
	SendCBuff();
}


//////////////////////////////////////////////////
//
// Motor, Move to target Position
//
//////////////////////////////////////////////////

void	DoCommand53(void) {

		// Make sure local motor control is not active,
		// Limits are set, Motor is not at the limts
		// and AtoD has settled

		if((GetState(UPDOWN_MONITOR_ID) == UDM_IDLE)
			&&(IsPositionLimitSet())
			&&(IS_ATOD_SETTLED())
			&&(!IsLocalControlActive())) {

			LATCH_REMOTE_MOVE;

			SetMotorActiveLED;

			tempWORD.bb.HiByte  = CommBuffer[DATA_1_SLOT];
			tempWORD.bb.LowByte = CommBuffer[DATA_2_SLOT];

			SetTarget(tempWORD.Word);

			SendMessage(MoveToTarget, MOTOR_CTRLR_SM_ID);

		} else {

			// Activate AtoD Driver if not yet

			if(GetState(ATOD_DRIVER_SM_ID) == ATDM_IDLE)
				SendMessage(AtoDGoActive,ATOD_DRIVER_SM_ID);

			SendMessage(WaitForCommand,SPUCOMM_SM_ID);
		}
}


//////////////////////////////////////////////////
//
// SPU Requested for the AtoD Reading
// or the Absolute Motor Position
//
//////////////////////////////////////////////////

#define		LAST_MOTOR_MOVE_BIT		0x80
#define		MOTOR_MOVED_LOCALLY		0x40

void	DoCommand54(void) {

	// Activate AtoD Driver if not yet

	if(GetState(ATOD_DRIVER_SM_ID) == ATDM_IDLE)
			SendMessage(AtoDGoActive,ATOD_DRIVER_SM_ID);

	// Reply only when AtoD has been activated
	// and its reading has settled.

	if(IS_ATOD_SETTLED()) {

		Build7ByteHeader();

			CommBuffer[STATUS_SLOT] = GetMotorStatus();

			tempWORD.Word = GetCurrPosition();

			CommBuffer[DATA_1_SLOT] = tempWORD.bb.HiByte;
			CommBuffer[DATA_2_SLOT] = tempWORD.bb.LowByte;

			// Last Motor Move Direction Bit

			if(SystemStatus.MOTOR_REVERSE == TRUE)
				CommBuffer[DATA_1_SLOT] |= LAST_MOTOR_MOVE_BIT;	// SET

			if(SystemStatus.MOTOR_LOCAL_MOVE == TRUE)
				CommBuffer[DATA_1_SLOT] |= MOTOR_MOVED_LOCALLY;	// SET

		BuildCRC();

		SendCBuff();

	} else {

		SendMessage(WaitForCommand,SPUCOMM_SM_ID);
	}
}


//////////////////////////////////////////////////
//
// Set Nudge Parameters
//
//////////////////////////////////////////////////

void	DoCommand55(void) {

		// Activate AtoD Driver if not yet

		if(GetState(ATOD_DRIVER_SM_ID) == ATDM_IDLE)
			SendMessage(AtoDGoActive,ATOD_DRIVER_SM_ID);

		SetNudgeOnTime(CommBuffer[DATA_1_SLOT]);
		SetDistanceToNudge(CommBuffer[DATA_2_SLOT]);

	SendMessage(WaitForCommand,SPUCOMM_SM_ID);
}


//////////////////////////////////////////////////
//
// Nudge To Target Position
//
//////////////////////////////////////////////////

void	DoCommand56(void) {

		// Make sure local motor control is not active,
		// Limits are set, Motor is not at the limts
		// and AtoD has settled

		if((GetState(UPDOWN_MONITOR_ID) == UDM_IDLE)
			&&(IsPositionLimitSet())
			&&(IS_ATOD_SETTLED())
			&&(!IsLocalControlActive())) {

			LATCH_REMOTE_MOVE;

			SetMotorActiveLED;

			tempWORD.bb.HiByte  = CommBuffer[DATA_1_SLOT];
			tempWORD.bb.LowByte = CommBuffer[DATA_2_SLOT];

			SetTarget(tempWORD.Word);

			SendMessage(NudgeToTarget,MOTOR_CTRLR_SM_ID);

		} else {

			// Activate AtoD Driver if not yet

			if(GetState(ATOD_DRIVER_SM_ID) == ATDM_IDLE)
				SendMessage(AtoDGoActive,ATOD_DRIVER_SM_ID);

			SendMessage(WaitForCommand,SPUCOMM_SM_ID);
		}
}


//////////////////////////////////////////////////
//
// Nudge X times
//
//////////////////////////////////////////////////

#define		NUDGE_DIR_FORWARD	1
#define		NUDGE_DIR_REVERSE   0

void	DoCommand57(void) {

	// Make sure local motor control is not active

	if(GetState(UPDOWN_MONITOR_ID) == UDM_IDLE)
	{
		// Check if "X" times is not zero

		if(CommBuffer[DATA_1_SLOT] != 0)
		{
			SetNudgeCount(CommBuffer[DATA_1_SLOT]);

			if(CommBuffer[DATA_2_SLOT] == NUDGE_DIR_REVERSE)
				SendMessage(MotorNudgeDown,MOTOR_CTRLR_SM_ID);
			else
				SendMessage(MotorNudgeUp,MOTOR_CTRLR_SM_ID);

			LATCH_REMOTE_MOVE;

			SetMotorActiveLED;
		}
	}
}


//////////////////////////////////////////////////
//
// Set Local Nudge Factors
//
//////////////////////////////////////////////////

#define		STARTING_ON_TIME	0x0F
#define		ACCEL_FACTOR		0xF0

void	DoCommand58(void) {

		// Activate Digital Input Monitor Driver if not yet

		if(GetState(DIGI_IN_MONITOR_ID) == DIM_IDLE)
			SendMessage(DigiInGoActive,DIGI_IN_MONITOR_ID);

		SET_PM_LOCAL();	// The system can perform
						// local positioning

		SetStartNudgeOnTime
			(CommBuffer[DATA_1_SLOT] & STARTING_ON_TIME);

		SetNudgeAccelFactor
			((CommBuffer[DATA_1_SLOT] & ACCEL_FACTOR) >> 4);

		SetEndNudgeOnTime(CommBuffer[DATA_2_SLOT]);

	SendMessage(WaitForCommand,SPUCOMM_SM_ID);
}


//////////////////////////////////////////////////
//
// SPU Requested for the Digital
// Input and Output Status
//
//////////////////////////////////////////////////

void	DoCommand59(void) {

	// Activate Digital Input Monitor Driver if not yet

	if(GetState(DIGI_IN_MONITOR_ID) == DIM_IDLE)
		SendMessage(DigiInGoActive,DIGI_IN_MONITOR_ID);

	Build7ByteHeader();

		CommBuffer[DATA_1_SLOT] = (GetLatchedInputState()) << 4;

		CommBuffer[DATA_1_SLOT] |= (GetCurrInputState());

		CommBuffer[DATA_2_SLOT] = GetDigitalOutputState();

	BuildCRC();

	SendCBuff();
}


//////////////////////////////////////////////////
//
// SPU Command to Control Outputs
//
//////////////////////////////////////////////////

void	DoCommand5A(void) {

		// Enable the digital ouput
		// controller machine

		if(GetState(DIGI_OUT_CNTRLR_ID) == DOC_IDLE)
			SendMessage(DigiOutGoActive,DIGI_OUT_CNTRLR_ID);

		SetDigitalOutputs(CommBuffer[DATA_1_SLOT]);

	SendMessage(WaitForCommand,SPUCOMM_SM_ID);
}


//////////////////////////////////////////////////
//
// Set Min Move factor
//
//////////////////////////////////////////////////

void	DoCommand5B(void) {

	// Activate AtoD Driver if not yet

	if(GetState(ATOD_DRIVER_SM_ID) == ATDM_IDLE)
			SendMessage(AtoDGoActive,ATOD_DRIVER_SM_ID);

		SetMinMove(CommBuffer[DATA_1_SLOT]);

	SendMessage(WaitForCommand,SPUCOMM_SM_ID);
}


//////////////////////////////////////////////////
//
// PCU command to Set the Minimum Motor Position
//
//////////////////////////////////////////////////

void	DoCommand5C(void) {

	// Activate AtoD Driver if not yet

	if(GetState(ATOD_DRIVER_SM_ID) == ATDM_IDLE)
			SendMessage(AtoDGoActive,ATOD_DRIVER_SM_ID);

		tempWORD.bb.HiByte  = CommBuffer[DATA_1_SLOT];
		tempWORD.bb.LowByte = CommBuffer[DATA_2_SLOT];

		SetMinPosition(tempWORD.Word);

	SendMessage(WaitForCommand,SPUCOMM_SM_ID);
}


//////////////////////////////////////////////////
//
// PCU command to Set the Maximum Motor Position
//
//////////////////////////////////////////////////

void	DoCommand5D(void) {

	// Activate AtoD Driver if not yet

	if(GetState(ATOD_DRIVER_SM_ID) == ATDM_IDLE)
			SendMessage(AtoDGoActive,ATOD_DRIVER_SM_ID);

		tempWORD.bb.HiByte  = CommBuffer[DATA_1_SLOT];
		tempWORD.bb.LowByte = CommBuffer[DATA_2_SLOT];

		SetMaxPosition(tempWORD.Word);

	SendMessage(WaitForCommand,SPUCOMM_SM_ID);
}


//////////////////////////////////////////////////
//
// SPU requested for the Sheet Count
//
//////////////////////////////////////////////////

void	DoCommand5E(void) {

	// Enable the sheet counter
	// controller machine

	if(GetState(SHEET_COUNTER_ID) == SCM_IDLE)
		SendMessage(SCounterGoActive,SHEET_COUNTER_ID);

	Build7ByteHeader();

		tempWORD.Word = GetSheetCount();

		CommBuffer[DATA_1_SLOT] = tempWORD.bb.HiByte;
		CommBuffer[DATA_2_SLOT] = tempWORD.bb.LowByte;

	BuildCRC();

	SendCBuff();
}


//////////////////////////////////////////////////
//
// SPU command to clear the sheet count register
//
//////////////////////////////////////////////////

void	DoCommand5F(void) {

	// Enable the sheet counter
	// controller machine

	if(GetState(SHEET_COUNTER_ID) == SCM_IDLE)
		SendMessage(SCounterGoActive,SHEET_COUNTER_ID);

		ClearSheetCount();

	SendMessage(WaitForCommand,SPUCOMM_SM_ID);
}


//////////////////////////////////////////////////
//
// SPU command to Set the sheet counter
//  scanning rate
//
//////////////////////////////////////////////////

void	DoCommand60(void) {

		// Enable the sheet counter
		// controller machine

		if(GetState(SHEET_COUNTER_ID) == SCM_IDLE)
			SendMessage(SCounterGoActive,SHEET_COUNTER_ID);

		SetSCsamplingRate(CommBuffer[DATA_1_SLOT]);

	SendMessage(WaitForCommand,SPUCOMM_SM_ID);
}




//////////////////////////////////////////////////
//
// SPU command to Set the PCU Mode
//
//////////////////////////////////////////////////

#define	LOCAL_CTRL_BIT			0x01

// undefined					0x02
// undefined					0x04
// undefined					0x08
// undefined					0x10
// undefined					0x20
// undefined					0x40
// undefined					0x80

void	DoCommand61(void) {

		//////////////////////////////////////////////////
		// Local Positioning
		//////////////////////////////////////////////////

		if(CommBuffer[DATA_1_SLOT] & LOCAL_CTRL_BIT) {

			// Activate Digital Input Monitor Driver if not yet

			if(GetState(DIGI_IN_MONITOR_ID) == DIM_IDLE)
				SendMessage(DigiInGoActive,DIGI_IN_MONITOR_ID);

			SET_PM_LOCAL();

		} else {

			CLEAR_PM_LOCAL();	// inhibit local
								// positioning control
		}


	SendMessage(WaitForCommand,SPUCOMM_SM_ID);
}


//////////////////////////////////////////////////
//
// SPU request for the current PCU Mode
// settings
//
//////////////////////////////////////////////////

void	DoCommand62(void) {

	Build7ByteHeader();

		CommBuffer[DATA_1_SLOT] = PCUmode;

	BuildCRC();
	SendCBuff();
}





//////////////////////////////////////////////////
//
// A Packet with an unknown command was
// received.
//
//////////////////////////////////////////////////

void	UnknownCommand()
{
	LogUnknownCmdError;

	SendMessage(WaitForCommand,SPUCOMM_SM_ID);
}


//////////////////////////////////////////////////
//
// This function sets up the first two
//  header bytes of a 7 byte reply packet
//
//////////////////////////////////////////////////

void	Build7ByteHeader(void)
{
	ClearCommBuffer();

	CommBuffer[LENGTH_SLOT] = PACKET_7_LEN;
	CommBuffer[ID_SLOT] 	= PCU_ID;
}


//////////////////////////////////////////////////
//
// This function calls ProcessCRC to generate
// values for CRCGX1 and 2. These are then
// attached at the end of the packet to be sent.
//
//////////////////////////////////////////////////

void	BuildCRC(void)
{
	ProcessCRC();

		CommBuffer[CRC_HI_SLOT] = CRCGX2;
		CommBuffer[CRC_LO_SLOT] = CRCGX1;
}



//////////////////////////////////////////////////
//
// These routines are used for stack checking.
//
// There should be enough room for stack use
// that normal operation will never cause the
// program to write to STACK BOUND.
//
//////////////////////////////////////////////////

BOOL	IsStackOk(void) {

#asm

BOUND	EQU		$eb		;STACKBOUND, in system.h

	LDA		BOUND		; Make sure memory location
						; is still clear

	BEQ		STACKOK

	LDA		#FALSE
	RTS

STACKOK:
	LDA		#TRUE
	RTS

#endasm

}

void	TrapStackError(void) {

	while(1)
		;   	// Trap
}



//////////////////////////////////////////////////
//
// State Matrix Table
//
//////////////////////////////////////////////////

#asm
_PCUMANAGER:
	fdb 	xPCUM_IDLE_MATRIX
	fdb		xPCUM_CFGHI_MATRIX
	fdb		xPCUM_CFGME_MATRIX
	fdb		xPCUM_2CFGHI_MATRIX
	fdb		xPCUM_ACTIVE_MATRIX
#endasm


//////////////////////////////////////////////////
//
// Message/Exit Function Matrix Table
//
//////////////////////////////////////////////////

#asm
xPCUM_IDLE_MATRIX:
	fcb		TimeOut
	fdb		PMM_exitA
	fcb		NULL_MESSAGE
#endasm

#asm
xPCUM_CFGHI_MATRIX:
	fcb		TimeOut
	fdb		PMM_exitB
	fcb		NULL_MESSAGE
#endasm

#asm
xPCUM_CFGME_MATRIX:
	fcb		TimeOut
	fdb		PMM_exitF
	fcb		NULL_MESSAGE
#endasm

#asm
xPCUM_2CFGHI_MATRIX:
	fcb		TimeOut
	fdb		PMM_exitG
	fcb		NULL_MESSAGE
#endasm

#asm
xPCUM_ACTIVE_MATRIX:
	fcb		TimeOut
	fdb		PMM_exitD
	fcb		CommandReceived
	fdb		PMM_exitC
	fcb		InvalidPacket
	fdb		PMM_exitE
	fcb		CommBuffSent
	fdb		PMM_exitH
	fcb		MotorMoveDone
	fdb		PMM_exitH
	fcb		UpDnSWdetected
	fdb		PMM_exitI
	fcb		MotorNudgeUp
	fdb		PMM_exitJ
	fcb		MotorNudgeDown
	fdb		PMM_exitK
	fcb		MotorStopNudging
	fdb		PMM_exitL
	fcb		NULL_MESSAGE
#endasm



