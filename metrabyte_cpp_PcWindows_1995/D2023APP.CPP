/*  Project d202ui3
    Radionics Inc.
    Copyright © 1994. All Rights Reserved.

    SUBSYSTEM:    d202ui3.exe Application
    FILE:         d2023app.cpp
    AUTHOR:       Paul L Calinawan


    OVERVIEW
    ========
    Source file for implementation of d202ui3App (TApplication).      
*/


#include <owl\owlpch.h>
#pragma hdrstop


#include "d2023app.h"
#include "2023tdlg.h"                        // Definition of client class.       


//{{d202ui3App Implementation}}


//
// Build a response table for all messages/commands handled
// by the application.
//
DEFINE_RESPONSE_TABLE1(d202ui3App, TApplication)
//{{d202ui3AppRSP_TBL_BEGIN}}
    EV_COMMAND(CM_HELPABOUT, CmHelpAbout),
//{{d202ui3AppRSP_TBL_END}}
END_RESPONSE_TABLE;


//////////////////////////////////////////////////////////
// d202ui3App
// =====
//
d202ui3App::d202ui3App () : TApplication("d202ui3")
{

    // INSERT>> Your constructor code here.
}


d202ui3App::~d202ui3App ()
{
    // INSERT>> Your destructor code here.
}


//////////////////////////////////////////////////////////
// d202ui3App
// =====
// Application intialization.
//
void d202ui3App::InitMainWindow ()
{
    if (nCmdShow != SW_HIDE)
        nCmdShow = (nCmdShow != SW_SHOWMINNOACTIVE) ? SW_SHOWNORMAL : nCmdShow;

    SDIDecFrame *frame = new SDIDecFrame(0, GetName(), 0, false);
    frame->SetFlag(wfShrinkToClient);

    // Override the default window style for the main window.
	frame->Attr.Style |= WS_BORDER | WS_CAPTION | WS_CLIPCHILDREN | WS_MINIMIZEBOX | WS_THICKFRAME | WS_VISIBLE;
	frame->Attr.Style &= ~(WS_CHILD | WS_MAXIMIZEBOX);

	//
	// Assign ICON w/ this application.
	//
	frame->SetIcon(this, IDI_SDIAPPLICATION);

  
	SetMainWindow(frame);


	//
	// Borland Windows custom controls.
	//
	EnableBWCC();
}


//
// Build a response table for all messages/commands handled
// by the application.
//
DEFINE_RESPONSE_TABLE1(SDIDecFrame, TDecoratedFrame)
//{{SDIDecFrameRSP_TBL_BEGIN}}
    EV_WM_CHAR,
//{{SDIDecFrameRSP_TBL_END}}
END_RESPONSE_TABLE;


//{{SDIDecFrame Implementation}}


SDIDecFrame::SDIDecFrame (TWindow *parent, const char far *title, TWindow *clientWnd, bool trackMenuSelection, TModule *module)
	: TDecoratedFrame(parent, title, clientWnd == 0 ? new d202ui3TDLGClient(0) : clientWnd, trackMenuSelection, module)
{
	// INSERT>> Your constructor code here.

}


SDIDecFrame::~SDIDecFrame ()
{
	// INSERT>> Your destructor code here.

}


//////////////////////////////////////////////////////////
// d202ui3App
// ===========
// Menu Help About d202ui3.exe command
void d202ui3App::CmHelpAbout ()
{
}
int OwlMain (int , char* [])
{
	try {
		d202ui3App    app;
		return app.Run();
	}
	catch (xmsg& x) {
		::MessageBox(0, x.why().c_str(), "Exception", MB_OK);
	}

	return -1;
}




void SDIDecFrame::EvChar (uint key, uint repeatCount, uint flags)
{
    TDecoratedFrame::EvChar(key, repeatCount, flags);

	// INSERT>> Your code here.
    	MessageBox("Char Message Received", "D202A Message");


}

