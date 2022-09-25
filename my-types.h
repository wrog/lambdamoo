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

#ifndef My_Types_h
#define My_Types_h 1

#include "config.h"

/* The Linux manpage for select indicates that sys/time.h is necessary
   for the FD_ZERO et al declarations.  If this causes problems on
   other systems, we'll have to put this into autoconf.     --Jay */

#include <sys/time.h>

#include <sys/types.h>

#if NEED_BSDTYPES_H
#include <sys/bsdtypes.h>
#endif

#ifndef FD_ZERO
#define	NFDBITS		(sizeof(fd_set)*8)
#define	FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))
#define	FD_SET(n, p)	((p)->fds_bits[0] |= (1L<<((n)%NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[0] &  (1L<<((n)%NFDBITS)))
#endif				/* FD_ZERO */

#endif				/* !My_Types_h */

/*
 * $Log$
 * Revision 1.4  1998/12/14 13:18:22  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.3  1997/03/03 06:39:28  nop
 * sys/time.h necessary for FD_ZERO et al...maybe.
 *
 * Revision 1.2  1997/03/03 04:18:58  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:05  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:00:13  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:59:47  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.11  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.10  1992/10/23  19:35:43  pavel
 * Moved bzero declaration to my-string.h.
 *
 * Revision 1.9  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.8  1992/10/17  20:41:53  pavel
 * Added some more system-dependent #if's.
 *
 * Revision 1.7  1992/09/27  19:27:19  pavel
 * Added declaration of mode_t for non-POSIX systems.
 *
 * Revision 1.6  1992/09/23  17:15:03  pavel
 * Moved declaration of bzero (for SunOS) to here from net_multi.c.
 *
 * Revision 1.5  1992/09/21  17:43:16  pavel
 * Added missing #include of "config.h".
 *
 * Revision 1.4  1992/08/31  23:42:15  pavel
 * Changed to conditionalize on FD_ZERO rather than AUX for the fd_set macros.
 *
 * Revision 1.3  1992/08/21  00:45:50  pavel
 * Changed to conditionalize on the option AVOID_POSIX rather than USE_POSIX.
 *
 * Revision 1.2  1992/07/30  00:39:05  pavel
 * Add support for compiling on RISC/os 4.52 and NonStop-UX A22.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
