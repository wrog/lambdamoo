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

#ifndef My_Time_H
#define My_Time_H 1

#include "config.h"

#if TIME_H_NEEDS_HELP
/* Some systems' time.h does not include time_t or clock_t */
#include "my-types.h"
#endif

#include <time.h>

#if NDECL_TIME
#include "my-types.h"

extern time_t time(time_t *);
#endif

#include <sys/time.h>

#if defined(MACH) && defined(CMU)
/* These clowns blew the declaration of strftime() in their <time.h> */
#undef HAVE_STRFTIME
#endif

#if HAVE_STRFTIME && NDECL_STRFTIME
#include "my-types.h"

extern size_t strftime(char *s, size_t smax, const char *fmt,
		       const struct tm *tp);
#endif

#if HAVE_TZNAME && NDECL_TZNAME
extern char *tzname;
#endif

#endif				/* !My_Time_H */

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:18:21  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:58  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:05  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:00:41  pavel
 * *** empty log message ***
 *
 * Revision 2.0  1995/11/30  04:59:30  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.5  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.4  1992/10/23  19:35:14  pavel
 * Added check for avoiding strftime() on CMU MACH systems, since they declare
 * it wrong (though they implement it right).
 * Added missing #include "my-types.h" for strftime() declaration.
 *
 * Revision 1.3  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.2  1992/07/30  00:38:08  pavel
 * Add support for compiling on RISC/os 4.52 and NonStop-UX A22.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
