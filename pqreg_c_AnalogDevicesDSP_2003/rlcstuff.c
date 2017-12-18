/**SDOC*************************************************************************
 *
 *  rlcstuff.c - image processing for PQ camera
 *
 *  Author: Mark Colvin				June 12, 2001
 *
 * $Header:   K:/Projects/PQCamera/PQCamera/PQLIB/rlcstuff.c_v   1.3   Dec 30 2002 13:43:14   APeng  $
 *
 * $Log:   K:/Projects/PQCamera/PQCamera/PQLIB/rlcstuff.c_v  $
 * 
 *    Rev 1.3   Dec 30 2002 13:43:14   APeng
 * 2002-12-20: Version 0.60.6:
 * 
 * added code for grayscale average made roi size multible of 8
 * added new module for loading of test images
 * added mechanism for converting images
 * added passing of features to narrowroi
 * changed 'bigger'-parameter of most AlignRoi-calls to FALSE
 * added alignment of ROIs,
 * added template function for pixel access more speed optimizations
 * NarrowRoi-bug seemly fixed, some speed optimizations
 * turned 'CheckBlobs' into external module 'chkblob'
 * small changes to reduce code size
 * 
 * 
 *
 *    Rev 1.2   Dec 10 2002 15:59:24   APeng
 * some work has been done to the reduced
 * ROI
 *
 *    Rev 1.0   09 Jul 2001 12:40:22   MARKC
 * Checked in from initial workfile by PVCS Version Manager Project Assistant.
 *
 *
 *********************************************************************SDOCEND**/

/*=======================  I N C L U D E   F I L E S  ========================*/
#include <cam0.h>
#include <vcrt.h>
#include <vc65.h>
#include <vclib.h>
#include <macros.h>
#include <string.h>
#include <limits.h>
#include <bool.h>

#include "config.h"
#include "global.h"
#include "comm.h"
#include "pqlib.h"

/*==================  E X T E R N A L   F U N C T I O N S  ===================*/

int *sgm_init(rlc_desc *x);
int *rlc_sgmt(rlc_desc *x);

/*==================  E X T E R N A L   V A R I A B L E S  ===================*/

/*================  I N T E R N A L   D E F I N I T I O N S  =================*/

/*========  I N T E R N A L   F U N C T I O N   P R O T O T Y P E S  =========*/

/*==================  I N T E R N A L   V A R I A B L E S  ===================*/

/*===========================  F U N C T I O N S  ============================*/

/*
	GMI - find objects for PrintQuick

*/
/*=======================  I N C L U D E   F I L E S  ========================*/


/**FDOC*************************************************************************
 *
 *  Function:       rlcout2
 *
 *  Author:         Mark Colvin             Date: June 12, 2001
 *
 *  Description:
 *
 *  Parameters:
 *
 *  Returns:
 *
 *  Side effects:
 *
 *  History:
 *
 *********************************************************************FDOCEND**/
int rlcout2(image *a, long rlc, int dark, int light)
{
    long st;
    int dx, dx2, dy, hp;
    int *p1, *pr, *px=NULL;
    int i;

    rlc += 2;               /* skip label information   */
    dx = rd20(rlc);
    rlc++;                  /* read line delimiter      */
    dy = rd20(rlc);
    rlc++;                  /* read dy                  */


    dx2 = dx / 2;

    if ( (a->dx != dx) || (a->dy != dy) )
        return(-1);                         /* format error   */

    st = a->st >> 1;
    hp = a->pitch / 2;

    p1 = vcmalloc(dx);
	if ( !p1 )
		return(-2);

    pr = vcmalloc(dx+1);    /* need one more word for line delimiter    */
	if ( !pr )
	{
		vcfree(p1);
		return(-2);
	}

    {
        for(i=0;i<dy;i++)
        {
           px=pr;
           rlc += (long)rdrlc(dx,px,rlc);
           rlcoutf(pr,p1,dx,dark,light);
           blwrb(dx2,p1,st);
           st += (long)hp;
        }
        vcfree(pr);
        vcfree(p1);
    }

    return(0);
}


/*
	sgmt		segment run length code (object labeling)

	long sgmt(long rlc, long slc)

	description		The function sgmt() segments the run length code
					stored starting at the DRAM address rlc.
					The address of the object number information slc
					is also passed to the function - enough DRAM must
					be available for the memory needs of the RLC.
					The passed address slc is stored in the run length
					code at address rlc and rlc+1. This means that
					it is labelled RLC.
					The number of found objects and the object number
					information are stored in the SLC.
					The object numbers begin at 0; a total of 1580 object
					numbers are allowed. An 'object number overrun' occurs
					if this number is exceeded.

					The return value of this function is the next free DRAM
					address. It returns 0L in case of object number overrun.

			Note	The image width dx of the RLC determines how much memory
					the function requires on the heap. For image size 740,
					the entire heap is required. However, the entire heap
					is available after the function is exited.

					This version is checked to be sure there is enough heap
					space. If not enough, return with -1L.
*/

#define MAX_SLC_OBJS 1580               /* max number of active objects     */

long sgmt2(long rlc, long slc)
{
    rlc_desc y;
    int i, dx, dx1, dy;
    int *sx,*s;
    int count, offset, error=0;
    long cnt, slc0 = slc;
    int obj_lst = 0;

	/* long heap = CheckHeap(); */

    wr32(slc,rlc);
    rlc += 2L;                      /* write slc address into the rlc area  */
    dx = rd20(rlc);
    rlc++;                          /* read line delimiter                  */
    dy = rd20(rlc);
    rlc++;                          /* read dy - rlc line count             */
    dx1 = dx+1;						/* max rlc line size                    */

/*	SendDbgMsg(DBG_DEBUG, "sgmt2: dx1=%d nobjs=%d", dx1, MAX_SLC_OBJS);
*/
    wr20(0,slc);
	slc++;							/* write number of objects = 0 first    */

	/* memory allocations */
    y.eqv_tbl=vcmalloc(MAX_SLC_OBJS);   /* equivalence table                    */
    y.rlca=vcmalloc(dx1);				/* ptr to rlc of upper line             */
    y.rlcb=vcmalloc(dx1);				/* ptr to rlc of lower line             */
    y.obja=vcmalloc(dx1);				/* ptr to objects of upper line         */
    y.objb=vcmalloc(dx1);				/* ptr to objects of lower line         */
										/* obj_fre is set by sgm_init()         */
    y.obj_tbl=vcmalloc(MAX_SLC_OBJS);   /* ptr to start of object table         */
    y.obj_lng=MAX_SLC_OBJS;             /* length of object table               */
    y.eol=dx;							/* end of line symbol                   */
    y.obj_act=y.obj_tbl;				/* pointer to active object list        */

	/*	test memory allocations for valid addresses */

    if( !(y.rlca && y.rlcb && y.obja && y.objb && y.obj_tbl && y.eqv_tbl) )
    {                           /* a malloc failed, free the rest and bail out */
/*		SendDbgMsg(DBG_ERRORS, "sgmt2: no heap ot%4x ob=%4x oa=%4x rb%4x ra%4x et%4x",
				 y.obj_tbl, y.objb, y.obja, y.rlcb, y.rlca, y.eqv_tbl );
*/      if( y.obj_tbl )
            vcfree( y.obj_tbl );
        if( y.objb )
            vcfree( y.objb );
        if( y.obja )
            vcfree( y.obja );
        if( y.rlcb )
            vcfree( y.rlcb );
        if( y.rlca )
            vcfree( y.rlca );
        if( y.eqv_tbl )
            vcfree( y.eqv_tbl );
        return(-1L);
    }
    /* read first line of rlc                                               */


    rlc += (long)rdrlc(dx, y.rlca, rlc);

    /* init tables and generate labels for first line                       */


    sx=sgm_init(&y);
    count = (int) (sx - (int *)y.obja) & 0x3fff;    /* compiler bug         */
    blwrw(count, y.obja, slc);                      /* output slc to DRAM   */
    slc += (long) count;                            /* update slc pointer   */


    for(i=0;i<(dy-1);i++)
    {
        rlc += (long)rdrlc(dx,y.rlcb,rlc);            /* read rlc             */
        sx=rlc_sgmt(&y);                              /* do labeling          */
        if(((int)y.obj_fre & 0x3fff) < obj_lst)       /* check for object ovr */
        {
			error = 1;                                  /* set error flag       */
			break;
		}
        obj_lst = (int)y.obj_fre & 0x3fff;
        count = (int) (sx - (int *)y.objb) & 0x3fff;  /* compiler bug         */
        blwrw(count,y.objb,slc);                      /* output slc to DRAM   */
        slc += (long) count;                          /* update slc pointer   */
        rlc_exc2(&y);                                  /* exchange pointers    */
    }

    /* release memory used - except eqv_tbl                                 */

    vcfree(y.obj_tbl);              /* ptr to start of object table         */
    vcfree(y.objb);                 /* ptr to objects of lower line         */
    vcfree(y.obja);                 /* ptr to objects of upper line         */
    vcfree(y.rlcb);                 /* ptr to rlc of lower line             */
    vcfree(y.rlca);                 /* ptr to rlc of upper line             */

	if( error )
	{
		/*SendDbgMsg(DBG_DEBUG, "sgmt2: dy=%d newobjs=%d objs=%d", i,
			((int)y.obj_fre & 0x3fff), 	obj_lst);*/

	    vcfree(y.eqv_tbl);
		return(0L);                 /* return 0L if object number overrun   */

	}
    /* sort equivalence table and                                           */
    /* output number of objects found                                       */

    wr20(sort_eqv2(&y), slc0); slc0++;

    /* second pass for unification of object numbers                        */

    cnt = slc - slc0;               /* slc count                            */

    sx = vcmalloc( 1024 );
    offset = (int)y.eqv_tbl & 0x3fff;

    do
    {
        if(cnt < 1024L)
            count = (int) cnt;
        else
            count = 1024;

        blrdw(count,sx,slc0);         /* read slc into buffer                 */

        s = sx;
        for(i=0;i<count;i++)          /* update slc number                    */
            *s++ = * (int *) (*s) - offset;

        blwrw(count,sx,slc0);         /* write back to DRAM                   */

        slc0 += (long) count;
        cnt -= (long) count;
    } while (cnt != 0L);

    /* release rest of memory used                                          */

    vcfree(sx);
    vcfree(y.eqv_tbl);

		/*
		if( CheckHeap() != heap )
		{
			SendDbgMsg(DBG_ERRORS, " sgmt2, heap error %x sb %x", CheckHeap(), heap);
		}
		*/

    if (error==0)
		return(slc);
    else
		return(0L);                   /* return 0L if object number overrun   */
}

/**FDOC*************************************************************************
 *
 *  Function:       rlc_exc2
 *
 *  Author:         Mark Colvin             Date: June 12, 2001
 *
 *  Description:
 *
 *  Parameters:
 *
 *  Returns:
 *
 *  Side effects:
 *
 *  History:
 *
 *********************************************************************FDOCEND**/
void rlc_exc2(rlc_desc *x)            /* exchange pointers in rlc descriptor  */
{
    unsigned int *q;

    q       = x->rlca;
    x->rlca = x->rlcb;
    x->rlcb = q;

    q       = x->obja;
    x->obja = x->objb;
    x->objb = q;

}


/**FDOC*************************************************************************
 *
 *  Function:       sort_eqv2
 *
 *  Author:         Mark Colvin             Date: June 12, 2001
 *
 *  Description:
 *
 *  Parameters:
 *
 *  Returns:
 *
 *  Side effects:
 *
 *  History:
 *
 *********************************************************************FDOCEND**/
int sort_eqv2(rlc_desc *x)
{
    int *p, *q, *y, *z;
    int i, imax;

    p=x->eqv_tbl;
    z=p;
    imax=((int)x->obj_fre-(int)x->obj_tbl) & 0x3fff;

    for(i=0;i<imax;i++)
    {
        if ( (*p - ((int)p & 0x3fff))   > 0)
        {
            q = p;
            while((((y = (int *) *q) - z) & 0x3fff) != 0)
            {
                *q = (int)z & 0x3fff;
                q = y;
            }
            z++;
        }
        else
        {
            if ( ((*p - (int)p) & 0x3fff ) == 0)
            {
                *p = (int)z++  & 0x3fff;
            }
        }
        p++;
    }
    return(((int)z - (int)x->eqv_tbl) & 0x3fff);
}

/* end of file rlcstuff.C */
