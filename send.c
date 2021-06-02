#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#include "sbuf.h"
#include "struct.h"
#include "send.h"


int send_message(struct lsock *cptr, char *message, int length)
{
	sbuf_put(&cptr->sendq, message, length);
	return 0;
}

#define to_digit(ch) ((ch) - '0')
#define to_char(num) ((num) + '0')
#define is_digit(ch) ((unsigned)to_digit(ch) <= '9')


#define zeropad 1
#define altfmt 2
// #define numprefix 4

#define flagshortint 8
#define flaglongint 16
#define flagquadint 32

#define leftjust 64
#define centjust 128

/* only accessible through S and c */
#define flagcap 256

const char numupper[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char numlower[] = "0123456789abcdefghijklmnopqrstuvwxyz";
int tvprintf(struct lsock *cptr, const char *format, va_list ap)
{
	char *fmt;
	char *cp;
	char ch;
	char *src;
	char *eos;
	char *dest;
	char *eod;
	int tlen;
	int collen;
	int num;
/* writeout settings */
	int width; /* width (ie. %3d) */
	int prec;  /* precision (ie. %.4d) */
	unsigned long flags;
	char sign;
	const char *numfmt;
/* aftercalc settings */
	int lpad;
	int rpad;
	int calcsz;
/* buffers */
	int base;
	unsigned long ulval;
	char preout[512];
	char buf[68]; /* buf space for %c %[oubxX...] etc */

	fmt = (char *)format;
	dest = preout;
	eod = dest + sizeof(preout);
	
	tlen = 0;
	collen = 0;
	for(;;)
	{
		/* reset settings */
		flags = 0;
		width = 0;
		prec = -1;
		sign = '\0';
		numfmt = numlower;
		/* scan for a format, or end of string */
		for(cp = fmt; (ch = *fmt) != '\0' && ch != '%'; fmt++);
		if(fmt - cp > 0)
		{
			src = cp;
			eos = fmt;
			goto writeout;
		}
		if(*fmt == '\0')
			break; /* done processing */
		fmt++; /* skip the '%' */
		
getnchar:
		ch = *fmt++;
reschar:
		switch(ch)
		{
			case ' ':
				if(!sign)
					sign = ' ';
				goto getnchar;
			case '?':
				if( va_arg(ap, int) )
					flags |= altfmt;
				goto getnchar;
			case '#':
				flags |= altfmt;
				goto getnchar;
			case '*':
				/* XXX */
				goto getnchar;
			case '-':
				if(flags & leftjust)
					flags |= centjust;
				else
					flags |= leftjust;
				goto getnchar;
			case '+':
				sign = '+';
				goto getnchar;
			case '.':
				ch = *fmt++;
				if(ch == '*')
				{
					/* XXX */
					goto getnchar;
				}
				num = 0;
				while(is_digit(ch))
				{
					num *= 10;
					num += to_digit(ch);
					ch = *fmt++;
				}
				prec = num < 0 ? -1 : num;
				goto reschar;
				break;
			case '0':
				flags |= zeropad;
				goto getnchar;
				break;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				num = 0;
				do
				{
					num *= 10;
					num += to_digit(ch);
					ch = *fmt++;
				} while( is_digit(ch) );
				
				width = num;
				goto reschar;
			case 'h':
				flags |= flagshortint;
				goto getnchar;
			case 'l':
				if(flags & flaglongint)
					flags |= flagquadint;
				else
					flags |= flaglongint;
				
				goto getnchar;
			case 'q':
				flags |= flagquadint;
				goto getnchar;
			case 'c':
				if(flags & altfmt)
					flags |= flagcap;
				sign = '\0';
				src = buf;
				eos = src + 1;
				*src = (char)va_arg(ap, int);
				goto writeout;
			case 's':
				src = va_arg(ap, char *);
				if(src == NULL)
					src = "(null)";
				if(flags & altfmt)
					flags |= flagcap;
				if(prec < 0)
					for(eos = src; *eos; eos++);
				else
				{
					char *eob;
					eob = src + prec;
					for(eos = src; eos < eob && *eos; eos++);
				}
				goto writeout;
			case 'X':
				numfmt = numupper;
				base = 16;
				goto nosign;
			case 'x':
				numfmt = numlower;
				base = 16;
				goto nosign;
			case 'o':
				base = 8;
				goto nosign;
			case 'b':
				base = 2;
				goto nosign;
			case 'u':
				base = 10;
				goto nosign;
nosign:
				ulval = flags & flaglongint ? va_arg(ap, long)
				: flags & flagshortint ? (long)(short)va_arg(ap,int)
				: (long)va_arg(ap,int) ;
				
				goto number;
			case 'p':
				base = 16;
				ulval = (unsigned long)(void *)va_arg(ap, void *);
				
				goto number;
			case 'D':
				flags |= flaglongint;
			case 'd':
			case 'i':
				ulval = flags & flaglongint ? va_arg(ap, long)
				: flags & flagshortint ? (long)(short)va_arg(ap,int)
				: (long)va_arg(ap,int) ;
				
				if((long)ulval < 0)
				{
					ulval = -ulval;
					sign = '-';
				}
				base = 10;
				goto number;
number:
				if(prec >= 0)
					flags &= ~zeropad;
				
				eos = buf + sizeof(buf);
				src = eos;
				if(base <= 10)
				{
					int dprec = prec;
					
					if(dprec == 0)
						goto writeout;
						
					if(ulval == 0)
					{
						*(--src) = '0';
						goto writeout;
					}
					for(;ulval > 0 && dprec; ulval /= base, dprec--)
						*(--src) = to_char( ulval % base );
					if(flags & altfmt)
					{
						*(--src) = ch;
						*(--src) = '0';
					}
					goto writeout;
				}
				else
				{
					int dprec = prec;
					
					if(dprec == 0)
						goto writeout;
						
					if(ulval == 0)
					{
						*(--src) = '0';
						goto writeout;
					}
					for(;ulval > 0 && dprec; ulval /= base, dprec--)
						*(--src) = numfmt[ ulval % base];
					if(flags & altfmt)
					{
						*(--src) = ch;
						*(--src) = '0';
					}
					goto writeout;
				}
				
				continue;
			case 'z':
				src = eos = buf;
				width -= collen;
				if(width < 0)
					continue; /* nothing to do */
				flags &= ~(centjust);
				flags |= leftjust;
			
				goto writeout;
			default:
				if(!ch)
					goto done;
				sign = '\0';
				src = buf;
				eos = src + 1;
				*src = ch;
				goto writeout;
		}

writeout:
		calcsz = eos - src;
		if(sign)
			calcsz++;
		lpad = rpad = 0;
		if(width > calcsz)
		{
			if(flags & centjust)
			{
				rpad = width - calcsz;
				lpad = rpad / 2;
				rpad -= lpad;
			}
			else if(flags & leftjust)
				rpad = width - calcsz;
			else
				lpad = width - calcsz;
		}


		if((flags & (leftjust|centjust)) && !(flags & zeropad))
		{
			for(;lpad;lpad--)
			{
				if(dest >= eod)
				{
					send_message(cptr, preout, dest - preout);
					dest = preout;
				}
				collen++;
				tlen++;
				*dest++ = ' ';
			}
		}
		if(sign)
		{
			if(dest >= eod)
			{
				send_message(cptr, preout, dest - preout);
				dest = preout;
			}
			collen++;
			tlen++;
			*dest++ = sign;
		}

		if(flags & zeropad)
		{
			for(;lpad;lpad--)
			{
				if(dest >= eod)
				{
					send_message(cptr, preout, dest - preout);
					dest = preout;
				}
				collen++;
				tlen++;
				*dest++ = '0';
			}
		}

		if(flags & flagcap)
		{
			if(src < eos)
			{
				if(dest >= eod)
				{
					send_message(cptr, preout, dest - preout);
					dest = preout;
				}
				collen++;
				tlen++;
				*dest++ = toupper(*src++);
			}
		}
		while(src < eos)
		{
			if(dest >= eod)
			{
				send_message(cptr, preout, dest - preout);
				dest = preout;
			}
			if(*src == '\r' || *src == '\n')
				collen = 0;
			else if(*src >= ' ')
				collen++;
			tlen++;
			*dest++ = *src++;
		}

		for(;rpad;rpad--)
		{
			if(dest >= eod)
			{
				send_message(cptr, preout, dest - preout);
				dest = preout;
			}
			collen++;
			tlen++;
			*dest++ = ' ';
		}
	}
done:		
	if(dest > preout)
	{
		send_message(cptr, preout, dest - preout);
	}
	
	
	
	return tlen;
}

int tprintf(struct lsock *cptr, const char *format, ...)
{
	va_list ap;
	int res;
	va_start(ap, format);
	res = tvprintf(cptr, format, ap);
	va_end(ap);
	return res;
}


