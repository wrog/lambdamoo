/******************************************************************************
  Copyright (c) 1992, 1995, 1996 Xerox Corporation.  All rights reserved.
  Portions of this code were written by Stephen White, aka ghond.
  Use and copying of this software and preparation of derivative works based
  upon this software are permitted.  Any distribution of this software or
  derivative works must comply with all applicable United States export
  control laws.  This software is made available AS IS, and Xerox Corporation
  makes no warranty about the software, its performance or its conformity to
  any specification.  Any person obtaining a copy of this software is requested
  to send their name and post office or electronic mail address to:
    Pavel Curtis
    Xerox PARC
    3333 Coyote Hill Rd.
    Palo Alto, CA 94304
    Pavel@Xerox.Com
 *****************************************************************************/

#include <float.h>
#include "my-stdarg.h"
#include "my-string.h"
#include "my-stdio.h"

#include "config.h"
#include "log.h"
#include "storage.h"
#include "streams.h"

Stream *
new_stream(int size)
{
    Stream *s = mymalloc(sizeof(Stream), M_STREAM);

    s->buffer = mymalloc(size, M_STREAM);
    s->buflen = size;
    s->current = 0;

    return s;
}

static void
grow(Stream * s, int newlen)
{
    char *newbuf;

    newbuf = mymalloc(newlen, M_STREAM);
    memcpy(newbuf, s->buffer, s->current);
    myfree(s->buffer, M_STREAM);
    s->buffer = newbuf;
    s->buflen = newlen;
}

void
stream_add_char(Stream * s, char c)
{
    if (s->current + 1 >= s->buflen)
	grow(s, s->buflen * 2);

    s->buffer[s->current++] = c;
}

void
stream_delete_char(Stream * s)
{
    if (s->current > 0)
      s->current--;
}

void
stream_add_string(Stream * s, const char *string)
{
    int len = strlen(string);

    if (s->current + len >= s->buflen) {
	int newlen = s->buflen * 2;

	if (newlen <= s->current + len)
	    newlen = s->current + len + 1;
	grow(s, newlen);
    }
    strcpy(s->buffer + s->current, string);
    s->current += len;
}

static const char *
itoa(int n, int radix)
{
    if (n == 0)			/* zero produces "" below. */
	return "0";
    else if (n == -2147483647 - 1)	/* min. integer won't work below. */
	switch (radix) {
	case 16:
	    return "-7FFFFFFF";
	case 10:
	    return "-2147483648";
	case 8:
	    return "-17777777777";
	default:
	    errlog("STREAM_PRINTF: Illegal radix %d!\n", radix);
	    return "0";
    } else {
	static char buffer[20];
	char *ptr = buffer + 19;
	int neg = 0;

	if (n < 0) {
	    neg = 1;
	    n = -n;
	}
	*(ptr) = '\0';
	while (n != 0) {
	    int digit = n % radix;
	    *(--ptr) = (digit < 10 ? '0' + digit : 'A' + digit - 10);
	    n /= radix;
	}
	if (neg)
	    *(--ptr) = '-';
	return ptr;
    }
}

static const char *
dbl_fmt(void)
{
    static const char *fmt = 0;
    static char buffer[10];

    if (!fmt) {
	sprintf(buffer, "%%.%dg", DBL_DIG);
	fmt = buffer;
    }
    return fmt;
}

void
stream_printf(Stream * s, const char *fmt,...)
{
    char buffer[40];
    va_list args;

    va_start(args, fmt);
    while (*fmt) {
	char c = *fmt;

	if (c == '%') {
	    char pad = ' ';
	    int width = 0, base;
	    const char *string = "";	/* initialized to silence warning */

	    while ((c = *(++fmt)) != '\0') {
		switch (c) {
		case 's':
		    string = va_arg(args, char *);
		    break;
		case 'x':
		    base = 16;
		    goto finish_number;
		case 'o':
		    base = 8;
		    goto finish_number;
		case 'd':
		    base = 10;
		  finish_number:
		    string = itoa(va_arg(args, int), base);
		    break;
		case 'g':
		    sprintf(buffer, dbl_fmt(), va_arg(args, double));
		    if (!strchr(buffer, '.') && !strchr(buffer, 'e'))
			strcat(buffer, ".0");	/* make it look floating */
		    string = buffer;
		    break;
		case '0':
		    if (width == 0) {
			pad = '0';
			continue;
		    }
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		    width = width * 10 + c - '0';
		    continue;
		default:
		    errlog("STREAM_PRINTF: Unknown format directive: %%%c\n",
			   c);
		    goto abort;
		}
		break;
	    }

	    if (width && (width -= strlen(string)) > 0)
		while (width--)
		    stream_add_char(s, pad);
	    stream_add_string(s, string);
	} else
	    stream_add_char(s, *fmt);

	fmt++;
    }

  abort:
    va_end(args);
}

void
free_stream(Stream * s)
{
    myfree(s->buffer, M_STREAM);
    myfree(s, M_STREAM);
}

char *
reset_stream(Stream * s)
{
    s->buffer[s->current] = '\0';
    s->current = 0;
    return s->buffer;
}

char *
stream_contents(Stream * s)
{
    s->buffer[s->current] = '\0';
    return s->buffer;
}

int
stream_length(Stream * s)
{
    return s->current;
}

char rcsid_streams[] = "$Id$";

/*
 * $Log$
 * Revision 1.7  2010/04/23 05:04:28  wrog
 * remove max=0 meaning no limit
 *
 * Revision 1.6  2010/04/22 21:47:48  wrog
 * Improve stream robustness (rob@mars.org)
 *
 * Revision 1.5  2010/03/30 22:13:22  wrog
 * Added stream exception API to catch mymalloc failures
 *
 * Revision 1.4  2006/12/06 23:57:51  wrog
 * New INPUT_APPLY_BACKSPACE option to process backspace/delete characters on nonbinary connections (patch 1571939)
 *
 * Revision 1.3  1998/12/14 13:19:01  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:19:28  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:01  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.5  1996/03/19  07:14:02  pavel
 * Fixed default printing of floating-point numbers.  Release 1.8.0p2.
 *
 * Revision 2.4  1996/03/11  23:33:04  pavel
 * Added support for hexadecimal to stream_printf().  Release 1.8.0p1.
 *
 * Revision 2.3  1996/03/10  01:04:38  pavel
 * Increased the precision of printed floating-point numbers by two digits.
 * Release 1.8.0.
 *
 * Revision 2.2  1996/02/08  06:50:39  pavel
 * Added support for %g conversion in stream_printf().  Renamed err/logf() to
 * errlog/oklog().  Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.1  1996/01/11  07:43:01  pavel
 * Added support for `%o' to stream_printf().  Release 1.8.0alpha5.
 *
 * Revision 2.0  1995/11/30  04:31:43  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.5  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.4  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.3  1992/08/10  16:52:24  pjames
 * Updated #includes.
 *
 * Revision 1.2  1992/07/21  00:06:51  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS ident. string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
