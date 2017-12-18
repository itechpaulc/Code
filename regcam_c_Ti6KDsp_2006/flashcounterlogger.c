




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


#include "flashcounterlogger.h"


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////


DWORD       flashCount;

int         fcntInFile;


int         loggerWriteCount;
int         packFileCount;


void        IncrementFlashCounter(void)
{
    ++flashCount;
}

void        SetFlashCount(DWORD flashCnt)
{
    flashCount = flashCnt;
}

DWORD       GetFlashCount(void)
{
    return flashCount;
}


///////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////

void        CheckForFcntFileUpdate(void)
{
    loggerWriteCount++;

    if ( loggerWriteCount == FLASH_COUNT_WRITE_FREQUENCY )
    {
        WriteFileIncrementFlashCount();

        loggerWriteCount = 0;

        packFileCount++;

        if ( packFileCount == FILE_PACK_COUNT_WRITE_FREQUENCY )
        {
            // TODO
            // PACK FILE

            packFileCount = 0;
        }
    }
}

///////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////

int        ReadFileFlashCount(void)
{
    FILE *fp;

    int  res, data;

    char fName[] = FCNT_FILE;

    fcntInFile = 0;

    fp = io_fopen(fName,"r");

    if ( fp )
    {
        res = io_read(fp, (char *)&data, sizeof(data));

//            if ( res >= 0 )
//            {
//                //print("%d bytes sucessfully read from file %s\n",res,fName);
//            }
//            else
//                print("Cannot Read from file %s\n",fName);

        fcntInFile = data;
        //    print("FCNT: %dx100K from Flash Memory \n", fcntInFile);

        io_fclose(fp);
    }
    //  else
    //  {
    //      print("Cannot Open file for read\n");
    //  }

    return fcntInFile;
}

void        WriteFileIncrementFlashCount(void)
{
    FILE *fp;

    int res;

    char fName[] = FCNT_FILE;

    fp = io_fopen(fName,"w");

    if ( fp )
    {
        fcntInFile += 1; // Increment by 1 = 100K Flash Counts

        res = io_write(fp, (char *)&fcntInFile, sizeof(fcntInFile));

        //  if ( res >= 0 )
        //  {
        //      print("FCNT: %d x100K to Flash Memory\n", fcntInFile);
        //  }
        //  else
        //  {
        //      print("Cannot Write to Flash Memory\n");
        //  }

        io_fclose(fp);
    }
    // else
    // {
    //     print("Cannot open file for write\n");
    // }
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

void    Init_FlashCounterLogger(void)
{
    SetFlashCount(0);

    loggerWriteCount = 0;
    packFileCount = 0;
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

NEW_STATE   FCL_exitA(void)
{
    int fcnt;

    fcnt = ReadFileFlashCount();

    fcnt *= FLASH_COUNT_WRITE_FREQUENCY;

    SetFlashCount(fcnt);

    return FCL_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// IncrementFlashCount
//      while in FCL_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   FCL_exitB(void)
{
    IncrementFlashCounter();

    CheckForFcntFileUpdate();

    return FCL_ACTIVE;
}


////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_FCL_IDLE)
EV_HANDLER(GoActive, FCL_exitA)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_FCL_ACTIVE)
EV_HANDLER(IncrementFlashCount, FCL_exitB)
STATE_TRANSITION_MATRIX_END;


// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(FCL_Main_Entry)
STATE(_FCL_IDLE)            ,           
STATE(_FCL_ACTIVE)   
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////


