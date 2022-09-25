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

#include <tiuser.h>

#if NDECL_T_OPEN
extern int t_accept(int, int, struct t_call *);
extern void *t_alloc(int, int, int);
extern int t_bind(int, struct t_bind *, struct t_bind *);
extern int t_close(int);
extern int t_connect(int, struct t_call *, struct t_call *);
extern int t_listen(int, struct t_call *);
extern int t_open(const char *, int, struct t_info *);

extern int t_errno;
#endif

#if NDECL_T_ERRLIST
extern char *t_errlist[];
#endif

/*
 * $Log$
 * Revision 1.2  1997/03/03 04:18:58  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:05  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:00:32  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:59:38  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.5  1993/10/11  19:09:23  pavel
 * Separated out the check for an undeclared t_errlist, since Solaris, for one,
 * declares everying *but* that...
 *
 * Revision 1.4  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.3  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.2  1992/10/17  20:41:40  pavel
 * Added some more system-dependent #if's.
 *
 * Revision 1.1  1992/10/06  01:40:02  pavel
 * Initial RCS-controlled version.
 */
