




//
//
//
//  Author :    Paul Calinawan
//
//  Date:       June 22, 2006
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


#include "itechsys.h"


////////////////////////////////////////////////////////////////////


#include "kernel.h"


////////////////////////////////////////////////////////////////////
#include "imageacquirerhandler.h"

#include "imagetransmitter.h"

#include "tcpcomm.h"

#include "imaging.h"

#include "imageprocessinghandler.h"

#include "cameraconfig.h"

////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////

#define     MAKE_EVEN(x)                   ((x) & 0xFFFFFFFE)


// TODO

SCANNER_IMAGE_COLLECT_MODE      requestedScannerImageCollectMode,
currentScannerImageCollectMode;

//ITECH_TCP_MESSAGE  imageTransmissionMessage;
ITECH_IMAGE_MESSAGE  imageTransmissionMessage;

VC_IMAGE    txImageFrame;

WORD        txCurrImageFramePartitionId,
txTotalImageFramePartitions;

///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////
void                        
SetRequestedScannerImageCollectMode(SCANNER_IMAGE_COLLECT_MODE reqImageCollectMode)
{
    requestedScannerImageCollectMode = reqImageCollectMode; 
}

SCANNER_IMAGE_COLLECT_MODE  GetRequestedScannerImageCollectMode(void)
{
    return requestedScannerImageCollectMode;
}

void                        
SetCurrentScannerImageCollectMode(SCANNER_IMAGE_COLLECT_MODE currImageCollectMode)
{
    currentScannerImageCollectMode = currImageCollectMode;
}

SCANNER_IMAGE_COLLECT_MODE  GetCurrentScannerImageCollectMode(void)
{
    return currentScannerImageCollectMode;
}

void    SetTxImageFrame(VC_IMAGE iFrame)
{
    txImageFrame = iFrame;
}

POINT QuarterFrameTransmitLocation(int x, int y)
{
    POINT imageAOI;
    int currentX = 0;
    int currentY = 0;


    if ( GetDeviceType() == DEV_TYPE_REGISTER )
    {
        if ( currentSystemConfiguration.cameraImaging.rotateImage )
        {
           // currentX =  x;
            //currentY =  y;
			
	    currentX =  -x;
        currentY =  y;

        }
        else
        {
           // currentX  = y;
            //currentY  = x;
			
			currentX  = -y;
            currentY  = -x;

        }
    }
    else
    {
            currentX  = y; //For Ribbon
            currentY  = x;

    }

    currentX = CENTER_OF_FOV_X - currentX;
    currentY = CENTER_OF_FOV_Y - currentY;

    currentX /= SUB_PIXEL_SCALE;
    currentY /= SUB_PIXEL_SCALE;

    if ( currentSystemConfiguration.cameraImaging.rotateImage )
    {
        imageAOI.x = currentX - 120; 
        imageAOI.y = currentY - 160;

        if ( imageAOI.x > 399 )
            imageAOI.x = 399;

        if ( imageAOI.y > 159 )
            imageAOI.y = 159;
    }
    else
    {
        imageAOI.x = currentX - 160; 
        imageAOI.y = currentY - 120; 

        if ( imageAOI.x > 319 )
            imageAOI.x = 319;

        if ( imageAOI.y > 239 )
            imageAOI.y = 239;
    }

    if ( imageAOI.x < 0 )
        imageAOI.x = 0;

    if ( imageAOI.y < 0 )
        imageAOI.y = 0;

    imageAOI.x = MAKE_EVEN(imageAOI.x);
    imageAOI.y = MAKE_EVEN(imageAOI.y);

    return imageAOI;
}


BOOL TransmitImage( FRAME_BUFFER_IMAGE  *ptr)
{
    int     imageTransmitType;
    image   aOiImage;

    POINT   xyStart;
    POINT   xyArea;

    BYTE    refIndex = GetReferenceMarkIndex();


    imageTransmitType = GetRequestedScannerTransmitType();

    switch ( imageTransmitType )
    {
        case FULL_FRAME_COLOR:
            memcpy((void *)imageTXBuffer.frameImage.st,(void *)ptr->frameImage.st,((SizeOfScreen)+1024+BytesPerWord-1));
            VC_COPY_IMAGE(ptr->frameImage, imageTXBuffer.frameImage, 1, 1);
            break;

        case QUARTER_FRAME_COLOR:

            xyStart = QuarterFrameTransmitLocation(markRecognitionData[ refIndex ].markFovLateral, markRecognitionData[ refIndex ].markFovCircum);
            if ( currentSystemConfiguration.cameraImaging.rotateImage )
            {
                xyArea.x = 240;
                xyArea.y = 320;
            }
            else
            {
                xyArea.x = 320;
                xyArea.y = 240;
            }

            ImageAssign(&aOiImage, ptr->frameImage.st + (xyStart.y * ptr->frameImage.pitch) + xyStart.x, 
                        xyArea.x, xyArea.y, ptr->frameImage.pitch);    

            memcpy((void *)imageTXBuffer.frameImage.st,(void *)aOiImage.st,((SizeOfScreen)+1024+BytesPerWord-1));
            VC_COPY_IMAGE(aOiImage, imageTXBuffer.frameImage,1,1);

            break;

        case FULL_2X_SUB_BW:
            // memcpy((void *)imageTXBuffer.frameImage.st,(void *)fbiPtr->frameImage.st,((SizeOfScreen)+1024+BytesPerWord-1));
            VC_COPY_IMAGE(ptr->frameImage, imageTXBuffer.frameImage,
                          SUBSAMPLE_HORIZONTAL_RATIO_2X,SUBSAMPLE_VERTICALL_RATIO_2X);

            VC_SUBSAMPLE(&ptr->frameImage, &imageTXBuffer.frameImage,
                         SUBSAMPLE_HORIZONTAL_RATIO_2X, SUBSAMPLE_VERTICALL_RATIO_2X);
            break;

        case FULL_4X_SUB_BW:
            //  memcpy((void *)imageTXBuffer.frameImage.st,(void *)fbiPtr->frameImage.st,((SizeOfScreen)+1024+BytesPerWord-1));
            VC_COPY_IMAGE(ptr->frameImage, imageTXBuffer.frameImage,
                          SUBSAMPLE_HORIZONTAL_RATIO_4X, SUBSAMPLE_VERTICALL_RATIO_4X);

            VC_SUBSAMPLE(&ptr->frameImage, &imageTXBuffer.frameImage,
                         SUBSAMPLE_HORIZONTAL_RATIO_4X, SUBSAMPLE_VERTICALL_RATIO_4X);
            break;

        case FULL_8X_SUB_BW:
            // memcpy((void *)imageTXBuffer.frameImage.st,(void *)fbiPtr->frameImage.st,((SizeOfScreen)+1024+BytesPerWord-1));
            VC_COPY_IMAGE(ptr->frameImage, imageTXBuffer.frameImage,
                          SUBSAMPLE_HORIZONTAL_RATIO_8X, SUBSAMPLE_VERTICALL_RATIO_8X);
            VC_SUBSAMPLE(&ptr->frameImage, &imageTXBuffer.frameImage,
                         SUBSAMPLE_HORIZONTAL_RATIO_8X, SUBSAMPLE_VERTICALL_RATIO_8X);
            break;


        case QUARTER_2X_SUB_BW:
            //  memcpy((void *)imageTXBuffer.frameImage.st,(void *)aOiFullImage.st,((SizeOfScreen)+1024+BytesPerWord-1));
            ImageAssign(&aOiImage, ptr->frameImage.st + (120L * ptr->frameImage.pitch) + 160L, 
                        320, 240, ptr->frameImage.pitch);    

            VC_COPY_IMAGE(aOiImage, imageTXBuffer.frameImage,SUBSAMPLE_HORIZONTAL_RATIO_2X,SUBSAMPLE_VERTICALL_RATIO_2X);

            VC_SUBSAMPLE(&aOiImage, &imageTXBuffer.frameImage,
                         SUBSAMPLE_HORIZONTAL_RATIO_2X, SUBSAMPLE_VERTICALL_RATIO_2X);
            break;
        case QUARTER_4X_SUB_BW:
            // memcpy((void *)imageTXBuffer.frameImage.st,(void *)aOiFullImage.st,((SizeOfScreen)+1024+BytesPerWord-1));
            ImageAssign(&aOiImage, ptr->frameImage.st + (120L * ptr->frameImage.pitch) + 160L, 
                        320, 240, ptr->frameImage.pitch);    

            VC_COPY_IMAGE(aOiImage, imageTXBuffer.frameImage,SUBSAMPLE_HORIZONTAL_RATIO_4X,SUBSAMPLE_VERTICALL_RATIO_4X);

            VC_SUBSAMPLE(&aOiImage, &imageTXBuffer.frameImage,
                         SUBSAMPLE_HORIZONTAL_RATIO_4X, SUBSAMPLE_VERTICALL_RATIO_4X);

            break;

        case QUARTER_8X_SUB_BW:
            //  memcpy((void *)imageTXBuffer.frameImage.st,(void *)aOiFullImage.st,((SizeOfScreen)+1024+BytesPerWord-1));
            ImageAssign(&aOiImage, ptr->frameImage.st + (120L * ptr->frameImage.pitch) + 160L, 
                        320, 240, ptr->frameImage.pitch);    

            VC_COPY_IMAGE(aOiImage, imageTXBuffer.frameImage,SUBSAMPLE_HORIZONTAL_RATIO_8X,SUBSAMPLE_VERTICALL_RATIO_8X);

            VC_SUBSAMPLE(&aOiImage, &imageTXBuffer.frameImage,
                         SUBSAMPLE_HORIZONTAL_RATIO_8X, SUBSAMPLE_VERTICALL_RATIO_8X);
            break;

        case QUARTER_RLC:
            imageTXBuffer.frameImage.dx = rlcImageCopy.dx;
            imageTXBuffer.frameImage.dy = rlcImageCopy.dy;
            imageTXBuffer.frameImage.pitch = 640;
            rlcout(&imageTXBuffer.frameImage,&rlcImageCopy.rlcCopy[0],0,255);

            break;
        default:
            //printf("ImageTranmitter Type Error\n");
            return FAIL;
    }

    SetTxImageFrame(imageTXBuffer.frameImage);

    return PASS;
}


////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////

void    Init_ImageTransmitter(void)
{


}

////////////////////////////////////////////////////////////////////
//
// Exit Procedures
//
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//
// GoActive
//
////////////////////////////////////////////////////////////////////

// TODO


NEW_STATE   ITX_exitA(void)
{
    //print("ITX_A ACTIVE\n");

    memset(&imageTransmissionMessage, 0, sizeof(ITECH_IMAGE_MESSAGE)); 

    return ITX_ACTIVE;
}

//
// TODO fix server to properly do the packet class 
// design and handling
//

#define     IMAGE_PARAM_SIZE        (sizeof(DWORD)*5)

#define     MAX_IMAGE_DATA_SIZE     ((IMAGE_MAX_DATA) - (IMAGE_PARAM_SIZE + HEADER_OVER_HEAD))
//#define     MAX_IMAGE_DATA_SIZE     ((300) - IMAGE_PARAM_SIZE)

typedef struct imageData
{
    DWORD p1, p2, p3, p4, p5;

    BYTE imageArray[MAX_IMAGE_DATA_SIZE];

} IMAGE_DATA;

////////////////////////////////////////////////////////////////////
//
// StartImageTransmission
//      While in ITX_ACTIVE
//
////////////////////////////////////////////////////////////////////

IMAGE_DATA imgData;

int     remainingRowsToTransmit = 0;
int     currentRow = 0;
int     rowsPerPartition = 0;
int     pixelsInPartion;
int     totalRowsToTransmit;
//extern int     serverCommTxBufferAvailableCount;

extern FRAME_BUFFER_IMAGE      frameBufferImage[MAX_NUM_FRAME_BUFFERS];
U32                     lastFlashCount = 0;

NEW_STATE   ITX_exitB(void)
{
    U32     newestFlashCount;

    //print("ITX SENDING exit B\n");

    // TODO
    // ShowFrameImageAttributes(txImageFrame);

    if ( (GetRequestedScannerImageCollectMode() == SCANNER_IMAGE_COLLECT_ENABLED) )
    {
        if ( GetRequestedScannerTransmitType() == QUARTER_RLC )
            newestFlashCount    =   rlcImageCopy.latestFlashCount;
        else
            newestFlashCount    =   newestImage->flashCount;

        if ( (newestFlashCount <= lastFlashCount) )
        {
            return SAME_STATE;
        }

        if ( !TransmitImage(newestImage) )
        {
            return SAME_STATE;
        }

        lastFlashCount = newestImage->flashCount;
    }
    else
        return SAME_STATE;

    txCurrImageFramePartitionId = 1;
    txTotalImageFramePartitions = 0;
    currentRow = 0;

    rowsPerPartition = MAX_IMAGE_DATA_SIZE /  txImageFrame.dx;
    totalRowsToTransmit = txImageFrame.dy;
    remainingRowsToTransmit = txImageFrame.dy;
    txTotalImageFramePartitions = txImageFrame.dy / rowsPerPartition;

    if ( (txTotalImageFramePartitions * rowsPerPartition) < txImageFrame.dy ) //Round Up
        txTotalImageFramePartitions++;

    SendMessage(THIS_MACHINE, WaitForServerMsgSlotAvailable);
    return ITX_SENDING_IMAGE_PARTITION;
}
////////////////////////////////////////////////////////////////////
//
// NewImagePartitionReadyForTransfer
//      while in ITX_SENDING_IMAGE_PARTITION
//
////////////////////////////////////////////////////////////////////

NEW_STATE   ITX_exitC(void)
{
    InitBuildTcpPacket(&imageTransmissionMessage.header);
    SetTcpMessageId(&imageTransmissionMessage.header, ITECH_MSG_CurImageData);

    SetTcpParam1234(&imageTransmissionMessage.header, lastFlashCount, 
                    txCurrImageFramePartitionId, txTotalImageFramePartitions, 
                    0);

    // TODO
    imgData.p1 = lastFlashCount;
    imgData.p2 = txImageFrame.dx;
    imgData.p3 = txImageFrame.dy;
    imgData.p4 = txTotalImageFramePartitions;
    imgData.p5 = txCurrImageFramePartitionId;


    if ( remainingRowsToTransmit >= rowsPerPartition )
    {
        GetImageData(&txImageFrame, &imgData.imageArray[0], currentRow, rowsPerPartition);

        pixelsInPartion = rowsPerPartition * txImageFrame.dx;
        SetTcpData((ITECH_TCP_MESSAGE *)&imageTransmissionMessage, (BYTE *)&imgData, pixelsInPartion+IMAGE_PARAM_SIZE);

        remainingRowsToTransmit -= rowsPerPartition;
        currentRow += rowsPerPartition;
    }
    else
    {
        GetImageData(&txImageFrame, &imgData.imageArray[0], currentRow, remainingRowsToTransmit);

        pixelsInPartion = remainingRowsToTransmit * txImageFrame.dx;
        SetTcpData((ITECH_TCP_MESSAGE *) &imageTransmissionMessage, (BYTE *)&imgData, pixelsInPartion+IMAGE_PARAM_SIZE);

        remainingRowsToTransmit = 0;
    }

    txCurrImageFramePartitionId++;
    imageTransmissionMessage.imageTransmitReady = TRUE;

    StartTimer( MILLISECONDS( 1000 ));

    PostMessage(TransmitManager, CheckServerTransmitQueue);

    return SAME_STATE;
}

NEW_STATE   ITX_exitD(void)
{
//TX Failed

    CancelTimer();
    //LogString("Transmit Failed");
    if ( remainingRowsToTransmit )
    {
        SendMessage(THIS_MACHINE, WaitForServerMsgSlotAvailable);
        return SAME_STATE;
    }


    return ITX_ACTIVE;
}

NEW_STATE   ITX_exitE(void)
{
//TX Passed
    CancelTimer();

    if ( remainingRowsToTransmit )
    {
        SendMessage(THIS_MACHINE, WaitForServerMsgSlotAvailable);
        return SAME_STATE;
    }

    return ITX_ACTIVE;
}

NEW_STATE   ITX_exitF(void)
{
    //Image Transfer Timed out Move on to new image.

    //LogString("Image Transfer Timed Out");

    return ITX_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_ITX_IDLE)
EV_HANDLER(GoActive, ITX_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_ITX_ACTIVE)
EV_HANDLER(CheckForNewImage, ITX_exitB)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_ITX_SENDING_IMAGE_PARTITION)
EV_HANDLER(TimeOut, ITX_exitF),
EV_HANDLER(NewImagePartitionReadyForTransfer, ITX_exitC),
EV_HANDLER(WaitForServerMsgSlotAvailable, ITX_exitC),
EV_HANDLER(ImageTXfailed, ITX_exitD),
EV_HANDLER(ImageTXpassed, ITX_exitE),
EV_HANDLER(CheckForNewImage, DoNothing)

STATE_TRANSITION_MATRIX_END;


// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(ITX_Main_Entry)
STATE(_ITX_IDLE)                    ,   
STATE(_ITX_ACTIVE)                  ,
STATE(_ITX_SENDING_IMAGE_PARTITION) 
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////



