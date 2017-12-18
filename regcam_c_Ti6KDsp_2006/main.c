


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
#define     NEW_IMAGE_VAR

#include <register.h>
#include <vcrt.h>
#include <vclib.h>
#include <macros.h>
#include <sysvar.h>



////////////////////////////////////////////////////////////////////

#include "kernel.h"
#include "spicomm.h"

#define SBC4018


////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////
//
//  
//
////////////////////////////////////////////////////////////////////


void    InitializeCameraCopyrights(void);


////////////////////////////////////////////////////////////////////
//
//  
//
////////////////////////////////////////////////////////////////////

const char ITECH_PROPERTY[] =
    "A property of Imaging Technologies Inc.                            \
            This document contains CONFIDENTIAL and proprietary information     \
            which is the property of Imaging Technologies, Inc. It may not be   \
            copied or transmitted in whole or in part by any means to any       \
            media without prior written permission from                         \
            Imaging Technologies, Inc." ;



////////////////////////////////////////////////////////////////////
//
//  
//
////////////////////////////////////////////////////////////////////

#define ID_FILE     "fd:/#ID.001"

int  GetCameraModel   (char *CameraModel);
int  GetCameraID      (int *CameraID);
int  ReadFileLine     (char *FileName, int line, int maxlength, char *text);


////////////////////////////////////////////////////////////////////
//
//  
//
////////////////////////////////////////////////////////////////////

extern DWORD           serverConnectionSocket;

void main(void)
{

    DWORD   connectionResult;
    ADDRESS temp;
    FILE *fp;



    init_licence("T254E0909E2");
  //  init_vclib("C25FB69D1AE");

    InitializeKernel();

    InitializeCameraCopyrights();

    InitializeVcCamera();

    RunKernel();

    //print("\n\nSystem Terminated\n\n");

    // TODO

    ShowAllMessageInQueue();
    ShowAllMachineStates();
    ShowAllTimerValues();

    connectionResult = shutdown(serverConnectionSocket, FLAG_ABORT_CONNECTION);
    //print("Send shutdown error %d\n", connectionResult);

	SPI_ISSUE_DISABLE_WATCHDOG();
}

////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

void    InitializeCameraCopyrights(void)
{

    char type[10];
    int  CamId;


    print("Imaging Technologies Inc. Copyright 2006\n");


    print("Version %d.%d.%d\n", 
          MAJOR_VERSION, MINOR_VERSION, ENGINEERING_VERSION);

    // TODO print fpga and os versions
    // TODO

    GetCameraModel(type);
    print("Camera Type = %s\n", type);

    GetCameraID(&CamId);
    print("Camera ID  = %d\n", CamId);    

//    if ( RIBBON_CAMERA )
//        print("Intelligent Ribbon Camera\n");
//    else
//        print("Intelligent Register Camera\n");
}


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

int GetCameraModel (char *CameraModel)

{
    int  res, error=0, line;
    char text[80], *ptext;

    line=0;
    res = ReadFileLine (ID_FILE, line, 80, text);

    if ( res>=0 )
    {
        ptext = text;

        while ( *ptext != 'V' ) ptext++;
        while ( *ptext ) *CameraModel++ = *ptext++;
        *CameraModel=0;
    }
    else
    {
        error=-1;
    }

    return(error);
}

////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////

int GetCameraID (int *CameraID)
{
    int  res, error=0, nr=0, line;
    char text[80], *ptext;

    line=1;
    res = ReadFileLine (ID_FILE, line, 80, text);

    if ( res>=0 )
    {
        ptext = text;

        while ( *ptext )
        {
            if ( isdigit(*ptext) ) nr = nr * 10 + (*ptext & 0x0F);
            ptext++;
        }

        *CameraID = nr;
    }
    else
    {
        error=-1;
    }

    return(error);
}

////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////

int ReadFileLine (char *FileName, int line, int maxlength, char *text)
{
    int  i, res, error=0;
    FILE *fp;

    // read datas from file 

    fp = io_fopen(FileName, "r");

    if ( fp )
    {
        // skip lines
        for ( i=0; i<line; i++ )
        {
            do
            {
                res=io_fgetc(fp);
                if ( res == '\n' ) break;
            }
            while ( res >= 0 );
        }

        // read line
        for ( i=0; i<maxlength-1; i++ )
        {
            res=io_fgetc(fp);

            if ( res <     0 )
            {
                error = -2; break;
            }
            if ( res == '\n' ) break;

            *text++=(char)res;
        }

        // end of string

        *text=0;

        io_fclose(fp);
    }
    else
    {
        // File doesn't exist
        error = -1;
    }

    return(error);
}
