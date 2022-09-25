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

#include "config.h"
#include "db_io.h"
#include "decompile.h"
#include "eval_vm.h"
#include "execute.h"
#include "log.h"
#include "options.h"
#include "storage.h"
#include "structures.h"
#include "tasks.h"

/**** external functions ****/

vm
new_vm(int task_id, int stack_size)
{
    vm the_vm = mymalloc(sizeof(vmstruct), M_VM);

    the_vm->task_id = task_id;
    the_vm->activ_stack = mymalloc(sizeof(activation) * stack_size, M_VM);

    return the_vm;
}

void
free_vm(vm the_vm, int stack_too)
{
    int i;

    if (stack_too)
	for (i = the_vm->top_activ_stack; i >= 0; i--)
	    free_activation(&the_vm->activ_stack[i], 1);
    myfree(the_vm->activ_stack, M_VM);
    myfree(the_vm, M_VM);
}

activation
top_activ(vm the_vm)
{
    return the_vm->activ_stack[the_vm->top_activ_stack];
}

Objid
progr_of_cur_verb(vm the_vm)
{
    return top_activ(the_vm).progr;
}

unsigned
suspended_lineno_of_vm(vm the_vm)
{
    activation top;

    top = top_activ(the_vm);
    return find_line_number(top.prog, (the_vm->top_activ_stack == 0
				       ? the_vm->root_activ_vector
				       : MAIN_VECTOR),
			    top.error_pc);
}

/**** read/write data base ****/

void
write_vm(vm the_vm)
{
    unsigned i;

    dbio_printf("%u %d %u %u\n",
		the_vm->top_activ_stack, the_vm->root_activ_vector,
		the_vm->func_id, the_vm->max_stack_size);

    for (i = 0; i <= the_vm->top_activ_stack; i++)
	write_activ(the_vm->activ_stack[i]);
}

vm
read_vm(int task_id)
{
    unsigned i, top, func_id, max;
    int vector;
    char c;
    vm the_vm;

    if (dbio_scanf("%u %d %u%c", &top, &vector, &func_id, &c) != 4
	|| (c == ' '
	    ? dbio_scanf("%u%c", &max, &c) != 2 || c != '\n'
	    : (max = DEFAULT_MAX_STACK_DEPTH, c != '\n'))) {
	errlog("READ_VM: Bad vm header\n");
	return 0;
    }
    the_vm = new_vm(task_id, top + 1);
    the_vm->max_stack_size = max;
    the_vm->top_activ_stack = top;
    the_vm->root_activ_vector = vector;
    the_vm->func_id = func_id;

    for (i = 0; i <= top; i++)
	if (!read_activ(&the_vm->activ_stack[i],
			i == 0 ? vector : MAIN_VECTOR)) {
	    errlog("READ_VM: Bad activ number %d\n", i);
	    return 0;
	}
    return the_vm;
}

char rcsid_eval_vm[] = "$Id$";

/*
 * $Log$
 * Revision 1.4  2002/08/18 09:47:26  bjj
 * Finally made free_activation() take a pointer after noticing how !$%^&
 * much time it was taking in a particular profiling run.
 *
 * Revision 1.3  1998/12/14 13:17:46  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:36  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:44:59  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.2  1996/02/08  07:12:16  pavel
 * Renamed err/logf() to errlog/oklog().  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.1  1995/12/31  03:19:43  pavel
 * Added missing #include "options.h".  Release 1.8.0alpha4.
 *
 * Revision 2.0  1995/11/30  04:22:07  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.10  1993/12/21  23:40:11  pavel
 * Add new `outcome' result from the interpreter.
 *
 * Revision 1.9  1993/08/11  06:03:12  pavel
 * Fixed mistaken %d->%u change.
 *
 * Revision 1.8  1993/08/11  02:58:03  pavel
 * Changed some bogus %d's to %u's in calls to *scanf() and *printf(), guided
 * by warnings from GCC...
 *
 * Revision 1.7  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.6  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.5  1992/10/17  20:25:59  pavel
 * Changed return-type of read_vm() from `char' to `int', for systems that use
 * unsigned chars.
 *
 * Revision 1.4  1992/09/25  21:12:12  pjames
 * Passed correct pc (error_pc) to get_lineno.
 *
 * Revision 1.3  1992/09/02  18:43:11  pavel
 * Added mechanism for resuming a read()ing task with an error.
 *
 * Revision 1.2  1992/08/31  22:22:24  pjames
 * Changed some `char *'s to `const char *'
 *
 * Revision 1.1  1992/08/10  16:20:00  pjames
 * Initial RCS-controlled version
 */
