





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


#include "itechsys.h"


////////////////////////////////////////////////////////////////////


#include "kernel.h"


////////////////////////////////////////////////////////////////////

//
//  Internal VC Camera events:
//
//  #define     TIMER           0
//  #define     MM_CARD         1
//  #define     IMAGE_READY     4
//  #define     EXP_READY       5
//


////////////////////////////////////////////////////////////////////
//
// State Machine Header Files
//
////////////////////////////////////////////////////////////////////

#include "ribboncameramanager.h"

#include "heartbeat.h"

#include "imaging.h"

#include "triggermonitor.h"
#include "imageacquirerhandler.h"
#include "imageprocessinghandler.h"

#include "imagetransmitter.h"
#include "datatransmitter.h"
#include "servercommunicationmanager.h"
#include "transmitManager.h"
#include "servermessageparser.h"

#include "scannermanager.h"

#include "transportmanager.h"

#include "encoderhandler.h"

#include "flashcounterlogger.h"
#include "flashpowercontroller.h"

#include "systemhardwaremonitor.h"

#include "spicomm.h"
#include "watchDog.h"

////////////////////////////////////////////////////////////////////
//
//  Local Kernel Variables
//
////////////////////////////////////////////////////////////////////

SYSTEM_EVENT        *lastMessageIndex,
    *nextMessage,
    *currentMessage;

int     kernelMessageCounter; 
BOOL    deviceRebooted;
char    logArray[ 128 ];
char    *ptrLogArray;

extern  DWORD   TXsequence;

const char* smID[] = 
{
    "RibbonCameraManager"         ,
    "HeartBeatHandler"            ,

    "ScannerManager"              ,

    "TriggerMonitor"              ,
    "ImageAcquirerHandler"        ,
    "ImageProcessingHandler"      ,

    "ImageTransmitter"            ,
    "DataTransmitter"             ,
    "ServerCommunicationManager"  ,

    "ServerMessageParser"         ,
    "TransmitManager"             , 

    "TransportManager"            ,

    "EncoderHandler"              ,

    "FlashCounterLogger"          ,
    "FlashPowerController"        ,

    "SystemHardwareMonitor"       ,

    "SPI_Manager"      			 ,           
    "WDT_Manager"      			           
};

//
// The Message QUEUE
//

SYSTEM_EVENT            kernelMessageQueue[MAX_MESSAGE_COUNT];

void                *   smEventHandlerEntries[MAX_STATE_MACHINES_COUNT];

NEW_STATE                   smStates[MAX_STATE_MACHINES_COUNT];

//
//
//

NEW_STATE               currentSmState;

STATE_MACHINE_ID    currentDestinationSmId,
    currentSourceSmId;

U32                 currentMessageData1,
    currentMessageData2;

//
//
//

EVENT_HANDLER       * eventHandlerSelect;

void                **responseEntry;

NEW_STATE               newState;

//
//
//
//

typedef struct
{
    int     milliseconds;   // 0 - 999 msec
    long    seconds;        // seconds since 1900

} TIME_TAG;


TIME_TAG            kernelSystemTick,
    nextKernelTimeout;


unsigned int        systemTimers[MAX_STATE_MACHINES_COUNT];

U16                 activeTimersToDecrement,
    currentSmTimer;


////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

void    DoHandleLostEvent(void);

void    InitializeKernelAndMachines(void);

void    InitializeKernelTimers(void);

void    InitializeStateMachines(void);


inline void     DoKernelTimerCheck(void);



////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

inline  
    void    SendKernelMessage
    (STATE_MACHINE_ID smID, SYSTEM_MESSAGE_ID msgID);

inline  
    void    SendKernelMessageAndData
    (STATE_MACHINE_ID smID, SYSTEM_MESSAGE_ID msgID, 
     U32 msgDAT1, U32 msgDAT2);


////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

#define MAX_MSEC_COUNT      1000


inline  void    GetCurrentTime(TIME_TAG *currTime);

inline  void    AddTime(TIME_TAG *currTime, TIME_TAG *addTime);

inline  BOOL    IsTimerElapsed(TIME_TAG *checkTime);


void    ShowCurrentTime(void);



////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

void    InitializeKernel(void)
{
    ptrLogArray = &logArray[0];

    InitializeKernelAndMachines();

    InitializeKernelTimers();

    InitializeStateMachines();

    // Activate the state machines

    SendKernelMessage(RibbonCameraManager, GoActive);


}
////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

void    RunKernel(void)
{
    char newKey;
    static char resetCmd[] = "000";

    // while ( TRUE && !kbhit() )
    deviceRebooted = TRUE;

    while ( TRUE )
    {
        if ( kbhit() )
        {
            newKey = getchar();

            resetCmd[0] = resetCmd[1];
            resetCmd[1] = resetCmd[2];
            resetCmd[2] = newKey;

            if (resetCmd[0] == 'r' && resetCmd[1] == 's' && resetCmd[2] == 't')
                SendMessageAndData(WatchDog_Manager,ResetWDT, FALSE, 0);

            if ( newKey == 's' || newKey =='S' )
            {
                ShowAllMessageInQueue();
                ShowAllMachineStates();
                ShowAllTimerValues();
                ShowFrameBuffers();
            }
            else if ( newKey == 'C' || newKey =='c' )
                DisplayCameraConfig();
            else if ( newKey == 'T' || newKey =='t' )
                DisplayTransportConfig();

            else if ( newKey == ESC )
                break;
        }

        if ( currentMessage != nextMessage )
        {
            if ( kernelMessageCounter == 100 )
            {
                //printf("Kernel Q at 100");
                ShowAllMessageInQueue();
                ShowAllMachineStates();
                ShowAllTimerValues();
                // reset();
                continue;
            }

            currentSmState = smStates[currentMessage->smDestinationId];
            currentDestinationSmId  = currentMessage->smDestinationId;
            currentSourceSmId       = currentMessage->smSourceId;
            currentMessageData1 = currentMessage->messageData1;
            currentMessageData2 = currentMessage->messageData2;

            //
            // Point to the state machine matrix entry
            //

            responseEntry = 
                (void **)smEventHandlerEntries[currentDestinationSmId];

            eventHandlerSelect = 
                (EVENT_HANDLER *)(*(responseEntry + currentSmState));

            while ( TRUE )
            {
                // search for the state matrix for matching event id    
                if ( eventHandlerSelect->eventId == currentMessage->messageId )
                {
                    // call the exit procedure          

                    newState = eventHandlerSelect->exitProcedure();

                    smStates[currentDestinationSmId] = newState;

                    break;
                }

                if ( eventHandlerSelect->eventId == NULL_MESSAGE_ID )
                {
                    if ( !currentMessage->postedMessage )
                        DoHandleLostEvent();

                    break;
                }

                // select the next in the event handler entries

                ++eventHandlerSelect;                   
            }

            if ( currentMessage == LAST_MESSAGE_INDEX )
                currentMessage = kernelMessageQueue;
            else
                ++currentMessage;

            kernelMessageCounter--;
        }
     //   else
     //       print("!!!!!!!!!!!!!!!! Kernel Message Queue Empty !!!!!!!\n");
     //
        //
        // Check other system wide events
        //

        DoKernelTimerCheck();

    }

    //TODO

    ReleaseFrameBuffers();
}



////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

inline  void    DoKernelTimerCheck(void)
{
    if ( IsTimerElapsed(&nextKernelTimeout) )
    {

        GetCurrentTime(&nextKernelTimeout);
        AddTime(&nextKernelTimeout , &kernelSystemTick);

        if ( KERNEL_HAS_RUNNING_TIMERS() )
        {
            for ( currentSmTimer=0; 
                currentSmTimer<MAX_STATE_MACHINES_COUNT; currentSmTimer++ )
            {
                if ( systemTimers[currentSmTimer] != 0 )
                {
                    --systemTimers[currentSmTimer];

                    if ( systemTimers[currentSmTimer] == 0 )
                    {
                        SendKernelMessage((STATE_MACHINE_ID)currentSmTimer, TimeOut);

                        --activeTimersToDecrement;

                        if ( activeTimersToDecrement != 0 )
                            continue;
                        else
                            break;
                    }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////


void    InitializeKernelAndMachines(void)
{
    // Reset all message queue pointers

    currentMessage  = kernelMessageQueue;
    nextMessage     = kernelMessageQueue;

    // Define the end of the queue

    lastMessageIndex = 
        &kernelMessageQueue[MAX_MESSAGE_COUNT - 1];

    kernelMessageCounter = 0;
    //  
    //  Build the State Machine Response Entries
    //  and Initialize the State Machine States
    //

    smEventHandlerEntries[RibbonCameraManager]      =   &RCM_Main_Entry;
    smStates[RibbonCameraManager]                   =   INIT_IDLE_STATE;

    smEventHandlerEntries[HeartBeatHandler]         =   &HBH_Main_Entry;
    smStates[HeartBeatHandler]                      =   INIT_IDLE_STATE;

    smEventHandlerEntries[TriggerMonitor]           =   &TGM_Main_Entry;
    smStates[TriggerMonitor]                        =   INIT_IDLE_STATE;

    smEventHandlerEntries[ImageAcquirerHandler]     =   &IAH_Main_Entry;
    smStates[ImageAcquirerHandler]                  =   INIT_IDLE_STATE;

    smEventHandlerEntries[ImageProcessingHandler]   =   &IPH_Main_Entry;
    smStates[ImageProcessingHandler]                =   INIT_IDLE_STATE;

    smEventHandlerEntries[ImageTransmitter]         =   &ITX_Main_Entry;
    smStates[ImageTransmitter]                      =   INIT_IDLE_STATE;

    smEventHandlerEntries[DataTransmitter]          =   &DTX_Main_Entry;
    smStates[DataTransmitter]                       =   INIT_IDLE_STATE;

    smEventHandlerEntries[ServerCommunicationManager]   =   &SRM_Main_Entry;
    smStates[ServerCommunicationManager]                =   INIT_IDLE_STATE;

    smEventHandlerEntries[ServerMessageParser]      =   &SMP_Main_Entry;
    smStates[ServerMessageParser]                   =   INIT_IDLE_STATE;

    smEventHandlerEntries[TransmitManager]          =   &TXM_Main_Entry;
    smStates[TransmitManager]                       =   INIT_IDLE_STATE;

    smEventHandlerEntries[ScannerManager]           =   &SCM_Main_Entry;
    smStates[ScannerManager]                        =   INIT_IDLE_STATE;

    smEventHandlerEntries[TransportManager]         =   &TRM_Main_Entry;
    smStates[TransportManager]                      =   INIT_IDLE_STATE;

    smEventHandlerEntries[EncoderHandler]           =   &ECH_Main_Entry;
    smStates[EncoderHandler]                        =   INIT_IDLE_STATE;

    smEventHandlerEntries[FlashCounterLogger]       =   &FCL_Main_Entry;
    smStates[FlashCounterLogger]                    =   INIT_IDLE_STATE;

    smEventHandlerEntries[FlashPowerController]     =   &FPC_Main_Entry;
    smStates[FlashPowerController]                  =   INIT_IDLE_STATE;

    smEventHandlerEntries[SystemHardwareMonitor]    =   &SHM_Main_Entry;
    smStates[SystemHardwareMonitor]                 =   INIT_IDLE_STATE;

    smEventHandlerEntries[SPI_Manager]              =   &SPI_Main_Entry;
    smStates[SPI_Manager]                           =   INIT_IDLE_STATE;

    smEventHandlerEntries[WatchDog_Manager]         =   &WDT_Main_Entry;
    smStates[WatchDog_Manager]                      =   INIT_IDLE_STATE;

}


////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

void    InitializeStateMachines(void)
{
    //print("Initializing State Machines\n\n");

    Init_RibCameraManager();
    Init_HeartBeatHandler();

    Init_TriggerMonitor();
    Init_ImageAcquirerHandler();

    Init_ImageTransmitter();

    Init_ServerCommunicationManager();

    Init_ServerMessageParser();

    Init_ScannerManager();

    Init_SPI_Manager();
    Init_TransportManager();

    Init_EncoderHandler();

    Init_FlashCounterLogger();
    Init_FlashPowerController();

    Init_SystemHardwareMonitor();

	Init_WDTHandlerMachine();
}


////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

void    InitializeKernelTimers(void)
{
    int timerCount;

    kernelSystemTick.milliseconds   = KERNEL_SYSTEM_MSEC_TICK;
    kernelSystemTick.seconds        = KERNEL_SYSTEM_SEC_TICK;

    // Reset all timers to zero

    activeTimersToDecrement = 0;

    for ( timerCount=0; timerCount<MAX_STATE_MACHINES_COUNT ;timerCount++ )
    {
        systemTimers[timerCount] = 0;  
    }

    GetCurrentTime(&nextKernelTimeout);

    AddTime(&nextKernelTimeout , &kernelSystemTick);
}


////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

inline  
    void    SendKernelMessage
    (STATE_MACHINE_ID smID, SYSTEM_MESSAGE_ID msgID)
{
    nextMessage->smDestinationId    = smID;            
    nextMessage->messageId          = msgID;

    nextMessage->smSourceId         = KERNEL_SM_ID; 

    nextMessage->messageData1       = NO_DATA;
    nextMessage->messageData2       = NO_DATA;

    if ( nextMessage == LAST_MESSAGE_INDEX )
        nextMessage = kernelMessageQueue;
    else
        ++nextMessage;

    kernelMessageCounter++;
}


inline  
    void    SendKernelMessageAndData
    (STATE_MACHINE_ID smID, SYSTEM_MESSAGE_ID msgID, 
     U32 msgDAT1, U32 msgDAT2)
{
    nextMessage->smDestinationId    = smID;            
    nextMessage->messageId          = msgID;

    nextMessage->smSourceId         = KERNEL_SM_ID; 

    nextMessage->messageData1       = msgDAT1;
    nextMessage->messageData2       = msgDAT2;

    if ( nextMessage == LAST_MESSAGE_INDEX )
        nextMessage = kernelMessageQueue;
    else
        ++nextMessage;

    kernelMessageCounter++;
}

void    SendMessage
    (STATE_MACHINE_ID smID, SYSTEM_MESSAGE_ID msgID)
{
    nextMessage->smDestinationId    = smID;            
    nextMessage->messageId          = msgID;
    nextMessage->postedMessage      = FALSE;

    nextMessage->smSourceId         = currentDestinationSmId; 

    nextMessage->messageData1       = 0x0000;
    nextMessage->messageData2       = 0x0000;

    if ( nextMessage == LAST_MESSAGE_INDEX )
        nextMessage = kernelMessageQueue;
    else
        ++nextMessage;

    kernelMessageCounter++;
}


void    SendMessageAndData
    (STATE_MACHINE_ID smID, SYSTEM_MESSAGE_ID msgID, 
     U32 msgDAT1, U32 msgDAT2)
{
    nextMessage->smDestinationId    = smID;            
    nextMessage->messageId          = msgID;
    nextMessage->postedMessage      = FALSE;

    nextMessage->smSourceId         = currentDestinationSmId; 

    nextMessage->messageData1       = msgDAT1;
    nextMessage->messageData2       = msgDAT2;

    if ( nextMessage == LAST_MESSAGE_INDEX )
        nextMessage = kernelMessageQueue;
    else
        ++nextMessage;

    kernelMessageCounter++;
}

void    PostMessage
    (STATE_MACHINE_ID smID, SYSTEM_MESSAGE_ID msgID)
{
    nextMessage->smDestinationId    = smID;            
    nextMessage->messageId          = msgID;
    nextMessage->postedMessage      = TRUE;

    nextMessage->smSourceId         = currentDestinationSmId; 

    nextMessage->messageData1       = 0x0000;
    nextMessage->messageData2       = 0x0000;

    if ( nextMessage == LAST_MESSAGE_INDEX )
        nextMessage = kernelMessageQueue;
    else
        ++nextMessage;

    kernelMessageCounter++;
}


void    PostMessageAndData
    (STATE_MACHINE_ID smID, SYSTEM_MESSAGE_ID msgID, 
     U32 msgDAT1, U32 msgDAT2)
{
    nextMessage->smDestinationId    = smID;            
    nextMessage->messageId          = msgID;
    nextMessage->postedMessage      = TRUE;

    nextMessage->smSourceId         = currentDestinationSmId; 

    nextMessage->messageData1       = msgDAT1;
    nextMessage->messageData2       = msgDAT2;

    if ( nextMessage == LAST_MESSAGE_INDEX )
        nextMessage = kernelMessageQueue;
    else
        ++nextMessage;

    kernelMessageCounter++;
}



////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

inline  void    GetCurrentTime(TIME_TAG *currTime)
{
    currTime->milliseconds = getvar(MSEC);
    currTime->seconds = getvar(SEC);
}

inline  void    AddTime(TIME_TAG *currTime, TIME_TAG *addTime)
{
    currTime->milliseconds += addTime->milliseconds;
    currTime->seconds += addTime->seconds;

    if ( currTime->milliseconds > MAX_MSEC_COUNT )
    {
        currTime->milliseconds -= MAX_MSEC_COUNT;
        ++currTime->seconds;
    }
} 

inline  BOOL    IsTimerElapsed(TIME_TAG *checkTime)
{
    int             currTimeMilliseconds = getvar(MSEC);
    long    currTimeSeconds = getvar(SEC);

    if ( (currTimeSeconds > checkTime->seconds) )
        return TRUE;

    if ( (currTimeSeconds == checkTime->seconds)  
         &&(currTimeMilliseconds >= checkTime->milliseconds) )
        return TRUE;

    return FALSE;
}

void    ShowCurrentTime(void)
{
    int     currTimeMilliseconds = getvar(MSEC);
    long    currTimeSeconds = getvar(SEC);

    //print("SEC = %lx  MSEC = %d\n\n", currTimeSeconds,currTimeMilliseconds);
}

////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////
extern DWORD spiReadRetries;
extern DWORD spiWriteRetries;
void    ShowMessage(SYSTEM_EVENT *msg)
{
   if ( msg->smDestinationId >= LAST_SM_ID )
   {
       //print("Invalid DEST MACHINE ID  (0x%x)\n",  msg->smDestinationId);
   }
   else
       print("DEST MACHINE ID   %s-(0x%x)\n", smID[msg->smDestinationId], msg->smDestinationId);

   print("DEST MACH STATE   0x%x\n", smStates[msg->smDestinationId]);
   print("MESSAGE ID        0x%X\n", msg->messageId);

 if ( msg->smSourceId >= LAST_SM_ID )
 {
       print("Invalid SOURCE MACHINE ID -(0x%x)\n", msg->smSourceId);
     return;
 }

   print("SOURCE MACHINE ID %s-(0x%x)\n", smID[msg->smSourceId], msg->smSourceId); 
   print("SOURCE MACH STATE   0x%x\n", smStates[msg->smSourceId]);

   print("MESSAGE DATA1 %d\n", msg->messageData1);
   print("MESSAGE DATA2 %d\n", msg->messageData2);
   print("CheckSum Read Retries: 0x%x\n",spiReadRetries);
   print("CheckSum Write Retries: 0x%x\n\n",spiWriteRetries);
}

void    ShowCurrentMessage(void)
{
    ShowMessage(currentMessage);
}

void    ShowAllMessageInQueue(void)
{
    SYSTEM_EVENT    *tempNextMessage = nextMessage;
    SYSTEM_EVENT    *tempCurrentMessage = currentMessage;

   // print("\n\nSHOW MESSAGES IN QUEUE\n\n");

    if ( tempNextMessage == tempCurrentMessage );
        //print("\n\nNO MESSAGES IN QUEUE\n\n");
    else
        while ( tempNextMessage != tempCurrentMessage )
        {
            ShowMessage(tempCurrentMessage);

            if ( tempCurrentMessage == LAST_MESSAGE_INDEX )
                tempCurrentMessage = kernelMessageQueue;
            else
                ++tempCurrentMessage;
        }
}

void    ShowAllMachineStates(void)
{
   int smCount;

  // print("\n\nSHOW MACHINE STATES\n\n");

   for ( smCount=0; smCount<LAST_SM_ID; smCount++ )
   {
       print("MACHINE %s\t\tSTATE 0x%X\n",smID[smCount], smStates[smCount]);
   }
}

void    ShowAllTimerValues(void)
{
 //   int smCount;
 //
 //   print("\n\nSHOW MACHINE TIMERS\n\n");
 //   print("ACTIVE COUNT %d\n", activeTimersToDecrement);
 //
 //   for ( smCount=0; smCount<LAST_SM_ID; smCount++ )
 //   {
 //       print("MACHINE ID 0x%X TIME %d\n", smCount, systemTimers[smCount]);
 //   }
}

void    ShowCameraStatus(void)
{
//    print("EXPOSING %d\n", VC_GET_TRACK_NUMBER_IMAGE_EXPOSING());
//    print("STORING %d\n",  VC_GET_TRACK_NUMBER_IMAGE_STORING());
//    print("IMGREADY %d\n", VC_GET_TRACK_NUMBER_IMAGE_READY());
}

////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////

void    DoHandleLostEvent(void)
{
 //   print("\nWARNING UNHANDLED EVENT\n");

    ShowCurrentMessage();

 //   print("\n\n"); 
}

void    LogString(char *ASCIIstring)
{
    ITECH_TCP_MESSAGE log;

    InitBuildTcpPacket(&log.header);
    SetTcpMessageId(&log.header, ITECH_MSG_LogString);
    SetTcpData(&log, ASCIIstring, strlen(ASCIIstring));

 //   print("%s \n", ASCIIstring);

    PostServerItechMessage(&log);
}


NEW_STATE   DoNothing(void)
{
    return SAME_STATE;
}

