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

#ifndef Parser_h
#define Parser_h 1

#include "config.h"
#include "program.h"
#include "version.h"

typedef struct {
    void (*error) (void *, const char *);
    void (*warning) (void *, const char *);
    int (*getch) (void *);
} Parser_Client;

extern Program *parse_program(DB_Version, Parser_Client, void *);
extern Program *parse_list_as_program(Var code, Var * errors);

#endif

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:18:44  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:19:16  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.2  1996/02/08  06:15:30  pavel
 * Removed ungetch() method on Parser_Client, added version number to
 * parse_program.  Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.1  1995/12/28  00:47:29  pavel
 * Added support for MOO-compilation warnings.  Release 1.8.0alpha3.
 *
 * Revision 2.0  1995/11/30  04:54:19  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.5  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.4  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.3  1992/10/06  18:26:41  pavel
 * Changed name of global Parser_Client to avoid a name clash.
 *
 * Revision 1.2  1992/09/14  17:42:46  pjames
 * Moved db_modification code to db modules.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
