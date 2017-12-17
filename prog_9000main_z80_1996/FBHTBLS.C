

/*****************************************************************************/
/*                                                                           */
/*  MAIN 9000C - FBH Tables                                                  */
/*  11/7/1996                                                                */
/*                                                                           */
/*  RADIONICS  PROPRIETARY                                                   */
/*                                                                           */
/*  Paul Calinawan                                                           */
/*                                                                           */
/*****************************************************************************/

/*

$Header:   G:/5200/9000main.vcs/fbhtbls.c_v   1.0   14 Nov 1996  9:15:22   PAULC  $
$Log:   G:/5200/9000main.vcs/fbhtbls.c_v  $
      
         Rev 1.0   14 Nov 1996  9:15:22   PAULC
      Initial revision.
      
*/


#include <tomahawk.h>   /* in include directory */
#include <global.h>
#include "overlay.h"
#include "MAIN.h"


extern char areas[];
extern char phones[];
extern char panels[];
extern int  PrintIndex;
extern char keypads[];
extern char printers[];


char YesNodata[2][4] = {"Yes","No "};


/* ----------  Ground Start ----------------------------------------- */
char ground_text[2][6] = {"Short",
			  "Long "};

CUSTOMDATA ground_amd[2][1] = {
		{ &phones[1],
		  0x80,
		  1 }
		{ &phones[1],
		  0x80,
		  0 }
		};


char  *ground_tbl[] = {ground_amd, ground_text};

/* ---------- Ac fail/Rst report arrays, power supervision ----------------- */

CUSTOMDATA AcReptAMD[2][2] = {
		{&panels[3],
		 0x40, 1,
		 &panels[3],
		 0x80, 1}
		{&panels[3],
		 0x40, 0,
		 &panels[3],
		 0x80, 0}
		};

char  *AcReptAddPt[] = {AcReptAMD, YesNodata};




/* ---------- Battery fail/restoral report arrays ------------------------ */

CUSTOMDATA BatRptAMD[2][2] = {
		{&panels[3],
		 0x20, 1,
		 &panels[3],
		 0x10, 1}
		{&panels[3],
		 0x20, 0,
		 &panels[3],
		 0x10, 0}
		};

char  *BatRptAddPt[] = {BatRptAMD, YesNodata};




/* ---------- Printer arrays, printer module -------------------------- */
char Printdata[3][3] = {"17","18","19"};

CUSTOMDATA PrintAMD[3][1] = {
		{&PrintIndex,
		 0xFF, 0}
		{&PrintIndex,
		 0xFF, 1}
		{&PrintIndex,
		 0xFF, 2}
		};

char  *PrintAddPt[] = {PrintAMD, Printdata};




/* ---------- Scope for SDI device -------------------------- */
char Scopedata[5][11] = {"No Printer",
			 "Area Wide ",
			 "Account   ",
			 "Panel Wide",
			 "Custom    " };

CUSTOMDATA ScopeAMD[5][1] = {
		{&printers[0],
		 0x70, 0}
		{&printers[0],
		 0x70, 1}
		{&printers[0],
		 0x70, 2}
		{&printers[0],
		 0x70, 3}
		{&printers[0],
		 0x70, 4}
		};

char  *ScopeAddPt[]= {ScopeAMD, Scopedata};




/* ---------- keypads           -------------------------- */
char keypadsdata[5][11] = {"No KeyPad ",
			   "Area Wide ",
			   "Account   ",
			   "Panel Wide",
			   "Custom    "};

CUSTOMDATA keypadsAMD[5][1] = {
		{&keypads[0],
		 0x70, 0}
		{&keypads[0],
		 0x70, 1}
		{&keypads[0],
		 0x70, 2}
		{&keypads[0],
		 0x70, 3}
		{&keypads[0],
		 0x70, 4}
		};

char  *KeyPadsAddPt[]={keypadsAMD,keypadsdata};




/* ---------- Fire bell output Pattern -------------------------- */
char BellPatdata[4][7] = {"Steady",
			  "Pulse ",
			  "CaStnd",
			  "TmCod3"};


CUSTOMDATA FirePatAMD[4][1] = {
		{&areas[6],
		 0xC0, 0}
		{&areas[6],
		 0xC0, 1}
		{&areas[6],
		 0xC0, 2}
		{&areas[6],
		 0xC0, 3}
		};

char  *FirePatAddPt[]={FirePatAMD, BellPatdata};




/* ---------- Burg bell output Pattern -------------------------- */

CUSTOMDATA BurgPatAMD[4][1] = {
		{&areas[6],
		 0x30, 0}
		{&areas[6],
		 0x30, 1}
		{&areas[6],
		 0x30, 2}
		{&areas[6],
		 0x30, 3}
		};

char  *BurgPatAddPt[]={BurgPatAMD, BellPatdata};




/* ----------  Area Type  ---------- */
char AreaTypedata[4][8] = {"Regular",
			  "Master ",
			  "Assoc. ",
			  "Shared "};

CUSTOMDATA AreaTypeAMD[4][3] = {
		{	&areas[4], 
			0x20, 
			0,
			&areas[4], 
			0x08, 
			0,
			&areas[4], 
			0x04, 
			0 }

		{	&areas[4], 
			0x20, 
			0,
			&areas[4], 
			0x08, 
			0,
			&areas[4], 
			0x04, 
			1 }

		{	&areas[4], 
			0x20, 
			0,
			&areas[4], 
			0x08, 
			1,
			&areas[4], 
			0x04, 
			0 }

		{	&areas[4], 
			0x20, 
			1,
			&areas[4], 
			0x08, 
			0, 
			&areas[4], 
			0x04, 
			0 }
};

char  *AreaTypeAddPt[]={AreaTypeAMD, AreaTypedata};






