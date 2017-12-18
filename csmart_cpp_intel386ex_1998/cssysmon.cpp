
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




#include "cssysmon.h"





//////////////////////////////////////////////////
//
// Static Member Definitions
//
//////////////////////////////////////////////////

ColorSmartSystemStatus      ColorSmartSystemMonitor::csStat;



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

ColorSmartSystemMonitor::
ColorSmartSystemMonitor(void) { }

ColorSmartSystemMonitor::
~ColorSmartSystemMonitor(void) { }



//////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////

void
ColorSmartSystemMonitor::LinkSystemMachines(void) {


}



//////////////////////////////////////////////////
//
//  public interface functions :
//
//////////////////////////////////////////////////

ColorSmartSystemStatus  &
ColorSmartSystemMonitor::GetSystemStatus(void) {

        // update csStat and return

        for(int smId=0; smId<LAST_SM_ID; smId++)
        {
            csStat.systemStatus[smId].smCurrState =
                        GetStateMachineState(smId);

            csStat.systemStatus[smId].smErrorCount =
                        GetStateMachineErrorCount(smId);
        }


    return csStat;
}
