

/////////////////////////////////////////////////////////////////////////////
//
//
//    $Header:      $
//    $Log:         $
//
//
//    Author : Paul Calinawan        February 1998
//
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//    NOTE:
//
//    This document contains CONFIDENTIAL and proprietary information
//    which is the property of Graphics Microsystems, Inc. It may not
//    be copied or transmitted in whole or in part by any means to any
//    media without Graphics Microsystems Inc's prior written permission.
//
/////////////////////////////////////////////////////////////////////////////



#include "prxcomm.h"



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  PciRxCommMachine
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
//  PciRxCommMachine
//
//      - public interface functions :
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
PciRxCommMachine::LinkAll(PciTxCommMachine *pPTC, ColorSmartSystemMonitor *pCSMON) {

    LinkPciTxCommMachine(pPTC);
    LinkColorSmartSystemMonitor(pCSMON);
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
PciRxCommMachine::LinkPciTxCommMachine(PciTxCommMachine *pPTC) {

    ptrPTC = pPTC;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void
PciRxCommMachine::LinkColorSmartSystemMonitor(ColorSmartSystemMonitor *pCSMON) {

    ptrCSMON = pCSMON;
}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void const *
PciRxCommMachine::GetPciRxCommBuffer(void) {

    return PCI_RX_BUFFER_ADDRESS;
}



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// PciRxCommMachine - private helper functions
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////

void    PciRxCommMachine::processQuery(void)
{
    // get the command and extension Id

    WORD    queryId = currMailboxMessage.GetCommandAndExtension();

    switch(queryId)
    {
        case CE_GetControllerVersion    : doGetControllerVersion(); break;
        case CE_GetProbeHeadVersion     : doGetProbeHeadVersion();  break;
        case CE_GetSystemStatus         : doGetSystemStatus();      break;
        case CE_GetProbeHeadStatus      : doGetProbeHeadStatus();   break;
        case CE_GetProbeHeadPosition    : doGetProbeHeadPosition(); break;

            default :

                // unknown
                break;


    }

    // Send message to PRC


}

//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    PciRxCommMachine::doGetControllerVersion(void) {


}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    PciRxCommMachine::doGetProbeHeadVersion(void) {


}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    PciRxCommMachine::doGetSystemStatus(void) {


}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    PciRxCommMachine::doGetProbeHeadStatus(void) {


}


//////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////

void    PciRxCommMachine::doGetProbeHeadPosition(void) {


}



//////////////////////////////////////////////////
//
// PciRxCommMachine - RESPONSE ENTRIES
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//
// State Transition Matrices
//
//////////////////////////////////////////////////

STATE_TRANSITION_MATRIX(PciRxCommMachine, _PRC_IDLE)
    EV_HANDLER(PciMessageReceived, PRC_h1)
STATE_TRANSITION_MATRIX_END;


//////////////////////////////////////////////////
//
// Matrix Table
//
//////////////////////////////////////////////////

DEFINE_RESPONSE_TABLE_ENTRY(PciRxCommMachine)
    STATE_MATRIX_ENTRY(_PRC_IDLE)
RESPONSE_TABLE_END;



//////////////////////////////////////////////////
//
// Static Member Definitions
//
//////////////////////////////////////////////////

WORD                PciRxCommMachine::tempTxData[TEMP_TX_DATA_SIZE];

MAILBOX_MESSAGE     PciRxCommMachine::currMailboxMessage;

WORD                PciRxCommMachine::errorCount = 0;


PciTxCommMachine            * PciRxCommMachine::ptrPTC = 0;

ColorSmartSystemMonitor     * PciRxCommMachine::ptrCSMON = 0;



//////////////////////////////////////////////////
//
// PciRxCommMachine - Constructors, Destructors
//
//////////////////////////////////////////////////

PciRxCommMachine::PciRxCommMachine(STATE_MACHINE_ID sMsysID)
    :StateMachine(sMsysID)
{
    ASSIGN_RESPONSE_TABLE();

    SetCurrState(PRC_IDLE);
}

PciRxCommMachine::~PciRxCommMachine(void) { }


WORD    PciRxCommMachine::GetErrorCount(void) {

    return  PciRxCommMachine::errorCount;
}


//////////////////////////////////////////////////
//////////////////////////////////////////////////
//
// PciRxCommMachine - private EXIT PROCEDURES
//
//////////////////////////////////////////////////
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//
// Message : Pci Message Received
//
//////////////////////////////////////////////////

struct CE_TRANSLATION {

    CE_CMD_EXT_ID       ceId;
    SYSTEM_MESSAGE_ID   sysMsgId;
};

CE_TRANSLATION      CETranslateTable[] = {

    { CE_RunController                  , RunController                 },
    { CE_ReleaseEquipment               , GoShutDown                    },
    { CE_SetTableParameters             , SetTableParameters            },
    { CE_SetTimingParameters            , SetTimingParameters           },
    { CE_SetMoveProfiles                , SetMoveProfiles               },
    { CE_GoHomeIdle                     , GoHomeIdle                    },
    { CE_GoScanningMode                 , GoScanningMode                },
    { CE_GoTrackballMode                , GoTrackballMode               },
    { CE_GoDevelopmentDiagnosticMode    , GoDevelopmentDiagnosticMode   },
    { CE_MoveToPosition                 , MoveToPosition                },
    { CE_MoveToHomePosition             , GoHome                        },
    { CE_FindLimitPosition              , FindLimits                    },
    { CE_FindHomePosition               , FindHome                      },
    { CE_PointTargetLamp                , PointTargetLamp               },
    { CE_TurnOnTargetLamp               , TargetLampOn                  },
    { CE_TurnOffTargetLamp              , TargetLampOff                 },
    { CE_BlinkTargetLamp                , BlinkTargetLamp               },
    { CE_FlashTargetLamp                , FlashTargetLamp               },
    { CE_SetScanParameterData           , SetScanParameterData          },
    { CE_StartScanningNoReports         , StartScanningNoReports        },
    { CE_StartScanningWithReports       , StartScanningWithReports      },
    { CE_DoTargetAlignmentScan          , DoTargetAlignmentScan         },
    { CE_DoAlignmentScan                , DoAlignmentScan               },

    { CE_NULL , NULL_MESSAGE_ID }
};

WORD
PciRxCommMachine::PRC_h1(void)
{
        // Read mail box data

        currMailboxMessage = ReadMailboxMessage();

        // Check if system operation or query command

        if(currMailboxMessage.GetCommand() < COMMAND_QUERIES)
        {
            // regular commands, translate for CSM

            WORD    ceMailbox = currMailboxMessage.GetCommandAndExtension();

            CE_TRANSLATION      * ceTableSearch = CETranslateTable;

            do {
                    // Check for match

                    if(ceTableSearch->ceId == ceMailbox)
                    {
                        // Send message to CSM

                        SendLowPrMsg(ColorSmartManagerID, ceTableSearch->sysMsgId,
                                        currMailboxMessage.GetParam1(),
                                        currMailboxMessage.GetParam2());

                        break;
                    }

                 ++ceTableSearch; // next entry

            }while(ceTableSearch->ceId != CE_NULL);


            if(ceTableSearch->ceId == CE_NULL)
            {
                // Respond to NT, Unknown Message


            }
        }
        else
        {
            // query commands

            processQuery();
        }


    return  PRC_IDLE;
}










