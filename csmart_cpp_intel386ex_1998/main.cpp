



/////////////////////////////////////////////////////////////////////////////
//
//
//    $Header:      $
//    $Log:         $
//
//
//    Author : Paul Calinawan        January 1998
//
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//    NOTE:
//
//    This document contains CONFIDENTIAL and proprietary information
//    which is the property of Graphics Microsystems, Inc. It may not
//    be copied or transmitted in whole or in part by any means to any
//    media without Graphics Microsystems Inc's prior written permission.
//
/////////////////////////////////////////////////////////////////////////////



#include "kernel.h"



#include "xatimer.h"
#include "yatimer.h"
#include "mtrcomm.h"

#include "hccomm.h"
#include "meascomm.h"


#include "hdopcoor.h"
#include "tltimer.h"

#include "hmmngr.h"


#include "ledstat.h"

#include "csmngr.h"


#include "tblctrl.h"
#include "spdmngr.h"



void main(void)
{

    //////////////////////////////////////////////////
    //
    // Initialize 386EX Perhiperals
    //
    //////////////////////////////////////////////////

    InitPerhiperals();


    //////////////////////////////////////////////////
    //
    // Initialize Kernel
    //
    //////////////////////////////////////////////////

    InitKernel();



    //////////////////////////////////////////////////
    //
    // Instantiate the state machines
    //
    //  Subordinate first then followed by the
    //  subordiante managers
    //
    //////////////////////////////////////////////////

    XAxisTimer                      XAT(XAxisTimerID);
    YAxisTimer                      YAT(YAxisTimerID);
    MotorCommMachine                MTRC(MotorCommID);

    HeadCommandCommMachine          HCC(HeadCommandCommID);
    MeasurementCommMachine          MCM(MeasurementCommMachineID);

    HeadMachinesManager             HMM(HeadMachinesManagerID);
    TargetLampTimer                 TLT(TargetLampTimerID);


    SysLedStatusMachine             SYS_SLM(SysLedStatusMachineID);
    PciLedStatusMachine             PCI_SLM(PciLedStatusMachineID);
    MotorCommLedStatusMachine       MTRC_SLM(MotorCommLedStatusMachineID);
    HeadCmdCommLedStatusMachine     HCC_SLM(HeadCmdCommLedStatusMachineID);
    HeadMeasLedStatusMachine        MCM_SLM(HeadMeasLedStatusMachineID);

    HeadOperationsCoordinator       HOC(HeadOperationsCoordinatorMachineID);

    ColorSmartManager               CSM(ColorSmartManagerID);



    //////////////////////////////////////////////////
    //
    // Instantiate the other static
    //  (No States) machines
    //
    //////////////////////////////////////////////////

    TableControl                    TC;
    ScanParametersDataManager       SPDM;



    //////////////////////////////////////////////////
    //
    // Add to entries the Kernel
    //
    //////////////////////////////////////////////////

    AddStateMachineToKernel(&XAT);
    AddStateMachineToKernel(&YAT);
    AddStateMachineToKernel(&MTRC);

    AddStateMachineToKernel(&HCC);
    AddStateMachineToKernel(&MCM);

    AddStateMachineToKernel(&HMM);
    AddStateMachineToKernel(&TLT);

    AddStateMachineToKernel(&SYS_SLM);
    AddStateMachineToKernel(&PCI_SLM);
    AddStateMachineToKernel(&MTRC_SLM);
    AddStateMachineToKernel(&HCC_SLM);
    AddStateMachineToKernel(&MCM_SLM);

    AddStateMachineToKernel(&HOC);

    AddStateMachineToKernel(&CSM);


    //////////////////////////////////////////////////
    //
    // Initial Messsages
    //
    //
    //////////////////////////////////////////////////

    SendHiPrMsg(SysLedStatusMachineID, TimeOut);
    SendHiPrMsg(PciLedStatusMachineID, TimeOut);
    //SendHiPrMsg(MotorCommLedStatusMachineID, TimeOut);
    //SendHiPrMsg(HeadCmdCommLedStatusMachineID, TimeOut);
    //SendHiPrMsg(HeadMeasLedStatusMachineID, TimeOut);


    //////////////////////////////////////////////////
    //
    // Run
    //
    //////////////////////////////////////////////////

    RunKernel();
}


