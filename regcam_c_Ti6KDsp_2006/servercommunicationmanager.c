




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

#include "servercommunicationmanager.h"


////////////////////////////////////////////////////////////////////
//
//  Local StateMachine Variables
//
////////////////////////////////////////////////////////////////////

DWORD           serverConnectionSocket;

SOCKET_ADDRESS  localSocketAddr, remoteHostSocketAddr;
uint_16         remoteHostSocketAddr_len; 


int             connectRetryCount;


RX_DATA rxData;

#define RX_BUFF_BACK    (&rxData.serverCommRxBuffer[rxData.backIndex])
#define RX_BUFF_FRONT   (&rxData.serverCommRxBuffer[rxData.frontIndex])
#define MAX_RX_DATA     (TCP_MAX_RECEIVE_BUFFER - rxData.backIndex)
///////////////////////////////////////////////////////////////////
//
// Function prototypes
//
////////////////////////////////////////////////////////////////////

DWORD DecimalToIp(BYTE d1, BYTE d2, BYTE d3, BYTE d4)
{
    DWORD dwIP = 0x00000000;

    dwIP |= d1<<24;
    dwIP |= d2<<16;
    dwIP |= d3<<8;
    dwIP |= d4;

    return dwIP;
}

void ResetRXData()
{
    memset(rxData,0, sizeof(RX_DATA));
}

////////////////////////////////////////////////////////////////////
//
// Initialize Machine
//
////////////////////////////////////////////////////////////////////

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

#define BINH_LAPTOP     5
#define JAAP_LAPTOP     0
#define JAAP_LAPTOP_IP  50
#define PAUL_LAPTOP     0
#define PAUL_LAPTOP_IP  51
#define TEST_PC         1

NEW_STATE   SRM_exitA(void)
{
    //print("SRM_A ACTIVE\n");

    localSocketAddr.sin_family      = AF_INET;
    localSocketAddr.sin_port        = 10001;
    localSocketAddr.sin_addr.s_addr = INADDR_ANY;

    memset((char *) &remoteHostSocketAddr, 0, sizeof(SOCKET_ADDRESS));

    // TODO
    remoteHostSocketAddr.sin_family = AF_INET;
    remoteHostSocketAddr.sin_port = 0;
    remoteHostSocketAddr.sin_addr.s_addr = 
    DecimalToIp(0, 0, 0, 0);

#ifdef _TCP
    serverConnectionSocket = socket_stream();
    //print("Using TCP: %x\n", getvar(IPADDR));

#else
    serverConnectionSocket = socket_dgram();
    //print("Using UDP: %x\n", getvar(IPADDR));
#endif

    if ( serverConnectionSocket == VCRT_HANDLE_ERROR )
    {
        //print("Creation of Socket FAILED\n");

        SendMessage(THIS_MACHINE, GoActive);
        return SRM_IDLE;
    }
  //  else
  //  {
  //      //print("Creation of Socket PASSED: Port: %d\n",remoteHostSocketAddr.sin_port);
  //  }

    ResetRXData();

    SendMessage(THIS_MACHINE, DoBindServerCommManager);

    return SRM_BINDING;
}

////////////////////////////////////////////////////////////////////
//
// DoBindServerCommManager
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SRM_exitB(void)
{    
    int error;

#ifdef _TCP
    DWORD   opt_length = sizeof(DWORD);
    DWORD   opt_value = TRUE;


    opt_value = TRUE;

    error =
    setsockopt(serverConnectionSocket, SOL_TCP, OPT_NO_NAGLE_ALGORITHM,
               &opt_value, opt_length);

    if ( error != VCRT_OK )
    {
        //print("Socket Option Seting FAILED : OPT_NO_NAGLE_ALGORITHM - %d\n", error);
        SendMessage(THIS_MACHINE, GoActive);
        return SRM_IDLE;
    }
  //  else
        //print("Socket Option Setting PASSED : OPT_NO_NAGLE_ALGORITHM \n");


    opt_value = 1000;

    error =
    setsockopt(serverConnectionSocket, SOL_TCP, OPT_CONNECT_TIMEOUT,
               &opt_value, opt_length);

    if ( error != VCRT_OK )
    {
        //print("Socket Option Seting FAILED : OPT_CONNECT_TIMEOUT - %d\n", error);
        SendMessage(THIS_MACHINE, GoActive);
        return SRM_IDLE;
    }
   // else
   //     print("Socket Option Setting PASSED : OPT_CONNECT_TIMEOUT \n");


    opt_value = TRUE;
#endif

    error = bind(serverConnectionSocket, &localSocketAddr, sizeof(SOCKET_ADDRESS));

    if ( error != VCRT_OK )
    {
        //print("Socket Binding FAILED - 0x%lx\n", error);
        SendMessage(THIS_MACHINE, GoActive);
        return SRM_IDLE;
    }
#ifdef _TCP
    else
    {
        //print("Socket Binding PASSED\n");

        error =
        setsockopt(serverConnectionSocket, SOL_TCP, OPT_RECEIVE_NOWAIT,
                   &opt_value, opt_length);

        if ( error != VCRT_OK )
        {
            //print("Socket Option Seting FAILED : OPT_RECEIVE_NOWAIT - %d\n", error);
            SendMessage(THIS_MACHINE, GoActive);
            return SRM_IDLE;
        }
       // else
       //     print("Socket Option Setting PASSED : OPT_RECEIVE_NOWAIT \n");



    }
#endif
    connectRetryCount = 0;
//     SendMessage(THIS_MACHINE, DoConnectServerCommManager);
//
//     return SRM_CONNECT;

    //By pass the connect attempts, using UDP
    SendMessage(THIS_MACHINE, CheckServerReceiveQueue);

    return SRM_CHECK_RECEIVE_QUEUE;  
}

////////////////////////////////////////////////////////////////////
//
// DoConnectServerCommManager 
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SRM_exitC(void)
{
    DWORD   connectionResult;

    //print("CONNECT TRY %d (BLOCKING)\n", connectRetryCount);

    connectionResult = connect(serverConnectionSocket, &remoteHostSocketAddr, sizeof(SOCKET_ADDRESS));

    if ( connectionResult != VCRT_OK )
    {
        //print("CONNECT FAILED error %d, %lx\n", connectionResult, remoteHostSocketAddr.sin_addr.s_addr);
        connectionResult = shutdown(serverConnectionSocket, FLAG_ABORT_CONNECTION);
        //print("shutdown error %d, %lx\n", connectionResult, remoteHostSocketAddr.sin_addr.s_addr);

        connectRetryCount++;
        StartTimer(SECONDS(2));

        SendMessage(THIS_MACHINE, GoActive);
        return SRM_IDLE;
    }

    //   SendMessage(THIS_MACHINE, CheckServerTransmitQueue);

    //print("CONNECTION PASSED to %lx, port %x\n", 
//          remoteHostSocketAddr.sin_addr.s_addr, remoteHostSocketAddr.sin_port);

    return SRM_CHECK_TRANSMIT_QUEUE;  
}

////////////////////////////////////////////////////////////////////
//
// TimeOut 
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SRM_exitD(void)
{
    DWORD   connectionResult;

    //print("CONNECT RE-TRY %d (BLOCKING)\n", connectRetryCount);

    connectionResult = connect(serverConnectionSocket, &remoteHostSocketAddr, sizeof(SOCKET_ADDRESS));

    if ( connectionResult != VCRT_OK )
    {
        //print("CONNECT FAILED error %d, %lx\n", connectionResult, remoteHostSocketAddr.sin_addr.s_addr);
        connectionResult = shutdown(serverConnectionSocket, FLAG_ABORT_CONNECTION);

        connectRetryCount++;
        StartTimer(SECONDS(2));

        return SRM_CONNECT_RETRY;
    }

    //print("CONNECT PASSED to %lx, port %d\n", 
  //        remoteHostSocketAddr.sin_addr.s_addr, remoteHostSocketAddr.sin_port);

    //  SendMessage(THIS_MACHINE, CheckServerTransmitQueue);

    return SRM_CHECK_TRANSMIT_QUEUE;  
}

////////////////////////////////////////////////////////////////////
//
// CheckServerTransmitQueue 
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SRM_exitE(void)
{   
//     BYTE transmitState;
//     ITECH_TCP_MESSAGE *itechMsgToTransmitPtr;
//
//     while ( IsMessagePosted() )
//     {
//         itechMsgToTransmitPtr = GetNextItechMessage();
//
//         // TODO, check if all were sent
//         transmitState = SendTcpMessage(itechMsgToTransmitPtr);
//
//
//         if ( transmitState == FAIL )
//         {
//             SendMessage(THIS_MACHINE, GoActive);
//             return SRM_IDLE;
//         }
//     }
//
//     SendMessage(THIS_MACHINE, CheckServerReceiveQueue);
//
    return SRM_CHECK_RECEIVE_QUEUE;  
}


////////////////////////////////////////////////////////////////////
//
// CheckServerReceiveQueue 
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SRM_exitF(void)
{
    unsigned int serverCommRxByteCounter;
    int b;
    char *p = (BYTE *)rxData.serverCommRxBuffer;
    DWORD   connectionResult;
    BYTE    data;

    if ( 0 )
    {
        return SRM_COMMUNCATION_ERROR;
    }

#ifdef _TCP
    serverCommRxByteCounter = 
    recv(serverConnectionSocket, RX_BUFF_BACK, MAX_RX_DATA, 0);
    { //Need to keep for matching brackets
#else
    if ( VCRT_selectset(&serverConnectionSocket,1,-1) != 0 )
    {
        remoteHostSocketAddr_len = sizeof(remoteHostSocketAddr);


        serverCommRxByteCounter = 
        recvfrom(serverConnectionSocket, RX_BUFF_BACK, MAX_RX_DATA, 0, &remoteHostSocketAddr, &remoteHostSocketAddr_len);
#endif
        if ( serverCommRxByteCounter == VCRT_ERROR )
        {
            connectionResult = shutdown(serverConnectionSocket, FLAG_ABORT_CONNECTION);
            //print("Recieve shutdown error %d, %lx\n", connectionResult, remoteHostSocketAddr.sin_addr.s_addr);
            SendMessage(THIS_MACHINE, GoActive);
            return SRM_IDLE;
        }

        if ( serverCommRxByteCounter != 0 )
        {
            //print("rxBytes %d\n", serverCommRxByteCounter);

//             print("RX_DATA: %d %d \n",rxData.frontIndex,rxData.backIndex);
//
//              for ( b=rxData.backIndex; b<(serverCommRxByteCounter+rxData.backIndex); b++ )
//              {
//                  //data = (*p);
//                  print("%d ", rxData.serverCommRxBuffer[b]);
//                  //p++;
//              }
//              print("\n ");

            // TODO WAIT For Parser to finish

            rxData.backIndex += serverCommRxByteCounter;
            SendMessage(ServerMessageParser, NewServerMessageDetected);

            SPI_ISSUE_RX_ONE_SHOT();
        }
    }


    SendMessage(THIS_MACHINE, CheckServerReceiveQueue);

    return SAME_STATE;  
}


////////////////////////////////////////////////////////////////////
//
// TimeOut 
//
////////////////////////////////////////////////////////////////////

NEW_STATE   SRM_exitG(void)
{

    // Close connection
    // flush buffers rx/tx and drivers
    // timer then reconnect
    // inform others of the reconnect

    SendMessage(THIS_MACHINE, GoActive);
    return SRM_IDLE;  
}

////////////////////////////////////////////////////////////////////
//
// State Matrix Tables
//
////////////////////////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(_SRM_IDLE)
EV_HANDLER(GoActive, SRM_exitA)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SRM_BINDING)
EV_HANDLER(DoBindServerCommManager, SRM_exitB)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SRM_CONNECT)
EV_HANDLER(DoConnectServerCommManager, SRM_exitC)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SRM_CONNECT_RETRY)
EV_HANDLER(TimeOut, SRM_exitD)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SRM_CHECK_TRANSMIT_QUEUE)
EV_HANDLER(CheckServerTransmitQueue, SRM_exitE)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SRM_CHECK_RECEIVE_QUEUE)
EV_HANDLER(CheckServerReceiveQueue, SRM_exitF)
STATE_TRANSITION_MATRIX_END;

STATE_TRANSITION_MATRIX(_SRM_COMMUNCATION_ERROR)
EV_HANDLER(TimeOut, SRM_exitG)
STATE_TRANSITION_MATRIX_END;

// 
// VERY IMPORTANT : 
//      State Entry definition order MUST match the 
//      order of the state definition in the .H File 
//
//
//      This the State Machine Response Entry
//

SM_RESPONSE_ENTRY(SRM_Main_Entry)
STATE(_SRM_IDLE)                    ,           
STATE(_SRM_BINDING)                 ,   
STATE(_SRM_CONNECT)                 ,           
STATE(_SRM_CONNECT_RETRY)           ,
STATE(_SRM_CHECK_TRANSMIT_QUEUE)    ,           
STATE(_SRM_CHECK_RECEIVE_QUEUE)     ,
STATE(_SRM_COMMUNCATION_ERROR)      
SM_RESPONSE_END


////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////


