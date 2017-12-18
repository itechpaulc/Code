


//
// #include "pcumngr.h"
//




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

		StartTimer(POWERUP_DELAY);

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

	if(CfgLineIsHi) {

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

		PCU_ID = CommBuffer[ID_SLOT];

		SendMessage(WaitForCommand,SPUCOMM_SM_ID);
	}
		else

	switch(CommBuffer[CMD_SLOT])
	{
		case	0x41 : DoCommand41();	break;
		case	0x42 : DoCommand42();	break;
		case	0x43 : DoCommand43();	break;
		case	0x44 : DoCommand44();	break;
		case	0x48 : DoCommand48();	break;
		case	0x49 : DoCommand49();	break;
		case	0x4A : DoCommand4A();	break;
		case	0x4B : DoCommand4B();	break;
		case	0x4E : DoCommand4E();	break;
		case	0x51 : DoCommand51();	break;

		case	0x52 : DoCommand52();	break;
		case	0x53 : DoCommand53();	break;
		case	0x54 : DoCommand54();	break;
		case	0x55 : DoCommand55();  	break;
		case	0x56 : DoCommand56();	break;
		case	0x57 : DoCommand57();	break;
		case	0x58 : DoCommand58();	break;
		case	0x59 : DoCommand59();	break;

		default : UnknownCommand();
	}

	return	PCUM_ACTIVE;
}



//////////////////////////////////////////////////
//
// TimeOut message
//
//////////////////////////////////////////////////

BYTE	PMM_exitD(void) {

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

		if(CfgLineIsLow)
		{
			// Configuring Done

			DoNotPassConfigLine;

			// debug
			// Activate Machines

			SendMessage(LedGoActive,LED_SM_ID);
			SendMessage(AtoDGoActive,ATOD_DRIVER_SM_ID);
			SendMessage(DigiInGoActive,DIGI_IN_MONITOR_ID);
			SendMessage(DigiOutGoActive,DIGI_OUT_CNTRLR_ID);


			if(ConfigTimer > BAUD_48_LO && ConfigTimer < BAUD_48_HI) {

				SetBaudRate(BAUD_4800);
				SystemStatus.SYS_CONFIGURED = TRUE;

				return	PCUM_2CFGHI_WAIT;
			}

			if(ConfigTimer > BAUD_96_LO && ConfigTimer < BAUD_96_HI) {

				SetBaudRate(BAUD_9600);
				SystemStatus.SYS_CONFIGURED = TRUE;

				return	PCUM_2CFGHI_WAIT;
			}

			// Error : Config Not OK	

			SystemErrors.ERR_CFG_MEASURE = TRUE;

			StartTimer(SYSTEM_CHECK_RATE);

			return	PCUM_ACTIVE;
		}

	return	SAME_STATE;
}



//////////////////////////////////////////////////
//
// TimeOut while in 2nd Config Hi Wait			
//
//////////////////////////////////////////////////

BYTE	PMM_exitG(void)
{
		if(CfgLineIsHi) {

			if(SystemStatus.SYS_CONFIGURED == TRUE)
				SendMessage(WaitForCommand,SPUCOMM_SM_ID);

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
		SendMessage(WaitForCommand,SPUCOMM_SM_ID);

	return	SAME_STATE;
}


//////////////////////////////////////////////////
//
// Private Helper Functions	Definition
//
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

	// debug SendCByte

	SendMessage(SendCommBuff,SPUCOMM_SM_ID);
	SendMessage(LedGoTransmit,LED_SM_ID);
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
// servo
//
//////////////////////////////////////////////////

void	DoCommand48(void) {

	ClearCommBuffer();

	CommBuffer[LENGTH_SLOT] = PACKET_7_LEN;
	CommBuffer[ID_SLOT] 	= PCU_ID;

	ProcessCRC();

	CommBuffer[CRC_HI_SLOT] = CRCGX2;
	CommBuffer[CRC_LO_SLOT] = CRCGX1;

   	SendMessage(SendCommBuff,SPUCOMM_SM_ID);
	SendMessage(LedGoTransmit,LED_SM_ID);
}



//////////////////////////////////////////////////
//
// SPU Requested for the "Servo Type"
//
//////////////////////////////////////////////////

void	DoCommand49(void) {

	// Send SPECIAL ack

	CommBuffer[LENGTH_SLOT] = PCU_TYPE_ID;

	// debug SendCByte

   	SendMessage(SendCommBuff,SPUCOMM_SM_ID);
	SendMessage(LedGoTransmit,LED_SM_ID);
}


//////////////////////////////////////////////////
//
// SPU Requested for a Reset of the PCU System
//
//////////////////////////////////////////////////

void	DoCommand4A(void) {


}


//////////////////////////////////////////////////
//
// Set Moving Timeout command from SPU
//
//////////////////////////////////////////////////

void	DoCommand4B(void) {

	SetMovingTimeOut(CommBuffer[DATA_1_SLOT]);

	SendMessage(WaitForCommand,SPUCOMM_SM_ID);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoCommand4E(void) {


}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoCommand51(void) {


}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoCommand52(void) {


}


//////////////////////////////////////////////////
//
// Motor, Move to target
//
// ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

void	DoCommand53(void) {

	tempWORD.HiByte  = CommBuffer[DATA_1_SLOT];
	tempWORD.LowByte = CommBuffer[DATA_2_SLOT];

	SetTarget(tempWORD.Word);

	SendMessage(MoveToTarget, MOTOR_CTRLR_SM_ID);
}


//////////////////////////////////////////////////
//
// SPU Requested for the Absolute Position
//
//////////////////////////////////////////////////

#define		LAST_MOTOR_MOVE_BIT		0x80

void	DoCommand54(void) {

	ClearCommBuffer();

	CommBuffer[LENGTH_SLOT] = PACKET_7_LEN;
	CommBuffer[ID_SLOT] 	= PCU_ID;
	CommBuffer[STATUS_SLOT] = GetMotorStatus();

	tempWORD.Word = GetCurrPosition();

	CommBuffer[DATA_1_SLOT] = tempWORD.HiByte;
	CommBuffer[DATA_2_SLOT] = tempWORD.LowByte;

	if(SystemStatus.MOTOR_REVERSE == TRUE)
		CommBuffer[DATA_1_SLOT] |= LAST_MOTOR_MOVE_BIT;
	else
		CommBuffer[DATA_1_SLOT] &= ~LAST_MOTOR_MOVE_BIT;

	ProcessCRC();

	CommBuffer[CRC_HI_SLOT] = CRCGX2;
	CommBuffer[CRC_LO_SLOT] = CRCGX1;

	SendMessage(SendCommBuff,SPUCOMM_SM_ID);
	SendMessage(LedGoTransmit,LED_SM_ID);
}


//////////////////////////////////////////////////
//
// SPU Requested for the Digital Input Status
//
//////////////////////////////////////////////////

#define		DIG_INPUT_STATUS_MASK	0x07

void	DoCommand55(void) {

	ClearCommBuffer();

	CommBuffer[LENGTH_SLOT] = PACKET_7_LEN;
	CommBuffer[ID_SLOT] 	= PCU_ID;

	CommBuffer[DATA_1_SLOT] =
		(GetLatchedInputState() & DIG_INPUT_STATUS_MASK) << 4;

	CommBuffer[DATA_1_SLOT] |=
		(GetCurrInputState() & DIG_INPUT_STATUS_MASK);

	ProcessCRC();

	CommBuffer[CRC_HI_SLOT] = CRCGX2;
	CommBuffer[CRC_LO_SLOT] = CRCGX1;

	SendMessage(SendCommBuff,SPUCOMM_SM_ID);
	SendMessage(LedGoTransmit,LED_SM_ID);
}


//////////////////////////////////////////////////
//
// SPU Command to Control Outputs
//
//////////////////////////////////////////////////

void	DoCommand56(void) {

	SetDigitalOutputs(CommBuffer[DATA_1_SLOT]);

	SendMessage(WaitForCommand,SPUCOMM_SM_ID);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoCommand57(void) {


}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoCommand58(void) {

	tempWORD.HiByte  = CommBuffer[DATA_1_SLOT];
	tempWORD.LowByte = CommBuffer[DATA_2_SLOT];

	SetMinPosition(tempWORD.Word);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	DoCommand59(void) {

	tempWORD.HiByte  = CommBuffer[DATA_1_SLOT];
	tempWORD.LowByte = CommBuffer[DATA_2_SLOT];

	SetMaxPosition(tempWORD.Word);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void	UnknownCommand()
{
	while(1)
		;

	// Log in system error
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
	fcb		NULL_MESSAGE
#endasm



