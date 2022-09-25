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

#include "config.h"
#include "options.h"

#include <sys/time.h>

#if NEED_SELECT_H
#  include "my-types.h"
#  include <sys/select.h>
#endif

#if NDECL_GETITIMER
extern int getitimer(int, struct itimerval *);
#endif

#if NDECL_SETITIMER
extern int setitimer(int, struct itimerval *, struct itimerval *);
#endif

#if NDECL_SELECT  &&  MPLEX_STYLE == MP_SELECT
#include "my-types.h"

extern int select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
#endif

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:18:20  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:57  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:05  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:01:00  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:59:21  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.5  1993/08/12  01:23:11  pavel
 * -- Split out optional declaration of setitimer() to be independent of the
 *    one for getitimer().
 * -- Only use declaration of select() if MP_SELECT is enabled.
 * -- Include <sys/select.h> if it exists.
 *
 * Revision 1.4  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.3  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.2  1992/10/17  21:01:10  pavel
 * Dyked out useless declaration of gettimeofday().
 * Added some more system-dependent #if's.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
