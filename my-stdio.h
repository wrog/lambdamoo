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

#ifndef My_Stdio_H
#define My_Stdio_H 1

#include "config.h"

#include <stdio.h>

#if NDECL_FCLOSE
#include "my-types.h"

extern int fclose(FILE *);
extern int fflush(FILE *);
extern int fileno(FILE *);
extern size_t fwrite(const void *, size_t, size_t, FILE *);
extern int fgetc(FILE *);
extern int fprintf(FILE *, const char *,...);
extern int fscanf(FILE *, const char *,...);
extern int sscanf(const char *, const char *,...);
extern int printf(const char *,...);
extern int ungetc(int, FILE *);
#endif

#if NDECL_PERROR
extern void perror(const char *);
#endif

#if NDECL_REMOVE
extern int remove(const char *);
extern int rename(const char *, const char *);
#endif

#if NDECL_VFPRINTF
#include "my-stdarg.h"

extern int vfprintf(FILE *, const char *, va_list);
extern int vfscanf(FILE *, const char *, va_list);
#endif

#if !HAVE_REMOVE
#  include "my-unistd.h"
#  define remove(x)	unlink(x)
#endif

#if !HAVE_RENAME
#  include "my-unistd.h"
#  define rename(old, new)	(link(old, new) && unlink(old))
#endif

#endif				/* !My_Stdio_H */

/*
 * $Log$
 * Revision 1.4  2007/11/12 11:17:03  wrog
 * sync so that checkpoint is physically written before prior checkpoint is unlinked
 *
 * Revision 1.3  1998/12/14 13:18:17  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:56  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:05  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.2  1996/02/08  06:02:04  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.1  1996/01/11  07:44:48  pavel
 * Added declaration for fwrite().  Release 1.8.0alpha5.
 *
 * Revision 2.0  1995/11/30  04:58:44  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.7  1992/10/28  01:58:05  pavel
 * Changed NDECL_VPRINTF to NDECL_VFPRINTF, which is the one we care about...
 *
 * Revision 1.6  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.5  1992/10/23  19:30:14  pavel
 * Split up the declarations into four equivalence classes instead of just one.
 *
 * Revision 1.4  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.3  1992/10/17  20:37:59  pavel
 * Added support for systems that lack rename() and/or remove().
 *
 * Revision 1.2  1992/09/03  23:53:10  pavel
 * Added declaration for printf().
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
