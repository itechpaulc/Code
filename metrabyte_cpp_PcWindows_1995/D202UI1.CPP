


// -------------------------------------------------------------
// Designed by  :       Paul Calinawan
// Date         :       April 9, 1995
// -------------------------------------------------------------
// Compiler     : BORLAND 4.0 c/c++
// -------------------------------------------------------------
// Rev 00.05    : First QRT library release
// -------------------------------------------------------------
// Copyright (c)    :   by Radionics Inc., 1993,1994, 1995
// -------------------------------------------------------------


#include <owl\owlpch.h>
#include <owl\applicat.h>
#include <owl\dc.h>
#include <owl\dialog.h>
#include <owl\framewin.h>
#include <cstring.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define min(a,b)   (((a) <(b)) ? (a) :(b))

const char AppName[] = "D202A"; // Name of app, window title & icon resource id

const int DisplayDigits = 17;  // number of digits in keypad display

const int ID_DISPLAY = 400;    // control ID of display static text


//----------------------------------------------------------------------------

//
// Calculator application object
//
class TD202KeypadApp : public TApplication {

  public:

	TD202KeypadApp(const char far* name) : TApplication(name) {}

	void   InitMainWindow();
};


// Calculator dialog window object
//
class TD202Keypad : public TDialog {

  public:

	char        Display[DisplayDigits + 1];
	TBrush      BlueBrush;

	TD202Keypad();

	void          FlashButton(char key);

	virtual void  UpdateDisplay();

	void          CheckFirst();
	void          InsertKey(char key);
	void          KeypadKey(char key);

  protected:
	//
	// override EvCommand() defined by class TWindow
	//
	LRESULT       EvCommand(UINT, HWND, UINT);

	void          EvPaint();

	//
	// message response functions
	//
	HBRUSH        EvCtlColor(HDC, HWND hWndChild, UINT ctlType);

  DECLARE_RESPONSE_TABLE(TD202Keypad);
};

DEFINE_RESPONSE_TABLE1(TD202Keypad, TDialog)
  EV_WM_PAINT,
  EV_WM_CTLCOLOR,
END_RESPONSE_TABLE;

//
// Calculator constructor.  Create blue brush for D202 Keypad background,
// and do a clear command.
//
TD202Keypad::TD202Keypad()
  : TWindow((TWindow*)0),
	TDialog(0, AppName),
	BlueBrush(TColor(50, 50, 205))
{ 
}

//
// Colorize the D202 Keypad. Allows background to show through corners of
//  buttons, uses yellow text on black background in the display, and sets
//  the dialog background to blue.
//
HBRUSH
TD202Keypad::EvCtlColor(HDC hDC, HWND hWndChild, UINT ctlType)
{
  switch (ctlType) {
	case CTLCOLOR_BTN:
	  SetBkMode(hDC, TRANSPARENT);
	  return (HBRUSH)GetStockObject(NULL_BRUSH);

	case CTLCOLOR_STATIC:
	  SetTextColor(hDC, TColor::LtYellow);
	  SetBkMode(hDC, TRANSPARENT);
	  return (HBRUSH)GetStockObject(BLACK_BRUSH);

	case CTLCOLOR_DLG:
	  SetBkMode(hDC, TRANSPARENT);
	  return (HBRUSH)BlueBrush;

	default:
	  return TDialog::EvCtlColor(hDC, hWndChild, ctlType);
  }
}

//
// Even dialogs can have their backgrounds painted on.  This creates
//  a red ellipse over the blue background.
//
void
TD202Keypad::EvPaint()
{
  TBrush    redBrush(TColor(255, 100, 100));
  TPaintDC  dc(*this);

  dc.SelectObject(redBrush);
  dc.SelectStockObject(NULL_PEN);

  TRect clientRect = GetClientRect();
  clientRect.bottom = clientRect.right;
  clientRect.Offset(-clientRect.right/4, -clientRect.right/4);
  dc.Ellipse(clientRect);
}

//
// Flash a button with the value of Key.  Looks exactly like a
// click of the button with the mouse.
//
void
TD202Keypad::FlashButton(char key)
{
  HWND button = GetDlgItem(toupper(key));

  if (button) {
	::SendMessage(button, BM_SETSTATE, 1, 0);

	for (int more = 0; more < 40 ; more++)		
		for (int delay = 1; delay <= 30000; ++delay)
				   ;

	::SendMessage(button, BM_SETSTATE, 0, 0);
  }
}

//
// here we handle all of the child id notifications (BN_CLICKED from the
// buttons) and all accelerators at once rather than have separate response
// table entries for each...
//
LRESULT
TD202Keypad::EvCommand(UINT id, HWND hWndCtl, UINT notifyCode)
{
  if (hWndCtl != 0 && notifyCode == BN_CLICKED)
	KeypadKey(char(id));  // button notification

  else if (hWndCtl == 0 && notifyCode == 1) {
	//
	// from an accelerator
	//
	FlashButton(char(id));
	KeypadKey(char(id));
  }

  return TDialog::EvCommand(id, hWndCtl, notifyCode);
}

//
// Set Display text to the current value.
//
void
TD202Keypad::UpdateDisplay()
{
  char  str[DisplayDigits + 2];

	str[0] = '\0';

  ::SetWindowText(GetDlgItem(ID_DISPLAY), strcat(str, Display));
}


void
TD202Keypad::CheckFirst()
{
	strcpy(Display, "Last KeyPress ");
}

void
TD202Keypad::InsertKey(char key)
{
  int l = strlen(Display);

  if (l < DisplayDigits) {
	Display[l] = key;
	Display[l + 1] = ' ';
	Display[l + 2] = 0;
  }
}

//
// Process D202 Keypad key.
//
void
TD202Keypad::KeypadKey(char key)
{
  key = (char)toupper(key);

  if (key >= '0' && key <= '9') 
  {
	CheckFirst();   
	InsertKey(key); 
  } 

  else
	switch(key) {

	  case 'K':
		CheckFirst();
		InsertKey('K');   	// clear
		break;

	  case 'A':
		CheckFirst();
		InsertKey(key);    	// abc keys
		break;
				
	  case 'B':
		CheckFirst();
		InsertKey('B');
		break;
				
	  case 'C':
		CheckFirst();
		InsertKey('C');
		break;		  
	}

  UpdateDisplay();
}



//
// Create D202 Keypad as the application's main window.
//

void
TD202KeypadApp::InitMainWindow()
{
  TWindow* D202KeypadWin = new TD202Keypad;

  D202KeypadWin->Attr.AccelTable = AppName;
  
  MainWindow = new TFrameWindow(0, Name, D202KeypadWin, TRUE);

  MainWindow->SetIcon(this, AppName);
  MainWindow->Attr.Style &= ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
}


int
OwlMain(int /*argc*/, char* /*argv*/ [])
{
  return TD202KeypadApp(AppName).Run();
}