/******************************************************************************
  Copyright (c) 1995, 1996 Xerox Corporation.  All rights reserved.
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

/*****************************************************************************
 * Routines for manipulating DB objects
 *****************************************************************************/

#include "config.h"
#include "db.h"
#include "db_private.h"
#include "list.h"
#include "program.h"
#include "storage.h"
#include "utils.h"

static Object **objects;
static int num_objects = 0;
static int max_objects = 0;

static Var all_users;


/*********** Objects qua objects ***********/

Object *
dbpriv_find_object(Objid oid)
{
    if (oid < 0 || oid >= num_objects)
	return 0;
    else
	return objects[oid];
}

int
valid(Objid oid)
{
    return dbpriv_find_object(oid) != 0;
}

Objid
db_last_used_objid(void)
{
    return num_objects - 1;
}

void
db_reset_last_used_objid(void)
{
    while (!objects[num_objects - 1])
	num_objects--;
}

static void
ensure_new_object(void)
{
    if (max_objects == 0) {
	max_objects = 100;
	objects = mymalloc(max_objects * sizeof(Object *), M_OBJECT_TABLE);
    }
    if (num_objects >= max_objects) {
	int i;
	Object **new;

	new = mymalloc(max_objects * 2 * sizeof(Object *), M_OBJECT_TABLE);
	for (i = 0; i < max_objects; i++)
	    new[i] = objects[i];
	myfree(objects, M_OBJECT_TABLE);
	objects = new;
	max_objects *= 2;
    }
}

Object *
dbpriv_new_object(void)
{
    Object *o;

    ensure_new_object();
    o = objects[num_objects] = mymalloc(sizeof(Object), M_OBJECT);
    o->id = num_objects;
    num_objects++;

    return o;
}

void
dbpriv_new_recycled_object(void)
{
    ensure_new_object();
    objects[num_objects++] = 0;
}

Objid
db_create_object(void)
{
    Object *o;
    Objid oid;

    o = dbpriv_new_object();
    oid = o->id;

    o->name = str_dup("");
    o->flags = 0;
    o->parent = o->child = o->sibling = NOTHING;
    o->location = o->contents = o->next = NOTHING;

    o->propval = 0;

    o->propdefs.max_length = 0;
    o->propdefs.cur_length = 0;
    o->propdefs.l = 0;

    o->verbdefs = 0;

    return oid;
}

void
db_destroy_object(Objid oid)
{
    Object *o = dbpriv_find_object(oid);
    Verbdef *v, *w;
    int i;

    db_priv_affected_callable_verb_lookup();

    if (!o)
	panic("DB_DESTROY_OBJECT: Invalid object!");

    if (o->location != NOTHING || o->contents != NOTHING
	|| o->parent != NOTHING || o->child != NOTHING)
	panic("DB_DESTROY_OBJECT: Not a barren orphan!");

    if (is_user(oid)) {
	Var t;

	t.type = TYPE_OBJ;
	t.v.obj = oid;
	all_users = setremove(all_users, t);
    }
    free_str(o->name);

    for (i = 0; i < o->propdefs.cur_length; i++) {
	/* As an orphan, the only properties on this object are the ones
	 * defined on it directly, so these two arrays must be the same length.
	 */
	free_str(o->propdefs.l[i].name);
	free_var(o->propval[i].var);
    }
    if (o->propval)
	myfree(o->propval, M_PVAL);
    if (o->propdefs.l)
	myfree(o->propdefs.l, M_PROPDEF);

    for (v = o->verbdefs; v; v = w) {
	if (v->program)
	    free_program(v->program);
	free_str(v->name);
	w = v->next;
	myfree(v, M_VERBDEF);
    }

    myfree(objects[oid], M_OBJECT);
    objects[oid] = 0;
}

Objid
db_renumber_object(Objid old)
{
    Objid new;
    Object *o;

    db_priv_affected_callable_verb_lookup();

    for (new = 0; new < old; new++) {
	if (objects[new] == 0) {
	    /* Change the identity of the object. */
	    o = objects[new] = objects[old];
	    objects[old] = 0;
	    objects[new]->id = new;

	    /* Fix up the parent/children hierarchy */
	    {
		Objid oid, *oidp;

		if (o->parent != NOTHING) {
		    oidp = &objects[o->parent]->child;
		    while (*oidp != old && *oidp != NOTHING)
			oidp = &objects[*oidp]->sibling;
		    if (*oidp == NOTHING)
			panic("Object not in parent's children list");
		    *oidp = new;
		}
		for (oid = o->child;
		     oid != NOTHING;
		     oid = objects[oid]->sibling)
		    objects[oid]->parent = new;
	    }

	    /* Fix up the location/contents hierarchy */
	    {
		Objid oid, *oidp;

		if (o->location != NOTHING) {
		    oidp = &objects[o->location]->contents;
		    while (*oidp != old && *oidp != NOTHING)
			oidp = &objects[*oidp]->next;
		    if (*oidp == NOTHING)
			panic("Object not in location's contents list");
		    *oidp = new;
		}
		for (oid = o->contents;
		     oid != NOTHING;
		     oid = objects[oid]->next)
		    objects[oid]->location = new;
	    }

	    /* Fix up the list of users, if necessary */
	    if (is_user(new)) {
		int i;

		for (i = 1; i <= all_users.v.list[0].v.num; i++)
		    if (all_users.v.list[i].v.obj == old) {
			all_users.v.list[i].v.obj = new;
			break;
		    }
	    }
	    /* Fix the owners of verbs, properties and objects */
	    {
		Objid oid;

		for (oid = 0; oid < num_objects; oid++) {
		    Object *o = objects[oid];
		    Verbdef *v;
		    Pval *p;
		    int i, count;

		    if (!o)
			continue;

		    if (o->owner == new)
			o->owner = NOTHING;
		    else if (o->owner == old)
			o->owner = new;

		    for (v = o->verbdefs; v; v = v->next)
			if (v->owner == new)
			    v->owner = NOTHING;
			else if (v->owner == old)
			    v->owner = new;

		    count = dbpriv_count_properties(oid);
		    p = o->propval;
		    for (i = 0; i < count; i++)
			if (p[i].owner == new)
			    p[i].owner = NOTHING;
			else if (p[i].owner == old)
			    p[i].owner = new;
		}
	    }

	    return new;
	}
    }

    /* There are no recycled objects less than `old', so keep its number. */
    return old;
}

int
db_object_bytes(Objid oid)
{
    Object *o = objects[oid];
    int i, len, count;
    Verbdef *v;

    count = sizeof(Object) + sizeof(Object *);
    count += memo_strlen(o->name) + 1;

    for (v = o->verbdefs; v; v = v->next) {
	count += sizeof(Verbdef);
	count += memo_strlen(v->name) + 1;
	if (v->program)
	    count += program_bytes(v->program);
    }

    count += sizeof(Propdef) * o->propdefs.cur_length;
    for (i = 0; i < o->propdefs.cur_length; i++)
	count += memo_strlen(o->propdefs.l[i].name) + 1;

    len = dbpriv_count_properties(oid);
    count += (sizeof(Pval) - sizeof(Var)) * len;
    for (i = 0; i < len; i++)
	count += value_bytes(o->propval[i].var);

    return count;
}


/*********** Object attributes ***********/

Objid
db_object_owner(Objid oid)
{
    return objects[oid]->owner;
}

void
db_set_object_owner(Objid oid, Objid owner)
{
    objects[oid]->owner = owner;
}

const char *
db_object_name(Objid oid)
{
    return objects[oid]->name;
}

void
db_set_object_name(Objid oid, const char *name)
{
    Object *o = objects[oid];

    if (o->name)
	free_str(o->name);
    o->name = name;
}

Objid
db_object_parent(Objid oid)
{
    return objects[oid]->parent;
}

int
db_count_children(Objid oid)
{
    Objid c;
    int i = 0;

    for (c = objects[oid]->child; c != NOTHING; c = objects[c]->sibling)
	i++;

    return i;
}

int
db_for_all_children(Objid oid, int (*func) (void *, Objid), void *data)
{
    Objid c;

    for (c = objects[oid]->child; c != NOTHING; c = objects[c]->sibling)
	if (func(data, c))
	    return 1;

    return 0;
}

#define LL_REMOVE(where, listname, what, nextname) { \
    Objid lid; \
    if (objects[where]->listname == what) \
	objects[where]->listname = objects[what]->nextname; \
    else { \
	for (lid = objects[where]->listname; lid != NOTHING; \
	      lid = objects[lid]->nextname) { \
	    if (objects[lid]->nextname == what) { \
		objects[lid]->nextname = objects[what]->nextname; \
		break; \
	    } \
	} \
    } \
    objects[what]->nextname = NOTHING; \
}

#define LL_APPEND(where, listname, what, nextname) { \
    Objid lid; \
    if (objects[where]->listname == NOTHING) { \
	objects[where]->listname = what; \
    } else { \
	for (lid = objects[where]->listname; \
	     objects[lid]->nextname != NOTHING; \
	     lid = objects[lid]->nextname) \
	    ; \
	objects[lid]->nextname = what; \
    } \
    objects[what]->nextname = NOTHING; \
}

int
db_change_parent(Objid oid, Objid parent)
{
    Objid old_parent;

    if (!dbpriv_check_properties_for_chparent(oid, parent))
	return 0;

    if (objects[oid]->child == NOTHING && objects[oid]->verbdefs == NULL) {
	/* Since this object has no children and no verbs, we know that it
	   can't have had any part in affecting verb lookup, since we use first
	   parent with verbs as a key in the verb lookup cache. */
	/* The "no kids" rule is necessary because potentially one of the kids
	   could have verbs on it--and that kid could have cache entries for
	   THIS object's parentage. */
	/* In any case, don't clear the cache. */
	;
    } else {
	db_priv_affected_callable_verb_lookup();
    }

    old_parent = objects[oid]->parent;

    if (old_parent != NOTHING)
	LL_REMOVE(old_parent, child, oid, sibling);

    if (parent != NOTHING)
	LL_APPEND(parent, child, oid, sibling);

    objects[oid]->parent = parent;
    dbpriv_fix_properties_after_chparent(oid, old_parent);

    return 1;
}

Objid
db_object_location(Objid oid)
{
    return objects[oid]->location;
}

int
db_count_contents(Objid oid)
{
    Objid c;
    int i = 0;

    for (c = objects[oid]->contents; c != NOTHING; c = objects[c]->next)
	i++;

    return i;
}

int
db_for_all_contents(Objid oid, int (*func) (void *, Objid), void *data)
{
    Objid c;

    for (c = objects[oid]->contents; c != NOTHING; c = objects[c]->next)
	if (func(data, c))
	    return 1;

    return 0;
}

void
db_change_location(Objid oid, Objid location)
{
    Objid old_location = objects[oid]->location;

    if (valid(old_location))
	LL_REMOVE(old_location, contents, oid, next);

    if (valid(location))
	LL_APPEND(location, contents, oid, next);

    objects[oid]->location = location;
}

int
db_object_has_flag(Objid oid, db_object_flag f)
{
    return (objects[oid]->flags & (1 << f)) != 0;
}

void
db_set_object_flag(Objid oid, db_object_flag f)
{
    objects[oid]->flags |= (1 << f);
    if (f == FLAG_USER) {
	Var v;

	v.type = TYPE_OBJ;
	v.v.obj = oid;
	all_users = setadd(all_users, v);
    }
}

void
db_clear_object_flag(Objid oid, db_object_flag f)
{
    objects[oid]->flags &= ~(1 << f);
    if (f == FLAG_USER) {
	Var v;

	v.type = TYPE_OBJ;
	v.v.obj = oid;
	all_users = setremove(all_users, v);
    }
}

int
db_object_allows(Objid oid, Objid progr, db_object_flag f)
{
    return (progr == db_object_owner(oid)
	    || is_wizard(progr)
	    || db_object_has_flag(oid, f));
}

int
is_wizard(Objid oid)
{
    return valid(oid) && db_object_has_flag(oid, FLAG_WIZARD);
}

int
is_programmer(Objid oid)
{
    return valid(oid) && db_object_has_flag(oid, FLAG_PROGRAMMER);
}

int
is_user(Objid oid)
{
    return valid(oid) && db_object_has_flag(oid, FLAG_USER);
}

Var
db_all_users(void)
{
    return all_users;
}

void
dbpriv_set_all_users(Var v)
{
    all_users = v;
}

char rcsid_db_objects[] = "$Id$";

/*
 * $Log$
 * Revision 1.5  2006/09/07 00:55:02  bjj
 * Add new MEMO_STRLEN option which uses the refcounting mechanism to
 * store strlen with strings.  This is basically free, since most string
 * allocations are rounded up by malloc anyway.  This saves lots of cycles
 * computing strlen.  (The change is originally from jitmoo, where I wanted
 * inline range checks for string ops).
 *
 * Revision 1.4  1998/12/14 13:17:36  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.3  1997/07/07 03:24:53  nop
 * Merge UNSAFE_OPTS (r5) after extensive testing.
 *
 * Revision 1.2.2.2  1997/07/07 01:40:20  nop
 * Because we use first-parent-with-verbs as a verb cache key, we can skip
 * a generation bump if the target of a chparent has no kids and no verbs.
 *
 * Revision 1.2.2.1  1997/03/20 07:26:01  nop
 * First pass at the new verb cache.  Some ugly code inside.
 *
 * Revision 1.2  1997/03/03 04:18:29  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:44:59  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.5  1996/04/08  00:42:11  pavel
 * Adjusted computation in `db_object_bytes()' to account for change in the
 * definition of `value_bytes()'.  Release 1.8.0p3.
 *
 * Revision 2.4  1996/02/08  07:18:13  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.3  1996/01/16  07:23:45  pavel
 * Fixed object-array overrun when a recycled object is right on the boundary.
 * Release 1.8.0alpha6.
 *
 * Revision 2.2  1996/01/11  07:30:53  pavel
 * Fixed memory-smash bug in db_renumber_object().  Release 1.8.0alpha5.
 *
 * Revision 2.1  1995/12/11  08:08:28  pavel
 * Added `db_object_bytes()'.  Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  04:20:51  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.1  1995/11/30  04:20:41  pavel
 * Initial revision
 */
