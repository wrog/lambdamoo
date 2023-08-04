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

#ifndef Unparse_h
#define Unparse_h 1

#include "my-stdio.h"

#include "config.h"
#include "program.h"
#include "structures.h"

typedef void (*Unparser_Receiver) (void *, const char *);

extern void unparse_program(Program *, Unparser_Receiver, void *,
			    int fully_parenthesize,
			    int indent_lines, int f_index);

extern void unparse_to_file(FILE * fp, Program *,
			    int fully_parenthesize,
			    int indent_lines, int f_index);
extern void unparse_to_stderr(Program *, int fully_parenthesize,
			      int indent_lines, int f_index);

extern const char *error_name(enum error);	/* E_NONE -> "E_NONE" */
extern const char *unparse_error(enum error);	/* E_NONE -> "No error" */

#endif

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:19:13  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:19:35  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:08:46  pavel
 * Added unparse_to_file() and unparse_to_stderr().  Updated copyright notice
 * for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:56:22  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.7  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.6  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.5  1992/08/31  22:23:46  pjames
 * Changed some `char *'s to `const char *'
 *
 * Revision 1.4  1992/08/21  00:40:15  pavel
 * Renamed include file "parse_command.h" to "parse_cmd.h".
 *
 * Revision 1.3  1992/08/13  23:59:27  pavel
 * Converted to a typedef of `var_type' = `enum var_type'.
 *
 * Revision 1.2  1992/08/10  16:41:25  pjames
 * Changed ip to pc.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
