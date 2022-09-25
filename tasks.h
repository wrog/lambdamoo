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

#ifndef Tasks_H
#define Tasks_H 1

#include "config.h"
#include "execute.h"
#include "structures.h"

typedef struct {
    void *ptr;
} task_queue;

extern task_queue new_task_queue(Objid player, Objid handler);
extern void free_task_queue(task_queue q);

extern int tasks_connection_option(task_queue, const char *,
				   Var *);
extern Var tasks_connection_options(task_queue, Var);
extern int tasks_set_connection_option(task_queue, const char *,
				       Var);

extern void new_input_task(task_queue, const char *, int);
extern void task_suspend_input(task_queue);
extern enum error enqueue_forked_task2(activation a, int f_index,
			       unsigned after_seconds, int vid);
extern enum error enqueue_suspended_task(vm the_vm, void *data);
				/* data == &(int after_seconds) */
extern enum error make_reading_task(vm the_vm, void *data);
				/* data == &(Objid connection) */
extern void resume_task(vm the_vm, Var value);
				/* Make THE_VM (a suspended task) runnable on
				 * the appropriate task queue; when it resumes
				 * execution, return VALUE from the built-in
				 * function that suspended it.  If VALUE.type
				 * is TYPE_ERR, then VALUE is raised instead of
				 * returned.
				 */
extern vm find_suspended_task(int id);

/* External task queues:

 * The registered enumerator should apply the given closure to every VM in the
 * external queue for as long as the closure returns TEA_CONTINUE, also passing
 * it a short string describing the current state of the VM (relative to that
 * queue) and the (void *) datum passed to the enumerator.  If the closure
 * returns TEA_KILL, then the task associated with that VM has been killed and
 * the VM freed; the external queue should clean up any local state associated
 * with the queue.  If the closure returns TEA_STOP or TEA_KILL, the enumerator
 * should immediately return without applying the closure to any further VMs.
 * The enumerator should return the same value as was returned by the last
 * invocation of the closure, or TEA_CONTINUE if there were no VMs in the
 * queue.
 */
typedef enum {
    TEA_CONTINUE,		/* Enumerator should continue with next VM */
    TEA_STOP,			/* Enumerator should stop now */
    TEA_KILL			/* Enumerator should stop and forget VM */
} task_enum_action;

typedef task_enum_action(*task_closure) (vm, const char *, void *);
typedef task_enum_action(*task_enumerator) (task_closure, void *);
extern void register_task_queue(task_enumerator);

extern Var read_input_now(Objid connection);

extern int next_task_start(void);
extern void run_ready_tasks(void);
extern enum outcome run_server_task(Objid player, Objid what,
				    const char *verb, Var args,
				    const char *argstr, Var * result);
extern enum outcome run_server_task_setting_id(Objid player, Objid what,
					       const char *verb, Var args,
					       const char *argstr,
					     Var * result, int *task_id);
extern enum outcome run_server_program_task(Objid this, const char *verb,
					    Var args, Objid vloc,
					    const char *verbname,
					  Program * program, Objid progr,
					    int debug, Objid player,
					    const char *argstr,
					    Var * result);

extern int current_task_id;
extern int last_input_task_id(Objid player);

extern void write_task_queue(void);
extern int read_task_queue(void);

extern db_verb_handle find_verb_for_programming(Objid player,
						const char *verbref,
						const char **message,
						const char **vname);

#endif				/* !Tasks_H */

/*
 * $Log$
 * Revision 1.5  2004/05/22 01:25:44  wrog
 * merging in WROGUE changes (W_SRCIP, W_STARTUP, W_OOB)
 *
 * Revision 1.4  2003/06/12 18:16:57  bjj
 * Suspend input on connection until :do_login_command() can run.
 *
 * Revision 1.3.10.1  2003/06/11 10:40:17  wrog
 * added binary argument to new_input_task()
 *
 * Revision 1.3  1998/12/14 13:19:08  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2.2.1  1998/12/06 07:13:23  bjj
 * Rationalize enqueue_forked_task interface and fix program_ref leak in
 * the case where fork fails with E_QUOTA.  Make .queued_task_limit=0 really
 * enforce a limit of zero tasks (for old behavior set it to 1, that's the
 * effect it used to have).
 *
 * Revision 1.2  1997/03/03 04:19:32  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.5  1996/03/19  07:10:22  pavel
 * Added run_server_program_task() for use from emergency mode.
 * Release 1.8.0p2.
 *
 * Revision 2.4  1996/03/10  01:14:59  pavel
 * Added support for `connection_option()'.  Release 1.8.0.
 *
 * Revision 2.3  1996/02/08  06:10:19  pavel
 * Added support for tasks-module connection options, displacing
 * set_input_task_holding().  Renamed enqueue_input_task() to
 * new_input_task().  Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.2  1995/12/31  03:20:51  pavel
 * Added handler argument to new_task_queue(), to support multiple listening
 * points.  Release 1.8.0alpha4.
 *
 * Revision 2.1  1995/12/11  08:00:58  pavel
 * Removed another silly use of `unsigned'.  Added `find_suspended_task()'.
 * Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  04:56:05  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.5  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.4  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.3  1992/09/08  21:56:27  pjames
 * Removed some procedures which are no longer external.
 *
 * Revision 1.2  1992/08/10  16:48:00  pjames
 * Changed Parse_Info to activation in enqueue_forked_task argument list,
 * because all important information is now stored in activation.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
