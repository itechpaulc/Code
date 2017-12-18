




//
//
//
//  Author :    Paul Calinawan
//
//  Date:       February 2, 2006
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


#include "itechsys.h"


////////////////////////////////////////////////////////////////////


#include "kernel.h"


////////////////////////////////////////////////////////////////////


#include "encoderhandler.h"
#include "imageacquirerhandler.h"
#include "cameraconfig.h"
#include "spicomm.h"


////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

void CalculateEncoderTrigger(ENCODER_TRIGGER *encoderTrig, DWORD   triggerTarget)
{
    unsigned long   aIndex, remTarget, 
    remTempA, remTempB, fract;

    // Calc Pulse Count
    if (triggerTarget >= currentSystemConfiguration.encoder.cutOffLength )
    {
        triggerTarget = 0;  // cut-off target is same as zero
    }

    aIndex      = 
    remTempA    = 
    currentSystemConfiguration.encoder.aPulsesPerImpression * triggerTarget;

    aIndex      /= 
    currentSystemConfiguration.encoder.cutOffLength;

    encoderTrig->encoderIndexCount = (WORD)aIndex;

    // Calc Fraction

    remTempB    =  aIndex * currentSystemConfiguration.encoder.cutOffLength;

    remTarget   =   remTempA - remTempB; //Remainder Target

    fract       =   remTarget * 0xFFFF;
    fract       /=  
    currentSystemConfiguration.encoder.cutOffLength;

    encoderTrig->encoderFractionalCount = (WORD)fract;
}

void CalculateEncoderFlashTrigger(ENCODER_TRIGGER *encoderTrig, DWORD   triggerTarget)
{
    CalculateEncoderTrigger(encoderTrig, triggerTarget);
}

void CalculateEncoderShutterTrigger(ENCODER_TRIGGER *shutEncoderTrig, 
                                    DWORD   triggerTarget, DWORD currATicks)
{
    long            shutterOpenTarget;

    unsigned long   tempA, tempB;

    tempA   =  (unsigned long) 
               ((unsigned long) (currentSystemConfiguration.encoder.cutOffLength) *
                (unsigned long) (FPGA_CLOCK_TICKS_PER_MICROSEC) *
                (unsigned long) (currentSystemConfiguration.imageCapture.shutterTriggerDelayTime));

    tempB   =   currATicks * currentSystemConfiguration.encoder.aPulsesPerImpression;

    shutterOpenTarget = triggerTarget - ( tempA / tempB );

    // Handle underflow

    if ( shutterOpenTarget < 0 )
        shutterOpenTarget += currentSystemConfiguration.encoder.cutOffLength;

    // Calculate in terms of trigger pulseCount and fraction

    CalculateEncoderTrigger(shutEncoderTrig, shutterOpenTarget);
}


////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////


// TODO


void    Init_EncoderHandler(void)
{
    // TODO shoudl be in Strobe Power Manager

//    print("FLASH POWER ON\n");

}

////////////////////////////////////////////////////////////////////
//
// Exit Procedures
//
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//
// GoActive
//
////////////////////////////////////////////////////////////////////

NEW_STATE   ECH_exitA(void)
{
    //print("ECH ACTIVE\n");

    return ECH_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// SetupTriggerPositions
//  while in ECH_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   ECH_exitB(void)
{
    ENCODER_TRIGGER shutterEncoderTrig;

    DWORD   currEncPeriod = 0 ;

    DWORD   flashTriggerTarget = GetMessageData1();

    WORD    shutterTriggerIndex, shutterTriggerFraction;

    GetSPIDataWithRetries(GET_CURRENT_ENCODER_PERIOD, &currentPeriod );

    // TODO States to SET Trigger data through SPI COMM
    if ( GetCurrentSimulationMode() != SIMULATION_RUN )
    {
        CalculateEncoderShutterTrigger(&shutterEncoderTrig, flashTriggerTarget, currentPeriod);

        shutterTriggerIndex = shutterEncoderTrig.encoderIndexCount;
        shutterTriggerFraction = shutterEncoderTrig.encoderFractionalCount;

        SPI_ISSUE_RESET_TRIGGERING();

        SetShutterTriggerIndexAndFraction(shutterTriggerIndex, shutterTriggerFraction);
    }

    SendMessage(ScannerManager, TriggerPositionSet);

    return SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
// BeginTriggerProcess
//  while in ECH_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   ECH_exitC(void)
{
    ENCODER_TRIGGER flashEncoderTrig;

    DWORD   flashTriggerTarget = GetMessageData1();

    WORD    flashTriggerIndex, flashTriggerFraction;

    // TODO States to SET Trigger data through SPI COMM

//     if ( GetCurrentSimulationMode() == SIMULATION_RUN )
//     {
//         print("ECH TRIG NOW\n");
//         SetStartImageCaptureNow(SHUTTER_ON, TRIGGER_ON);      //Enable both Shutter and Trigger
//     }
//     else
    {
        CalculateEncoderTrigger(&flashEncoderTrig, flashTriggerTarget);

        flashTriggerIndex = flashEncoderTrig.encoderIndexCount;
        flashTriggerFraction = flashEncoderTrig.encoderFractionalCount;

        SetFlashTriggerIndexAndFraction(flashTriggerIndex, flashTriggerFraction);
        //print("ECH ENCODER TRIG: Index: %d Fract: %d\n", flashTriggerIndex,flashTriggerFraction); 
    }

    SendMessage(ScannerManager, TriggerProcessBegins);

    return SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_ECH_IDLE)
EV_HANDLER(GoActive, ECH_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_ECH_ACTIVE)
EV_HANDLER(SetupTriggerPositions, ECH_exitB),
EV_HANDLER(BeginTriggerProcess, ECH_exitC)
STATE_TRANSITION_MATRIX_END;





// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(ECH_Main_Entry)
STATE(_ECH_IDLE)            ,           
STATE(_ECH_ACTIVE)   
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////

void   setFPGAframeCaptureLocation(DWORD flashTarget)
{
    ENCODER_TRIGGER shutterEncoderTrig,flashEncoderTrig;

    WORD    IndexShutter, FractionShutter, IndexTrig, FractionTrig;

    DWORD   shutterTarget;

    GetSPIDataWithRetries(GET_CURRENT_ENCODER_PERIOD, &currentPeriod );

    // flashTarget -= (638371 * 205)/4096/currEncPeriod;  //PMDebug  Trigger to Flash delay need to replace with real formula

    flashTarget = NormalizeCircumPosition(flashTarget);

    SPI_ISSUE_RESET_TRIGGERING();


    CalculateEncoderTrigger(&flashEncoderTrig, flashTarget);
    IndexTrig       = flashEncoderTrig.encoderIndexCount;

    // FractionTrig    = flashEncoderTrig.encoderFractionalCount;
    FractionTrig = 0x0000;  //Temporary to accomodate Speed changes

    shutterTarget = IndexTrig * currentSystemConfiguration.encoder.cutOffLength;
    shutterTarget /= currentSystemConfiguration.encoder.aPulsesPerImpression;

    //Set Shutter Trigger Location
    CalculateEncoderShutterTrigger(&shutterEncoderTrig, shutterTarget, currentPeriod);
    IndexShutter    = shutterEncoderTrig.encoderIndexCount;
    FractionShutter = shutterEncoderTrig.encoderFractionalCount;

    //if ( FractionShutter > 0x7FFF )  //Use only 50% of the total value for when increasing spreed.
    //    FractionShutter = 0x7FFF;

    //Set Flash Trigger Location

    SetShutterTriggerIndexAndFraction(IndexShutter, FractionShutter);
    SetFlashTriggerIndexAndFraction(IndexTrig, FractionTrig);

    if ( frameCaptureMode == SINGLE_FRAME_ANALYSIS )
    {
        singleFrameBuffer->encIndex = IndexTrig;
        singleFrameBuffer->fractional = FractionTrig;
    }
    else
    {
        nextFrameBufferImageEmpty->encIndex = IndexTrig;
        nextFrameBufferImageEmpty->fractional = FractionTrig;
    }
}





