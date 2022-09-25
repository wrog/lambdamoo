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

#include "my-stdio.h"

#include "ast.h"
#include "config.h"
#include "exceptions.h"
#include "log.h"
#include "storage.h"
#include "structures.h"
#include "sym_table.h"
#include "utils.h"
#include "version.h"

static Names *
new_names(unsigned max_size)
{
    Names *names = mymalloc(sizeof(Names), M_NAMES);

    names->names = mymalloc(sizeof(char *) * max_size, M_NAMES);
    names->max_size = max_size;
    names->size = 0;

    return names;
}

static Names *
copy_names(Names * old)
{
    Names *new = new_names(old->size);
    unsigned i;

    new->size = old->size;
    for (i = 0; i < new->size; i++)
	new->names[i] = str_ref(old->names[i]);

    return new;
}

int
first_user_slot(DB_Version version)
{
    int count = 16;		/* DBV_Prehistory count */

    if (version >= DBV_Float)
	count += 2;

    return count;
}

Names *
new_builtin_names(DB_Version version)
{
    static Names *builtins[Num_DB_Versions];

    if (builtins[version] == 0) {
	Names *bi = new_names(first_user_slot(version));

	builtins[version] = bi;
	bi->size = bi->max_size;

	bi->names[SLOT_NUM] = str_dup("NUM");
	bi->names[SLOT_OBJ] = str_dup("OBJ");
	bi->names[SLOT_STR] = str_dup("STR");
	bi->names[SLOT_LIST] = str_dup("LIST");
	bi->names[SLOT_ERR] = str_dup("ERR");
	bi->names[SLOT_PLAYER] = str_dup("player");
	bi->names[SLOT_THIS] = str_dup("this");
	bi->names[SLOT_CALLER] = str_dup("caller");
	bi->names[SLOT_VERB] = str_dup("verb");
	bi->names[SLOT_ARGS] = str_dup("args");
	bi->names[SLOT_ARGSTR] = str_dup("argstr");
	bi->names[SLOT_DOBJ] = str_dup("dobj");
	bi->names[SLOT_DOBJSTR] = str_dup("dobjstr");
	bi->names[SLOT_PREPSTR] = str_dup("prepstr");
	bi->names[SLOT_IOBJ] = str_dup("iobj");
	bi->names[SLOT_IOBJSTR] = str_dup("iobjstr");

	if (version >= DBV_Float) {
	    bi->names[SLOT_INT] = str_dup("INT");
	    bi->names[SLOT_FLOAT] = str_dup("FLOAT");
	}
    }
    return copy_names(builtins[version]);
}

int
find_name(Names * names, const char *str)
{
    unsigned i;

    for (i = 0; i < names->size; i++)
	if (!mystrcasecmp(names->names[i], str))
	    return i;
    return -1;
}

unsigned
find_or_add_name(Names ** names, const char *str)
{
    unsigned i;

    for (i = 0; i < (*names)->size; i++)
	if (!mystrcasecmp((*names)->names[i], str)) {	/* old name */
	    return i;
	}
    if ((*names)->size == (*names)->max_size) {
	unsigned old_max = (*names)->max_size;
	Names *new = new_names(old_max * 2);
	unsigned i;

	for (i = 0; i < old_max; i++)
	    new->names[i] = (*names)->names[i];
	new->size = old_max;
	myfree((*names)->names, M_NAMES);
	myfree(*names, M_NAMES);
	*names = new;
    }
    (*names)->names[(*names)->size] = str_dup(str);
    return (*names)->size++;
}

void
free_names(Names * names)
{
    unsigned i;

    for (i = 0; i < names->size; i++)
	free_str(names->names[i]);
    myfree(names->names, M_NAMES);
    myfree(names, M_NAMES);
}

char rcsid_sym_table[] = "$Id$";

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:19:05  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:19:29  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:01  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.2  1996/03/10  01:16:32  pavel
 * Removed a bunch of obsolete unused functions.  Release 1.8.0.
 *
 * Revision 2.1  1996/02/08  06:49:24  pavel
 * Made new_builtin_names() and first_user_slot() version-dependent.  Renamed
 * TYPE_NUM to TYPE_INT.  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:31:53  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.10  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.9  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.8  1992/10/17  20:54:04  pavel
 * Global rename of strdup->str_dup, strref->str_ref, vardup->var_dup, and
 * varref->var_ref.
 *
 * Revision 1.7  1992/09/08  21:58:33  pjames
 * Updated #includes.
 *
 * Revision 1.6  1992/08/31  22:24:18  pjames
 * Changed some `char *'s to `const char *'
 *
 * Revision 1.5  1992/08/28  16:07:24  pjames
 * Changed vardup to varref.
 * Changed some strref's to strdup.
 * Changed myfree(*, M_STRING) to free_str(*).
 *
 * Revision 1.4  1992/08/10  16:50:28  pjames
 * Updated #includes.
 *
 * Revision 1.3  1992/07/27  18:23:48  pjames
 * Changed M_CT_ENV to M_NAMES.  Freed some memory that wasn't being
 * freed when growing or freeing a list of names.
 *
 * Revision 1.2  1992/07/21  00:07:06  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS ident. string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
