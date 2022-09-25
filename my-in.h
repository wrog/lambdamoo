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

#ifndef My_In_H
#define My_In_H 1

#include "config.h"

#include "my-types.h"
#include <netinet/in.h>

#if HAVE_MACHINE_ENDIAN_H

#  include <machine/endian.h>

#else

#  if !defined(htonl)  &&  NDECL_HTONL
extern unsigned short htons();
extern unsigned32 htonl();
extern unsigned short ntohs();
extern unsigned32 ntohl();
#  endif

#endif

#  if NDECL_IN_ADDR_T
typedef unsigned32 in_addr_t;
#  endif

#  ifndef INADDR_NONE
#    define INADDR_NONE ((in_addr_t)-1)
#  endif

#endif

/*
 * $Log$
 * Revision 1.4  2004/05/22 01:25:43  wrog
 * merging in WROGUE changes (W_SRCIP, W_STARTUP, W_OOB)
 *
 * Revision 1.3.10.1  2003/06/01 12:39:27  wrog
 * ensure definitions for in_addr_t, INADDR_NONE
 *
 * Revision 1.3  1998/12/14 13:18:09  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:53  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:04:58  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:57:23  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.7  1993/08/10  20:56:44  pavel
 * Fixed syntax error in default declaration of htonl() and friends.
 *
 * Revision 1.6  1993/08/04  00:11:59  pavel
 * Removed default big-endian definition for htonl() and friends, replacing it
 * with an external function declaration.
 *
 * Revision 1.5  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.4  1992/10/23  19:28:46  pavel
 * Added protection against multiple inclusions, for systems that aren't
 * clever enough to do that for themselves.
 *
 * Revision 1.3  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.2  1992/10/04  04:56:05  pavel
 * Removed misplaced declaration for inet_addr().
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
