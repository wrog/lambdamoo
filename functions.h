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

#ifndef Functions_H
#define Functions_H 1

#include "config.h"

#include "my-stdio.h"

#include "execute.h"
#include "program.h"
#include "structures.h"

/******************************************
 |  HOW BUILT-IN FUNCTIONS WORK
 |  (+ rules to follow to prevent leaks):
 +-----------------------------------------
 *
 *  Invoking a built-in function type-checks the arguments
 *  according to the register_function() description,
 *  then calls the registered bf_NAME() function as follows:
 *
 *    bf_NAME( arglist, pc = 1, vdata = NULL, caller_perms )
 *
 *  Here, arglist carries a refcount that bf_NAME() needs to
 *  dispose of, meaning free_var(arglist) has to happen somewhere.
 *
 *  This must return a 'package' specifying the immediate outcome:
 *    value returned, error raised, nested call, aborted, or suspended.
 *
 *  For BI_CALL (nested call) -- make_call_pack(pc, vdata) -- bf_NAME
 *  is expected to have set up a nested verb call (see call_verb* in
 *  execute.c), i.e., leaving the new activation pushed and the vm
 *  jumped to the start of the verb code.
 *
 *  BI_CALL with .pc == 0 is a "tail call"
 *  -- always use tail_call_pack() for this --
 *  in which case whatever the nested call returns/raises
 *  is what the built-in itself will return/raise.
 *
 *  For all other nested calls, bf_NAME() is repeatedly called until
 *  the outcome is either a tail call or *not* a nested call [i].
 *
 *  Never do
 *    make_call_pack(0, vdata != NULL)
 *      no opportunity to reclaim vdata -> leak.
 *    make_call_pack(1, ...)
 *      pc == 1 is reserved for the first bf_NAME call.
 *
 *  Subsequent calls to bf_NAME() will be of the form
 *
 *    bf_NAME( return_value, pc, vdata, caller_perms )
 *
 *  where return_value is from the prior nested call (== 0 on error or
 *  abort) and also carries a refcount.  All nonzero values indicate a
 *  normal return, but it is not currently possible to disinguish a
 *  zero return from the nested call having errored/aborted.
 *
 *  pc and vdata are from the prior bf_NAME() return package.
 *  Use/re-use of pc values other than 0 or 1 is unrestricted,
 *  but vdata must be non-NULL in those cases [ii,iii]
 *
 *  (*) Use alloc_data() to create vdata; reclaim using free_data()
 *      before the final bf_NAME() return for this built-in
 *      invocation
 *
 *  (*) Use register_function_dbio() to declare read and write hooks
 *      so that vdata from not-yet-completed built-in function calls
 *      can be saved in db checkpoint files [iii]
 *
 *  Notes:
 *
 *  [i] Currently, the sequence of bf_NAME() calls completes even if a
 *      nested call raises an error or aborts the task -- for nested
 *      calls after the error/abort the bf_NAME() calls still happen
 *      but the nested calls themselves are skipped
 *      (??? FIX ??? -- do we really still need to do this?)
 *
 *  [ii] ... unless this is database version 3 (DBV_FLOAT), in which
 *      case pc==2 && vdata==NULL is allowed as a special bugfix that
 *      gets immediately patched to be pc==0 and saved that way
 *      in version 4+.
 *
 *  [iii] Read hook returning NULL to indicate failure is the main
 *      reason we can't yet allow make_call_pack(2+,NULL)
 *      (??? FIX ??? -- not a problem so far).
 *
 **********************/

/*------------------*
 |  struct package  |
 *------------------*/

enum abort_reason {
    ABORT_KILL    = -1, 	/* kill_task(task_id()) */
    ABORT_SECONDS = 0,		/* out of seconds */
    ABORT_TICKS   = 1		/* out of ticks */
};

typedef struct {
    enum {
	BI_RETURN,		/* Normal function return */
	BI_RAISE,		/* Raising an error */
	BI_CALL,		/* Making a nested verb call */
	BI_SUSPEND,		/* Suspending the current task */
	BI_ABORT		/* Aborting the current task */
    } kind;
    union {
	Var ret;
	struct {
	    Var code;
	    const char *msg;
	    Var value;
	} raise;
	struct {
	    Byte pc;
	    void *data;
	} call;
	struct {
	    enum error (*proc) (vm, void *);
	    void *data;
	} susp;
	enum abort_reason why;
    } u;
} package;

package make_abort_pack(enum abort_reason reason);
package make_error_pack(enum error err);
package make_raise_pack(enum error err, const char *msg, Var value);
package make_var_pack(Var v);
package make_int_pack(Num v);
package make_float_pack(FlNum v);
package make_string_pack(const char *s);
package no_var_pack(void);
package make_call_pack(Byte pc, void *vdata);
package tail_call_pack(void);
package make_suspend_pack(enum error (*) (vm, void *), void *);
package make_space_pack(void);

/*----------------*
 |  registration  |
 *----------------*/

typedef package (*bf_type) (Var, Byte, void *, Objid);
typedef void    (*bf_write_type) (void *vdata);
typedef void *  (*bf_read_type) (void);

#define MAX_FUNC         256
#define FUNC_NOT_FOUND   MAX_FUNC
/* valid function numbers are 0 - 255, or a total of 256 of them.
   function number 256 is reserved for func_not_found signal.
   hence valid function numbers will fit in one byte but the
   func_not_found signal will not */

extern const char *name_func_by_num(unsigned);
extern unsigned number_func_by_name(const char *);

extern unsigned register_function(const char *, int, int, bf_type,...);
extern unsigned register_function_with_read_write(const char *, int, int,
						  bf_type, bf_read_type,
						  bf_write_type,...);

/*--------------*
 |  invocation  |
 *--------------*/

extern package call_bi_func(unsigned, Var, Byte, Objid, void *);
/* will free or use Var arglist */

/*-----------------*
 |  serialization  |
 *-----------------*/

extern void write_bi_func_data(Byte f_id, void *vdata);
extern int read_bi_func_data(Byte f_id, void **vdata, Byte *pc);
extern Byte *pc_for_bi_func_data(void);

/*--------------*
 |  protection  |
 *--------------*/

extern void load_server_options(void);

#endif		/* !Functions_H */

/*
 * $Log$
 * Revision 2.2  1996/04/19  01:22:04  pavel
 * Added tail_call_pack() declaration and patches to allow generation of the
 * new warning in read_bi_func_data().  Release 1.8.0p4.
 *
 * Revision 2.1  1996/02/08  06:25:15  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:51:31  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.8  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.7  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.6  1992/10/17  20:31:21  pavel
 * Changed return-type of read_bi_func_data() from char to int, for systems
 * that use unsigned chars.
 *
 * Revision 1.5  1992/08/14  00:00:56  pavel
 * Converted to a typedef of `var_type' = `enum var_type'.
 *
 * Revision 1.4  1992/08/13  21:25:52  pjames
 * Added register_bi_functions() which registers all bi_functions.
 *
 * Revision 1.3  1992/08/12  01:49:19  pjames
 * Var_types in bft_entry is now a pointer instead of a preallocated array.
 *
 * Revision 1.2  1992/08/10  17:38:21  pjames
 * Added func_pc and func_data to package struct.  Built in functions now
 * receive an Objid (progr) instead of a Parse_Info.  Changed
 * registration method to use var_args.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
