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

#include <fcntl.h>

#if NDECL_FCNTL
extern int fcntl(int, int,...);
extern int open(const char *, int,...);
#endif

#if POSIX_NONBLOCKING_WORKS
/* Prefer POSIX-style nonblocking, if available. */
#define NONBLOCK_FLAG O_NONBLOCK
#else
#define NONBLOCK_FLAG O_NDELAY
#endif

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:18:08  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:52  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:05:07  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:57:10  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.5  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.4  1992/10/23  19:27:57  pavel
 * Changed to conditionalize on POSIX_NONBLOCKING_WORKS instead of
 * defined(O_NONBLOCK).
 *
 * Revision 1.3  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.2  1992/10/17  20:34:42  pavel
 * Added some more system-dependent #if's.
 * Added definition of NONBLOCK_FLAG, to allow use of POSIX-style non-blocking
 * on systems where it is available.
 *
 * Revision 1.1  1992/10/06  01:38:46  pavel
 * Initial RCS-controlled version.
 */
