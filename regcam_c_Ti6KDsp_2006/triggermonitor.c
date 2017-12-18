




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


#include "triggermonitor.h"



////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////

int     currentCapturePage;

int     nextImagePageAddress;

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

void    Init_TriggerMonitor(void)
{


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

NEW_STATE   TGM_exitA(void)
{
    //print("TGM IS ACTIVE\n\n");

    // Set Screen Capture

    currentCapturePage = VC_GET_START_CAPTURE_PAGE();


    return TGM_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// RestartImageTracking while in TGM_ACTIVE
//
////////////////////////////////////////////////////////////////////

NEW_STATE   TGM_exitB(void)
{
//     VC_SET_SCREEN_CAPTURE_PAGE(currentCapturePage);
//
//     VC_ENABLE_PICTURE_INTERRUPT();


    //CAMERA_IS_READY_FOR_CAPTURE();

    //SendMessage(THIS_MACHINE, CheckForVcImageStart);
    SendMessage(THIS_MACHINE, CheckForVcImageReady);
    StartTimer(5000);
    //TODO set a timeout
  //  print("TGM B SetUpTrigInt\n");

    //return TGM_ACTIVE_WAIT_FOR_VC_IMAGE_START;
    return TGM_ACTIVE_WAIT_FOR_VC_IMAGE_READY;
}

////////////////////////////////////////////////////////////////////
//
// CheckForVcImageStart while in 
//          TGM_ACTIVE_WAIT_FOR_VC_IMAGE_START
//
////////////////////////////////////////////////////////////////////

NEW_STATE   TGM_exitC(void)
{
    if ( VC_IS_IMAGE_EXPOSING() || VC_IS_IMAGE_STORING() ||  GET_TRIG_SIGNAL() )
    {
        //print("TRIGGER DETECTED C******\n\n");

        SendMessage(THIS_MACHINE, CheckForVcImageReady);

        return TGM_ACTIVE_WAIT_FOR_VC_IMAGE_READY;
    }

    if ( VC_IMAGE_IS_READY() )
    {
        //print("PICTURE TAKEN C******\n\n");

        SendMessage(ImageAcquirerHandler, NewVcImageReady);

        return TGM_ACTIVE;
    }

    SendMessage(THIS_MACHINE, CheckForVcImageStart);

    return TGM_ACTIVE_WAIT_FOR_VC_IMAGE_START;
}

////////////////////////////////////////////////////////////////////
//
// CheckForVcImageReady
//
////////////////////////////////////////////////////////////////////

NEW_STATE   TGM_exitD(void)
{
    if ( VC_IMAGE_IS_READY() )
    {

        //print("TGM D PICTURE TAKEN\n");
        SendMessage(FlashCounterLogger, IncrementFlashCount);

        SendMessage(ImageAcquirerHandler, NewVcImageReady);

        return TGM_ACTIVE;
    }

    SendMessage(THIS_MACHINE, CheckForVcImageReady);

    return SAME_STATE;
}

NEW_STATE   TGM_TO1(void)
{
    //LogString("Image not Ready Time Out");
    return TGM_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_TGM_IDLE)
EV_HANDLER(GoActive, TGM_exitA)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_TGM_ACTIVE)
EV_HANDLER(RestartImageTracking, TGM_exitB)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TGM_ACTIVE_WAIT_FOR_VC_IMAGE_START)
EV_HANDLER(CheckForVcImageStart, TGM_exitC)
STATE_TRANSITION_MATRIX_END;


STATE_TRANSITION_MATRIX(_TGM_ACTIVE_WAIT_FOR_VC_IMAGE_READY)
EV_HANDLER(CheckForVcImageReady, TGM_exitD),
EV_HANDLER(TimeOut, TGM_TO1)
STATE_TRANSITION_MATRIX_END;

// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(TGM_Main_Entry)
STATE(_TGM_IDLE)                            ,           
STATE(_TGM_ACTIVE)                          ,
STATE(_TGM_ACTIVE_WAIT_FOR_VC_IMAGE_START)  ,
STATE(_TGM_ACTIVE_WAIT_FOR_VC_IMAGE_READY)
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////


