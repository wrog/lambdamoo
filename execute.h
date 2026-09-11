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

#ifndef Execute_H
#define Execute_H 1

#include "config.h"

#include "db.h"
#include "opcode.h"
#include "parse_cmd.h"
#include "program.h"
#include "structures.h"
#include "waif.h"

#define ANY_RESUME_VECTOR (-2)

typedef struct {
    Program *prog;
    Var *rt_env;		/* same length as prog.var_names */
    Var *base_rt_stack;
    Var *top_rt_stack;		/* the stack has a fixed size equal to
				   vector.max_stack.  top_rt_stack
				   always points to next empty slot;
				   there is no need to check bounds! */
    int rt_stack_size;		/* size of stack allocated */
    unsigned pc;
    unsigned error_pc;
    ResumeKey resume_key;
    Byte bi_func_pc;		/* next == 0 means a normal activation, which just
				   returns to the previous activation (caller verb).
				   next == 1, 2, 3, ... means the returned value should be
				   fed to the bi_func (as specified in bi_func_id)
				   together with the next code. */
    Byte bi_func_id;
    void *bi_func_data;
    Var temp;			/* VM's temp register */

    /* verb information */
#ifdef WAIF_CORE
    /* waifs mean there can be other values for THIS, and we need a secure
     * way to store it so the verb can't spoof
     */
    Var THIS;
#endif
    Objid this;
    Objid player;
    Objid progr;
    Objid vloc;
    const char *verb;
    const char *verbname;
    int debug;
} activation;
#define BQM_DESCRIBE_activation(B,F,V,X)   ((4 * F) + (17 * V) + X(WAIF_CORE, B(Var)))

extern void free_activation(activation *, char data_too);

typedef struct {
    TaskID task_id;
    activation *activ_stack;
    unsigned max_stack_size;
    unsigned top_activ_stack;
    int root_activ_vector;
    /* root_activ_vector == MAIN_VECTOR
       means root activation is main_vector */
    unsigned func_id;
} vmstruct;
#define BQM_DESCRIBE_vmstruct(B,F,V,X)   ((4 * F) + (2 * V))

typedef vmstruct *vm;

#define alloc_data(size)   mymalloc(size, M_BI_FUNC_DATA)
#define free_data(ptr)     myfree((void *) ptr, M_BI_FUNC_DATA)

/* call_verb will only return E_MAXREC, E_INVIND, E_VERBNF,
   or E_NONE.  the vm will only be changed if E_NONE is returned */
extern enum error call_verb(Objid obj, const char *vname, Var args,
			    int do_pass);
/* if your vname is already a moo str (via str_dup) then you can
   use this interface instead */
extern enum error call_verb2(Objid obj, const char *vname
			     WAIF_COMMA_ARG(Var THIS),
			     Var args, int do_pass);

extern int setup_activ_for_eval(Program * prog);

enum outcome {
    OUTCOME_DONE,		/* Task ran successfully to completion */
    OUTCOME_ABORTED,		/* Task aborted, either by kill_task() or
				 * by an uncaught error. */
    OUTCOME_BLOCKED		/* Task called a blocking built-in function. */
};

extern enum outcome do_forked_task(Program * prog, Var * rt_env,
				   activation a, int f_id);
extern enum outcome do_input_task(Objid user, Parsed_Command * pc,
				  Objid this, db_verb_handle vh);
extern enum outcome do_server_verb_task(Objid this, const char *verb,
					Var args, db_verb_handle h,
					Objid player, const char *argstr,
					Var * result, int do_db_tracebacks);
extern enum outcome do_server_program_task(Objid this, const char *verb,
					   Var args, Objid vloc,
					   const char *verbname,
					   Program * program, Objid progr,
					   int debug, Objid player,
					   const char *argstr,
					   Var * result,
					   int do_db_tracebacks);
extern enum outcome resume_from_previous_vm(vm the_vm, Var value);

extern int task_timed_out;
extern void abort_running_task(void);
extern void print_error_backtrace(const char *, void (*)(const char *));
extern void output_to_log(const char *);
extern Objid caller(void);

extern void write_activ_as_pi(activation);
extern int read_activ_as_pi(activation *);
void write_rt_env(const char **var_names, Var * rt_env,
		  unsigned size);
int read_rt_env(const char ***old_names, Var ** rt_env,
		unsigned *old_size);
Var *reorder_rt_env(Var * old_rt_env, const char **old_names,
		    unsigned old_size, Program * prog);
extern void write_activ(activation a, int which_vector);
extern int read_activ(activation * a, int which_vector);

#endif		/* !Execute_H */

/*
 * $Log$
 * Revision 2.5  1996/03/10  01:20:08  pavel
 * Added new `caller()' entry point, for use by built-in fns.  Release 1.8.0.
 *
 * Revision 2.4  1996/02/08  06:25:47  pavel
 * Added support for in-DB traceback handling.  Updated copyright notice for
 * 1996.  Release 1.8.0beta1.
 *
 * Revision 2.3  1996/01/11  07:46:58  pavel
 * Added support for getting the value of a resumed task.
 * Release 1.8.0alpha5.
 *
 * Revision 2.2  1995/12/31  03:25:34  pavel
 * Removed extraneous #include "options.h".  Release 1.8.0alpha4.
 *
 * Revision 2.1  1995/12/11  07:58:55  pavel
 * Removed another silly use of `unsigned'.
 *
 * Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  04:51:15  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.10  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.9  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.8  1992/10/17  20:29:09  pavel
 * Changed return-type of read_activ() from char to int, for systems that use
 * unsigned chars.
 *
 * Revision 1.7  1992/09/25  21:11:03  pjames
 * Added error_pc to the activation data structure.
 *
 * Revision 1.6  1992/09/24  16:43:35  pavel
 * Added `task_timed_out' to the interface.
 *
 * Revision 1.5  1992/09/02  18:42:25  pavel
 * Fixed resume_from_previous_vm() to accept any MOO value as the resumption
 * value, instead of just strings.
 *
 * Revision 1.4  1992/08/31  22:27:38  pjames
 * Changed some `char *'s to `const char *'
 *
 * Revision 1.3  1992/08/10  17:44:28  pjames
 * Moved several functions declarations to eval_env.h and eval_vm.h
 *
 * Revision 1.2  1992/07/27  18:01:45  pjames
 * Changed name of ct_env to var_names
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 * */
