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

#ifndef Version_H
#define Version_H 1

#include "config.h"

extern const char *server_version;

/* The following list must never be reordered, only appended to.  There is one
 * element per version of the database format (including incompatible changes
 * to the language, such as the addition of new keywords).  The integer value
 * of each element is used in the DB header on disk to identify the format
 * version in use in that file.
 */
typedef enum {
    DBV_Prehistory,		/* Before format versions */
    DBV_Exceptions,		/* Addition of the `try', `except', `finally',
				 * and `endtry' keywords.
				 */
    DBV_BreakCont,		/* Addition of the `break' and `continue'
				 * keywords.
				 */
    DBV_Float,			/* Addition of `FLOAT' and `INT' variables and
				 * the `E_FLOAT' keyword, along with version
				 * numbers on each frame of a suspended task.
				 */
    DBV_BFBugFixed,		/* Bug in built-in function overrides fixed by
				 * making it use tail-calling.  This DB_Version
				 * change exists solely to turn off special
				 * bug handling in read_bi_func_data().
				 */
    Num_DB_Versions		/* Special: the current version is this - 1. */
} DB_Version;

#define current_version	((DB_Version) (Num_DB_Versions - 1))

extern int check_version(DB_Version);
				/* Returns true iff given version is within the
				 * known range.
				 */

#endif				/* !Version_H */

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:19:19  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:19:39  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.3  1996/04/19  01:25:21  pavel
 * Added somewhat bogus DBV_BFBugFixed version.  Release 1.8.0p4.
 *
 * Revision 2.2  1996/02/08  06:07:10  pavel
 * Added DBV_BreakCont and DBV_Float versions, check_version().  Moved
 * db_in_version to db_io.h and renamed db_out_version to current_version.
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.1  1995/12/28  00:48:42  pavel
 * Fixed list of new keywords in comment to remove transient `raise' keyword.
 * Release 1.8.0alpha3.
 *
 * Revision 2.0  1995/11/30  04:56:42  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.3  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.2  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
