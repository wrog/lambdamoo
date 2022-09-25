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

#ifndef Functions_h
#define Functions_h 1

#include "my-stdio.h"

#include "config.h"
#include "execute.h"
#include "program.h"
#include "structures.h"

typedef struct {
    enum {
	BI_RETURN,		/* Normal function return */
	BI_RAISE,		/* Raising an error */
	BI_CALL,		/* Making a nested verb call */
	BI_SUSPEND,		/* Suspending the current task */
	BI_KILL			/* Kill the current task */
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
    } u;
} package;

void register_bi_functions(void);

package make_kill_pack(void);
package make_error_pack(enum error err);
package make_raise_pack(enum error err, const char *msg, Var value);
package make_var_pack(Var v);
package make_int_pack(Num v);
package make_float_pack(double v);
package no_var_pack(void);
package make_call_pack(Byte pc, void *data);
package tail_call_pack(void);
package make_suspend_pack(enum error (*)(vm, void *), void *);

typedef package(*bf_type) (Var, Byte, void *, Objid);
typedef void (*bf_write_type) (void *vdata);
typedef void *(*bf_read_type) (void);

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

extern package call_bi_func(unsigned, Var, Byte, Objid, void *);
/* will free or use Var arglist */

extern void write_bi_func_data(void *vdata, Byte f_id);
extern int read_bi_func_data(Byte f_id, void **bi_func_state,
			     Byte * bi_func_pc);
extern Byte *pc_for_bi_func_data(void);

extern void load_server_options(void);

#endif

/*
 * $Log$
 * Revision 1.5  2001/03/12 03:25:16  bjj
 * Added new package type BI_KILL which kills the task calling the builtin.
 * Removed the static int task_killed in execute.c which wa tested on every
 * loop through the interpreter to see if the task had been killed.
 *
 * Revision 1.4  1998/12/14 13:17:54  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.3  1997/03/03 05:03:51  nop
 * steak2: move protectedness into builtin struct, load_server_options()
 * now required for $server_options updates.
 *
 * Revision 1.2  1997/03/03 04:18:42  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:03  nop
 * LambdaMOO 1.8.0p5
 *
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
