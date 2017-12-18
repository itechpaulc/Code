




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

#include "kernel.h"

#include "imageprocessinghandler.h"

#include "markrecognitionhandler.h"
#include "registermarkrecognitionhandler.h"
#include "cameraconfig.h"



///////////////////////////////////////////////////////////////////
//
// Utility Function Definitions
//
///////////////////////////////////////////////////////////////////
extern BOOL    diagnosticsEnabled;
extern BOOL     topEdgeFirst; //PM don't check in


int     finalThresh;

extern int maxElementArea;
///////////////////////////////////////////////////////////////////
//
//
//
///////////////////////////////////////////////////////////////////



// Area of the circle/oval is nominally 30% smaller than its perimeter

///////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//
//
//
///////////////////////////////////////////////////////////////////
#define CIRCLE_MIN_AREA_03      (25)
#define CIRCLE_NOM_AREA_03      (49)
#define CIRCLE_MAX_AREA_03      (100)
#define CIRCLE_MIN_WIDTH_03     (5)
#define CIRCLE_NOM_WIDTH_03     (7)
#define CIRCLE_MAX_WIDTH_03     (10)
#define CIRCLE_MIN_HEIGHT_03    (5)
#define CIRCLE_NOM_HEIGHT_03    (7)
#define CIRCLE_MAX_HEIGHT_03    (10)
#define COMPLEX_MIN_AXIS        (5)

#define PERIMITER_TOL_03        (4)

#define ACTIVE_MARKS            (5)

#define MIN_MARK_FOUND          (2)

VECTOR NormalizeRegElementVectors( ELEMENT_PAIR_DEFINITION *elementPair )
{
    VECTOR currentVector;

    int temp;

    currentVector.y = elementPair->nominalHorizontalDistToPartner;
    currentVector.x = elementPair->nominalVerticalDistToPartner;

    if ( currentSystemConfiguration.cameraImaging.rotateImage )
    {
        temp = currentVector.y;
        currentVector.y = -currentVector.x;
        currentVector.x = temp;
    }

    if ( topEdgeFirst )
    {
        currentVector.x = -currentVector.x;     
        currentVector.y = -currentVector.y;     
    }

    return currentVector;
}

void    InitializeRegPatternRecognitionDefaults(void)
{
    int x;
    BYTE  markDefIndex; 
    ELEMENT_PAIR_DEFINITION elemPair;

    CameraImaging   *camImagingPtr  = 
    &GetCurrSystemConfig()->cameraImaging;

    SET_WHITE_LEVEL_TARGET(REGISTER_WHITE_LEVEL_TARGET);
    SET_RANGE_SHIFT_DIVISOR(REGISTER_RANGE_SHIFT_DIVISOR);
    SET_START_THRESHOLD_MULTIPLIER(REGISTER_START_THRESHOLD_MULTIPLIER);

    SET_MAX_STEPPING_RETRIES_SEARCHING(REGISTER_MAX_STEPPING_RETRIES_SEARCHING);
    SET_MAX_STEPPING_RETRIES_LOCKED_ON(REGISTER_MAX_STEPPING_RETRIES_LOCKED_ON);

    SET_INCREMENT_THRESHOLD_MULTIPLIER(REGISTER_INCREMENT_THRESHOLD_MULTIPLIER);

    //Initialize Defined Marks
    for ( x = 0; x < MAX_MARKS; x++ )
        camImagingPtr->markDefinition[x].enable = FALSE;

    //Set Reference
    camImagingPtr->refIndex = 1;

    //Define Element(s)  only a circle in this case
    //Define Single Element
    camImagingPtr->elements[0].minAreaSize = CIRCLE_MIN_AREA_03;
    camImagingPtr->elements[0].nominalAreaSize = CIRCLE_NOM_AREA_03;
    camImagingPtr->elements[0].maxAreaSize = CIRCLE_MAX_AREA_03;
    camImagingPtr->elements[0].minWidth = CIRCLE_MIN_WIDTH_03;
    camImagingPtr->elements[0].minHeight = CIRCLE_MIN_HEIGHT_03;
    camImagingPtr->elements[0].maxWidth = CIRCLE_MAX_WIDTH_03;
    camImagingPtr->elements[0].maxHeight = CIRCLE_MAX_HEIGHT_03;
    camImagingPtr->elements[0].widthHeightPercentRatioTolerance =
    CIRCLE_W_H_PERCENT_RATIO_TOLERANCE;
    camImagingPtr->elements[0].perimeterAreaPercentRatioTolerance =
    CIRCLE_P_A_PERCENT_RATIO_TOLERANCE;
    camImagingPtr->elements[0].minComplexAxis = 0;

    //Define Complex Element
    camImagingPtr->elements[1].minAreaSize = CIRCLE_NOM_AREA_03;
    camImagingPtr->elements[1].nominalAreaSize = CIRCLE_MAX_AREA_03;
    camImagingPtr->elements[1].maxAreaSize = CIRCLE_MAX_AREA_03 * ACTIVE_MARKS; //Single * Active Elments
    camImagingPtr->elements[1].minWidth = CIRCLE_NOM_WIDTH_03;
    camImagingPtr->elements[1].minHeight = CIRCLE_NOM_HEIGHT_03;
    camImagingPtr->elements[1].maxWidth = CIRCLE_MAX_WIDTH_03 * ACTIVE_MARKS;
    camImagingPtr->elements[1].maxHeight = CIRCLE_MAX_HEIGHT_03 * ACTIVE_MARKS;
    camImagingPtr->elements[1].widthHeightPercentRatioTolerance = -1;  //Not Used
    camImagingPtr->elements[1].perimeterAreaPercentRatioTolerance = -1;//Not Used
    camImagingPtr->elements[1].minComplexAxis = COMPLEX_MIN_AXIS;

    //********************************Mark 1 Definition***********************************//
    markDefIndex = 0;

    camImagingPtr->markDefinition[ markDefIndex ].enable = TRUE;
    camImagingPtr->markDefinition[ markDefIndex ].numPairsToSearch = 1;

    //Define element Pair Parmeters
    elemPair.thisElementType                = SINGLE;  //This is the index to element difinition
    elemPair.partnerElementType             = SINGLE;

    elemPair.nominalHorizontalDistToPartner = 69;
    elemPair.nominalVerticalDistToPartner = -23;

    elemPair.perimeterToleranceCenterOfPartner = PERIMITER_TOL_03;
    camImagingPtr->markDefinition[ markDefIndex ].elementPairs[0] = elemPair;

    //********************************Mark 2 Definition***********************************//
    markDefIndex = 1;

    camImagingPtr->markDefinition[ markDefIndex ].enable = TRUE;
    camImagingPtr->markDefinition[ markDefIndex ].numPairsToSearch = 1;

    //Define element Pair Parmeters
    elemPair.thisElementType                = SINGLE;  //This is the index to element difinition
    elemPair.partnerElementType             = SINGLE;

    elemPair.nominalHorizontalDistToPartner = 38;
    elemPair.nominalVerticalDistToPartner = -23;

    elemPair.perimeterToleranceCenterOfPartner = PERIMITER_TOL_03;
    camImagingPtr->markDefinition[ markDefIndex ].elementPairs[0] = elemPair;

    //********************************Mark 3 Definition***********************************//
    markDefIndex = 2;

    camImagingPtr->markDefinition[ markDefIndex ].enable = TRUE;
    camImagingPtr->markDefinition[ markDefIndex ].numPairsToSearch = 1;

    //Define element Pair Parmeters
    elemPair.thisElementType                = SINGLE;  //This is the index to element difinition
    elemPair.partnerElementType             = SINGLE;

    elemPair.nominalHorizontalDistToPartner = -38;
    elemPair.nominalVerticalDistToPartner = -23;

    elemPair.perimeterToleranceCenterOfPartner = PERIMITER_TOL_03;
    camImagingPtr->markDefinition[ markDefIndex ].elementPairs[0] = elemPair;

    //********************************Mark 4 Definition***********************************//
    markDefIndex = 3;

    camImagingPtr->markDefinition[ markDefIndex ].enable = TRUE;
    camImagingPtr->markDefinition[ markDefIndex ].numPairsToSearch = 1;

    //Define element Pair Parmeters
    elemPair.thisElementType                = SINGLE;  //This is the index to element difinition
    elemPair.partnerElementType             = SINGLE;

    elemPair.nominalHorizontalDistToPartner = -69;
    elemPair.nominalVerticalDistToPartner = -23;

    elemPair.perimeterToleranceCenterOfPartner = PERIMITER_TOL_03;
    camImagingPtr->markDefinition[ markDefIndex ].elementPairs[0] = elemPair;

    //********************************Mark 5 Definition***********************************//
    markDefIndex = 4;

    camImagingPtr->markDefinition[ markDefIndex ].enable = TRUE;

    camImagingPtr->markDefinition[ markDefIndex ].numPairsToSearch = 1;

    //Define element Pair Parmeters
    elemPair.thisElementType                = SINGLE;  //This is the index to element difinition
    elemPair.partnerElementType             = SINGLE;
    elemPair.nominalHorizontalDistToPartner = 100;

    elemPair.nominalVerticalDistToPartner = 0;
    elemPair.perimeterToleranceCenterOfPartner = PERIMITER_TOL_03;
    camImagingPtr->markDefinition[ markDefIndex ].elementPairs[0] = elemPair;

}   

//
//
//

#define DEBUG_IS_CIRCLE     (FALSE)
#define DEBUG_IS_OVAL       (FALSE)

int     GetNumMarksEnable(void)
{
    int numMarksEnabled = 0;

    int markNumber;

    MARK_DEFINITION currentMark;

    CameraImaging   *camImagingPtr  = 
    &GetCurrSystemConfig()->cameraImaging;

    for ( markNumber = 0; markNumber < MAX_MARKS; markNumber++ )
    {
        currentMark = camImagingPtr->markDefinition[markNumber];

        if ( currentMark.enable )
        {
            numMarksEnabled++;
        }
    }

    return numMarksEnabled;
}

int     GetNumRegMarksFound(void)
{
    int markNumber;

    CameraImaging   *camImagingPtr  = 
    &GetCurrSystemConfig()->cameraImaging;

    int found = 0;

    for ( markNumber = 0; markNumber < MAX_MARKS; markNumber++ )
    {
        // DO IF FOR EACH

        if ( IsRegMarkFound(markNumber) )
        {
            found++;
        }
    }

    return(found);
}

BOOL    IsRegMarkFound(int markNumber)
{
    MARK_DEFINITION currentMark;
    MARK_STATUS     currentMarkSearchStatus;

    CameraImaging   *camImagingPtr  = 
    &GetCurrSystemConfig()->cameraImaging;

    currentMark = camImagingPtr->markDefinition[markNumber];
    currentMarkSearchStatus = markRecognitionData[ markNumber ].markSearchStatus;

    // DO IF FOR EACH

    if ( (currentMark.enable) &&
         ((currentMarkSearchStatus == MARK_FOUND_SINGLE_TO_SINGLE) || 
          (currentMarkSearchStatus == MARK_FOUND_SINGLE_TO_COMPLEX) ||
          (currentMarkSearchStatus == MARK_FOUND_SINGLE_TO_COMPOUND) ||
          (currentMarkSearchStatus == MARK_FOUND_COMPOUND_TO_SINGLE) ||
          (currentMarkSearchStatus == MARK_FOUND_COMPOUND_TO_COMPLEX) ||
          (currentMarkSearchStatus == MARK_FOUND_COMPOUND_TO_COMPOUND) ||
          (currentMarkSearchStatus == MARK_FOUND_VIRTUAL_TO_COMPLEX_REF) ||
          (currentMarkSearchStatus == MARK_FOUND_COMPLEX_TO_VIRTUAL_REF)) )
    {
        return TRUE;
    }

    return FALSE;
}

BOOL    AreAllRegMarksFound(void)
{
    int numMarksFound, numMarksEnabled;

    numMarksEnabled = GetNumMarksEnable(); 
    numMarksFound = GetNumRegMarksFound();

    if ( numMarksEnabled == numMarksFound )
        return TRUE;

    return FALSE;
}

BOOL    IsRegReferenceMarkFound(void)
{
    int markNumber;

    CameraImaging   *camImagingPtr  = 
    &GetCurrSystemConfig()->cameraImaging;

    markNumber = GetReferenceMarkIndex();

    if ( IsRegMarkFound(markNumber) )
        return TRUE;

    return FALSE;
}

BOOL    IsRegMinMarksFound(void)
{
    int  numberOfMarksFound = GetNumRegMarksFound();

    if ( IsRegReferenceMarkFound() )
    {
        if ( numberOfMarksFound >= MIN_MARK_FOUND )
        {
            return TRUE;
        }
    }

    return FALSE;
}



#define OPPOSITE_VECTOR     (TRUE)
#define NORMAL_VECTOR       (FALSE)

void DoFindSingleToComplex( VC_IMAGE *frameImage, MARK_DEFINITION markDef, 
                            VALID_OBJECT_FEATURES *objectList1, VALID_OBJECT_FEATURES *objectList2, int markNumber)
{
    POINT   thisCenterPoint;
    VECTOR  distToPartnerCenterPoint;
    int     perimeterTolerance;
    RECT    partnerSmallRect, partnerSmallRectOpposite, partnerBoundRect;

    OBJECT_FEATURES *obj1 = objectList1->objectFeatures;
    OBJECT_FEATURES *obj2 = objectList2->objectFeatures;

    int objCount1 = objectList1->objectFeaturesCount;
    int objCount2 = objectList2->objectFeaturesCount;
    int x,y,n;

    int minGreySmallRect1, minGreySmallRect2;
    int currThreshold;

    BOOL matchFoundPrimary = FALSE;
    BOOL matchFoundOpposite = FALSE;

    for ( x=0; x < objCount1; x++ )
    {
        thisCenterPoint.x = obj1->features.x_center;
        thisCenterPoint.y = obj1->features.y_center;

        perimeterTolerance = SINGLE_TO_COMPLEX_PERIMETER_TOLERANCE;

        // Note: Width and dist x y inversion for camera Vertical FOV

        distToPartnerCenterPoint = NormalizeRegElementVectors( &markDef.elementPairs[0] );

        partnerSmallRect         = GeneratePartnerRect(thisCenterPoint, 
                                                       distToPartnerCenterPoint, perimeterTolerance, 
                                                       NORMAL_VECTOR);

        partnerSmallRectOpposite = GeneratePartnerRect(thisCenterPoint, 
                                                       distToPartnerCenterPoint, perimeterTolerance, 
                                                       OPPOSITE_VECTOR);

        if ( !IsPerimeterRectValid(partnerSmallRect) )
        {
            // DEBUG what about the opposite validation?

            obj1++;
            continue;
        }

        for ( y=0; y < objCount2; y++ )
        {
            if ( obj1 != obj2 )
            {
                partnerBoundRect = GenerateFeatureRect(&obj2->features);

                // Validate Grey Level to make sure we are not projecting on white

                currThreshold = GetCurrentThreshold();

                n = markRecognitionData[ markNumber ].numMatchFound;

                matchFoundPrimary = IsRectInRectBound(&partnerSmallRect, &partnerBoundRect);
                matchFoundOpposite = IsRectInRectBound(&partnerSmallRectOpposite, &partnerBoundRect);

                if ( matchFoundPrimary )
                {
                    minGreySmallRect1 = GetRectMinLevel( &partnerSmallRect );

                    if ( minGreySmallRect1 > currThreshold )
                    {
                        matchFoundPrimary = FALSE;
                        //print("Grey Level FAILED\n");
                    }
                }
                else
                    if ( matchFoundOpposite )
                {
                    minGreySmallRect2 = GetRectMinLevel( &partnerSmallRectOpposite );

                    if ( minGreySmallRect2 > currThreshold )
                    {
                        matchFoundOpposite = FALSE; 
                        //print("Grey Level FAILED\n");
                    }
                }
                else
                {
                    // NOT FOUND
                }


                // printf("ObjCount2[%d]:x:%d y:%d\n",y, partnerCenterPoint.x,partnerCenterPoint.y);

                if ( matchFoundPrimary || matchFoundOpposite )
                {
                    //print("S-CPLX FOUND\n");

                    //FIll in arrays if both objects

                    // Increase Hit Count
                    obj1->hitCount++;
                    obj2->hitCount++;

                    // DrawBoxObject(frameImage, partnerObjectElem);

                    //if ( diagnosticsEnabled )
                    //    DrawLineBetweenOjbects(frameImage, &obj1->features, &obj2->features);

                    if ( n < MAX_VECTOR_MATCH )
                    {
                        markRecognitionData[ markNumber ].thisObject[n] = obj1;

                        if ( obj1->hitCount == SINGLE_HIT )
                            markRecognitionData[ markNumber ].thisObject[n]->isSingle = TRUE;
                        else
                        {
                            markRecognitionData[ markNumber ].thisObject[n]->isSingle = FALSE;
                            markRecognitionData[ markNumber ].thisObject[n]->isCompound = TRUE;
                        }

                        markRecognitionData[ markNumber ].partnerObject[n] = obj2;
                        markRecognitionData[ markNumber ].partnerObject[n]->isComplex = TRUE;

                        if ( matchFoundPrimary )
                        {
                            markRecognitionData[ markNumber ].isVectorOpposite[ n ] = FALSE;

                            //print("Match Found Primary MN:%d\n", markNumber);


                            //print("Grey Level1 %d:%d\n", minGreySmallRect1, currThreshold);

                            //DrawBoxRect(frameImage, &partnerSmallRect);

                        }
                        else
                            if ( matchFoundOpposite )
                        {
                            markRecognitionData[ markNumber ].isVectorOpposite[ n ] = TRUE;

                            //print("Match Found Opposite MN:%d\n", markNumber);


                            //print("Grey Level2 %d:%d\n", minGreySmallRect2, currThreshold);

                            //DrawBoxRect(frameImage, &partnerSmallRect);
                        }
                        else
                        {
                            // NOT FOUND
                        }

                    }
                    else
                    {
                        //print("S-CPLX FOUND TOO MANY: MN: %d\n", markNumber);

                        markRecognitionData[ markNumber ].markSearchStatus = MARK_FOUND_TOO_MANY;
                        break;
                    }

                    markRecognitionData[ markNumber ].numMatchFound++;

                    //   print("FOUND MARK[%d] :%d\n",markNumber , markRecognitionData[ markNumber ].numMatchFound);
                }
            }

            if ( markRecognitionData[ markNumber ].markSearchStatus == MARK_FOUND_TOO_MANY )
                break;

            obj2++;

        }// objCount 2

        obj2 = objectList2->objectFeatures;
        obj1++;

    }// for objCount 1

}


void DoFindSingleToSingleToCompounds( VC_IMAGE *frameImage, MARK_DEFINITION markDef, 
                                      VALID_OBJECT_FEATURES *objectList1, VALID_OBJECT_FEATURES *objectList2, int markNumber)
{
    POINT   thisCenterPoint, partnerCenterPoint;
    VECTOR  distToPartnerCenterPoint;
    int     perimeterTolerance;
    RECT    partnerSmallRect;

    OBJECT_FEATURES *obj1 = objectList1->objectFeatures;
    OBJECT_FEATURES *obj2 = objectList2->objectFeatures;

    int objCount1 = objectList1->objectFeaturesCount;
    int objCount2 = objectList2->objectFeaturesCount;
    int x,y,n;

    BOOL matchFound = FALSE;


    for ( x=0; x < objCount1; x++ )
    {
        thisCenterPoint.x = obj1->features.x_center;
        thisCenterPoint.y = obj1->features.y_center;

        //  printf("ObjCount[%d]:x:%d y:%d\n",x, thisCenterPoint.x,thisCenterPoint.y);


        perimeterTolerance = markDef.elementPairs[0].perimeterToleranceCenterOfPartner;


        // Note: Width and dist x y inversion for camera Vertical FOV

        distToPartnerCenterPoint = NormalizeRegElementVectors( &markDef.elementPairs[0] );


        partnerSmallRect         = GeneratePartnerRect(thisCenterPoint, distToPartnerCenterPoint, perimeterTolerance, FALSE);
//        partnerSmallRectInverted = GeneratePartnerRect(thisCenterPoint, distToPartnerCenterPoint, perimeterTolerance, TRUE);

        if ( !IsPerimeterRectValid(partnerSmallRect) )
        {
            obj1++;
            continue;
        }

        for ( y=0; y < objCount2; y++ )
        {
            if ( obj1 != obj2 )
            {
                partnerCenterPoint.x = obj2->features.x_center;
                partnerCenterPoint.y = obj2->features.y_center;

                matchFound = IsPointInRectBound(&partnerCenterPoint, &partnerSmallRect);


                // printf("ObjCount2[%d]:x:%d y:%d\n",y, partnerCenterPoint.x,partnerCenterPoint.y);

                if ( matchFound == TRUE )
                {
                    //print("FOUND\n");

                    //FIll in arrays if both objects

                    // Increase Hit Count
                    obj1->hitCount++;
                    obj2->hitCount++;

                    // DrawBoxObject(frameImage, partnerObjectElem);

                    //if ( diagnosticsEnabled )
                    //    DrawLineBetweenOjbects(frameImage, &obj1->features, &obj2->features);

                    n = markRecognitionData[ markNumber ].numMatchFound;

                    if ( n < MAX_VECTOR_MATCH )
                    {
                        markRecognitionData[ markNumber ].thisObject[n] = obj1;

                        if ( obj1->hitCount == SINGLE_HIT )
                            markRecognitionData[ markNumber ].thisObject[n]->isSingle = TRUE;
                        else
                        {
                            markRecognitionData[ markNumber ].thisObject[n]->isSingle = FALSE;
                            markRecognitionData[ markNumber ].thisObject[n]->isCompound = TRUE;
                        }


                        markRecognitionData[ markNumber ].partnerObject[n] = obj2;

                        if ( obj2->hitCount == SINGLE_HIT )
                            markRecognitionData[ markNumber ].partnerObject[n]->isSingle = TRUE;
                        else
                        {
                            markRecognitionData[ markNumber ].partnerObject[n]->isSingle = FALSE;
                            markRecognitionData[ markNumber ].partnerObject[n]->isCompound = TRUE;
                        }
                    }
                    else
                    {
                        //print("S-S FOUND TOO MANY: MN: %d\n", markNumber);

                        markRecognitionData[ markNumber ].markSearchStatus = MARK_FOUND_TOO_MANY;
                        break;
                    }

                    markRecognitionData[ markNumber ].numMatchFound++;

                    //   print("FOUND MARK[%d] :%d\n",markNumber , markRecognitionData[ markNumber ].numMatchFound);
                }
            }

            if ( markRecognitionData[ markNumber ].markSearchStatus == MARK_FOUND_TOO_MANY )
                break;

            obj2++;

        }// objCount 2

        obj2 = objectList2->objectFeatures;
        obj1++;

    }// for objCount 1
}

//
//
//


void DoFindVirtualComplex( VC_IMAGE *frameImage, MARK_DEFINITION markDef, 
                           VALID_VIRTUAL_OBJECT_FEATURES *objectList1, VALID_OBJECT_FEATURES *objectList2, int markNumber)
{
    POINT   thisCenterPoint;

    VECTOR  distToPartnerCenterPoint;

    int     perimeterTolerance;

    RECT    partnerSmallRect, partnerSmallRectOpposite;
    RECT    partnerBoundRect;

    OBJECT_FEATURES *obj1 = objectList1->objectFeatures;
    OBJECT_FEATURES *obj2 = objectList2->objectFeatures;

    int objCount1 = objectList1->objectFeaturesCount;
    int objCount2 = objectList2->objectFeaturesCount;
    int x,y,n;

    BOOL matchFoundPrimary = FALSE;
    BOOL matchFoundOpposite = FALSE;

    CameraImaging   *camImagingPtr  = 
    &GetCurrSystemConfig()->cameraImaging;

    for ( x=0; x < objCount1; x++ )
    {
        thisCenterPoint.x = obj1->features.x_center;
        thisCenterPoint.y = obj1->features.y_center;

        perimeterTolerance = SINGLE_TO_COMPLEX_PERIMETER_TOLERANCE;

        // Note: Width and dist x y inversion for camera Vertical FOV

        distToPartnerCenterPoint = NormalizeRegElementVectors( &markDef.elementPairs[0] );


        partnerSmallRect         = GeneratePartnerRect(thisCenterPoint, 
                                                       distToPartnerCenterPoint, perimeterTolerance, NORMAL_VECTOR);

        partnerSmallRectOpposite = GeneratePartnerRect(thisCenterPoint, 
                                                       distToPartnerCenterPoint, perimeterTolerance, OPPOSITE_VECTOR);

        if ( !IsPerimeterRectValid(partnerSmallRect) )
        {
            // DEBUG what about the opposite validation?

            obj1++;
            continue;
        }


        for ( y=0; y < objCount2; y++ )
        {
            if ( obj1 != obj2 )
            {
                partnerBoundRect = GenerateFeatureRect(&obj2->features);

                n = markRecognitionData[ markNumber ].numMatchFound;

                matchFoundPrimary = IsRectIntersectingRectBound(&partnerSmallRect, &partnerBoundRect);
                matchFoundOpposite = IsRectIntersectingRectBound(&partnerSmallRectOpposite, &partnerBoundRect);

                // printf("ObjCount2[%d]:x:%d y:%d\n",y, partnerCenterPoint.x,partnerCenterPoint.y);

                if ( matchFoundPrimary || matchFoundOpposite )
                {
                    //FIll in arrays if both objects

                    // Increase Hit Count
                    obj1->hitCount++;
                    obj2->hitCount++;

                    // DrawBoxObject(frameImage, partnerObjectElem);

                    //if ( diagnosticsEnabled )
                    //    DrawLineBetweenOjbects(frameImage, &obj1->features, &obj2->features);

                    if ( n < MAX_VECTOR_MATCH )
                    {
                        markRecognitionData[ markNumber ].thisObject[n] = obj1;
                        markRecognitionData[ markNumber ].thisObject[n]->isSingleVirtual = TRUE;


                        markRecognitionData[ markNumber ].partnerObject[n] = obj2;
                        markRecognitionData[ markNumber ].partnerObject[n]->isComplex = TRUE;

                        if ( matchFoundPrimary )
                        {
                            markRecognitionData[ markNumber ].isVectorOpposite[ n ] = FALSE;
                            //print("V-C FOUND Primary\n");
                        }
                        else
                            if ( matchFoundOpposite )
                        {
                            markRecognitionData[ markNumber ].isVectorOpposite[ n ] = TRUE;
                            //print("V-C FOUND Opposite\n");
                        }
                    }
                    else
                    {
                        //print("V-CPLX FOUND TOO MANY: MN: %d Matches: %d\n", markNumber, n);

                        markRecognitionData[ markNumber ].markSearchStatus = MARK_FOUND_TOO_MANY;
                        break;
                    }

                    markRecognitionData[ markNumber ].numMatchFound++;

                    //   print("FOUND MARK[%d] :%d\n",markNumber , markRecognitionData[ markNumber ].numMatchFound);
                }
            }

            if ( markRecognitionData[ markNumber ].markSearchStatus == MARK_FOUND_TOO_MANY )
                break;

            obj2++;

        }// objCount 2

        obj2 = objectList2->objectFeatures;
        obj1++;

    }// for objCount 1

}

// 
// 
// 
//


void DoFindVirtualRefMark( VC_IMAGE *frameImage, VALID_OBJECT_FEATURES *objectList1)
{
    int refMarkNumber = GetReferenceMarkIndex();

    DoFindVirtualMarks(frameImage, objectList1, refMarkNumber);
}


void DoFindVirtualMarks( VC_IMAGE *frameImage, VALID_OBJECT_FEATURES *objectList1, int markNumber)
{
    int v;

    MARK_DEFINITION currentMark;

    int objCount1 = objectList1->objectFeaturesCount;

    OBJECT_FEATURES *obj1 = objectList1->objectFeatures;

    int vFeaturesCount = 0;

    CameraImaging   *camImagingPtr = &GetCurrSystemConfig()->cameraImaging;

    //print("CPLX COUNT:%d\n", objCount1);

    for ( v=0; v < objCount1; v++ )
    {

        virtualSingleObjects.objectFeatures[vFeaturesCount].features.x_center = 
        virtualSingleObjects.objectFeatures[vFeaturesCount].features.x_min =
        virtualSingleObjects.objectFeatures[vFeaturesCount].features.x_max = obj1->features.x_min;

        virtualSingleObjects.objectFeatures[vFeaturesCount].features.y_center = 
        virtualSingleObjects.objectFeatures[vFeaturesCount].features.y_min =
        virtualSingleObjects.objectFeatures[vFeaturesCount].features.y_max = obj1->features.y_min;
        vFeaturesCount++;

        //

        virtualSingleObjects.objectFeatures[vFeaturesCount].features.x_center =
        virtualSingleObjects.objectFeatures[vFeaturesCount].features.x_min = 
        virtualSingleObjects.objectFeatures[vFeaturesCount].features.x_max = obj1->features.x_min;

        virtualSingleObjects.objectFeatures[vFeaturesCount].features.y_center =
        virtualSingleObjects.objectFeatures[vFeaturesCount].features.y_min =
        virtualSingleObjects.objectFeatures[vFeaturesCount].features.y_max = obj1->features.y_max;
        vFeaturesCount++;

        //

        virtualSingleObjects.objectFeatures[vFeaturesCount].features.x_center =
        virtualSingleObjects.objectFeatures[vFeaturesCount].features.x_min = 
        virtualSingleObjects.objectFeatures[vFeaturesCount].features.x_max = obj1->features.x_max;

        virtualSingleObjects.objectFeatures[vFeaturesCount].features.y_center =
        virtualSingleObjects.objectFeatures[vFeaturesCount].features.y_min =
        virtualSingleObjects.objectFeatures[vFeaturesCount].features.y_max = obj1->features.y_min;
        vFeaturesCount++;

        //

        virtualSingleObjects.objectFeatures[vFeaturesCount].features.x_center = 
        virtualSingleObjects.objectFeatures[vFeaturesCount].features.x_min = 
        virtualSingleObjects.objectFeatures[vFeaturesCount].features.x_max = obj1->features.x_max;

        virtualSingleObjects.objectFeatures[vFeaturesCount].features.y_center =
        virtualSingleObjects.objectFeatures[vFeaturesCount].features.y_min =
        virtualSingleObjects.objectFeatures[vFeaturesCount].features.y_max = obj1->features.y_max;
        vFeaturesCount++;

        virtualSingleObjects.objectFeaturesCount += VIRTUAL_OBJECT_MULTIPLIER;
    }

    //print("S-VIRTUAL-CPLX FIND:%d SVO:%d\n", refMarkNumber, virtualSingleObjects.objectFeaturesCount );

    currentMark = camImagingPtr->markDefinition[markNumber];

    DoFindVirtualComplex(frameImage, currentMark,
                         &virtualSingleObjects,
                         &validObjectFeatures[ COMPLEX_FEATURES ], markNumber);
}



#define     MIN_MARK_FOUND_FOR_VIRT_REF_SEARCH      (1)

void    DoRegMarkRecognition(VC_IMAGE *frameImage)
{
    int markNumber;

    MARK_DEFINITION currentMark;

    CameraImaging   *camImagingPtr  = 
    &GetCurrSystemConfig()->cameraImaging;

    // TODO RESET, Mark Search status

    // Search through all the first object element type
    // For matches and possible duplicates


    if ( validObjectFeatures[ SINGLE_FEATURES ].objectFeaturesCount > 0 )
    {
        // FIRST - MATCH SINGLE TO COMPLEX

        if ( validObjectFeatures[ COMPLEX_FEATURES ].objectFeaturesCount > 0 )
        {
            for ( markNumber = 0; markNumber < MAX_MARKS; markNumber++ )
            {
                currentMark = camImagingPtr->markDefinition[markNumber];

                if ( currentMark.enable &&
                     markRecognitionData[ markNumber ].markSearchStatus == MARK_NOT_FOUND )
                {
                    markRecognitionData[markNumber].numMatchFound = 0;

                    //print("S-CPLX FIND:%d\n", markNumber);

                    DoFindSingleToComplex( frameImage, currentMark, 
                                           &validObjectFeatures[ SINGLE_FEATURES ],
                                           &validObjectFeatures[ COMPLEX_FEATURES ], markNumber);
                }
            }
        }


        // SECOND - MATCH SINGLE TO SINGLE

        for ( markNumber = 0; markNumber < MAX_MARKS; markNumber++ )
        {
            currentMark = camImagingPtr->markDefinition[markNumber];

            if ( currentMark.enable &&
                 markRecognitionData[ markNumber ].markSearchStatus == MARK_NOT_FOUND )
            {
                //print("S-S FIND:%d\n", markNumber);

                DoFindSingleToSingleToCompounds( frameImage, currentMark, 
                                                 &validObjectFeatures[ SINGLE_FEATURES ],
                                                 &validObjectFeatures[ SINGLE_FEATURES ], markNumber);
            }
        }

        DoRegSearchAnalysis();

        //

        //    if ( AreAllRegMarksFound() )
        //    {
        //        print("ALL MARKS FOUND DONE SINGLE TO COMPLEX\n");
        //    }

        //ShowValidObjectArray();
        //ShowMarkValidationData();



        if ( IsRegReferenceMarkFound() )
        {
            ; //print("REFERENCE FOUND\n");
        }
        else
        {
            if ( GetNumRegMarksFound() >= MIN_MARK_FOUND_FOR_VIRT_REF_SEARCH )
            {
                //  print("VIRTUAL FIND REFERENCE\n");

                DoFindVirtualRefMark(frameImage, &validObjectFeatures[ COMPLEX_FEATURES ]);

                DoRegSearchAnalysis();

                // if ( IsRegReferenceMarkFound() )
                // {
                //     print("VIRTUAL REFERENCE IS FOUND\n");
                // }
                // else
                // {
                //     print("REFERENCE IS STILL MISSING\n");
                // }
            }
            // else
            // {
            //     print("NOT ENOUGH MAKRS TO FIND MISSING REFERENCE\n");
            // }
        }


        if ( AreAllRegMarksFound() )
        {
            ;// print("ALL MARKS FOUND DONE\n");
        }
        else if ( IsRegReferenceMarkFound() )
        {
            for ( markNumber = 0; markNumber < MAX_MARKS; markNumber++ )
            {
                currentMark = camImagingPtr->markDefinition[markNumber];

                if ( currentMark.enable &&
                     markRecognitionData[ markNumber ].markSearchStatus == MARK_NOT_FOUND )
                {
                    //    print("VIRTUAL MARK FIND:%d\n", markNumber);

                    DoFindVirtualMarks( frameImage, &validObjectFeatures[ COMPLEX_FEATURES ], markNumber); 

                    DoRegSearchAnalysis();

                    //  if ( markRecognitionData[ markNumber ].markSearchStatus != MARK_NOT_FOUND )
                    //      print("VIRTUAL MARK IS FOUND:%d\n", markNumber);
                    //  else
                    //      print("VIRTUAL MARK IS STILL MISSING:%d\n", markNumber);
                }
            }
        }

        // print("NUM MARKS FOUND %d\n", GetNumRegMarksFound());
    }
}


void    DoRegSearchAnalysis(void)
{
    int markNumber, n, m, perimeterTolerance;

    POINT   thisCenterPoint;

    RECT    partnerSmallRect;

    VECTOR  distToPartnerCenterPoint;

    MARK_DEFINITION currentMark;

    MARK_STATUS currMarkStatus, newMarkStatus;

    BOOL    checkForPromotion;


    CameraImaging   *camImagingPtr  =
    &GetCurrSystemConfig()->cameraImaging;

    for ( markNumber = 0; markNumber < MAX_MARKS; markNumber++ )
    {
        currentMark = camImagingPtr->markDefinition[markNumber];

        if ( (currentMark.enable) &&
             (markRecognitionData[ markNumber ].markSearchStatus == MARK_NOT_FOUND) )
        {
            n = markRecognitionData[ markNumber ].numMatchFound;

            //print("MN:%d NMF:%d\n", markNumber, n);

            if ( markRecognitionData[ markNumber ].markSearchStatus != MARK_FOUND_TOO_MANY )
            {
                for ( m=0 ; m<n; m++ )
                {
                    currMarkStatus = markRecognitionData[ markNumber ].markSearchStatus;
                    newMarkStatus = MARK_NOT_FOUND;

                    checkForPromotion = TRUE;

                    if ( (markRecognitionData[ markNumber ].thisObject[m]->isSingle) &&
                         (markRecognitionData[ markNumber ].partnerObject[m]->isSingle) )
                    {
                        if ( currMarkStatus == MARK_FOUND_SINGLE_TO_SINGLE )
                        {
                            markRecognitionData[ markNumber ].markSearchStatus = MARK_FOUND_DUPLICATES;
                            //print("S-S DUPLICATES\n");
                        }
                        else
                        {
                            newMarkStatus = MARK_FOUND_SINGLE_TO_SINGLE;
                            // print("S-S FOUND MN:%d MO:%d\n", markNumber, m);
                        }
                    }
                    else
                        if ( (markRecognitionData[ markNumber ].thisObject[m]->isSingle) &&
                             (markRecognitionData[ markNumber ].partnerObject[m]->isComplex) )
                    {
                        if ( currMarkStatus == MARK_FOUND_SINGLE_TO_COMPLEX )
                        {
                            markRecognitionData[ markNumber ].markSearchStatus = MARK_FOUND_DUPLICATES;
                            //print("S-CPLX DUPLICATES\n");
                        }
                        else
                        {
                            newMarkStatus = MARK_FOUND_SINGLE_TO_COMPLEX;
                            //print("S-CPLX MN:%d MO:%d\n", markNumber, m);
                        }
                    }
                    else
                        if ( (markRecognitionData[ markNumber ].thisObject[m]->isSingle) &&
                             (markRecognitionData[ markNumber ].partnerObject[m]->isCompound) )
                    {
                        if ( currMarkStatus == MARK_FOUND_SINGLE_TO_COMPOUND )
                        {
                            markRecognitionData[ markNumber ].markSearchStatus = MARK_FOUND_DUPLICATES;
                            //print("S-CMPND DUPLICATES\n");
                        }
                        else
                        {
                            newMarkStatus = MARK_FOUND_SINGLE_TO_COMPOUND;
                            //print("S-CMPND MN:%d MO:%d\n", markNumber, m);
                        }
                    }
                    else
                        if ( (markRecognitionData[ markNumber ].thisObject[m]->isCompound) &&
                             (markRecognitionData[ markNumber ].partnerObject[m]->isSingle) )
                    {
                        if ( currMarkStatus == MARK_FOUND_COMPOUND_TO_SINGLE )
                        {
                            markRecognitionData[ markNumber ].markSearchStatus = MARK_FOUND_DUPLICATES;
                            //print("CMPND-S DUPLICATES\n");
                        }
                        else
                        {
                            newMarkStatus = MARK_FOUND_SINGLE_TO_COMPOUND;
                            //print("CMPND-S MN:%d MO:%d\n", markNumber, m);
                        }
                    }
                    else
                        if ( (markRecognitionData[ markNumber ].thisObject[m]->isCompound) &&
                             (markRecognitionData[ markNumber ].partnerObject[m]->isComplex) )
                    {
                        if ( currMarkStatus == MARK_FOUND_COMPOUND_TO_COMPLEX )
                        {
                            markRecognitionData[ markNumber ].markSearchStatus = MARK_FOUND_DUPLICATES;
                            //print("CMPND-CMPLX DUPLICATES\n");
                        }
                        else
                        {
                            newMarkStatus = MARK_FOUND_COMPOUND_TO_COMPLEX;
                            //print("CMPND-CMPLX MN:%d MO:%d\n", markNumber, m);
                        }
                    }
                    else
                        if ( (markRecognitionData[ markNumber ].thisObject[m]->isCompound) &&
                             (markRecognitionData[ markNumber ].partnerObject[m]->isCompound) )
                    {
                        if ( currMarkStatus == MARK_FOUND_COMPOUND_TO_COMPOUND )
                        {
                            markRecognitionData[ markNumber ].markSearchStatus = MARK_FOUND_DUPLICATES;
                            //print("CMPND-CMPND DUPLICATES\n");
                        }
                        else
                        {
                            newMarkStatus = MARK_FOUND_COMPOUND_TO_COMPOUND;
                            //print("CMPND-CMPND MN:%d MO:%d\n", markNumber, m);
                        }
                    }
                    else
                        if ( (markRecognitionData[ markNumber ].thisObject[m]->isSingleVirtual) &&
                             (markRecognitionData[ markNumber ].partnerObject[m]->isComplex) )
                    {
                        if ( currMarkStatus == MARK_FOUND_VIRTUAL_TO_COMPLEX_REF )
                        {
                            //print("VIRT-CPLX-REF DUPLICATES - OK \n");
                        }
                        else
                        {
                            newMarkStatus = MARK_FOUND_VIRTUAL_TO_COMPLEX_REF;
                            //print("VIRT-CPLX-REF MN:%d MO:%d\n", markNumber, m);
                        }
                    }
                    else
                        if ( (markRecognitionData[ markNumber ].thisObject[m]->isComplex) &&
                             (markRecognitionData[ markNumber ].partnerObject[m]->isSingleVirtual) )
                    {
                        if ( currMarkStatus == MARK_FOUND_COMPLEX_TO_VIRTUAL_REF )
                        {
                            //print("CPLX-VIRT_REF DUPLICATES - OK \n");
                        }
                        else
                        {
                            newMarkStatus = MARK_FOUND_VIRTUAL_TO_COMPLEX_REF;
                            //print("CPLX-VIRT_REF MN:%d MO:%d\n", markNumber, m);
                        }
                    }
                    else
                    {
                        // print("MISSED MN:%d MO:%d\n", markNumber, m);
                        checkForPromotion = FALSE;
                    }

                    if ( checkForPromotion == TRUE )
                    {
                        if ( markRecognitionData[ markNumber ].markSearchStatus == MARK_FOUND_DUPLICATES )
                        {
                            break;
                        }
                        else
                        {
                            // MarkSearchStatus Promotion

                            if ( newMarkStatus < currMarkStatus )
                            {
                                // PROMOTION LOGIC

                                //print("PROMOTE NMS:%d CMS:%d ", newMarkStatus , currMarkStatus);
                                //print("MN:%d MO:%d\n", markNumber, m);

                                markRecognitionData[ markNumber ].markSearchStatus = newMarkStatus;

                                markRecognitionData[ markNumber ].firstObject = 
                                (*markRecognitionData[ markNumber ].thisObject[m]);
                                markRecognitionData[ markNumber ].lastObject = 
                                (*markRecognitionData[ markNumber ].partnerObject[m]);

                                finalThresh = markRecognitionData[ markNumber ].threshold = GetCurrentThreshold();

                                // HANDLE SINGLE TO COMPLEX MID POINT ADJUSTMENT

                                if ( markRecognitionData[ markNumber ].lastObject.isComplex == TRUE )
                                {
                                    //   print("LAST OBJ COMPLEX\n");

                                    thisCenterPoint.x = markRecognitionData[ markNumber ].firstObject.features.x_center;
                                    thisCenterPoint.y = markRecognitionData[ markNumber ].firstObject.features.y_center;

                                    perimeterTolerance = currentMark.elementPairs[0].perimeterToleranceCenterOfPartner;

                                    distToPartnerCenterPoint = NormalizeRegElementVectors( &currentMark.elementPairs[0] );

                                    if ( markRecognitionData[ markNumber ].isVectorOpposite[ m ] == TRUE )
                                    {
                                        partnerSmallRect         = 
                                        GeneratePartnerRect(thisCenterPoint, distToPartnerCenterPoint, 
                                                            perimeterTolerance, OPPOSITE_VECTOR);

//                                         print("A partnerSmallRect[%d]\n", markNumber);
//                                         ShowPointAttributes(&thisCenterPoint);
//                                         ShowVectorAttributes(&distToPartnerCenterPoint);
//                                         ShowRectAttributes(&partnerSmallRect);
                                    }
                                    else
                                    {
                                        partnerSmallRect = 
                                        GeneratePartnerRect(thisCenterPoint, distToPartnerCenterPoint, 
                                                            perimeterTolerance, NORMAL_VECTOR);

//                                         print("B partnerSmallRect[%d]\n", markNumber);
//                                         ShowPointAttributes(&thisCenterPoint);
//                                         ShowVectorAttributes(&distToPartnerCenterPoint);
//                                         ShowRectAttributes(&partnerSmallRect);
                                    }

                                    AssignRectToFeature(&markRecognitionData[ markNumber ].lastObject.features, &partnerSmallRect);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

BOOL    DoRegMarkRecognitionResultAnalysis(VC_IMAGE *frameImage)
{
    BYTE markNumber;
    BYTE refIndex; 
    POINT centerPoint;

    CameraImaging   *camImagingPtr  = 
    &GetCurrSystemConfig()->cameraImaging;

    refIndex = GetReferenceMarkIndex();

    if ( (IsRegReferenceMarkFound() == TRUE ) && ( IsRegMinMarksFound() ) )
    {
        for ( markNumber = 0; markNumber < MAX_MARKS; markNumber++ )
        {
            if ( IsRegMarkFound(markNumber) )
            {
                centerPoint =
                GetCenterOfTwoObjects(frameImage, 
                                      &markRecognitionData[ markNumber ].firstObject.features, 
                                      &markRecognitionData[ markNumber ].lastObject.features);

                if ( markRecognitionData[ markNumber ].firstObject.isSingle  && (markRecognitionData[ markNumber ].firstObject.features.area > maxElementArea) )
                {
                    maxElementArea = markRecognitionData[ markNumber ].firstObject.features.area;
                }

                if ( markRecognitionData[ markNumber ].lastObject.isSingle  && (markRecognitionData[ markNumber ].lastObject.features.area > maxElementArea) )
                {
                    maxElementArea = markRecognitionData[ markNumber ].lastObject.features.area;
                }

                markRecognitionData[ markNumber ].markFovLateral = centerPoint.x + 
                                                                   (markRecognitionData[ refIndex ].markFovlateralStart * SUB_PIXEL_SCALE);

                markRecognitionData[ markNumber ].markFovCircum  = centerPoint.y + 
                                                                   (markRecognitionData[ refIndex ].markFovCircumStart * SUB_PIXEL_SCALE);

                if ( diagnosticsEnabled )
                {
                    DrawLineBetweenOjbects(frameImage, 
                                           &markRecognitionData[ markNumber ].firstObject.features, 
                                           &markRecognitionData[ markNumber ].lastObject.features);
                }

                //print("MARK POSITION[%d] x:%d  y:%d, cx:%d  cy:%d\n",markNumber, markRecognitionData[ markNumber ].markFovLateral
                //      ,  markRecognitionData[ markNumber ].markFovCircum,centerPoint.x,centerPoint.y );

                NormalizeToCameraCenterReg(frameImage, markNumber);

                //print("CENTER NORMALZIED MARK POSITION x:%d  y:%d\n",
                //      markRecognitionData[ markNumber ].markFovLateral,  markRecognitionData[ markNumber ].markFovCircum);
            }
        }
    }

    if ( AreAllRegMarksFound() )
        return TRUE;
    else
        return FALSE;
}

BOOL RLC_areaValidation( FRAME_BUFFER_IMAGE *currframeBuffImage, RECT whiteSpaceRect )
{
    POINT startingPix;
    POINT area;

    U16 *runlengthCodeAddr1;
    U16 *runlengthCodeAddr2;
    U32    areaSize;
    U32    maxAreaSize;

    U32    maxRlcLength    = 0x010000;

    int temp;
    image   RLC_Image;

    int marks, elementArea;

    CameraImaging   *camImagingPtr  = 
    &GetCurrSystemConfig()->cameraImaging;

    int quadrantOffsetX = curAOI.startX;
    int quadrantOffsetY = curAOI.startY;


    //First Set up the RLC area based on the whiteSpaceRect
    startingPix.x = whiteSpaceRect.top.x + quadrantOffsetX;
    startingPix.y = whiteSpaceRect.top.y + quadrantOffsetY;

    area.x = whiteSpaceRect.bottom.x - whiteSpaceRect.top.x;
    area.y = whiteSpaceRect.bottom.y - whiteSpaceRect.top.y;

    temp = startingPix.x + area.x;

    if ( temp > 639 ) //Make sure we do not over run
        area.x = area.x - ( temp - 639 );

    temp = startingPix.y + area.y;
    if ( temp > 479 ) //Make sure we do not over run
        area.y = area.y - ( temp - 479 );

    ImageAssign(&RLC_Image, 
                currframeBuffImage->frameImage.st + (startingPix.y * currframeBuffImage->frameImage.pitch) + startingPix.x, 
                area.x , area.y, currframeBuffImage->frameImage.pitch);   

    //Second Generate the RLC
    runlengthCodeAddr1 = rlcmalloc(maxRlcLength);

    if ( runlengthCodeAddr1 == NULL )
    {
        // print("RLC_areaValidation: DRAM Memory overrun\n");
        sysfree(runlengthCodeAddr1);
        return FALSE;
    }

    runlengthCodeAddr2 = 
    VC_CREATE_RLC(&RLC_Image, finalThresh, runlengthCodeAddr1, maxRlcLength);

    if ( runlengthCodeAddr2 == 0 )
    {
        //   LogString("RLC_areaValidation: RLC overrun"); 
        //VC_FREE_DRAM_WORD(runlengthCodeAddr1); 
        sysfree(runlengthCodeAddr1); 
        return FALSE;
    }

    areaSize = rlc_area( runlengthCodeAddr1, BLACK);

    //
    marks = GetNumMarksEnable(); 
    elementArea =  maxElementArea * 2; // 2 elements per mark
    maxAreaSize = ( marks * elementArea);

    //  print("RLC_areaValidation Area: %d MaxArea: %d\n", areaSize, maxAreaSize);

    sysfree(runlengthCodeAddr1);



    if ( areaSize > maxAreaSize )
    {
        //sprintf( logArray, "maxAreaSize: %d, currentArrea: %d Marks:%d", maxAreaSize, areaSize, marks);
        //LogString( logArray);

        return FALSE;
    }

    return TRUE;
}

BOOL    DoRegMarkValidations(FRAME_BUFFER_IMAGE *currframeBuffImage, VC_IMAGE *frameImage)
{
    RECT whiteSpaceRect;

    if ( IsRegReferenceMarkFound()  && IsRegMinMarksFound() )
    {
        whiteSpaceRect = GetWhiteSpaceRect( );

        if ( diagnosticsEnabled )
        {
            DrawBoxRect(frameImage, &whiteSpaceRect);
        }

        if ( RLC_areaValidation( currframeBuffImage, whiteSpaceRect ) )
            return TRUE;
    }
    return FALSE;
}

