




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




#include "hdopcoor.h"




//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  HeadOperationsCoordinator
//
//      - public interface functions :
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

void
HeadOperationsCoordinator::LinkScanParametersDataManager(ScanParametersDataManager *pSPDM)
{
    ptrSPDM = pSPDM;
}

void
HeadOperationsCoordinator::LinkHeadMachinesManager(HeadMachinesManager *pHMM)
{
    ptrHMM = pHMM;
}

//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// HeadOperationsCoordinator - private helper functions
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////




//////////////////////////////////////////////////
//
// HeadOperationsCoordinator - RESPONSE ENTRIES
//
//////////////////////////////////////////////////



//////////////////////////////////////////////////
//
// State Transition Matrices
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_IDLE)
    EV_HANDLER(StartScanning, HOC_h1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(HeadOperationsCoordinator, _HOC_WAITING_FOR_ANY_SCAN_TYPE_DONE)
    EV_HANDLER(ColorBarScanDone, HOC_h2),
    EV_HANDLER(PointToPointScanDone, HOC_h2)
STATE_TRANSITION_MATRIX_END;



//////////////////////////////////////////////////
//
// Matrix Table
//
//////////////////////////////////////////////////

DEFINE_RESPONSE_TABLE_ENTRY(HeadOperationsCoordinator)
    STATE_MATRIX_ENTRY(_HOC_IDLE),
    STATE_MATRIX_ENTRY(_HOC_WAITING_FOR_ANY_SCAN_TYPE_DONE)
RESPONSE_TABLE_END;




//////////////////////////////////////////////////
//
// Static Member Definitions
//
//////////////////////////////////////////////////

WORD        HeadOperationsCoordinator::totalPointsToScan = 0;
WORD        HeadOperationsCoordinator::pointsScanned = 0;

WORD        HeadOperationsCoordinator::startIdx = 0;
WORD        HeadOperationsCoordinator::endIdx = 0;

BYTE        HeadOperationsCoordinator::currColorBarGroupId = 0;

BOOL        HeadOperationsCoordinator::nextSequenceIsColorBar = FALSE;

ScanParametersDataManager   *   HeadOperationsCoordinator::ptrSPDM = 0;

HeadMachinesManager         *   HeadOperationsCoordinator::ptrHMM = 0;



//////////////////////////////////////////////////
//
// HeadOperationsCoordinator - Constructors, Destructors
//
//////////////////////////////////////////////////

HeadOperationsCoordinator::HeadOperationsCoordinator(BYTE sMsysID)
    :StateMachine(sMsysID)
{
    ASSIGN_RESPONSE_TABLE();

    SetCurrState(HOC_IDLE);
}

HeadOperationsCoordinator::~HeadOperationsCoordinator(void) { }



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// HeadOperationsCoordinator - private EXIT PROCEDURES
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h1(void)
{
        totalPointsToScan = ptrSPDM->GetPointCoordiateCount();

        if(totalPointsToScan == 0)
        {
            // Send Message to CSM

            return  HOC_IDLE;
        }

        // Determine the next scan's behavior

        SearchForScanPattern();

        if(nextSequenceIsColorBar)
        {
            // Send message to HMM

        }


        // Send message to HMM


    return  HOC_WAITING_FOR_ANY_SCAN_TYPE_DONE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

WORD
HeadOperationsCoordinator::HOC_h2(void)
{
    pointsScanned = endIdx+1;

        if(pointsScanned == totalPointsToScan)
        {
            // Send Message to CSM

            return  HOC_IDLE;
        }

        // Determine the next scan's behavior

        SearchForScanPattern();

        if(nextSequenceIsColorBar)
        {
            // Send message to HMM


        }


        // Send message to HMM


    return  HOC_WAITING_FOR_ANY_SCAN_TYPE_DONE;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
HeadOperationsCoordinator::SearchForScanPattern(void)
{
        // Look at the first point coordinate attribute

        startIdx = endIdx = pointsScanned;

        if(ptrSPDM->GetAt(startIdx).IsColorBarPoint())
        {
            currColorBarGroupId = ptrSPDM->GetAt(startIdx).GetGroupingId();

            // Search for all the Color Bar, points
            // that has the same grouping identification

            while(  (ptrSPDM->GetAt(endIdx+1).IsColorBarPoint())
                  &&(ptrSPDM->GetAt(endIdx+1).GetGroupingId()==currColorBarGroupId)
                  &&(pointsScanned < totalPointsToScan))
            {
                ++endIdx;
            }

            // Make sure there is at least 2 swatches

            if(startIdx > endIdx)
                nextSequenceIsColorBar = TRUE;
            else
                nextSequenceIsColorBar = FALSE;
        }
        else
        {
            nextSequenceIsColorBar = FALSE;

            // Search for all the point to point, points

            while( !(ptrSPDM->GetAt(endIdx+1).IsColorBarPoint())
                  &&(pointsScanned < totalPointsToScan))
            {
                ++endIdx;
            }
        }

        // Set HMM's start and end scan index, for the next scan

        ptrHMM->SetScanIndexes(startIdx, endIdx);
}



