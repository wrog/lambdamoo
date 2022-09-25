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
#include "db_io.h"
#include "exceptions.h"
#include "execute.h"
#include "functions.h"
#include "list.h"
#include "numbers.h"
#include "quota.h"
#include "server.h"
#include "storage.h"
#include "structures.h"
#include "utils.h"

static int
controls(Objid who, Objid what)
{
    return is_wizard(who) || who == db_object_owner(what);
}

static Var
make_arglist(Objid what)
{
    Var r;

    r = new_list(1);
    r.v.list[1].type = TYPE_OBJ;
    r.v.list[1].v.obj = what;

    return r;
}


struct bf_move_data {
    Objid what, where;
};

static package
do_move(Var arglist, Byte next, struct bf_move_data *data, Objid progr)
{
    Objid what = data->what, where = data->where;
    Objid oid, oldloc;
    int accepts;
    Var args;
    enum error e;

    switch (next) {
    case 1:			/* Check validity and decide `accepts' */
	if (!valid(what) || (!valid(where) && where != NOTHING))
	    return make_error_pack(E_INVARG);
	else if (!controls(progr, what))
	    return make_error_pack(E_PERM);
	else if (where == NOTHING)
	    accepts = 1;
	else {
	    args = make_arglist(what);
	    e = call_verb(where, "accept", args, 0);
	    /* e will not be E_INVIND */

	    if (e == E_NONE)
		return make_call_pack(2, data);
	    else {
		free_var(args);
		if (e == E_VERBNF)
		    accepts = 0;
		else		/* (e == E_MAXREC) */
		    return make_error_pack(e);
	    }
	}
	goto accepts_decided;

    case 2:			/* Returned from `accepts' call */
	accepts = is_true(arglist);

      accepts_decided:
	if (!is_wizard(progr) && accepts == 0)
	    return make_error_pack(E_NACC);

	if (!valid(what)
	    || (where != NOTHING && !valid(where))
	    || db_object_location(what) == where)
	    return no_var_pack();

	/* Check to see that we're not trying to violate the hierarchy */
	for (oid = where; oid != NOTHING; oid = db_object_location(oid))
	    if (oid == what)
		return make_error_pack(E_RECMOVE);

	oldloc = db_object_location(what);
	db_change_location(what, where);

	args = make_arglist(what);
	e = call_verb(oldloc, "exitfunc", args, 0);

	if (e == E_NONE)
	    return make_call_pack(3, data);
	else {
	    free_var(args);
	    if (e == E_MAXREC)
		return make_error_pack(e);
	}
	/* e == E_INVIND or E_VERBNF, in both cases fall through */

    case 3:			/* Returned from exitfunc call */
	if (valid(where) && valid(what)
	    && db_object_location(what) == where) {
	    args = make_arglist(what);
	    e = call_verb(where, "enterfunc", args, 0);
	    /* e != E_INVIND */

	    if (e == E_NONE)
		return make_call_pack(4, data);
	    else {
		free_var(args);
		if (e == E_MAXREC)
		    return make_error_pack(e);
		/* else e == E_VERBNF, fall through */
	    }
	}
    case 4:			/* Returned from enterfunc call */
	return no_var_pack();

    default:
	panic("Unknown PC in DO_MOVE");
	return no_var_pack();	/* Dead code to eliminate compiler warning */
    }
}

static package
bf_move(Var arglist, Byte next, void *vdata, Objid progr)
{
    struct bf_move_data *data = vdata;
    package p;

    if (next == 1) {
	data = alloc_data(sizeof(*data));
	data->what = arglist.v.list[1].v.obj;
	data->where = arglist.v.list[2].v.obj;
    }
    p = do_move(arglist, next, data, progr);
    free_var(arglist);

    if (p.kind != BI_CALL)
	free_data(data);

    return p;
}

static void
bf_move_write(void *vdata)
{
    struct bf_move_data *data = vdata;

    dbio_printf("bf_move data: what = %"PRIdN", where = %"PRIdN"\n",
		data->what, data->where);
}

static void *
bf_move_read()
{
    struct bf_move_data *data = alloc_data(sizeof(*data));

    if (dbio_scanf("bf_move data: what = %"PRIdN", where = %"PRIdN"\n",
		   &data->what, &data->where) == 2)
	return data;
    else
	return 0;
}

static package
bf_toobj(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    Num i;
    enum error e;

    r.type = TYPE_OBJ;
    e = become_integer(arglist.v.list[1], &i, 0);
    r.v.obj = i;

    free_var(arglist);
    if (e != E_NONE)
	return make_error_pack(e);

    return make_var_pack(r);
}

static package
bf_typeof(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    r.type = TYPE_INT;
    r.v.num = (int) arglist.v.list[1].type & TYPE_DB_MASK;
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_valid(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (object) */
    Var r;

    r.type = TYPE_INT;
    r.v.num = valid(arglist.v.list[1].v.obj);
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_max_object(Var arglist, Byte next, void *vdata, Objid progr)
{				/* () */
    Var r;

    free_var(arglist);
    r.type = TYPE_OBJ;
    r.v.obj = db_last_used_objid();
    return make_var_pack(r);
}

static package
bf_create(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (parent [, owner]) */
    Objid *data = vdata;
    Var r;

    if (next == 1) {
	Objid parent, owner;

	parent = arglist.v.list[1].v.obj;
	owner = (arglist.v.list[0].v.num == 2
		 ? arglist.v.list[2].v.obj
		 : progr);
	free_var(arglist);

	if ((valid(parent) ? !db_object_allows(parent, progr, FLAG_FERTILE)
	     : (parent != NOTHING))
	    || (owner != progr && !is_wizard(progr)))
	    return make_error_pack(E_PERM);

	if (valid(owner) && !decr_quota(owner))
	    return make_error_pack(E_QUOTA);
	else {
	    enum error e;
	    Objid oid = db_create_object();
	    Var args;

	    db_set_object_name(oid, str_dup(""));
	    db_set_object_owner(oid, owner == NOTHING ? oid : owner);
	    db_change_parent(oid, parent);

	    data = alloc_data(sizeof(*data));
	    *data = oid;
	    args = new_list(0);
	    e = call_verb(oid, "initialize", args, 0);
	    /* e will not be E_INVIND */

	    if (e == E_NONE)
		return make_call_pack(2, data);

	    free_data(data);
	    free_var(args);

	    if (e == E_MAXREC)
		return make_error_pack(e);
	    else {		/* (e == E_VERBNF) do nothing */
		r.type = TYPE_OBJ;
		r.v.obj = oid;
		return make_var_pack(r);
	    }
	}
    } else {			/* next == 2, returns from initialize verb_call */
	r.type = TYPE_OBJ;
	r.v.obj = *data;
	free_data(data);
	return make_var_pack(r);
    }
}

static void
bf_create_write(void *vdata)
{
    dbio_printf("bf_create data: oid = %"PRIdN"\n", *((Objid *) vdata));
}

static void *
bf_create_read(void)
{
    Objid *data = alloc_data(sizeof(Objid));

    if (dbio_scanf("bf_create data: oid = %"PRIdN"\n", data) == 1)
	return data;
    else
	return 0;
}

static package
bf_chparent(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (object, new_parent) */
    Objid what = arglist.v.list[1].v.obj;
    Objid parent = arglist.v.list[2].v.obj;
    Objid oid;

    free_var(arglist);
    if (!valid(what)
	|| (!valid(parent) && parent != NOTHING))
	return make_error_pack(E_INVARG);
    else if (!controls(progr, what)
	     || (valid(parent)
		 && !db_object_allows(parent, progr, FLAG_FERTILE)))
	return make_error_pack(E_PERM);
    else {
	for (oid = parent; oid != NOTHING; oid = db_object_parent(oid))
	    if (oid == what)
		return make_error_pack(E_RECMOVE);

	if (!db_change_parent(what, parent))
	    return make_error_pack(E_INVARG);
	else
	    return no_var_pack();
    }
}

static package
bf_parent(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (object) */
    Var r;
    Objid obj = arglist.v.list[1].v.obj;

    free_var(arglist);

    if (!valid(obj))
	return make_error_pack(E_INVARG);
    else {
	r.type = TYPE_OBJ;
	r.v.obj = db_object_parent(obj);
	return make_var_pack(r);
    }
}

struct children_data {
    Var r;
    int i;
};

static int
add_to_list(void *data, Objid child)
{
    struct children_data *d = data;

    d->i++;
    d->r.v.list[d->i].type = TYPE_OBJ;
    d->r.v.list[d->i].v.obj = child;

    return 0;
}

static package
bf_children(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (object) */
    Objid oid = arglist.v.list[1].v.obj;

    free_var(arglist);

    if (!valid(oid))
	return make_error_pack(E_INVARG);
    else {
	struct children_data d;

	d.r = new_list(db_count_children(oid));
	d.i = 0;
	db_for_all_children(oid, add_to_list, &d);

	return make_var_pack(d.r);
    }
}

static int
move_to_nothing(Objid oid)
{
    /* All we need to do is change the location and run the exitfunc. */
    Objid oldloc = db_object_location(oid);
    Var args;
    enum error e;

    db_change_location(oid, NOTHING);

    args = make_arglist(oid);
    e = call_verb(oldloc, "exitfunc", args, 0);

    if (e == E_NONE)
	return 1;

    free_var(args);
    return 0;
}

static int
first_proc(void *data, Objid oid)
{
    Objid *oidp = data;

    *oidp = oid;
    return 1;
}

static Objid
get_first(Objid oid, int (*for_all) (Objid, int (*)(void *, Objid), void *))
{
    Objid result = NOTHING;

    for_all(oid, first_proc, &result);

    return result;
}

static package
bf_recycle(Var arglist, Byte func_pc, void *vdata, Objid progr)
{				/* (object) */
    Objid oid, c;
    Var args;
    enum error e;
    Objid *data = vdata;

    switch (func_pc) {
    case 1:
	oid = arglist.v.list[1].v.obj;
	free_var(arglist);

	if (!valid(oid))
	    return make_error_pack(E_INVARG);
	else if (!controls(progr, oid))
	    return make_error_pack(E_PERM);

	data = alloc_data(sizeof(*data));
	*data = oid;
	args = new_list(0);
	e = call_verb(oid, "recycle", args, 0);
	/* e != E_INVIND */

	if (e == E_NONE)
	    return make_call_pack(2, data);
	/* else e == E_VERBNF or E_MAXREC; fall through */
	free_var(args);
	goto moving_contents;

    case 2:			/* moving all contents to #-1 */
	free_var(arglist);
	oid = *data;

      moving_contents:
	if (!valid(oid)) {
	    free_data(data);
	    return no_var_pack();
	}
	while ((c = get_first(oid, db_for_all_contents)) != NOTHING)
	    if (move_to_nothing(c))
		return make_call_pack(2, data);

	if (db_object_location(oid) != NOTHING
	    && move_to_nothing(oid))
	    /* Return to same case because this :exitfunc might add new */
	    /* contents to OID or even move OID right back in. */
	    return make_call_pack(2, data);

	/* We can now be confident that OID has no contents and no location */

	/* Do the same thing for the inheritance hierarchy (much easier!) */
	while ((c = get_first(oid, db_for_all_children)) != NOTHING)
	    db_change_parent(c, db_object_parent(oid));
	db_change_parent(oid, NOTHING);

	/* Finish the demolition. */
	incr_quota(db_object_owner(oid));
	db_destroy_object(oid);

	free_data(data);
	return no_var_pack();
    }

    panic("Can't happen in BF_RECYCLE");
    return no_var_pack();
}

static void
bf_recycle_write(void *vdata)
{
    Objid *data = vdata;

    dbio_printf("bf_recycle data: oid = %"PRIdN", cont = 0\n", *data);
}

static void *
bf_recycle_read(void)
{
    Objid *data = alloc_data(sizeof(*data));
    int dummy;

    /* I use a `dummy' variable here and elsewhere instead of the `*'
     * assignment-suppression syntax of `scanf' because it allows more
     * straightforward error checking; unfortunately, the standard says that
     * suppressed assignments are not counted in determining the returned value
     * of `scanf'...
     */
    if (dbio_scanf("bf_recycle data: oid = %"PRIdN", cont = %d\n",
		   data, &dummy) == 2)
	return data;
    else
	return 0;
}


static package
bf_players(Var arglist, Byte next, void *vdata, Objid progr)
{				/* () */
    free_var(arglist);
    return make_var_pack(var_ref(db_all_users()));
}

static package
bf_is_player(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (object) */
    Var r;
    Objid oid = arglist.v.list[1].v.obj;

    free_var(arglist);

    if (!valid(oid))
	return make_error_pack(E_INVARG);

    r.type = TYPE_INT;
    r.v.num = is_user(oid);
    return make_var_pack(r);
}

static package
bf_set_player_flag(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (object, yes/no) */
    Var obj;
    char bool;

    obj = arglist.v.list[1];
    bool = is_true(arglist.v.list[2]);

    free_var(arglist);

    if (!valid(obj.v.obj))
	return make_error_pack(E_INVARG);
    else if (!is_wizard(progr))
	return make_error_pack(E_PERM);

    if (bool) {
	db_set_object_flag(obj.v.obj, FLAG_USER);
    } else {
	boot_player(obj.v.obj);
	db_clear_object_flag(obj.v.obj, FLAG_USER);
    }
    return no_var_pack();
}

static package
bf_object_bytes(Var arglist, Byte next, void *vdata, Objid progr)
{
    Objid oid = arglist.v.list[1].v.obj;
    Var v;

    free_var(arglist);

    if (!is_wizard(progr))
	return make_error_pack(E_PERM);
    else if (!valid(oid))
	return make_error_pack(E_INVIND);

    v.type = TYPE_INT;
    v.v.num = db_object_bytes(oid);

    return make_var_pack(v);
}

void
register_objects(void)
{
    register_function("toobj", 1, 1, bf_toobj, TYPE_ANY);
    register_function("typeof", 1, 1, bf_typeof, TYPE_ANY);
    register_function_with_read_write("create", 1, 2, bf_create,
				      bf_create_read, bf_create_write,
				      TYPE_OBJ, TYPE_OBJ);
    register_function_with_read_write("recycle", 1, 1, bf_recycle,
				      bf_recycle_read, bf_recycle_write,
				      TYPE_OBJ);
    register_function("object_bytes", 1, 1, bf_object_bytes, TYPE_OBJ);
    register_function("valid", 1, 1, bf_valid, TYPE_OBJ);
    register_function("parent", 1, 1, bf_parent, TYPE_OBJ);
    register_function("children", 1, 1, bf_children, TYPE_OBJ);
    register_function("chparent", 2, 2, bf_chparent, TYPE_OBJ, TYPE_OBJ);
    register_function("max_object", 0, 0, bf_max_object);
    register_function("players", 0, 0, bf_players);
    register_function("is_player", 1, 1, bf_is_player, TYPE_OBJ);
    register_function("set_player_flag", 2, 2, bf_set_player_flag,
		      TYPE_OBJ, TYPE_ANY);
    register_function_with_read_write("move", 2, 2, bf_move,
				      bf_move_read, bf_move_write,
				      TYPE_OBJ, TYPE_OBJ);
}

char rcsid_objects[] = "$Id$";

/*
 * $Log$
 * Revision 1.4  1998/12/14 13:18:39  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.3  1997/07/07 03:24:54  nop
 * Merge UNSAFE_OPTS (r5) after extensive testing.
 *
 * Revision 1.2.2.1  1997/03/20 18:07:50  bjj
 * Add a flag to the in-memory type identifier so that inlines can cheaply
 * identify Vars that need actual work done to ref/free/dup them.  Add the
 * appropriate inlines to utils.h and replace old functions in utils.c with
 * complex_* functions which only handle the types with external storage.
 *
 * Revision 1.2  1997/03/03 04:19:12  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:01  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.3  1996/04/19  01:17:48  pavel
 * Rationalized the errors that can be raised from chparent().
 * Release 1.8.0p4.
 *
 * Revision 2.2  1996/02/08  06:55:49  pavel
 * Renamed TYPE_NUM to TYPE_INT and become_number() to become_integer().
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.1  1995/12/11  08:02:36  pavel
 * Moved `value_bytes()' to list.c.  Added `object_bytes()'.
 * Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  04:29:18  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.12  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.11  1992/10/23  22:02:24  pavel
 * Eliminated all uses of the useless macro NULL.
 *
 * Revision 1.10  1992/10/17  20:47:39  pavel
 * Global rename of strdup->str_dup, strref->str_ref, vardup->var_dup, and
 * varref->var_ref.
 *
 * Revision 1.9  1992/09/14  18:40:24  pjames
 * Updated #includes.  Moved rcsid to bottom.
 *
 * Revision 1.8  1992/09/14  17:32:23  pjames
 * Moved db_modification code to db modules.
 *
 * Revision 1.7  1992/09/08  22:02:11  pjames
 * Renamed bf_obj.c to objects.c
 *
 * Revision 1.6  1992/09/04  01:24:21  pavel
 * Added support for the `f' (for `fertile') bit on objects.
 *
 * Revision 1.5  1992/09/03  16:32:24  pjames
 * Free's propdefs using new array structure.
 *
 * Revision 1.4  1992/08/28  16:04:12  pjames
 * Changed myfree(*, M_STRING) to free_str(*).
 *
 * Revision 1.3  1992/08/10  17:18:58  pjames
 * Updated #includes.  Move bf_pass to execute.c Updated to use new
 * registration format.  Built in functions now only receive programmer,
 * instead of entire Parse_Info.
 *
 * Revision 1.2  1992/07/20  23:52:48  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS ident. string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
