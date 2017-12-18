/*
********************************************************************************

        Modul	: vsprintf
        System	: VC/RT
        Autor	: Manuel Jochem
                  FiberVision GmbH
                  Hirzenrott 2 - 4
                  D-52076 Aachen
                  Tel.: 02408 / 1408-21
                  Fax : 02408 / 1408-14
        Sprache	: ANSI C + ADSP Extensions
        Version : $Revision:   1.0  $
        Stand   : $Date:   May 02 2003 15:59:44  $

********************************************************************************

	$Id: vsprintf.c,v 1.2 1999/11/10 17:39:10 Manuel Exp $

********************************************************************************
*/

#include <stdarg.h>
#include <string.h>	/* strlen */
#include <stdlib.h>	/* _ultoa */
#include <ctype.h>	/* isdigit, toupper*/
#include <stdio.h>

/******************************************************************************/

#define ENDSTR '\0' /* end of a string */
#define ZERO '0'	/* zero */
#define DOT  '.'	/* dot */
#define LF 10		/* line feed */
#define CR 13		/* carriage return */

static __inline__ int todigit(char c)
{
/*
  if (c<='0') return 0;
  if (c>='9') return 9;
*/
  return c - ZERO;
}

/******************************************************************************/

int intvsprintf (char *stream, const char *format, va_list arg_ptr)
{
	register char pch;
	register int  pi,pj,pk,longflag;
	int pmaxwidth,pmaxpoints;
	char ps[BUFSIZ];
	char *strp;
	char *streamptr = stream;
	long pvalue;
	unsigned long ptemp;

	while ((pch = *format++))
	{
		if (pch == '%')
		{
			while((pch=*format++) == ZERO) /* nop */;
			pi = 1;
			pj = 1;
			longflag   = 0;
			pmaxwidth  = 0;
			pmaxpoints = 6;

			while(isdigit(pch))
			{
/*#define ENGEL*/
#ifdef ENGEL
				pmaxwidth += todigit(pch) * pi;
				pi *= 10;
				pch = *format++;
#else
				pmaxwidth = pmaxwidth * 10 + todigit(pch);
				pch = *format++;
#endif
			}

			if (pch == DOT)
			{
				pmaxpoints = 0;
				pch = *format++;
				while(isdigit(pch))
				{
#ifdef ENGEL
					pmaxpoints += todigit(pch) * pj;
					pj *= 10;
					pch = *format++;
#else
					pmaxpoints = pmaxpoints * 10 + todigit(pch);
					pch = *format++;
#endif
				}
			}

			if (toupper(pch) == 'L')
			{
				longflag = 1;                        /* flag for LONG */
			   	pch = *format++;
			}

			switch (pch=toupper(pch))
			{
			  case 'D':
			  case 'U':
			  case 'X':
			    if (longflag == 0)                    /* read int  */
				{
					pvalue = (long)va_arg(arg_ptr,int);
					ptemp = (unsigned long) pvalue & 0xffffL;
				}
				else
				{                             		/* read long */
					pvalue = va_arg(arg_ptr,long);
					ptemp = (unsigned long) pvalue;
				}

				if (pch == 'D' && pvalue < 0L)
			  	{
					*streamptr++ = '-';
					pvalue = -pvalue;
					ptemp = (unsigned long) pvalue;
				}

				if ((pch == 'D') || (pch == 'U'))
					pk = 10;
				else
					pk = 16;

				_ultoa (ptemp, ps, pk);

				pmaxwidth = pmaxwidth - strlen(ps);

				while (pmaxwidth > 0)
				{
					*streamptr++ = ZERO;
					pmaxwidth--;
				}

				for (pi=0; ps[pi] != ENDSTR; pi++)
					*streamptr++ = ps[pi];

				break;
 			  case 'C':
				*streamptr++ = (char) (va_arg(arg_ptr,int));
				break;
 			  case 'S':
			    strp = (char*) (va_arg(arg_ptr, char*));
				while (*strp != ENDSTR)
				{
					*streamptr++ = *strp++;
					pmaxwidth--;
				}

				while (pmaxwidth > 0)
				{
					*streamptr++ = ' ';
					pmaxwidth--;
				}

				break;
			  case '%':
				*streamptr++ = '%';
				break;
			  default:
				format--;  /* need to back up if unrecognized */
			}		/* switch c */
		}
		else if (pch == '\n')
		{
			*streamptr++ = CR;   /* cr */
			*streamptr++ = LF;   /* lf */
		}
		else
			*streamptr++ = pch;
	}
	*streamptr = ENDSTR;    /* terminate string */

	return strlen(stream);	/* return count of converted chars */
}

/******************************************************************************/
/*************  EOF   *********************************************************/
/******************************************************************************/
