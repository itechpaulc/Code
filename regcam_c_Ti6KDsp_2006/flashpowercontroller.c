




//
//
//
//  Author :    Paul Calinawan
//
//  Date:       June 27, 2006
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


#include "flashpowercontroller.h"



////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////

int     flashPowerLevel;

BYTE    digitalResistorLevel;

int     fplInFile;

int     fplLogCounter;


///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////

void    SetFlashPowerLevelControl(int fPLevel)
{
    if(fPLevel > (MAX_FLASH_POWER_LEVEL_VALUE * FPL_SCALE))
        flashPowerLevel = (MAX_FLASH_POWER_LEVEL_VALUE * FPL_SCALE);
    else if(fPLevel < (MIN_FLASH_POWER_LEVEL_VALUE * FPL_SCALE))
        flashPowerLevel = (MIN_FLASH_POWER_LEVEL_VALUE * FPL_SCALE);
    else
        flashPowerLevel = fPLevel;

    digitalResistorLevel = ((MAX_DIGITAL_RESISTOR_VALUE * FPL_SCALE) - flashPowerLevel)/FPL_SCALE ;

    // TODO, sending to the FPGA in states

    //print("FPLVL %d DIGITAL RESITOR %d\n", flashPowerLevel, digitalResistorLevel);

    SetFlashPowerLevel(digitalResistorLevel);
}

int    GetFlashPowerLevelControl(void)
{
    return  flashPowerLevel;
}


void    IncrementFlashPowerLevel(int incrementVal)
{   
    int fpLevel = flashPowerLevel + incrementVal;

    // TODO, sending to the FPGA in states
    //print("INCREMENT FPL\n");
    SetFlashPowerLevelControl(fpLevel);
}

void    DecrementFlashPowerLevel(int decrementVal)
{
    int fpLevel = flashPowerLevel - decrementVal;

    // TODO, sending to the FPGA in states
    //print("DECREMENT FPL\n");
    SetFlashPowerLevelControl(fpLevel);
}


void    CheckForFplFileUpdate(void)
{
    int currFpl;

    currFpl = GetFlashPowerLevelControl();

        if ( fplLogCounter > MAX_FPL_LOG_IMP_COUNT )
        {
            if ( (abs(fplInFile - currFpl)) > FPL_MAX_CHANGE_BEFORE_SAVING )
            {
                WriteFileFlashPowerLevel(currFpl);
            }

            fplLogCounter = 0;
        }
        else
        {
            fplLogCounter++;
        }
}



///////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////

int    ReadFileFlashPowerLevel(void)
{
    FILE *fp;

    int  res, data;

    int    flashPowerLevel;

    char fName[] = FPL_FILE;

    flashPowerLevel = 0;

        fp = io_fopen(fName,"r");
    
        if ( fp )
        {
            res = io_read(fp, (char *)&data, sizeof(data));
    
 //           if ( res >= 0 )
 //           {
 //               //print("%d bytes sucessfully read from file %s\n",res,fName);
 //           }
 //           else
 //               print("Cannot Read from file %s\n",fName);
 //   
            flashPowerLevel = data;
            //print("FPL: %d from Flash Memory\n", flashPowerLevel);
    
            io_fclose(fp);
        }
 //       else
 //       {
 //           print("Cannot Open file for read\n");
 //       }
    
        fplInFile = flashPowerLevel;

    return flashPowerLevel;
}


void    WriteFileFlashPowerLevel(int flashPowerLevel)
{
   FILE *fp;
 
   int res;
 
   char fName[] = FPL_FILE;
 
   fp = io_fopen(fName,"w");
 
       if ( fp )
       {
           res = io_write(fp, (char *)&flashPowerLevel, sizeof(flashPowerLevel));
   
//            if ( res >= 0 )
//            {
//                print("FPL: %d to Flash Memory\n", flashPowerLevel);
//            }
//            else
//            {
//                print("Cannot Write to Flash Memory\n");
//            }
   
           io_fclose(fp);
       }
//        else
//        {
//            print("Cannot open file for write\n");
//        }
 
   fplInFile = flashPowerLevel;
}




////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////


void    Init_FlashPowerController(void)
{
    SetFlashPowerLevelControl(FPL_INIT);

    //print("SETTING FLASH POWER %d\n", (GetFlashPowerLevelControl()/FPL_SCALE));
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

NEW_STATE   FPC_exitA(void)
{
    int fplFromFile;

    //print("FPC ACTIVE\n");

        fplFromFile = ReadFileFlashPowerLevel();
    
        if ( fplFromFile == 0 )
            fplFromFile = FPL_MID_POINT_START;
    
        SetFlashPowerLevelControl(fplFromFile);
    
        fplLogCounter = 0;

    return FPC_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// NULL
//
////////////////////////////////////////////////////////////////////

NEW_STATE   FPC_exitB(void)
{

    return FPC_ACTIVE;
}


////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_FPC_IDLE)
    EV_HANDLER(GoActive, FPC_exitA)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_FPC_ACTIVE)
    EV_HANDLER(NULL_MESSAGE_ID, FPC_exitB)
STATE_TRANSITION_MATRIX_END;


// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(FPC_Main_Entry)
    STATE(_FPC_IDLE)            ,           
    STATE(_FPC_ACTIVE)   
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////


