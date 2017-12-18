/*  Project d202ui4
    Radionics Inc
    Copyright © 1994. All Rights Reserved.

    SUBSYSTEM:    d202ui4.exe Application
    FILE:         d2024app.cpp
    AUTHOR:       Paul L Calinawan


    OVERVIEW
    ========
    Source file for implementation of d202ui4App (TApplication).      
*/


#include <owl\owlpch.h>
#pragma hdrstop

#include <dir.h>

#include "d2024app.h"
#include "d2024wnv.h"                        // Definition of client class.       
#include "d2024abd.h"                        // Definition of about dialog.       


// Drag / Drop support:
TFileDrop::TFileDrop (char* fileName, TPoint& p, bool inClient, TModule*)
{
    char    exePath[MAXPATH];
    
    exePath[0] = 0;
    FileName = strcpy(new char[strlen(fileName) + 1], fileName);
    Point = p;
    InClientArea = inClient;
}

TFileDrop::~TFileDrop ()
{
    delete FileName;
}

const char *TFileDrop::WhoAmI ()
{
  return FileName;
}


//{{d202ui4App Implementation}}



//{{DOC_VIEW}}
DEFINE_DOC_TEMPLATE_CLASS(TFileDocument, d202ui4WindowView, DocType1);
//{{DOC_VIEW_END}}

//{{DOC_MANAGER}}
DocType1 __dvt1("All Files (*.*)", "*.*", 0, "TXT", dtAutoDelete | dtUpdateDir);
//{{DOC_MANAGER_END}}


//
// Build a response table for all messages/commands handled
// by the application.
//
DEFINE_RESPONSE_TABLE1(d202ui4App, TApplication)
//{{d202ui4AppRSP_TBL_BEGIN}}
    EV_OWLVIEW(dnCreate, EvNewView),
    EV_OWLVIEW(dnClose,  EvCloseView),
    EV_COMMAND(CM_HELPABOUT, CmHelpAbout),
    EV_COMMAND(CM_FILEPRINT, CmFilePrint),
    EV_COMMAND(CM_FILEPRINTERSETUP, CmFilePrintSetup),
    EV_COMMAND(CM_FILEPRINTPREVIEW, CmFilePrintPreview),
    EV_COMMAND_ENABLE(CM_FILEPRINT, CmPrintEnable),
    EV_COMMAND_ENABLE(CM_FILEPRINTERSETUP, CmPrintEnable),
    EV_COMMAND_ENABLE(CM_FILEPRINTPREVIEW, CmPrintEnable),
    EV_WM_DROPFILES,
    EV_WM_WININICHANGE,
//{{d202ui4AppRSP_TBL_END}}
END_RESPONSE_TABLE;


//////////////////////////////////////////////////////////
// d202ui4App
// =====
//
d202ui4App::d202ui4App () : TApplication("d202ui4")
{
    Printer = 0;
    Printing = 0;

    SetDocManager(new TDocManager(dmSDI, this));

    // INSERT>> Your constructor code here.
}


d202ui4App::~d202ui4App ()
{
    if (Printer)
        delete Printer;

    // INSERT>> Your destructor code here.
}




void d202ui4App::CreateGadgets (TControlBar *cb, bool server)
{
    if (!server) {
        cb->Insert(*new TButtonGadget(CM_FILENEW, CM_FILENEW));
        cb->Insert(*new TButtonGadget(CM_FILEOPEN, CM_FILEOPEN));
        cb->Insert(*new TButtonGadget(CM_FILESAVE, CM_FILESAVE));
        cb->Insert(*new TSeparatorGadget(6));
    }

    cb->Insert(*new TButtonGadget(CM_EDITCUT, CM_EDITCUT));
    cb->Insert(*new TButtonGadget(CM_EDITCOPY, CM_EDITCOPY));
    cb->Insert(*new TButtonGadget(CM_EDITPASTE, CM_EDITPASTE));
    cb->Insert(*new TSeparatorGadget(6));
    cb->Insert(*new TButtonGadget(CM_EDITUNDO, CM_EDITUNDO));
    cb->Insert(*new TSeparatorGadget(6));
    cb->Insert(*new TButtonGadget(CM_EDITFIND, CM_EDITFIND));
    cb->Insert(*new TButtonGadget(CM_EDITFINDNEXT, CM_EDITFINDNEXT));

    if (!server) {
        cb->Insert(*new TSeparatorGadget(6));
        cb->Insert(*new TButtonGadget(CM_FILEPRINT, CM_FILEPRINT));
        cb->Insert(*new TButtonGadget(CM_FILEPRINTPREVIEW, CM_FILEPRINTPREVIEW));
    }

    // Add fly-over help hints.
    cb->SetHintMode(TGadgetWindow::EnterHints);
}


void d202ui4App::SetupSpeedBar (TDecoratedFrame *frame)
{ 
    //
    // Create default toolbar New and associate toolbar buttons with commands.
    //   
    TControlBar* cb = new TControlBar(frame);
    CreateGadgets(cb);

    // Setup the toolbar ID used by OLE 2 for toolbar negotiation.
    cb->Attr.Id = IDW_TOOLBAR;

    frame->Insert(*cb, TDecoratedFrame::Top);
}


//////////////////////////////////////////////////////////
// d202ui4App
// =====
// Application intialization.
//
void d202ui4App::InitMainWindow ()
{
    if (nCmdShow != SW_HIDE)
        nCmdShow = (nCmdShow != SW_SHOWMINNOACTIVE) ? SW_SHOWNORMAL : nCmdShow;

    SDIDecFrame *frame = new SDIDecFrame(0, GetName(), 0, true, this);

    // Override the default window style for the main window.
    frame->Attr.Style |= WS_BORDER | WS_CAPTION | WS_CLIPCHILDREN | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME | WS_VISIBLE;
    frame->Attr.Style &= ~(WS_CHILD | WS_MAXIMIZEBOX);

    //
    // Assign ICON w/ this application.
    //
    frame->SetIcon(this, IDI_SDIAPPLICATION);

    //
    // Menu associated with window and accelerator table associated with table.
    //
    frame->AssignMenu(SDI_MENU);
    
    //
    // Associate with the accelerator table.
    //
    frame->Attr.AccelTable = SDI_MENU;

    SetupSpeedBar(frame);

    TStatusBar *sb = new TStatusBar(frame, TGadget::Recessed,
                                    TStatusBar::CapsLock        |
                                    TStatusBar::NumLock         |
                                    TStatusBar::ScrollLock      |
                                    TStatusBar::Overtype);
    frame->Insert(*sb, TDecoratedFrame::Bottom);
  
    SetMainWindow(frame);

    frame->SetMenuDescr(TMenuDescr(SDI_MENU));

    //
    // Borland Windows custom controls.
    //
    EnableBWCC();
}


//////////////////////////////////////////////////////////
// d202ui4App
// =====
// Response Table handlers:
//
void d202ui4App::EvNewView (TView& view)
{
    GetMainWindow()->SetClientWindow(view.GetWindow());
    if (!view.IsOK())
        GetMainWindow()->SetClientWindow(0);
    else if (view.GetViewMenu())
        GetMainWindow()->MergeMenu(*view.GetViewMenu());
}


void d202ui4App::EvCloseView (TView&)
{
    GetMainWindow()->SetClientWindow(0);
    GetMainWindow()->SetCaption("d202ui4");
}


//{{SDIDecFrame Implementation}}


SDIDecFrame::SDIDecFrame (TWindow *parent, const char far *title, TWindow *clientWnd, bool trackMenuSelection, TModule *module)
    : TDecoratedFrame(parent, title, clientWnd, trackMenuSelection, module)
{
    // INSERT>> Your constructor code here.

}


SDIDecFrame::~SDIDecFrame ()
{
    // INSERT>> Your destructor code here.

}


//////////////////////////////////////////////////////////
// d202ui4App
// ==========
// Menu File Print command
void d202ui4App::CmFilePrint ()
{
    //
    // Create Printer object if not already created.
    // 
    if (!Printer)
        Printer = new TPrinter(this);

    //
    // Create Printout window and set characteristics.
    //
    APXPrintOut printout(Printer, "Title", GetMainWindow()->GetClientWindow());
    printout.SetBanding(true);

    Printing++;

    //
    // Bring up the Print dialog and print the document.
    //
    Printer->Print(GetWindowPtr(GetActiveWindow()), printout, true);

    Printing--;
}


//////////////////////////////////////////////////////////
// d202ui4App
// ==========
// Menu File Print Setup command
void d202ui4App::CmFilePrintSetup ()
{
    if (!Printer)
        Printer = new TPrinter(this);

    //
    // Bring up the Print Setup dialog.
    //
    Printer->Setup(GetMainWindow());
}


//////////////////////////////////////////////////////////
// d202ui4App
// ==========
// Menu File Print Preview command
void d202ui4App::CmFilePrintPreview ()
{
    SDIDecFrame *sdiFrame = TYPESAFE_DOWNCAST(GetMainWindow(), SDIDecFrame); 
    if (sdiFrame) {
        if (!Printer)
            Printer = new TPrinter(this);

        Printing++;

        PreviewWindow *prevW = new PreviewWindow(sdiFrame, Printer, sdiFrame->GetClientWindow(), "Print Preview", new TLayoutWindow(0));
        prevW->Create();

        BeginModal(GetMainWindow());

        Printing--;

        // Now that printing is off we can invalidate because the edit window to repaint.
        GetMainWindow()->SetRedraw(true);
        GetMainWindow()->Invalidate();

        // We must destroy the preview window explicitly.  Otherwise, the window will not be destroyed until
        // it's parent the MainWindow is destroyed.
        prevW->Destroy();
        delete prevW;
    }
}


//////////////////////////////////////////////////////////
// d202ui4App
// ==========
// Menu enabler used by Print, Print Setup and Print Preview.
void d202ui4App::CmPrintEnable (TCommandEnabler &tce)
{
    // If we have a Printer already created just test if all is okay.
    // Otherwise create a Printer object and make sure the printer
    // really exists and then delete the Printer object.
    if (!Printer) {
        Printer = new TPrinter(this);
        
        tce.Enable(Printer->GetSetup().Error == 0);
    } else
        tce.Enable(Printer->GetSetup().Error == 0);
}


//////////////////////////////////////////////////////////
// d202ui4App
// ===========
// Menu Help About d202ui4.exe command
void d202ui4App::CmHelpAbout ()
{
    //
    // Show the modal dialog.
    //
    d202ui4AboutDlg(GetMainWindow()).Execute();
}


void d202ui4App::InitInstance ()
{
    TApplication::InitInstance();

    // Accept files via drag/drop in the frame window.
    GetMainWindow()->DragAcceptFiles(true);
}


void d202ui4App::EvDropFiles (TDropInfo drop)
{
	// Tell DragQueryFile the file interested in (0) and the length of your buffer.
	int     fileLength = drop.DragQueryFileNameLen(0) + 1;
	char    *fileName = new char[fileLength];

	drop.DragQueryFile(0, fileName, fileLength);

	// Open the file that was dropped.
	AddFile(fileName);
    // Release the memory allocated for this handle with DragFinish.
    drop.DragFinish();
}


void d202ui4App::AddFile (const char *FileName)
{
    TDocTemplate* tpl = GetDocManager()->MatchTemplate(FileName);
    if (tpl)
      tpl->CreateDoc(FileName);
}


void d202ui4App::EvWinIniChange (char far* section)
{
    if (strcmp(section, "windows") == 0) {
        // If the device changed in the WIN.INI file then the printer
        // might have changed.  If we have a TPrinter (Printer) then
        // check and make sure it's identical to the current device
        // entry in WIN.INI.
        if (Printer) {
            char printDBuffer[255];
            LPSTR printDevice = printDBuffer;
            LPSTR devName;
            LPSTR driverName = 0;
            LPSTR outputName = 0;
        
            if (::GetProfileString("windows", "device", "", printDevice, sizeof(printDevice))) {
                // The string which should come back is something like:
                //
                //      HP LaserJet III,hppcl5a,LPT1:
                //
                // Where the format is:
                //
                //      devName,driverName,outputName
                //
                devName = printDevice;
                while (*printDevice) {
                    if (*printDevice == ',') {
                        *printDevice++ = 0;
                        if (!driverName)
                            driverName = printDevice;
                        else
                            outputName = printDevice;
                    } else
                        printDevice = ::AnsiNext(printDevice);
                }

                if ((Printer->GetSetup().Error != 0)                                ||
                    (strcmp(devName, Printer->GetSetup().GetDeviceName()) != 0)    ||
                    (strcmp(driverName, Printer->GetSetup().GetDriverName()) != 0) ||
                    (strcmp(outputName, Printer->GetSetup().GetOutputName()) != 0)) {
                
                    // New printer installed so get the new printer device now.
                    delete Printer;
                    Printer = new TPrinter(this);
                }
            } else {
                // No printer installed (GetProfileString failed).
                delete Printer;
                Printer = new TPrinter(this);
            }
        }
    }
}


int OwlMain (int , char* [])
{
    try {
        d202ui4App    app;
        return app.Run();
    }
    catch (xmsg& x) {
        ::MessageBox(0, x.why().c_str(), "Exception", MB_OK);
    }

    return -1;
}
