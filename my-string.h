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

#ifndef My_String_H
#define My_String_H 1

#include "config.h"

#if USE_OWN_STRING_H

#include "my-types.h"

extern void bzero(char *, int);
extern void *memset(void *, int, size_t);
extern char *strcpy(char *, const char *);
extern char *strncpy(char *, const char *, size_t);
extern char *strcat(char *, const char *);
extern int strcmp(const char *, const char *);
extern int strncmp(const char *, const char *, size_t);
extern char *strchr(const char *, int);
extern char *strerror(int);

/* We don't need to declare these because we're only using our own string.h
 * due to GCC complaining about conflicting built-in declarations for them.
 */

#if 0
extern void *memcpy(void *, const void *, size_t);
extern int memcmp(const void *, const void *, size_t);
extern size_t strlen(const char *);
#endif

#else

#  include <string.h>

#  if NEED_MEMORY_H
#    include <memory.h>
#  else
#    if NDECL_MEMCPY
#      include "my-types.h"
extern void *memcpy(void *, const void *, size_t);
extern void *memmove(void *, const void *, size_t);
extern int memcmp(const void *, const void *, size_t);
#    endif
#  endif

#endif

#if HAVE_STRERROR
# if NDECL_STRERROR
extern char *strerror(int);
# endif
#else
extern const char *sys_errlist[];
#  define strerror(error_code)	sys_errlist[error_code]
#endif

#if NDECL_BZERO && !defined(bzero)
#  if BZERO_IN_STDLIB_H
#    include "my-stdlib.h"
#  else
extern void bzero(char *, int);
#  endif
#endif

#if NDECL_MEMSET
#include "my-types.h"

extern void *memset(void *, int, size_t);
#endif

#endif				/* !My_String_H */

/*
 * $Log$
 * Revision 1.4  2004/05/22 01:25:43  wrog
 * merging in WROGUE changes (W_SRCIP, W_STARTUP, W_OOB)
 *
 * Revision 1.3.10.1  2003/06/11 10:36:14  wrog
 * added memmove()
 *
 * Revision 1.3  1998/12/14 13:18:19  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:57  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:05  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:01:23  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:59:02  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.8  1993/08/03  21:32:18  pavel
 * Added support for bzero() being declared in <stdlib.h> on some systems.
 *
 * Revision 1.7  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.6  1992/10/23  19:33:23  pavel
 * Added declarations of bcopy() and memset().
 * Commented out unnecessary USE_OWN_STRING_H declarations of memcmp(),
 * memcpy(), and strlen().
 *
 * Revision 1.5  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.4  1992/10/17  20:40:26  pavel
 * Added some more system-dependent #if's.
 * Added declaration of memcmp() for SunOS.
 *
 * Revision 1.3  1992/08/31  22:26:51  pjames
 * Changed some `char *'s to `const char *'
 *
 * Revision 1.2  1992/07/30  00:34:15  pavel
 * Add support for compiling on RISC/os 4.52 and NonStop-UX A22.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
