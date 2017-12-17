

/**********************************************************************
*
*	Created:	11/11/1996
*	Author: 	Paul Calinawan
*	Program:	maincomm.c
*	Description:	Communication drivers for MAIN handler.
*
***********************************************************************/

/*

$Header:   G:/5200/9000main.vcs/maincomm.c_v   1.2   22 Nov 1996 20:33:36   PAULC  $
$Log:   G:/5200/9000main.vcs/maincomm.c_v  $
      
         Rev 1.2   22 Nov 1996 20:33:36   PAULC
      Fixed error in checking for product ID.
      
         Rev 1.1   14 Nov 1996 13:22:52   PAULC
      Added 9412 and 9112NEW as the valid panel IDs that the handler
      can communicate.
      
         Rev 1.0   14 Nov 1996  9:15:24   PAULC
      Initial revision.
      
      
*/




#include <tomahawk.h>
#include <deflib.h>
#include <omacro.h>
#include <global.h>
#include <lib.h>

#include "overlay.h"
#include "main.h"


/******************** EXTERNAL DECLARATION ****************************/

extern item_tbl_ty item_table [];

extern  char  MessagLine []; 	/* buffer to hold messages */
extern  char  StatusLine []; 	/* status line buffer	   */
extern  char  send_buff  [];
extern  char  recv_buff  [];
extern  int	 Timeout;		/* 10 minutes timeout */

extern  char  item;
extern  char  sub_item;
extern  char  block_cnt;

extern  char  Codes	[];
extern  char  data_lock  [];

extern  char  comm_during_idle;

int lockcode;

char item_indx;
    
/* Address,   Len, Command   */
char  ack_msg      [3] =  {TOMA_ADDR, 2,   ACK_CMD };
char  nak_msg      [3] =  {TOMA_ADDR, 2,   NAK_CMD };
char  id_req       [4] =  {TOMA_ADDR, 3,   EXEC_FUNC_CMD, FUNC_PROD_ID };

/* ---------------------------------------------------------------------
 *
 * SDI communications
 *   9112 is the master / 5200 is the slave
 *
 * 1.0) Format
 *
 *	 ÚÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÂ	   ÂÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄ¿
 *	 ³ Address ³ Length ³ Command ³ Data  ..... Data ³ Checksum ³
 *	 ÀÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÁ	   ÁÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÙ
 *
 * 2.0) Comm Before contact has been established with the 5200
 *	ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
 *
 *	 ³ Poll ³	 ³ Poll ³	 ³ Poll ³	 ³ Poll ³
 *	 ÀÄÄÄÄÄÄÙ........ÀÄÄÄÄÄÄÙ........ÀÄÄÄÄÄÄÙ........ÀÄÄÄÄÄÄÙ
 *
 *	 Time between polls is dependent upon the number of devices
 *	 (keypads, etc) connected to the SDI buss.
 *
 *	 Minimum Time :
 *	 Maximum Time :
 *
 *
 *
 * 3.0) Contact established but nothing happening (idle)
 *	ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
 *
 *	 ³ Poll ³			 ³ Poll     ³
 *	 ÀÄÄÄÄÄÄÙ.ÚÄÄÄÄÄÄÄÄÄÄ¿...........ÀÄÄÄÄÄÄÄÄÄÄÙ.ÚÄÄÄÄÄÄÄÄÄÄ¿
 *		  ³ 5200 ACK ³			      ³ 5200 ACK ³
 *
 *	 Once the 5200 replys (contact established) then the 9112 speeds up
 *	 its polling rate to approimately once every 10 milsec.
 *
 *
 * 4.0) Contact established but 5200 missed a poll (it was busy doing displays)
 *	ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
 *
 *	 ³ Poll³   ³ No Reply Poll ³	³ No Reply Poll ³    ³ No Reply Poll ³
 *	 ÀÄÄÄÄÄÙ...ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ....ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ....ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
 *
 *
 *	 If the 5200 misses a POLL (which it will do any time it updates
 *	 the display) then the 9112 will switch to a "No Reply Poll" until
 *	 such time as the 5200 resumes communications.
 *
 *	 The "No Reply Poll" is also used to indicate that the last response
 *	 from the 5200 was invalid (i.e. Bad Checksum, Illegal command, etc).
 *
 * 5.0)
 *
 *
 *
 *    The communications utilizes the Request blocked Items so we
 *    can speed up communications.
 *    Example: Skeds has 64 sub-items of 8 bytes each which requires
 *	       64 "Request Item" messages.
 *	       If we group skeds into 4 blocks of 16 sub-items (128 bytes)
 *	       then we only need 4 "Request Block" messages.
 *
 * --------------------------------------------------------------------- */




/************************************************************************/
/*									   */
/*  Name:	 SendParameter ()					   */
/*  Function: This function loads the parameters to panel from	   */
/*		 the programmer.					   */
/*  Inputs:	 None							   */
/*  Outputs:  None							   */
/*									   */
/*  Globals effected : None						   */
/*  Modifications	: created 12/15/89				   */
/*									   */
/************************************************************************/
SendParameter ()
{
    char ret_value;
    
    if ( !Extr_Power() ) 			/* external pwr (panel connected ?) */
        return (PLUG_IN_IDX);			/* no; error exit */

    if ( (ret_value = valid_9112_chk()) != DATALOCK_OK ) /* recv'd datalock code ? */
        return (ret_value);				/* no; error exit */

      /* evaluate prompt values and set appropriate side effects */
    set_side_effects();      

    item_indx = 1;					/* skip item 0 (datalock) */
	item = item_table[item_indx].item_num;
    block_cnt = sub_item = 0;
    setup_send_block (item_indx, sub_item);
    do
    {						/* wait poll; send msg; wait reply */
        if (!poll_send_reply (REQ_CMD, 4)) 	/* some failure ? */
           return(SEND_FAIL_IDX);			/* yes; error exit */

        next_block ();				/* get next block, item, sub-item) */
        setup_send_block (item_indx, sub_item);
    }
    while ( item_indx <= MAX_ITEM) ;

    LoadGPT(Timeout);	      /* reset no key timeout */
    return(SEND_SUCC_IDX);
}  /* end send param */







/*************************************************************************/
/*									    */
/*  Name:	   RecvParameter ()					    */
/*  Function:   This functions receives parameter data from the panel.   */
/*		      1) wait for an "Inquiry" from the panel               */
/*		      2) request & validate datalock code		    */
/*		      3) send "Receive Code" to request data                */
/*		      4) receive param data from panel			    */
/*		      5) checksum data received to check for transmission   */
/*			 errors.					    */
/*  Inputs:	   None 						    */
/*  Outputs:    Returns:						    */
/*		     TRUE  - if parameters received and checksum'd ok.      */
/*		     FALSE - otherwise					    */
/*									    */
/*  Globals effected : PanelImage					    */
/*  Modifications	: created 12/15/89				    */
/*									    */
/*************************************************************************/
RecvParameter ()
{
    char ret_value;
    char reply_size;
	char psr_ret_val;

    if ( !Extr_Power() ) 			/* external pwr (panel connected ?) */
        return (PLUG_IN_IDX);			/* no; error exit */

    if ( (ret_value = valid_9112_chk()) != DATALOCK_OK ) /* recv'd datalock code ? */
        return (ret_value);				/* no; error exit */

    item_indx = 1;						/* skip item 0 (datalock) */
	item = item_table[item_indx].item_num;
    block_cnt = sub_item = 0;
    setup_req_block (item_indx, sub_item);
    do
    {
        reply_size = item_table[item_indx].block_size;
        /* wait poll; send msg; wait reply (+4 is sdi envelope size) */
		psr_ret_val = poll_send_reply(DATA_REPLY_CMD, reply_size+4);
        if (psr_ret_val == 0)
            return(RECV_FAIL_IDX);
		else if (psr_ret_val >= REQ_BAD_CHKSUM && psr_ret_val <= RPY_NO_MSG)
		    return(psr_ret_val);

        store_data (item_indx, sub_item);		/* store data received */
        next_block ();				/* get next block, item, sub-item) */
        setup_req_block (item_indx, sub_item);
    }
    while ( item_indx <= MAX_ITEM) ;

    LoadGPT(Timeout);	      /* reset no key timeout */
    return(RECV_SUCC_IDX);
}  /* end recv param */





/******************************************************************
 *	 Name: idle_comm
 *	       If connected to external power then reset key-input
 *	       timer.
 *	       Maintain idle communications:
 *		  wait for poll,
 *		  send the ACK	(no reply expected)
 *******************************************************************/
idle_comm()
{
    char msg_status;

    if ( Extr_Power() ) 			/* AC or panel connected ? */
    {
        LoadGPT(Timeout); 			/* re-start Warn timeout */
        if (comm_during_idle)			/* ok to communicate */
	    {
	        setup_msg (ack_msg);			/* setup ACK reply */
	        if (!poll_send_reply (0, 0))		/* comm failure ? */
	        {
	            comm_during_idle = FALSE;		/* stop idle comm     */
	            return (FALSE);
	        }
	    }
    }

    return (TRUE);
}








/***********************************************************************/
/*									   */
/*	 Name: valid_9112_chk ();					 */
/*	 Function: display appropriate messages when irregular or no	   */
/*		   connection between the TOMAHAWK and the PANEL is	   */
/*		   detected prior to data file transfer.		   */
/*		   Request and check PRODUCT ID.			   */
/*		   Request and check DATA LOCK code.			   */
/*		   Prevent communications if either is invalid. 	   */
/*		   This function is invoked when SEND and RECEIVE is	   */
/*		   requested.						   */
/*	 Inputs:   None 						   */
/*	 Outputs:  Return						   */
/*		      return COMM_ERR_IDX  if we never recv a valid POLL   */
/*		      return NO_ACCESS_IDX if we recv a bad datalock code  */
/*		 else return DATALOCK_OK				   */
/*	 Globals affected: None 					   */
/*	 Author   : Julian Hemmati					   */
/*									   */
/***********************************************************************/
valid_9112_chk()
{
    char ii;

    /*---- PROD ID check ------ */
    setup_msg (id_req);				/* setup prod id request */
    if (!poll_send_reply (DATA_REPLY_CMD, 17))	/* wait for poll; send msg; wait for reply */
        return (PROD_ID_FAIL_IDX);

	 if (  (recv_buff[PROD_ID_IDX] != D9412_ID)	/* talking to the right panel ? */
		  &&(recv_buff[PROD_ID_IDX] != D9112NEW_ID))
			  return (INCOMPAT_IDX);					/* no; error exit */

    /* ------ DATALOCK Request ---------- */
    setup_req_block (DATALOCK_ITEM, 0);		 /* setup datalock request */
    if (!poll_send_reply (DATA_REPLY_CMD, 6))	 /* wait for poll; send msg; wait for reply */
        return (DATALOCK_FAIL_IDX);


    /*-----------------------------------------------------------------------
     * DATALOCK CHECK (already have panels datalock code)
     * if the code received = the primary then exit w/datalock OK
     * if the code received matches the default or lockcode 1 - 35
     *   then send the primary to the panel &	exit w/datalock OK
     * if the code received matches lockcode 36 - 50
     *   then exit w/datalock OK
     * if all of the aboved failed then exit w/datalock bad
     *----------------------------------------------------------------------- */
    lockcode =  (*(int*) &recv_buff[DATA_IDX]);

    if ( lockcode == *(int *) &Codes[0] )	/* primary codes match ? */
        return (DATALOCK_OK);			/* yes */

    for ( ii = 0; ii <= 35; ii++)		   /* matches default or 1 - 35 ? */
    {
        if ( lockcode == *(int *) &Codes[(ii*2)+2] )
        {
            *(int *)&data_lock[0] = *(int *)&Codes[0];  /* put primary code in the item */
            setup_send_block (DATALOCK_ITEM, 0);	   /* send primary code to panel */
            if (!poll_send_reply (REQ_CMD, 4))	   /* wait for poll; send msg; wait for reply */
	            return (DATALOCK_SEND_IDX);

            CtrString( 1, "LOCK CODE SET");
            return (DATALOCK_OK);
        }
    }


    for ( ii = 36; ii <= 50; ii++)		/* matches code 16 - 50 ? */
    {
        if ( lockcode == *(int *) &Codes[(ii*2)+2] )
            return (DATALOCK_OK);			/* yes */
    }


    /********** recv'd invalid datalock code  ******************************/
    return (NO_ACCESS_IDX);
}





/* --------------------------------------------------------------------
 *	Name :	  poll_send_reply
 *	Function: This routine will wait for a poll then send the message
 *		  and wait for the expected reply.
 *		  If the "reply_lng" is 0 then no reply is expected so
 *		    return TRUE
 *	Inputs:   reply_cmd is the command # of the expected reply.
 *		  reply_lng is the # of bytes in the expected reply.
 *	Outputs:  True	if the reply was successfully received
 *		  False if any step failed.
 *		  NOTE: If the "reply_lng" is 0 then no reply is expected
 *			so return TRUE if the poll/send part was successful.
 *
 *	Note:	  Requires that the message is already set-up in send_buff
 * -------------------------------------------------------------------- */
poll_send_reply (reply_cmd, reply_lng)
char reply_cmd;
char reply_lng;
{
    char msg_status;
    char failure_cnt;
    char ret_code;

    failure_cnt = 0;
    do
    {
        msg_status = wait_for (REQ_CMD, 4);
        if ( msg_status == VALID_MSG || msg_status == NAK_RECVD )
        {
            softsend (BAUD9600, STANDARD_ORDER, POS_POLARITY, &send_buff[0], 
                      send_buff[LEN_IDX]+2);
            if (reply_lng == 0)		/* no reply expected ? */
	            return (TRUE);
        }

        /* wait for reply */
        if ( wait_for (reply_cmd, reply_lng) == VALID_MSG )
            return (TRUE);

        if (!Extr_Power()) 		/* lost AC or panel power ? */
	        return (FALSE); 		/* yes; can't comm          */
     }
     while (++failure_cnt < 6);

     return (FALSE);
}





/* -------------------------------------------------------------------
 *	Name :	    wait_for ();
 *	Function:   This function check for the COMMAND passed.
 *
 *	Inputs:     Command to check for; # of bytes to receive
 *	Outputs:    Returns TRUE if the correct command was received
 *		    otherwise FALSE.
 *
 *	Globals effected: None
 *	Modifications	: Created  6-11-91 mbm
 * ------------------------------------------------------------------- */
wait_for (command, size)
char command;
char size;
{
    char recv_cnt;
    int fail_cnt;

    fail_cnt = 0;
    do
    {
        recv_cnt = softrecv (BAUD9600,STANDARD_ORDER,POS_POLARITY,
                             &recv_buff[0],size,RECV_TIMEOUT);

        if ( (recv_cnt != size)				/* received some bytes ? */
          && (recv_buff[ADDR_IDX] == TOMA_ADDR) )		/* ... our address ? */
        {
            if (recv_buff[CMND_IDX] == NAK_CMD)		/* special case ... nak cmd ? */
	            return(NAK_RECVD);

            if (recv_buff[size-1] == cal_chk_sum (&recv_buff[0],size-1) )
	            return(VALID_MSG);				/* checksums matched  */
            else
	            return(BAD_CHK_SUM);
        }
    }
    while (fail_cnt++ < 100);

    return (NO_MSG);						/* never recvd */
}





/* --------------------------------------------------------------------
 *  setup_req_block (item, sub_item)
 *	 format the comm buffer with a request item message
 * -------------------------------------------------------------------- */
setup_req_block (itemIndex, sub_item)
char itemIndex;
char sub_item;
{
    send_buff [ADDR_IDX]    = TOMA_ADDR | 0x80;	   /* address + reply flag */
    send_buff [LEN_IDX]	   = 4; 		   /* length  */
    send_buff [CMND_IDX]    = REQ_BLOCK_CMD;	    /* command */
    send_buff [DATA_IDX+0]  = item_table[itemIndex].item_num;
    send_buff [DATA_IDX+1]  = sub_item;
    send_buff [5]	   = cal_chk_sum (&send_buff[0], 5);
}






/* --------------------------------------------------------------------
 *  setup_send_block (item, sub_item)
 *	 format the comm buffer with a send item message & data
 * -------------------------------------------------------------------- */
setup_send_block (item, sub_item)
char item;
char sub_item;
{
    char block_lng;
    int	sub_item_offset;
    char *address;

    sub_item_offset  = sub_item * item_table[item_indx].item_size;
    address = item_table[item_indx].item_addr + sub_item_offset;

    block_lng = item_table[item_indx].block_size;

    send_buff [ADDR_IDX]    = TOMA_ADDR | 0x80;		/* address + reply flag */
    send_buff [LEN_IDX]	   = block_lng + 4;		/* length  */
    send_buff [CMND_IDX]    = SEND_BLOCK_CMD;		/* comand  */
    send_buff [DATA_IDX+0]  = item_table[item_indx].item_num;
    send_buff [DATA_IDX+1]  = sub_item;
    memcpy (&send_buff[5], address, block_lng);		/* actual memory to send buffer */
    send_buff [block_lng+5] = cal_chk_sum (&send_buff[0], block_lng+5);
}








/* -------------------------------------------------------------------
 *
 *	Name: cal_chk_sum();
 *	Function: The fuction of this routine is to calculate a checksum
 *		  for the specified buffer.
 *	Inputs: buffer - buffer to perform checksum on.
 *		len    - number of bytes used to calculate checksum.
 *	Outputs: Checksum - returns one byte checksum.
 *	Globals affected:   None
 *	Note: Checksum calculation is sum of bytes specified in buffer
 *	      plus checksum equal zero.
 *	Modifications:	Created 6/7/89 MAS
 *
 * ------------------------------------------------------------------- */
cal_chk_sum (buffer, len)
char *buffer;			/* buffer to use for checksum */
char len;				/* number of bytes used in checksum */
{
    char sum;				/* sum of bytes in one byte */
    char i;				/* counter */

    sum = *buffer & 0x7f;		 /* lose high order bit */
    for (i = 1; i < len; i++)
        sum += *(buffer + i);		/* sum all bytes */

    return (0xff - sum);
}









/* -------------------------------------------------------------------
 *	Name :	  next_block ()
 *	Function: This function will increment through the item/subitem
 *		  table.
 *	Inputs:   None
 *	Outputs:  None
 *	Globals effected: item, sub_item & block variables are changed.
 * ------------------------------------------------------------------- */
next_block ()
{
    sub_item += item_table[item_indx].num_sub_item;		/* next block of sub-items */

    if (++block_cnt >= item_table[item_indx].num_of_blocks)
    {
        block_cnt = sub_item = 0;
        item_indx++;
    }
}








/* -------------------------------------------------------------------
 *	Name :	  store_data ();
 *	Function: This function will determine the parameter address from
 *		  the item/sub-item passed then move the data from the
 *		  receive buffer to the parameter address
 *	Inputs:   item, sub_item
 *	Outputs:  None
 *	Globals effected: parameter data
 * ------------------------------------------------------------------- */
store_data (item, sub_item)
char item;
char sub_item;
{
    char *address;
    char size;
    char ii, len;

    address = item_table[item_indx].item_addr;
    address += sub_item * item_table[item_indx].item_size;

    len = recv_buff[LEN_IDX]-2;
    for (ii=0; ii < len; ii++)
        *(address+ii) =  recv_buff [DATA_IDX+ii];
}






   /* --------------------------------------------------------------------
    *  setup_msg ()
    *	 format the comm buffer with message for the panel
    * -------------------------------------------------------------------- */
 setup_msg (source_addr)
   char *source_addr;
   {
   char length;

   length = *(source_addr + 1) + 1;
   memcpy (send_buff, source_addr, length );		/* move to comm buffer */
   send_buff[ADDR_IDX] |= 0x80;
   send_buff [length] = cal_chk_sum (&send_buff[0], length);
   }





