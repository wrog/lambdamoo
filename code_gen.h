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

#include "ast.h"
#include "program.h"
#include "version.h"

extern Program *generate_code(Stmt *, DB_Version);

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:17:31  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:25  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:02  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.2  1996/02/08  06:28:58  pavel
 * Made generate_code() version-dependent.  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.1  1995/11/30  05:04:34  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 2.0  1995/11/30  05:04:03  pavel
 * Initial RCS-controlled version.
 */
