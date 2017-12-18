

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



#include "mdatmngr.h"

#include <string.h>

#pragma _builtin_(memset)


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// MeasurementData -
//      Structure member function Definitions
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

BOOL
IsMeasurementDataBad(SCAN_MEASUREMENT_DATA *ptrMd)
{
    return TRUE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
ClearMeasurementData(SCAN_MEASUREMENT_DATA *ptrMd)
{
    memset(ptrMd, 0x00, ATOD_MEASUREMENT_SIZE);
}




//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// MeasurementDataManager -
//      class member function Definitions
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

SCAN_MEASUREMENT_DATA *
MeasurementDataManager::currMeasurementPtr = 0;

SCAN_MEASUREMENT_DATA *
MeasurementDataManager::startMeasurementPtr = 0;

MeasurementDataManager::MeasurementDataManager(void)
{
    Reset();
}

MeasurementDataManager::~MeasurementDataManager(void) { }



//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MeasurementDataManager::PutNext(SCAN_MEASUREMENT_DATA &measData)
{
    (*currMeasurementPtr) = measData;

    ++currMeasurementPtr;
    ++measurementCount;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
MeasurementDataManager::GetMeasurementCount(void)
{
    return  measurementCount;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
MeasurementDataManager::Reset(void)
{
    currMeasurementPtr = startMeasurementPtr;

    for(int idx=0; idx<MAX_MEASUREMENTS; idx++)
    {
        ClearMeasurementData(currMeasurementPtr);
        ++currMeasurementPtr;
    }

    currMeasurementPtr = startMeasurementPtr;
    measurementCount = 0;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL
MeasurementDataManager::IsEmpty(void)
{
    return (measurementCount == 0) ?
        TRUE : FALSE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

BOOL
MeasurementDataManager::IsFull(void)
{
    return (measurementCount == MAX_MEASUREMENTS) ?
        TRUE : FALSE;
}



