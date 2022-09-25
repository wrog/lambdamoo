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

#include <poll.h>

#if NDECL_POLL
extern int poll(struct pollfd *, unsigned long, int);
#endif

/*
 * $Log$
 * Revision 1.1  1997/03/03 03:45:04  nop
 * Initial revision
 *
 * Revision 2.1  1996/02/08  06:03:22  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:57:46  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.4  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.3  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.2  1992/10/06  18:27:51  pavel
 * Added `#ifdef sun' around the declaration of poll(), since only SunOS seems
 * to need it.
 *
 * Revision 1.1  1992/10/03  00:52:26  pavel
 * Initial RCS-controlled version.
 */
