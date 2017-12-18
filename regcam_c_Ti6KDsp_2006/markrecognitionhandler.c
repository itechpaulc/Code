




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

#include "markrecognitionhandler.h"

#include "cameraconfig.h"

#include "imageprocessinghandler.h"

#include "tcpcomm.h"


///////////////////////////////////////////////////////////////////
//
// Utility Function Definitions
//
///////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////
//
//
//
///////////////////////////////////////////////////////////////////

VALID_OBJECT_FEATURES           validObjectFeatures[ MAX_ELEMENTS ];

VALID_VIRTUAL_OBJECT_FEATURES   virtualSingleObjects;


//VALID_OBJECT_FEATURES       validCircleObjectFeatures;

//VALID_OBJECT_FEATURES       validOvalObjectFeatures;

MARK_RECOGNITION_DATA       markRecognitionData[ MAX_MARKS ];

extern FRAME_BUFFER_IMAGE      imageDiagBuffer;

extern BOOL     topEdgeFirst; //PM don't check in
extern BOOL     flipMark; //PM don't check in



// Area of the circle/oval is nominally 30% smaller than its perimeter

#define     CIRCLE_NOMINAL_P_A_DIFF         (0)

#define     OVAL_NOMINAL_P_A_DIFF           (0)

///////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////

MARK_RECOGNITION_DATA   *GetMarkRecognitionData( BYTE markIndex )
{
    return &markRecognitionData[ markIndex ];
}

///////////////////////////////////////////////////////////////////
//
//
//
///////////////////////////////////////////////////////////////////

void    InitializePatternRecognitionDefaults(void)
{
    int x;
    ELEMENT_PAIR_DEFINITION elemPair;

    CameraImaging   *camImagingPtr  = 
    &GetCurrSystemConfig()->cameraImaging;




    SET_WHITE_LEVEL_TARGET(RIBBON_WHITE_LEVEL_TARGET);
    SET_RANGE_SHIFT_DIVISOR(RIBBON_RANGE_SHIFT_DIVISOR);
    SET_START_THRESHOLD_MULTIPLIER(RIBBON_START_THRESHOLD_MULTIPLIER);
    SET_MAX_STEPPING_RETRIES_SEARCHING(RIBBON_MAX_STEPPING_RETRIES);
    SET_MAX_STEPPING_RETRIES_LOCKED_ON(RIBBON_MAX_STEPPING_RETRIES);
    SET_INCREMENT_THRESHOLD_MULTIPLIER(RIBBON_INCREMENT_THRESHOLD_MULTIPLIER);

    for ( x = 0; x < MAX_MARKS; x++ )
        camImagingPtr->markDefinition[x].enable = FALSE;

    camImagingPtr->refIndex = 0;

    camImagingPtr->elements[0].minAreaSize = CIRCLE_MIN_AREA;
    camImagingPtr->elements[0].nominalAreaSize = CIRCLE_NOM_AREA;
    camImagingPtr->elements[0].maxAreaSize = CIRCLE_MAX_AREA;
    camImagingPtr->elements[0].minWidth = CIRCLE_MIN_WIDTH;
    camImagingPtr->elements[0].minHeight = CIRCLE_MIN_HEIGHT;
    camImagingPtr->elements[0].maxWidth = CIRCLE_MAX_WIDTH;
    camImagingPtr->elements[0].maxHeight = CIRCLE_MAX_HEIGHT;
    camImagingPtr->elements[0].widthHeightPercentRatioTolerance =
    CIRCLE_W_H_PERCENT_RATIO_TOLERANCE;
    camImagingPtr->elements[0].perimeterAreaPercentRatioTolerance =
    CIRCLE_P_A_PERCENT_RATIO_TOLERANCE;

    camImagingPtr->elements[1].minAreaSize = OVAL_MIN_AREA;
    camImagingPtr->elements[1].nominalAreaSize = OVAL_NOM_AREA;
    camImagingPtr->elements[1].maxAreaSize = OVAL_MAX_AREA;
    camImagingPtr->elements[1].minWidth = OVAL_MIN_WIDTH;
    camImagingPtr->elements[1].minHeight = OVAL_MIN_HEIGHT;
    camImagingPtr->elements[1].maxWidth = OVAL_MAX_WIDTH;
    camImagingPtr->elements[1].maxHeight = OVAL_MAX_HEIGHT;
    camImagingPtr->elements[1].widthHeightPercentRatioTolerance = 
    OVAL_W_H_PERCENT_RATIO_TOLERANCE;
    camImagingPtr->elements[1].perimeterAreaPercentRatioTolerance =
    OVAL_P_A_PERCENT_RATIO_TOLERANCE;

    camImagingPtr->markDefinition[0].numPairsToSearch = TWO_PAIRS;

    camImagingPtr->markDefinition[0].enable = TRUE;

    elemPair.thisElementType                = 1;
    elemPair.partnerElementType             = 0;
    elemPair.nominalHorizontalDistToPartner = ELEM_DIST_BETWEEN_CIRC_AND_OVAL;

    elemPair.nominalVerticalDistToPartner = ELEM_PAIR_0_VERT_DIST_TO_PARTNER;
    elemPair.perimeterToleranceCenterOfPartner = ELEM_PAIR_0_TOLERANCE_CENTER_OF_PARTNER;
    camImagingPtr->markDefinition[0].elementPairs[0] = elemPair;


    elemPair.thisElementType                = 0;
    elemPair.partnerElementType             = 0;
    elemPair.nominalHorizontalDistToPartner = ELEM_DIST_BETWEEN_CIRC_AND_CIRC;

    elemPair.nominalVerticalDistToPartner = ELEM_PAIR_1_VERT_DIST_TO_PARTNER;
    elemPair.perimeterToleranceCenterOfPartner = ELEM_PAIR_1_TOLERANCE_CENTER_OF_PARTNER;
    camImagingPtr->markDefinition[0].elementPairs[1] = elemPair;

    camImagingPtr->markDefinition[0].elementPairs[2].thisElementType = ELEM_PAIR_2_TYPE;

    SetReferenceMarkIndex( 0 );
}   

//
//
//

#define DEBUG_IS_CIRCLE     (TRUE)
#define DEBUG_IS_OVAL       (FALSE)


BOOL    IsValidElement(VC_FEATURES *objFeature, MARK_ELEMENT *elementDef, MARK_ELEMENT_TYPE elementType)
{
    int objectWidth, objectHeight, minAxis;
    int objectWidthHeightDiff, objectWidthHeightRatio,tgtWidthHeightRatio;
    int objectPerimeterArea, objectPerimeterAreaDiff, currentDensityAreaRatio;
    int minAreaDensityRatio;
    int objectPerimeterAreaTolerance, objectWidthHeightRatioTolerance;

//     CameraImaging   *camImagingPtr  =
//     &GetCurrSystemConfig()->cameraImaging;
//    if ( DEBUG_IS_CIRCLE )
//        print("W-Height OK..\n  ");
//
//    if ( DEBUG_IS_CIRCLE )
//print("TEST NomArea[%d]:%d elemType:%d \n",elementDef->nominalAreaSize, objFeature->area, elementType );

    if ( (objFeature->area <= elementDef->maxAreaSize) &&
         (objFeature->area >= elementDef->minAreaSize) )
    {
        objectWidth     = objFeature->x_max - objFeature->x_min;
        objectHeight    = objFeature->y_max - objFeature->y_min;

        objectPerimeterArea = objectWidth * objectHeight;

       //if ( DEBUG_IS_CIRCLE )
       //print("Area OK..\n ");
       //print(" H:%d W:%d A:%d.\n ", objectHeight, objectWidth,objFeature->area);

        if ( (objectWidth <= elementDef->maxWidth) &&
             (objectWidth >= elementDef->minWidth) &&
             (objectHeight <= elementDef->maxHeight) &&
             (objectHeight >= elementDef->minHeight) )
        {
              //if ( DEBUG_IS_CIRCLE )
              //    print("W-Height OK..\n  ");

            // perimeter square-ness check

            tgtWidthHeightRatio =   (elementDef->minWidth*100)/elementDef->minHeight;
            objectWidthHeightRatio = ((objectWidth * 100) / objectHeight);

            objectWidthHeightDiff = abs(tgtWidthHeightRatio - objectWidthHeightRatio);

            if ( elementType == COMPLEX )
            {
                minAxis = MIN(objectWidth,objectHeight);
                minAreaDensityRatio = (elementDef->minComplexAxis * 100) / minAxis;

                currentDensityAreaRatio =  ((long int)( objFeature->area * 100)) / objectPerimeterArea;

                objectWidthHeightRatioTolerance = 0;

                //print("CDAR:%d MADR:%d H:%d W:%d A:%d.\n ", currentDensityAreaRatio, minAreaDensityRatio, 
                //			objectHeight, objectWidth,objFeature->area);

                if ( currentDensityAreaRatio >= minAreaDensityRatio )
                {
                    return TRUE;
                }
                else
				{
                     //if ( DEBUG_IS_CIRCLE )
                     //print("Fail AreaRatio Current:%d >= Min:%d \n ", currentDensityAreaRatio, minAreaDensityRatio);
				}

            }
            else
            {
                objectWidthHeightRatioTolerance = elementDef->widthHeightPercentRatioTolerance;

                if ( (objectWidthHeightDiff <= objectWidthHeightRatioTolerance) || ( objectWidthHeightRatioTolerance == (-1)) )
                {
                     //if ( DEBUG_IS_CIRCLE )
                     //    print("Sqrness OK.. WHDiff:%d\n ", objectWidthHeightDiff);

                    // round-ness check

                    currentDensityAreaRatio = ((long int)( objFeature->area * 100)) / objectPerimeterArea;

                    objectPerimeterAreaTolerance =
                    	elementDef->perimeterAreaPercentRatioTolerance;

                    objectPerimeterAreaDiff = abs((currentDensityAreaRatio - 100) - CIRCLE_NOMINAL_P_A_DIFF);

                    if ( (objectPerimeterAreaDiff <= objectPerimeterAreaTolerance) || ( objectPerimeterAreaTolerance == (-1)) )
                    {
                        //if ( DEBUG_IS_CIRCLE )
                        //{
                        //    print("Rndness OK PADiff:%d.. Element FOUND\n", objectPerimeterAreaDiff);
                        //    print("ObjectCenter: %d,%d\n", objFeature->x_center,objFeature->y_center);
                        //}

                        return TRUE;
                    }
                     else
                     {
                         //if ( DEBUG_IS_CIRCLE )
                         //    print("Fail PADiff :%d Tol:%d .. ", objectPerimeterAreaDiff, objectPerimeterAreaTolerance);
						 return FALSE;
                     }
                }
                else
                {
                    //if ( DEBUG_IS_CIRCLE )
                    //{
                    //    print("Fail Sqrness Diff:%d Tol:%d.. \n", objectWidthHeightDiff, objectWidthHeightRatioTolerance);
                    //    print("Fail W:%d H:%d..  \n", objectWidth, objectHeight);
                    //}

					return FALSE;
                }

            }



//             if ( (objectWidthHeightDiff <= objectWidthHeightRatioTolerance) || ( objectWidthHeightRatioTolerance == (-1)) )
//             {
//                 if ( DEBUG_IS_CIRCLE )
//                     print("Sqrness OK.. WHDiff:%d\n ", objectWidthHeightDiff);
//
//                 // round-ness check
//
//                 currentDensityAreaRatio = ((objectPerimeterArea * 100) / objFeature->area);
//
//                 objectPerimeterAreaTolerance =
//                     elementDef->perimeterAreaPercentRatioTolerance;
//
//                 objectPerimeterAreaDiff = abs((currentDensityAreaRatio - 100) - CIRCLE_NOMINAL_P_A_DIFF);
//
//                 if ( objectPerimeterAreaDiff <= objectPerimeterAreaTolerance || ( objectPerimeterAreaTolerance == (-1)) )
//                 {
//                     if ( DEBUG_IS_CIRCLE )
//                     {
//                         print("Rndness OK PADiff:%d.. Element FOUND\n", objectPerimeterAreaDiff);
//                         print("ObjectCenter: %d,%d\n", objFeature->x_center,objFeature->y_center);
//                     }
//                     return TRUE;
//                 }
//                 else
//                 {
//                     if ( DEBUG_IS_CIRCLE )
//                         print("Fail PADiff :%d Tol:%d .. ", objectPerimeterAreaDiff, objectPerimeterAreaTolerance);
//                 }
//             }
//             else
//             {
//                 if ( DEBUG_IS_CIRCLE )
//                 {
//                     print("Fail Sqrness Diff:%d Tol:%d.. \n", objectWidthHeightDiff, objectWidthHeightRatioTolerance);
//                     print("Fail W:%d H:%d..  ", objectWidth, objectHeight);
//                 }
//             }
        }
        // else
        // {
        //     if ( DEBUG_IS_CIRCLE )
        //     {
        //         print("Height Fail %d >= %d >= %d..\n  ", elementDef->maxHeight, objectHeight, elementDef->minHeight);
        //         print("Width Fail  %d >= %d >= %d..  ", elementDef->maxWidth, objectWidth, elementDef->minWidth );
        //     }
        // }
    }

    // if ( DEBUG_IS_CIRCLE )
    //     print("Element NOT FOUND\n");
    //
    return FALSE;
}

void    InitValidObjectArray(void)
{
    int x, y;

    for ( x = 0; x < MAX_ELEMENTS; x++ )
    {
        validObjectFeatures[x].objectFeaturesCount = 0;

        for ( y = 0; y < MAX_VALID_OBJECTS; y++ )
        {
            validObjectFeatures[x].objectFeatures[y].hitCount = 0;

            validObjectFeatures[x].objectFeatures[y].isSingle = FALSE;
            validObjectFeatures[x].objectFeatures[y].isComplex = FALSE;
            validObjectFeatures[x].objectFeatures[y].isCompound = FALSE;
            validObjectFeatures[x].objectFeatures[y].isSingleVirtual = FALSE;

            virtualSingleObjects.objectFeatures[y].isSingle = FALSE;
            virtualSingleObjects.objectFeatures[y].isComplex = FALSE;
            virtualSingleObjects.objectFeatures[y].isCompound = FALSE;
            validObjectFeatures[x].objectFeatures[y].isSingleVirtual = FALSE;
        }
    }

    virtualSingleObjects.objectFeaturesCount = 0;
}

VECTOR NormalizeRibElementVectors( ELEMENT_PAIR_DEFINITION elementDef)
{
    VECTOR currentVector;

    if ( currentSystemConfiguration.cameraImaging.rotateImage ) //Rotated
    {
        if ( topEdgeFirst )
        {
            currentVector.x = elementDef.nominalHorizontalDistToPartner;
            currentVector.y = elementDef.nominalVerticalDistToPartner;     
        }
        else
        {
            currentVector.x = -elementDef.nominalHorizontalDistToPartner;
            currentVector.y = -elementDef.nominalVerticalDistToPartner;     
        } 
    }
    else
    {
        if ( !topEdgeFirst )
        {
            currentVector.y = elementDef.nominalHorizontalDistToPartner;
            currentVector.x = elementDef.nominalVerticalDistToPartner;     
        }
        else
        {
            currentVector.y = -elementDef.nominalHorizontalDistToPartner;
            currentVector.x = -elementDef.nominalVerticalDistToPartner;     
        }

		if( flipMark )
		{
			currentVector.y = -currentVector.y;
			currentVector.x = -currentVector.x;
		}
    }

    return currentVector;
}

void    ShowValidObjectArray(void)
{
    // int x, y, c;
    //
    // for ( x = 0; x < MAX_ELEMENTS; x++ )
    // {
    //     c = validObjectFeatures[x].objectFeaturesCount;
    //
    //     for (y = 0; y < c; y++ ) 
    //     {
    //         print("E:%d HitC %d S:%d CPLX:%d CMPD:%d SV:%d\n", x,
    //               validObjectFeatures[x].objectFeatures[y].hitCount,
    //               validObjectFeatures[x].objectFeatures[y].isSingle,
    //               validObjectFeatures[x].objectFeatures[y].isComplex,
    //               validObjectFeatures[x].objectFeatures[y].isCompound,
    //               validObjectFeatures[x].objectFeatures[y].isSingleVirtual);
    //     }
    // }
}

void    ResetValidObjectArray(void)
{
    InitValidObjectArray();
}

void    AddValidObject(VC_FEATURES *objFeature, BYTE markElemType)
{
    int featuresCount = validObjectFeatures[ markElemType ].objectFeaturesCount;

    if ( featuresCount < MAX_VALID_OBJECTS )
    {
        validObjectFeatures[ markElemType ].objectFeatures[featuresCount].features =
        *(objFeature);

        validObjectFeatures[ markElemType ].objectFeaturesCount++;
    }
    /// else
    /// {
    ///     print("TOO MANY VALID ELEMENTS: %d\n", markElemType);
    /// }
}


VC_FEATURES * GetValidObject(int objIdx, BYTE elem)
{
    return &(validObjectFeatures[ elem ].objectFeatures[objIdx].features);
}

int     GetValidPartnerCount(BYTE markElemType)
{
    return GetObjectCount( markElemType );
}

int     GetObjectCount(BYTE element)
{
    return  validObjectFeatures[element].objectFeaturesCount;
}

void        ExtractValidObjects(VC_FEATURES  objFeatures[], int objectCount)
{
    int objNo, validObjectCount;
    BYTE    elem;
    MARK_ELEMENT_TYPE elementType;
    POINT objCenter;

    CameraImaging   *camImagingPtr  = 
    &GetCurrSystemConfig()->cameraImaging;

    //print("Extract Valid Objects:%d\n", objectCount);

    ResetValidObjectArray();

    validObjectCount = 0;

    for ( objNo=0; objNo<objectCount; objNo++ )
    {
        //check for valid element or enabled element
        if ( objFeatures[objNo].color == BLACK )
        {
            for ( elem = 0; elem < MAX_ELEMENTS; elem++ )
            {
                if (GetDeviceType() == DEV_TYPE_REGISTER)
				{
                	if(elem == 1)
                    	elementType = COMPLEX;
                	else
                    	elementType = SINGLE;
				}
				else
				{
                	if(elem == 1)
                    	elementType = OVAL;
                	else
                    	elementType = CIRCLE;
				}

				//printf("Obj[%d] X:%d, Y:%d Type:%d\n", objNo, objCenter.x, objCenter.y, elementType);

                if ( IsValidElement(&objFeatures[objNo], &camImagingPtr->elements[ elem ], elementType ) )
                {
                    AddValidObject(&objFeatures[objNo], elem);
                    validObjectCount++;

                    if ( diagnosticsEnabled )
                    {
                        objCenter.x = objFeatures[objNo].x_center + curAOI.startX;
                        objCenter.y = objFeatures[objNo].y_center + curAOI.startY;
                        DrawCrossHair(&imageDiagBuffer.frameImage, objCenter);
                        //                      printf("Obj[%d] %d, %d Type:%d\n", objNo, objCenter.x,objCenter.y, elementType);
                        //DrawBoxObject(&imageDiagBuffer.frameImage, &objFeatures[objNo]);
                    }
                    break;
                }
            }
        }
    }

    //print("Valid Objects Count :%d\n", validObjectCount);
}

//
// TODO
// Split into different subroutines
// Preselect partners based on size ratio of first element
// Or validate that the marks are scaled properly relative
// to each other
//


void    DoMarkRecognition(VC_IMAGE *frameImage)
{
    POINT   thisCenterPoint, partnerCenterPoint;
    VECTOR  distToPartnerCenterPoint;
    int     firstElementNominalArea, diffRatio;
    int     perimeterTolerance;
    RECT    partnerPerimeterRect;

    VALID_OBJECT_FEATURES *validFirstObjects;

    int numFirstObjects;
    int validObjIdx;

    int numPairsToSearch = 0;
    int numPairsFound = 0;
    int lastNumPair = 0;

    int firstObjIdx, numValidPartners, numPair, partnerObjIdx;

    int markNumber = 0;

    CameraImaging   *camImagingPtr  = 
    &GetCurrSystemConfig()->cameraImaging;

    MARK_ELEMENT_TYPE   firstElemType, thisElemType, partnerElemType;

    VC_FEATURES         *firstObjectElem, *thisObjectElem, *partnerObjectElem;

    // TODO RESET, Mark Search status


    // Search through all the first object element type
    // For matches and possible duplicates

    for ( markNumber = 0; markNumber < MAX_MARKS; markNumber++ )
    {
        if ( !camImagingPtr->markDefinition[markNumber].enable )
            continue;

        markRecognitionData[markNumber].numMatchFound = 0;

        firstElemType =
        camImagingPtr->markDefinition[markNumber].elementPairs[0].thisElementType ;

        validFirstObjects = &validObjectFeatures[firstElemType];
        firstElementNominalArea = camImagingPtr->elements[firstElemType].nominalAreaSize;

        numFirstObjects = validFirstObjects->objectFeaturesCount;
        numPairsToSearch = camImagingPtr->markDefinition[markNumber].numPairsToSearch;


        for ( firstObjIdx = 0; firstObjIdx < numFirstObjects; firstObjIdx++ )
        {
            validObjIdx = firstObjIdx;
            numPairsFound = 0;
            lastNumPair = 0;

            firstObjectElem = GetValidObject(firstObjIdx, firstElemType);

            // Note Scaling is non linear, take only 50% of the difference and apply

            diffRatio = (firstObjectElem->area * 100) / firstElementNominalArea;
            markRecognitionData[ markNumber ].diffRatioNominalSize = 100 + ((diffRatio - 100) >> 1);

            //print("DIFF RATIO %d %d %d\n", 
            //markRecognitionData[ markNumber ].diffRatioNominalSize, firstElementNominalArea, firstObjectElem->area);

            for ( numPair=0; numPair<numPairsToSearch; numPair++ )
            {
                thisElemType = camImagingPtr->markDefinition[markNumber].elementPairs[numPair].thisElementType;
                partnerElemType = camImagingPtr->markDefinition[markNumber].elementPairs[numPair].partnerElementType;

                perimeterTolerance = 
                camImagingPtr->markDefinition[markNumber].elementPairs[numPair].perimeterToleranceCenterOfPartner;

                // Note: Width and dist x y inversion for camera Vertical FOV

                distToPartnerCenterPoint = NormalizeRibElementVectors( camImagingPtr->markDefinition[markNumber].elementPairs[numPair] );

                // If in the begining of search this-element is the firstObject to find
                // else the found partnerobject becomes the new this-element

                if ( numPairsFound == 0 )
                {
                    thisObjectElem = firstObjectElem;
                }
                else
                {
                    thisObjectElem = GetValidObject(partnerObjIdx, thisElemType);
                }

                //DrawBoxObject(frameImage, thisObjectElem);

                numValidPartners = GetValidPartnerCount(partnerElemType);

                thisCenterPoint.x = thisObjectElem->x_center;
                thisCenterPoint.y = thisObjectElem->y_center;

                // Apply Dynamic Scaling

                distToPartnerCenterPoint.x =
                (distToPartnerCenterPoint.x * markRecognitionData[ markNumber ].diffRatioNominalSize) / 100;
                distToPartnerCenterPoint.y =
                (distToPartnerCenterPoint.y * markRecognitionData[ markNumber ].diffRatioNominalSize) / 100;

                partnerPerimeterRect = GeneratePartnerRect(thisCenterPoint, distToPartnerCenterPoint, perimeterTolerance, FALSE);

                //DrawBoxRect(frameImage, &partnerPerimeterRect);

                validObjIdx++;

                for ( partnerObjIdx =0; partnerObjIdx<numValidPartners; partnerObjIdx++ )
                {
                    partnerObjectElem = GetValidObject(partnerObjIdx, partnerElemType);

                    partnerCenterPoint.x = partnerObjectElem->x_center;
                    partnerCenterPoint.y = partnerObjectElem->y_center;

                    if ( IsPointInRectBound(&partnerCenterPoint, &partnerPerimeterRect) )
                    {
                        // Remember the first object

                        if ( numPair == 0 )
                        {
                            markRecognitionData[ markNumber ].firstObject.features = (*thisObjectElem);
                        }

                        numPairsFound++;
                        validObjIdx = 0;

                        // DrawBoxObject(frameImage, partnerObjectElem);

                        break;
                    }
                }//for ( partnerObjIdx =0;

                if ( numPairsFound == 0 )
                {
                       print("FIRST PAIR NOT FOUND\n");
                    break;
                }
                else
                {
                    if ( numPairsFound == lastNumPair )
                    {
                              print("NEXT PAIR NOT FOUND\n"); 
                        break;
                    }
                    else
                        if ( numPairsFound == numPairsToSearch )
                    {
                        lastNumPair = numPairsFound;

                        // Remember the last object, when mark is found

                        markRecognitionData[ markNumber ].lastObject.features = (*partnerObjectElem);

                        markRecognitionData[ markNumber ].numMatchFound++;

                        if ( diagnosticsEnabled )
                            DrawLineBetweenOjbects(frameImage, 
                                                   &markRecognitionData[ markNumber ].firstObject.features, 
                                                   &markRecognitionData[ markNumber ].lastObject.features);

                        //print("FOUND MARK[%d] :%d\n",markNumber , markRecognitionData[ markNumber ].numMatchFound);

                        break;
                    }
                }
            }//for ( numPair=0;
        }//for ( firstObjIdx = 0;
    }
}

MARK_STATUS    DoMarkRecognitionResultAnalysis(VC_IMAGE *frameImage)
{
    POINT centerPoint;

    BYTE markNumber ;

    CameraImaging   *camImagingPtr  = 
    &GetCurrSystemConfig()->cameraImaging;

    for ( markNumber = 0; markNumber < MAX_MARKS; markNumber++ )
    {
        if ( !camImagingPtr->markDefinition[markNumber].enable )
            continue;

        if ( markRecognitionData[ markNumber ].numMatchFound == 1 )
        {
            //  print("MARK FOUND\n");

            markRecognitionData[ markNumber ].markSearchStatus = MARK_FOUND;

            centerPoint = 
            GetCenterOfTwoObjects(frameImage, 
                                  &markRecognitionData[ markNumber ].firstObject.features, 
                                  &markRecognitionData[ markNumber ].lastObject.features);

            markRecognitionData[ markNumber ].markFovLateral = centerPoint.x + (markRecognitionData[ markNumber ].markFovlateralStart * SUB_PIXEL_SCALE);
            markRecognitionData[ markNumber ].markFovCircum  = centerPoint.y + (markRecognitionData[ markNumber ].markFovCircumStart * SUB_PIXEL_SCALE);

            //    print("MARK POSITION[%d] x:%d  y:%d\n",markNumber, markRecognitionData[ markNumber ].markFovLateral
            //                                          ,  markRecognitionData[ markNumber ].markFovCircum);

            //DrawBoxTwoObjects(frameImage, &mar]kRecognitionData.firstObject, &markRecognitionData.lastObject);

            NormalizeToCameraCenter(frameImage, markNumber);

            //print("CENTER NORMALZIED MARK POSITION x:%d  y:%d\n", 
            //      markRecognitionData.markFovLateral,  markRecognitionData.markFovCircum);
        }
        else if ( markRecognitionData[ markNumber ].numMatchFound == 0 )
        {
            //   print("NO MARKS FOUND\n");

            markRecognitionData[ markNumber ].markSearchStatus = MARK_NOT_FOUND;
            markRecognitionData[ markNumber ].markFovLateral = 0;
            markRecognitionData[ markNumber ].markFovCircum = 0;
        }
        else
        {
            //  print("TOO MANY MARKS FOUND %d\n", markRecognitionData[ markNumber ].numMatchFound );
            markRecognitionData[ markNumber ].markSearchStatus = MARK_FOUND_TOO_MANY;
        }
    }

    markNumber = GetReferenceMarkIndex();

    if ( markRecognitionData[ markNumber ].markSearchStatus == MARK_FOUND )
        return MARK_FOUND;
    else
        return MARK_NOT_FOUND;
}


//

void    DoMarkValidations(VC_IMAGE *frameImage)
{
    // Area Size

    // Vertical or Horizontal

    // White Space Nominal



}

void    SetStartingFovStarts(long x, long y)
{
    BYTE markNumber = GetReferenceMarkIndex();

    markRecognitionData[ markNumber ].markFovlateralStart   = x;
    markRecognitionData[ markNumber ].markFovCircumStart    = y;
}

void    InitMarkValidationData(void)
{
    BYTE markNumber, v ;

    for ( markNumber = 0; markNumber < MAX_MARKS; markNumber++ )
    {
        markRecognitionData[ markNumber ].markSearchStatus = MARK_NOT_FOUND;
        markRecognitionData[ markNumber ].markFovLateral   = 0;
        markRecognitionData[ markNumber ].markFovCircum    = 0;

        markRecognitionData[ markNumber ].numMatchFound = 0;


        markRecognitionData[ markNumber ].isColorValid = TRUE;
        markRecognitionData[ markNumber ].threshold = 0;

        for ( v = 0; v < MAX_VECTOR_MATCH; v++ )
        {
            markRecognitionData[ markNumber ].isVectorOpposite[ v ] = FALSE;
            markRecognitionData[ markNumber ].thisObject[ v ] = NULL;
            markRecognitionData[ markNumber ].partnerObject[ v ] = NULL;
        }
    }
}

void    ShowMarkValidationData(void)
{
    BYTE markNumber, v , m;

    CameraImaging   *camImagingPtr  = 
    &GetCurrSystemConfig()->cameraImaging;

    MARK_DEFINITION currentMark;

    for ( markNumber = 0; markNumber < MAX_MARKS; markNumber++ )
    {
        //markRecognitionData[ markNumber ].markSearchStatus = MARK_UNDEFINED;
        //markRecognitionData[ markNumber ].markFovLateral   = 0;
        //markRecognitionData[ markNumber ].numMatchFound = 0;

        m = markRecognitionData[ markNumber ].numMatchFound;

        for ( v = 0; v < m; v++ )
        {
            currentMark = camImagingPtr->markDefinition[markNumber];

            if ( currentMark.enable )
            {
                //  print("MN:%d VM:%d VOP:%d\n", 
                //        markNumber, v, 
                //        markRecognitionData[ markNumber ].isVectorOpposite[ v ]);

                ShowObjectFeatures(markRecognitionData[ markNumber ].thisObject[ v ]);
                ShowObjectFeatures(markRecognitionData[ markNumber ].partnerObject[ v ]);
            }
        }
    }
}



void    ShowObjectFeatures(OBJECT_FEATURES *objFeatures)
{
    //  print("OF HitC:%d S:%d CMPLX:%d CMPND:%d SV:%d\n", 
    //          objFeatures->hitCount, objFeatures->isSingle,
    //          objFeatures->isComplex, objFeatures->isCompound, 
    //          objFeatures->isSingleVirtual);
}

