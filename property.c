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

#include "db.h"
#include "functions.h"
#include "list.h"
#include "storage.h"
#include "utils.h"

struct prop_data {
    Var r;
    int i;
};

static int
add_to_list(void *data, const char *prop_name)
{
    struct prop_data *d = data;

    d->i++;
    d->r.v.list[d->i].type = TYPE_STR;
    d->r.v.list[d->i].v.str = str_ref(prop_name);

    return 0;
}

static package
bf_properties(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (object) */
    Objid oid = arglist.v.list[1].v.obj;

    free_var(arglist);

    if (!valid(oid))
	return make_error_pack(E_INVARG);
    else if (!db_object_allows(oid, progr, FLAG_READ))
	return make_error_pack(E_PERM);
    else {
	struct prop_data d;

	d.r = new_list(db_count_propdefs(oid));
	d.i = 0;
	db_for_all_propdefs(oid, add_to_list, &d);

	return make_var_pack(d.r);
    }
}


static package
bf_prop_info(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (object, prop-name) */
    Objid oid = arglist.v.list[1].v.obj;
    const char *pname = arglist.v.list[2].v.str;
    db_prop_handle h;
    Var r;
    unsigned flags;
    char *s;

    if (!valid(oid)) {
	free_var(arglist);
	return make_error_pack(E_INVARG);
    }
    h = db_find_property(oid, pname, 0);
    free_var(arglist);

    if (!h.ptr || h.built_in)
	return make_error_pack(E_PROPNF);
    else if (!db_property_allows(h, progr, PF_READ))
	return make_error_pack(E_PERM);

    r = new_list(2);
    r.v.list[1].type = TYPE_OBJ;
    r.v.list[1].v.obj = db_property_owner(h);
    r.v.list[2].type = TYPE_STR;
    r.v.list[2].v.str = s = str_dup("xxx");
    flags = db_property_flags(h);
    if (flags & PF_READ)
	*s++ = 'r';
    if (flags & PF_WRITE)
	*s++ = 'w';
    if (flags & PF_CHOWN)
	*s++ = 'c';
    *s = '\0';

    return make_var_pack(r);
}

static enum error
validate_prop_info(Var v, Objid * owner, unsigned *flags, const char **name)
{
    const char *s;
    int len = (v.type == TYPE_LIST ? v.v.list[0].v.num : 0);

    if (!((len == 2 || len == 3)
	  && v.v.list[1].type == TYPE_OBJ
	  && v.v.list[2].type == TYPE_STR
	  && (len == 2 || v.v.list[3].type == TYPE_STR)))
	return E_TYPE;

    *owner = v.v.list[1].v.obj;
    if (!valid(*owner))
	return E_INVARG;

    for (*flags = 0, s = v.v.list[2].v.str; *s; s++) {
	switch (*s) {
	case 'r':
	case 'R':
	    *flags |= PF_READ;
	    break;
	case 'w':
	case 'W':
	    *flags |= PF_WRITE;
	    break;
	case 'c':
	case 'C':
	    *flags |= PF_CHOWN;
	    break;
	default:
	    return E_INVARG;
	}
    }

    if (len == 2)
	*name = 0;
    else
	*name = v.v.list[3].v.str;

    return E_NONE;
}

static enum error
set_prop_info(Objid oid, const char *pname, Var info, Objid progr)
{
    Objid new_owner;
    unsigned new_flags;
    const char *new_name;
    enum error e;
    db_prop_handle h;

    if (!valid(oid))
	e = E_INVARG;
    else
	e = validate_prop_info(info, &new_owner, &new_flags, &new_name);

    if (e != E_NONE)
	return e;

    h = db_find_property(oid, pname, 0);

    if (!h.ptr || h.built_in)
	return E_PROPNF;
    else if (!db_property_allows(h, progr, PF_WRITE)
	     || (!is_wizard(progr) && db_property_owner(h) != new_owner))
	return E_PERM;

    if (new_name) {
	if (!db_rename_propdef(oid, pname, new_name))
	    return E_INVARG;

	h = db_find_property(oid, new_name, 0);
    }
    db_set_property_owner(h, new_owner);
    db_set_property_flags(h, new_flags);

    return E_NONE;
}

static package
bf_set_prop_info(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (object, prop-name, {owner, perms [, new-name]}) */
    Objid oid = arglist.v.list[1].v.obj;
    const char *pname = arglist.v.list[2].v.str;
    Var info = arglist.v.list[3];
    enum error e = set_prop_info(oid, pname, info, progr);

    free_var(arglist);
    if (e == E_NONE)
	return no_var_pack();
    else
	return make_error_pack(e);
}

static package
bf_add_prop(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (object, prop-name, initial-value, initial-info) */
    Objid oid = arglist.v.list[1].v.obj;
    const char *pname = arglist.v.list[2].v.str;
    Var value = arglist.v.list[3];
    Var info = arglist.v.list[4];
    Objid owner;
    unsigned flags;
    const char *new_name;
    enum error e;

    if ((e = validate_prop_info(info, &owner, &flags, &new_name)) != E_NONE);	/* Already failed */
    else if (new_name)
	e = E_TYPE;
    else if (!valid(oid))
	e = E_INVARG;
    else if (!db_object_allows(oid, progr, FLAG_WRITE)
	     || (progr != owner && !is_wizard(progr)))
	e = E_PERM;
    else if (!db_add_propdef(oid, pname, value, owner, flags))
	e = E_INVARG;

    free_var(arglist);
    if (e == E_NONE)
	return no_var_pack();
    else
	return make_error_pack(e);
}

static package
bf_delete_prop(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (object, prop-name) */
    Objid oid = arglist.v.list[1].v.obj;
    const char *pname = arglist.v.list[2].v.str;
    enum error e = E_NONE;

    if (!valid(oid))
	e = E_INVARG;
    else if (!db_object_allows(oid, progr, FLAG_WRITE))
	e = E_PERM;
    else if (!db_delete_propdef(oid, pname))
	e = E_PROPNF;

    free_var(arglist);
    if (e == E_NONE)
	return no_var_pack();
    else
	return make_error_pack(e);
}

static package
bf_clear_prop(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (object, prop-name) */
    Objid oid = arglist.v.list[1].v.obj;
    const char *pname = arglist.v.list[2].v.str;
    db_prop_handle h;
    Var value;
    enum error e;

    if (!valid(oid))
	e = E_INVARG;
    else {
	h = db_find_property(oid, pname, 0);
	if (!h.ptr)
	    e = E_PROPNF;
	else if (h.built_in
		 || (progr != db_property_owner(h) && !is_wizard(progr)))
	    e = E_PERM;
	else if (h.definer == oid)
	    e = E_INVARG;
	else {
	    value.type = TYPE_CLEAR;
	    db_set_property_value(h, value);
	    e = E_NONE;
	}
    }

    free_var(arglist);
    if (e == E_NONE)
	return no_var_pack();
    else
	return make_error_pack(e);
}

static package
bf_is_clear_prop(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (object, prop-name) */
    Objid oid = arglist.v.list[1].v.obj;
    const char *pname = arglist.v.list[2].v.str;
    Var r;
    db_prop_handle h;
    enum error e;

    if (!valid(oid))
	e = E_INVARG;
    else {
	h = db_find_property(oid, pname, 0);
	if (!h.ptr)
	    e = E_PROPNF;
	else if (!h.built_in && !db_property_allows(h, progr, PF_READ))
	    e = E_PERM;
	else {
	    r.type = TYPE_INT;
	    r.v.num = (!h.built_in && db_property_value(h).type == TYPE_CLEAR);
	    e = E_NONE;
	}
    }

    free_var(arglist);
    if (e == E_NONE)
	return make_var_pack(r);
    else
	return make_error_pack(e);
}

void
register_property(void)
{
    (void) register_function("properties", 1, 1, bf_properties, TYPE_OBJ);
    (void) register_function("property_info", 2, 2, bf_prop_info,
			     TYPE_OBJ, TYPE_STR);
    (void) register_function("set_property_info", 3, 3, bf_set_prop_info,
			     TYPE_OBJ, TYPE_STR, TYPE_LIST);
    (void) register_function("add_property", 4, 4, bf_add_prop,
			     TYPE_OBJ, TYPE_STR, TYPE_ANY, TYPE_LIST);
    (void) register_function("delete_property", 2, 2, bf_delete_prop,
			     TYPE_OBJ, TYPE_STR);
    (void) register_function("clear_property", 2, 2, bf_clear_prop,
			     TYPE_OBJ, TYPE_STR);
    (void) register_function("is_clear_property", 2, 2, bf_is_clear_prop,
			     TYPE_OBJ, TYPE_STR);
}

char rcsid_property[] = "$Id$";

/*
 * $Log$
 * Revision 1.4  2008/08/20 04:25:23  bjj
 * Fix iffy usage of str_dup in verb_info() and property_info() which could
 * cause poor behavior with MEMO_STRLEN (and other future string optimizations)
 *
 * Revision 1.3  1998/12/14 13:18:50  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:19:18  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:01  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.3  1996/02/11  00:44:45  pavel
 * Fixed potential panic in clear_property() when called on the definer of the
 * property in question.  Release 1.8.0beta2.
 *
 * Revision 2.2  1996/02/08  06:54:18  pavel
 * Renamed TYPE_NUM to TYPE_INT.  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.1  1995/12/11  07:53:03  pavel
 * Added support for renaming properties.
 *
 * Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  04:30:40  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.12  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.11  1992/09/14  18:39:04  pjames
 * Updated #includes.  Moved rcsid to bottom.
 *
 * Revision 1.10  1992/09/14  17:42:25  pjames
 * Moved db_modification code to db modules.
 *
 * Revision 1.9  1992/09/08  22:05:03  pjames
 * changed `register_bf_prop()' to `register_property()'
 *
 * Revision 1.8  1992/09/04  22:41:36  pavel
 * Fixed some picky ANSI C problems with (const char *)'s.
 *
 * Revision 1.7  1992/09/04  01:16:50  pavel
 * Added support for the `f' (for `fertile') bit on objects.
 *
 * Revision 1.6  1992/09/03  16:28:46  pjames
 * Added `new_propdef()' and `str_hash()' for hash-lookup of properties.
 * Changed code to handle new representation of property definitions (arrays).
 * Added TYPE_CLEAR handling.
 * Added built-in functions `clear_property()' and `is_clear_property()'.
 *
 * Revision 1.5  1992/08/31  22:25:39  pjames
 * Changed some `char *'s to `const char *'
 *
 * Revision 1.4  1992/08/28  16:18:45  pjames
 * Changed myfree(*, M_STRING) to free_str(*).
 * Changed some strref's to strdup.
 * Changed vardup to varref.
 *
 * Revision 1.3  1992/08/10  17:41:48  pjames
 * Moved get_bi_prop, get_prop, set_bi_prop, and put_prop from execute.c.
 *
 * Revision 1.2  1992/07/21  00:06:01  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS ident. string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
