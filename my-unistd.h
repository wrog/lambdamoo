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

#if HAVE_UNISTD_H  &&  !NDECL_FORK

#include <unistd.h>

#else

#include "my-types.h"

extern unsigned alarm(unsigned);
extern int chmod(const char *, mode_t);
extern int close(int);
extern int dup(int);
extern void _exit(int);
extern pid_t fork(void);
extern pid_t getpid(void);
extern int link(const char *, const char *);
extern int pause(void);
extern int pipe(int *fds);
extern int read(int, void *, unsigned);
extern unsigned sleep(unsigned);
extern int unlink(const char *);
extern int write(int, const void *, unsigned);
extern int fsync(int);

#endif

/*
 * $Log$
 * Revision 1.4  2007/11/12 11:17:03  wrog
 * sync so that checkpoint is physically written before prior checkpoint is unlinked
 *
 * Revision 1.3  1998/12/14 13:18:23  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:59  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:05  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:00:01  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:59:57  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.11  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.10  1992/10/23  19:37:01  pavel
 * Added support for systems that have <unistd.h>, but still don't declare
 * most of the functions in it...
 *
 * Revision 1.9  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.8  1992/10/17  20:42:25  pavel
 * Added declarations of link() and unlink().
 *
 * Revision 1.7  1992/09/27  19:30:09  pavel
 * Added declarations of chmod() and pipe() for non-POSIX systems.
 *
 * Revision 1.6  1992/09/21  17:08:59  pavel
 * Added missing #include of config.h.
 *
 * Revision 1.5  1992/09/03  23:53:33  pavel
 * Added declaration for sleep().
 *
 * Revision 1.4  1992/08/31  22:26:21  pjames
 * Changed some `char *'s to `const char *'
 *
 * Revision 1.3  1992/08/21  00:46:08  pavel
 * Changed to conditionalize on the option AVOID_POSIX rather than USE_POSIX.
 *
 * Revision 1.2  1992/08/13  23:19:46  pavel
 * Added declaration for dup()...
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
