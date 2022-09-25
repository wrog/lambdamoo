/******************************************************************************
  Copyright (c) 1992 Xerox Corporation.  All rights reserved.
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

#if NETWORK_PROTOCOL == NP_SINGLE
#  include "net_single.c"
#else
#  include "net_multi.c"
#endif

Var
network_connection_options(network_handle nh, Var list)
{
    CONNECTION_OPTION_LIST(NETWORK_CO_TABLE, nh, list);
}

int
network_connection_option(network_handle nh, const char *option, Var * value)
{
    CONNECTION_OPTION_GET(NETWORK_CO_TABLE, nh, option, value);
}

int
network_set_connection_option(network_handle nh, const char *option, Var value)
{
    CONNECTION_OPTION_SET(NETWORK_CO_TABLE, nh, option, value);
}


/*
 * $Log$
 * Revision 1.3  2004/05/22 01:25:44  wrog
 * merging in WROGUE changes (W_SRCIP, W_STARTUP, W_OOB)
 *
 * Revision 1.2.10.1  2003/06/07 12:59:04  wrog
 * introduced connection_option macros
 *
 * Revision 1.2  1998/12/14 13:18:35  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.1.1.1  1997/03/03 03:45:00  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.0  1995/11/30  05:11:37  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.5  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.4  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.3  1992/09/26  02:29:24  pavel
 * Dyked out useless support plans for XNS protocols.
 *
 * Revision 1.2  1992/09/23  17:18:44  pavel
 * Now supports the new networking architecture, switching among protocol and
 * multiplexing-wait implementations based on settings in config.h.
 *
 * Revision 1.1  1992/09/03  21:09:51  pavel
 * Initial RCS-controlled version.
 */
