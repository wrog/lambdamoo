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

/* The server no longer uses variable-length argument lists of large (size > 8
 * bytes) structures, so this code is no longer needed and BUGGY_STDARG is
 * never defined in config.h.  I've left the code here for possible future use.
 */

#if BUGGY_STDARG

/*
 * This implementation of the stdarg.h stuff is very simplistic; it ignores all
 * promotion and alignment issues.  This is good enough for LambdaMOO's uses on
 * most machines.  Regardless, this implementation should not be used unless
 * the one on your machine won't work.
 */

#ifndef STDARG_H
#define STDARG_H 1

#ifndef _VA_LIST_
#define _VA_LIST_ 1
typedef void *va_list;
#endif

#define va_start(ptr, arg) \
		(ptr = (void *) ((unsigned) &arg + sizeof(arg)))

#define va_arg(ptr, type) \
		((type *) (ptr = (void *) ((unsigned) ptr + sizeof(type))))[-1]

#define va_end(ptr)

#endif				/* STDARG_H */

#else

#include <stdarg.h>

#endif				/* BUGGY_STDARG */

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:18:16  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:55  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:05  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:02:14  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:58:36  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.4  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.3  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.2  1992/09/22  22:47:15  pavel
 * Added missing #include of "config.h".
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
