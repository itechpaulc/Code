




//
//
//
//  Author :    Paul Calinawan
//
//  Date:       October 2, 2006
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


#include "servermessageparser.h"

#include "tcpcomm.h"

#include "spicomm.h"

#include "transportmanager.h"

#include "flashpowercontroller.h"

#include "cameraconfig.h"

#include "scannermanager.h"

#include "imageprocessinghandler.h"

#include "imageacquirerhandler.h"


////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////

#define     TCP_MAX_RECEIVE_BUFFER_SIZE     (TCP_MAX_RECEIVE_BUFFER/4)

//#define     TRANPORT_STEP_SIZE      55718  //Old Transport
#define     TRANPORT_STEP_SIZE      70072  //Old Transport


unsigned int totalMsgData;

int     serverParserRxBuffer[TCP_MAX_RECEIVE_BUFFER_SIZE];
int     serverParserRxBufferLenght;

int     tempRxBuffer[TCP_MAX_RECEIVE_BUFFER_SIZE];
BYTE    *currTempRxBufferPtr;

ITECH_TCP_MESSAGE  tempItechTcpMessage;

BOOL    parserParsing;

BYTE    *currMsgBytePtr;
int     tokenMatchCount;

BYTE    *currItechTokenPtr;

int     byteToDataEndCount;
int     dataEndPosition;

int     byteToCheckSumEndCount;
int     checkSumEndPosition;

DWORD   dataLength;
DWORD   runningCheckSum;


BYTE    itechChar;
BYTE    msgChar;

extern RX_DATA rxData;

extern BOOL     topEdgeFirst; //PM don't check in
extern MULTICUT_VARS        multiCuts;
       BOOL     flipMark; //PM don't check in


DWORD tempVariable;

///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////
extern void ResetRXData();

///////////////////////////////////////////////////////////////////
//
// Helper Functions
//
////////////////////////////////////////////////////////////////////
void    RealigneRxData()
{
    unsigned int moveSize;

    //   printf("RealigneRxData1: %d %d\n",rxData.frontIndex, rxData.backIndex);
    moveSize = rxData.backIndex - rxData.frontIndex;

    if ( moveSize == 0 )
        ResetRXData();
    else
    {
        memcpy(&rxData.serverCommRxBuffer[0], &rxData.serverCommRxBuffer[rxData.frontIndex], moveSize);

        rxData.backIndex -= rxData.frontIndex;
        rxData.frontIndex = 0;
    }

    //  printf("RealigneRxData2: %d %d Move size: %d\n",rxData.frontIndex, rxData.backIndex, moveSize);
}
BOOL    IsServerMessageParserParsing(void)
{
    return parserParsing;
}

void    CopyMessageToParserBuffer(int *msgPtr, int length)
{
    memcpy(serverParserRxBuffer, msgPtr, length);

    serverParserRxBufferLenght = length;
}

void    ResetParserBuffers(void)
{
    parserParsing = FALSE;
    tokenMatchCount = 0;

    currItechTokenPtr = NULL;
    currMsgBytePtr = NULL;
    currTempRxBufferPtr = NULL;

    dataLength = 0x00000000;

    byteToDataEndCount = 0;
    dataEndPosition = 0;
    byteToCheckSumEndCount = 0; 
    checkSumEndPosition = 0;

    //checkSum = 0x00000000;
    runningCheckSum = 0x00000000;

    memset((BYTE *)tempRxBuffer, 0x00, TCP_MAX_RECEIVE_BUFFER_SIZE);

    InitBuildTcpPacket(&tempItechTcpMessage.header);
}

void DisplayCameraConfig()
{
    BOOL homeDir = FALSE;

    if ( currentSystemConfiguration.cameraImaging.homeDirection == MOVE_TO_RIGHT )
        homeDir = TRUE;

    print("\n\nCamConfig .......................\n");

    printf("\tCamType:%x RImg:%x RegMirror: %x\n",
           currentSystemConfiguration.camType,
           currentSystemConfiguration.cameraImaging.rotateImage,
           currentSystemConfiguration.cameraImaging.regMirror);

    printf("\tRevTran:%x MinWL:%x MaxBL : %x\n",
           currentSystemConfiguration.cameraImaging.reverseTransportCentering,
           currentSystemConfiguration.cameraImaging.minWhiteLevel,
           currentSystemConfiguration.cameraImaging.maxBlackLevel);

    printf("\tMinCont:%x sftDiv:%x BLKA:%x\n ",
           currentSystemConfiguration.cameraImaging.minContrast,
           currentSystemConfiguration.cameraImaging.shiftDivisor,
           currentSystemConfiguration.cameraImaging.blackLevelArea);

    printf("\tThrMul:%x incThr:%x ThrRetry : %x\n",
           currentSystemConfiguration.cameraImaging.thresholdMultiplier,
           currentSystemConfiguration.cameraImaging.incThresholdMultiplier,
           currentSystemConfiguration.cameraImaging.thresholdRetries);

    printf("\HomeDirRight:%x TopEdgeFirst:%x\n", homeDir, 
           topEdgeFirst);
}

void DisplayTransportConfig()
{
    printf("\n\nSearch Area: %d, %d\n", 
           GetCameraCircumAreaLimitedLengthSearch(),
           GetCameraLateralAreaLimitedLengthSearch()); 

    printf("  Step Size: %d, %d\n", 
           GetCameraSearchLateralStepSize(),
           GetCameraSearchCircumStepSize());

};
void    ShowTempRxBuffer(void)
{
//    int b, n=0;
//    BYTE *bPtr = (BYTE *)tempRxBuffer;
//
//    for ( b=0; b<TCP_MAX_RECEIVE_BUFFER_SIZE; b++ )
//    {
//        //print("Ox%x ", (BYTE)(*bPtr));
//        bPtr++;
//
//  //      if ( n++ == 16 )
//  //      {
//  //          print("\n");
//  //          n = 0;
//  //      }
//    }
//
//    print("\n\n");
}

////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////

void    Init_ServerMessageParser(void)
{
    SetDeviceType( DEV_TYPE_NONE );
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

NEW_STATE   SMP_exitA(void)
{
    //print("SMP ACTIVE\n");

    ResetParserBuffers();

    //StartTimer(SECONDS(5));
    return SMP_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// NewServerMessageDetected
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SMP_exitB(void)
{
    //print("NEW SERVER MESSAGE DETECTED\n");
//  print("SMP_exitB\n");

    if ( rxData.backIndex > HEADER_OVER_HEAD )
    {
        SendMessage(THIS_MACHINE, ParseSearchI);

        return SMP_SEARCHING_I;
    }

    return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
// ParseSearchI
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SMP_exitC1(void)
{
    msgChar = *currMsgBytePtr;
    itechChar = *currItechTokenPtr;

    //   print("SMP_exitC1\n");

    for ( rxData.frontIndex ; rxData.frontIndex < rxData.backIndex; rxData.frontIndex++ )
    {
        if ( rxData.serverCommRxBuffer[rxData.frontIndex] == 'I' )
        {
            SendMessage(THIS_MACHINE, ParseSearchTECH);

            return SMP_SEARCHING_TECH;
        }
    }

    //print("I MISSED\n");
    ResetRXData();

    return SMP_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// ParseSearchTECH
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SMP_exitC2(void)
{
    msgChar = *currMsgBytePtr;
    itechChar = *currItechTokenPtr;

//  print("SMP_exitC2\n");

    if ( (rxData.serverCommRxBuffer[rxData.frontIndex] != 'I')  ||
         (rxData.serverCommRxBuffer[rxData.frontIndex+1] != '-')  ||
         (rxData.serverCommRxBuffer[rxData.frontIndex+2] != 'T')  ||
         (rxData.serverCommRxBuffer[rxData.frontIndex+3] != 'E')  ||
         (rxData.serverCommRxBuffer[rxData.frontIndex+4] != 'C')  ||
         (rxData.serverCommRxBuffer[rxData.frontIndex+5] != 'H')  ||
         (rxData.serverCommRxBuffer[rxData.frontIndex+6] != '!')  ||
         (rxData.serverCommRxBuffer[rxData.frontIndex+7] != '!') )
    {
        rxData.frontIndex++;

        if ( rxData.frontIndex < rxData.backIndex )  //More Data To Process
        {
            SendMessage(THIS_MACHINE, ParseSearchI);
            return SMP_SEARCHING_I;
        }
        else
        {
            //print("I-TECH!! MISSED\n");
            ResetRXData();

            return SMP_ACTIVE;
        }
    }

    memcpy(&tempItechTcpMessage.header, &rxData.serverCommRxBuffer[ rxData.frontIndex], HEADER_OVER_HEAD);

    totalMsgData = HEADER_OVER_HEAD + tempItechTcpMessage.header.dataLen + sizeof(DWORD);

    if ( (rxData.frontIndex + totalMsgData) >= TCP_MAX_RECEIVE_BUFFER )
    {
        //RealigneRxData();
    }

    SendMessage(THIS_MACHINE, WaitForDataLengthPos);
    return SMP_WAIT_FOR_DATA_LENGTH_POS;
}


////////////////////////////////////////////////////////////////////
//
// WaitForDataLengthPos
//
////////////////////////////////////////////////////////////////////
#define NEW_DATA_IN_BUFF  (rxData.backIndex-rxData.frontIndex)
#define DATA_START   ( rxData.frontIndex + HEADER_OVER_HEAD )  

NEW_STATE   SMP_exitD(void)
{
//  print("SMP_exitD: %d\n", DATA_START);
    if ( totalMsgData <= NEW_DATA_IN_BUFF ) //Complete Msg RX
    {
        // Header Portion is complete

        memcpy(&tempItechTcpMessage.data[0], &rxData.serverCommRxBuffer[DATA_START],
               tempItechTcpMessage.header.dataLen);

        memcpy(&tempItechTcpMessage.checksum, &rxData.serverCommRxBuffer[HEADER_OVER_HEAD+tempItechTcpMessage.header.dataLen],
               sizeof(DWORD));

        SendMessage(THIS_MACHINE, WaitForCheckSum);

        return SMP_WAIT_FOR_CHECKSUM;
    }

    SendMessage(THIS_MACHINE, WaitForDataLengthPos);

    return SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
// WaitForDataLength
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SMP_exitE(void)
{
    return SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
// WaitForData
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SMP_exitF(void)
{
    return SAME_STATE;
}

////////////////////////////////////////////////////////////////////
//
// WaitForChecksum
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SMP_exitG(void)
{
    DWORD checksum, diff;
    int frontOffset = HEADER_OVER_HEAD+tempItechTcpMessage.header.dataLen+4;

    checksum    =   CalculateTcpChecksum(&tempItechTcpMessage);

    if ( checksum == tempItechTcpMessage.checksum )
    {
        rxData.frontIndex += frontOffset;

        RealigneRxData();
        SendMessage(THIS_MACHINE, ProcessServerMessage);

        return SMP_PROCESS_SERVER_MESSAGE;
    }

    //print("CheckSumFailed: %x != %x : ID: 0x%x Len:0x%x\n", checksum, tempItechTcpMessage.checksum,tempItechTcpMessage.header.messageId, tempItechTcpMessage.header.dataLen);

    rxData.frontIndex++;

    SendMessage(THIS_MACHINE, ParseSearchI);

    return SMP_SEARCHING_I;

}


////////////////////////////////////////////////////////////////////
//
// ProcessServerMessage
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SMP_exitH(void)
{
	FILE *fp;
    DWORD   msgId = tempItechTcpMessage.header.messageId;
    ITECH_TCP_MESSAGE replyMessage;
    Encoder         rxEncoderMsg;
    MarkSearch      rxMarkSearch;
    CameraDirection rxCamDirectionMsg; // Need to review definition
    CameraConfigMsg *rxCamConfig;
    int             x,y, index, *intPtr, ref, numberOfmarks;

    CameraImaging   *camImagingPtr  = 
    &GetCurrSystemConfig()->cameraImaging;

    // int x;
    DWORD encSim = 0;

    U32 data[3];


    //print("PROCESS SERVER MESSAGE\n");

    //  print("SMP_exitH\n");
    switch ( msgId )
    {
        case SetDeviceTypeConfig:
            data[0] = tempItechTcpMessage.header.param1;

            data[1] = GetDeviceType();
            if ( data[1] != data[0] )
            {
                if ( DEV_TYPE_RIBBON == data[0] )
                {
                    SetDeviceType( DEV_TYPE_RIBBON );
                    InitializePatternRecognitionDefaults();
                    Init_RibbonImageProcessingHandler();

                }
                else if ( DEV_TYPE_REGISTER == data[0] )
                {
                    SetDeviceType( DEV_TYPE_REGISTER );
                    InitializeRegPatternRecognitionDefaults();
                    Init_RegImageProcessingHandler();
                    print("SetDeviceTypeConfig Detected - DEV_TYPE_REGISTER\n");
                }
                else
                {
                    SetDeviceType( DEV_TYPE_NONE );
                    print("SetDeviceTypeConfig Detected - DEV_TYPE_NONE\n");
                    break;
                }
            }

            data[1] = GetDeviceType();

            InitBuildTcpPacket(&replyMessage.header);
            SetTcpParam12(&replyMessage.header, data[1],  tempItechTcpMessage.header.param1);
            SetTcpMessageId(&replyMessage.header, SetDeviceTypeConfigReply);
            PostServerItechMessage(&replyMessage);

            break;


        case ITECH_MSG_SetMarkDefinition:

            intPtr = (int *)&tempItechTcpMessage.data[0];

            ref = *intPtr++;
            SetReferenceMarkIndex( (BYTE) ref);

            index = 4; 
            y = sizeof(MARK_ELEMENT );

            //print("ITECH_MSG_SetMarkDefinition Detected - Ref = %d\n", ref);

            for ( x = 0; x < MAX_ELEMENTS; x++ )
            {
                memcpy(&camImagingPtr->elements[x], &tempItechTcpMessage.data[index], y);
                index += y;

                //              print("Elem[%d], MinArea:%d , NomArea:%d, MaxArea:%d\n", x
                //                    , camImagingPtr->elements[x].minAreaSize
                //                    , camImagingPtr->elements[x].nominalAreaSize
                //                    , camImagingPtr->elements[x].maxAreaSize);
                //
                //              print("      MinWidth:%d , MinHieght:%d, MinCompAxis:%d\n" 
                //                    , camImagingPtr->elements[x].minWidth
                //                    , camImagingPtr->elements[x].minHeight
                //                    , camImagingPtr->elements[x].minComplexAxis);
                //
                //              print("       MaxWidth:%d , MaxHieght:%d\n" 
                //                    , camImagingPtr->elements[x].maxWidth
                //                    , camImagingPtr->elements[x].maxHeight);
                //
                //              print("      WidthHgtPerRatioTol:%d , PerAreaPerRatioTol:%d, MinFound:%d\n" 
                //                    , camImagingPtr->elements[x].widthHeightPercentRatioTolerance
                //                    , camImagingPtr->elements[x].perimeterAreaPercentRatioTolerance
                //                    , camImagingPtr->elements[x].minFound);

            }

            y = sizeof( MARK_DEFINITION );

            numberOfmarks = 0;

            for ( x = 0; x < MAX_MARKS; x++ )
            {
                memcpy(&camImagingPtr->markDefinition[x], &tempItechTcpMessage.data[index], y);
                index += y;

                if ( camImagingPtr->markDefinition[x].enable )
                    numberOfmarks++;

                //               print("MarkDef[%d], Enable:%d , Color:%d, NumberOfPairs:%d\n", x
                //                     , camImagingPtr->markDefinition[x].enable
                //                     , camImagingPtr->markDefinition[x].color
                //                     , camImagingPtr->markDefinition[x].numPairsToSearch);
                //
                //               print(" Element Pair[ 0 ]   E1:%d X:%d Y:%d E2%d PerTol:%d\n" 
                //                     , camImagingPtr->markDefinition[x].elementPairs[0].thisElementType
                //                     , camImagingPtr->markDefinition[x].elementPairs[0].nominalHorizontalDistToPartner
                //                     , camImagingPtr->markDefinition[x].elementPairs[0].nominalVerticalDistToPartner
                //                     , camImagingPtr->markDefinition[x].elementPairs[0].partnerElementType
                //                     , camImagingPtr->markDefinition[x].elementPairs[0].perimeterToleranceCenterOfPartner);
                //
                //               print(" Element Pair[ 1 ]   E1:%d X:%d Y:%d E2%d PerTol:%d\n" 
                //                     , camImagingPtr->markDefinition[x].elementPairs[1].thisElementType
                //                     , camImagingPtr->markDefinition[x].elementPairs[1].nominalHorizontalDistToPartner
                //                     , camImagingPtr->markDefinition[x].elementPairs[1].nominalVerticalDistToPartner
                //                     , camImagingPtr->markDefinition[x].elementPairs[1].partnerElementType
                //                     , camImagingPtr->markDefinition[x].elementPairs[1].perimeterToleranceCenterOfPartner);
                //
                //               print(" Element Pair[ 2 ]   E1:%d X:%d Y:%d E2%d PerTol:%d\n\n\n" 
                //                     , camImagingPtr->markDefinition[x].elementPairs[2].thisElementType
                //                     , camImagingPtr->markDefinition[x].elementPairs[2].nominalHorizontalDistToPartner
                //                     , camImagingPtr->markDefinition[x].elementPairs[2].nominalVerticalDistToPartner
                //                     , camImagingPtr->markDefinition[x].elementPairs[2].partnerElementType
                //                     , camImagingPtr->markDefinition[x].elementPairs[2].perimeterToleranceCenterOfPartner);
            }

            if ( GetDeviceType() == DEV_TYPE_REGISTER )
            {
                //Define Complex Element
                camImagingPtr->elements[1].minAreaSize =   camImagingPtr->elements[1].nominalAreaSize;
                camImagingPtr->elements[1].nominalAreaSize = camImagingPtr->elements[1].maxAreaSize;
                camImagingPtr->elements[1].maxAreaSize = camImagingPtr->elements[1].maxAreaSize * numberOfmarks; //Single * Active Elments
                camImagingPtr->elements[1].minWidth = (camImagingPtr->elements[1].maxWidth +  camImagingPtr->elements[1].minWidth)/2;
                camImagingPtr->elements[1].minHeight = (camImagingPtr->elements[1].maxHeight +  camImagingPtr->elements[1].minHeight)/2;
                camImagingPtr->elements[1].maxWidth = camImagingPtr->elements[1].maxWidth * numberOfmarks;
                camImagingPtr->elements[1].maxHeight = camImagingPtr->elements[1].maxHeight * numberOfmarks;
                camImagingPtr->elements[1].widthHeightPercentRatioTolerance = -1;  //Not Used
                camImagingPtr->elements[1].perimeterAreaPercentRatioTolerance = -1;//Not Used
                camImagingPtr->elements[1].minComplexAxis = camImagingPtr->elements[0].minComplexAxis;
            }

            InitBuildTcpPacket(&replyMessage.header);
            SetTcpMessageId(&replyMessage.header, ITECH_MSG_MarkDefinitionReply);
            PostServerItechMessage(&replyMessage);

            break;
        case ITECH_MSG_CameraSoftReset:
            //print("ITECH_MSG_CameraSoftReset Detected\n");

            //print("Packing Flash");
            exec("fd:/shell.exe",1,"pk");
            //print("Done Packing");

            reset();
            break;
        case ITECH_MSG_CameraHardReset:
            //   print("ITECH_MSG_CameraHardReset Detected\n");

            //print("Packing Flash");
            exec("fd:/shell.exe",1,"pk");
            //print("Done Packing");

            SendMessageAndData(WatchDog_Manager,ResetWDT, FALSE, 0);
            break;

        case ITECH_MSG_SetMarkEnable:
            print("ITECH_MSG_SetMarkEnable Detected!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

            intPtr = (int *)&tempItechTcpMessage.data[0];

            ref = *intPtr++;
            numberOfmarks = *intPtr++;

            print("Reference: %d #ofMarks:%d\n",ref,numberOfmarks);

            if ( (ref < MAX_MARKS) && (numberOfmarks < MAX_MARKS) )
            {
                SetReferenceMarkIndex( (BYTE) ref);

                for ( x = 0; x < MAX_MARKS ; x++ )
                    camImagingPtr->markDefinition[ x ].enable = FALSE;

                for ( x = 0; x < numberOfmarks; x++ )
                {
                    camImagingPtr->markDefinition[ *intPtr ].enable = TRUE;
                    //      printf("Mark[%d]: %d ",*intPtr,camImagingPtr->markDefinition[ *intPtr ].enable);
                    *intPtr++;
                }
                //   printf("\n");
            }

            InitBuildTcpPacket(&replyMessage.header);
            SetTcpMessageId(&replyMessage.header, ITECH_MSG_MarkEnableConfigReply);
            PostServerItechMessage(&replyMessage);
            break;

        case ITECH_MSG_SetCamConfig:

			fp = io_fopen("fd:/notran.001","r");

			if ( fp )
			{
				flipMark = 1;
				io_fclose(fp);
	
				printf("Flipping Mark\n");
			}
			else
			{
				flipMark = 0;
				printf("Not-Flipping Mark\n");

			}

            rxCamConfig = ( CameraConfigMsg *) &tempItechTcpMessage.data[0];

            currentSystemConfiguration.camType                      = (BYTE) rxCamConfig->camType;
            currentSystemConfiguration.cameraImaging.rotateImage    = (BOOL) rxCamConfig->rotateImage;
            currentSystemConfiguration.cameraImaging.regMirror      = (BOOL) rxCamConfig->regMirror;
            currentSystemConfiguration.cameraImaging.reverseTransportCentering  = (BOOL) rxCamConfig->reverseTransportCentering;
            currentSystemConfiguration.cameraImaging.minWhiteLevel      = (int) rxCamConfig->minWhiteLevel;
            currentSystemConfiguration.cameraImaging.maxBlackLevel      = (int) rxCamConfig->maxBlackLevel;
            currentSystemConfiguration.cameraImaging.minContrast        = (int) rxCamConfig->minContrast;
            currentSystemConfiguration.cameraImaging.shiftDivisor       = (BYTE) rxCamConfig->shiftDivisor;
            currentSystemConfiguration.cameraImaging.blackLevelArea       = (int) rxCamConfig->blackLevelArea;
            currentSystemConfiguration.cameraImaging.thresholdMultiplier        = (BYTE) rxCamConfig->thresholdMultiplier;
            currentSystemConfiguration.cameraImaging.incThresholdMultiplier         = (BYTE) rxCamConfig->incThresholdMultiplier;
            currentSystemConfiguration.cameraImaging.thresholdRetries       = (BYTE) rxCamConfig->thresholdRetries;

            if ( rxCamConfig->right2Home )
                currentSystemConfiguration.cameraImaging.homeDirection          = MOVE_TO_RIGHT;
            else
                currentSystemConfiguration.cameraImaging.homeDirection          = MOVE_TO_LEFT;

            InitBuildTcpPacket(&replyMessage.header);
            SetTcpMessageId(&replyMessage.header, ITECH_MSG_CamConfigReply);

            PostServerItechMessage(&replyMessage);

            break;

        case ITECH_MSG_SetCameraDirectionConfig:

            memcpy(&rxCamDirectionMsg,(BYTE *)&tempItechTcpMessage.data, sizeof(CameraDirection));

            topEdgeFirst = rxCamDirectionMsg.circDirection; 

            //  LogString(logArray);

            InitBuildTcpPacket(&replyMessage.header);
            SetTcpMessageId(&replyMessage.header, ITECH_MSG_CameraDirectionConfigReplied);

            PostServerItechMessage(&replyMessage);
            break;

        case ITECH_MSG_SetSearchConfig:

            print("ITECH_MSG_SetSearchConfig Detected\n");
            memcpy(&rxMarkSearch,(BYTE *)&tempItechTcpMessage.data, sizeof(MarkSearch));

//             sprint(logArray,"SerachConfig Msg: Area:%d,%d Step:%d,%d",
//                    rxMarkSearch.limLateralSearchRange,
//                    rxMarkSearch.limCircumSearchRange,
//                    rxMarkSearch.limLateralSearchStepSize,
//                    rxMarkSearch.limCircumSearchStepSize);

            

            SetCameraCircumAreaLimitedLengthSearch( rxMarkSearch.limCircumSearchRange ); 

            data[0] = currentSystemConfiguration.transport.micronsPerStep; //Temp Holder

            SetCameraLateralAreaLimitedLengthSearch( (rxMarkSearch.limLateralSearchRange * 1000)/data[0]  );//Transport Step Size 55.718um

            SetCameraSearchLateralStepSize( (rxMarkSearch.limLateralSearchStepSize * 1000)/data[0]);

            SetCameraSearchCircumStepSize(rxMarkSearch.limCircumSearchStepSize);  // 1/3 overlap

//             sprint(logArray,"SerachConfig Msg: Area:%d,%d StepSize:%d,%d - Lat in Steps:%d,%d ",
//                    rxMarkSearch.limLateralSearchRange,
//                    rxMarkSearch.limCircumSearchRange,
//                    rxMarkSearch.limLateralSearchStepSize,
//                    rxMarkSearch.limCircumSearchStepSize,
//                    GetCameraLateralAreaLimitedLengthSearch(),
//                    GetCameraSearchLateralStepSize() );
//
//             LogString(logArray);

            InitBuildTcpPacket(&replyMessage.header);
            SetTcpMessageId(&replyMessage.header, ITECH_MSG_SearchConfigReply);

            PostServerItechMessage(&replyMessage);
            break;

        case ITECH_MSG_SetTransportConfig:

            print("ITECH_MSG_SetTransportConfig Detected\n");
            memcpy(&currentSystemConfiguration.transport, (BYTE *)&tempItechTcpMessage.data, sizeof(Transport));

             printf("XPORT Msg: %d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                   currentSystemConfiguration.transport.enabled,
                   currentSystemConfiguration.transport.stepMotorDirection,
                   currentSystemConfiguration.transport.homeDirection,
                   currentSystemConfiguration.transport.distanceFromHome,
                   currentSystemConfiguration.transport.transportLength,
                   currentSystemConfiguration.transport.micronsPerStep,
                   currentSystemConfiguration.transport.startSpeed,
                   currentSystemConfiguration.transport.maxSpeed,
                   currentSystemConfiguration.transport.maxAcceleration);

            InitBuildTcpPacket(&replyMessage.header);
            SetTcpMessageId(&replyMessage.header, ITECH_MSG_TransportConfigReply);

            PostServerItechMessage(&replyMessage);
            break;

        case ITECH_MSG_SetEncoderConfig:

            //print("ITECH_MSG_SetEncoderConfig Detected\n");
            memcpy(&rxEncoderMsg,(BYTE *)&tempItechTcpMessage.data, sizeof(Encoder));
            // printf("----TwoAround: %d \n", rxEncoderMsg.twiceAroundOption);
            // printf("----Cutoff: %d PulsesPerRev: %d\n", rxEncoderMsg.cutOffLength, rxEncoderMsg.aPulsesPerImpression);

            InitBuildTcpPacket(&replyMessage.header);
            SetTcpMessageId(&replyMessage.header, ITECH_MSG_EncoderConfigReply);

            PostServerItechMessage(&replyMessage);

            SetCameraSearchCutoffLength(rxEncoderMsg.cutOffLength);
            SetCutOffLength(&currentSystemConfiguration.encoder, rxEncoderMsg.cutOffLength);
            SetApulsesPerImpression(&currentSystemConfiguration.encoder, rxEncoderMsg.aPulsesPerImpression );

            if ( rxEncoderMsg.simulateZpulse )//Need Special FLag
                WriteSPIDataWithRetries( SET_ENC_SIM_Z_PULSE_TGT, rxEncoderMsg.aPulsesPerImpression - 1 );//Because Count Starts at zero.
            else
                WriteSPIDataWithRetries( SET_ENC_SIM_Z_PULSE_TGT, 0xFFFF );//Effectively Disable.

            multiCuts.numberOfCuts = rxEncoderMsg.numberOfCuts;

            printf(" Encoder PPR: %d CutOff: %d\n", rxEncoderMsg.aPulsesPerImpression, rxEncoderMsg.cutOffLength);
            printf(" Encoder Z pulse Sim: %d NumberOfcuts: %d\n", rxEncoderMsg.simulateZpulse, rxEncoderMsg.numberOfCuts);
            break;

        case ITECH_MSG_GetVersion:
            //print("ITECH_MSG_GetVersion Detected\n");
            SendMessage(SystemHardwareMonitor, GetVersion);
            deviceRebooted = FALSE;
            break;

        case ITECH_MSG_SetScannerMode:
            //  print("ITECH_MSG_SetScannerMode Detected :%d\n", tempItechTcpMessage.header.param1);
            SendMessageAndData(RibbonCameraManager, SetScannerMode, 
                               tempItechTcpMessage.header.param1, NO_DATA);
            break;

        case ITECH_MSG_SetImageCollect:
            //  print("ITECH_MSG_SetImageCollect Detected :%d\n", tempItechTcpMessage.header.param1);
            SendMessageAndData(RibbonCameraManager, SetImageCollectMode, 
                               tempItechTcpMessage.header.param1, NO_DATA);
            break;

            // TODO
        case ITECH_MSG_SetXYPosition:
            //   print("ITECH_MSG_SetXYPosition Detected X:%d Y:%d\n", 
            //        tempItechTcpMessage.header.param1, tempItechTcpMessage.header.param2);
            SendMessageAndData(RibbonCameraManager, SetXYPositions, 
                               tempItechTcpMessage.header.param1, tempItechTcpMessage.header.param2);


            if ( GetRequestedScannerScanMode() != SCANNER_DISABLED )
                SendMessage(ScannerManager, ScannerModeChangeRequested);

            break;

        case ITECH_MSG_SetImageTransmitType:
            //  print("ITECH_MSG_SetImageTransmitType Detected ITYPE:%d \n", 
            //         tempItechTcpMessage.header.param1);
            SendMessageAndData(RibbonCameraManager, SetImageTransmitType, 
                               tempItechTcpMessage.header.param1, NO_DATA);
            break;

        case ITECH_MSG_SetImageAnalysis:
            SendMessageAndData(RibbonCameraManager, SetImageAnalysis, 
                               tempItechTcpMessage.header.param1, tempItechTcpMessage.header.param2);

            if ( tempItechTcpMessage.header.param2 == SIMULATION_RUN )
            {
                encSim = 0x80000000;
                encSim |= tempItechTcpMessage.header.param3;
            }
            else
                encSim = 0;

            SetLabelObjects( (BOOL) tempItechTcpMessage.header.param4 );

            data[0] = GetApulsesPerImpression( &currentSystemConfiguration.encoder );

            WriteSPIDataWithRetries( SET_ENC_SIM_PULSE_COUNT , data[0] - 1 );
            WriteSPIDataWithRetries( SET_ENCODER_SIM, encSim );

            //     print("ITECH_MSG_SetImageAnalysis Detected ANALYSIS:%d  SIM:%d, Period: 0x%x \n        ObjectLabel: %d\n", 
            //           tempItechTcpMessage.header.param1, tempItechTcpMessage.header.param2, encSim,tempItechTcpMessage.header.param4);

            break;

        case ITECH_MSG_SetImageCaptureConfig:  // Last Init config message

            memcpy((BYTE *)&data[0],(BYTE *)&tempItechTcpMessage.data, 12);

            //    print("ITECH_MSG_SetImageCaptureConfig %d %d %d\n",data[0],data[1],data[2]);

            data[0] *= 2;
            data[1] *= 2;
            data[2] *= 2;

            InitBuildTcpPacket(&replyMessage.header);
            SetTcpMessageId(&replyMessage.header, ITECH_MSG_ImageCaptureConfigReply);
            SetTcpData(&replyMessage, (BYTE *)&data[0], tempItechTcpMessage.header.dataLen);

            PostServerItechMessage(&replyMessage);

            SendMessage(ScannerManager, GoActive);

            break;

        case ITECH_MSG_CurImageDataReply:
            //    print("ITECH_MSG_CurImageDataReply:\n",replyMessage.header.sequence);
            ShowCurrentTime();
            SendMessageAndData(TransmitManager, AckReceived,tempItechTcpMessage.header.param1,tempItechTcpMessage.header.param2); 

            break;

        case ITECH_MSG_GetCamSysStatus:
            CancelTimer();
            StartTimer(SECONDS(5));
            //     print("ITECH_MSG_GetCamSysStatus Detected: %d Seq: %d\n", 
            //                tempItechTcpMessage.header.param1, tempItechTcpMessage.header.sequence);
            SendMessageAndData(SystemHardwareMonitor, GetCamSysStatus,tempItechTcpMessage.header.sequence,0);

            break;

        case ITECH_MSG_SetDiagnosticLevel:
            break;

        case ITECH_MSG_GetSystemStates:
            break;

        case ITECH_MSG_SetImpressionCount:
            data[0] = tempItechTcpMessage.header.param1;
            //   print("ITECH_MSG_SetImpressionCount : %d \n",data[0]);
            WriteSPIDataWithRetries( SET_IMPRESSION_COUNT, data[0] );

            InitBuildTcpPacket(&replyMessage.header);
            SetTcpMessageId(&replyMessage.header, ITECH_MSG_ImpressionCountReply);
            SetTcpParam1(&replyMessage.header, data[0]);
            PostServerItechMessage(&replyMessage);

            //           SetFlashPowerLevelControl( data[0] );
//          GetSPIDataWithRetries(GET_IMPRESSION_COUNT, &data[1] );
            break;

        default:
            ; //print("UNKNOWN MESSAGE ID %x\n", msgId);
    }

    //print("RESET FOR NEXT SERVER MESSAGE\n");

    // ResetParserBuffers();

    if ( rxData.backIndex != 0 )
    {
        SendMessage(THIS_MACHINE, ParseSearchI);
        return SMP_SEARCHING_I;
    }

    return SMP_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// TimeOUt
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SMP_exitT1(void)
{
    //print("SMP - Timed Out\n");
    SendMessageAndData(RibbonCameraManager, SetScannerMode, 
                       1, NO_DATA);

    SendMessage(ServerMessageParser, GoActive);

    return SMP_ACTIVE;
}

////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_SMP_IDLE)
EV_HANDLER(GoActive, SMP_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SMP_ACTIVE)
EV_HANDLER(NewServerMessageDetected, SMP_exitB),
EV_HANDLER(TimeOut, SMP_exitT1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SMP_SEARCHING_I)
EV_HANDLER(ParseSearchI, SMP_exitC1)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SMP_SEARCHING_TECH)
EV_HANDLER(ParseSearchTECH, SMP_exitC2)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SMP_WAIT_FOR_DATA_LENGTH_POS)
EV_HANDLER(WaitForDataLengthPos, SMP_exitD)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SMP_WAIT_FOR_DATA_LENGTH)
EV_HANDLER(WaitForDataLength, SMP_exitE)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SMP_WAIT_FOR_DATA)
EV_HANDLER(WaitForData, SMP_exitF)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SMP_WAIT_FOR_CHECKSUM)
EV_HANDLER(WaitForCheckSum, SMP_exitG)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SMP_PROCESS_SERVER_MESSAGE)
EV_HANDLER(ProcessServerMessage, SMP_exitH)
STATE_TRANSITION_MATRIX_END;


// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(SMP_Main_Entry)
STATE(_SMP_IDLE)                        ,           
STATE(_SMP_ACTIVE)                      ,
STATE(_SMP_SEARCHING_I)                 ,
STATE(_SMP_SEARCHING_TECH)              ,
STATE(_SMP_WAIT_FOR_DATA_LENGTH_POS)    ,      
STATE(_SMP_WAIT_FOR_DATA_LENGTH)        ,
STATE(_SMP_WAIT_FOR_DATA)               ,
STATE(_SMP_WAIT_FOR_CHECKSUM)           ,
STATE(_SMP_PROCESS_SERVER_MESSAGE)      
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////





