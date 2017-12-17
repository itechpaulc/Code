

/**********************************************************************
*
*       Created:        11/06/1996
*       Author:         Paul Calinwan
*       Program:        custom1.c
*       Description:    Custom formats for the 9000 handler.
*
***********************************************************************/

/*

$Header:   G:/5200/9000main.vcs/custom1.c_v   1.4   01 Dec 1996 14:29:16   PAULC  $
$Log:   G:/5200/9000main.vcs/custom1.c_v  $      
      
         Rev 1.4   01 Dec 1996 14:29:16   PAULC
      Door Control in User Interface, Command Center Function is always visible. 
      User Interface, Command Center Function, DISARM, "no edit" feature
      implemented. Made Invisible Test, Honeywell Exclusive , invisible.
      
         Rev 1.1   14 Nov 1996 13:12:40   PAULC
      Added new functions to handle Command Center Function to allow entries
      of P, E and Blank.
      
         Rev 1.0   14 Nov 1996  9:15:24   PAULC
      Initial revision.
        
*/

#include <tomahawk.h>
#include <global.h>
#include <deflib.h>
#include <omacro.h>
#include <lib.h>

#include "overlay.h"
#include "MAIN.h"

#define     MAX_EVENTS      2047


/******************** EXTERNAL DECLARATION *********************/
extern char areas[][AREA_SIZE];
extern char phones[];
extern char routing[][ROUTING_SIZE];
extern char authority[][AUTHORITY_SIZE];
extern char printers[][DEVICE_SIZE];
extern char keypads[][DEVICE_SIZE];

extern char AreaIndx;
extern int  RouteIndx;
extern char UserIndx;
extern int  AuthIndex;
extern int  PrintIndex;
extern int  KypdIndex;

extern MENUTYPE EarlyBegin;
extern MENUTYPE cEarlyBegin;
extern MENUTYPE HourDay;

extern MENUTYPE CustRouteStrt;
extern MENUTYPE BeginCustRoute;
extern MENUTYPE GoCustRoute;

extern MENUTYPE PhoneStrt;
extern MENUTYPE phone4b;
extern MENUTYPE phone5a;
extern MENUTYPE phone5b;

extern MENUTYPE GotoDoorCtrl;
extern MENUTYPE PCGoDrCtrl;
extern MENUTYPE passc_1;
extern MENUTYPE passc_14;

extern MENUTYPE PtCustScope;
extern MENUTYPE KpCustScope;

extern MENUTYPE SendRestOC;
extern MENUTYPE PrintArea;


extern char  *menu_addr;
extern char  menu_msk;
extern char  menu_len;
extern char  menu_bits;
extern char  *menu_prompt;

char   blank_buff  [] = {"   "};
extern char None[];
extern char ClearNone[];



/******************* GLOBAL VARIABLES **************************/
				       /* Element zero is null */
int (*CustFmts[])( ) = { 
			disp_ph_supervision,       acpt_ph_supervision,
			disp_hex_2_lines,          acpt_hex_2_lines,
			disp_area_mask,            acpt_area_mask,
			disp_area_mask,			   acpt_roc_area_mask,
			disp_log_dump,             acpt_log_dump,
			disp_scope_fbh,			   acpt_pt_scope_fbh,
			disp_scope_fbh,			   acpt_kp_scope_fbh,
            disp_area_authority,       acpt_area_authority,
			disp_auth_level,           acpt_auth_level,
			disp_auth_level,		   acpt_dc_auth_lvl,
			disp_acs_alw_lvl,		   acpt_acs_alw_lvl,
			disp_tkn_disa_lvl,		   acpt_tkn_disa_lvl,
			disp_cust_func_lvl,		   acpt_cust_func_lvl,
			disp_relays,               acpt_relays,
			disp_close_time,           acpt_close_time,
			disp_account_text,         acpt_account_text,
			disp_compressed_text,	   acpt_compressed_text,
			disp_time_date,            acpt_time_date,
			disp_func_number,          acpt_func_number,
			disp_cust_route_yn,		   acpt_cust_route_yn,
			disp_prim_indx,			   acpt_prim_indx,
			disp_back_indx,			   acpt_back_indx,
			disp_blank_ff, 			   acpt_blank_ff, 
			disp_bell_test_yn, 		   acpt_bell_test_yn,
            disp_ph_fmt,               acpt_ph_fmt,
				disp_burg_rest_yn,         acpt_burg_rest_yn,
            disp_passcode,              acpt_passcode
};





/* -------------------------------------------------------------
 *   disp_ph_supervsion() _ Calls display decimal.
 * ------------------------------------------------------------- */
void disp_ph_supervision()
{
   DDECIMAL();
}



/* -------------------------------------------------------------
 *   acpt_ph_supervsion()
 *     If data is blank
 *       store a 0 for supervision time and
 *       clear the ph supervision bit
 *     else
 *       store the supervision time just entered and
 *       set the ph supervision bit
 * ------------------------------------------------------------- */
int acpt_ph_supervision (charin)
char charin;
{
   int stat;
   int temp;

   if (charin == ENTER_KEY)
   {
      atoi(DataHold,&temp);
      if (temp % 10)                           /* must be a TENS */
	     return(BADENTER);
   }

   if ( (stat = ADECIMAL(charin)) == GOODENTER)  /* valid entry ? */
   {
      atoi(DataHold,&temp);                    /* yes; convert to integer */
      RES(&phones[0],5);                       /* assume blank data was entered */
      RES(&phones[1],5);
      if( temp != 0 )                          /* if data is not blank */
	  {
	     SET(&phones[0],5);
	     SET(&phones[1],5);
	  }
   }

   return(stat);
}




/* -------------------------------------------------------------
 *  disp_hex_2_lines ()
 *    Used by Phone numbers & key strokes
 *    If phone number
 *       Displays : ' ' when 'F' is encountered.
 *                  '*' when 'B' is encountered.
 *                  '#' when 'A' is encountered.
 *    If key stroke
 *       Displays : ' ' when 'F' is encountered.
 * ------------------------------------------------------------- */
void disp_hex_2_lines ()
{

   char len_offset;
   char temp_buf [32];

   SetUpDisplay();

   /* read data; convert to ascii; store in temp buffer */
   convert_to_ascii (temp_buf, menu_addr, menu_len+menu_len);

   /* move 1st or 2nd line to the display buffer */
   len_offset = 0;
   if (menu_bits & SECOND_LINE)
      len_offset = menu_len;
   memcpy (DataHold, temp_buf+len_offset, menu_len);

    UpdateDisplay();
}




/* -------------------------------------------------------------
 * acpt_hex_2_lines ()
 *   If phone number
 *      Converts '#' to 'A',
 *               '*' to 'B',
 *               ' ' to 'F'
 *      prior to storing in memory.
 *      Also set send RAM failure bit if there is a RAM phone
 *      number and reset it otherwise.
 *
 *      The 24 digit phone number is stored in 12 bytes as packed-hex.
 *      It is displayed and entered in 2 seprate 12 digits segments.
 *      If less then 24 digits are used it is terminated and filled
 *      with 0x0F's.
 *
 *
 *   If key stroke
 *      Converts ' ' to 'F',
 *
 *      The 32 key strokes are stored in 16 bytes as packed-hex.
 *      They are displayed and entered in 2 separate 16 digits segments.
 *      If less then 32 digits are used it is terminated and filled
 *      with F's.
 * ------------------------------------------------------------- */
int acpt_hex_2_lines (charin)
char charin;
{
   char len_offset;
   char temp_buf [32];

   if (charin == ENTER_KEY)
   {
      SetUpAccept();

      /* read old data; convert to ascii; store in temp buffer */
      convert_to_ascii (temp_buf, menu_addr, (menu_len+menu_len));

      /* take the new data just entered (datahold) and overwrite the old data */
      len_offset = 0;
      if (menu_bits & SECOND_LINE)
	     len_offset = menu_len;

      memcpy (temp_buf+len_offset, DataHold, menu_len);

	  /* take the new data; convert to hex; store in panel image */
      convert_to_hex (menu_addr, temp_buf, (menu_len+menu_len));

      return (AUTOINCREMENT());
   }

   if ( iscntrl(charin) )                     /* is it a control character ? */
      return (DoNavEditKeys(charin));

   charin = toupper(charin);
   if (phone_num_menus())                     /* at a phone number menu */
   {
      if (char_in_set (charin, " 0123456789*#CD") )    /* accept these chars */
	 return (NORMENTRY (charin) );
   }
   else
   if (char_in_set (charin, " 0123456789ABCDE") )    /* accept these chars */
	  return (NORMENTRY (charin) );

   return(BADCHAR);
}





/* -------------------------------------------------------------
 *  disp_area_mask ()
 *    This function calculates the mask for this prompt based on
 *    the value of the area index.
 * --------------------------------------------------------------- */
disp_area_mask ()
{
   char *Address;
   char *IndxPtr;

   PRMPT_INDEX(&Address,&IndxPtr);
   SayString(DataHold,(*Address & (GetMenuMask(MPtr) >> AreaIndx)) ? Yes : NoPrmt);
}



/* -------------------------------------------------------------
 *  acpt_area_mask ()
 *    This function calculates the mask for this prompt based on
 *      the value of the area index.
 *    If YES set the Enabled & Passcode Required bits in Authority
 *      Levels (AuthLvls) for the area selected.
 *  Used by Area Wide Parameters; Opening & Closing; Opne/Close Options
 *   Prompts:
 *     Ax Area O/C
 *     Ax Perimeter O/C
 * --------------------------------------------------------------- */
acpt_area_mask (charin)
char charin;
{
   char value;

   if (charin == ENTER_KEY)
   {
      SetUpAccept();

      menu_msk =  menu_msk >> AreaIndx;
      value =  (*DataHold == 'Y') ? 1 : 0;         
      MSK_DATA( value, menu_msk, menu_addr);
      MSK_DATA( value, menu_msk, menu_addr+1);
      

      /* current menu number restricted o/c prompt ? */
      if (*MPtr == GetMenu(&SendRestOC))        
         {
         MSK_DATA( value, menu_msk, menu_addr+4);  /* also change restricted closing */
         MSK_DATA( value, menu_msk, menu_addr+5);  /* also change restricted closing */
         }
      
      return(AUTOINCREMENT());
   }
   else
      return(AYESNO(charin));
}



/* -------------------------------------------------------------
 *  acpt_roc_area_mask ()
 *    This function calculates the mask for this prompt based on
 *      the value of the area index.
 *    If YES set the Enabled & Passcode Required bits in Authority
 *      Levels (AuthLvls) for the area selected.
 *  Used by Area Wide Parameters; Opening & Closing; Opne/Close Options
 *   Prompts:
 *     Ax Restricted O/C
 * --------------------------------------------------------------- */
acpt_roc_area_mask (charin)
char charin;
{
   char value;

   if (charin == ENTER_KEY)
   {
      SetUpAccept();

      menu_msk =  menu_msk >> AreaIndx;
      value =  (*DataHold == 'Y') ? 1 : 0;
	  /* Send Restricted Open */
      MSK_DATA( value, menu_msk, menu_addr);
      MSK_DATA( value, menu_msk, menu_addr+1);
	  /* Send Restricted Close */
	  MSK_DATA( value, menu_msk, menu_addr+2);
	  MSK_DATA( value, menu_msk, menu_addr+3);
      return(AUTOINCREMENT());
   }
   else
      return(AYESNO(charin));
}



/****************************************************************************
 *																			*
 *	disp_log_dump()															*
 *	It translates the number of events to a number of percentages and		*
 *	displays it.  															*
 *																			*
 ****************************************************************************/

disp_log_dump()
{
    int val;
    int percentage;

    SetUpDisplay();

	val = *menu_addr;
	val = val << 8;
	val = val | (0x00FF & *(menu_addr+1));

	fillchr(DataHold, 0x20, menu_len);
    
	percentage = val/(MAX_EVENTS/100);

	if(percentage != 0)
	{
	    itoa(percentage, DataHold);
	}
	
    UpdateDisplay();
}



/****************************************************************************
 *																			*
 *	acpt_log_dump()															*
 *	It accepts a number in percentages and translates that number to the 	*
 *	number of events and stores it in the memory.							*
 *                                                                          *
 ****************************************************************************/

int acpt_log_dump(charin)
char charin;
{
    int  value, percentage;
    char *menu_addr;
    char *IndxPtr;

    if (iscntrl(charin))              
    {
		switch (charin)
		{
		case ENTER_KEY:
				FINDADDRESS(&menu_addr,&IndxPtr);

				atoi(DataHold, &percentage);
				value = (MAX_EVENTS/100) * percentage;   
     
				*menu_addr = value >> 8;
				*(menu_addr+1) = value & 0x00ff;

				return(AUTOINCREMENT());

		    default:

				return(DoNavEditKeys(charin));

		}
    }
    else                              
    {
		if ((!isdigit(charin)) && (charin != ' ')) return(BADCHAR);

		return(NORMENTRY(charin));

    }
}




   
/****************************************************************************
 *																			*
 *	disp_scope_fbh()															*
 *																			*
 ****************************************************************************/

void disp_scope_fbh()
{
	DFBH();
}




   
/****************************************************************************
 *																			*
 *	acpt_pt_scope_fbh()														*
 *	It accepts a fbh type input that represents different scopes of a 		*
 *	printer.  The actual acpt is in the function scope_fbh.					*
 *																			*
 ****************************************************************************/

int acpt_pt_scope_fbh(charin)
char charin;
{
	int stat;
	
	stat = AFBH(charin);
	
	if ( (stat == GOODENTER) && (charin == ENTER_KEY))  /* valid entry ? */
		scope_fbh(&printers[0][0], PrintIndex, &PtCustScope);

	return(stat);
}






/****************************************************************************
 *																			*
 *	acpt_kp_scope_fbh()														*
 *	It accepts a fbh type input that represents different scopes of a 		*
 *	keypad.  The actual acpt is in the function scope_fbh.					*
 *																			*
 ****************************************************************************/
   
int acpt_kp_scope_fbh(charin)
char charin;
{
	int stat;
	
	stat = AFBH(charin);
	
	if ( (stat == GOODENTER) && (charin == ENTER_KEY)) /* valid entry ? */
		scope_fbh(&keypads[0][0], KypdIndex, &KpCustScope);

	return(stat);
}






/* -------------------------------------------------------------
 *  disp_area_authority()
 *
 * --------------------------------------------------------------- */
disp_area_authority()
{
   DDECIMAL();
}


/* -------------------------------------------------------------
 *  acpt_area_authority()
 *    This is a STANDARD deciaml format EXCEPT:
 *      If user 0 is being programmed
 *      The only except a blank entry and then do nothing with it.
 * --------------------------------------------------------------- */
acpt_area_authority(charin)
char charin;
{
   if  ( UserIndx != 0)               
      return(ADECIMAL(charin));       

   if (charin == ENTER_KEY)
      return(AUTOINCREMENT());         
   else
      return(NORMNAVIG(charin));       
}





/* -------------------------------------------------------------
 *  Disp_auth_level:  Displayes 'X' or blank depends on the state
 *    of authority bit in a two byte memory location.
 *    Authority bit is found by shifting the preset mask (0x4000)
 *    to the right AuthIndex times and ANDING it with two bytes.
 * --------------------------------------------------------------- */
disp_auth_level()
{
   char *IndxPtr; 
   char *Address;
   int mask;

   mask = MASK;                       
   PRMPT_INDEX(&Address,&IndxPtr);

   *DataHold = ((( mask >> AuthIndex ) & RevBytes(*(int *)Address) ) == 0) ? ' ' : 'E';

   bdos(6, *DataHold);
   BackCursor (1);
}






/* -------------------------------------------------------------
 *  Acpt_auth_level:  Accepts either blank or 'E' for input and
 *     translate them to 0 or  1.
 *     There is an initial preset mask and data (0x4000) where we start
 *     shifting both of them to the right AuthIndex times.
 *
 *                        ÚÄÄÄÄ authority level 1
 *                        ³  ÚÄÄÄÄÄ authority level 2
 *                        ³  ³
 *                    ÚÄÄÂÁÄÂÁÄÂÄÄÂÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÂÄÄÂÄÄÂÄÄ¿
 *     Command Center ³..³1 ³2 ³3 ³4  5  6  7 ³8  9  10 11 ³12³13³14³..³
 *        Function    ³..³  ³  ³  ³           ³            ³  ³  ³  ³..³
 *                    ÀÄÄÁÂÄÁÂÄÁÂÄÁÄÂÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÁÄÄÁÄÄÁÄÄÙ
 *                        ³  ³  ³   ³
 *                        ³  ³  ³   ÀÄÄÄ ....
 *                        ³  ³  ÀÄÄÄÄ0x1000
 *                        ³  ÀÄÄÄÄ0x2000
 *                        ÀÄÄÄ0x4000
 *
 *    if we chose authority level 2, then the initial mask would be
 *      shifted once.
 *    since the actual index is 0 and not 1, data will also be shifted
 *      once to match the mask.
 *    There are 63 of this two bytes storage each of which is assigned
 *     to one command center function.
 *     Enable any command center function for this authority level by
 *     selecting and inputing E.
 *     Disable it by inputting blank.
 *
 * --------------------------------------------------------------- */
acpt_auth_level(charin)
char charin;
{
   int data, Mask_Data;

   data = MASK;                               
   if (charin == ENTER_KEY)
   {
      SetUpAccept();

      Mask_Data = RevBytes(data >> AuthIndex);
      data = (*DataHold == 'E') ? Mask_Data : 0;
      *(int *)menu_addr = data | ((*(int *)menu_addr) & (Mask_Data ^ 0xFFFF));
      return (AUTOINCREMENT());
   }

   if ( iscntrl(charin) )                     
      return (DoNavEditKeys(charin));

   switch (charin = toupper(charin))
   {
      case ' ': 
	 charin =  (*DataHold == ' ' ? 'E' : ' ');
	     break;

      case 'E':
	     break;                                 

      default:
	     return (BADCHAR);                      
   }

   bdos (6, *DataHold = charin);
   BackCursor (1);                            
   *NoNavFlag = TRUE;                         
   return(GOODCHAR);
}







/****************************************************************************
 *																			*
 *	acpt_dc_auth_lvl()														*
 *	It is similar to acpt_auth_level() execpt that it goes to the door		*
 *	control auth level menu if the input is an E.							*
 *																			*
 ****************************************************************************/

acpt_dc_auth_lvl(charin)
char charin;
{
	int data, Mask_Data;
	MENUTYPE *temp_addr;

	data = MASK;                               
	if (charin == ENTER_KEY)
	{
        SetUpAccept();

		Mask_Data = RevBytes(data >> AuthIndex);
		data = (*DataHold == 'E') ? Mask_Data : 0;
		*(int *)menu_addr = data | ((*(int *)menu_addr) & (Mask_Data ^ 0xFFFF));
		
		if(*DataHold == 'E')
		{
			temp_addr = &GotoDoorCtrl;
			*MPtr = GetMenu(temp_addr);
			return(AENTERTO(ENTER_GRP_KEY));
		}
      
		return (AUTOINCREMENT());
	}

	if ( iscntrl(charin) )                     
		return (DoNavEditKeys(charin));

	switch (charin = toupper(charin))
	{
		case ' ': 
			charin =  (*DataHold == ' ' ? 'E' : ' ');
			break;

		case 'E':
			break;                                 

		default:
			return (BADCHAR);                      
	}

	bdos (6, *DataHold = charin);
	BackCursor (1);                            
	*NoNavFlag = TRUE;                         
	return(GOODCHAR);
}





/****************************************************************************
 *																			*
 *	disp_acs_alw_lvl()														*
 *	It is similar to disp_auth_level() except that it displays 'D', 'P',	*
 *	'M' and ' '.															*
 *																			*
 ****************************************************************************/

disp_acs_alw_lvl()
{
    char *IndxPtr;
	char *Address;
	int  Mask_Data;

	PRMPT_INDEX(&Address,&IndxPtr);

    Mask_Data = RevBytes(MASK >> AuthIndex);            /* MASK is in Main.h */

        if ((Mask_Data & (*(int *)(Address))) != 0)     /* Check MasterArm Lvl */
            *DataHold = 'M';
        else 
        if ((Mask_Data & (*(int *)(Address+4))) != 0)   /* Check PerimArm Lvl */
            *DataHold = 'P';
        else 
        if ((Mask_Data & (*(int *)(Address+8))) != 0)   /* Check DisArm Lvl */
            *DataHold = 'D';
        else 
            *DataHold = ' ';

	bdos(6, *DataHold);
	BackCursor (1);
}





/****************************************************************************
 *																			*
 *	acpt_acs_alw_lvl()														*
 *	It is similar to acpt_auth_level() except that it accepts a 'D', 'P',	*
 *	'M' and ' '.															*
 *																			*
 ****************************************************************************/

acpt_acs_alw_lvl(charin)
char charin;
{
	int Mask_Data;

	if (charin == ENTER_KEY)
	{
        SetUpAccept();

		Mask_Data = RevBytes(MASK >> AuthIndex);    /* MASK is in Main.h */

        /* Set = ENABLE, Clear = DISABLE for Authority levels */

		if (*DataHold == 'M')
		{
   			*(int *)menu_addr     |= Mask_Data;     /* Set Granted MstrArm Level */        
			*(int *)(menu_addr+4) |= Mask_Data;     /* Set Granted PerimArm Level */ 
			*(int *)(menu_addr+8) |= Mask_Data;     /* Set Granted Disarmed Level */ 
		}
		else if (*DataHold == 'P')
		{
            *(int *)menu_addr     &= ~Mask_Data;    /* Clear Granted MstrArm Level */
            *(int *)(menu_addr+4) |= Mask_Data;     /* Set Granted MstrArm Level */
			*(int *)(menu_addr+8) |= Mask_Data;
		}
		else if (*DataHold == 'D')
		{
   			*(int *)menu_addr     &= ~Mask_Data;            
			*(int *)(menu_addr+4) &= ~Mask_Data;
            *(int *)(menu_addr+8) |= Mask_Data;     /* Set Granted Disarmed Level */ 
		}
		else if (*DataHold == ' ')
		{
   			*(int *)menu_addr     &= ~Mask_Data;    /* Clear Granted MstrArm Level */          
			*(int *)(menu_addr+4) &= ~Mask_Data;    /* Clear Granted MstrArm Level */ 
			*(int *)(menu_addr+8) &= ~Mask_Data;    /* Clear Granted MstrArm Level */ 
		}
		return (AUTOINCREMENT());
	}

	if ( iscntrl(charin) )                     
		return (DoNavEditKeys(charin));

	switch (charin = toupper(charin))
	{
		case ' ': 
			if (*DataHold == ' ')           /* space bar pressed? */
				charin = 'M';               /* scroll through choices */
			else if (*DataHold == 'M')
				charin = 'P';
			else if (*DataHold == 'P')
				charin = 'D';
			else if (*DataHold == 'D')
				charin = ' ';                                    				
			break;

		case 'M':                           /* Direct key choice entries */
			break;                                                        
		case 'P':
			break;
		case 'D':
			break;

		default:
			return (BADCHAR);                      
	}

	bdos(6, *DataHold = charin);
	BackCursor(1);                            
	*NoNavFlag = TRUE;                         
	return(GOODCHAR);
}




/****************************************************************************
 *																			*
 *	disp_tkn_disa_lvl()														*
 *	It is similar to disp_auth_level() except that it displays 'D', 'P',	*
 *	and ' '.																*
 *																			*
 ****************************************************************************/

disp_tkn_disa_lvl()
{
    char *IndxPtr;
	char *Address;
	int  Mask_Data;

	PRMPT_INDEX(&Address,&IndxPtr);                 

    Mask_Data = RevBytes(MASK >> AuthIndex);            /* MASK is in Main.h */

        if ((Mask_Data & (*(int *)(Address+4))) != 0)   /* Check for DisArm interior */
            *DataHold = 'I';
        else 
        if ((Mask_Data & (*(int *)(Address))) != 0)     /* Check for DisArm Area */
            *DataHold = 'D';
        else 
            *DataHold = ' ';

	bdos(6, *DataHold);
	BackCursor (1);
}






/****************************************************************************
 *																			*
 *	acpt_tkn_disa_lvl()														*
 *	It is similar to acpt_auth_level() except that it accepts a 'D', 'I',	*
 *	and ' '.																*
 *																			*
 ****************************************************************************/

acpt_tkn_disa_lvl(charin)
char charin;
{
	int Mask_Data;

	if (charin == ENTER_KEY)
	{
        SetUpAccept();

		Mask_Data = RevBytes(MASK >> AuthIndex);    /* MASK is in Main.h */

        /* Set = ENABLE, Clear = DISABLE for Authority levels */

		if (*DataHold == 'I')
		{
            *(int *)(menu_addr+4) |= Mask_Data;     /* Set DisArm Interior */
            *(int *)menu_addr     &= ~Mask_Data;    /* Clear DisArm Area */
		}
		else if (*DataHold == 'D')
		{
            *(int *)(menu_addr+4) &= ~Mask_Data;    /* Clear DisArm Interior */
            *(int *)menu_addr     |= Mask_Data;     /* Set DisArm Area */
		}
		else if (*DataHold == ' ')
		{
            *(int *)(menu_addr+4) &= ~Mask_Data;    /* Clear DisArm Interior */
            *(int *)menu_addr     &= ~Mask_Data;    /* Clear DisArm Area */
		}
		
		return (AUTOINCREMENT());
	}

	if ( iscntrl(charin) )                     
		return (DoNavEditKeys(charin));

	switch (charin = toupper(charin))
	{
		case ' ':                           /* space bar pressed? */
			if (*DataHold == ' ')           /* scroll through choices */
				charin = 'I';           
			else 
            if (*DataHold == 'I')
				charin = 'D';
			else 
            if (*DataHold == 'D')
				charin = ' ';
			break;

		case 'I':                           /* Direct key choice entries */
			break;
		case 'D':
			break;

		default:
			return (BADCHAR);                      
	}

	bdos (6, *DataHold = charin);
	BackCursor (1);                            
	*NoNavFlag = TRUE;                         
	return(GOODCHAR);
}






/****************************************************************************
 *																			*
 *	disp_cust_func_lvl()													*
 *	It is similar to disp_auth_level() except that it accepts a 'D', 'C',	*
 *	'M' and ' '.															*
 *																			*
 ****************************************************************************/

disp_cust_func_lvl()
{
    char *IndxPtr;
	char *Address;

	int Mask_Data;

	PRMPT_INDEX(&Address,&IndxPtr);

    Mask_Data = RevBytes(MASK >> AuthIndex);                /* MASK is in Main.h */

        if (((Mask_Data & (*(int *)(Address))) != 0) &&     /* Armed Set? and */
            ((Mask_Data & (*(int *)(Address+4))) != 0))     /* Area Disarmed Set? */
                *DataHold = 'C';
        else 
        if (((Mask_Data & (*(int *)(Address))) != 0) &&     /* Armed Set? and */
            ((Mask_Data & (*(int *)(Address+4))) == 0))     /* Area Disarmed Clear? */
                *DataHold = 'M';
        else 
        if (((Mask_Data & (*(int *)(Address))) == 0) &&     /* Armed Clear? and */
            ((Mask_Data & (*(int *)(Address+4))) != 0))     /* Area Disarmed Set? */
                *DataHold = 'D';
        else 
            *DataHold = ' ';                                /* Both are cleared */
        
	bdos(6, *DataHold);
	BackCursor (1);
}






/****************************************************************************
 *																			*
 *	acpt_cust_func_lvl()														*
 *	It is similar to acpt_auth_level() except that it accepts a 'D', 'C',	*
 *	'M' and ' '.															*
 *																			*
 ****************************************************************************/

acpt_cust_func_lvl(charin)
char charin;
{
	int Mask_Data;

	if (charin == ENTER_KEY)
	{
        SetUpAccept();

		Mask_Data = RevBytes(MASK >> AuthIndex);

        /* Set = ENABLE, Clear = DISABLE for Authority levels */

		if (*DataHold == 'C')
		{
            *(int *)menu_addr     |= Mask_Data;     /* Set AreaArmed */
            *(int *)(menu_addr+4) |= Mask_Data;     /* Set Area DisArmed  */
		}
		else if (*DataHold == 'M')
		{
            *(int *)menu_addr     |= Mask_Data;     /* Set AreaArmed */
            *(int *)(menu_addr+4) &= ~Mask_Data;    /* Clear Area DisArmed  */
		}
		else if (*DataHold == 'D')
		{
            *(int *)menu_addr     &= ~Mask_Data;    /* Clear AreaArmed */
            *(int *)(menu_addr+4) |= Mask_Data;     /* Set Area DisArmed  */
		}
		else if (*DataHold == ' ')
		{
            *(int *)menu_addr     &= ~Mask_Data;    /* Clear AreaArmed */
            *(int *)(menu_addr+4) &= ~Mask_Data;    /* Clear Area DisArmed  */
		}
		
		return (AUTOINCREMENT());
	}

	if ( iscntrl(charin) )                     
		return (DoNavEditKeys(charin));

	switch (charin = toupper(charin))
	{
		case ' ':                           /* space bar pressed ? */
			if (*DataHold == ' ')           /* scroll through choices */
				charin = 'M';
			else if (*DataHold == 'M')
				charin = 'D';
			else if (*DataHold == 'D')
				charin = 'C';
			else if (*DataHold == 'C')
				charin = ' ';
			break;

		case 'M':                           /* direct choice key entries */
			break;                                
		case 'C':
			break;
		case 'D':
			break;

		default:
			return (BADCHAR);                      
	}

	bdos (6, *DataHold = charin);
	BackCursor (1);                            
	*NoNavFlag = TRUE;                         
	return(GOODCHAR);
}






/* -------------------------------------------------------------
 *  disp_relays: This function displays decimal digits and converts
 *    253, 254, and 255 to A, B, and C.
 *    This is done to differentiate this 3 relays from others.
 *    Added by marketing request at 7/23/91.
 *
 * --------------------------------------------------------------- */
disp_relays ()
{
   int temp;

   DDECIMAL();                                /* normal decimal display */

   atoi (DataHold,&temp);                     /* convert to A, B, or C */
   if (temp >= 253)
   {
      memcpy (DataHold, blank_buff, 4);        /* blank display buffer */
      *DataHold  =   temp == 253  ? 'A'        /* 253 is displayed as A */
			 : ((temp == 254) ? 'B'        /* 254 is displayed as B */
		     :                  'C');      /* 255 is displayed as C */
      printf(DataHold);
      BackCursor(3);
   }
}




/* -------------------------------------------------------------
 * acpt_relays: This function handles relays number by calling
 *    decimal format.
 *    User enteres A, or B, or C and this function converts them
 *    to 253,254, and 255 are handled.
 *    above number.
 * --------------------------------------------------------------- */
acpt_relays(charin)
char charin;
{
	int value;
	
	if (charin == ENTER_KEY)
	{ 
        SetUpAccept();

		value = *DataHold;                    /* input char */
		if ( char_in_set (value, "ABC") )
		{
			value =   value == 'A'  ? 253      /* A is store as 253 */
				  : ((value == 'B') ? 254          /* B is store as 254 */
				  :                   255);        /* C is store as 255 */
		}
		else
		{
			atoi(DataHold,&value);
			if ( value > 128)                  /* upper limit = 128 */
				return (BADENTER);
		}

		MSK_DATA( value, menu_msk, menu_addr);
		return(AUTOINCREMENT());
	}


	charin = toupper(charin);                /* force to upper case */
	if ( char_in_set (charin, "ABC") )
		return(AASCII(charin));
	else
		return(ADECIMAL(charin));
}





/* -------------------------------------------------------------
 *  disp_close_time
 *    Displays the prompt and close time
 * --------------------------------------------------------------- */
void disp_close_time ()
{
   char disp_hr, disp_min;
   char temp_time;

   SetUpDisplay();

   temp_time = field(menu_msk, *menu_addr);
   if (temp_time == 0xFF)
   {
      disp_hr = disp_min = 0xFF;
   }
   else
   {
      disp_min = 0;
      disp_hr = temp_time/2;
      if (temp_time % 2)
	 disp_min = 30;
   }

   disp_date_or_time (disp_hr, disp_min, ':');
}



/* -------------------------------------------------------------
 *  acpt_close_time
 *    Stored as multiples of 1/2 hour; 2:00 am is stored as a 4
 *
 * --------------------------------------------------------------- */
int acpt_close_time (charin)
char charin;
{
   char ret_value;
   char new_hr, new_min;

   if (charin != ENTER_KEY)
      return(acpt_time_date(charin));
   else
   {
      SetUpAccept();

      if ( (ret_value = acpt_date_or_time (&new_hr, &new_min, ':')) == BADENTER)
	 return (BADENTER);                       /* yes; error exit */

      if (new_hr != 0xFF)
	  {
	     if (new_min == 0 || new_min == 30)
	     {
		new_hr = new_hr * 2;
		if (new_min != 0)
		   new_hr++;
	     }
	     else
		return (BADENTER);
	  }
      else
	     new_hr = 0;                        /* rev 50 of panel won't except ff */

      MSK_DATA(new_hr, menu_msk, menu_addr);  /* opening windows in sked array */
      return (AUTOINCREMENT());
   }
}










/* -------------------------------------------------------------
 *  disp_account_text
 *
 * --------------------------------------------------------------- */
disp_account_text ()
{
   disp_compressed_text();
}


/* -------------------------------------------------------------
 *  acpt_account_text
 *    Sets use account text boolean when account in on
 *    has been programmed (text assigned to it). and
 *    resets use account text boolean otherwise.
 * --------------------------------------------------------------- */
acpt_account_text (charin)
char charin;
{
   if (charin == ENTER_KEY)
   {
      SET(&areas[AreaIndx][1],1);
      if (!strcmp(DataHold,"                "))
         RES(&areas[AreaIndx][1],1);
   }

   return(acpt_compressed_text(charin));
}








/********************************************************
*														*
*		disp_compressed_text()							*
*														*
********************************************************/

disp_compressed_text()
{ 
    char decomp[16];
	char CompText[12];
    char temp;
    char mask;
    int ii, jj, kk;

    SetUpDisplay();

	memcpy(CompText, menu_addr, 12);

    mask = 0x40;
   
    for(ii = 0; ii < 12; ii++)
    {
		jj = ii % 3;
		kk = ii*4/3;

		switch(jj)
		{
	    	case 0:
	    
				temp = CompText[ii];
				decomp[kk] = temp >> 2;
	    
				if(decomp[kk] >= 0x00 && decomp[kk] <= 0x1A)
			    	decomp[kk] = decomp[kk] | mask;

				temp = CompText[ii];
				decomp[kk+1] = (temp << 4) & 0x3F;
	    
				break;

	    	case 1:

				temp = CompText[ii];
				decomp[kk] = decomp[kk] | (temp >> 4);

				if(decomp[kk] >= 0x00 && decomp[kk] <= 0x1A)
		    		decomp[kk] = decomp[kk] | mask; 

				temp = CompText[ii];
				decomp[kk + 1] = (temp << 2) & 0x3F;
	    
				break;

	    	case 2:

				temp = CompText[ii];
				decomp[kk] = decomp[kk] | (temp >> 6);

				if(decomp[kk] >= 0x00 && decomp[kk] <= 0x1A)
				    decomp[kk] = decomp[kk] | mask;

				temp = CompText[ii];
				decomp[kk + 1] = temp & 0x3F;

				if(decomp[kk + 1] >= 0x00 && decomp[kk + 1] <= 0x1A)
				    decomp[kk+1] = decomp[kk + 1] | mask;

				break;

		}
    }

    memcpy (DataHold, decomp, menu_len);

    UpdateDisplay();
}




/********************************************************
*														*
*		acpt_compressed_text()							*
*														*
********************************************************/

acpt_compressed_text(charin)
char charin;
{
    char comp[12];
    char orig[16];
    char temp;
    int  ii, jj, kk;

    if (charin == ENTER_KEY)
    {
        SetUpAccept();

		for(ii = 0; ii < 16; ii++)
		{
	    	jj = ii % 4;
	    	temp = DataHold[ii] & 0x3F;	/* clear the first two bits */
	
	    	switch(jj)
	    	{
				case 0:   /* First Character */
			   
		    		kk = ((ii + 1) * 3) / 4;
		    		comp[kk] = temp << 2;

			    	break;

				case 1:   /* Second Character */

					kk = (ii * 3) / 4;
					comp[kk] = comp[kk] | (temp >> 4);

					temp = DataHold[ii] & 0x3F;
					comp[kk + 1] = temp << 4;

					break;

				case 2:   /* Third Character */

			    	kk = (ii * 3) / 4;
					comp[kk] = comp[kk] | (temp >> 2);
				
					temp = DataHold[ii] & 0x3F;
					comp[kk + 1] = temp << 6;	
					
					break;

				case 3:   /* Forth Character */

					kk = (ii * 3) / 4;
					comp[kk] = comp[kk] | temp;

					break;
	    	}
		} 
	    	
		memcpy(menu_addr, comp, 12);

		return(AUTOINCREMENT());
    }

    if ( iscntrl(charin) ) 
        return (DoNavEditKeys(charin));

    charin = toupper(charin);
    if (isascii(charin) && !char_in_set(charin, "!%()=<>.,:") ) 
        return (NORMENTRY (charin) );

    return(BADCHAR);

}








/* -------------------------------------------------------------
 *  disp_time_date
 *    See acpt-time-date for description
 *
 * --------------------------------------------------------------- */
void disp_time_date ()
{
   char ii,hour,month;
   int TimeInMinutes;

   SetUpDisplay();

   TimeInMinutes = 0;

   if (menu_bits & DATEMARK) 
   {
      month = (*menu_addr & 0x0F);
      if ( month == 0x00 ) 
         month = 0xFF;

      disp_date_or_time (month, *(menu_addr) & 0x1F, '/'); 
   }
   else
   {
      for( ii = 0; ii < 3; ii++)               
      {
	     hex_nibble_2_ascii ( field(menu_msk, *menu_addr), (DataHold+ii));
	     bumpAM(&menu_addr, &menu_msk);
	  }
      *(DataHold+3) = 0;
      TimeInMinutes = ((atoh(*DataHold)) * 256)+(atoh(*(DataHold+1)) * 16)+(atoh(*(DataHold+2)));
      hour = TimeInMinutes/60;

      disp_date_or_time (hour, TimeInMinutes%60 , ':');
   }
}




/* -------------------------------------------------------------
 *  acpt_time_date
 *    Accept Date or Time data and convert it to integers to be used
 *    for storing the data.
 *    This routine accepts only SKED date data
 *
 *    This routine accepts 2 types of time data;
 *      WINDOW TIME DATA
 *      SKED TIME DATA
 *
 *    WINDOW TIME DATA
 *      Time Format in Windows - 3 bytes per window
 *         byte 0 xxxx xxxx                        (x = Start time)
 *         byte 1           xxxx yyyy              (y = Stop  time)
 *         byte 2                      yyyy yyyy
 *
 *      The values for the following prompts are stored as WINDOWS:
 *          Open Start Window    _ _ : _ _   (areas PARAM - O/C Windows)
 *          Close Start Window   _ _ : _ _   (areas PARAM - O/C Windows)
 *          Access Window Start  _ _ : _ _   (User Access Windows)
 *
 *          Open Stop Window     _ _ : _ _   (areas PARAM - O/C Windows)
 *          Close Stop Window    _ _ : _ _   (areas PARAM - O/C Windows)
 *          Access Window Stop   _ _ : _ _   (User Access Windows)
 *
 *    SKED TIME DATA
 *      Time Format in skeds - 8 bytes per Sked
 *          byte 0 - 2  Misc sked data
 *          byte 3      uuuu xxxx                  (u = unused)
 *          byte 4      xxxx xxxx                  (x = time)
 *          byte 5 - 7  Misc sked data
 *
 *      The values for the following prompts are stored as skeds:
 *          Open Early Begin     _ _ : _ _   (areas PARAM - O/C Windows)
 *          Close Early Begin    _ _ : _ _   (areas PARAM - O/C Windows)
 *          Time                 _ _ : _ _   (skeds)
 *
 *
 *
 *    inputs :  charin - contains the character to be processed
 *    outputs:
 *
 * --------------------------------------------------------------- */
int acpt_time_date (charin)
char charin;
{
   char ret_value, symbol;
   char new_hr, new_min, new_month, new_day;
   int  TimeInMinutes, mask;
   char *IndxPtr; 

   setup_cur_menu_data ();

   if ( iscntrl(charin) ) 
   {
      switch (charin)
	  {
	     case ENTER_KEY:
	        FINDADDRESS(&menu_addr,&IndxPtr); 

	        if (menu_bits & DATEMARK)
	        {
	           if ((ret_value = acpt_date_or_time (&new_month, &new_day, '/')) == BADENTER)
		          return (BADENTER); 

	           if ( new_month == 0xFF && new_day == 0xFF )
		          new_month = new_day = 0;
	     
	           MSK_DATA(new_month, 0x0F, menu_addr); 
	           MSK_DATA(new_day,   0x1F, menu_addr+1);
	        }
	        else
	        {
	           if ((ret_value = acpt_date_or_time (&new_hr, &new_min, ':')) == BADENTER)
		          return (BADENTER); 

	           TimeInMinutes = ((new_hr * 60) + new_min); 

	           *(int *)menu_addr = (RevBytes(TimeInMinutes) |(*(int *)menu_addr & (mask ^ 0xFFFF)));
	        }
	        return (AUTOINCREMENT());


	     case RT_ARROW_KEY: 
	        if (*cursor == 1)
	        {
		       RightCursor(1);
		       ++*cursor;
	        }                                  

	     default:
	        if ( charin == LT_ARROW_KEY) 
	        {
		       if (*cursor == 3 )
		       {
		          BackCursor(1);
		          --*cursor;
		       }
	        }
	        return (DoNavEditKeys(charin));

	  } 
   }   
   else
   {
      if ( char_in_set (charin, " :/0123456789") )
	  {
	     putcpm(charin);
	     *(DataHold + *cursor) = charin; 
	     if (*cursor < (menu_len - 1)) 
	        ++*cursor;
	     else
	        BackCursor(1); 

	     if ( *cursor == 2 ) 
	     {
	        symbol = (menu_bits & DATEMARK) ? '/' : ':';
	        putcpm(symbol);
	        *(DataHold + *cursor) = symbol;
	        ++*cursor;
	     }

	     *NoNavFlag = TRUE; 
	     return(GOODCHAR);
	  }
      else
	     return(BADCHAR);
   }
}










/* -------------------------------------------------------------
 *   disp_func_number()
 * ------------------------------------------------------------- */
void disp_func_number()
{
   DDECIMAL();
}



/* -------------------------------------------------------------
 *   acpt_func_number()
 *     If data is blank
 *       resets other data.
 *     else
 *       set other data.
 * ------------------------------------------------------------- */
int acpt_func_number (charin)
char charin;
{
   int temp;

   if (charin == ENTER_KEY)
   {
      SetUpAccept();

      atoi(DataHold,&temp);

      if ( (temp == 23)||(temp == 24)        /* ALLOW 1-37 (except 23,24)... */
        || (temp == 31)                      /* ALLOW 1-37 (except 31)... */
        || (temp >= 38 && temp <= 127)       /* ... and 128-143           */    
        || (temp >= 144) )                    
         return (BADENTER);

      MSK_DATA(temp, menu_msk, menu_addr);     
      return (AUTOINCREMENT());
   }

   return(ADECIMAL(charin));
}





/****************************************************************************
 *																			*
 *	disp_cust_route_yn()													*
 *																			*
 ****************************************************************************/
 
disp_cust_route_yn()
{
	DYESNO();
}



/****************************************************************************
 *																			*
 *	acpt_cust_route_yn()													*
 *	If the input value is yes, then it will jumps to the custom routing		*
 *	menu.  Otherwise, all routing messages will be set to yes.				*
 *																			*
 ****************************************************************************/

int acpt_cust_route_yn(charin)
char charin;
{
	int stat;
	MENUTYPE *temp_addr;
	int ii;

	if ( (stat = AYESNO(charin)) == GOODENTER)  /* valid entry ? */
	{
		/* check if custom bit is set */
		if (routing[RouteIndx][25] & 0x08)		
		{
			/* goto custom menu */
			temp_addr = &GoCustRoute;
			*MPtr = GetMenu(temp_addr);
			return(AENTERTO(ENTER_GRP_KEY));
		}	
		/* if custom bit not set */
		else 	
		{
			/* set all event to default values (YES) */
			for(ii = 5; ii < 22; ii++)
            {
				MSK_DATA(255, 0xff, &routing[RouteIndx][ii]);

                if(ii == 7)
                    MSK_DATA(0, 0x01, &routing[RouteIndx][ii]);     /* Default Extra Poit to NO */
            }
		}

	}

	return(stat);

}





/****************************************************************************
 *																			*
 *	disp_prim_indx()														*
 *	If the value of primary device is 0, a blank will be displayed.			*
 *	Otherwise, the value of the primary index will be displayed.			*
 *																			*
 ****************************************************************************/

disp_prim_indx()
{
	char PrimDev;
	auto int PrimIndx;

    SetUpDisplay();

	/* get primary device type */
	PrimDev = field(0xE0, *menu_addr);
	
	if (PrimDev == 0)
	{
		/* set display to be blank */
		*DataHold = ' ';
	}	
	else if (PrimDev != 0)
	{
		PrimIndx = field(menu_msk, *menu_addr);
		PrimIndx++;
    
    	itoa(PrimIndx, DataHold);
	}

    UpdateDisplay();
}





/****************************************************************************
 *	acpt_prim_indx()														
 *	If the Primary Device is a blank,
 *  then set primary dev type = 0; and Alternate Dest = NO; Ning to
 *    alert the intaller that data was changed. 	
 *	If the Primary Device is from 1 to 4, 
 *  then set primary dev type = 0; Alternate Dest = YES and decrement value 
 *  for storage.
 *																
 ****************************************************************************/
int acpt_prim_indx(charin)
char charin;
{
	char devNumber;
	
    if (charin == ENTER_KEY)
    {
        SetUpAccept();
		
        atoi(DataHold, &devNumber);
      
        if (*DataHold == ' ')                       /* Primary Device= blank?     */
		{
			MSK_DATA(0, 0xE0, menu_addr);           /* set primary dev type = 0   */

            devNumber = 0;                          /* clear primary device       */
        
            if(field(0x04, *menu_addr) != 0)        /* is there an alternate device? */
            {
                RES(menu_addr, 2);                  /* Backup Device = NO        */
                NING();                             /* annuciate change being made */
            }                                       
		}
		else 					                    /* 1 < Primary Device < 4     */
		{
			MSK_DATA(1, 0xE0, menu_addr);           /* set primary dev type = 1   */
			devNumber--;                            /* 1-4 stored as 0-3          */
		}

		MSK_DATA(devNumber, menu_msk, menu_addr);
		return(AUTOINCREMENT());
    }       
	return(ADECIMAL(charin));
}




		
/****************************************************************************
 *																			*
 *	disp_back_indx()														*
 *	It displays the backup routing index.  If the value of the primary		*
 *	device is 0, it shows a blank.  If the backup route bit is				*
 *	reset, it displays a blank.  Otherwise, it shows the value+1 for 		*
 *	offset purpose.															*
 *																			*
 ****************************************************************************/

disp_back_indx()
{
	char BackUpDev;
	auto int BackIndx;

    SetUpDisplay();

	/* get primary device type */
	BackUpDev = field(0xE0, *menu_addr);
	
	if (BackUpDev == 0)
	{
		/* set display to be blank */
		*DataHold = ' ';
	}	
	else if (BackUpDev != 0)
	{
		/* alt? bit is not set */
		if (!field(0x04, *menu_addr))
			*DataHold = ' ';
		else
		{
			BackIndx = field(menu_msk, *menu_addr);
			BackIndx++;

   		 	itoa(BackIndx, DataHold);
		}
	}

    UpdateDisplay();
}





/*************************************************************************
 *														
 *	acpt_back_indx()
 * If the primary device is blank don't accept 1-4 for the backup device											
 *	If the backup Device is a blank,
 *  then set Alternate Dest = NO	
 *	If the backup Device is from 1 to 4, 
 *  then set Alternate Dest = YES and decrement value for storage.
 *************************************************************************/

int acpt_back_indx(charin)
   {
	char data;
	
	if (charin == ENTER_KEY)
	   {

        SetUpAccept();

		if ((field(0xE0, *menu_addr) == 0)     /* if prim dev type = 0    */
		 && (*DataHold != ' '))                /* and entry = 1,2,3or4 ?  */
			return(BADENTER);                   /* can't have backup */
		
		atoi(DataHold, &data);                 
		
		if (*DataHold == ' ')	               /* Backup Index = blank */
			RES(menu_addr, 2);                  /* Alternate Dest = NO */
		else 					                     /* 1 < Backup device < 4 */
		   {
			data--;                             /* 1-4 stored 0-3       */
			SET(menu_addr, 2);                  /* Alternate Dest = YES */
		   }

		MSK_DATA(data, menu_msk, menu_addr);
		return(AUTOINCREMENT());
	   }

	return(ADECIMAL(charin));
   }





/* -------------------------------------------------------------
 *   disp_blank_ff()
 * ------------------------------------------------------------- */
void disp_blank_ff()
{
	char value;
   
    SetUpDisplay();

	value = field(menu_msk, *menu_addr);
	
        if (value == 0x0F)                  /* blank value stored ? */
        {
            *DataHold = ' ';                /* set display to be blank */ 
        }
            else
        {
            value++;                        /* adjust ofset for display */
            itoa(value, DataHold);
        }

    UpdateDisplay();    
}



/* -------------------------------------------------------------
 *   acpt_blank_ff()
 *    Regular deimal except a blank is stord as a 0xFF (all bits 
 *    set depending upon the mask).
 * ------------------------------------------------------------- */
int acpt_blank_ff (charin)
char charin;
{

   if (charin == ENTER_KEY)
   {
      SetUpAccept();
        
      if (*DataHold == ' ')
      {
         MSK_DATA(0x0F, menu_msk, menu_addr);      /* store as an F */
         return (AUTOINCREMENT());
      }         
   }   
   return(ADECIMAL(charin));
}





  /*******************************************************************
   *	disp_bell_test_yn()													
   ******************************************************************* */
disp_bell_test_yn()
   {
	DYESNO();
   }



  /********************************************************************
   *	acpt_bell_test_yn()
   *     Bell test Yes/No
   *     if Yes enable bell test authority level and set passcode 
   *        required for the current area.												
   *     if No disable bell test authority level and clear passcode 
   *        required for the current area.												
   ******************************************************************** */
int acpt_bell_test_yn(charin)
char charin;
{
   char value;

    if (charin == ENTER_KEY)
    {
      SetUpAccept();

      value =  (*DataHold == 'Y') ? 1 : 0;         
      MSK_DATA( value, menu_msk, menu_addr);    /* bell test Y/n */
      
      menu_msk = (0x80 >> AreaIndx);            /* align area mask */
                         
      if (value) 
      {                  
         authority[24][0] |= menu_msk;          /* Area Enabled = Y */
         authority[24][1] |= menu_msk;          /* Area PassCode = Y */
      }
      else
      {                   
         authority[24][0] &= ~menu_msk;          /* Area Enabled = N */
         authority[24][1] &= ~menu_msk;          /* Area PassCode = N */
      }

      return(AUTOINCREMENT());
    }

    return(AYESNO(charin));
}



/********************************************************************
disp_ph_fmt()
Displays Yes or No for the "Modem Format" field.
********************************************************************/

void    disp_ph_fmt()
{
char value;
   
    SetUpDisplay();

	value = field(menu_msk, *menu_addr);        /* retrieve data */

    if (value == 2) {                           /* Modem IIe ? */
        memcpy (DataHold, "Yes", menu_len);    
    } else {
        memcpy (DataHold, "No ", menu_len);     /* BFSK */ 
    }        

    UpdateDisplay();
}


/********************************************************************
acpt_ph_fmt()
Accepts the entry for the "Modem Format" field. A 2 is stored for
ModemII communication and 1 for BFSK...
********************************************************************/
 
acpt_ph_fmt(char charin)
{
char value;

    if (charin == ENTER_KEY)
    {
      SetUpAccept();

      value =  (*DataHold == 'Y') ? 
        2 :     /* Modem IIe */
        1;      /* BFSK */  
          
      MSK_DATA(value, menu_msk, menu_addr);    /* Store data */

      return(AUTOINCREMENT());
    }
    return(AYESNO(charin));    
}

/********************************************************************
    Burg Restoral After an Alarm and
    Burg Restoral After a Trouble/Missing/S 
    prompts have been combined. These two custom functions
    handles the processing.
*********************************************************************/

void    disp_burg_rest_yn()
{
	DYESNO();
}

/*******************************************************************
    acpt_burg_rest_yn()
********************************************************************/

acpt_burg_rest_yn(char charin)
{
    if (charin == ENTER_KEY)
    {
      SetUpAccept();

      if (*DataHold == 'Y')
      {
        routing[RouteIndx][8] |= 0x20;     /* After Alarm */
        routing[RouteIndx][7] |= 0x20;     /* After T/M/S */
      } 
        else
      {
        routing[RouteIndx][8] &= ~0x20;     /* After Alarm */
        routing[RouteIndx][7] &= ~0x20;     /* After T/M/S */
      }

      return(AUTOINCREMENT());
    }

    return(AYESNO(charin));
}




/********************************************************************
disp_passcode()
Displays 'P' for - Passcode required and
         'E' for - NO Passcode required.
********************************************************************/

void disp_passcode()
{
     SetUpDisplay();
      
     *DataHold = (*(int *)(menu_addr-2) == 0xFFFF) ? 'P'
	       : (*(int *)(menu_addr-2) == 0x00FF) ? 'E'
	       : ' ';
              
     UpdateDisplay();
}

/********************************************************************
acpt_passcode()
Accepts  'P' for - Passcode required and enables passcode required 
                        for areas 1-8

         'E' for - NO Passcode required and disables passcode required 
                        for areas 1-8          
********************************************************************/

int acpt_passcode(char charin)
{
    MENUTYPE *temp_addr;

    if (*MPtr == GetMenu(&passc_1)&&(charin != ENTER_KEY))  /* Disarm programmed ? */
        return(NORMNAVIG(charin));                          /* tweedle */     

    if (charin == ENTER_KEY)
    {
        SetUpAccept();

           *(int *)(menu_addr-2) = 
                     (*DataHold == 'P') ? 0xFFFF
                   : (*DataHold == 'E') ? 0x00FF
                   : 0x0000;

            /*
            if(*DataHold == 'E')    
            {    
                if (*MPtr == GetMenu(&passc_14)) 
                {
                    temp_addr = &PCGoDrCtrl;
                    *MPtr = GetMenu(temp_addr);
                    return(AENTERTO(ENTER_GRP_KEY));
                }
            }
            */

        return (AUTOINCREMENT()); 
    }

   if ( iscntrl(charin) )                        
      return (DoNavEditKeys(charin));

   switch (charin = toupper(charin))
   {
       case ' ':
	   charin =  (*DataHold == 'P') ? 'E'   /* If  P  then switch to E  */
		  : ((*DataHold == 'E') ? ' '       /* If  E  then switch to ' '*/
		  :    'P');                        /* If ' ' then switch to P  */
	   break;

      case 'E': 	     
      case 'P':
	     break;                                 

      default:
	     return (BADCHAR);                      
   }

   bdos (6, *DataHold = charin);
   BackCursor (1);                            
   *NoNavFlag = TRUE;                         
   return(GOODCHAR);
}



/* -------------------------------------------------------------------------
 * -------------------------------------------------------------------------
 *
 *      MISCELLANEOUS SUB-ROUTINES
 *
 * ------------------------------------------------------------------------
 * ------------------------------------------------------------------------ */





/* -------------------------------------------------------------------
 *  Clear_Phone() _ called by acpt_routing
 *    Clear any routing assignments for this phone index
 * ------------------------------------------------------------------- */
void Clear_Phone(address,data)
char *address;
char  data;
{
   char mask;

   /* 0x01 and 0xFF are dummy masks */
   mask = (  field (0x70,*address)   == data ) ? 0x70   /* back-up ? */
	  : (( field(0x07,*address)    == data ) ? 0x07   /* primary ? */
	  : (( field(0x07,*(address+1))== data)  ? 0x01   /* duplicate ? */
	  :    0xff ) );                                  /* no match */

   if (mask == 0x01)                  /* matched duplicate ? */
   {
      address++;                       /* yes; set addr & mask */
      mask = 0x07;
   }

   if (mask != 0xff)                  /* match found ? */
      MSK_DATA(0, mask, address );     /* yes; clear original data */
}








/* -------------------------------------------------------------------
 *  Add_Zero ()
 *    Add a zero in front of the single digit months and days.
 * ------------------------------------------------------------------- */
Add_Zero(Buff)
char *Buff;
{
   char temp[9];

   temp[0] = '0';
   temp[1] = '\0';
   strcat(temp,Buff);
   strcpy(Buff,temp);
}





/**********************************************************
 * convert_to_ascii   ()
 *   Read hex data and convert it for display
 *   Used by phone numbers & key strokes
 ***********************************************************/
convert_to_ascii (dest_addr, source_addr, lng)
char *dest_addr;
char *source_addr;
char lng;
{
   auto char *tmpptr;
   auto char mask;
   auto char input;
   auto char temp_buf[6];                     /* to hold hex conversions */

   mask = 0xF0;
   for (tmpptr=dest_addr; tmpptr < dest_addr + lng; tmpptr++)
   {
      htoa(field(mask, *source_addr), temp_buf);   /* convert nibble to ascii */
      *tmpptr = input = temp_buf[3];

      if (phone_num_menus())
      {
	     *tmpptr =  (input == 'F') ? ' '        /* convert to display chars */
			: ((input == 'A') ? '#'
			: ((input == 'B') ? '*'
			: input) );
	  }
      else
	  if (input == 'F')                      /* convert F to a blank */
	     *tmpptr =  ' ';

      bumpAM(&source_addr,&mask);              /* manipulate the address and the mask */
   }
}



/**********************************************************
 * convert_to_hex   ()
 *   Read ascii data; convert it hex; and store in panel image
 *   Used by phone numbers & key strokes
 ***********************************************************/
convert_to_hex (dest_addr, source_addr, lng)
char *dest_addr;
char *source_addr;
char lng;
{
   auto char *temp_ptr;
   auto char mask;
   auto char value;
   auto char blank_encountered;

   blank_encountered = FALSE;
   mask = 0xF0;
   for (temp_ptr = source_addr; temp_ptr < source_addr+lng; temp_ptr++)
   {
      if (blank_encountered)           /* already found a blank ? */
	     value = 0xF;                  /* yes; blank everything that follows */
      else
	  {
	     value = *temp_ptr;            /* read value from source addr */
	     if (value == ' ')             /* is it a blank ? */
	     {
		value = 'F';               /* yes; convert to an "f" */
		blank_encountered = TRUE;
	     }
	     else
	     {
		    if (phone_num_menus())                 /* at a phone number menu */
		    {
		       value =   value == '#'  ? 'A'      /* convert other special chars */
				     : ((value == '*') ? 'B'
				     :   value);
		    }
	     }

	     value = atoh(value);
      }

      MSK_DATA(value, mask, dest_addr);
      bumpAM(&dest_addr,&mask);        /* next param addr & mask */
   }
}



/**********************************************************
 * phone_num_menus ()
 *   returns TRUE if the current menu is any of the 4 phone
 *   numbers or a RAM phone number
 ***********************************************************/
phone_num_menus ()
{
   /* current menu number */
   if ( (*MPtr >= GetMenu(&PhoneStrt) && *MPtr <= GetMenu(&phone4b) )
      || (*MPtr >= GetMenu(&phone5a)   && *MPtr <= GetMenu(&phone5b) ) )
      return (TRUE);
   else
      return (FALSE);
}







/***********************************************************/
/* setup_cur_menu_data ()                                  */
/*   Based upon the current menu; read the menu data into  */
/*   global prompts.                                       */
/*                                                         */
/***********************************************************/
setup_cur_menu_data ()
{
   menu_addr   = GetMenuDataAddr (MPtr);
   menu_msk    = GetMenuMask     (MPtr);
   menu_len    = GetMenuLength   (MPtr);
   menu_bits   = GetMenuBitFields(MPtr);
   menu_prompt = GetMenuPrompt   (MPtr);
}

/***********************************************************/
/* These were added for optimization                       */
/***********************************************************/

SetUpDisplay()
{
    char *IndxPtr;

    setup_cur_menu_data ();            /* setup data from cur menu table */
    PRMPT_INDEX(&menu_addr, &IndxPtr);
}

SetUpAccept()	
{
    char *IndxPtr;

    setup_cur_menu_data ();             /* setup data from cur menu table */
    FINDADDRESS(&menu_addr,&IndxPtr);   /* index to proper data addr */
}


UpdateDisplay()
{
    setup_cur_menu_data ();             /* setup data from cur menu table */

    *(DataHold+menu_len) = 0;           /* terminate display buffer */
    printf(DataHold);                   /* Print it */
    BackCursor(menu_len);               /* Bring Cursor to front */
}

/***********************************************************/
/***********************************************************/
compare_bytes(source1, source2, len)
char *source1;
char *source2;
int len;
{
	int ii;

	for(ii = 0; ii < len; ii++)
	{
		if (*(source1+ii) != *(source2+ii))
			return(0);
	}

	return(1);

}






/****************************************************************************
 *																			*
 *	scope_fbh()																*
 *	It set the device scope bits based on the input.  If the input is 		*
 *	custom, it jumps to the custom scope menu.								*
 *																			*
 ****************************************************************************/
scope_fbh(device, DevIndex, CustomMenu)
char     *device;
int      DevIndex;
MENUTYPE *CustomMenu;
{
    char scope, ar;
    int  currAcctNo;

    currAcctNo = (int)(areas[AreaIndx][0]);
	scope = field(0x70, *(device+(DevIndex * DEVICE_SIZE)));

    set_side_effects();

	switch(scope)
    {           
        /* See set_side_effects in smash.c */
        /* for automatic scope setting */

		case 4:		                        /* Custom ? */
			*MPtr = GetMenu(CustomMenu);
			return(AENTERTO(ENTER_GRP_KEY));
			break;

		default:                            /* all others do nothing */
			break;
	}  
}



