




//
//
//
//  Author :    Paul Calinawan
//
//  Date:       June 22, 2006
//
//  Copyrights: Imaging Technologies Inc.
//
//  Product:    Intelli-Ribbon Control
//  
//  Subsystem:  Camera System
//  -------------------------------------------
//
//
//      CONFIDENTIAL DOCUMENT
//
//      Property of Imaging Technologies Inc.
//
//


////////////////////////////////////////////////////////////////////

#include <vcrt.h>
#include <vclib.h>
#include <macros.h>
#include <sysvar.h>


////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////
#include "itechsys.h"
#include "kernel.h"
#include "spicomm.h"
#include "transportmanager.h"

////////////////////////////////////////////////////////////////////
//
//  External Variables
//
////////////////////////////////////////////////////////////////////

extern TRANSPORT_MOVE_PARAM transMoveParam;

////////////////////////////////////////////////////////////////////
//
//  External Variables
//
////////////////////////////////////////////////////////////////////

DWORD spiReadRetries = 0, spiWriteRetries = 0;

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

BYTE logicOutputData = 0x00;

////////////////////////////////////////////////////////////////////
//
// Exit Procedures
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SPI_exitA(void)
{
   // print("\n*****************SPI exitA");
    return SPI_ACTIVE;
}
////////////////////////////////////////////////////////////////////
//
// Set Transport Location
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SPI_exitB1(void)
{
    DWORD newLocation;

    //print("\n*****************SPI exitB1");
    newLocation = GetMessageData1();

    SPI_SET_TRANSPORT_LOCATION( newLocation );

    SendMessage(GetSmSourceId(), TransportLocationSet); 

    return SAME_STATE;
}



////////////////////////////////////////////////////////////////////
//
// Get Transport Status
//
////////////////////////////////////////////////////////////////////
NEW_STATE   SPI_exitB2(void)
{
    DWORD   *tranStatus;

    //print("\n*****************SPI exitB2");
    SPI_GET_TRANSPORT_STATUS( tranStatus );
    SendMessageAndData(GetSmSourceId(), TransportStatus, *tranStatus,0); 
    return SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
// Issue Transport Move
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SPI_exitB3(void)
{

   // print("*****************SPI exitB3: A:%x D:%x T:%x\n",transMoveParam.accelTarget,transMoveParam.decelTarget,transMoveParam.moveTarget);
    SPI_SET_TRANSPORT_ACCEL_TARGET( transMoveParam.accelTarget );
    SPI_SET_TRANSPORT_DECEL_TARGET( transMoveParam.decelTarget );
    SPI_ISSUE_TRANSPORT_MOVE( transMoveParam.moveTarget);

    SendMessage(GetSmSourceId(), TransportMoveSent); 

    return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////

void    Init_SPI_Manager(void)
{


}


STATE_TRANSITION_MATRIX(_SPI_IDLE)
EV_HANDLER(GoActive, SPI_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SPI_ACTIVE)
EV_HANDLER(SetTranLocation, SPI_exitB1),
EV_HANDLER(GetTransporStatus, SPI_exitB2),
EV_HANDLER(SetupTransportMove, SPI_exitB3)
STATE_TRANSITION_MATRIX_END;


SM_RESPONSE_ENTRY(SPI_Main_Entry)
STATE(_SPI_IDLE),           
STATE(_SPI_ACTIVE)           
SM_RESPONSE_END

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

BYTE SpiCalcCheckSum(DWORD data, BYTE cmd)
{
    BYTE checkSum;

    union chkSumUnion
    {
        DWORD   rawData;
        BYTE    databyte[4];

    } checkSumData;

    checkSumData.rawData = data;

    checkSum = cmd + 
               checkSumData.databyte[3] + 
               checkSumData.databyte[2] + 
               checkSumData.databyte[1] + 
               checkSumData.databyte[0];

    return checkSum;
}

BOOL    SpiVerifyCheckSum( DWORD data, BYTE cmd, BYTE rxCheckSum )
{
    BYTE checkSum;

    union chkSumUnion
    {
        DWORD   rawData;
        BYTE    databyte[4];

    } checkSumData;

    checkSumData.rawData = data;

    checkSum =  cmd + 
                checkSumData.databyte[3] + 
                checkSumData.databyte[2] + 
                checkSumData.databyte[1] + 
                checkSumData.databyte[0];

    return( checkSum == rxCheckSum );
}



////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

// Setting of Data Lines

void    SET_SPI_DATA_OUT_LO(void)                                                   
{
    SET_LOGIC_OUTPUT_DATA(GET_LOGIC_OUTPUT_DATA() | SPI_DATA_OUT);
    SET_LOGIC_OUT(GET_LOGIC_OUTPUT_DATA());
}

void    SET_SPI_CLOCK_LO(void)                                                      
{
    SET_LOGIC_OUTPUT_DATA(GET_LOGIC_OUTPUT_DATA() | SPI_CLOCK_OUT);     
    SET_LOGIC_OUT(GET_LOGIC_OUTPUT_DATA());  
}

void    SET_SPI_CHIP_SEL0_LO(void)                                                
{
    SET_LOGIC_OUTPUT_DATA(GET_LOGIC_OUTPUT_DATA() | SPI_CHIP_SEL0_OUT);
    SET_LOGIC_OUT(GET_LOGIC_OUTPUT_DATA());  
}

void    SET_SPI_CHIP_SEL1_LO(void)                                                  
{
    SET_LOGIC_OUTPUT_DATA(GET_LOGIC_OUTPUT_DATA() | SPI_CHIP_SEL1_OUT);
    SET_LOGIC_OUT(GET_LOGIC_OUTPUT_DATA());  
}


// Clearing of Data Lines

void    SET_SPI_DATA_OUT_HI(void)
{
    SET_LOGIC_OUTPUT_DATA(GET_LOGIC_OUTPUT_DATA() & ~SPI_DATA_OUT);     
    SET_LOGIC_OUT(GET_LOGIC_OUTPUT_DATA());  
}

void    SET_SPI_CLOCK_HI(void)
{
    SET_LOGIC_OUTPUT_DATA(GET_LOGIC_OUTPUT_DATA() & ~SPI_CLOCK_OUT);    
    SET_LOGIC_OUT(GET_LOGIC_OUTPUT_DATA());  
}

void    SET_SPI_CHIP_SEL0_HI(void)
{
    SET_LOGIC_OUTPUT_DATA(GET_LOGIC_OUTPUT_DATA() & ~SPI_CHIP_SEL0_OUT);
    SET_LOGIC_OUT(GET_LOGIC_OUTPUT_DATA());  
}

void    SET_SPI_CHIP_SEL1_HI(void)
{
    SET_LOGIC_OUTPUT_DATA(GET_LOGIC_OUTPUT_DATA() & ~SPI_CHIP_SEL1_OUT);
    SET_LOGIC_OUT(GET_LOGIC_OUTPUT_DATA());  
}


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////


void    SpiClock(void)
{
    int x;
    for ( x= 0; x < 15 ; x++ ) //PMDEBUG extend spi clock for now
        SET_SPI_CLOCK_HI();
    for ( x= 0; x < 15 ; x++ ) //PMDEBUG extend spi clock for now
        SET_SPI_CLOCK_LO();
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SendSpiDataByte(BYTE dataByte)
{
    int b, bitSel=8, bitMask = 0x80;

    for ( b=0; b<bitSel; b++ )
    {
        if ( dataByte & bitMask )
            SET_SPI_DATA_OUT_HI();
        else
            SET_SPI_DATA_OUT_LO();

        SpiClock();

        bitMask = bitMask>>1;
    }
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SendSpiData(BYTE *dataByte, int dataLength)
{
    int byte;

    for ( byte=0; byte<dataLength; byte++ )
    {
        SendSpiDataByte(*dataByte);

        ++dataByte;
    }
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SendSpiDataWord(WORD dataWord)
{
    SendSpiDataByte((dataWord & 0xFF00) >> 8);
    SendSpiDataByte((dataWord & 0x00FF));
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SendSpiDataDWord(DWORD dataDWord)
{
    SendSpiDataByte((dataDWord & 0xFF000000) >> 24);
    SendSpiDataByte((dataDWord & 0x00FF0000) >> 16);
    SendSpiDataByte((dataDWord & 0x0000FF00) >> 8);
    SendSpiDataByte((dataDWord & 0x000000FF));
}

////////////////////////////////////////////////////////////////////
//
// CSEL0 = 0
// CSEL1 = 0
//
////////////////////////////////////////////////////////////////////

void    SetSpiAddressReset(void)
{
    SET_SPI_DATA_OUT_LO();
    SET_SPI_CLOCK_LO();

    SET_SPI_CHIP_SEL0_LO();
    SET_SPI_CHIP_SEL1_LO();
}

////////////////////////////////////////////////////////////////////
//
// CSEL0 = 1
// CSEL1 = 0
//
////////////////////////////////////////////////////////////////////

void    SetSpiAddressWrite(void)
{
    SET_SPI_DATA_OUT_LO();
    SET_SPI_CLOCK_LO();

    SET_SPI_CHIP_SEL0_HI();
    SET_SPI_CHIP_SEL1_LO();
}


////////////////////////////////////////////////////////////////////
//
// CSEL0 = 0
// CSEL1 = 1
//
////////////////////////////////////////////////////////////////////

void    SetSpiAddressRead(void)
{
    SET_SPI_DATA_OUT_LO();
    SET_SPI_CLOCK_LO();

    SET_SPI_CHIP_SEL0_LO();
    SET_SPI_CHIP_SEL1_HI();
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

BYTE    ReadSpiDataByte(void)
{
    int b, bitSel=7, dataByte=0x00;

    for ( b=bitSel; b>=0; b-- )
    {
        //    SET_SPI_CLOCK_HI();  //Load
        //    SET_SPI_CLOCK_LO();

        SpiClock();
        dataByte |= (GET_SPI_DATA_IN()<<b);
    }

    return dataByte;
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

WORD     ReadSpiDataWord(void)
{
    WORD dataWord = 0x0000;

    dataWord |= ReadSpiDataByte()<< 8;
    dataWord |= ReadSpiDataByte();

    return dataWord;
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

DWORD     ReadSpiDataDWord(void)
{
    DWORD dataWord = 0x00000000;             

    dataWord |= ReadSpiDataByte() << 24; 
    dataWord |= ReadSpiDataByte() << 16; 
    dataWord |= ReadSpiDataByte() << 8;  
    dataWord |= ReadSpiDataByte();       

    return dataWord;
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    ResetWatchdog(void)
{
    WriteSPIData( SET_WATCHDOG_RESET, 0 );
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

// void    ForcedReset(void)
// {
//     BYTE checkSum;
//     SetSpiAddressWrite();
//
//     SendSpiDataByte(SET_FORCED_RESET);
//     SendSpiDataDWord(0);
//
//     checkSum = SpiCalcCheckSum(0, ENABLE_STEPPER_CONTROL);
//     SendSpiDataByte(checkSum);
//
//     SetSpiAddressReset();
// }

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    EnableStepperControl(void)
{
    WriteSPIData( ENABLE_STEPPER_CONTROL, 0 );
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    DisableStepperControl(void)
{
    WriteSPIData( DISABLE_STEPPER_CONTROL, 0 );
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    MoveStepper(DWORD stepCount, BOOL direction)
{
    DWORD   moveStepperData;

    moveStepperData = stepCount & 0x7FFFFFF;// Set your move counts

    if ( direction )
        moveStepperData = moveStepperData | 0x80000000;

    WriteSPIData( MOVE_STEPPER, moveStepperData );
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    ResetStepperControl(void)
{
    DisableStepperControl();

    EnableStepperControl();
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetStepperStartSpeed(DWORD startSpeed)
{
    WriteSPIData( SET_STEPPER_START_SPEED, startSpeed );
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////
void    SetStepperAccelerationTarget(DWORD accelerationTarget)
{
    WriteSPIData( SET_STEPPER_ACCEL_TARGET, accelerationTarget );
}


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////
void    SetStepperDecelerationTarget(DWORD decelerationTarget)
{
    WriteSPIData( SET_STEPPER_DECEL_TARGET, decelerationTarget );
}
////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetStepperMaxSpeed(WORD maxSpeed)
{
    WriteSPIData( SET_STEPPER_MAX_SPEED, maxSpeed );
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetStepperAcceleration(WORD accel)
{
    WriteSPIData( SET_STEPPER_ACCELERATION, accel );
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetStepperAccelerationFreq(WORD freqency)
{
    WriteSPIData( SET_STEPPER_ACCEL_FREQ, freqency );
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetStepperFindHomeSpeed(WORD findHomeSpeed)
{
//  NEED To Do
}


// Camera Controls

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetFlashPowerLevel(BYTE flashPowerLevel)
{
    WriteSPIData( SET_FLASH_POWER_LEVEL, flashPowerLevel );
}


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    EnableFlashPower( BYTE flashPowerEnable )
{
    WriteSPIData( ENABLE_FLASH_POWER, (flashPowerEnable & 0x01) );
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetFlashTriggerIndexAndFraction(WORD index, WORD fraction)
{
    BYTE checkSum;
    DWORD sendData = fraction;

    sendData =  (sendData<<16);
    sendData |= (DWORD) index;

    WriteSPIData( SET_FLASH_TRIG_IDX_AND_FRAC, sendData);
}


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetShutterTriggerIndexAndFraction(WORD index, WORD fraction)
{
    BYTE checkSum;
    DWORD sendData = fraction;

    sendData =  (sendData<<16);
    sendData |= (DWORD) index;

    WriteSPIData( SET_SHUTTER_TRIG_IDX_AND_FRAC, sendData);
}


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetImpressionCount(int impressionCount)
{
    WriteSPIData( SET_IMPRESSION_COUNT, impressionCount);
}


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

// LED Diagnostic Controls

void    SetLedRgbLevels(BYTE red, BYTE green, BYTE blue)
{
    SetSpiAddressWrite();

    SendSpiDataByte(SET_LED_RGB_LEVELS);

    SendSpiDataByte(red);
    SendSpiDataByte(green);
    SendSpiDataByte(blue);

    SetSpiAddressReset();
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

// void    SetLedRgbCadence(BYTE cadence)
// {
//     SetSpiAddressWrite();
//
//     SendSpiDataByte(SET_LED_RGB_CADENCE);
//
//     SendSpiDataByte(cadence);
//
//     SetSpiAddressReset();
// }


////////////////////////////////////////////////////////////////////
//
//  See RGB LED Color Definitions
//
////////////////////////////////////////////////////////////////////

void    SetLedRgbColor(int rgbColor)
{
    BYTE    red, green, blue;

    switch ( rgbColor )
    {
        case RGB_COLOR_ID_WHITE:
            red = 0x4F; green=0x5F; blue=0x7F;
            break;
        case RGB_COLOR_ID_BLUE:
            red = 0x00; green=0x00; blue=0xFF;
            break;
        case RGB_COLOR_ID_RED:
            red = 0x4F; green=0x00; blue=0x00;
            break;
        case RGB_COLOR_ID_GREEN:
            red = 0x00; green=0xFF; blue=0x00;
            break;
        case RGB_COLOR_ID_YELLOW:
            red = 0x3F; green=0x7F; blue=0x00;
            break;
        case RGB_COLOR_ID_PINK:
            red = 0x9F; green=0x1F; blue=0x2F;
            break;
    }

    SetLedRgbLevels(red, green, blue);
}


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetLedReadyOn(void)
{
    SetSpiAddressWrite();

    SendSpiDataByte(SET_LED_READY);

    SetSpiAddressReset();
}


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

// void    SetLedReadyOff(void)
// {
//     SetSpiAddressWrite();
//
//     SendSpiDataByte(SET_LED_READY_OFF);
//
//     SetSpiAddressReset();
// }

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetLedRxOneShot(void)
{
    SetSpiAddressWrite();

//    SendSpiDataByte(SET_LED_RX_ONE_SHOT);

    SetSpiAddressReset();
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetLedTxOneShot(void)
{
    SetSpiAddressWrite();

    SendSpiDataByte(SET_LED_TX_ONE_SHOT);

    SetSpiAddressReset();
}


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetStartImageCaptureNow(BOOL shutter, BOOL trigger)
{
    BYTE checkSum;
    BYTE triggerMode = 0x00;

    if ( shutter )
        triggerMode |= 0x01;

    if ( trigger )
        triggerMode |= 0x02;

    WriteSPIData( SET_START_IMAGE_CAPTURE, triggerMode);
}


////////////////////////////////////////////////////////////////////
//
// See ENCODER Filter Type Definitions
//
////////////////////////////////////////////////////////////////////

// void    SetEncoderFilterType(BYTE filterType)
// {
//     SetSpiAddressWrite();
//
//     SendSpiDataByte(SET_ENCODER_FILTER_TYPE);
//
//     SendSpiDataByte(filterType);
//
//     SetSpiAddressReset();
// }

////////////////////////////////////////////////////////////////////
//
//              Low Level SPI Communitcaton
//
////////////////////////////////////////////////////////////////////
BOOL GetSPIDataWithRetries( BYTE cmd, DWORD *rxData )
{
    BYTE x; 
    for ( x = 0; x < 3; x++ )
    {
        if ( GetSPIData( cmd, rxData ) )
            return PASS;

        spiReadRetries++;
    }

    return FAIL;
}

BOOL GetSPIData( BYTE cmd, DWORD *data )
{
    BYTE    rxCheckSum;
    DWORD   rxData = 0x00000000;

    SetSpiAddressRead();

    SendSpiDataByte(cmd);

    rxData          = ReadSpiDataDWord();
    rxCheckSum      = ReadSpiDataByte();

    SetSpiAddressReset();

    if ( SpiVerifyCheckSum( rxData, cmd, rxCheckSum) )
    {
        *data = rxData;
        return PASS;
    }
    else
        return FAIL;
}

BOOL WriteSPIDataWithRetries( BYTE writeCmd, DWORD writeData )
{
    BYTE x;

    for ( x = 0; x < 3; x++ )
    {
        WriteSPIData(  writeCmd,  writeData );

        if ( GET_SPI_WRITE_MSG_STATUS() )
            return PASS;

        spiWriteRetries++;
    }

    return FAIL;
}

void WriteSPIData( BYTE cmd, DWORD data )
{
    BYTE checkSum;

    SetSpiAddressWrite();

    SendSpiDataByte(cmd);
    SendSpiDataDWord(data);

    checkSum = SpiCalcCheckSum(data, cmd);
    SendSpiDataByte(checkSum);

    SetSpiAddressReset();
}

