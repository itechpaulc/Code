




//
//
//
//  Author :    Paul Calinawan
//
//  Date:       June 27, 2006
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
#include "spicomm.h"



////////////////////////////////////////////////////////////////////

#include "transmitManager.h"


////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////

ITECH_TCP_MESSAGE    *startServerCommTxBuffer;
ITECH_TCP_MESSAGE    *currServerCommTxBuffer;
ITECH_TCP_MESSAGE    *nextServerCommTxBuffer;
ITECH_TCP_MESSAGE    *lastServerCommTxBuffer;

int     serverCommTxBufferAvailableCount;
DWORD   TXsequence = 0;

BYTE    imageTXretries;

extern DWORD           serverConnectionSocket;
extern SOCKET_ADDRESS  localSocketAddr, remoteHostSocketAddr;
extern ITECH_IMAGE_MESSAGE  imageTransmissionMessage;

///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////

#define SERVER_TX_BUFFER_SIZE           (sizeof(ITECH_TCP_MESSAGE))

#define SERVER_TX_FULL_BUFFER_SIZE      (SERVER_TX_BUFFER_SIZE * MAX_SERVER_TX_QUEUE_COUNT)

void    InitServerCommTxBuffers(void)
{
    ADDRESS serverCommTxBufferPtr;

    serverCommTxBufferPtr =
    VC_MALLOC_DRAM_BYTE(SERVER_TX_FULL_BUFFER_SIZE);

    if ( serverCommTxBufferPtr < 0L )
    {
        startServerCommTxBuffer     =
        currServerCommTxBuffer      = 
        nextServerCommTxBuffer      = 
        lastServerCommTxBuffer      = NULL;

        serverCommTxBufferAvailableCount = 0;

        //print("ALLOCATION ERROR !!! InitServerCommTxBuffers\n");
    }
    else
    {
        //print("ALLOCATION PASS InitServerCommTxBuffers\n");

        startServerCommTxBuffer =
        currServerCommTxBuffer  = 
        nextServerCommTxBuffer  = 
        (ITECH_TCP_MESSAGE *)serverCommTxBufferPtr;

        serverCommTxBufferAvailableCount = MAX_SERVER_TX_QUEUE_COUNT;

        lastServerCommTxBuffer = 
        currServerCommTxBuffer + MAX_SERVER_TX_QUEUE_COUNT;

        memset(startServerCommTxBuffer, 0x00, SERVER_TX_FULL_BUFFER_SIZE);

        //print("START ADDR OK 0x%lx\n", (ADDRESS)startServerCommTxBuffer);
      //  print("CURR ADDR OK  0x%lx\n", (ADDRESS)currServerCommTxBuffer);
      //  print("LAST ADDR OK  0x%lx\n", (ADDRESS)lastServerCommTxBuffer);
      //  print("SERVER_TX_FULL_BUFFER_SIZE 0x%lx\n", SERVER_TX_FULL_BUFFER_SIZE);
      //  print("SERVER_TX_BUFFER_SIZE 0x%lx\n", SERVER_TX_BUFFER_SIZE);
    }
}

void    ShowCommTxBuffer(void)
{
    int     b;

    ITECH_TCP_MESSAGE *itechMsg;

    //print("BUFFER AVAIL COUNT %d\n", serverCommTxBufferAvailableCount);

    for ( b=0; b<MAX_SERVER_TX_QUEUE_COUNT; b++ )
    {
        itechMsg = (ITECH_TCP_MESSAGE *)(startServerCommTxBuffer + b);

        ShowTcpMessage(itechMsg);
    }
}

BOOL    PostServerItechMessage(ITECH_TCP_MESSAGE *itechTcpMessage)
{
    if ( serverCommTxBufferAvailableCount > 0 )
    {
        memcpy(nextServerCommTxBuffer, itechTcpMessage, SERVER_TX_BUFFER_SIZE);   

        serverCommTxBufferAvailableCount--;

        nextServerCommTxBuffer++;

        if ( nextServerCommTxBuffer == lastServerCommTxBuffer )
        {
            nextServerCommTxBuffer = startServerCommTxBuffer;
        }

        PostMessage(TransmitManager, CheckServerTransmitQueue);

        return TRUE;
    }

    //print("ITECH MESSAGE POST ERROR\n");

    return FALSE;
}

ITECH_TCP_MESSAGE * GetNextItechMessage(void)
{
    ITECH_TCP_MESSAGE *itechMsg;

    itechMsg = currServerCommTxBuffer;

    //print("GET 0x%p\n", itechMsg);

    serverCommTxBufferAvailableCount++;

    currServerCommTxBuffer++;

    if ( currServerCommTxBuffer == lastServerCommTxBuffer )
    {
        currServerCommTxBuffer = startServerCommTxBuffer;
    }

    return itechMsg;
}

BOOL     IsServerMessageSlotAvailable(void)
{
    if ( serverCommTxBufferAvailableCount > 0 )
        return TRUE;

    return FALSE;
}

BOOL    IsMessagePosted(void)
{
    if ( serverCommTxBufferAvailableCount < MAX_SERVER_TX_QUEUE_COUNT )
        return TRUE;

    return FALSE;
}

BYTE    tcpTransmitBuffer[TCP_MAX_TRANSMIT_BUFFER];

BYTE    SendTcpMessage(void  *msgPtr, BOOL imageMsgType)
{
    DWORD   connectionResult;
    ITECH_IMAGE_MESSAGE         *imageMsg;
    ITECH_TCP_MESSAGE           *tcpMessage;
    ITECH_TCP_HEADER_MESSAGE    *tcpMsgHeader;
    BYTE                        *data;
    DWORD                       *checkSum;
    int                         dataLength;

    static DWORD lastSeq = 0; 


    int     nextTransmitBufferPosition = 0;

    int headerMessageSize = sizeof(ITECH_TCP_HEADER_MESSAGE);
    int checksumSize = sizeof(tcpMessage->checksum);

    if ( imageMsgType )
    {
        imageMsg      = msgPtr;
//      print("Packet Data: %x\n",  imageMsg->header.param4);
        tcpMsgHeader    = &imageMsg->header;
        data            = &imageMsg->data[0];
        dataLength      = imageMsg->header.dataLen;
        checkSum        = &imageMsg->checksum;
        //      imageTransmissionMessage.imageTransmitReady = FALSE;
    }
    else
    {
        tcpMessage      = msgPtr;
        tcpMsgHeader    = &tcpMessage->header;
        data            = &tcpMessage->data[0];
        dataLength      = tcpMessage->header.dataLen;
        checkSum        = &tcpMessage->checksum;
    }

   // if ( (lastSeq+1) != (tcpMsgHeader->sequence) )
   //     LogString("Missed sequence");

    lastSeq = tcpMsgHeader->sequence;

    memcpy(tcpTransmitBuffer, tcpMsgHeader, headerMessageSize);
    nextTransmitBufferPosition += headerMessageSize;

    memcpy(tcpTransmitBuffer + nextTransmitBufferPosition,
           data, dataLength);
    nextTransmitBufferPosition += tcpMessage->header.dataLen;

    memcpy(tcpTransmitBuffer + nextTransmitBufferPosition,
           checkSum, checksumSize);
    nextTransmitBufferPosition += checksumSize;

    // TODO, get the return of send in case a send
    // completion state needs to be done

    //print("SRM SENDING MESSAGE\n");

    connectionResult = 0;

    if ( remoteHostSocketAddr.sin_port != 0 ) //PMDebug need to check address
    {
        connectionResult = sendto(serverConnectionSocket, (char *)tcpTransmitBuffer, nextTransmitBufferPosition, 0, &remoteHostSocketAddr, sizeof(localSocketAddr));
    }
   // else
        //print("Invalid Address: %lx\n", remoteHostSocketAddr.sin_addr.s_addr);

    if ( connectionResult == VCRT_ERROR )
    {
        connectionResult = shutdown(serverConnectionSocket, FLAG_ABORT_CONNECTION);
    //    print("Send shutdown error %d, %lx\n", connectionResult, remoteHostSocketAddr.sin_addr.s_addr);
        return FAIL;
    }

    SPI_ISSUE_TX_ONE_SHOT(); 
    return PASS;
}

// BYTE    SendTcpMessage(void  *tcpMessage)
// {
//     DWORD   connectionResult;
//
//     ITECH_TCP_HEADER_MESSAGE    *messageHeader ;
//     BYTE                        *msgData;
//     DWORD                       *chkSum;
//
//     ITECH_IMAGE_MESSAGE         *imagePtr;
//     ITECH_TCP_MESSAGE           *msgPtr;
//
//     int     nextTransmitBufferPosition = 0;
//
//     int headerMessageSize = sizeof(ITECH_TCP_HEADER_MESSAGE);
//     int checksumSize = sizeof(DWORD);
//
//     if (TRUE)
//     {
//         imagePtr        = (ITECH_IMAGE_MESSAGE *) tcpMessage;
//         messageHeader   = &imagePtr->header;
//         msgData         = &imagePtr->data[0];
//         chkSum          = &imagePtr->checksum;
//     }
//     else
//     {
//         msgPtr          = (ITECH_TCP_MESSAGE *) tcpMessage;
//         messageHeader   = &imagePtr->header;
//         msgData         = &imagePtr->data[0];
//         chkSum          = &imagePtr->checksum;
//     }
//
//     memcpy(tcpTransmitBuffer, messageHeader, headerMessageSize);
//     nextTransmitBufferPosition += headerMessageSize;
//
// //     memcpy(tcpTransmitBuffer + nextTransmitBufferPosition,
// //            &tcpMessage->header.dataLen, dataLenSize);
// //     nextTransmitBufferPosition += dataLenSize;
//
//     memcpy(tcpTransmitBuffer + nextTransmitBufferPosition,
//            msgData, messageHeader->dataLen);
//     nextTransmitBufferPosition += messageHeader->dataLen;
//
//     memcpy(tcpTransmitBuffer + nextTransmitBufferPosition,
//            chkSum, checksumSize);
//     nextTransmitBufferPosition += checksumSize;
//
//     // TODO, get the return of send in case a send
//     // completion state needs to be done
//
//     //print("SRM SENDING MESSAGE\n");
//
//     connectionResult = sendto(serverConnectionSocket, (char *)tcpTransmitBuffer, nextTransmitBufferPosition, 0, &remoteHostSocketAddr, sizeof(localSocketAddr));
//
//     if ( connectionResult == VCRT_ERROR )
//     {
//         connectionResult = shutdown(serverConnectionSocket, FLAG_ABORT_CONNECTION);
//         print("Send shutdown error %d, %lx\n", connectionResult, remoteHostSocketAddr.sin_addr.s_addr);
//         return FAIL;
//     }
//
//     SPI_ISSUE_TX_ONE_SHOT();
//     return PASS;
// }


////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////

void    Init_ServerCommunicationManager(void)
{
    InitServerCommTxBuffers();
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

NEW_STATE   TXM_exitA(void)
{
    //print("TX_A ACTIVE\n");

    //  SendMessage(THIS_MACHINE, CheckServerTransmitQueue);
    return TXM_TXQ;
}

////////////////////////////////////////////////////////////////////
//
// CheckServerTransmitQueue 
//
////////////////////////////////////////////////////////////////////
BYTE sendImage = TRUE;

NEW_STATE   TXM_exitB(void)
{
    BYTE transmitState;
    ITECH_TCP_MESSAGE *itechMsgToTransmitPtr;
    BYTE *dataPtr;

    static DWORD txFails    =   0;

    if ( IsMessagePosted() )
    {
        itechMsgToTransmitPtr = GetNextItechMessage();

        itechMsgToTransmitPtr->header.sequence = TXsequence++;

        itechMsgToTransmitPtr->checksum    =   CalculateTcpChecksum(itechMsgToTransmitPtr);

        transmitState = SendTcpMessage(itechMsgToTransmitPtr, FALSE);

        if ( transmitState == FAIL )
        {
            //printf("TX_exitB TX Failed\n");
         //   LogString("TX_exitB TX Failed\n");

            SendMessage(THIS_MACHINE, GoActive);
            return TXM_IDLE;
        }
    }
    else if ( imageTransmissionMessage.imageTransmitReady && sendImage )
    {
        imageTransmissionMessage.header.sequence = TXsequence++;
        imageTransmissionMessage.checksum = CalculateTcpChecksum((ITECH_TCP_MESSAGE *) &imageTransmissionMessage);

        transmitState = SendTcpMessage( &imageTransmissionMessage, TRUE);

        if ( transmitState == FAIL )
        {
          //  printf("TX_exitB TX Image Failed\n");

     //       LogString("TX_exitB TX Image Failed");
            SendMessage(THIS_MACHINE, GoActive);
            return TXM_IDLE;
        }

        StartTimer(MILLISECONDS(250));
        sendImage = FALSE;
        return SAME_STATE;
        //return TXM_TXQ;
    }

    return SAME_STATE;
}

NEW_STATE   TXM_exitC(void)
{
//    Do a check for image flash count and current partition;
//    Got ACK
    DWORD rxFlashCount      = GetMessageData1(); 
    DWORD rxCurrentPacket   = GetMessageData2(); 

    DWORD txFlashCount      = imageTransmissionMessage.header.param1; 
    DWORD txCurrentPacket   = imageTransmissionMessage.header.param2; 

    CancelTimer();

    if ( (rxFlashCount != txFlashCount) || (rxCurrentPacket != txCurrentPacket) )
    {
       // LogString("Response to incorrect flash count");
        StartTimer(MILLISECONDS(200));
        return SAME_STATE;
    }

    imageTXretries = 0;
    imageTransmissionMessage.imageTransmitReady = FALSE; 
    PostMessage(ImageTransmitter, ImageTXpassed);
    PostMessage(THIS_MACHINE, CheckServerTransmitQueue);
    sendImage = TRUE;

    return SAME_STATE;
}

NEW_STATE   TXM_exitD(void)
{
    //Timed OUt
    DWORD txFlashCount      = imageTransmissionMessage.header.param1; 
    DWORD txCurrentPacket   = imageTransmissionMessage.header.param2; 


    imageTXretries++;

    //LogString("Image Ack Time Out");

    if ( imageTXretries > 0 )
    {
        PostMessage(ImageTransmitter, ImageTXfailed);
        imageTransmissionMessage.imageTransmitReady = FALSE; 
        imageTXretries = 0;
    }

    sendImage = TRUE;
    PostMessage(THIS_MACHINE, CheckServerTransmitQueue);
    return SAME_STATE;
}


////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_TXM_IDLE)
EV_HANDLER(GoActive, TXM_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_TXM_TXQ)
EV_HANDLER(CheckServerTransmitQueue, TXM_exitB),
EV_HANDLER(AckReceived, TXM_exitC),
EV_HANDLER(TimeOut, TXM_exitD)
STATE_TRANSITION_MATRIX_END;

// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(TXM_Main_Entry)
STATE(_TXM_IDLE)     ,           
STATE(_TXM_TXQ)      
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////



