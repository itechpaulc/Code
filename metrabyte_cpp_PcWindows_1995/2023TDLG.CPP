/*  Project d202ui3
    Radionics Inc.
    Copyright © 1994. All Rights Reserved.

    SUBSYSTEM:    d202ui3.exe Application
    FILE:         2023tdlg.cpp
    AUTHOR:       Paul L Calinawan


    OVERVIEW
    ========
    Source file for implementation of d202ui3TDLGClient (TDialog).      
*/


#include <owl\owlpch.h>
#pragma hdrstop

#include "d2023app.h"
#include "2023tdlg.h"


//
// Build a response table for all messages/commands handled
// by the application.
//
DEFINE_RESPONSE_TABLE1(d202ui3TDLGClient, TDialog)
//{{d202ui3TDLGClientRSP_TBL_BEGIN}}
//{{d202ui3TDLGClientRSP_TBL_END}}
END_RESPONSE_TABLE;


//{{d202ui3TDLGClient Implementation}}


//////////////////////////////////////////////////////////
// d202ui3TDLGClient
// ==========
// Construction/Destruction handling.
d202ui3TDLGClient::d202ui3TDLGClient (TWindow *parent, TResId resId, TModule *module)
    : TDialog(parent, resId, module)
{
    // INSERT>> Your constructor code here.
}


d202ui3TDLGClient::~d202ui3TDLGClient ()
{
    Destroy();

    // INSERT>> Your destructor code here.
}



