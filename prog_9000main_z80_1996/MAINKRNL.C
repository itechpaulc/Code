

/****************************************************************/
/*                                                              */
/*  MAIN 9000C  Kernal file                                     */
/*  11/09/1996                                                  */
/*                                                              */
/*  RADIONICS  PROPRIETARY                                      */
/*                                                              */
/*  Paul Calinawan                                              */
/*                                                              */
/*  mainkrnl.c file:                                            */
/*      Provides the user with an interface for programming     */
/*      9112 parameters.                                        */
/*      Updates the screen;                                     */
/*      displays the menus (parameters);                        */
/*      and processes user key strokes.                         */
/*                                                              */
/****************************************************************/

/*

$Header:   G:/5200/9000main.vcs/mainkrnl.c_v   1.5   01 Dec 1996 14:28:54   PAULC  $
$Log:   G:/5200/9000main.vcs/mainkrnl.c_v  $      
      
         Rev 1.5   01 Dec 1996 14:28:54   PAULC
      Changed Rev # to 00.06.
      
         Rev 1.2   22 Nov 1996 20:32:58   PAULC
      Changed Rev # to 00.03.
      
         Rev 1.1   14 Nov 1996 13:18:42   PAULC
      Changed Rev # to 0.02.
      
         Rev 1.0   14 Nov 1996  9:15:22   PAULC
      Initial revision.
         
*/


#include <tomahawk.h>
#include <global.h>
#include <deflib.h>
#include <omacro.h>
#include <lib.h>

#include "overlay.h"
#include "main.h"




/******************** EXTERNAL DECLARATION *********************/
extern MENUTYPE repl_hdr;               /* last menu item */
extern MENUTYPE save_hdr;               /* save menu item */

extern MENUTYPE filelist;
extern MENUTYPE StrtMenu;
extern MENUTYPE EndFile;

extern MENUTYPE DeletName;              /* file name to be deleted */
extern MENUTYPE CpyToName;              /* file name to be copy to */
extern MENUTYPE CpyFrmName;
extern MENUTYPE SaveName;               /* file name to be saved */
extern MENUTYPE VisMode;
extern MENUTYPE LockRecName;

extern FILETYPE DeletFile;              /* filename to delete */
extern FILETYPE CopyFrmFile;            /* filename to copy from */
extern FILETYPE CopyToFile;             /* filename to copy from */
extern FILETYPE savefils;               /* filename to copy to and save to */
extern FILETYPE replfils;               /* filename to copy to and save to */

extern char     InqPackg[];             /* inquiry package */
extern char     clrscrn[];

extern MENUTYPE grp_1;
extern MENUTYPE LastMenu;

extern MENUTYPE DoorCtrlBeg;
extern MENUTYPE DoorCtrlEnd;
extern MENUTYPE auth_14;

extern MENUTYPE CustRoute;
extern MENUTYPE BegCustRouteMenu;
extern MENUTYPE EndCustRouteMenu;

extern MENUTYPE PrinterScope;
extern MENUTYPE BegPtCustScope;
extern MENUTYPE EndPtCustScope;

extern MENUTYPE KeypadScope;
extern MENUTYPE BegKpCustScope;
extern MENUTYPE EndKpCustScope;




/*******************  GLOBAL VARIABLES **************************/
/*---  Panel Image --------------------------*/
char    areas           [8]     [AREA_SIZE];
char    phones                  [PHONE_SIZE];
char    panels                  [PANEL_SIZE];
char    commands        [16]    [COMMAND_SIZE];
char    routing         [4]     [ROUTING_SIZE];
char    menus           [64]    [MENU_SIZE];
char	authority		[80]	[AUTHORITY_SIZE];

/***************************************************************************
**      "devices" is separated into "keypads", "printers" and "automation"
**      char    devices         [24]    [DEVICE_SIZE];
**      WARNING: The order of variables in the following section cannot be 
                 rearranged.
**
**      SECTION BEGIN
*/
char    keypads         [8]     [DEVICE_SIZE];
char    printers        [3]     [DEVICE_SIZE];
char    automation      [1]     [DEVICE_SIZE];
/*
**      SECTION END
**
***************************************************************************/

char    KRNL_VIS_ARRAY          [VIS_SIZE];
char    SYS_VIS_ARRAY           [VIS_SIZE];

/*---  End of Panel Image ------------------*/


char     data_lock      [2];                    /* item 0; not really part of panel image */


/*---  Index variables    ------------------*/
int    AreaIndx         = 0;
int    CmdIndx          = 0;
int    RouteIndx        = 0;
int    AuthIndex        = 0;
int    SkedIndex        = 0;
int    DayIndex         = 0;
int    KypdIndex        = 0;
int    PrintIndex       = 0;

int    UserIndx         = 1;
int    menu_indx        = 0;
int    custom_indx      = 128;

char   item;                            /* used during send/recv */
char   sub_item;                        /* used during send/recv */
char   block_cnt;                       /* used during send/recv */

char   comm_during_idle = FALSE;        /* no communications during idle */

char   send_buff [255];                 /* sdi communications */
char   recv_buff [255];                 /* sdi communications */

int    Timeout;                         /* minutes to timeout */


/*************** SYSTEM VARIABLES  REQUIRED FOR ALL HANDLERS *********/
MENUTYPE *BegMenu = {&StrtMenu};        /* Pointer to menu table */
char    Files      [1250];              /* 100 12 char record names + extra */
char    Codes      [104];               /* buffer to hold lock codes */
char    StatusLine [80];                /* Display status line */
char    MessagLine [21];                /* message line display */
char    FileName   [13];                /* buffer to hold file name */
char   *FileExt;                        /* not used in handler */
char    DataField  [40];                /* buffer to hold data */
char    StopNavig=  FALSE;              /* flag used for disabling navigations*/
char    HelpFlag =  FALSE;              /* flag used to detect the help mode*/
char    WarnFlag =  FALSE;              /* key input timeout warning flag */
char    Header     [] = {"DELETE      COPY        VISMODE     LOCKRECORD  "};
char    HandName   [] = {"9000MAIN"};   /* Data files to search for */
char    CurDisk;                        /* Current disk */

int     retmenu [30];                   /* return menu stack */
int     retindx = {0};                  /* menu stack index */
int     CurMenu;                        /* pointer to current menu */
int     CurTOW;                         /* pointer to current top of window */
int     currow=2;                       /* row cursor is on. */
int     curcol=0;                       /* col cursor is on */
int     IndxStr[6];                     /* buffer to hold displayable indx */
int     TOWHelp=0;                      /* top of the help window counter */
int     LineNo=0;                       /* help bullet line counter */
int     StickyTen = {1};                /* non-zero value ensures low bat func*/
int     ParamBegin;                     /* begin of parameter list */
int     ParamEnd;                       /* end of parameter list */
int     SaveHdr;                        /* menu number of the SAVE prompt*/
int     RecListStrt;                    /* begin of list of record names */
int     OLDCRC;                         /* previous CRC value */
int     LockMode=FALSE;
char    visinvis = FALSE;
char    LockName[12];
int     in_copy_del_vis_lock = FALSE;

char    help_text [MAX_HELP];           /* help text buffer */
char    thelp_buff[MAX_HELP];           /* temporary help text buffer */
unsigned int  help_size = MAX_HELP;     /* size of help message */

char *SaveTimeOut = "SAVE TIMEOUT RECORD";

AutoIdx FileIndx = {0,0,0,Files};      /* pointer to file, set tow, and count*/
char   HandlrHdr[]        =    "9000MAIN [00.06]";
char   HandlrPrefix[]     =    "9000MAIN:";
char   Blanks_12[]        =    "            ";
char   Blanks14[]         =    "              ";
char   *CommMsg[]         =   { SendSucc,
				SendFail,
				ReceiveSucc,
				ReceiveFail,
				"Plug In 9000",
				"COMM ERROR",
				"UNKNOWN VERSION",
				InvalidLcode,
				"Incompatible Panel",
				"Check Cord/Reset Pin",
				"Recv Fail: DataLock",
				"Send Fail: DataLock",
				"Recv Req Bad Chksum",
				"Recv Rpy Bad Chksum",
				"Recv Req No Msg",
				"Recv Rpy No Msg"

			      };

AUTH_ELEM AuthTable[]     =   {  /** Order is last to first w.r.t Header[] **/
				 /** AUTHORITY          MENU POINTER **/
				 { AUTH_LOCK_RECS*256,  &LockRecName },
				 { AUTH_VISMODE,        &VisMode     },
				 { AUTH_COPY,           &CpyFrmName  },
				 { AUTH_DEL_REC,        &DeletName   }
				};




/*************** More 9000 handler variables ******************************/
char    Dates      [DATE_ARRAY_SIZE];   /* buffer to hold holiday index dates */
AutoIdx DateIndx = {0,0,0,Dates};       /* pointer to Holiday index, set tow, and count*/
int     in_cdvl  = FALSE;               /* in copy delete visible or lockmode */
int     HolidayIndxAddr = 0;            /* should be character */

char   code_minus_2 [3];                /* used by ACPT_PASS for duress check */
char   code_minus_1 [3];
char   temp_code    [3];
char   code_plus_1  [3];
char   code_plus_2  [3];
char   *code_tbl    [5] {&code_minus_2 [0],
			 &code_minus_1 [0],
			 &temp_code    [0],
			 &code_plus_1  [0],
			 &code_plus_2  [0]};




char Month = 0;
char Day = 0;

char   *menu_addr;                     /* used by all custom formats */
char   menu_msk;                       /* used by all custom formats */
char   menu_len;                       /* used by all custom formats */
char   menu_bits;                      /* used by all custom formats */
char   *menu_prompt;                   /* used by all custom formats */




/***   FILE TYPES        ******************************************************/
/* file descriptors need to be here because the old compiler can initialize */
/* structures, while new one does not                                   */

FILETYPE CopyFrmFile ={ "              ",
			&areas[0],
			RECORD_SIZE};
FILETYPE CopyToFile  ={ "              ",
			&areas[0],
			RECORD_SIZE};
FILETYPE DeletFile   ={ "              ",
			&areas[0],
			RECORD_SIZE};
FILETYPE savefils    ={ "              ",
			&areas[0],
			RECORD_SIZE};
FILETYPE replfils    ={ "              ",
			&areas[0],
			RECORD_SIZE};











	/************************************************************/
	/*                                                          */
	/*   Name: MAIN ()                                          */
	/*   Function: Start the display of the data files.         */
	/*     Handle all key input at all levels (data file,       */
	/*     param header, actual param) until the user hits      */
	/*     exit group at the data file level; then return to    */
	/*     caller.                                              */
	/*     Also, while waiting for key input do a system check  */
	/*     and a panel status check                             */
	/*                                                          */
	/*   Inputs:                                                */
	/*                                                          */
	/*   Outputs: None                                          */
	/*   Globals affected:   none                               */
	/*   Modifications:                                         */
	/*                                                          */
	/************************************************************/
   main()
     {
     INDEX_RECORD    idx_rec;

     ProgVisMode = visinvis = 0;              /* restore vismode */

     fillchr( KRNL_VIS_ARRAY, 0, VIS_SIZE+VIS_SIZE);
     /*
     VisArrayPtr = SYS_VIS_ARRAY;
     ProgVisMode = visinvis = 3;
     */
     HelpSetUp ();

     FileName[0] = DataField[0] = Files[0] = MessagLine[0] = 0;
     RecListStrt = CurMenu = CurTOW = GetMenu(&filelist);
     ParamBegin  = GetMenu(&grp_1);
     ParamEnd    = GetMenu(&LastMenu);
     SaveHdr     = GetMenu(&save_hdr);
     strcpy (StatusLine, HandlrHdr);

     CurDisk = bdos(0x19,0);                    /* get lock codes from drive a:*/
     LoginToDisk(0);
     if ( LoadFile (SystemDat, Codes, LOCK_CODE_SIZE))
       fillchr (Codes, 0, LOCK_CODE_SIZE);
     LoginToDisk( CurDisk );
						 /* valid database? */
     if ( RetRecs() )
       {
       printf("%s\n Press any key to    exit ..........\n", BadDBFile);
       NING();
       getc();
       handexit();
       }
     else
       {
       if (_FIND(HandName, search_name, TimeOutSave, &idx_rec, get_record))
	 {
	 LoadMsg(SaveTimeOut);
	 NING();
	 }
       else
	 *MsgPtr=0;

       WriteScreen();
       Timeout = ProgTimeOut* 600;

       do {
	 LoadGPT( Timeout );                    /* load General Purpose Timer */
	 while ( !constat() )                   /* loop till we get a key*/
	   {
	   idle_comm ();                        /* idle comm with panel */
	   if ( SystemChk() )
	     {
	     RecordLock (HandName, TimeOutSave, 0);        /* unlock TimeOutSave file */
	     if ( in_a_record() )                  /* inside a record ? */
	       SaveRecord (TimeOutSave);           /* yes; save record */
	     while (TRUE)
	       outport(AUX_OUT,0);                 /* force Tomahawk to shut off */
	     }
	   }  /* end while constat */

	 }
       while (! ProcessInput (getc()) );         /* handle key just pressed */

       }  /* else retrec() */

     CloseHelp ();
     handexit();                           /* exit handler !!!! */
     } /* end main */




	/*********************************************************************
	**   Name:      ProcessInput();
	**   Function:
	**   Inputs:    Key pressed by user
	**   Outputs:   It will return a FALSE to wait for more input.
	**              It will return a TRUE to exit the handler.
	**   Globals affected:  Entire panel parameter image.
	**   Modifications:     Created
	**********************************************************************/
   int ProcessInput(inchar)
     char    inchar;
     {
     char  rewrite_scr;                    /* if TRUE then re-write screen */
     char  msg_idx;                        /* msg index */
     char  ii;
     int   ret_value;                      /* code returned from AMENU */
     int   idx;                            /* index variable */

     static int crc();                     /* sub-function: checksum params    */
     static int at_rec_name_lvl();         /* sub-function: cursor at a record name level */
     static int at_a_record_name();        /* sub-function: cursor at an actual record name */
     static     set_rewrite_scr();         /* sub-function: rewrite the screen */


     if (WarnFlag)                         /* in timeout warning ? */
       {
       WarnFlag = FALSE;
       WriteScreen();                      /* re-wrtie to clear warn message */
       }

     rewrite_scr = FALSE;
     switch ( ret_value = AMENU(inchar))   /* handle key - normal processing */
       {
       case EXIT_HANDLER:
	 PIP();
	 return(TRUE);                     /* EXIT THE HANDLER !!!!!!!!!! */

       case GOODENTER:
                                        /* Not a traditional 5200 operation */
     set_side_effects();                /* Update scoping on every */
                                        /* valid key entry */

	 PIP();
	 set_rewrite_scr();
	 break;

       case GOODCHAR:
	 break;

       case NING_ENTER:
	 NING();
	 set_rewrite_scr();
	 goto L_End;

	 /* case BADENTER, BADCHAR: */
       default:
	 TWEEDLE();
	 goto L_End;
       }



     switch (inchar)
       {
       case  HELP_KEY:
	 if (HelpFlag)
	   PIP();                                  /* PIP on entering help */
	 break;


       case  UP_ARROW_KEY:
       case  DN_ARROW_KEY:                         /* arrow keys if in help; scroll help */
	 if (!HelpFlag)
	   set_rewrite_scr();
	 break;


       case  SEND_KEY:                             /* send params */
	 if ( StopNavig || !in_a_record() )        /* have un-entered data or no in a record ? */
	   TWEEDLE();                              /* yes; disallow send */
	 else
	   {
	   set_rewrite_scr();
	   HelpFlag = FALSE;
	   CtrString( 1, Sending);                 /* display message */

	   comm_during_idle = TRUE;                /* ok to comm during idle */
	   if (!idle_comm())                       /* panel present ? */
	     {
	     NING();
	     msg_idx = PLUG_IN_IDX;                /* no */
	     }
	   else
	     if ( (msg_idx = SendParameter()) == SEND_SUCC_IDX )  /* sent params ok ? */
	       PIP ();
	     else
	       {
	       comm_during_idle = FALSE;           /* no comm during idle */
	       NING();
	       }

	   DisplayMsg (msg_idx);                   /* display the message */
	   }
	 break;


       case  RECV_KEY:
	 ResetStrings();
	 if ( at_a_record_name() )                 /* cursor at valid record name ? */
	   {
	   set_rewrite_scr();
	   HelpFlag = FALSE;
	   CtrString( 1, Receiving);               /* display message */
						   /* load file to get Visarray */
	   if ( !DB_LOAD(HandName, FileName, areas) )
	     {
	     DisplayMsg (RECV_FAIL_IDX);           /* failed to load file */
	     NING ();
	     break;
	     }

	   comm_during_idle = TRUE;                /* ok to comm during idle */
	   if (!idle_comm())                       /* panel present ? */
	     {
	     NING();
	     msg_idx = PLUG_IN_IDX;                /* no */
	     }
	   else
	     if ( (msg_idx = RecvParameter()) == RECV_SUCC_IDX )  /* recv'd params ok ? */
	       {
	       PIP();
	       EnterGroup();                         /* enter into the record */
	       CurTOW = CurMenu = ParamBegin;        /* set menu ptr */
	       currow = 2;
	       SmashRam();
	       OLDCRC = crc()+251;                   /* for "exit to save"  */
	       }
	     else                                    /* recv failure */
	       {
	       comm_during_idle = FALSE;             /* no comm during idle */
	       NING();
	       }

	   DisplayMsg (msg_idx);                   /* display message */
	   }
	 else
	   TWEEDLE();
	 break;


       case  CANCEL_KEY :
	 if (HelpFlag)
	   {
	   LineNo = curcol = 0;           /*will be put in DoHelp*/
	   PIP();
	   DoHelp();
	   }
	 else
	   set_rewrite_scr();
	 break;


       case  ENTER_KEY :
	 set_rewrite_scr();
	 break;


       case  ENTER_GRP_KEY :
	 ResetStrings();                           /* reinitialize strings */
	 if ( at_rec_name_lvl () )
	   {
	   SetStatusLine();
	   if ( at_a_record_name() )               /* at an actual "recordname" ? */
	     {
	     DB_LOAD (HandName, FileName, areas);
	     SmashRam();
	     OLDCRC  = crc();                      /* update OLDCRC */
	     CurMenu = ParamBegin;                 /* position menu ptr */
	     }
	   else
	     {      /* at COPY, DELETE, VISMODE or LOCKREC */
	     in_copy_del_vis_lock = TRUE;
	     idx = strlen( Files )/12 - FileIndx.CurIdx -1;
	     if (UserAuthorized( AuthTable[idx].auth ))
	       {
	       ProgVisMode = FALSE;
	       CurMenu = GetMenu(AuthTable[idx].menu_ptr);
	       }
	     else
	       NORMNAVIG( EXIT_GRP_KEY );
	     }

	   CurTOW = CurMenu;
	   set_rewrite_scr();
	   }
	 break;



       case  EXIT_GRP_KEY :
	 if ( at_rec_name_lvl () )
	   {
	   currow = 2;                             /* position cursor */
	   if (  !in_copy_del_vis_lock             /* if cursor points to record  */
	     &&  !(MenuItemInvis(&SaveHdr))        /* ...and Save prompt is visible */
	     &&  OLDCRC != crc() )                 /* .. and user added data ? */
	     {
	     EnterGroup();                         /* enter record */
	     CurTOW = CurMenu = SaveHdr;           /* position at save */
	     strcpy (savefils.FileName, Blanks_12);
	     AMENU( ENTER_GRP_KEY );               /* enter save menu */
	     set_rewrite_scr();
	     break;
	     }

	   in_copy_del_vis_lock = FALSE;
	   FileIndx.TowIdx = FileIndx.CurIdx = 0;             /* position cursor at top */
	   if ( (ProgVisMode & 1) && !SaveRecord(FileName) )  /* automatic save for vismode*/
	     NING();
	   }  /* at_rec_name_lvl */

	 ProgVisMode = visinvis;                   /* restore vismode */
	 LockMode    = FALSE;                      /* reset LockMode */

	    /* update OLDCRC if user has entered SAVE or REPLACE */
	 if ( CurMenu == SaveHdr || CurMenu == GetMenu(&repl_hdr) )
	   OLDCRC = crc();

	    /* position at next prompt */
	 if (  (BegMenu+CurMenu)->DataType  == ENTERTO)
	   {  /* if exiting a special sked menu; then change to the correct return menu */
		if ((CurMenu > GetMenu(&BegCustRouteMenu)) && (CurMenu < GetMenu(&EndCustRouteMenu)))
		{
			currow = 2;
			CurTOW = CurMenu = GetMenu(&CustRoute);
		}

		if ((CurMenu > GetMenu(&BegPtCustScope)) && (CurMenu < GetMenu(&EndPtCustScope)))
		{
			currow = 2;
			CurTOW = CurMenu = GetMenu(&PrinterScope);
		}

		if ((CurMenu > GetMenu(&BegKpCustScope)) && (CurMenu < GetMenu(&EndKpCustScope)))
		{
			currow = 2;
			CurTOW = CurMenu = GetMenu(&KeypadScope);
		}

	   if ((CurMenu > GetMenu(&DoorCtrlBeg)) && (CurMenu < GetMenu(&DoorCtrlEnd)))
	   {
	     currow = 2;
	     CurTOW = CurMenu = GetMenu(&auth_14);
	   }
	    
	   NORMNAVIG(DN_ARROW_KEY);
	   }

	 set_rewrite_scr();
	 break;

       } /* end switch (inchar)  */




    L_End:
     if ( rewrite_scr )
       {
       if ( at_rec_name_lvl () )
	 strcpy (StatusLine, HandlrHdr);
       WriteScreen();                    /* display current menu item */
       }

     return(FALSE);
     /* end of process input is below sub-functions */


/*****************************************************************************/
/***********   SUBFUNCTIONS    ***********************************************/
/*****************************************************************************/

      /** refresh screen -- mathew c. **/
  subfunc set_rewrite_scr:
    {
    rewrite_scr = TRUE;
    }

      /** COMPUTE crc   --  mathew c. **/
  subfunc crc:
    {
    ckscrc( areas, IMAGE_SIZE );
    }


      /** at the record name level  **/
  subfunc at_rec_name_lvl:
    {
    CurMenu == RecListStrt;
    }


      /** at the record name level and not at COPY, DELETE, LOCKMODE, VISMODE **/
  subfunc at_a_record_name:
    {
    CurMenu == RecListStrt && FileIndx.CurIdx < (strlen(Files)/12 - 4);
    }


    } /* end ProcessInput */







	/**********************************************************************
	**
	**  Name :    ResetStrings( )
	**  Function: initialiaze name strings in SAVE, DELETE, COPY, REPLACE and
	**           current FileName
	**  Inputs:   none
	**  Outputs:  none
	**  Globals affected: above mentioned strings
	**  Created:  Mathew Chacko, Sept/17/90
	**  Modified:
	***********************************************************************/
   ResetStrings()
     {
     memcpy(FileName,&Files[FileIndx.CurIdx*12],12);
     FileName[12] = '\0';
     strcpy (replfils.FileName,    FileName);
     strcpy (savefils.FileName,    Blanks_12);
     strcpy (DeletFile.FileName,   Blanks_12);
     strcpy (CopyFrmFile.FileName, Blanks14);
     strcpy (CopyToFile.FileName,  Blanks14);
     }






	/*************************************************************************
	**
	**  Name :    SetStatusLine( )
	**  Function: set the status line to appropriate heading and remove padding
	**  Inputs:   none
	**  Outputs:  none
	**  Globals affected: StatusLine
	**  Created:  Mathew Chacko Sept/6/90
	**  Modified:
	**************************************************************************/
     SetStatusLine()
       {
       strcpy( StatusLine, HandlrPrefix );
       strcat( StatusLine, FileName );
       RemoveTrailBlanks( StatusLine );
       }




	/************************************************************************
	**   Name:       in_a_record()
	**   Function:   Checks to see if user is currently positioned in a record.
	**   Inputs:     None
	**   Outputs:    TRUE    if user is in record
	**               FALSE   otherwise
	**   Globals Affected:  None
	**   Created:    Mathew Chacko  Nov/1/90
	**   Modified:
	*************************************************************************/
   int in_a_record()
     {
     return( CurMenu >= ParamBegin && CurMenu <= ParamEnd );
     }




	/************************************************************************
	**   Name:       SaveRecord()
	**   Function:   Try saving the record.  If this fails try replacing the record.
	**   Inputs:     None
	**   Outputs:    TRUE    if able to save or replace record
	**               FALSE   otherwise
	**   Globals Affected: None
	**   Created:    Mathew Chacko Dec/5/90
	**   Modified:
	*************************************************************************/
   SaveRecord( recname )
     char *recname;
     {
     return ( DB_SAVE( HandName, recname, areas, RECORD_SIZE, READ_WRITE)
       ||     DB_REPLACE( HandName, recname, areas, RECORD_SIZE, READ_WRITE) );
     }







	/************************************************************************
	**   Name:       EnterGroup()
	**   Function:   Enters the group the cursor is currently positioned at and
	**               resets various system strings.
	**   Inputs:     None
	**   Outputs:    None
	**   Globals Affected: StatusLine and strings affected by ResetStrings
	**   Created:    Mathew Chacko  Dec/5/90
	**   Modified:
	*************************************************************************/
   int EnterGroup()
     {
     AMENU( ENTER_GRP_KEY );

     ResetStrings();
     SetStatusLine();
     }





     /*************************************************************************
     **  Name :    DisplayMsg (idx):
     **  Function: Dsiplay the message from CommMsg using the index passed
     **  Inputs:   offset to message
     **  Outputs:
     **  Globals affected: MessagLine get new string
     **  Created:   mbm
     **  Modified:  mc Dec/5/90  -- optimization
     **************************************************************************/
   DisplayMsg (idx)
       char idx;
       {
       strcpy (MessagLine, CommMsg[idx]);
       }


