

/*****************************************************************************/
/*                                                                           */
/*  MAIN 9000C - Communication TABLES                                        */
/*  11/16/1996                                                               */
/*                                                                           */
/*  RADIONICS  PROPRIETARY                                                   */
/*                                                                           */
/*  Paul Calinawan                                                           */
/*                                                                           */
/*****************************************************************************/

/*

$Header:   G:/5200/9000main.vcs/commtbls.c_v   1.0   14 Nov 1996  9:15:20   PAULC  $
$Log:   G:/5200/9000main.vcs/commtbls.c_v  $
      
         Rev 1.0   14 Nov 1996  9:15:20   PAULC
      Initial revision.

*/


#include <tomahawk.h>   /* in include directory */
#include <global.h>
#include "overlay.h"
#include "MAIN.h"

extern char data_lock [];

extern char areas     [] [AREA_SIZE];
extern char phones       [PHONE_SIZE];
extern char panels       [PANEL_SIZE];
extern char commands  [] [COMMAND_SIZE];
extern char routing   [] [ROUTING_SIZE];
extern char menus     [] [MENU_SIZE];
extern char authority [] [AUTHORITY_SIZE];
extern char keypads   [] [DEVICE_SIZE];
extern char printers  [] [DEVICE_SIZE];

extern char Codes[];                        /* lock codes array */



  /*-----------------------------------------------------------------------
   *  item tables
   *    used for sending parameters to/from a 9000
   *    This table is setup to use BLOCKed items
   *    By sending Blocked Items we can speed up communications
   *    Example: Skeds has 64 sub-items of 8 bytes each which requires
   *             64 "Request Item" messages.
   *             If we group skeds into 4 blocks of 16 sub-items
   *             of 128 bytes then we only need 4 "Request Block" messages.
   *------------------------------------------------------------------------ */
item_tbl_ty item_table [9] = {
	0,
	data_lock,                      /* item 0 addr            */
	1,                              /*    # of blocks         */
	2,                              /*    block size          */
	1,                              /*    sub-items per block */
	2,                              /*    item size           */

	1,
	areas,                          /* item 1  addr           */
	8,                              /*    # of blocks         */
	AREA_SIZE,                      /*    block size          */
	1,                              /*    sub-items per block */
	AREA_SIZE,                      /*    item size           */

	2,
	phones,                         /* item 2  addr           */
	1,                              /*    # of blocks         */
	PHONE_SIZE,                     /*    block size          */
	1,                              /*    sub-items per block */
	PHONE_SIZE,                     /*    item size           */

	3,
	panels,                         /* item 3  addr           */
	1,                              /*    # of blocks         */
	PANEL_SIZE,                     /*    block size          */
	1,                              /*    sub-items per block */
	PANEL_SIZE,                     /*    item size           */

	5,
	commands,                       /* item 5  addr           */
	4,                              /*    # of blocks         */
	112,                            /*    block size          */
	4,                              /*    sub-items per block */
	COMMAND_SIZE,                   /*    item size           */

	9,
	routing,                        /* item 9  addr           */
	1,                              /*    # of blocks         */
	104,                            /*    block size          */
	4,                              /*    sub-items per block */
	ROUTING_SIZE,                   /*    item size           */

	11,
	menus,                          /* item 11 addr    */
	1,                              /*    # of blocks         */
	128,                            /*    block size          */
	64,                             /*    sub-items per block */
	MENU_SIZE,                      /*    item size           */

	12,
	authority,                  /* item 12 authority addr */
	4,                              /* Start by ArmByPasscode */
	80,
	20,
	AUTHORITY_SIZE,

	15,
	keypads,                         /* item 15 device addr */
	1,                               /* keypads, printers, automation */
	72,
	12,
	DEVICE_SIZE,

};

