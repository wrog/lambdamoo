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

#ifndef Math_h
#define Math_h 1

#include <float.h>
#include <math.h>

#define IS_REAL(x)	(-DBL_MAX <= (x) && (x) <= DBL_MAX)

#endif

/*
 * $Log$
 * Revision 1.2  1998/12/14 13:18:12  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.2  1996/03/10  01:06:01  pavel
 * Replaced HUGE_VAL with DBL_MAX, since some systems (e.g., BSDi 1.1)
 * mis-define the former as positive infinity.  Release 1.8.0.
 *
 * Revision 2.1  1996/02/08  06:04:22  pavel
 * Updated copyright notice for 1996.  Added inclusion of <math.h> for all of
 * the standard math functions.  Added IS_REAL() check for NaNs and
 * infinities.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  05:15:40  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 */
