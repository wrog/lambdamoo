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

#if SYS_SOCKET_H_NEEDS_HELP
/* Some systems aren't sufficiently careful about headers including the other
 * headers that they depend upon...
 */
#include "my-types.h"
#endif

#include <sys/socket.h>

#if NDECL_ACCEPT
extern int accept(int, struct sockaddr *, int *);
extern int listen(int, int);
extern int setsockopt(int, int, int, const char *, int);
extern int socket(int, int, int);
#endif

#if NDECL_BIND
extern int bind(int, struct sockaddr *, int);
extern int getsockname(int, struct sockaddr *, int *);
extern int connect(int, struct sockaddr *, int);
#endif

#if NDECL_SHUTDOWN
extern int shutdown(int, int);
#endif

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:18:14  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:54  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:05  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.2  1996/02/08  06:02:39  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.1  1995/12/31  03:24:41  pavel
 * Added getsockname().  Release 1.8.0alpha4.
 *
 * Revision 2.0  1995/11/30  04:58:09  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.8  1993/08/03  21:41:53  pavel
 * Broke out bind() and connect() for a separate NDECL test.
 *
 * Revision 1.7  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.6  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.5  1992/10/17  20:36:37  pavel
 * Added some more system-dependent #if's.
 *
 * Revision 1.4  1992/10/06  01:34:00  pavel
 * Added SGI and AIX to the list of losers who don't #include the right things
 * from inside their own header files.
 *
 * Revision 1.3  1992/09/23  22:35:52  pavel
 * Added conditional #include of "my-types.h", for systems that need it.
 *
 * Revision 1.2  1992/08/31  22:26:43  pjames
 * Changed some `char *'s to `const char *'
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
