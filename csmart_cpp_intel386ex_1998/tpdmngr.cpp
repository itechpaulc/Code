

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




#include "tpdmngr.h"





//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

int     XYCoordinate::GetXCoordinate(void) { return xCoord; }

int     XYCoordinate::GetYCoordinate(void) { return yCoord; }


//////////////////////////////////////////////////
//
// Static Member Definitions
//
//////////////////////////////////////////////////

MoveProfileTable    TableParametersDataManager::moveProfTable;

TableAttributes     TableParametersDataManager::tableAttrib;

TimingParameters    TableParametersDataManager::timingParam;



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

TableParametersDataManager::
TableParametersDataManager(void) { }

TableParametersDataManager::
~TableParametersDataManager(void) { }


//////////////////////////////////////////////////
//
//  Move Profiles
//
//////////////////////////////////////////////////

MoveProfile     &
TableParametersDataManager::GetMoveProfile(MoveProfileIndex mpIdx) {

    switch(mpIdx)
    {
        case TRACK_BALL_X       : return moveProfTable.TrackBallX;
        case TRACK_BALL_Y       : return moveProfTable.TrackBallY;
        case COLOR_BAR_X        : return moveProfTable.ColorBarX;
        case COLOR_BAR_Y        : return moveProfTable.ColorBarY;
        case FIND_LIMIT_OUT_X   : return moveProfTable.FindLimitOutX;
        case FIND_LIMIT_OUT_Y   : return moveProfTable.FindLimitOutY;
        case FIND_LIMIT_IN_X    : return moveProfTable.FindLimitInX;
        case FIND_LIMIT_IN_Y    : return moveProfTable.FindLimitInY;
        case FULL_SPEED_X       : return moveProfTable.FullSpeedX;
        case FULL_SPEED_Y       : return moveProfTable.FullSpeedY;
        case GOING_HOME_SLOW_X  : return moveProfTable.GoingHomeSlowX; 

        default :
            // not valid
            return moveProfTable.GoingHomeSlowX;
    }
}


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  Table Attributes
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

XYCoordinate &
TableParametersDataManager::GetBacklashOffset(void) {

    return  tableAttrib.backlashOffset;
}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

XYCoordinate &
TableParametersDataManager::GetMinTablePosition(void) {

    return  tableAttrib.minTablePosition;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

XYCoordinate &
TableParametersDataManager::GetMaxTablePosition(void) {

    return  tableAttrib.maxTablePosition;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

XYCoordinate &
TableParametersDataManager::GetHomeLocation(void) {

    return  tableAttrib.homeLocation;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

XYCoordinate &
TableParametersDataManager::GetWhitePlaqueLocation(void) {

    return  tableAttrib.whitePlaqueLocation;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

XYCoordinate &
TableParametersDataManager::GetBlackHoleLocation(void) {

    return  tableAttrib.blackHoleLocation;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

XYCoordinate &
TableParametersDataManager::GetTargetLampOffset(void) {

    return  tableAttrib.targetLampOffset;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

XYCoordinate &
TableParametersDataManager::GetFindLimitMoveOutDistance(void) {

    return  tableAttrib.findLimitMoveOutDistance;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

XYCoordinate &
TableParametersDataManager::GetGoingHomeSlowXPosition(void) {

    return  tableAttrib.goingHomeSlowXPosition;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TableParametersDataManager::GetColorBarLeadOffsetX(void) {

    return tableAttrib.colorBarLeadOffsetX;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TableParametersDataManager::GetColorBarStartOffsetX(void) {

    return tableAttrib.colorBarStartOffsetX;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TableParametersDataManager::GetColorBarEndOffsetX(void) {

    return tableAttrib.colorBarEndOffset;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TableParametersDataManager::GetAlignScanLeadOffset(void) {

    return tableAttrib.alignScanLeadOffset;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TableParametersDataManager::GetAlignScanStartOffset(void) {

    return tableAttrib.alignScanStartOffset;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TableParametersDataManager::GetAlignScanEndOffset(void) {

    return tableAttrib.alignScanEndOffset;
}




//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// Timing Parameters
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

WORD
TableParametersDataManager::GetPointToPointMoveSettleTime(void) {

    return  timingParam.pointToPointMoveSettleTime;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TableParametersDataManager::GetTargetAlignMoveSettleTime(void) {

    return  timingParam.targetAlignMoveSettleTime;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TableParametersDataManager::GetReverseBacklashSettleTime(void) {

    return  timingParam.reverseBacklashSettleTime;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TableParametersDataManager::GetForwardBacklashSettleTime(void) {

    return  timingParam.forwardBacklashSettleTime;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TableParametersDataManager::GetMaxTargetLampOnTime(void) {

    return  timingParam.maxTargetLampOnTime;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TableParametersDataManager::GetMeasurementFlashRechargeTime(void) {

    return  timingParam.measurementFlashRechargeTime;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TableParametersDataManager::GetAirOnHeadDownTime(void) {

    return  timingParam.airOnHeadDownTime;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TableParametersDataManager::GetAirOffHeadUpTime(void) {

    return  timingParam.airOffHeadUpTime;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TableParametersDataManager::GetHeadCommTimeOut(void) {

    return  timingParam.headCommTimeOut;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TableParametersDataManager::GetMotorCommTimeOut(void) {

    return  timingParam.motorCommTimeOut;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TableParametersDataManager::GetPciCommTimeOut(void) {

    return  timingParam.pciCommTimeOut;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
TableParametersDataManager::GetMeasurementCommTimeOut(void) {

    return  timingParam.measurementCommTimeOut;
}





