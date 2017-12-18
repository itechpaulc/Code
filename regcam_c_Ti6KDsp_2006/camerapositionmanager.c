




//
//
//
//  Author :    Paul Calinawan
//
//  Date:       June 28, 2006
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


////////////////////////////////////////////////////////////////////


#include "camerapositionmanager.h"
extern SCANNER_POSITION    GetRequestedScannerStartPosition(void);
extern CAMERA_POSITION  currrentCamPosition;


#define LATERAL_INDEX_VALIDATION    (idx < MAX_CAMERA_SEARCH_POSITIONS_LAT)
#define CIRC_INDEX_VALIDATION       (idx < MAX_CAMERA_SEARCH_POSITIONS_CIRC)

POSITION  cameraLatSearchPositions[MAX_CAMERA_SEARCH_POSITIONS_LAT];
POSITION  cameraCircSearchPositions[MAX_CAMERA_SEARCH_POSITIONS_CIRC];
POSITION  singleLocationLat;
POSITION  singleLocationCirc;


WORD currCameraLatSearchPositionIndex;
WORD currCameraCircSearchPositionIndex;



CAMERA_POSITION     cameraSearchStartPosition;

DWORD               cameraSearchCutoffLength;
DWORD               cameraSearchTransportLength;

DWORD               cameraSearchCircumStepSize;
DWORD               cameraSearchLateralStepSize;


DWORD               cameraSearchNumberOfLatPositions;
DWORD               cameraSearchNumberOfCircPositions;
WORD                cameraNextAvailableLatPosition;
WORD                cameraNextAvailableCircPosition;


int                 limitedSearchMaxRetry;

DWORD               circumAreaLimitedLengthSearch;
DWORD               lateralAreaLimitedLengthSearch;

SEARCH_STRATEGY     searchStrategySelection;

////////////////////////////////////////////////////////////////////
//
// Local Variables
//

int                 cameraNextAvailablePositionLat, cameraNextAvailablePositionCirc;

DWORD               circumAreaLengthSearch;
DWORD               lateralAreaLengthSearch;


////////////////////////////////////////////////////////////////////
//
//

void    ShowAllCameraSearchPositions(void)
{
 //   int p;

//     //print("Lateral Search Locations\n");
//     for ( p=0; p< MAX_CAMERA_SEARCH_POSITIONS_LAT; p++ )
//     {
//         if ( !cameraLatSearchPositions[p].isTaken )
//             print("( %d ) %u\n",p, cameraLatSearchPositions[p].position);
//     }
//
//     print("Circ Search Locations\n");
//     for ( p=0; p< MAX_CAMERA_SEARCH_POSITIONS_CIRC; p++ )
//     {
//         if ( !cameraCircSearchPositions[p].isTaken )
//             print("( %d ) %u\n",p, cameraCircSearchPositions[p].position);
//     }
}

////////////////////////////////////////////////////////////////////
//
//
//

DWORD    NormalizeCircumPosition( I32 circPos)
{
    // Overflow check
    if ( (circPos) < 0l )
        (circPos) += cameraSearchCutoffLength;
    else
        if ( (circPos) > cameraSearchCutoffLength )
        (circPos) -= cameraSearchCutoffLength;

    return(DWORD)circPos;
}

void    NormalizeLateralPosition(long *latPos)
{
    // Overflow check

    if ( (*latPos) < 0 )
        (*latPos) = 0;
    else
        if ( (*latPos) > cameraSearchTransportLength )
        (*latPos) = cameraSearchTransportLength;
}

////////////////////////////////////////////////////////////////////
//
//  Camera Position Parameter Configurations
//      Measurements in 1 micron increments
//

void    SetCameraSearchStartPosition(CAMERA_POSITION camSearchStartPos)
{
    cameraSearchStartPosition = camSearchStartPos;
}

CAMERA_POSITION    GetCameraSearchStartPosition(void)
{
    return cameraSearchStartPosition;
}

void    SetCameraSearchCutoffLength(DWORD camSearchCutoffLen)
{
    cameraSearchCutoffLength = camSearchCutoffLen;
}

DWORD   GetCameraSearchCutoffLength(void)
{
    return  cameraSearchCutoffLength;
}

void    SetCameraSearchTransportLength(DWORD camSearchTransportLen)
{
    cameraSearchTransportLength = camSearchTransportLen;
}

DWORD   GetCameraSearchTransportLength(void)
{
    return  cameraSearchTransportLength;
}

void    SetCameraSearchCircumStepSize(WORD camSearchCircStepSize)
{
    cameraSearchCircumStepSize = camSearchCircStepSize;
}

DWORD   GetCameraSearchCircumStepSize(void)
{
    return cameraSearchCircumStepSize;
}

void    SetCameraSearchLateralStepSize(WORD camSearchLateralStepSize)
{
    cameraSearchLateralStepSize    =    camSearchLateralStepSize;
}

DWORD   GetCameraSearchLateralStepSize(void)
{
    return cameraSearchLateralStepSize;
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

int     GetCameraSearcNumberOfLatPositions(void)
{
    return cameraSearchNumberOfLatPositions;
}

int     GetCameraSearcNumberOfCircPositions(void)
{
    return cameraSearchNumberOfCircPositions;
}

void    SetCameraSearcNumberOfLatPositions(int camSearchNumberOfPos)
{
    cameraSearchNumberOfLatPositions = camSearchNumberOfPos;
}

void    SetCameraSearcNumberOfCircPositions(int camSearchNumberOfPos)
{
    cameraSearchNumberOfCircPositions = camSearchNumberOfPos;
}

void    IncrementCameraSearcNumberOfLatPositions(void)
{
    ++cameraSearchNumberOfLatPositions;
}

void    IncrementCameraSearcNumberOfCircPositions(void)
{
    ++cameraSearchNumberOfCircPositions;
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetCameraLimitedSearchMaxRetry(int limSearchMaxRetry)
{
    limitedSearchMaxRetry = limSearchMaxRetry;
}

int     GetCameraLimitedSearchMaxRetry(void)
{
    return limitedSearchMaxRetry;
}

void    SetCameraCircumAreaLimitedLengthSearch(DWORD circAreaLimLenSearch)
{
    circumAreaLimitedLengthSearch = circAreaLimLenSearch;
}

DWORD   GetCameraCircumAreaLimitedLengthSearch(void)
{
    return circumAreaLimitedLengthSearch;
}

void    SetCameraLateralAreaLimitedLengthSearch(DWORD lateralAreaLimLenSearch)
{
    lateralAreaLimitedLengthSearch = lateralAreaLimLenSearch;
}

DWORD   GetCameraLateralAreaLimitedLengthSearch(void)
{
    return lateralAreaLengthSearch;
}


////////////////////////////////////////////////////////////////////
//
// Limited Search order is as follows
//
//         . . . . .
//         . 6 3 0 .
//         . 4 1 7 .
//         . 5 2 8 .
//         . . . . .
//
// Number of searches is based on
//      circumAreaLimitedLengthSearch; lateralAreaLengthSearch; and
//      their corresponding search step sizes
//

#define     EVEN_STEPS(x)                   ((x) & 0xFFFFFFFE)
#define     ODD_STEPS(x)                     ((x) | 0x00000001)

#define     MIN_CIRC_SEARCH_STEP_SIZE       (2)

// Including one circumferential search iteration

#define     MIN_LAT_SEARCH_STEP_SIZE        (2)


void    FillCircSearchPositions(DWORD camStartPos, BYTE searchType)
{
    int     c, circNumPositions=0;
    I32     currCircPos;
    DWORD   searchLength;

    // Do Starting search point

    // Calculate and Fill all possible positions
    // 0 - will not generate any search positions
    // LIMITED Search has a minimum of 2 search positions
    // All other _FULL Search types handles the wrap around, 
    //  and will create overlapping or duplicate frames

    //  ClearAllPositions();
    ResetCircCameraSearchPosition();

    if ( searchType == LIMITED_AREA )
        searchLength = GetCameraCircumAreaLimitedLengthSearch();
    else
        searchLength = GetCameraSearchCutoffLength();


    if ( circumAreaLengthSearch != 0 && cameraSearchCircumStepSize != 0 )
    {
        circNumPositions = MIN_CIRC_SEARCH_STEP_SIZE +
                           (EVEN_STEPS(searchLength / cameraSearchCircumStepSize));

        if ( circNumPositions > MAX_CAMERA_SEARCH_POSITIONS_CIRC )
            circNumPositions = MAX_CAMERA_SEARCH_POSITIONS_CIRC;
    }

    currCircPos = camStartPos;

 //   print("CIRC-Start:%d ",currCircPos); 
 //   print("Length:%d ",searchLength); 
 //   print("Step:%d\n" ,cameraSearchCircumStepSize); 
 //
 //   print("CIRC NUM POS: %u\n", circNumPositions); 


    InsertNextCircCameraSearchPosition( currCircPos );

    currCircPos = camStartPos - (searchLength/2) ;

    for ( c=0; c < circNumPositions; c++ )
    {
        // Force Linear Search based on searchStrategySelection
        // else Alternating is done
        currCircPos = NormalizeCircumPosition( currCircPos);
        InsertNextCircCameraSearchPosition( currCircPos );
        currCircPos += cameraSearchCircumStepSize;
    }
}

void    FillLatSearchPositions(DWORD camStartPos)
{
    long    currSearchLeftPosition, currSearchRightPosition,
        currCircPos, currLateralPos;

    int     l,  latNumPositions=0;
    BOOL    left = TRUE;

    // Do Starting search point

    // Calculate and Fill all possible positions
    // 0 - will not generate any search positions
    // LIMITED Search has a minimum of 2 search positions
    // All other _FULL Search types handles the wrap around, 
    //  and will create overlapping or duplicate frames

    if ( lateralAreaLengthSearch != 0 && cameraSearchLateralStepSize != 0 )
    {
        latNumPositions     = MIN_LAT_SEARCH_STEP_SIZE +
                              (EVEN_STEPS(lateralAreaLengthSearch / cameraSearchLateralStepSize));

        if ( latNumPositions > MAX_CAMERA_SEARCH_POSITIONS_LAT )
            latNumPositions = MAX_CAMERA_SEARCH_POSITIONS_LAT;
    }

    currLateralPos = 
        currSearchLeftPosition = 
        currSearchRightPosition = camStartPos;

//    print("LAT-Start:%d ",currLateralPos); 
//    print("Length:%d ",lateralAreaLengthSearch); 
//    print("Step:%d\n", cameraSearchLateralStepSize); 
//
//    print("LAT NUM POS: %u\n", latNumPositions); 

    InsertNextLatCameraSearchPosition(currLateralPos);

    for ( l=0; l < latNumPositions; l++ )
    {
        if ( left == TRUE )
        {
            currSearchLeftPosition -= cameraSearchLateralStepSize;

            currLateralPos = currSearchLeftPosition;
            left = FALSE;
        }
        else
        {
            currSearchRightPosition += cameraSearchLateralStepSize;

            // Limit distance check TODO
            currLateralPos = currSearchRightPosition;
            left = TRUE;
        }

        if ( currLateralPos < 0 )
            currLateralPos = 0;

        if ( currLateralPos > cameraSearchTransportLength )
            currLateralPos = cameraSearchTransportLength;

        InsertNextLatCameraSearchPosition(currLateralPos);
    }
}

void    FillSearchPositions(BYTE searchType )
{
    SCANNER_POSITION startLocation = GetRequestedScannerStartPosition() ;

    FillCircSearchPositions(startLocation.circumPosition, searchType);
    FillLatSearchPositions(startLocation.lateralPosition);
}



void    PrepareCameraSearchPositions(SEARCH_STRATEGY searchStrategy)
{
    SetSearchStrategySelection(searchStrategy);

    ClearSearchPositions();

    switch ( searchStrategy )
    {
        case    LIMITED_ALTERNATING_SEQUENCE:
            PrepareLimitedAlternatingSearchPositions();
            break;

        case    FULL_ALTERNATING_SEQUENCE:
            PrepareFullAlternatingSearchPositions();
            break;

        case    FULL_LINEAR_SEQUENCE:
            PrepareFullLinearSearchPositions();
            break;

        case    FULL_SMART_SEQUENCE:
            PrepareFullSmartSearchPositions();
            break;

        case    FULL_CUSTOM_SEQUENCE:
            PrepareFullCustomSearchPositions();
            break;

        case    MANUAL_SEARCH:
            break;

    }
}

//
//
//

void    PrepareLimitedAlternatingSearchPositions(void)
{
    circumAreaLengthSearch = circumAreaLimitedLengthSearch;
    lateralAreaLengthSearch = lateralAreaLimitedLengthSearch;
    FillSearchPositions( LIMITED_AREA );
}

void    PrepareFullAlternatingSearchPositions(void)
{
    circumAreaLengthSearch = GetCameraSearchCutoffLength();
    lateralAreaLengthSearch = lateralAreaLimitedLengthSearch;
    FillSearchPositions( FULL_CIRC );
}

void    PrepareFullLinearSearchPositions(void)
{
    circumAreaLengthSearch = GetCameraSearchCutoffLength();
    lateralAreaLengthSearch = lateralAreaLimitedLengthSearch;
    FillSearchPositions( FULL_CIRC );
}

void    PrepareFullSmartSearchPositions(void)
{
    circumAreaLengthSearch = GetCameraSearchCutoffLength();
    lateralAreaLengthSearch = lateralAreaLimitedLengthSearch;
    FillSearchPositions( FULL_CIRC );
}

void    PrepareFullCustomSearchPositions(void)
{

}


void    
    SetSearchStrategySelection
    (SEARCH_STRATEGY searchStrategySel)
{
    searchStrategySelection = searchStrategySel;
}

SEARCH_STRATEGY     
    GetCameraSearchStrategySelection(void)
{
    return searchStrategySelection;
}


////////////////////////////////////////////////////////////////////

void    ClearSearchPositions(void)
{
    ClearAllPositions();
    ResetCameraSearchPosition();
    SetCameraSearcNumberOfLatPositions(0);
    SetCameraSearcNumberOfCircPositions(0);
}

// void    SetCustomCameraSearchPositions(CAMERA_POSITION* srcCustomPos, int numPos)
// {
//     CAMERA_POSITION* ptrSrcCustomPos = srcCustomPos;
//
//     int p;
//
//     for ( p=0; p<numPos; p++ )
//     {
//         cameraSearchPositions[p] = (*ptrSrcCustomPos);
//         cameraSearchPositions[p].isTaken = FALSE;
//
//         ++ptrSrcCustomPos;
//     }
//
//     SetCameraSearcNumberOfPositions(numPos);
// }

void    ResetLatCameraSearchPositionAt(BYTE axis, WORD idx )
{
    if ( idx < MAX_CAMERA_SEARCH_POSITIONS_LAT )
        cameraLatSearchPositions[idx].isTaken = TRUE;
}

void    ResetCircCameraSearchPositionAt(BYTE axis, WORD idx )
{
    if ( idx < MAX_CAMERA_SEARCH_POSITIONS_CIRC )
        cameraCircSearchPositions[idx].isTaken = TRUE;
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    InitializeCameraPositionManager(void)
{
    CAMERA_POSITION camSearchInitPos;

    cameraNextAvailablePositionLat  = 0x0000;
    cameraNextAvailablePositionCirc = 0x0000;

    camSearchInitPos.circumPosition = NULL_CAMERA_POSITION;
    camSearchInitPos.lateralPosition = NULL_CAMERA_POSITION;

    SetCameraSearchStartPosition(camSearchInitPos);

    SetCameraSearchCutoffLength(0x00000000);
    SetCameraSearchTransportLength(0x00000000);

    SetCameraCircumAreaLimitedLengthSearch(0x00000000);
    SetCameraLateralAreaLimitedLengthSearch(0x00000000);

    SetCameraSearchCircumStepSize(0x0000);
    SetCameraSearchLateralStepSize(0x0000);

    ClearSearchPositions();

    // TODO

//    SetCameraSearchCutoffLength(1160018); //Conley
    SetCameraSearchCutoffLength(638372);
    SetCameraSearchTransportLength(10721);

    SetCameraCircumAreaLimitedLengthSearch(152400);
    SetCameraLateralAreaLimitedLengthSearch(912);  //Transport step size is 55.175um
//    SetCameraCircumAreaLimitedLengthSearch(76200);
//    SetCameraLateralAreaLimitedLengthSearch(292);

//     SetCameraSearchCircumStepSize(20000);  // 1/3 overlap
//     SetCameraSearchLateralStepSize(331);

 //   SetCameraSearchCircumStepSize(18000);  // 1/3 overlap Ribbon
 //   SetCameraSearchLateralStepSize( 200 ); //Ribbon

    SetCameraSearchCircumStepSize(6000);  // 1/3 overlap
    SetCameraSearchLateralStepSize( 200 );


    PrepareLimitedAlternatingSearchPositions();
    ShowAllCameraSearchPositions();

    //print("\nLAT : %u \n", lateralAreaLengthSearch);
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    ClearAllPositions(void)
{
    int p;

    cameraNextAvailablePositionLat  = 0x0000;
    cameraNextAvailablePositionCirc = 0x0000;
    cameraSearchNumberOfLatPositions= 0;
   // printf("ClearAllPositions numLatPos %d\n",cameraSearchNumberOfLatPositions);

    for ( p=0; p<MAX_CAMERA_SEARCH_POSITIONS_LAT; p++ )
    {
        cameraLatSearchPositions[p].position = NULL_CAMERA_POSITION;

        cameraLatSearchPositions[p].isTaken = TRUE;
    }

    for ( p=0; p<MAX_CAMERA_SEARCH_POSITIONS_CIRC; p++ )
    {
        cameraCircSearchPositions[p].position = NULL_CAMERA_POSITION;

        cameraCircSearchPositions[p].isTaken = TRUE;
    }

}

void    SetLatPosition(WORD idx, DWORD pos)
{
    if ( idx < MAX_CAMERA_SEARCH_POSITIONS_LAT )
        cameraLatSearchPositions[idx].position = pos;

  //  else
        //print("\nSetLatPosition Invalid:0x%x Pos: 0x%x Idx: 0x%x", \
       //       pos, idx);
}

void    SetCircPosition(WORD idx, DWORD pos)
{
    if ( idx < MAX_CAMERA_SEARCH_POSITIONS_CIRC )
        cameraCircSearchPositions[idx].position = pos;

 //   else
 //      // print("\nSetCircPosition Invalid: Pos: 0x%x Idx: 0x%x", \
 //             pos, idx);
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

DWORD   GetCircumPositionAt(int idx)
{
    cameraCircSearchPositions[idx].isTaken = TRUE;
    cameraSearchNumberOfCircPositions--;
    return cameraCircSearchPositions[idx].position;
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

DWORD   GetLateralPositionAt(int idx)
{
    cameraLatSearchPositions[idx].isTaken;
    cameraSearchNumberOfLatPositions--;
    return cameraLatSearchPositions[idx].position;
}


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    ResetCameraSearchPosition(void)
{
    cameraSearchNumberOfLatPositions = 0;
    cameraSearchNumberOfCircPositions = 0;
    currCameraLatSearchPositionIndex = 0;
    currCameraCircSearchPositionIndex = 0;
    cameraNextAvailableLatPosition = 0;
    cameraNextAvailableCircPosition = 0;

    //printf("ResetCameraSearchPosition numLatPos %d\n",cameraSearchNumberOfLatPositions);
}

void    ResetCircCameraSearchPosition(void)
{
    int p;

    cameraSearchNumberOfCircPositions = 0;
    currCameraCircSearchPositionIndex = 0;
    cameraNextAvailableCircPosition = 0;

    for ( p=0; p<MAX_CAMERA_SEARCH_POSITIONS_CIRC; p++ )
    {
        cameraCircSearchPositions[p].position = NULL_CAMERA_POSITION;

        cameraCircSearchPositions[p].isTaken = TRUE;
    }
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    InsertNextLatCameraSearchPosition( DWORD pos)
{
    if ( cameraNextAvailableLatPosition >= MAX_CAMERA_SEARCH_POSITIONS_LAT )
    {
        //print("\nInsertNextLatCameraSearchPosition: INVALID INDEX: 0x%x",
      //        cameraNextAvailableLatPosition);

        return;
    }


    cameraLatSearchPositions[cameraNextAvailableLatPosition].position   = pos;
    cameraLatSearchPositions[cameraNextAvailableLatPosition].isTaken    = FALSE;

    cameraNextAvailableLatPosition++;

    IncrementCameraSearcNumberOfLatPositions();
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    InsertNextCircCameraSearchPosition( DWORD pos)
{
    if ( cameraNextAvailableCircPosition >= MAX_CAMERA_SEARCH_POSITIONS_CIRC )
    {
        //print("\nInsertNextCircCameraSearchPosition: INVALID INDEX: 0x%x",
         //     cameraNextAvailableCircPosition);

        return;
    }


    cameraCircSearchPositions[cameraNextAvailableCircPosition].position   = pos;
    cameraCircSearchPositions[cameraNextAvailableCircPosition].isTaken    = FALSE;

    cameraNextAvailableCircPosition++;

    IncrementCameraSearcNumberOfCircPositions();
}

void SetCurrentCamLatPosition( DWORD camPos )
{
    currrentCamPosition.lateralPosition = camPos;
}


DWORD GetNextCameraLatSeachPosition(void)
{
    DWORD camPos =
        cameraLatSearchPositions[currCameraLatSearchPositionIndex].position;

    cameraLatSearchPositions[currCameraLatSearchPositionIndex].isTaken = TRUE;

    cameraSearchNumberOfLatPositions--;
    currCameraLatSearchPositionIndex++;

    currrentCamPosition.lateralPosition = camPos;
    return camPos;
}

DWORD GetNextCameraCircSeachPosition(void)
{
    DWORD camPos =
        cameraCircSearchPositions[currCameraCircSearchPositionIndex].position;

    cameraCircSearchPositions[currCameraCircSearchPositionIndex].isTaken = TRUE;

    currCameraCircSearchPositionIndex++;
    cameraSearchNumberOfCircPositions--;
    return camPos;
}


////////////////////////////////////////////////////////////////////
//
// Return the closest 
// 
// 
////////////////////////////////////////////////////////////////////

CAMERA_POSITION GetNextCameraSmartSeachPosition(DWORD currEncIdx)
{
    // int selectedSearchIndex;

    CAMERA_POSITION camPos;

    // TODO 
    // Time Based??

    //CAMERA_POSITION camPos =
    //    cameraSearchPositions[currCameraSearchPositionIndex];

    //currCameraSearchPositionIndex++;

    //selectedSearchIndex = 0;

    camPos.circumPosition = 0;
    camPos.lateralPosition = 0;
    camPos.isTaken = TRUE;

    return camPos;
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

BOOL   IsLatPositionTaken(int idx)
{
    return cameraLatSearchPositions[idx].isTaken;
}

BOOL   IsCircPositionTaken(int idx)
{
    return cameraCircSearchPositions[idx].isTaken;
}


