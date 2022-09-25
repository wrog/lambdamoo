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

#include "my-stdarg.h"

#include "bf_register.h"
#include "config.h"
#include "db_io.h"
#include "functions.h"
#include "list.h"
#include "log.h"
#include "server.h"
#include "storage.h"
#include "streams.h"
#include "structures.h"
#include "unparse.h"
#include "utils.h"

/*****************************************************************************
 * This is the table of procedures that register MOO built-in functions.  To
 * add new built-in functions to the server, add to the list below the name of
 * a C function that will register your new MOO built-ins; your C function will
 * be called exactly once, during server initialization.  Also add a
 * declaration of that C function to `bf_register.h' and add the necessary .c
 * files to the `CSRCS' line in the Makefile.
 ****************************************************************************/

typedef void (*registry) ();

static registry bi_function_registries[] =
{
    register_disassemble,
    register_extensions,
    register_execute,
    register_functions,
    register_list,
    register_log,
    register_numbers,
    register_objects,
    register_property,
    register_server,
    register_tasks,
    register_verbs
};

void
register_bi_functions()
{
    int loop, num_registries =
    sizeof(bi_function_registries) / sizeof(bi_function_registries[0]);

    for (loop = 0; loop < num_registries; loop++)
	(void) (*(bi_function_registries[loop])) ();
}

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
    int protected;
};

static struct bft_entry bf_table[MAX_FUNC];
static unsigned top_bf_table = 0;

static unsigned
register_common(const char *name, int minargs, int maxargs, bf_type func,
		bf_read_type read, bf_write_type write, va_list args)
{
    int va_index;
    int num_arg_types = maxargs == -1 ? minargs : maxargs;
    static Stream *s = 0;

    if (!s)
	s = new_stream(30);

    if (top_bf_table == MAX_FUNC) {
	errlog("too many functions.  %s cannot be registered.\n", name);
	return 0;
    }
    bf_table[top_bf_table].name = str_dup(name);
    stream_printf(s, "protect_%s", name);
    bf_table[top_bf_table].protect_str = str_dup(reset_stream(s));
    stream_printf(s, "bf_%s", name);
    bf_table[top_bf_table].verb_str = str_dup(reset_stream(s));
    bf_table[top_bf_table].minargs = minargs;
    bf_table[top_bf_table].maxargs = maxargs;
    bf_table[top_bf_table].func = func;
    bf_table[top_bf_table].read = read;
    bf_table[top_bf_table].write = write;
    bf_table[top_bf_table].protected = 0;

    if (num_arg_types > 0)
	bf_table[top_bf_table].prototype =
	    mymalloc(num_arg_types * sizeof(var_type), M_PROTOTYPE);
    else
	bf_table[top_bf_table].prototype = 0;
    for (va_index = 0; va_index < num_arg_types; va_index++)
	bf_table[top_bf_table].prototype[va_index] = va_arg(args, var_type);

    return top_bf_table++;
}

unsigned
register_function(const char *name, int minargs, int maxargs,
		  bf_type func,...)
{
    va_list args;
    unsigned ans;

    va_start(args, func);
    ans = register_common(name, minargs, maxargs, func, 0, 0, args);
    va_end(args);
    return ans;
}

unsigned
register_function_with_read_write(const char *name, int minargs, int maxargs,
				  bf_type func, bf_read_type read,
				  bf_write_type write,...)
{
    va_list args;
    unsigned ans;

    va_start(args, write);
    ans = register_common(name, minargs, maxargs, func, read, write, args);
    va_end(args);
    return ans;
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
	/* if (caller() != SYSTEM_OBJECT && server_flag_option(f->protect_str)) { */
	if (caller() != SYSTEM_OBJECT && f->protected) {
	    /* Try calling #0:bf_FUNCNAME(@ARGS) instead */
	    enum error e = call_verb2(SYSTEM_OBJECT, f->verb_str, arglist, 0);

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
write_bi_func_data(void *vdata, Byte f_id)
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


package
make_kill_pack()
{
    package p;

    p.kind = BI_KILL;

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
make_float_pack(double v)
{
    package p;

    p.kind = BI_RETURN;
    p.u.ret.type = TYPE_FLOAT;
    p.u.ret.v.fnum = v;

    return p;
}

package
make_call_pack(Byte pc, void *data)
{
    package p;

    p.kind = BI_CALL;
    p.u.call.pc = pc;
    p.u.call.data = data;

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
bf_function_info(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    int i;

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
load_server_protect_flags(void)
{
    int i;

    for (i = 0; i < top_bf_table; i++) {
	bf_table[i].protected = server_flag_option(bf_table[i].protect_str);
    }
    oklog("Loaded protect cache for %d builtins\n", i);
}

void
load_server_options(void)
{
    load_server_protect_flags();
}

static package
bf_load_server_options(Var arglist, Byte next, void *vdata, Objid progr)
{
    free_var(arglist);

    if (!is_wizard(progr)) {
	return make_error_pack(E_PERM);
    }
    load_server_options();

    return no_var_pack();
}

void
register_functions(void)
{
    register_function("function_info", 0, 1, bf_function_info, TYPE_STR);
    register_function("load_server_options", 0, 0, bf_load_server_options);
}

char rcsid_functions[] = "$Id$";

/*
 * $Log$
 * Revision 1.7  2001/03/12 05:10:54  bjj
 * Split out call_verb and call_verb2.  The latter must only be called with
 * strings that are already MOO strings (str_ref-able).
 *
 * Revision 1.6  2001/03/12 03:25:16  bjj
 * Added new package type BI_KILL which kills the task calling the builtin.
 * Removed the static int task_killed in execute.c which wa tested on every
 * loop through the interpreter to see if the task had been killed.
 *
 * Revision 1.5  1998/12/14 13:17:53  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.4  1997/07/07 03:24:54  nop
 * Merge UNSAFE_OPTS (r5) after extensive testing.
 *
 * Revision 1.3.2.2  1997/05/12 04:03:21  bjj
 * This time for sure!
 *
 * Revision 1.3.2.1  1997/05/11 04:31:54  bjj
 * Missed the place in bf_function_info where TYPE_* constants make it into
 * the database.  Masked off the complex flag in the obvious place.
 *
 * Revision 1.3  1997/03/03 05:03:50  nop
 * steak2: move protectedness into builtin struct, load_server_options()
 * now required for $server_options updates.
 *
 * Revision 1.2  1997/03/03 04:18:42  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:00  nop
 * LambdaMOO 1.8.0p5
 *
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
