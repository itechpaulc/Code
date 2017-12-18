



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
#include "mtrcomm.h"
#include "hccomm.h"
#include "mftimer.h"
#include "hmmngr.h"
#include "ledstat.h"
#include "csmngr.h"
#include "prxcomm.h"
#include "ptxcomm.h"
#include "tblctrl.h"
#include "xatimer.h"
#include "yatimer.h"
#include "meascomm.h"
#include "spdmngr.h"
#include "hdopcoor.h"
#include "tlmngr.h"
#include "tpdmngr.h"
#include "mdatmngr.h"



//////////////////////////////////////////////////
//
// Color Smart Boot - Main Entry
//
//////////////////////////////////////////////////

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

    SysLedStatusMachine             SYS_SLM(SysLedStatusMachineID);

    MotorCommMachine                MTRC(MotorCommID);


    PciLedStatusMachine             PCI_SLM(PciLedStatusMachineID);
    MotorCommLedStatusMachine       MTRC_SLM(MotorCommLedStatusMachineID);
    HeadCmdCommLedStatusMachine     HCC_SLM(HeadCmdCommLedStatusMachineID);
    HeadMeasLedStatusMachine        MCM_SLM(HeadMeasLedStatusMachineID);

    XAxisTimer                      XAT(XAxisTimerID);
    YAxisTimer                      YAT(YAxisTimerID);

    MeasurementFlashTimer           MFT(MeasurementFlashTimerID);
    HeadCommandCommMachine          HCC(HeadCommandCommID);
    MeasurementCommMachine          MCM(MeasurementCommMachineID);
    HeadMachinesManager             HMM(HeadMachinesManagerID);
    TargetLampManager               TLM(TargetLampManagerID);
    HeadOperationsCoordinator       HOC(HeadOperationsCoordinatorMachineID);
    ColorSmartManager               CSM(ColorSmartManagerID);
    PciTxCommMachine                PTC(PciTxCommID);
    PciRxCommMachine                PRC(PciRxCommID);



    //////////////////////////////////////////////////
    //
    // Instantiate the other static
    //  (No States) machines
    //
    //////////////////////////////////////////////////

    TableControl                    TC;


    ScanParametersDataManager       SPDM;
    TableParametersDataManager      TPDM;
    MeasurementDataManager          MDM;
    ColorSmartSystemMonitor         CSMON;


    //////////////////////////////////////////////////
    //
    // Add to entries the Kernel
    //
    //////////////////////////////////////////////////


    AddStateMachineToKernel(&MTRC);


    AddStateMachineToKernel(&HCC);
    AddStateMachineToKernel(&HMM);
    AddStateMachineToKernel(&CSM);
    AddStateMachineToKernel(&SYS_SLM);

    AddStateMachineToKernel(&XAT);
    AddStateMachineToKernel(&YAT);
    AddStateMachineToKernel(&MCM);
    AddStateMachineToKernel(&TLM);

    AddStateMachineToKernel(&PCI_SLM);
    AddStateMachineToKernel(&MTRC_SLM);
    AddStateMachineToKernel(&HCC_SLM);
    AddStateMachineToKernel(&MCM_SLM);

    AddStateMachineToKernel(&HOC);
    AddStateMachineToKernel(&PTC);
    AddStateMachineToKernel(&PRC);
    AddStateMachineToKernel(&MFT);


    //////////////////////////////////////////////////
    //
    // LINK THE MACHINES
    //
    //////////////////////////////////////////////////


    HMM.LinkAll(&TPDM, &MCM, &MDM, &MTRC, &HCC, &SPDM, &TC, &MFT);
    CSM.LinkAll(&PTC, &PRC, &HMM, &SPDM, &MDM, &HOC);
    PRC.LinkAll(&PTC, &CSMON);
    HCC.LinkAll(&TPDM);
    MTRC.LinkAll(&TPDM, &XAT, &YAT);


    //////////////////////////////////////////////////
    //
    // Initial Messsages
    //
    //
    //////////////////////////////////////////////////

    SendLowPrKernelMsg(SysLedStatusMachineID, TimeOut);
    SendLowPrKernelMsg(PciLedStatusMachineID, TimeOut);
    SendLowPrKernelMsg(MotorCommLedStatusMachineID, TimeOut);
    SendLowPrKernelMsg(HeadCmdCommLedStatusMachineID, TimeOut);
    SendLowPrKernelMsg(HeadMeasLedStatusMachineID, TimeOut);


    //////////////////////////////////////////////////
    //
    // Run Color Smart Boot Code
    //
    //////////////////////////////////////////////////

    RunKernel();
}


