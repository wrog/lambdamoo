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

#include "functions.h"
#include "bf_register.h"

#include "my-stdarg.h"

#include "db_io.h"
#include "execute.h"
#include "list.h"
#include "log.h"
#include "server.h"
#include "storage.h"
#include "streams.h"
#include "structures.h"
#include "tasks.h"
#include "unparse.h"
#include "utils.h"
#include "waif.h"

/*******************************************************
 * see bf_register.h for how to define registration
 * functions that run at server startup.
 *******************************************************/

/*** register ***/

struct bft_entry {
    const char *name;
    const char *protect_str;
    const char *verb_str;
    int minargs;
    int maxargs;
    var_type *prototype;
    bf_type func;
    bf_read_type read;
    bf_write_type write;
    bf_free_type free;
    bf_import_type import;
    bf_export_type export;
    int protected;
};

static struct bft_entry bf_table[MAX_FUNC];
static unsigned top_bf_table = 0;

unsigned
register_function(const char *name, int minargs, int maxargs, bf_type func, ...)
{
    int num_arg_types = maxargs == -1 ? minargs : maxargs;

    if (top_bf_table == MAX_FUNC) {
	errlog("too many functions.  %s cannot be registered.\n", name);
	return 0;
    }
    bf_table[top_bf_table].name = str_dup(name);

    Stream *s = new_stream(0);
    stream_printf(s, "protect_%s", name);
    bf_table[top_bf_table].protect_str = str_dup(reset_stream(s));
    stream_printf(s, "bf_%s", name);
    bf_table[top_bf_table].verb_str = str_dup_then_free_stream(s);

    bf_table[top_bf_table].minargs = minargs;
    bf_table[top_bf_table].maxargs = maxargs;
    bf_table[top_bf_table].func = func;
    bf_table[top_bf_table].read  = NULL;
    bf_table[top_bf_table].write = NULL;
    bf_table[top_bf_table].free  = NULL;
    bf_table[top_bf_table].import = NULL;
    bf_table[top_bf_table].export = NULL;
    bf_table[top_bf_table].protected = 0;

    var_type *proto = NULL;
    if (num_arg_types > 0) {
	va_list args;
	int va_index;

	proto = mymalloc(num_arg_types * sizeof(var_type), M_PROTOTYPE);
	va_start(args, func);
	for (va_index = 0; va_index < num_arg_types; ++va_index)
	    proto[va_index] = va_arg(args, var_type);
	va_end(args);
    }
    bf_table[top_bf_table].prototype = proto;
    return top_bf_table++;
}

void
register_function_dbio(bf_read_type read, bf_write_type write)
{
    if (top_bf_table == 0)
	panic("register_function_dbio: register_function() not called?");

    bf_table[top_bf_table-1].read = read;
    bf_table[top_bf_table-1].write = write;
}

void
register_function_free(bf_free_type free)
{
    if (top_bf_table == 0)
	panic("register_function_dbio: register_function() not called?");

    bf_table[top_bf_table-1].free = free;
}

void
register_function_state(bf_import_type import, bf_export_type export)
{
    if (top_bf_table == 0)
	panic("register_function_state: register_function() not called?");

    bf_table[top_bf_table-1].import = import;
    bf_table[top_bf_table-1].export = export;
}

/*** looking up functions -- by name or num ***/

static const char *func_not_found_msg = "no such function";

const char *
name_func_by_num(unsigned n)
{				/* used by unparse only */
    if (n >= top_bf_table)
	return func_not_found_msg;
    else
	return bf_table[n].name;
}

unsigned
number_func_by_name(const char *name)
{				/* used by parser only */
    unsigned i;

    for (i = 0; i < top_bf_table; i++)
	if (!mystrcasecmp(name, bf_table[i].name))
	    return i;

    return FUNC_NOT_FOUND;
}

/*** calling built-in functions ***/

package
call_bi_func(unsigned n, Var arglist, Byte func_pc,
	     Objid progr, void *vdata)
     /* requires arglist.type == TYPE_LIST
        call_bi_func will free arglist */
{
    struct bft_entry *f;

    if (n >= top_bf_table) {
	errlog("CALL_BI_FUNC: Unknown function number: %d\n", n);
	free_var(arglist);
	return no_var_pack();
    }
    f = bf_table + n;

    if (func_pc == 1) {		/* check arg types and count *ONLY* for first entry */
	int k, max;
	Var *args = arglist.v.list;

	/*
	 * Check permissions, if protected
	 */
	/* if (caller() != SYSTEM_OBJECT && server_flag_option(f->protect_str, 0)) { */
	if (caller() != SYSTEM_OBJECT && f->protected) {
	    /* Try calling #0:bf_FUNCNAME(@ARGS) instead */
	    enum error e = call_verb2(SYSTEM_OBJECT, f->verb_str
				      WAIF_COMMA_ARG(((Var){ .type=TYPE_OBJ,
							     .v.obj=SYSTEM_OBJECT })),
				      arglist, 0);

	    if (e == E_NONE)
		return tail_call_pack();

	    if (e == E_MAXREC || !is_wizard(progr)) {
		free_var(arglist);
		return make_error_pack(e == E_MAXREC ? e : E_PERM);
	    }
	}
	/*
	 * Check argument count
	 * (Can't always check in the compiler, because of @)
	 */
	if (args[0].v.num < f->minargs
	    || (f->maxargs != -1 && args[0].v.num > f->maxargs)) {
	    free_var(arglist);
	    return make_error_pack(E_ARGS);
	}
	/*
	 * Check argument types
	 */
	max = (f->maxargs == -1) ? f->minargs : args[0].v.num;

	for (k = 0; k < max; k++) {
	    var_type proto = f->prototype[k];
	    var_type arg = args[k + 1].type;

	    if (!(proto == TYPE_ANY
		  || (proto == TYPE_NUMERIC && (arg == TYPE_INT
						|| arg == TYPE_FLOAT))
		  || proto == arg)) {
		free_var(arglist);
		return make_error_pack(E_TYPE);
	    }
	}
    } else if (func_pc == 2 && vdata == &call_bi_func) {
	/* This is a return from calling #0:bf_FUNCNAME(@ARGS); return what
	 * it returned.  If it errored, what we do will be ignored.
	 */
	return make_var_pack(arglist);
    }
    /*
     * do the function
     */
    return (*(f->func)) (arglist, func_pc, vdata, progr);
    /* f->func is responsible for freeing/using up arglist. */
}

void
free_bi_func_data(Byte f_id, void *vdata)
{
    if (!vdata)
	return;

    if (f_id >= top_bf_table)
	panic("FREE_BI_FUNC_DATA for unknown function??");

    bf_free_type freefn = bf_table[f_id].free;
    if (freefn)
	freefn(vdata);
    free_data(vdata);
}

void
write_bi_func_data(Byte f_id, void *vdata)
{
    if (f_id >= top_bf_table)
	errlog("WRITE_BI_FUNC_DATA: Unknown function number: %d\n", f_id);
    else if (bf_table[f_id].write)
	(*(bf_table[f_id].write)) (vdata);
}

static Byte *pc_for_bi_func_data_being_read;

Byte *
pc_for_bi_func_data(void)
{
    return pc_for_bi_func_data_being_read;
}

int
read_bi_func_data(Byte f_id, void **bi_func_state, Byte * bi_func_pc)
{
    pc_for_bi_func_data_being_read = bi_func_pc;

    if (f_id >= top_bf_table) {
	errlog("READ_BI_FUNC_DATA: Unknown function number: %d\n", f_id);
	*bi_func_state = 0;
	return 0;
    } else if (bf_table[f_id].read) {
	*bi_func_state = (*(bf_table[f_id].read)) ();
	if (*bi_func_state == 0) {
	    errlog("READ_BI_FUNC_DATA: Can't read data for %s()\n",
		   bf_table[f_id].name);
	    return 0;
	}
    } else {
	*bi_func_state = 0;
	/* The following code checks for the easily-detectable case of the
	 * bug described in the Version 1.8.0p4 entry in ChangeLog.txt.
	 */
	if (*bi_func_pc == 2 && dbio_input_version == DBV_Float
	    && strcmp(bf_table[f_id].name, "eval") != 0) {
	    oklog("LOADING: Warning: patching bogus return to `%s()'\n",
		  bf_table[f_id].name);
	    oklog("         (See 1.8.0p4 ChangeLog.txt entry for details.)\n");
	    *bi_func_pc = 0;
	}
    }
    return 1;
}

int
export_bi_func_state(void *data, Byte f_id, unsigned *version, Var *payload)
{
    if (f_id >= top_bf_table)
	return 0;
    if (bf_table[f_id].export)
	return (*(bf_table[f_id].export)) (data, version, payload);
    if (data)
	return 0;

    *version = 1;
    payload->type = TYPE_NONE;
    return 1;
}

int
import_bi_func_state(Byte f_id, unsigned version, Var payload, void **data)
{
    if (f_id >= top_bf_table)
	return 0;
    if (bf_table[f_id].import) {
	*data = (*(bf_table[f_id].import)) (version, payload);
	return *data != 0;
    }
    if (version != 1 || payload.type != TYPE_NONE)
	return 0;

    *data = 0;
    return 1;
}


package
make_abort_pack(enum abort_reason reason)
{
    package p;

    p.kind = BI_ABORT;
    p.u.why = reason;
    return p;
}

package
make_error_pack(enum error err)
{
    return make_raise_pack(err, unparse_error(err), zero);
}

package
make_raise_pack(enum error err, const char *msg, Var value)
{
    package p;

    p.kind = BI_RAISE;
    p.u.raise.code.type = TYPE_ERR;
    p.u.raise.code.v.err = err;
    p.u.raise.msg = str_dup(msg);
    p.u.raise.value = value;

    return p;
}

package
make_space_pack(void)
{
    if (server_flag_option_cached(SVO_MAX_CONCAT_CATCHABLE))
	return make_error_pack(E_QUOTA);
    else
	return make_abort_pack(ABORT_SECONDS);
}

package
make_var_pack(Var v)
{
    package p;

    p.kind = BI_RETURN;
    p.u.ret = v;

    return p;
}

package
no_var_pack(void)
{
    return make_var_pack(zero);
}

package
make_int_pack(Num v)
{
    package p;

    p.kind = BI_RETURN;
    p.u.ret.type = TYPE_INT;
    p.u.ret.v.num = v;

    return p;
}

package
make_float_pack(FlNum v)
{
    package p;

    p.kind = BI_RETURN;
    p.u.ret.type = TYPE_FLOAT;
    p.u.ret.v.fnum = box_fl(v);

    return p;
}

package
make_string_pack(const char *s)
{
    package p;

    p.kind = BI_RETURN;
    p.u.ret.type = TYPE_STR;
    p.u.ret.v.str = s;

    return p;
}

package
make_call_pack(Byte pc, void *vdata)
{
    package p;

    p.kind = BI_CALL;
    p.u.call.pc = pc;
    p.u.call.data = vdata;

    return p;
}

package
tail_call_pack(void)
{
    return make_call_pack(0, 0);
}

package
make_suspend_pack(enum error(*proc) (vm, void *), void *data)
{
    package p;

    p.kind = BI_SUSPEND;
    p.u.susp.proc = proc;
    p.u.susp.data = data;

    return p;
}

static Var
function_description(int i)
{
    struct bft_entry entry;
    Var v, vv;
    int j, nargs;

    entry = bf_table[i];
    v = new_list(4);
    v.v.list[1].type = TYPE_STR;
    v.v.list[1].v.str = str_ref(entry.name);
    v.v.list[2].type = TYPE_INT;
    v.v.list[2].v.num = entry.minargs;
    v.v.list[3].type = TYPE_INT;
    v.v.list[3].v.num = entry.maxargs;
    nargs = entry.maxargs == -1 ? entry.minargs : entry.maxargs;
    vv = v.v.list[4] = new_list(nargs);
    for (j = 0; j < nargs; j++) {
	int proto = entry.prototype[j];
	vv.v.list[j + 1].type = TYPE_INT;
	vv.v.list[j + 1].v.num = proto < 0 ? proto : (proto & TYPE_DB_MASK);
    }

    return v;
}

static package
bf_function_info(Var arglist, Byte next UNUSED_, void *vdata UNUSED_, Objid progr UNUSED_)
{
    Var r;
    unsigned i;

    if (arglist.v.list[0].v.num == 1) {
	i = number_func_by_name(arglist.v.list[1].v.str);
	if (i == FUNC_NOT_FOUND) {
	    free_var(arglist);
	    return make_error_pack(E_INVARG);
	}
	r = function_description(i);
    } else {
	r = new_list(top_bf_table);
	for (i = 0; i < top_bf_table; i++)
	    r.v.list[i + 1] = function_description(i);
    }

    free_var(arglist);
    return make_var_pack(r);
}

static void
load_server_protect_function_flags(void)
{
    unsigned i;

    for (i = 0; i < top_bf_table; i++) {
	bf_table[i].protected
	    = server_flag_option(bf_table[i].protect_str, 0);
    }
    oklog("Loaded protect cache for %d builtin functions\n", i);
}

Num _server_int_option_cache[SVO__CACHE_SIZE];

void
load_server_options(void)
{
    Num value;

    load_server_protect_function_flags();

# define _BP_DO(PROPERTY, property)				\
      _server_int_option_cache[SVO_PROTECT_##PROPERTY]		\
	  = server_flag_option("protect_" #property, 0);	\

    BUILTIN_PROPERTIES(_BP_DO);

# undef _BP_DO

# define _SVO_DO(SVO_MISC_OPTION, misc_option,			\
		 kind, DEFAULT, CANONICALIZE)			\
      value = server_##kind##_option(#misc_option, DEFAULT);	\
      CANONICALIZE;						\
      _server_int_option_cache[SVO_MISC_OPTION] = value;	\

    SERVER_OPTIONS_CACHED_MISC(_SVO_DO, value);

# undef _SVO_DO
}

static package
bf_load_server_options(Var arglist, Byte next UNUSED_, void *vdata UNUSED_, Objid progr)
{
    free_var(arglist);

    if (!is_wizard(progr)) {
	return make_error_pack(E_PERM);
    }
    load_server_options();

    return no_var_pack();
}

static void
after_load_load_server_options(int success)
{
    if (success)
	load_server_options();
}

void
register_functions(void)
{
    register_db_load_hooks(DBLOAD_SEQ_functions,
			   NULL, after_load_load_server_options,
			   "loading server options");

    register_function("function_info", 0, 1, bf_function_info, TYPE_STR);
    register_function("load_server_options", 0, 0, bf_load_server_options);
}


/*
 * $Log$
 * Revision 2.6  1996/04/19  01:20:49  pavel
 * Fixed bug in how $bf_FOO() verbs are called to override built-in functions.
 * Added code to screen out the worst of the problems this bug could
 * potentially cause.  Added a way for built-in functions to tail-call a MOO
 * verb (i.e., to do so without having to handle the verb's returned value).
 * Release 1.8.0p4.
 *
 * Revision 2.5  1996/04/08  01:03:59  pavel
 * Moved `protected' verb test to before arg-count and -type checks.
 * Release 1.8.0p3.
 *
 * Revision 2.4  1996/03/19  07:13:04  pavel
 * Fixed $bf_FOO() calling to work even for wizards.  Release 1.8.0p2.
 *
 * Revision 2.3  1996/03/10  01:19:49  pavel
 * Added support for calling $bf_FOO() when FOO() is wiz-only.  Made
 * verbs on #0 exempt from the wiz-only check.  Release 1.8.0.
 *
 * Revision 2.2  1996/02/08  07:03:21  pavel
 * Renamed err/logf() to errlog/oklog() and TYPE_NUM to TYPE_INT.  Added
 * support for TYPE_NUMERIC wildcard.  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.1  1996/01/16  07:27:37  pavel
 * Added `function_info()' built-in function.  Release 1.8.0alpha6.
 *
 * Revision 2.0  1995/11/30  04:23:49  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.14  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.13  1992/10/23  19:25:30  pavel
 * Eliminated all uses of the useless macro NULL.
 *
 * Revision 1.12  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.11  1992/10/17  20:31:03  pavel
 * Changed return-type of read_bi_func_data() from char to int, for systems
 * that use unsigned chars.
 * Global rename of strdup->str_dup, strref->str_ref, vardup->var_dup, and
 * varref->var_ref.
 * Fixed bug in register_common() that sometimes read the wrong number of
 * argument-types from the argument list.
 *
 * Revision 1.10  1992/09/08  22:02:46  pjames
 * Updated register_* list to call functions by their new names.
 *
 * Revision 1.9  1992/08/28  16:01:31  pjames
 * Changed myfree(*, M_STRING) to free_str(*).
 *
 * Revision 1.8  1992/08/14  01:20:25  pavel
 * Made it entirely clear how to add new MOO built-in functions to the server.
 *
 * Revision 1.7  1992/08/14  00:40:31  pavel
 * Added missing #include "my-stdarg.h"...
 *
 * Revision 1.6  1992/08/14  00:01:03  pavel
 * Converted to a typedef of `var_type' = `enum var_type'.
 *
 * Revision 1.5  1992/08/13  21:27:11  pjames
 * Added register_bi_functions() which calls all procedures which
 * register built in functions.  To add another such procedure, just add
 * it to the static list and to functions.h
 *
 * Revision 1.4  1992/08/12  01:48:42  pjames
 * Builtin functions may now have as many arguments as desired.
 *
 * Revision 1.3  1992/08/10  17:39:46  pjames
 * Changed registration method to use var_args.  Changed 'next' field to
 * be func_pc.
 *
 * Revision 1.2  1992/07/21  00:02:29  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS ident. string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
