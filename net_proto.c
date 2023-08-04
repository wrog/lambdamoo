/******************************************************************************
  Copyright (c) 1994, 1995, 1996 Xerox Corporation.  All rights reserved.
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

#  if NETWORK_PROTOCOL == NP_TCP
#    if NETWORK_STYLE == NS_BSD
#      include "net_bsd_tcp.c"
#    endif
#    if NETWORK_STYLE == NS_SYSV
#      include "net_sysv_tcp.c"
#    endif
#  endif

#  if NETWORK_PROTOCOL == NP_LOCAL
#    if NETWORK_STYLE == NS_BSD
#      include "net_bsd_lcl.c"
#    endif
#    if NETWORK_STYLE == NS_SYSV
#      include "net_sysv_lcl.c"
#    endif
#  endif

char rcsid_net_proto[] = "$Id$";

/*
 * $Log$
 * Revision 1.2  1998/12/14 13:18:33  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.1.1.1  1997/03/03 03:45:00  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:58:12  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:28:43  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.1  1995/11/30  04:28:36  pavel
 * Initial revision
 */
