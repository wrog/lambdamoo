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

#include "my-ctype.h"
#include "my-stdio.h"
#include "my-stdlib.h"
#include "my-string.h"
#include "my-time.h"

#include "config.h"
#include "db.h"
#include "list.h"
#include "match.h"
#include "parse_cmd.h"
#include "storage.h"
#include "structures.h"
#include "utils.h"

char **
parse_into_words(char *input, int *nwords)
{
    static char **words = 0;
    static int max_words = 0;
    int in_quotes = 0;
    char *ptr = input;

    if (!words) {
	max_words = 50;
	words = mymalloc(max_words * sizeof(char *), M_STRING_PTRS);
    }
    while (*input == ' ')
	input++;

    for (*nwords = 0; *input != '\0'; (*nwords)++) {
	if (*nwords == max_words) {
	    int new_max = max_words * 2;
	    char **new = mymalloc(new_max * sizeof(char *), M_STRING_PTRS);
	    int i;

	    for (i = 0; i < max_words; i++)
		new[i] = words[i];

	    myfree(words, M_STRING_PTRS);
	    words = new;
	    max_words = new_max;
	}
	words[*nwords] = ptr;
	while (*input != '\0' && (in_quotes || *input != ' ')) {
	    char c = *(input++);

	    if (c == '"')
		in_quotes = !in_quotes;
	    else if (c == '\\') {
		if (*input != '\0')
		    *(ptr++) = *(input++);
	    } else
		*(ptr++) = c;
	}
	while (*input == ' ')
	    input++;
	*(ptr++) = '\0';
    }

    return words;
}

static char *
build_string(int argc, char *argv[])
{
    int i, len = 0;
    char *str;

    if (!argc)
	return str_dup("");

    len = strlen(argv[0]);
    for (i = 1; i < argc; i++)
	len += 1 + strlen(argv[i]);

    str = (char *) mymalloc(len + 1, M_STRING);

    strcpy(str, argv[0]);
    for (i = 1; i < argc; i++) {
	strcat(str, " ");
	strcat(str, argv[i]);
    }

    return str;
}

#define MAXWORDS		500	/* maximum number of words in a line */
					/* This limit should be removed...   */

Var
parse_into_wordlist(const char *command)
{
    int argc, i;
    char **argv;
    Var args;
    char *s = str_dup(command);

    argv = parse_into_words(s, &argc);
    args = new_list(argc);
    for (i = 1; i <= argc; i++) {
	args.v.list[i].type = TYPE_STR;
	args.v.list[i].v.str = str_dup(argv[i - 1]);
    }
    free_str(s);
    return args;
}

Parsed_Command *
parse_command(const char *command, Objid user)
{
    static Parsed_Command pc;
    const char *argstr;
    char *buf;
    const char *verb;
    int argc;
    char **argv;
    int pstart, pend, dlen;
    int i;

    while (*command == ' ')
	command++;
    switch (*command) {
    case '"':
	verb = "say";
	goto finish_specials;
    case ':':
	verb = "emote";
	goto finish_specials;
    case ';':
	verb = "eval";
	goto finish_specials;

      finish_specials:
	argstr = command + 1;
	buf = (char *) mymalloc(strlen(argstr) + strlen(verb) + 2,
				M_STRING);
	strcpy(buf, verb);
	strcat(buf, " ");
	strcat(buf, argstr);
	break;

    default:
	buf = str_dup(command);
	{			/* Skip past even complexly-quoted verbs */
	    int in_quotes = 0;

	    argstr = command;
	    while (*argstr && (in_quotes || *argstr != ' ')) {
		char c = *(argstr++);

		if (c == '"')
		    in_quotes = !in_quotes;
		else if (c == '\\' && *argstr)
		    argstr++;
	    }
	}
	while (*argstr == ' ')
	    argstr++;
	break;
    }
    argv = parse_into_words(buf, &argc);

    if (argc == 0) {
	free_str(buf);
	return 0;
    }
    pc.verb = str_dup(argv[0]);
    pc.argstr = str_dup(argstr);

    pc.args = new_list(argc - 1);
    for (i = 1; i < argc; i++) {
	pc.args.v.list[i].type = TYPE_STR;
	pc.args.v.list[i].v.str = str_dup(argv[i]);
    }

    /*
     * look for a preposition
     */
    if (argc > 1) {
	pc.prep = db_find_prep(argc - 1, argv + 1, &pstart, &pend);
	if (pc.prep == PREP_NONE) {
	    pstart = argc;
	    pend = argc;
	} else {
	    pstart++;
	    pend++;
	}
    } else {
	pc.prep = PREP_NONE;
	pstart = argc;
	pend = argc;
    }

    /*
     * if there's a preposition,
     * find the iobj & dobj around it, if any
     */
    if (pc.prep != PREP_NONE) {
	pc.prepstr = build_string(pend - pstart + 1, argv + pstart);
	pc.iobjstr = build_string(argc - (pend + 1), argv + (pend + 1));
	pc.iobj = match_object(user, pc.iobjstr);
    } else {
	pc.prepstr = str_dup("");
	pc.iobjstr = str_dup("");
	pc.iobj = NOTHING;
    }

    dlen = pstart - 1;
    if (dlen == 0) {
	pc.dobjstr = str_dup("");
	pc.dobj = NOTHING;
    } else {
	pc.dobjstr = build_string(dlen, argv + 1);
	pc.dobj = match_object(user, pc.dobjstr);
    }

    free_str(buf);

    return &pc;
}

void
free_parsed_command(Parsed_Command * pc)
{
    free_str(pc->verb);
    free_str(pc->argstr);
    free_var(pc->args);
    free_str(pc->dobjstr);
    free_str(pc->prepstr);
    free_str(pc->iobjstr);
}


char rcsid_parse_cmd[] = "$Id$";

/*
 * $Log$
 * Revision 1.4  1998/12/14 13:18:42  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.3  1997/07/07 03:24:54  nop
 * Merge UNSAFE_OPTS (r5) after extensive testing.
 *
 * Revision 1.2.2.2  1997/05/30 18:36:17  nop
 * Oops, make sure to free words as M_STRING_PTRS, not M_STRING.  I crashed
 * LambdaMOO for the first time with this!
 *
 * Revision 1.2.2.1  1997/05/20 03:01:34  nop
 * parse_into_words was allocating pointers to strings as strings.  Predictably,
 * the refcount prepend code was not prepared for this, causing unaligned memory
 * access on the Alpha.  Added new M_STRING_PTRS allocation class that could
 * be renamed to something better, perhaps.
 *
 * Revision 1.2  1997/03/03 04:19:14  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:01  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.2  1996/02/08  06:55:06  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.1  1995/12/28  00:40:37  pavel
 * Fixed memory leak in freeing Parsed_Command structures.
 * Release 1.8.0alpha3.
 *
 * Revision 2.0  1995/11/30  04:29:31  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.10  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.9  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.8  1992/10/17  20:49:22  pavel
 * Global rename of strdup->str_dup, strref->str_ref, vardup->var_dup, and
 * varref->var_ref.
 *
 * Revision 1.7  1992/09/14  17:30:26  pjames
 * Moved db_modification code to db modules.
 *
 * Revision 1.6  1992/08/31  22:32:16  pjames
 * Changed some `char *'s to `const char *' and fixed code accordingly.
 *
 * Revision 1.5  1992/08/28  16:15:47  pjames
 * Changed myfree(*, M_STRING) to free_str(*).
 * Changed some strref's to strdup.
 *
 * Revision 1.4  1992/08/20  23:49:12  pavel
 * Moved #define MAXARGS from config.h to here, renamed to MAXWORDS.  Also
 * changed the file name from `parse_command.c' to `parse_cmd.c', to make it
 * easier to use on 14-character-file-name systems.
 *
 * Revision 1.3  1992/08/10  17:35:34  pjames
 * Added find_verb() which doesn't fill in a Parse_Info, just returns
 * information about the verb.
 *
 * Revision 1.2  1992/07/21  00:04:53  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS ident. string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
