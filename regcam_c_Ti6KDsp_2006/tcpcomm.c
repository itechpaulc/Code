




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


////////////////////////////////////////////////////////////////////



#include "tcpcomm.h"


////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////

const char  itechHeaderToken[HEADER_ID_LEN] = "I-TECH!!";

#define GET_FAMILY_ID()                 (0x0055)
#define GET_PLANT_ID()                  (0x0077)

#define GET_DEST_DEVICE_TYPE_ID()       (0x0066)
#define GET_DEST_DEVICE_MODULE_ID()     (0x0067)

#define GET_SRC_DEVICE_TYPE_ID()        (0x0068)
#define GET_SRC_DEVICE_MODULE_ID()      (0x0069)

DEVICE_TYPE     deviceTypeConfig;

////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////


void    InitBuildTcpPacket(ITECH_TCP_HEADER_MESSAGE  *tcpMessage)
{
    strcpy(tcpMessage->headerId,
           itechHeaderToken, HEADER_ID_LEN);

    tcpMessage->familyId     =   GET_FAMILY_ID();
    tcpMessage->plantId      =   GET_PLANT_ID(); 

    tcpMessage->messageId    =   ITECH_MSG_UndefinedMsgType;
    tcpMessage->protocolVersion = ITECH_PROTOCOL_VERSION;

    tcpMessage->destDeviceTypeId     =   GET_DEST_DEVICE_TYPE_ID();
    tcpMessage->destDeviceModuleId   =   GET_DEST_DEVICE_MODULE_ID();

    tcpMessage->srcDeviceTypeId      =   GET_SRC_DEVICE_TYPE_ID();
    tcpMessage->srcDeviceModuleId    =   GET_SRC_DEVICE_MODULE_ID();

    tcpMessage->param1 = 0x0000;
    tcpMessage->param2 = 0x0000;
    tcpMessage->param3 = 0x0000;
    tcpMessage->param4 = 0x0000;

    tcpMessage->dataLen = 0x0000;

  //  tcpMessage->checksum = 0x0000;
}


////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetTcpMessageId(ITECH_TCP_HEADER_MESSAGE  *tcpMessage, ITECH_TCP_MESSAGE_ID tcpMessageId)
{
    tcpMessage->messageId = tcpMessageId;
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetTcpParam1(ITECH_TCP_HEADER_MESSAGE  *tcpMessage, DWORD param1)
{
    tcpMessage->param1 = param1;
    tcpMessage->param2 = 0x0000;
    tcpMessage->param3 = 0x0000;
    tcpMessage->param4 = 0x0000;
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetTcpParam12(ITECH_TCP_HEADER_MESSAGE  *tcpMessage, 
                      DWORD param1,  DWORD param2)
{
    tcpMessage->param1 = param1;
    tcpMessage->param2 = param2;
    tcpMessage->param3 = 0x0000;
    tcpMessage->param4 = 0x0000;
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetTcpParam123(ITECH_TCP_HEADER_MESSAGE  *tcpMessage, 
                       DWORD param1,  DWORD param2, DWORD param3)
{
    tcpMessage->param1 = param1;
    tcpMessage->param2 = param2;
    tcpMessage->param3 = param3;
    tcpMessage->param4 = 0x0000;
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetTcpParam1234(ITECH_TCP_HEADER_MESSAGE  *tcpMessage, DWORD param1,  
                        DWORD param2, DWORD param3, DWORD param4)
{
    tcpMessage->param1 = param1;
    tcpMessage->param2 = param2;
    tcpMessage->param3 = param3;
    tcpMessage->param4 = param4;
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    SetTcpData(ITECH_TCP_MESSAGE  *tcpMessage, BYTE *dat, DWORD datLen)
{
    int d;

    BYTE *dataPtr = &tcpMessage->data[0];

    tcpMessage->header.dataLen = datLen;

    for ( d=0; d<datLen; d++ )
    {
        (*dataPtr) = (*dat);

        dataPtr++;
        dat++;
    }
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

DWORD    CalculateTcpChecksum(ITECH_TCP_MESSAGE  *tcpMessage)
{
    int d, 
    headerLen = sizeof(ITECH_TCP_HEADER_MESSAGE); 
    //   dataLenSize = sizeof(tcpMessage->header.dataLen);

    DWORD checkSum =  0x00000000;
    BYTE *dataPtr = (BYTE *)tcpMessage;


    int messageSize = (headerLen + tcpMessage->header.dataLen);

     for ( d=0; d<messageSize ; d++ )
     {
//         if ( log == TRUE )
//             print("%d ", *dataPtr);
         checkSum += *dataPtr;
         dataPtr++;
     }
//
//     if ( log == TRUE )
//         print("\n ");
//
    return checkSum;
    //TODO


    //print("CHECK 3 : %d ", tcpMessage->checksum);
}

////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////

void    ShowTcpMessage(ITECH_TCP_MESSAGE  *tcpMessage)
{ 
    BYTE data;
    int d;

    BYTE    *dataPtr = (BYTE *)tcpMessage;

    int headerMessageSize = sizeof(ITECH_TCP_HEADER_MESSAGE);
    int dataLenSize = sizeof(tcpMessage->header.dataLen);
    int checksumSize = sizeof(tcpMessage->checksum);

    //print("Header ");
    for ( d=0; d<headerMessageSize; d++ )
    {
        data = (*dataPtr);
        //print(" 0x%X ", data);
        dataPtr++;
    }
    //print("\n");    
    //print("Data Len ");
    for ( d=0; d<dataLenSize; d++ )
    {
        data = (*dataPtr);
        //print(" 0x%X ", data);
        dataPtr++;
    }
    //print("\n");
    //print("Data ");
    for ( d=0; d<tcpMessage->header.dataLen; d++ )
    {
        data = (*dataPtr);
        //print(" 0x%X ", data);
        dataPtr++;
    }
    //print("\n");
    //print("Checksum ");
    for ( d=0; d<checksumSize; d++ )
    {
        data = (*dataPtr);
        //print(" 0x%X ", data);
        dataPtr++;
    }
    //print("\n");
}


void    SetDeviceType( DEVICE_TYPE deviceType )
{
    deviceTypeConfig = deviceType;
}

DEVICE_TYPE   GetDeviceType( void )
{
    return deviceTypeConfig;
}


