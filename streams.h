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

#ifndef Streams_H
#define Streams_H 1

#include "config.h"

#include "my-string.h"
#include "exceptions.h"

typedef struct {
    char *buffer;
    size_t buflen;
    size_t current;
} Stream;

extern Stream *new_stream(size_t size);
extern void free_stream(Stream *);

extern char *reset_stream(Stream *);
extern char *stream_contents(Stream *);
extern size_t stream_length(Stream *);

extern void stream_delete_char(Stream *);

/*
 * If stream exceptions are enabled,
 * all of the following may RAISE(stream_too_big)
 */
extern void stream_add_char(Stream *, char);
extern void stream_add_bytes(Stream *, const char *, size_t);
inline void stream_add_string(Stream * s, const char *string)
{ stream_add_bytes(s, string, strlen(string)); }
extern void stream_printf(Stream *, const char *,...) FORMAT(printf,2,3);

/*
 * How to provide fixed-size buffers for others to fill:
 *
 *   setup:
 *      Stream *s;
 *      char *buffer;
 *      size_t blength;
 *      stream_beginfill(s, NEED, &buffer, &blength);
 *	  // may need to allocate and RAISE(stream_too_big);
 *	  // guarantees blength >= NEED,
 *	  // leaves space for \0 afterwards
 *
 *   paradigm 1:
 *      if (0 >= (count = read(fd, buffer, blength)))
 *	  ;// error|noop: nothing written; nothing to do
 *      else
 *	  stream_endfill(s, blength - count);
 *
 *   paradigm 2:
 *      iconv(cd, &in, &ilen, &buffer, &blength);
 *	  // always does (buffer += K, blength -= K)
 *      stream_endfill(s, blength);
 */
extern void stream_beginfill(Stream *, size_t, char **, size_t *);
extern void stream_endfill(Stream *, size_t);

/*
 * Helper for catching overly large allocations:
 *
 *   TRY_STREAM
 *     ...stuff that may allocate too much
 *   EXCEPT_STREAM
 *     ...recover from too much allocation
 *   ENDTRY_STREAM
 */
#define TRY_STREAM				\
{						\
    enable_stream_exceptions();			\
    if (ES_exceptionStack)			\
	panic("TRY_STREAM not outermost");	\
    TRY

#define EXCEPT_STREAM				\
    EXCEPT(stream_too_big)

#define ENDTRY_STREAM				\
    ENDTRY					\
    disable_stream_exceptions();		\
}
/*
 * (1) TRY_STREAM, if used, *must* be the outermost TRY.
 *     Otherwise, you would have to do
 *
 *       TRY
 *          enable_stream_exceptions();
 *          TRY
 *             ...something that may fail in additional ways
 *          EXCEPT(stream_too_big)
 *             ...recover from stream_too_big
 *          ENDTRY
 *       FINALLY
 *          disable_stream_exceptions();
 *	    ... other mandatory cleanups??
 *       ENDTRY;
 *
 *    ((XXX -- provide better TRY_STREAM if this ever comes up?..))
 *
 * see also notes in exceptions.h
 */

/* low level machinery for catching large allocations:
 */
extern void enable_stream_exceptions(void);
extern void disable_stream_exceptions(void);
extern size_t stream_alloc_maximum;
extern Exception stream_too_big;
/*
 * Calls to enable_stream_exceptions() and disable_stream_exceptions()
 * must be paired and nest properly.
 *
 * If enable_stream_exceptions() is in effect, then, upon any
 * attempt to grow a stream beyond stream_alloc_maximum bytes,
 * a stream_too_big exception will be raised.
 */

#endif		/* !Streams_H */

/*
 * $Log$
 * Revision 1.7  2010/04/23 05:10:55  wrog
 * remove max=0 meaning no limit
 *
 * Revision 1.4  2006/12/06 23:57:51  wrog
 * New INPUT_APPLY_BACKSPACE option to process backspace/delete characters on nonbinary connections (patch 1571939)
 *
 * Revision 1.3  1998/12/14 13:19:02  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:19:28  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:12:33  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:55:31  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.3  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.2  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
