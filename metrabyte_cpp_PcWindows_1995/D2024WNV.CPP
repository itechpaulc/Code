/*  Project d202ui4
    Radionics Inc
    Copyright © 1994. All Rights Reserved.

    SUBSYSTEM:    d202ui4.exe Application
    FILE:         d2024wnv.cpp
    AUTHOR:       Paul L Calinawan


    OVERVIEW
    ========
    Source file for implementation of d202ui4WindowView (TWindowView).      
*/


#include <owl\owlpch.h>
#pragma hdrstop

#include "d2024app.h"
#include "d2024wnv.h"

#include <stdio.h>


//{{d202ui4WindowView Implementation}}


//
// Build a response table for all messages/commands handled
// by d202ui4WindowView derived from TWindowView.
//
DEFINE_RESPONSE_TABLE1(d202ui4WindowView, TWindowView)
//{{d202ui4WindowViewRSP_TBL_BEGIN}}
    EV_WM_GETMINMAXINFO,
//{{d202ui4WindowViewRSP_TBL_END}}
END_RESPONSE_TABLE;


//////////////////////////////////////////////////////////
// d202ui4WindowView
// ==========
// Construction/Destruction handling.
d202ui4WindowView::d202ui4WindowView (TDocument& doc, TWindow* parent)
    : TWindowView(doc, parent)
{
    // INSERT>> Your constructor code here.

}


d202ui4WindowView::~d202ui4WindowView ()
{
    // INSERT>> Your destructor code here.

}


//
// Paint routine for Window, Printer, and PrintPreview for a TWindowView client.
//
void d202ui4WindowView::Paint (TDC&, bool, TRect& rect)
{
    d202ui4App *theApp = TYPESAFE_DOWNCAST(GetApplication(), d202ui4App);
    if (theApp) {
        // Only paint if we're printing and we have something to paint, otherwise do nothing.
        if (theApp->Printing && theApp->Printer && !rect.IsEmpty()) {
            // Use pageSize to get the size of the window to render into.  For a Window it's the client area,
            // for a printer it's the printer DC dimensions and for print preview it's the layout window.
            TSize   pageSize(rect.right - rect.left, rect.bottom - rect.top);

            TPrintDialog::TData &printerData = theApp->Printer->GetSetup();

            // Compute the number of pages to print.
            printerData.MinPage = 1;
            printerData.MaxPage = 1;

            // INSERT>> Special printing code goes here.

        } else {
            // INSERT>> Normal painting code goes here.

        }
    }
}


void d202ui4WindowView::EvGetMinMaxInfo (MINMAXINFO far& minmaxinfo)
{
    d202ui4App *theApp = TYPESAFE_DOWNCAST(GetApplication(), d202ui4App);
    if (theApp) {
        if (theApp->Printing) {
            minmaxinfo.ptMaxSize = TPoint(32000, 32000);
            minmaxinfo.ptMaxTrackSize = TPoint(32000, 32000);
            return;
        }
    }
    TWindowView::EvGetMinMaxInfo(minmaxinfo);
}
