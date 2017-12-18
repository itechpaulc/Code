



// -----------------------------------------------------
// Designed By  :       Paul Calinawan
// Date         :       April 25, 1995
// -----------------------------------------------------
// Compiler     : BORLAND 4.5 C/C++
// -----------------------------------------------------
// Purpose      : A class object for the metrabyte 
//                D202 Metrabyte Keypad User Interface
// -----------------------------------------------------        
// Rev          : 00.50 : First QRT library release
// -----------------------------------------------------

  
// -------------------------------------------------------------
// Copyright (c)    :   by Radionics Inc., 1995
// -------------------------------------------------------------



// --------------------------------------------------------
// Creates a d202 Radionics Keypad as a main window.
// Used as a GUI for controlling the actual keypad 
// connected on the metrabyte system

#include <owl\owlpch.h>


#include "d202ui2.rc"

#include "metclass.h"
#include "kpdclass.h"

// --------------------------------------------------------
// The application class.

class TD202App : public TApplication
{
public:
   TD202App(): TApplication() {}
   void InitMainWindow();
};

// --------------------------------------------------------
// The dialog class.

class TKpadWindow : public TDialog
{
public:
   TKpadWindow(TWindow *parent, TResId resId) :
	  TWindow((TWindow*)0),
	  TDialog(parent, resId)  
	  {
			cntrl = new mem32(0);

			MetKeypad = new D202LowerMem32(cntrl);
	  };

protected:
   
	void 			CmConfig(void);
	void			CmAbout(void);

	virtual BOOL	CanClose(void);

	void   	KeypadKey(char key);

	LRESULT  	EvCommand(UINT, HWND, UINT);

	HWND currButton;

	mem32			*cntrl;
	D202LowerMem32	*MetKeypad;

   DECLARE_RESPONSE_TABLE(TKpadWindow);
};

DEFINE_RESPONSE_TABLE1(TKpadWindow, TDialog)
   EV_WM_TIMER,
   EV_COMMAND(CM_CONFIG, CmConfig),
   EV_COMMAND(CM_ABOUT, CmAbout),       
END_RESPONSE_TABLE;


// --------------------------------------------------------
// TKpadWindow::CmConfig, About, Exit(), CanClose()

void TKpadWindow::CmConfig() {

   MessageBox("Configuration is not yet implemented", "Message", 
				MB_OK | MB_ICONQUESTION);
//				MB_ICONEXCLAMATION);
}

void TKpadWindow::CmAbout() {

   TDialog *dialog = new TDialog(this, D202ABOUT);
   dialog->Execute();
}

BOOL	TKpadWindow::CanClose() {

	TDialog *pDlg = new TDialog(this, D202AEXIT);

	return (pDlg->Execute() == IDOK)
		? TRUE : FALSE ;
} 

LRESULT
TKpadWindow::EvCommand(UINT id, HWND hWndCtl, UINT notifyCode)
{
  if (hWndCtl != 0 && notifyCode == BN_CLICKED)
  {
		KeypadKey(char(id));  										// button notification
  }
		else 
  if (hWndCtl == 0 && notifyCode == 1) 								// accelerator key
  {
	KeypadKey(char(id));
  }

 return TDialog::EvCommand(id, hWndCtl, notifyCode);
}


// --------------------------------------------------------
// Process D202 Keypad key.
//      

void	TKpadWindow::KeypadKey(char key)
{
	int onTm, offTm;

	DWORD strt, curr, diff;

	  strt = GetTickCount();	

	  MetKeypad->GetTimer(onTm, offTm);

	  currButton = GetDlgItem(toupper(key));

	  if (currButton) {
		::SendMessage(currButton, BM_SETSTATE, 1, 0);

		//::MessageBeep(-1);

			MetKeypad->Keypress(key,  HOLD);

		do { 	curr = GetTickCount();
				diff = curr - strt;
		} while(diff < onTm);               

			MetKeypad->Keypress(key, RELEASE);

		::SendMessage(currButton, BM_SETSTATE, 0, 0);

		do { 	curr = GetTickCount();
				diff = curr - strt;
		} while(diff < offTm);
	}
}


// --------------------------------------------------------
// TD202App::InitMainWindow()

void TD202App::InitMainWindow() {
		
	TWindow *KeypadDialog = 
	  new TKpadWindow(0, D202KPAD); 					// Construct the Keypad GUI front end box.


	KeypadDialog->Attr.AccelTable = D202ACCEL;

		TFrameWindow *mainWndw =						// Construct the main window with the
														// KeypadDialog box as the client window.
	  new TFrameWindow(0, "D202 Metrabyte GUI",
	  KeypadDialog, TRUE);                                                

	SetMainWindow(mainWndw);
	EnableBWCC();

	mainWndw->AssignMenu(D202MENU_1);
	mainWndw->SetIcon(this, D202ICON);
	mainWndw->Attr.Style &= ~(WS_MAXIMIZEBOX | WS_THICKFRAME);  
}



// --------------------------------------------------------
// OwlMain()

int OwlMain(int, char*[]) {


   return TD202App().Run();
}

