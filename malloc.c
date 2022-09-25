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

#include "options.h"

#ifdef USE_GNU_MALLOC

#include "exceptions.h"

#define MSTATS
#define rcheck
#define botch panic

#include "gnu-malloc.c"

unsigned
malloc_real_size(void *ptr)
{
    char *cp = (char *) ptr;
    struct mhead *p = (struct mhead *) (cp - ((sizeof *p + 7) & ~7));

    return 1 << (p->mh_index + 3);
}

unsigned
malloc_size(void *ptr)
{
    char *cp = (char *) ptr;
    struct mhead *p = (struct mhead *) (cp - ((sizeof *p + 7) & ~7));

#ifdef rcheck
    return p->mh_nbytes;
#else
    return p->mh_index >= 13 ? (1 << (p->mh_index + 3)) : p->mh_size;
#endif
}

#else				/* !defined(USE_GNU_MALLOC) */

int malloc_dummy;		/* Prevent `empty compilation unit' warning */

#endif

char rcsid_malloc[] = "$Id$";

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:18:01  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:49  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:00  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:59:50  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:27:16  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.7  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.6  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.5  1992/09/22  22:46:35  pavel
 * Added missing #include of "config.h".
 *
 * Revision 1.4  1992/08/31  22:32:52  pjames
 * Changed static variable dummy to extern in malloc_dummy to remove warning.
 *
 * Revision 1.3  1992/08/21  00:59:58  pavel
 * Changed to conditionalize on USE_GNU_MALLOC rather than USE_SYSTEM_MALLOC.
 *
 * Revision 1.2  1992/07/21  00:04:29  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS ident. string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
