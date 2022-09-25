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

#ifdef BSD
#ifndef BSD4_1
#define HAVE_GETPAGESIZE
#endif
#endif

#ifndef HAVE_GETPAGESIZE

#include <sys/param.h>

#ifdef EXEC_PAGESIZE
#define getpagesize() EXEC_PAGESIZE
#else
#ifdef NBPG
#define getpagesize() NBPG * CLSIZE
#ifndef CLSIZE
#define CLSIZE 1
#endif				/* no CLSIZE */
#else				/* no NBPG */
#define getpagesize() NBPC
#endif				/* no NBPG */
#endif				/* no EXEC_PAGESIZE */

#endif				/* not HAVE_GETPAGESIZE */


/*
 * $Log$
 * Revision 1.2  1997/03/03 04:18:43  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:03  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:24:58  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:51:46  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.2  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
