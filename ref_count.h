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

#if 0
extern void addref(const void *p);
extern unsigned int delref(const void *p);
#else
#define addref(X) (++((int *)(X))[-1])
#define delref(X) (--((int *)(X))[-1])
#define refcount(X) (((int *)(X))[-1])
#endif

/*
 * $Log$
 * Revision 1.4  1998/12/14 13:18:55  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.3  1997/07/07 03:24:55  nop
 * Merge UNSAFE_OPTS (r5) after extensive testing.
 *
 * Revision 1.2.2.1  1997/03/20 18:59:25  bjj
 * Allocate refcounts with objects that can be addref()'d (strings, lists,
 * floats).  Use macros to manipulate those counts.  This completely replaces
 * the external hash table addref and friends.
 *
 * Revision 1.2  1997/03/03 04:19:21  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:13:28  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  05:07:57  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 */
