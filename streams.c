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
#include "exceptions.h"
#include "log.h"
#include "storage.h"
#include "streams.h"

/*
 * v must be a variable.
 * set v to 1<<k: k smallest such that v < (1<<k)
 *   (Does not branch.)
 *   (Assumes v>>32 is optimzed out for 32-bit v?)
 */
#define NEXT_2_POWER(v)					\
    (v=(v|(v>>1)),v=(v|(v>>2)),v=(v|(v>>4)),		\
     v=(v|(v>>8)),v=(v|(v>>16)),v=1+(v|(v>>32)))

Stream *
new_stream(size_t size)
{
    Stream *s = mymalloc(sizeof(Stream), M_STREAM);

    /*  We probably get an entire cache line
     *  whether we want it or not, so force size >= 32.
     *  ((XXX separate M_STREAMSTR with 64b alignment?))
     */
    size |= 0x1f;
    NEXT_2_POWER(size);

    s->buffer = mymalloc(size, M_STREAM);
    s->buflen = size;
    s->current = 0;

    return s;
}

Exception stream_too_big;
size_t stream_alloc_maximum = 0;

static int allow_stream_exceptions = 0;

void
enable_stream_exceptions(void)
{
    ++allow_stream_exceptions;
}

void
disable_stream_exceptions(void)
{
    --allow_stream_exceptions;
}

static int
grew(Stream * s, size_t need)
{
    if (s->current + need < s->buflen)
	return 0;

    size_t newlen = s->current + need;
    NEXT_2_POWER(newlen);
    if (allow_stream_exceptions > 0) {
	if (newlen > stream_alloc_maximum) {
	    if (s->current + need < stream_alloc_maximum)
		newlen = stream_alloc_maximum;
	    else
		RAISE(stream_too_big, 0);
	}
    }
    char *newbuf = mymalloc(newlen, M_STREAM);

    memcpy(newbuf, s->buffer, s->current);
    myfree(s->buffer, M_STREAM);
    s->buffer = newbuf;
    s->buflen = newlen;
    return 1;
}

void
stream_add_char(Stream * s, char c)
{
    (void)grew(s, 1);
    s->buffer[s->current++] = c;
}

void
stream_delete_char(Stream * s)
{
    if (s->current > 0)
      s->current--;
}

void
stream_add_bytes(Stream * s, const char *bytes, size_t len)
{
    (void)grew(s, len);
    memcpy(s->buffer + s->current, bytes, len);
    s->current += len;
}

void
stream_printf(Stream * s, const char *fmt,...)
{
    va_list args, pargs;
    ssize_t len;

    va_start(args, fmt);

    va_copy(pargs, args);
    len = vsnprintf(s->buffer + s->current, s->buflen - s->current,
		    fmt, pargs);
    if (len < 0)
	panic("vsnprintf returned negative");

    va_end(pargs);

    if (grew(s, len))
	len = vsnprintf(s->buffer + s->current, s->buflen - s->current,
			fmt, args);
    va_end(args);
    s->current += len;
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

size_t
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
