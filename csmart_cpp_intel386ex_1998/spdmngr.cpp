

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



#include "spdmngr.h"



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// ScanCoordinateData -
//      Structure member function Definitions
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

void
ScanCoordinateData::SetCoordinates(int xCoord, int yCoord)
{
    xCoordinate = xCoord;
    yCoordinate = yCoord;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ScanCoordinateData::GetCoordinates(int &xCoord, int &yCoord)
{
    xCoord = xCoordinate;
    yCoord = yCoordinate;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ScanCoordinateData::SetXCoordinate(int xCoord)
{
    xCoordinate = xCoord;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

int
ScanCoordinateData::GetXCoordinate(void)
{
    return xCoordinate;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ScanCoordinateData::SetYCoordinate(int yCoord)
{
    yCoordinate = yCoord;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

int
ScanCoordinateData::GetYCoordinate(void)
{
    return yCoordinate;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL
ScanCoordinateData::IsColorBarPoint(void)
{
    return (scanAttribute & COLOR_BAR_MASK) ?
        TRUE : FALSE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BYTE
ScanCoordinateData::GetGroupingId(void)
{
    return (BYTE)(scanAttribute & COLOR_BAR_MASK);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ScanCoordinateData::Clear(void)
{
    xCoordinate = yCoordinate = 0x00000000;

    scanAttribute = 0x00;
}




//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// ScanParametersDataManager -
//      class member function Definitions
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

ScanParametersDataManager::ScanParametersDataManager(void)
{
    Reset();
}

ScanParametersDataManager::~ScanParametersDataManager(void) { }


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ScanParametersDataManager::PutNext(ScanCoordinateData &scData)
{
    scanParameters[scanParameterIndex] = scData;

    ++scanParameterIndex;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
ScanParametersDataManager::GetPointCoordiateCount(void)
{
    return  scanParameterIndex;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ScanParametersDataManager::Reset(void)
{
    for(int idx=0; idx<MAX_SCANS; idx++)
        scanParameters[idx].Clear();

    scanParameterIndex = 0;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL
ScanParametersDataManager::IsEmpty(void)
{
    return (scanParameterIndex == 0) ?
        TRUE : FALSE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL
ScanParametersDataManager::IsFull(void)
{
    return (scanParameterIndex == MAX_SCANS) ?
        TRUE : FALSE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

ScanCoordinateData    &
ScanParametersDataManager::GetAt(WORD index)
{
    return  scanParameters[index];
}


