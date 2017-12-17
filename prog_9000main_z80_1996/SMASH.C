

/*****************************************************************************/
/*                                                                           */
/*  MAIN 9000C - Smash                                                       */
/*  11/06/1996                                                               */
/*                                                                           */
/*  RADIONICS  PROPRIETARY                                                   */
/*                                                                           */
/*  Paul Calinawan                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

/*

$Header:   G:/5200/9000main.vcs/smash.c_v   1.2   24 Nov 1996 22:24:48   PAULC  $
$Log:   G:/5200/9000main.vcs/smash.c_v  $
      
         Rev 1.2   24 Nov 1996 22:24:48   PAULC
      Authority level smashing revised for Radionics version. Some authority
      levels are no longer being smashed but are controlled individually.
      
         Rev 1.1   14 Nov 1996 13:14:40   PAULC
      Area Enabled and Command Center Functions (0xffff, first two bytes in
      authority levels) are no longer smashed but are controlled individually
      
         Rev 1.0   14 Nov 1996  9:15:26   PAULC
      Initial revision.
      
         
*/


#include <tomahawk.h>
#include <global.h>
#include <deflib.h>
#include <omacro.h>
#include <lib.h>

#include "overlay.h"
#include "MAIN.h"


#define     SHOW_ACCT_TEXT_MASK     0x02


extern  char   panels          [PANEL_SIZE];
extern  char   areas       [8] [AREA_SIZE];
extern  char   phones          [PHONE_SIZE];
extern  char   authority   [80][AUTHORITY_SIZE];
extern  char   routing     [4] [ROUTING_SIZE];
extern  char   keypads     [8] [DEVICE_SIZE];
extern  char   printers    [3] [DEVICE_SIZE];
extern  char   keypads     [8] [DEVICE_SIZE];


 
   /* Sets Authority level 15 (4th Byte to x01) */
char auth_smash_tbl[] = {	14,	/* view memory */
							13, /* view point status */
							34, /* extended closing */
							43, /* fire test */
							25, /* send report */
							67, /* open door */
							68, /* hold door */
							69, /* lock door */
							70, /* change levels */
							44, /* invisible test */
							28, /* change time date */
							29, /* change passcode */
							30, /* add user */
							31, /* delete user */
							41, /* view log */
							42, /* print log */
							17, /* bypass a point */
							18, /* unbypass a point */
							20, /* reset sensor */
							21, /* change relays */
							37, /* remote program */
							4,  /* move to area */
							39, /* display rev */
							45, /* service walk */
							38, /* default text */
							32  /* change skeds */
};


   /* Clears the area 1-8 enabled & area 1-8 passcode required  
    * (first 2 Bytes to 0x00)                                   */
char disable_tbl[] = {	3,	 /* undefined */
						      12, /* reserved */
						      16, /* reserved */
						      22, /* reserved */
						      33, /* reserved */
						      40, /* reserved */
						      79  /* reserved */
                        };



     /* -----------------------------------------------------------
      *   SmashRam() -
      *     Force certain values into certain parameters.
      *     When NEWRECORD is created all parameters are set to 0.
      * ------------------------------------------------------------ */
SmashRam()
{
	char ii, jj;
	char sub_item;

	for(ii = 0; ii < 80; ii++)
	{
        if(ii==19 || ii==26 || ii == 24)  
        {                                           /* Bell Test, */
            *(int *)(&authority[ii][2]) = 0xFE7F;   /* Command4 and Silence Bell */
        }
        else
		if (ii == 50) {
			*(int *)(&authority[ii][2]) = 0xFE7F;	/* smash auth level for Duress */
            *(int *)(&authority[ii][0]) = 0xFFFF;   /* Areas and PassCode enabled  */
        }

        /* exceptions: clear some smashes just set */

		for (jj = 0; jj < 7; jj++)
		{
		    if (ii == disable_tbl[jj])
    			*(int *)(&authority[ii][0]) = 0;	   /* smash disabled & no passcode required */
        }
	}

	for (ii = 0; ii < 26; ii++)
	{
		sub_item = auth_smash_tbl[ii];
		authority[sub_item][3] |= 0x01;
	}
		 
	MSK_DATA(60, 0x7f, &panels[2]);				/* Battery Fail Time */

	MSK_DATA( 0, 0xf0, &phones[4]);				/* BFSK AC Fail Code */
	MSK_DATA( 9, 0x0f, &phones[4]);				/* BFSK Battery Fail Code */
    MSK_DATA( 0, 0x0f, &phones[5]);				/* reserved */

	MSK_DATA(45, 0xff, &phones[6]);				/* ACK Wait Time */
    MSK_DATA( 0, 0xff, &phones[7]);				/* reserved */

	/* LAP Code */
	MSK_DATA(6, 0xf0, &panels[24]);
	MSK_DATA(5, 0x0f, &panels[24]);
	MSK_DATA(4, 0xf0, &panels[25]);
	MSK_DATA(3, 0x0f, &panels[25]);
	MSK_DATA(2, 0xf0, &panels[26]);
	MSK_DATA(1, 0x0f, &panels[26]);

	RES(&panels[3], 3);				/* Log Full Call RAM */
	RES(&panels[4], 7);				/* Short Walk Test Time */
    RES(&panels[5], 6);				/* Normal Poll = NO, for 5300 local comm */

    panels[0x0f] = 0xff;            /* Reserved, smashed 0xff */

	for(ii = 0; ii < 8; ii++) 
    {
        areas[ii][0x48] = 0x00;       /* reserved smashed 0 */
        areas[ii][0x49] = 0x00;

        keypads[ii][1] &= 0x0F;     /* Smash High Nibble to 0 */
    }                               /* Keypad subitem 0-7, byte 0x01 */

    
	for(ii = 0; ii < 4; ii++) {

        /* smash all areas to Yes */
        routing[ii][3] = 0x00;      /* delay all reports */
	    routing[ii][4] = 0xFF;      /* Area events */

        MSK_DATA( 0, 0x10, &routing[ii][0x0a]);				/* Walk Test Pt Event   */

        MSK_DATA( 1, 0x80, &routing[ii][0x19]);				/* Buzz on Comm Fail    */
        MSK_DATA( 1, 0x40, &routing[ii][0x19]);				/* Comm Fail Rte#       */
        MSK_DATA( 1, 0x20, &routing[ii][0x19]);				/* Send User Text       */
        MSK_DATA( 1, 0x10, &routing[ii][0x19]);				/* Send Point Text      */
        MSK_DATA( 0, 0x08, &routing[ii][0x19]);             /* Custom               */
        MSK_DATA( 0, 0x04, &routing[ii][0x19]);				/* reserved             */
        MSK_DATA( 0, 0x02, &routing[ii][0x19]);				/* reserved             */

        MSK_DATA( 0, 0x01, &routing[ii][0x19]);				/* reserved             */
    }
}    





  /***********************************************************************
   *  Name:	 set_side_effects	 
   *  Function:  Before a 'SEND' read values and set side-effects.
   *      mostly keypad/printer scope
   *  Inputs:	 None							  
   *  Outputs:  None							  
   *  Globals effected : alot						   
   ************************************************************************/
set_side_effects()
{
   char ii, jj, acctMatchFound;
   char scope;
   char area_assigned;
   char *device_addr;

    /* set side effects for keypad/printer scope */
    /* 0 to 7   = keypads */
    /* 8 to 10 = printers */

	for(ii = 0; ii < 11; ii++)
      {
      if (ii < 8)
         device_addr = &keypads[ii][0]; 
      else   
         device_addr = &printers[ii-8][0];
          
	   area_assigned = field(0x07, *(device_addr));
	   scope = field(0x70, *(device_addr));

	   switch(scope)
	      {
            /* scope = No Printer/keypad; clear area 1-8 in scope */
		   case 0:		
            *(device_addr+3) = 0;      /* clear area 1-8 in scope */
			   break;

            /* scope = Area Wide;                        */
            /* then set area X scope; where X = value in Area Assign' prompt  */
            /* thus clearing all other area scopes       */
		   case 1:			
            *(device_addr+3) = (0x80 >> area_assigned); 
			   break;
            
           /* Acount Wide; Set the area scope for areas that have the same */
           /*  account # as the value in Area Assign                       */
		   case 2:	
            *(device_addr+3) = 0;      /* clear area 1-8 in scope */
			   for(jj = 0; jj < 8; jj++)
			      {
				   if (compare_bytes(&areas[area_assigned][0], &areas[jj][0], 2))
                  *(device_addr+3) |= (0x80 >> jj); 
				   }
			   break;
			
            /* scope = Panel Wide; set area 1-8 in scope */
		   case 3:		
            *(device_addr+3) = 0xFF;

			   break;
            
		   default:
			   break;
	      }
      }


      /* ii = keypad device */

      for(ii=0; ii < 8; ii++) 
      {
        acctMatchFound = FALSE;

        for(jj=0; jj < 8; jj++) 
        {
            if(ii != jj) 
            {
                if(compare_bytes(&areas[ii][0], &areas[jj][0], 2)) /* Compare Acct# */
                { 
                    areas[ii][4] |= SHOW_ACCT_TEXT_MASK; /* Set Bit */
                    acctMatchFound = TRUE;
                    break; /* Found a match */
                }
            }
        }

        if(!acctMatchFound)
            areas[ii][4] &= ~SHOW_ACCT_TEXT_MASK; /* Clear Bit */
      }       
}



