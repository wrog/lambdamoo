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
#include "structures.h"
#include "version.h"

struct keyword {
    const char *name;		/* the canonical spelling of the keyword */
    DB_Version version;		/* the DB version when it was introduced */
    int token;			/* the type of token the scanner should use */
    enum error error;		/* for token == ERROR, the value */
};

typedef const struct keyword Keyword;

extern Keyword *find_keyword(const char *);

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:17:56  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:46  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:03  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.2  1996/02/08  06:24:47  pavel
 * Fixed type of version number.  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.1  1995/12/11  08:15:02  pavel
 * Removed useless #include "tokens.h".  Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  05:06:50  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.1  1995/11/30  05:06:40  pavel
 * Initial revision
 */
