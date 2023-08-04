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

#include "my-stdlib.h"
#include "my-string.h"

#include "config.h"
#include "db.h"
#include "exceptions.h"
#include "structures.h"
#include "match.h"
#include "parse_cmd.h"
#include "storage.h"
#include "unparse.h"
#include "utils.h"

static Var *
aliases(Objid oid)
{
    Var value;
    db_prop_handle h;

    h = db_find_property(oid, "aliases", &value);
    if (!h.ptr || value.type != TYPE_LIST) {
	/* Simulate a pointer to an empty list */
	return &zero;
    } else
	return value.v.list;
}

struct match_data {
    int lname;
    const char *name;
    Objid exact, partial;
};

static int
match_proc(void *data, Objid oid)
{
    struct match_data *d = data;
    Var *names = aliases(oid);
    int i;
    const char *name;

    for (i = 0; i <= names[0].v.num; i++) {
	if (i == 0)
	    name = db_object_name(oid);
	else if (names[i].type != TYPE_STR)
	    continue;
	else
	    name = names[i].v.str;

	if (!mystrncasecmp(name, d->name, d->lname)) {
	    if (name[d->lname] == '\0') {	/* exact match */
		if (d->exact == NOTHING || d->exact == oid)
		    d->exact = oid;
		else
		    return 1;
	    } else {		/* partial match */
		if (d->partial == FAILED_MATCH || d->partial == oid)
		    d->partial = oid;
		else
		    d->partial = AMBIGUOUS;
	    }
	}
    }

    return 0;
}

static Objid
match_contents(Objid player, const char *name)
{
    Objid loc;
    int step;
    Objid oid;
    struct match_data d;

    d.lname = strlen(name);
    d.name = name;
    d.exact = NOTHING;
    d.partial = FAILED_MATCH;

    if (!valid(player))
	return FAILED_MATCH;
    loc = db_object_location(player);

    for (oid = player, step = 0; step < 2; oid = loc, step++) {
	if (!valid(oid))
	    continue;
	if (db_for_all_contents(oid, match_proc, &d))
	    /* We only abort the enumeration for exact ambiguous matches... */
	    return AMBIGUOUS;
    }

    if (d.exact != NOTHING)
	return d.exact;
    else
	return d.partial;
}

Objid
match_object(Objid player, const char *name)
{
    if (name[0] == '\0')
	return NOTHING;
    if (name[0] == '#') {
	char *p;
	Objid r = strtol(name + 1, &p, 10);

	if (*p != '\0' || !valid(r))
	    return FAILED_MATCH;
	return r;
    }
    if (!valid(player))
	return FAILED_MATCH;
    if (!mystrcasecmp(name, "me"))
	return player;
    if (!mystrcasecmp(name, "here"))
	return db_object_location(player);
    return match_contents(player, name);
}

char rcsid_match[] = "$Id$";

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:18:02  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:50  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:00  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:59:41  pavel
 * Fixed minor portability problem.  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:27:39  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.10  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.9  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.8  1992/09/14  18:39:47  pjames
 * Updated #includes.  Moved rcsid to bottom.
 *
 * Revision 1.7  1992/09/14  17:43:52  pjames
 * Moved db_modification code to db modules.
 *
 * Revision 1.6  1992/09/03  16:29:24  pjames
 * Changed to use hashing to find property definitions.
 *
 * Revision 1.5  1992/08/31  22:27:12  pjames
 * Changed some `char *'s to `const char *'
 *
 * Revision 1.4  1992/08/21  00:42:43  pavel
 * Renamed include file "parse_command.h" to "parse_cmd.h".
 *
 * Revision 1.3  1992/08/10  16:54:00  pjames
 * Updated #includes.
 *
 * Revision 1.2  1992/07/21  00:04:40  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS ident. string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 * */
