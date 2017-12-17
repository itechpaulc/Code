

/****************************************************************/
/*                                                              */
/*  MAIN 5900C INDEXER file                                     */
/*  11/8/1996                                                   */
/*                                                              */
/*  RADIONICS  PROPRIETARY                                      */
/*                                                              */
/*  Paul Calinawan                                              */
/*                                                              */
/*  5900indx.c file:                                            */
/*      handles indexing for all parameters                     */
/*                                                              */
/****************************************************************/


/*

$Header:   G:/5200/9000main.vcs/mainindx.c_v   1.0   14 Nov 1996  9:15:24   PAULC  $
$Log:   G:/5200/9000main.vcs/mainindx.c_v  $
      
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



/****   EXTERNAL DECLARATION    *********************************************/
  extern int      IndxStr[];

  extern MENUTYPE AreaOn;
  extern MENUTYPE EndOpCls;
  extern MENUTYPE BegCCAT;
  extern MENUTYPE EndCCAT;
  extern MENUTYPE BegAreaRelays;
  extern MENUTYPE EndAreaRelays;
  extern MENUTYPE DurEnable;
  extern MENUTYPE SendOCByArea;
  extern MENUTYPE SendPrimOC;
  extern MENUTYPE SendRestOC;

  extern MENUTYPE PrimaryDev;
  extern MENUTYPE EndAccessRpt;

  extern MENUTYPE BegCP;
  extern MENUTYPE BegPrint;
  extern MENUTYPE EndKpAreaScope;
  extern MENUTYPE EndPtAreaScope;
  extern MENUTYPE custom_1;
  extern MENUTYPE custom_2b;

  extern MENUTYPE auth_1;
  extern MENUTYPE auth_62;

  extern MENUTYPE menu_2;
  extern MENUTYPE menu_10;


  extern int    AreaIndx;
  extern int    RouteIndx;
  extern int    KypdIndex;
  extern int    PrintIndex;

  extern int    AuthIndex;
  extern int    custom_indx;
  extern int    menu_indx;




  /**************************************************************************/
  /*                                                                        */
  /*   Name: INDEXER                                                        */
  /*   Function: The function of this routine is to modify the address for  */
  /*             the input menu number using an index variable and a record */
  /*             size. This modified address will then be returned to the   */
  /*             format (system or custom) for use in locating indexed param*/
  /*                                                                        */
  /*   Inputs:  The menu number, base address of data                       */
  /*   Outputs: if this menu supports indexing                              */
  /*              change Address passed and                                 */
  /*              return the pointer to the Ascii string of the index value */
  /*            else                                                        */
  /*              return 0 (value doesn't matter)                           */
  /*   Globals affected:   None that I know of.                             */
  /*   Modifications:  Created 6/25/89                                      */
  /*                                                                        */
  /**************************************************************************/
int INDEXER (menumb,Address)
int menumb;
char **Address;
{
   char idx_value;
   char size;
   char decimal_offset;


   /*==== Area   =================================== */
   /*  All AREA indexed variables use "AreaIndx"                      */
   /*  ... except the custom menus between SendOCByArea & SendPrimOC  */
   if ( ((menumb >= GetMenu(&AreaOn))        && (menumb <= GetMenu(&EndOpCls)))
     || ((menumb >  GetMenu(&BegCCAT))       && (menumb <= GetMenu(&EndCCAT)))
     || ((menumb >  GetMenu(&BegAreaRelays)) && (menumb <= GetMenu(&EndAreaRelays))))
   {
		if (   menumb != GetMenu(&DurEnable)
			&& menumb != GetMenu(&SendOCByArea)
			&& menumb != GetMenu(&SendRestOC)
			&& menumb != GetMenu(&SendPrimOC)    )
				*Address = *Address + (AreaIndx * AREA_SIZE);

      itoa(AreaIndx+1,IndxStr);
      return(IndxStr);
   }


   /*==== Custom Functions  ================================= */
   /*====   Handled separately since index is 128 thru 143 == */
   if ((menumb >= GetMenu(&custom_1)) && (menumb <= GetMenu(&custom_2b)))
   {
      idx_value = custom_indx - 128;
      *Address = *Address + (idx_value * COMMAND_SIZE);  /* new address */
      itoa (custom_indx,IndxStr);                      /* convert index value to ascii */
      return(IndxStr);
   }


   /* --- the following checks use these three variables to save space ---- */
   idx_value = 0;/* if no new value assign; address will be unchanged */
   size = 1;/* if no new value assign; address will be changed only by idx_value */
   decimal_offset = TRUE;/* most index variables use decimal offset */


   /* --- The first few INDEXs use a unique address calculation or no address calculation  --- */
   /*==== Authorities  ==============================*/
   if ((menumb >= GetMenu(&auth_1)) && (menumb <= GetMenu(&auth_62)))
       idx_value = AuthIndex+1;

   /*==== Printers  =============================== */
   else if ((menumb > GetMenu(&BegPrint)) && (menumb <= GetMenu(&EndPtAreaScope)))
   {
      *Address = *Address + (PrintIndex * DEVICE_SIZE);
      idx_value = PrintIndex+17;         
   }



   if (idx_value != 0)               
   {
      itoa (idx_value,IndxStr);       
      return(IndxStr);
   }



   /* --- all following INDEXs use the address calculation at the end --- */


   /*==== Keypads  ================================= */
   if ( ((menumb > GetMenu(&BegCP)) && (menumb < GetMenu(&EndKpAreaScope))) )
   {
      idx_value = KypdIndex;
      size      = DEVICE_SIZE;
   }


   /*==== MENU LIST  =============================== */
   else if ((menumb >= GetMenu(&menu_2)) && (menumb <= GetMenu(&menu_10)))
   {
      idx_value = menu_indx;
      size      = MENU_SIZE;
   }

   /*==== Phone Routes =================================== */
   if ((menumb >= GetMenu(&PrimaryDev)) && (menumb < GetMenu(&EndAccessRpt)))
   {
     idx_value = RouteIndx;
	 size = ROUTING_SIZE;
	}



   *Address = *Address + (idx_value * size);   /* new address */
   if (decimal_offset)
      idx_value++;
   itoa (idx_value,IndxStr);                   /* convert index value to ascii */
   return(IndxStr);

}


