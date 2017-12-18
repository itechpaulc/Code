/*______________________________________________________________________________

    	$Id: fv_rlc.c,v 1.1 2000/09/21 17:04:44 Manuel Exp $

		Module	: PQ-RLC
        Function: create run-length code
    	Language: ISO C
	    System	: VC/RT

    	Author	: Manuel Jochem
    	          FiberVision GmbH
    	          Jens-Otto-Krag Str. 11
    	          D-52146 Wuerselen
    	          Tel.: +49 (0) 2405 / 4548-21
    	          Fax : +49 (0) 2405 / 4548-14

		(C) FiberVision GmbH, 2000-2003
  ______________________________________________________________________________
*/


#include <vcrt.h>
#include <vclib.h>
#include <cam0.h>
#include <limits.h>
#include <string.h>
#include <vcdefs.h>

#include "global.h" /* console */
#include "pqlib.h"

/*----------------------------------------------------------------------------*/

typedef struct tagPQ_RLC
{
	image* roi;
	int *buffer1;
	int *buffer2;
} PQ_RLC;

#define DUMMY_LINES 15

/*______________________________________________________________________________

	RLC creation in parallel with image aquisition
  ______________________________________________________________________________
*/

long fast_rlc_mk(HPQ_RLC Rlc, int thr, long rlcaddr, long size, int min_obj_size)
{
	long st, hp;
	long maxaddr = rlcaddr + size;
	int dx, dx2, dy;
	image* a = Rlc->roi;
	int *p1  = Rlc->buffer1;
	int *pr  = Rlc->buffer2;
	int line = (int)(a->st / (long) a->pitch) + DUMMY_LINES;
	int *px;
	int count;
	int i;

	st = a->st >> 1;
	dx = a->dx;
	dy = a->dy;
	hp = (long) (a->pitch / 2);
	dx2 = dx / 2;

	wr32(0L,rlcaddr);           /* output 0L for unlabeled rlc              */
	rlcaddr += 2L;
	wr20(dx,rlcaddr++);     	/* output line delimiter                    */
	wr20(dy,rlcaddr++);     	/* output number of lines                   */

	while( getvar(vline) < line) nop;

	for(i = 0; i < dy; i++)
	{
		/* read data from DRAM */
		blrdb(dx2, p1, st);
		st += hp;

		/* create rRLC code for one line */
		px = rlcmkf(dx, thr, p1, pr);

		/* filter out small objects from RLC */
		if( min_obj_size )
			px=rlc_medf(pr, pr, min_obj_size, BLACK, dx);

		count = (px - pr) & 0x3fff;  /* fix compiler bug          */

		if((rlcaddr + (long) count) > maxaddr)
		{
			rlcaddr = 0L;
			break;
		}

		blwrw(count, pr, rlcaddr);
		rlcaddr += (long) count;
	}

	return(rlcaddr);
}

/*______________________________________________________________________________

	RLC feature extraction w/o malloc
  ______________________________________________________________________________
*/

int fast_rlc_ftr2(HPQ_RLC Rlc, long rlc, ftr * f, int n)
{
	int i, dx, dy, cn, nobj;
	int *r = Rlc->buffer1;
	int *s = Rlc->buffer2;
	long area, area2;
	ftr *x;
	long slc;

	slc = rd32(rlc);
	rlc += 2L;					/* read slc address                     */
	dx = rd20(rlc);
	rlc++;						/* read line delimiter                  */
	dy = rd20(rlc);
	rlc++;						/* read dy                              */

	nobj = rd20(slc++);			/* read number of objects               */

	if (nobj > n)
		return (-1);			/* test for object number overrun       */

	/* clear result memory                                                  */
	memset(f, 0, sizeof(ftr) * nobj);
	for (i = 0; i < nobj; i++)
	{
		f[i].x_min = INT_MAX;
		f[i].y_min = INT_MAX;
	}

	for (i = 0; i < dy; i++)
	{
		/* read one line of rlc/slc             */
		cn = rdrlc(dx, r, rlc);	/* read RLC                             */
		blrdw(cn, s, slc);		/* read SLC                             */
		rlc += (long) cn;		/* update RLC pointer                   */
		slc += (long) cn;		/* update SLC pointer                   */

		rl_xyc(r, s, f, sizeof (ftr), i, dx);
		rl_mnmx(r, s, ((int *) f) + 6, sizeof (ftr), i, dx);
		rl_col(r, s, ((int *) f) + 11, sizeof (ftr), dx);
	}

	/* normalize results                                                    */

	x = f;

	for (i = 0; i < nobj; i++)
	{
		area = x->area;
		area2 = area >> 1;

		x->x_center *= SCALE_FACTOR;
		x->y_center *= SCALE_FACTOR;

		x->x_center = (x->x_center + area2) / area;
		x->y_center = (x->y_center + area2) / area;
		(x->x_max)--;
		(x->x_lst)--;
		x++;
	}

	return (nobj);
}

/*______________________________________________________________________________

	initialze PQ_RLC datatype
  ______________________________________________________________________________
*/

HPQ_RLC fast_rlc_init(image *img)
{
	/* create new PQ_RLC object */
	HPQ_RLC rlc = vcmalloc(sizeof(PQ_RLC));

	/* creation successfull? */
	if (rlc)
	{
		/* initialize components */
		rlc->buffer1  = (int*) vcmalloc(img->dx + 1);
		rlc->buffer2  = (int*) vcmalloc(img->dx + 1);
		rlc->roi      = img;

		/* no memory for buffers available? */
		if ((rlc->buffer1 == NULL) || (rlc->buffer2 == NULL))
		{
			/* clean up ...*/
			vcfree((void*) rlc->buffer1);
			rlc->buffer1 = NULL;
			vcfree((void*) rlc->buffer2);
			rlc->buffer2 = NULL;
			vcfree((void*) rlc);
			/* ... and return NULL pointer */
			rlc          = NULL;
		}
	}

	/* return handle to new object */
	return rlc;
}

/*______________________________________________________________________________

	destroy PQ_RLC object
  ______________________________________________________________________________
*/

void fast_rlc_clean(HPQ_RLC rlc)
{
	/* free used memory */
	vcfree((void*) rlc->buffer1);
	vcfree((void*) rlc->buffer2);
	vcfree((void*) rlc);
	rlc = NULL;
	return;
}

/*______________________________________________________________________________

		EOF
  ______________________________________________________________________________
*/

