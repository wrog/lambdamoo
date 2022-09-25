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

#ifndef Parse_Cmd_H
#define Parse_Cmd_H 1

#include "config.h"
#include "db.h"
#include "structures.h"

typedef struct {
    const char *verb;		/* verb (as typed by player) */
    const char *argstr;		/* arguments to verb */
    Var args;			/* arguments to the verb */

    const char *dobjstr;	/* direct object string */
    Objid dobj;			/* direct object */

    const char *prepstr;	/* preposition string */
    db_prep_spec prep;		/* preposition identifier */

    const char *iobjstr;	/* indirect object string */
    Objid iobj;			/* indirect object */
} Parsed_Command;

extern char **parse_into_words(char *input, int *nwords);
extern Var parse_into_wordlist(const char *command);
extern Parsed_Command *parse_command(const char *command, Objid user);
extern void free_parsed_command(Parsed_Command *);

#endif				/* !Parse_Cmd_H */

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:18:43  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:19:15  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:15:41  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:54:11  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.6  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.5  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.4  1992/09/14  17:43:03  pjames
 * Moved db_modification code to db modules.
 *
 * Revision 1.3  1992/08/31  22:26:06  pjames
 * Changed some `char *'s to `const char *'
 *
 * Revision 1.2  1992/08/10  17:34:34  pjames
 * Added find_verb() which doesn't fill in a Parse_Info, just returns
 * information about the verb.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
