




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
#include <flib.h>

////////////////////////////////////////////////////////////////////


#include "imaging.h"

#include "markrecognitionhandler.h"

#include "imageprocessinghandler.h"

#include "cameraconfig.h"


///////////////////////////////////////////////////////////////////

// TODO is part of config
// Also shorten default shutter timeto 250 microseconds

long    shutterTime;

int     whiteLevelTarget;

int     rangeShiftDivisor;

int     startThresholdMultiplier;

int     maxSteppingRetriesSearching;
int     maxSteppingRetriesLockedOn;
int     maxSteppingRetriesCurr;


int     incrementThresholdMultiplier;


///////////////////////////////////////////////////////////////////
//
// Camera Initialization
// Function Definition
//
///////////////////////////////////////////////////////////////////

void    InitializeVcCamera(void)
{
    //print("Initialize VC Camera\n\n");

    SET_SHUTTER_TIME_PARAMETER(INIT_EXPOSURE_TIME_TICKS);
    VC_SET_SHUTTER(GET_SHUTTER_TIME_PARAMETER());

    VC_VIDEO_MODE_STILL();

    // TODO
    VC_SET_TRIGINPUT_POSITIVE_EDGE();
    // VC_SET_TRIGINPUT_NEGATIVE_EDGE();
    VC_SET_TRIGOUT_EXPOSURE_MODE();
    VC_SET_TRIGOUT_POSITIVE();

    VC_TAKE_PICTURE();
}


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    ShowFrameImageAttributes(FRAME_BUFFER_IMAGE *fbiPtr)
{
    //print("FRAME IMAGE\n");
    ImagePrintMembers("ATTRIBUTES : ", &fbiPtr->frameImage);


    //VC_IMAGE    frameImage;
    //int         frameImageAddress;
    //int         trackNumber;
    //U32         flashCount;     
    //int         whiteLevel, blackLevel;
    //int         contrast;
    //int         threshold;
    //long         histogram[MAX_HISTOGRAM_POINTS];

    //RECT
}

//
// TODO 
// this is based on the image starting at address 0,0
// need one for frames starting at address other than 0
//

void    GetImageData(VC_IMAGE *image, BYTE *data, int startingRow, int rowCount)
{
    ADDRESS startImageAddress;
    ADDRESS currImageAddress;

    int     imageDx = image->dx;
    int     r;

    BYTE *dataPtr = data;

    startImageAddress = image->st;

    currImageAddress = 
    startImageAddress + (startingRow * image->pitch);

    for ( r=0; r<rowCount; r++ )
    {
        memcpy(dataPtr, (BYTE *)currImageAddress, imageDx);
        dataPtr += imageDx;
        currImageAddress += image->pitch;
    }
}

///////////////////////////////////////////////////////////////////
//
//
//
///////////////////////////////////////////////////////////////////

int    GetRectAverageLevel( RECT *rect )
{
    int x, y;

    int pixelSum    = 0;
    int pixelCount  = 0;
    int pixelAvg    = 0;

    int startX = rect->top.x;
    int startY = rect->top.y;
    int endX   = rect->bottom.x;
    int endY   = rect->bottom.y;

    int quadrantOffsetX = curAOI.startX;
    int quadrantOffsetY = curAOI.startY;

    for ( x=startX; x<=endX; x++ )
        for ( y=startY; y<=endY; y++ )
        {
            pixelSum += 
            VC_IMAGE_GET_PIXEL(&fbiPtr->frameImage, quadrantOffsetX+x, quadrantOffsetY+y);

            pixelCount++;
        }

    pixelAvg = pixelSum/pixelCount;

    return pixelAvg;
}


int    GetRectMinLevel( RECT *rect )
{
    int x, y;

    int pixelLevel      = 0;
    int minPixelLevel   = WHITE;

    int startX = rect->top.x;
    int startY = rect->top.y;
    int endX   = rect->bottom.x;
    int endY   = rect->bottom.y;

    int quadrantOffsetX = curAOI.startX;
    int quadrantOffsetY = curAOI.startY;

    for ( x=startX; x<=endX; x++ )
        for ( y=startY; y<=endY; y++ )
        {
            pixelLevel += 
            VC_IMAGE_GET_PIXEL(&fbiPtr->frameImage, quadrantOffsetX+x, quadrantOffsetY+y);

            minPixelLevel = MIN(pixelLevel, minPixelLevel);
        }

    return minPixelLevel;
}


// Identify the pixel color based on the bayer pattern

PIXEL_COLOR     GetPixelColor(int x, int y)
{
    if ( IS_EVEN(x) && IS_EVEN(y) ) return RED_PIXEL;
    if ( IS_ODD(x) && IS_EVEN(y) ) return BLUE_PIXEL;

    return GREEN_PIXEL;
}

CMYK_COLOR  GetAreaColor( RECT *rect )
{
    int x, y;

    PIXEL_COLOR pixelColor;

    int RED_pixelSum    = 0;
    int RED_pixelCount  = 0;
    int RED_pixelAvg    = 0;

    int GREEN_pixelSum      = 0;
    int GREEN_pixelCount    = 0;
    int GREEN_pixelAvg      = 0;

    int BLUE_pixelSum       = 0;
    int BLUE_pixelCount     = 0;
    int BLUE_pixelAvg       = 0;

    int startX = rect->top.x;
    int startY = rect->top.y;
    int endX   = rect->bottom.x;
    int endY   = rect->bottom.y;

    for ( x=startX; x<=endX; x++ )
        for ( y=startY; y<=endY; y++ )
        {
            pixelColor = GetPixelColor(x,y);

            switch ( pixelColor )
            {
                case RED_PIXEL:
                    //RED_pixelSum += VC_IMAGE_GET_PIXEL(image, x, y);
                    RED_pixelCount++;
                    break;

                case GREEN_PIXEL:
                    //GREEN_pixelSum += VC_IMAGE_GET_PIXEL(image, x, y);
                    GREEN_pixelCount++;
                    break;

                default: // BLUE_PIXEL
                    //BLUE_pixelSum += VC_IMAGE_GET_PIXEL(image, x, y);
                    BLUE_pixelCount++;
                    break;
            }
        }

    RED_pixelAvg = RED_pixelSum/RED_pixelCount;
    GREEN_pixelAvg = GREEN_pixelSum/GREEN_pixelCount;
    BLUE_pixelAvg = BLUE_pixelSum/BLUE_pixelCount;

    if ( (BLUE_pixelAvg > RED_pixelAvg) && (BLUE_pixelAvg > GREEN_pixelAvg) )
        return CYAN_COLOR;

    if ( (RED_pixelAvg > GREEN_pixelAvg) && (RED_pixelAvg > BLUE_pixelAvg) )
        return MAGENTA_COLOR;

    if ( (RED_pixelAvg > BLUE_pixelAvg) && (GREEN_pixelAvg > BLUE_pixelAvg) )
        return YELLOW_COLOR;

    // BLACK??

    return NO_COLOR;
}

///////////////////////////////////////////////////////////////////
//
// Function Definitions
//
///////////////////////////////////////////////////////////////////

BOOL    IsPointInRectBound(POINT *point, RECT *bound)
{
    if ( ((point->x >= bound->top.x) && (point->y >= bound->top.y)) &&
         ((point->x <= bound->bottom.x) && (point->y <= bound->bottom.y)) )
        return TRUE;

    return FALSE;
}


BOOL    IsRectInRectBound(RECT *rect, RECT *bound)
{

    if ( ((rect->top.x >= bound->top.x) && (rect->top.y >= bound->top.y)) &&
         ((rect->bottom.x <= bound->bottom.x) && (rect->bottom.y <= bound->bottom.y)) )
        return TRUE;

    return FALSE;
}


BOOL    IsRectIntersectingRectBound(RECT *rect, RECT *bound)
{
    if ( (((rect->top.x >= bound->top.x) && (rect->top.x <= bound->bottom.x)) ||
          ((rect->bottom.x >= bound->top.x) && (rect->bottom.x <= bound->bottom.x))) 
         &&
         (((rect->top.y >= bound->top.y) && (rect->top.y <= bound->bottom.y)) ||
          ((rect->bottom.y >= bound->top.y) && (rect->bottom.y <= bound->bottom.y))) )
        return TRUE;

    return FALSE;
}


void CenterRectAtPointCenter(POINT *pointCenter, RECT *rect)
{
    //NOTE:  Untested 
    rect->top.x = rect->top.x - rect->center.x + pointCenter->x;
    rect->top.y = rect->top.y - rect->center.y + pointCenter->y;

    rect->bottom.x = rect->bottom.x - rect->center.x + pointCenter->x;
    rect->bottom.y = rect->bottom.y - rect->center.y + pointCenter->y;

    rect->center = *(pointCenter);
}


void AssignRect(RECT *sourceRect, RECT *destRect)
{
    memcpy(destRect, sourceRect, sizeof(RECT));
}

void ShowRectAttributes(RECT *rect)
{
//    print("Tx:%d  Ty:%d  Cx:%d  Cy:%d  Bx:%d  By:%d\n",
//          rect->top.x, rect->top.y,
//          rect->center.x, rect->center.y,
//          rect->bottom.x, rect->bottom.y);
}

void ShowPointAttributes(POINT *point)
{
    //print("x:%d y:%d\n", point->x, point->y);
}


void    ShowVectorAttributes(VECTOR *vect)
{
    //print("VECTOR : ");

    ShowPointAttributes(vect);
}


//

BOOL IsPerimeterRectValid( RECT perimeterRect)
{
    if ( perimeterRect.bottom.x <= 0 || perimeterRect.bottom.y <= 0 ||
         perimeterRect.top.x <= 0 ||perimeterRect.top.y <= 0 )
    {
        return FAIL;
    }

    //DEBUG
    //NEED to Check if permeter is inside frame
    return PASS;
}

BOOL IsPartnerPointValid( POINT partnerPoint)
{
    if ( partnerPoint.x <= 0 || partnerPoint.y <= 0 ||
         partnerPoint.x > 1000 || partnerPoint.y > 1000 )
    {
        return FAIL;
    }

    //DEBUG
    //NEED to Check if permeter is inside frame
    return PASS;

}



POINT    GeneratePartnerPoint(POINT thisCenterPt, VECTOR distToPartnerCenterPt, BOOL invert)
{
    POINT point;

    if ( invert )
    {
        point.x = thisCenterPt.x - distToPartnerCenterPt.x;
        point.y = thisCenterPt.y - distToPartnerCenterPt.y;

    }
    else
    {
        point.x = thisCenterPt.x + distToPartnerCenterPt.x;
        point.y = thisCenterPt.y + distToPartnerCenterPt.y;
    }

    return point;
}

RECT    GeneratePartnerRect(POINT thisCenterPt, VECTOR distToPartnerCenterPt, int perimeterTol, BOOL invert)
{
    RECT rect;

    if ( invert )
    {
        thisCenterPt.x -= distToPartnerCenterPt.x;
        thisCenterPt.y -= distToPartnerCenterPt.y;

    }
    else
    {
        thisCenterPt.x += distToPartnerCenterPt.x;
        thisCenterPt.y += distToPartnerCenterPt.y;
    }

    rect.center.x = thisCenterPt.x;
    rect.center.y = thisCenterPt.y;

    rect.top.x = thisCenterPt.x - perimeterTol;
    rect.top.y = thisCenterPt.y - perimeterTol;
    rect.bottom.x = thisCenterPt.x + perimeterTol;
    rect.bottom.y = thisCenterPt.y + perimeterTol;

    return rect;
}

RECT    GenerateFeatureRect(VC_FEATURES *objFeatures)
{
    RECT rect;

    rect.top.x = objFeatures->x_min;
    rect.top.y = objFeatures->y_min;
    rect.bottom.x =  objFeatures->x_max;
    rect.bottom.y = objFeatures->y_max;

    rect.center.x = objFeatures->x_center;
    rect.center.y = objFeatures->y_center;

    return rect;
}

RECT    GenerateRectFromPoints(POINT topPt, POINT botPt)
{
    RECT tempRect;

    tempRect.top = topPt;
    tempRect.bottom = botPt;

    return tempRect;
}

void ShowFeaturetAttributes(VC_FEATURES *objFeatures)
{
//    print("A:%d  Color:%d  ", objFeatures->area, objFeatures->color);
//
//    print("xMin:%d  yMin:%d  xMax:%d  yMax:%d\n",
//          objFeatures->x_min, objFeatures->y_min,
//          objFeatures->x_max, objFeatures->y_max);
}


///////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////

void    DrawCrossHair(VC_IMAGE *frameImage, POINT ptCenter)
{
    POINT startPoint;
    POINT endPoint;
    startPoint.x    = ptCenter.x - CROSS_HAIR_SIZE;
    startPoint.y    = ptCenter.y;

    endPoint.x      = ptCenter.x + CROSS_HAIR_SIZE;
    endPoint.y      = ptCenter.y;

    DrawLine(frameImage,  startPoint,  endPoint);

    startPoint.x    = ptCenter.x;
    startPoint.y    = ptCenter.y - CROSS_HAIR_SIZE;

    endPoint.x      = ptCenter.x;
    endPoint.y      = ptCenter.y + CROSS_HAIR_SIZE;

    DrawLine(frameImage,  startPoint,  endPoint);
}

///////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////

void    DrawCrossHairSize(VC_IMAGE *frameImage, POINT ptCenter, int size)
{
    POINT startPoint;
    POINT endPoint;
    startPoint.x    = ptCenter.x - size;
    startPoint.y    = ptCenter.y;

    endPoint.x      = ptCenter.x + size;
    endPoint.y      = ptCenter.y;

    DrawLine(frameImage,  startPoint,  endPoint);

    startPoint.x    = ptCenter.x;
    startPoint.y    = ptCenter.y - size;

    endPoint.x      = ptCenter.x;
    endPoint.y      = ptCenter.y + size;

    DrawLine(frameImage,  startPoint,  endPoint);
}

//

void    DrawLine(VC_IMAGE *frameImage, POINT startPoint, POINT endPoint)
{
    if ( 0 > startPoint.x )
        startPoint.x   = 0;

    if ( startPoint.x > frameImage->dx )
        startPoint.x   = frameImage->dx;

    if ( 0 > endPoint.x )
        endPoint.x   = 0;

    if ( endPoint.x > frameImage->dx )
        endPoint.x   = frameImage->dx;

    if ( 0 > startPoint.y )
        startPoint.y   = 0;

    if ( startPoint.y > frameImage->dy )
        startPoint.y   = frameImage->dy;

    if ( 0 > endPoint.y )
        endPoint.y   = 0;

    if ( endPoint.y > frameImage->dy )
        endPoint.y   = frameImage->dy;

    linex( frameImage, startPoint.x, startPoint.y, endPoint.x, endPoint.y,0xFF);
}

void    DrawHorizontalLine(VC_IMAGE *frameImage, POINT startPoint, POINT endPoint)
{
    int x, lineLength;

    int     dxOrigin = frameImage->dx;
    int     dyOrigin = frameImage->dy;

    // Offset into correct frame image coordinates

    lineLength = endPoint.x - startPoint.x;

    startPoint.x += dxOrigin >> 1;
    startPoint.y += dyOrigin >> 1;

    for ( x=0; x<lineLength; x++ )
    {

        //  VC_XOR_PIX(ScrByteAddr(startPoint.x + x, startPoint.y));
        VC_XOR_PIX((U8 *)(ScrGetLogPage+(startPoint.x + x)+(startPoint.y)*ScrGetPitch));
    }
}


void    DrawVerticallLine(VC_IMAGE *frameImage, POINT startPoint, POINT endPoint)
{
    int y, lineLength;

    int     dxOrigin = frameImage->dx;
    int     dyOrigin = frameImage->dy;

    // Offset into correct frame image coordinates

    lineLength = endPoint.y - startPoint.y;

    startPoint.x += dxOrigin >> 1;
    startPoint.y += dyOrigin >> 1;

    for ( y=0; y<lineLength; y++ )
    {
        //VC_XOR_PIX(ScrByteAddr(startPoint.x, startPoint.y + y));
        VC_XOR_PIX((U8 *)(ScrGetLogPage+(startPoint.x)+( startPoint.y + y)*ScrGetPitch));
    }
}

///////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////

void    DrawBoxObject(VC_IMAGE *frameImage, VC_FEATURES *ObjFeature)
{
    POINT startPt, endPt;
    int quadrantOffsetX = curAOI.startX;
    int quadrantOffsetY = curAOI.startY;

    startPt.x = ObjFeature->x_min + quadrantOffsetX;
    startPt.y = ObjFeature->y_min + quadrantOffsetY;
    endPt.x = ObjFeature->x_max + quadrantOffsetX;
    endPt.y = ObjFeature->y_min + quadrantOffsetY;
    DrawLine(frameImage, startPt, endPt);

    startPt.x = ObjFeature->x_min + quadrantOffsetX;
    startPt.y = ObjFeature->y_max + quadrantOffsetY;
    endPt.x = ObjFeature->x_max + quadrantOffsetX;
    endPt.y = ObjFeature->y_max + quadrantOffsetY;
    DrawLine(frameImage, startPt, endPt);

    startPt.x = ObjFeature->x_min + quadrantOffsetX;
    startPt.y = ObjFeature->y_min + quadrantOffsetY;
    endPt.x = ObjFeature->x_min + quadrantOffsetX;
    endPt.y = ObjFeature->y_max + quadrantOffsetY;
    DrawLine(frameImage, startPt, endPt);

    startPt.x = ObjFeature->x_max + quadrantOffsetX;
    startPt.y = ObjFeature->y_min + quadrantOffsetY;
    endPt.x = ObjFeature->x_max + quadrantOffsetX;
    endPt.y = ObjFeature->y_max + quadrantOffsetY;
    DrawLine(frameImage, startPt, endPt);
}

///////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////

void    DrawBoxRect(VC_IMAGE *frameImage, RECT *rect)
{
    POINT startPt, endPt;
    int quadrantOffsetX = curAOI.startX;
    int quadrantOffsetY = curAOI.startY;

    startPt.x   = rect->top.x + quadrantOffsetX;
    startPt.y   = rect->top.y + quadrantOffsetY;
    endPt.x     = rect->bottom.x + quadrantOffsetX;
    endPt.y     = rect->top.y + quadrantOffsetY;
    DrawLine(frameImage, startPt, endPt); //Tjop  Line

    startPt.x   = rect->bottom.x + quadrantOffsetX;
    startPt.y   = rect->top.y + quadrantOffsetY;
    endPt.x     = rect->bottom.x + quadrantOffsetX;;
    endPt.y     = rect->bottom.y + quadrantOffsetY;
    DrawLine(frameImage, startPt, endPt);

    startPt.x   = rect->bottom.x + quadrantOffsetX;;
    startPt.y   = rect->bottom.y + quadrantOffsetY;
    endPt.x     = rect->top.x + quadrantOffsetX;
    endPt.y     = rect->bottom.y + quadrantOffsetY;
    DrawLine(frameImage, startPt, endPt);

    startPt.x   = rect->top.x + quadrantOffsetX;;
    startPt.y   = rect->bottom.y + quadrantOffsetY;
    endPt.x     = rect->top.x + quadrantOffsetX;;
    endPt.y     = rect->top.y + quadrantOffsetY;
    DrawLine(frameImage, startPt, endPt);
}

void    DrawLineBetweenOjbects(VC_IMAGE *frameImage, VC_FEATURES *ObjFeature1, VC_FEATURES *ObjFeature2)
{
    POINT startPt, endPt;
    int quadrantOffsetX = curAOI.startX;
    int quadrantOffsetY = curAOI.startY;

    startPt.x   = ObjFeature1->x_center + quadrantOffsetX;
    startPt.y   = ObjFeature1->y_center + quadrantOffsetY;
    endPt.x     = ObjFeature2->x_center + quadrantOffsetX;
    endPt.y     = ObjFeature2->y_center + quadrantOffsetY;
    DrawLine(frameImage, startPt, endPt); 
}

///////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////

void    DrawBoxPoint(VC_IMAGE *frameImage, POINT ptCenter)
{
    POINT startPt, endPt;

    startPt.x = ptCenter.x - POINT_BOX_SIZE;
    startPt.y = ptCenter.y - POINT_BOX_SIZE;
    endPt.x = ptCenter.x + POINT_BOX_SIZE;
    endPt.y = ptCenter.y - POINT_BOX_SIZE;
    DrawHorizontalLine(frameImage, startPt, endPt);

    startPt.x = ptCenter.x - POINT_BOX_SIZE;
    startPt.y = ptCenter.y + POINT_BOX_SIZE;
    endPt.x = ptCenter.x + POINT_BOX_SIZE;
    endPt.y = ptCenter.y + POINT_BOX_SIZE;
    DrawHorizontalLine(frameImage, startPt, endPt);

    startPt.x = ptCenter.x - POINT_BOX_SIZE;
    startPt.y = ptCenter.y - POINT_BOX_SIZE;
    endPt.x = ptCenter.x - POINT_BOX_SIZE;
    endPt.y = ptCenter.y + POINT_BOX_SIZE;
    DrawVerticallLine(frameImage, startPt, endPt);

    startPt.x = ptCenter.x + POINT_BOX_SIZE;
    startPt.y = ptCenter.y - POINT_BOX_SIZE;
    endPt.x = ptCenter.x + POINT_BOX_SIZE;
    endPt.y = ptCenter.y + POINT_BOX_SIZE;
    DrawVerticallLine(frameImage, startPt, endPt);
}

///////////////////////////////////////////////////////////////////
//
// Result = 1/100th of a pixel
//
///////////////////////////////////////////////////////////////////

POINT   GetCenterOfTwoObjects(VC_IMAGE *frameImage, VC_FEATURES *firstObj, VC_FEATURES *nextObj)
{
    POINT   centerPoint;

    centerPoint.x = (firstObj->x_center + nextObj->x_center) * SUB_PIXEL_SCALE;
    centerPoint.y = (firstObj->y_center + nextObj->y_center) * SUB_PIXEL_SCALE;

    centerPoint.x = centerPoint.x >> 1;
    centerPoint.y = centerPoint.y >> 1;

    return centerPoint;
}

//
// See
//  Mark Position Measurement Orientation
//

void     NormalizeToCameraCenter(VC_IMAGE *frameImage, BYTE markNumber)
{
    int tempInvert;

    // Inverted for Vertical FOV

    markRecognitionData[ markNumber ].markFovLateral  =  CENTER_OF_FOV_X - markRecognitionData[ markNumber ].markFovLateral;

    markRecognitionData[ markNumber ].markFovCircum   =  CENTER_OF_FOV_Y - markRecognitionData[ markNumber ].markFovCircum;

    // Swap

    tempInvert = markRecognitionData[ markNumber ].markFovLateral;

    markRecognitionData[ markNumber ].markFovLateral =
    markRecognitionData[ markNumber ].markFovCircum;

    markRecognitionData[ markNumber ].markFovCircum = tempInvert;
}

void     NormalizeToCameraCenterReg(VC_IMAGE *frameImage, BYTE markNumber)
{
    int tempInvert;

    // Inverted for Vertical FOV

    markRecognitionData[ markNumber ].markFovLateral  =  CENTER_OF_FOV_X - markRecognitionData[ markNumber ].markFovLateral;

    markRecognitionData[ markNumber ].markFovCircum   =  CENTER_OF_FOV_Y - markRecognitionData[ markNumber ].markFovCircum;

    // Swap

    if ( currentSystemConfiguration.cameraImaging.rotateImage )
    {
        markRecognitionData[ markNumber ].markFovLateral  *= -1;
        markRecognitionData[ markNumber ].markFovCircum  *= 1;
    }
    else
    {
        tempInvert = markRecognitionData[ markNumber ].markFovLateral;

        markRecognitionData[ markNumber ].markFovLateral =
        -markRecognitionData[ markNumber ].markFovCircum;

        markRecognitionData[ markNumber ].markFovCircum = -tempInvert;
    }
}



POINT    GetTopPointTwoObjects(VC_FEATURES *firstObj, VC_FEATURES *nextObj)
{
    POINT tempMinPoint;

    tempMinPoint.x = MIN(firstObj->x_min, nextObj->x_min);
    tempMinPoint.y = MIN(firstObj->y_min, nextObj->y_min);

    return tempMinPoint;
}

POINT    GetBotPointTwoObjects(VC_FEATURES *firstObj, VC_FEATURES *nextObj)
{
    POINT tempMaxPoint;

    tempMaxPoint.x = MAX(firstObj->x_max, nextObj->x_max);
    tempMaxPoint.y = MAX(firstObj->y_max, nextObj->y_max);

    return tempMaxPoint;
}


// TODO nominal white space validation size

void    DrawBoxTwoObjects(VC_IMAGE *frameImage, VC_FEATURES *firstObj, VC_FEATURES *nextObj)
{
    RECT markWhiteSpaceRect;

    int xMin, yMin, xMax, yMax;

    // Note of Vertical FOV

    xMin = MIN(firstObj->x_min, nextObj->x_min);
    yMin = MIN(firstObj->y_min, nextObj->y_min);
    xMax = MAX(firstObj->x_max, nextObj->x_max);
    yMax = MAX(firstObj->y_max, nextObj->y_max);

    markWhiteSpaceRect.top.x = xMin - RIBBON_NOMINAL_WHITE_SPACE_MARGIN;
    markWhiteSpaceRect.top.y = yMin - RIBBON_NOMINAL_WHITE_SPACE_MARGIN;
    markWhiteSpaceRect.bottom.x = xMax + RIBBON_NOMINAL_WHITE_SPACE_MARGIN;
    markWhiteSpaceRect.bottom.y = yMax + RIBBON_NOMINAL_WHITE_SPACE_MARGIN;

    DrawBoxRect(frameImage, &markWhiteSpaceRect);
}


void    AssignRectToFeature(VC_FEATURES *objFeatures, RECT *srcRect)
{
    objFeatures->x_min = srcRect->top.x;
    objFeatures->y_min = srcRect->top.y;

    objFeatures->x_max = srcRect->bottom.x;
    objFeatures->y_max = srcRect->bottom.y;

    objFeatures->x_center = srcRect->center.x;
    objFeatures->y_center = srcRect->center.y;
}
