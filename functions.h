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

   When a built-in function is first invoked, the corresponding
   bf_fname() declared using register_function() is called as follows:

     bf_name( arglist, 1, NULL, progr )

   progr, an object, indicates whose permissions we are running with.
   arglist, the list of arguments, has already been type-checked
   to the extent specified and carries a refcount (that this
   function now has the responsiblity to free_var())

   The return value, a 'package', specifies the immediate outcome
   (return something, raise error, abort, suspend...).  If this is
   something *other* than a (.kind=) BI_CALL *with* .pc nonzero,
   this built-in invocation is then deemed resolved.

   Returning BI_CALL -- make_call_pack(pc, vdata) -- means a
   subsidiary verb call has been set up (see execute:call_verb*),
   i.e., a new activation has been pushed with the vm jumped to the
   right place.

   Never return make_call_pack(1, ...).  1 is reserved.

   .pc zero -- tail_call_pack() -- means that the resolution of the
   verb call will *be* the resolution of this built-in invocation.
   Always use tail_call_pack() for this.

   Never return make_call_pack(0, non-NULL).

   Returning BI_CALL with .pc nonzero means bf_name() *will* be called
   again -- even in cases of external error or the task getting
   aborted -- as follows:

     bf_name( return_value, pc, vdata, progr )

   where return_value is from the verb call and carries a refcount,
   while pc and vdata are from the call package previously returned.
   bf_name() can be re-called as many times as you want and pc values
   other than 0 or 1 can be arbitrarily re-used.  The structure of
   vdata can depend on pc.

   [[[ There is currently no way to tell the difference between a
       normal verb call return of 0 and a verb call having done a
       raise/abort other than that, in the latter case, subsequent
       BI_CALL returns will immediately call bf_name() again,
       completing the sequence of bf_name() calls as if there had been
       no external error but not actually calling any other verbs.

       Meaning, when writing bf_name(), keep in mind that zero *might*
       be an error in which case further verb calls will not do what
       you expect, though, since verb calls are UserLand, you should
       never count on them doing anything in particular anyway.
   ]]].

   If vdata is ever non-NULL:

   (*) It must have been allocated with alloc_data() and must be
       reclaimed by free_data() before the last bf_name() return.

   (*) Use register_function_state() to declare import and export hooks
       so that vdata from not-yet-completed built-in function calls
       can be saved in db checkpoints.  Use register_function_dbio()
       for legacy database read and write hooks.

       [[[ Note that the read hook returns NULL to indicate failure,
           which then disallows bf_name() returning
           make_call_pack(nonzero,NULL) for this builtin at all
           (??? FIX ??? -- not a problem so far).
       ]]]

   (*) If *vdata is a structure with pointers to objects that are
       refcounted or otherwise needing to be reclaimed themselves
       prior to *vdata itself being reclaimed, then

       use register_function_free() to declare a destructor to do
       whatever is needed before the final free_data() so that those
       situations (e.g., task_queue destruction) where vdata is
       reclaimed outside of the bf_name() that created it do not leak
       memory.

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
typedef void    (*bf_free_type) (void *vdata);
/* `read' parses legacy database input and `write' is retained with that
 * callback ABI.  `export' returns an owned payload; `import' borrows its
 * payload for the call.
 */
typedef int     (*bf_export_type) (void *vdata, unsigned *version, Var *payload);
typedef void *  (*bf_import_type) (unsigned version, Var payload);

#define MAX_FUNC         256
#define FUNC_NOT_FOUND   MAX_FUNC
/* valid function numbers are 0 - 255, or a total of 256 of them.
   function number 256 is reserved for func_not_found signal.
   hence valid function numbers will fit in one byte but the
   func_not_found signal will not */

extern const char *name_func_by_num(unsigned);
extern unsigned number_func_by_name(const char *);

extern unsigned register_function(const char *, int, int, bf_type,...);

/* amends the previous register_function() call: */
extern void register_function_dbio(bf_read_type, bf_write_type);
extern void register_function_free(bf_free_type);
extern void register_function_state(bf_import_type, bf_export_type);

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
extern int export_bi_func_state(void *, Byte, unsigned *, Var *);
extern int import_bi_func_state(Byte, unsigned, Var, void **);
extern Byte *pc_for_bi_func_data(void);
extern void free_bi_func_data(Byte f_id, void *vdata);

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
