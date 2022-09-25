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

#if HAVE_LRAND48
extern long lrand48(void);
extern void srand48(long);
#    define RANDOM	lrand48
#    define SRANDOM	srand48
#else
#  include "my-stdlib.h"
#  if HAVE_RANDOM
#    define RANDOM	random
#    define SRANDOM 	srandom
#  else
#    define RANDOM	rand
#    define SRANDOM	srand
#  endif
#endif

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:18:53  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:19:20  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:13:41  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:54:47  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.7  1993/08/04  00:04:28  pavel
 * Moved declaration of random() and srandom() to my-stdlib.h.
 *
 * Revision 1.6  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.5  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.4  1992/10/17  20:50:13  pavel
 * Generalized support for rand() replacements on systems that have better
 * generators.
 *
 * Revision 1.3  1992/09/11  21:16:41  pavel
 * Fixed to include config.h, to test USE_RANDOM.
 *
 * Revision 1.2  1992/08/14  00:38:20  pavel
 * Changed declaration of srandom() to be compatible with IRIX 4.0; I sure hope
 * this doesn't break any other ports...
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
