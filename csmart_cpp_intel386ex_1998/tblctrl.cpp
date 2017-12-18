

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



#include "80386ex.h"


#include "tblctrl.h"


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

TableControl::TableControl(void)
{
    AirOffProbeHeadUp();
    HoldProbeHeadReset();
    HoldMotorChipSetReset();
}

TableControl::~TableControl(void) { }


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////
/*
void
TableControl::AirOnProbeHeadDown(void)
{
    p1Status = GetIO1Latch();       // Read Port 1

    p1Status |= AIR_BEARING_PORT;   // Set Air Bearing Port

    SetIO1Latch(p1Status);          // Write to Port

    airBearingOn = TRUE;
}
*/

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
TableControl::AirOffProbeHeadUp(void)
{
    p1Status = GetIO1Latch();       // Read Port 1

    p1Status &= ~AIR_BEARING_PORT;  // Clear Air Bearing Port

    SetIO1Latch(p1Status);          // Write to Port

    airBearingOn = FALSE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL
TableControl::IsAirOnProbeHeadDown(void)
{
    return airBearingOn;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
TableControl::HoldProbeHeadReset(void)
{
    p1Status = GetIO1Latch();       // Read Port 1

    p1Status &= ~HEAD_RESET_PORT;   // Clear Head Reset Port

    SetIO1Latch(p1Status);          // Write to Port
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
TableControl::ReleaseProbeHeadReset(void)
{
    p1Status = GetIO1Latch();       // Read Port 1

    p1Status |= HEAD_RESET_PORT;    // Set Head Reset Port

    SetIO1Latch(p1Status);          // Write to Port
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
TableControl::HoldMotorChipSetReset(void) {


}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
TableControl::ReleaseMotorChipSetReset(void) {


}



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL
TableControl::Is_48Volt_MotorDriverPowerOK(void) {

    WORD    auxInStatus = *(AUX_INPUT_ADDRESS);

        return (auxInStatus & DIAG_48V_MOTOR_DRIVER_POWER_STATUS) ?
            TRUE : FALSE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL
TableControl::Is_12Volt_HeadPowerSupplyOK(void) {

    WORD    auxInStatus = *(AUX_INPUT_ADDRESS);

        return (auxInStatus & DIAG_12V_INTERCONNECT_HEAD_POWER_STATUS) ?
            TRUE : FALSE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL
TableControl::Is_5Volt_InterconnectBoardPowerOK(void) {

    WORD    auxInStatus = *(AUX_INPUT_ADDRESS);

        return (auxInStatus & DIAG_5V_POWER_SUPPLY_STATUS) ?
            TRUE : FALSE;
}











