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

#include "my-types.h"		/* must be first on some systems */
#include "my-signal.h"
#include "my-stdarg.h"
#include "my-stdio.h"
#include "my-stdlib.h"
#include "my-string.h"
#include "my-unistd.h"
#include "my-wait.h"

#include "config.h"
#include "db.h"
#include "db_io.h"
#include "disassemble.h"
#include "execute.h"
#include "functions.h"
#include "list.h"
#include "log.h"
#include "network.h"
#include "options.h"
#include "parser.h"
#include "random.h"
#include "server.h"
#include "storage.h"
#include "streams.h"
#include "structures.h"
#include "tasks.h"
#include "timers.h"
#include "unparse.h"
#include "utils.h"
#include "version.h"

#include "execute.h"

static pid_t parent_pid;
int in_child = 0;

static const char *shutdown_message = 0;	/* shut down if non-zero */
static int in_emergency_mode = 0;
static Var checkpointed_connections;

typedef enum {
    CHKPT_OFF, CHKPT_TIMER, CHKPT_SIGNAL, CHKPT_FUNC
} Checkpoint_Reason;
static Checkpoint_Reason checkpoint_requested = CHKPT_OFF;

static int checkpoint_finished = 0;	/* 1 = failure, 2 = success */

typedef struct shandle {
    struct shandle *next, **prev;
    network_handle nhandle;
    time_t connection_time;
    time_t last_activity_time;
    Objid player;
    Objid listener;
    task_queue tasks;
    int disconnect_me;
    int outbound, binary;
    int print_messages;
} shandle;

static shandle *all_shandles = 0;

typedef struct slistener {
    struct slistener *next, **prev;
    network_listener nlistener;
    Objid oid;			/* listen(OID, DESC, PRINT_MESSAGES) */
    Var desc;
    int print_messages;
    const char *name;
} slistener;

static slistener *all_slisteners = 0;

server_listener null_server_listener = {0};

static void
free_shandle(shandle * h)
{
    *(h->prev) = h->next;
    if (h->next)
	h->next->prev = h->prev;

    free_task_queue(h->tasks);

    myfree(h, M_NETWORK);
}

static slistener *
new_slistener(Objid oid, Var desc, int print_messages, enum error *ee)
{
    slistener *l = mymalloc(sizeof(slistener), M_NETWORK);
    server_listener sl;
    enum error e;
    const char *name;

    sl.ptr = l;
    e = network_make_listener(sl, desc, &(l->nlistener), &(l->desc), &name);

    if (ee)
	*ee = e;

    if (e != E_NONE) {
	myfree(l, M_NETWORK);
	return 0;
    }
    l->oid = oid;
    l->print_messages = print_messages;
    l->name = str_dup(name);

    l->next = all_slisteners;
    l->prev = &all_slisteners;
    if (all_slisteners)
	all_slisteners->prev = &(l->next);
    all_slisteners = l;

    return l;
}

static int
start_listener(slistener * l)
{
    if (network_listen(l->nlistener)) {
	oklog("LISTEN: #%d now listening on %s\n", l->oid, l->name);
	return 1;
    } else {
	errlog("LISTEN: Can't start #%d listening on %s!\n", l->oid, l->name);
	return 0;
    }
}

static void
free_slistener(slistener * l)
{
    network_close_listener(l->nlistener);
    oklog("UNLISTEN: #%d no longer listening on %s\n", l->oid, l->name);

    *(l->prev) = l->next;
    if (l->next)
	l->next->prev = l->prev;

    free_var(l->desc);
    free_str(l->name);

    myfree(l, M_NETWORK);
}

static void
send_shutdown_message(const char *msg)
{
    shandle *h;
    Stream *s = new_stream(100);
    char *message;

    stream_printf(s, "*** Shutting down: %s ***", msg);
    message = stream_contents(s);
    for (h = all_shandles; h; h = h->next)
	network_send_line(h->nhandle, message, 1);
    free_stream(s);
}

static void
abort_server(void)
{
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
    signal(SIGILL, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
#ifdef SIGBUS
    signal(SIGBUS, SIG_DFL);
#endif
    signal(SIGUSR1, SIG_DFL);
    signal(SIGUSR2, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);

    abort();
}

void
panic(const char *message)
{
    static int in_panic = 0;

    errlog("PANIC%s: %s\n", in_child ? " (in child)" : "", message);
    if (in_panic) {
	errlog("RECURSIVE PANIC: aborting\n");
	abort_server();
    }
    in_panic = 1;

    log_command_history();

    if (in_child) {		/* We're a forked checkpointer */
	errlog("Child shutting down parent via USR1 signal\n");
	kill(parent_pid, SIGUSR1);
	_exit(1);
    }
    print_error_backtrace("server panic", output_to_log);
    send_shutdown_message("server panic");
    network_shutdown();
    db_flush(FLUSH_PANIC);

    abort_server();
}

enum Fork_Result
fork_server(const char *subtask_name)
{
    pid_t pid;
    Stream *s = new_stream(100);

    stream_printf(s, "Forking %s", subtask_name);
    pid = fork();
    if (pid < 0) {
	log_perror(stream_contents(s));
	free_stream(s);
	return FORK_ERROR;
    }
    free_stream(s);
    if (pid == 0) {
	in_child = 1;
	return FORK_CHILD;
    } else
	return FORK_PARENT;
}

static void
panic_signal(int sig)
{
    char message[100];

    sprintf(message, "Caught signal %d", sig);
    panic(message);
}

static void
shutdown_signal(int sig)
{
    shutdown_message = "shutdown signal received";
}

static void
checkpoint_signal(int sig)
{
    checkpoint_requested = CHKPT_SIGNAL;

    signal(sig, checkpoint_signal);
}

static void
call_checkpoint_notifier(int successful)
{
    Var args;

    args = new_list(1);
    args.v.list[1].type = TYPE_INT;
    args.v.list[1].v.num = successful;
    run_server_task(-1, SYSTEM_OBJECT, "checkpoint_finished", args, "", 0);
}

static void
child_completed_signal(int sig)
{
    int status;

    /* (Void *) casts to avoid warnings on systems that mis-declare the
     * argument type.
     */
#if HAVE_WAITPID
    while (waitpid(-1, (void *) &status, WNOHANG) > 0);
#else
#if HAVE_WAIT3
    while (wait3((void *) &status, WNOHANG, 0) >= 0);
#else
#if HAVE_WAIT2
    while (wait2((void *) &status, WNOHANG) >= 0);
#else
    wait((void *) &status);
#endif
#endif
#endif

    signal(sig, child_completed_signal);

    checkpoint_finished = (status == 0) + 1;	/* 1 = failure, 2 = success */
}

static void
setup_signals(void)
{
    signal(SIGFPE, SIG_IGN);
    if (signal(SIGHUP, panic_signal) == SIG_IGN)
	signal(SIGHUP, SIG_IGN);
    signal(SIGILL, panic_signal);
    signal(SIGQUIT, panic_signal);
    signal(SIGSEGV, panic_signal);
#ifdef SIGBUS
    signal(SIGBUS, panic_signal);
#endif

    signal(SIGINT, shutdown_signal);
    signal(SIGTERM, shutdown_signal);
    signal(SIGUSR1, shutdown_signal);	/* remote shutdown signal */
    signal(SIGUSR2, checkpoint_signal);		/* remote checkpoint signal */

    signal(SIGCHLD, child_completed_signal);
}

static void
checkpoint_timer(Timer_ID id, Timer_Data data)
{
    checkpoint_requested = CHKPT_TIMER;
}

static void
set_checkpoint_timer(int first_time)
{
    Var v;
    int interval, now = time(0);
    static Timer_ID last_checkpoint_timer;

    v = get_system_property("dump_interval");
    if (v.type != TYPE_INT || v.v.num < 60 || now + v.v.num < now) {
	free_var(v);
	interval = 3600;	/* Once per hour */
    } else
	interval = v.v.num;

    if (!first_time)
	cancel_timer(last_checkpoint_timer);
    last_checkpoint_timer = set_timer(interval, checkpoint_timer, 0);
}

static const char *
object_name(Objid oid)
{
    static Stream *s = 0;

    if (!s)
	s = new_stream(30);

    if (valid(oid))
	stream_printf(s, "%s (#%d)", db_object_name(oid), oid);
    else
	stream_printf(s, "#%d", oid);

    return reset_stream(s);
}

static void
call_notifier(Objid player, Objid handler, const char *verb_name)
{
    Var args;

    args = new_list(1);
    args.v.list[1].type = TYPE_OBJ;
    args.v.list[1].v.obj = player;
    run_server_task(player, handler, verb_name, args, "", 0);
}

int
get_server_option(Objid oid, const char *name, Var * r)
{
    if (((valid(oid) &&
	  db_find_property(oid, "server_options", r).ptr)
	 || (valid(SYSTEM_OBJECT) &&
	     db_find_property(SYSTEM_OBJECT, "server_options", r).ptr))
	&& r->type == TYPE_OBJ
	&& valid(r->v.obj)
	&& db_find_property(r->v.obj, name, r).ptr)
	return 1;

    return 0;
}

static void
send_message(Objid listener, network_handle nh, const char *msg_name,...)
{
    va_list args;
    Var msg;
    const char *line;

    va_start(args, msg_name);
    if (get_server_option(listener, msg_name, &msg)) {
	if (msg.type == TYPE_STR)
	    network_send_line(nh, msg.v.str, 1);
	else if (msg.type == TYPE_LIST) {
	    int i;

	    for (i = 1; i <= msg.v.list[0].v.num; i++)
		if (msg.v.list[i].type == TYPE_STR)
		    network_send_line(nh, msg.v.list[i].v.str, 1);
	}
    } else			/* Use default message */
	while ((line = va_arg(args, const char *)) != 0)
	    network_send_line(nh, line, 1);

    va_end(args);
}

static void
main_loop(void)
{
    int i;

    /* First, notify DB of disconnections for all checkpointed connections */
    for (i = 1; i <= checkpointed_connections.v.list[0].v.num; i++) {
	Var v;

	v = checkpointed_connections.v.list[i];
	call_notifier(v.v.list[1].v.obj, v.v.list[2].v.obj,
		      "user_disconnected");
    }
    free_var(checkpointed_connections);

    /* Second, run #0:server_started() */
    run_server_task(-1, SYSTEM_OBJECT, "server_started", new_list(0), "", 0);
    set_checkpoint_timer(1);

    /* Now, we enter the main server loop */
    while (shutdown_message == 0) {
	/* Check how long we have until the next task will be ready to run.
	 * We only care about three cases (== 0, == 1, and > 1), so we can
	 * map a `never' result from the task subsystem into 2.
	 */
	int task_seconds = next_task_start();
	int seconds_left = task_seconds < 0 ? 2 : task_seconds;
	shandle *h, *nexth;

	if (checkpoint_requested != CHKPT_OFF) {
	    if (checkpoint_requested == CHKPT_SIGNAL)
		oklog("CHECKPOINTING due to remote request signal.\n");
	    checkpoint_requested = CHKPT_OFF;
	    run_server_task(-1, SYSTEM_OBJECT, "checkpoint_started",
			    new_list(0), "", 0);
	    network_process_io(0);
#ifdef UNFORKED_CHECKPOINTS
	    call_checkpoint_notifier(db_flush(FLUSH_ALL_NOW));
#else
	    if (!db_flush(FLUSH_ALL_NOW))
		call_checkpoint_notifier(0);
#endif
	    set_checkpoint_timer(0);
	}
#ifndef UNFORKED_CHECKPOINTS
	if (checkpoint_finished) {
	    call_checkpoint_notifier(checkpoint_finished - 1);
	    checkpoint_finished = 0;
	}
#endif

	if (!network_process_io(seconds_left ? 1 : 0) && seconds_left > 1)
	    db_flush(FLUSH_ONE_SECOND);
	else
	    db_flush(FLUSH_IF_FULL);

	run_ready_tasks();

	{			/* Get rid of old un-logged-in or useless connections */
	    int now = time(0);

	    for (h = all_shandles; h; h = nexth) {
		Var v;

		nexth = h->next;

		if (!h->outbound && h->connection_time == 0
		    && (get_server_option(h->listener, "connect_timeout", &v)
			? (v.type == TYPE_INT && v.v.num > 0
			   && now - h->last_activity_time > v.v.num)
			: (now - h->last_activity_time
			   > DEFAULT_CONNECT_TIMEOUT))) {
		    call_notifier(h->player, h->listener, "user_disconnected");
		    oklog("TIMEOUT: #%d on %s\n",
			  h->player,
			  network_connection_name(h->nhandle));
		    if (h->print_messages)
			send_message(h->listener, h->nhandle, "timeout_msg",
				     "*** Timed-out waiting for login. ***",
				     0);
		    network_close(h->nhandle);
		    free_shandle(h);
		} else if (h->connection_time != 0 && !valid(h->player)) {
		    oklog("RECYCLED: #%d on %s\n",
			  h->player,
			  network_connection_name(h->nhandle));
		    if (h->print_messages)
			send_message(h->listener, h->nhandle,
				     "recycle_msg", "*** Recycled ***", 0);
		    network_close(h->nhandle);
		    free_shandle(h);
		} else if (h->disconnect_me) {
		    call_notifier(h->player, h->listener,
				  "user_disconnected");
		    oklog("DISCONNECTED: %s on %s\n",
			  object_name(h->player),
			  network_connection_name(h->nhandle));
		    if (h->print_messages)
			send_message(h->listener, h->nhandle, "boot_msg",
				     "*** Disconnected ***", 0);
		    network_close(h->nhandle);
		    free_shandle(h);
		}
	    }
	}
    }

    oklog("SHUTDOWN: %s\n", shutdown_message);
    send_shutdown_message(shutdown_message);
}

static shandle *
find_shandle(Objid player)
{
    shandle *h;

    for (h = all_shandles; h; h = h->next)
	if (h->player == player)
	    return h;

    return 0;
}

static char *cmdline_buffer;
static int cmdline_buflen;

static void
init_cmdline(int argc, char *argv[])
{
    char *p;
    int i;

    for (p = argv[0], i = 1;;) {
	if (*p++ == '\0' && (i >= argc || p != argv[i++]))
	    break;
    }

    cmdline_buffer = argv[0];
    cmdline_buflen = p - argv[0];
}

#define SERVER_CO_TABLE(DEFINE, H, VALUE, _)				\
    DEFINE(binary, _, TYPE_INT, num,					\
	   H->binary,							\
	   {								\
	       H->binary = is_true(VALUE);				\
	       network_set_connection_binary(H->nhandle, H->binary);	\
	   })								\

static int
server_set_connection_option(shandle * h, const char *option, Var value)
{
    CONNECTION_OPTION_SET(SERVER_CO_TABLE, h, option, value);
}

static int
server_connection_option(shandle * h, const char *option, Var * value)
{
    CONNECTION_OPTION_GET(SERVER_CO_TABLE, h, option, value);
}

static Var
server_connection_options(shandle * h, Var list)
{
    CONNECTION_OPTION_LIST(SERVER_CO_TABLE, h, list);
}

#undef SERVER_CO_TABLE

static char *
read_stdin_line()
{
    static Stream *s = 0;
    char *line, buffer[1000];
    int buflen;

    fflush(stdout);
    if (!s)
	s = new_stream(100);

    do {			/* Read even a very long line of input */
	fgets(buffer, sizeof(buffer), stdin);
	buflen = strlen(buffer);
	if (buflen == 0)
	    return 0;
	if (buffer[buflen - 1] == '\n') {
	    buffer[buflen - 1] = '\0';
	    buflen--;
	}
	stream_add_string(s, buffer);
    } while (buflen == sizeof(buffer) - 1);
    line = reset_stream(s);
    while (*line == ' ')
	line++;

    return line;
}

static void
emergency_notify(Objid player, const char *line)
{
    printf("#%d <- %s\n", player, line);
}

static int
emergency_mode()
{
    char *line;
    Var words;
    int nargs;
    const char *command;
    Stream *s = new_stream(100);
    Objid wizard = -1;
    int debug = 1;
    int start_ok = -1;

    oklog("EMERGENCY_MODE: Entering mode...\n");
    in_emergency_mode = 1;

    printf("\nLambdaMOO Emergency Holographic Wizard Mode\n");
    printf("-------------------------------------------\n");
    printf("\"Please state the nature of the wizardly emergency...\"\n");
    printf("(Type `help' for assistance.)\n\n");

    while (start_ok < 0) {
	/* Find/create a wizard to run commands as... */
	if (!is_wizard(wizard)) {
	    Objid first_valid = -1;

	    if (wizard >= 0)
		printf("** Object #%d is not a wizard...\n", wizard);

	    for (wizard = 0; wizard <= db_last_used_objid(); wizard++)
		if (is_wizard(wizard))
		    break;
		else if (valid(wizard) && first_valid < 0)
		    first_valid = wizard;

	    if (!is_wizard(wizard)) {
		if (first_valid < 0) {
		    first_valid = db_create_object();
		    printf("** No objects in database; created #%d.\n",
			   first_valid);
		}
		wizard = first_valid;
		db_set_object_flag(wizard, FLAG_WIZARD);
		printf("** No wizards in database; wizzed #%d.\n", wizard);
	    }
	    printf("** Now running emergency commands as #%d ...\n", wizard);
	}
	printf("\nMOO (#%d)%s: ", wizard, debug ? "" : "[!d]");
	line = read_stdin_line();

	if (!line)
	    start_ok = 0;	/* treat EOF as "quit" */
	else if (*line == ';') {	/* eval command */
	    Var code, errors;
	    Program *program;
	    Var str;

	    str.type = TYPE_STR;
	    code = new_list(0);

	    if (*++line == ';')
		line++;
	    else {
		str.v.str = str_dup("return");
		code = listappend(code, str);
	    }

	    while (*line == ' ')
		line++;

	    if (*line == '\0') {	/* long form */
		printf("Type one or more lines of code, ending with `.' ");
		printf("alone on a line.\n");
		for (;;) {
		    line = read_stdin_line();
		    if (!strcmp(line, "."))
			break;
		    else {
			str.v.str = str_dup(line);
			code = listappend(code, str);
		    }
		}
	    } else {
		str.v.str = str_dup(line);
		code = listappend(code, str);
	    }
	    str.v.str = str_dup(";");
	    code = listappend(code, str);

	    program = parse_list_as_program(code, &errors);
	    free_var(code);
	    if (program) {
		Var result;

		switch (run_server_program_task(NOTHING, "emergency_mode",
						new_list(0), NOTHING,
						"emergency_mode", program,
						wizard, debug, wizard, "",
						&result)) {
		case OUTCOME_DONE:
		    printf("=> %s\n", value_to_literal(result));
		    free_var(result);
		    break;
		case OUTCOME_ABORTED:
		    printf("=> *Aborted*\n");
		    break;
		case OUTCOME_BLOCKED:
		    printf("=> *Suspended*\n");
		    break;
		}
		free_program(program);
	    } else {
		int i;

		printf("** %d errors during parsing:\n",
		       errors.v.list[0].v.num);
		for (i = 1; i <= errors.v.list[0].v.num; i++)
		    printf("  %s\n", errors.v.list[i].v.str);
	    }
	    free_var(errors);
	} else {
	    words = parse_into_wordlist(line);
	    nargs = words.v.list[0].v.num - 1;
	    if (nargs < 0)
		continue;
	    command = words.v.list[1].v.str;

	    if ((!mystrcasecmp(command, "program")
		 || !mystrcasecmp(command, ".program"))
		&& nargs == 1) {
		const char *verbref = words.v.list[2].v.str;
		db_verb_handle h;
		const char *message, *vname;

		h = find_verb_for_programming(wizard, verbref,
					      &message, &vname);
		printf("%s\n", message);
		if (h.ptr) {
		    Var code, str, errors;
		    char *line;
		    Program *program;

		    code = new_list(0);
		    str.type = TYPE_STR;

		    while (strcmp(line = read_stdin_line(), ".")) {
			str.v.str = str_dup(line);
			code = listappend(code, str);
		    }

		    program = parse_list_as_program(code, &errors);
		    if (program) {
			db_set_verb_program(h, program);
			printf("Verb programmed.\n");
		    } else {
			int i;

			printf("** %d errors during parsing:\n",
			       errors.v.list[0].v.num);
			for (i = 1; i <= errors.v.list[0].v.num; i++)
			    printf("  %s\n", errors.v.list[i].v.str);
			printf("Verb not programmed.\n");
		    }

		    free_var(code);
		    free_var(errors);
		}
	    } else if (!mystrcasecmp(command, "list") && nargs == 1) {
		const char *verbref = words.v.list[2].v.str;
		db_verb_handle h;
		const char *message, *vname;

		h = find_verb_for_programming(wizard, verbref,
					      &message, &vname);
		if (h.ptr)
		    unparse_to_file(stdout, db_verb_program(h), 0, 1,
				    MAIN_VECTOR);
		else
		    printf("%s\n", message);
	    } else if (!mystrcasecmp(command, "disassemble") && nargs == 1) {
		const char *verbref = words.v.list[2].v.str;
		db_verb_handle h;
		const char *message, *vname;

		h = find_verb_for_programming(wizard, verbref,
					      &message, &vname);
		if (h.ptr)
		    disassemble_to_file(stdout, db_verb_program(h));
		else
		    printf("%s\n", message);
	    } else if (!mystrcasecmp(command, "abort") && nargs == 0) {
	        printf("Bye.  (%s)\n\n", "NOT saving database");
		exit(1);
	    } else if (!mystrcasecmp(command, "quit") && nargs == 0) {
		start_ok = 0;
	    } else if (!mystrcasecmp(command, "continue") && nargs == 0) {
		start_ok = 1;
	    } else if (!mystrcasecmp(command, "debug") && nargs == 0) {
		debug = !debug;
	    } else if (!mystrcasecmp(command, "wizard") && nargs == 1
		       && sscanf(words.v.list[2].v.str, "#%d", &wizard) == 1) {
		printf("** Switching to wizard #%d...\n", wizard);
	    } else {
		if (mystrcasecmp(command, "help")
		    && mystrcasecmp(command, "?"))
		    printf("** Unknown or malformed command.\n");

		printf(";EXPR                 "
		       "Evaluate MOO expression, print result.\n");
		printf(";;CODE	              "
		       "Execute whole MOO verb, print result.\n");
		printf("    (For above, omitting EXPR or CODE lets you "
		       "enter several lines\n");
		printf("     of input at once; type a period alone on a "
		       "line to finish.)\n");
		printf("program OBJ:VERB      "
		       "Set the MOO code of an existing verb.\n");
		printf("list OBJ:VERB         "
		       "List the MOO code of an existing verb.\n");
		printf("disassemble OBJ:VERB  "
		       "List the internal form of an existing verb.\n");
		printf("debug                 "
		       "Toggle evaluation with(out) `d' bit.\n");
		printf("wizard #XX            "
		       "Execute future commands as wizard #XX.\n");
		printf("continue              "
		       "End emergency mode, continue start-up.\n");
		printf("quit                  "
		       "Exit server normally, saving database.\n");
		printf("abort                 "
		       "Exit server *without* saving database.\n");
		printf("help, ?               "
		       "Print this text.\n\n");

		printf("NOTE: *NO* forked or suspended tasks will run "
		       "until you exit this mode.\n\n");
		printf("\"Please remember to turn me off when you go...\"\n");
	    }

	    free_var(words);
	}
    }

    printf("Bye.  (%s)\n\n", start_ok ? "continuing" : "saving database");
#if NETWORK_PROTOCOL != NP_SINGLE
    fclose(stdout);
#endif

    free_stream(s);
    in_emergency_mode = 0;
    oklog("EMERGENCY_MODE: Leaving mode; %s continue...\n",
	  start_ok ? "will" : "won't");
    return start_ok;
}


/*
 * Exported interface
 */

void
set_server_cmdline(const char *line)
{
    /* This technique works for all UNIX systems I've seen on which this is
     * possible to do at all, and it's safe on all systems.  The only systems
     * I know of where this doesn't work run System V Release 4, on which the
     * kernel keeps its own copy of the original cmdline for printing by the
     * `ps' command; on these systems, it would appear that there does not
     * exist a way to get around this at all.  Thus, this works everywhere I
     * know of where it's possible to do the job at all...
     */
    char *p = cmdline_buffer, *e = p + cmdline_buflen - 1;

    while (*line && p < e)
	*p++ = *line++;
    while (p < e)
	*p++ = ' ';		/* Pad with blanks, not nulls; on SunOS and
				 * maybe other systems, nulls would confuse
				 * `ps'.  (*sigh*)
				 */
    *e = '\0';
}

int
server_flag_option(const char *name)
{
    Var v;

    if (get_server_option(SYSTEM_OBJECT, name, &v))
	return is_true(v);
    else
	return 0;
}

int
server_int_option(const char *name, int defallt)
{
    Var v;

    if (get_server_option(SYSTEM_OBJECT, name, &v))
	return (v.type == TYPE_INT ? v.v.num : defallt);
    else
	return defallt;
}

const char *
server_string_option(const char *name, const char *defallt)
{
    Var v;

    if (get_server_option(SYSTEM_OBJECT, name, &v))
	return (v.type == TYPE_STR ? v.v.str : 0);
    else
	return defallt;
}

static Objid next_unconnected_player = NOTHING - 1;

server_handle
server_new_connection(server_listener sl, network_handle nh, int outbound)
{
    slistener *l = sl.ptr;
    shandle *h = (shandle *) mymalloc(sizeof(shandle), M_NETWORK);
    server_handle result;

    h->next = all_shandles;
    h->prev = &all_shandles;
    if (all_shandles)
	all_shandles->prev = &(h->next);
    all_shandles = h;

    h->nhandle = nh;
    h->connection_time = 0;
    h->last_activity_time = time(0);
    h->player = next_unconnected_player--;
    h->listener = l ? l->oid : SYSTEM_OBJECT;
    h->tasks = new_task_queue(h->player, h->listener);
    h->disconnect_me = 0;
    h->outbound = outbound;
    h->binary = 0;
    h->print_messages = l ? l->print_messages : !outbound;

    if (l || !outbound) {
	new_input_task(h->tasks, "", 0);
	/*
	 * Suspend input at the network level until the above input task
	 * is processed.  At the point when it is dequeued, tasks.c will
	 * notice that the queued input size is below the low water mark
	 * and resume input.
	 */
	task_suspend_input(h->tasks);
    }

    oklog("%s: #%d on %s\n",
	  outbound ? "CONNECT" : "ACCEPT",
	  h->player, network_connection_name(nh));

    result.ptr = h;
    return result;
}

void
server_refuse_connection(server_listener sl, network_handle nh)
{
    slistener *l = sl.ptr;

    if (l->print_messages)
	send_message(l->oid, nh, "server_full_msg",
		     "*** Sorry, but the server cannot accept any more"
		     " connections right now.",
		     "*** Please try again later.",
		     0);
    errlog("SERVER FULL: refusing connection on %s from %s\n",
	   l->name, network_connection_name(nh));
}

void
server_receive_line(server_handle sh, const char *line)
{
    shandle *h = (shandle *) sh.ptr;

    h->last_activity_time = time(0);
    new_input_task(h->tasks, line, h->binary);
}

void
server_close(server_handle sh)
{
    shandle *h = (shandle *) sh.ptr;

    oklog("CLIENT DISCONNECTED: %s on %s\n",
	  object_name(h->player),
	  network_connection_name(h->nhandle));
    h->disconnect_me = 1;
    call_notifier(h->player, h->listener, "user_client_disconnected");
    free_shandle(h);
}

void
server_suspend_input(Objid connection)
{
    shandle *h = find_shandle(connection);

    network_suspend_input(h->nhandle);
}

void
server_resume_input(Objid connection)
{
    shandle *h = find_shandle(connection);

    network_resume_input(h->nhandle);
}

void
player_connected(Objid old_id, Objid new_id, int is_newly_created)
{
    shandle *existing_h = find_shandle(new_id);
    shandle *new_h = find_shandle(old_id);

    if (!new_h)
	panic("Non-existent shandle connected");

    new_h->player = new_id;
    new_h->connection_time = time(0);

    if (existing_h) {
	/* we now have two shandles with the same player value while
	 * find_shandle assumes there can only be one.  This needs to
	 * be remedied before any call_notifier() call; luckily, the
	 * latter only needs listener value.
	 */
	Objid existing_listener = existing_h->listener;
	/* network_connection_name is allowed to reuse the same string
	 * storage, so we have to copy one of them.
	 */
	char *name1 = str_dup(network_connection_name(existing_h->nhandle));

	oklog("REDIRECTED: %s, was %s, now %s\n",
	      object_name(new_id),
	      name1,
	      network_connection_name(new_h->nhandle));
	free_str(name1);
	if (existing_h->print_messages)
	    send_message(existing_listener, existing_h->nhandle,
			 "redirect_from_msg",
			 "*** Redirecting connection to new port ***", 0);
	if (new_h->print_messages)
	    send_message(new_h->listener, new_h->nhandle, "redirect_to_msg",
			 "*** Redirecting old connection to this port ***", 0);
	network_close(existing_h->nhandle);
	free_shandle(existing_h);
	if (existing_listener == new_h->listener)
	    call_notifier(new_id, new_h->listener, "user_reconnected");
	else {
	    new_h->disconnect_me = 1;
	    call_notifier(new_id, existing_listener,
			  "user_client_disconnected");
	    new_h->disconnect_me = 0;
	    call_notifier(new_id, new_h->listener, "user_connected");
	}
    } else {
	oklog("%s: %s on %s\n",
	      is_newly_created ? "CREATED" : "CONNECTED",
	      object_name(new_h->player),
	      network_connection_name(new_h->nhandle));
	if (new_h->print_messages) {
	    if (is_newly_created)
		send_message(new_h->listener, new_h->nhandle, "create_msg",
			     "*** Created ***", 0);
	    else
		send_message(new_h->listener, new_h->nhandle, "connect_msg",
			     "*** Connected ***", 0);
	}
	call_notifier(new_id, new_h->listener,
		      is_newly_created ? "user_created" : "user_connected");
    }
}

void
notify(Objid player, const char *message)
{
    shandle *h = find_shandle(player);

    if (h && !h->disconnect_me)
	network_send_line(h->nhandle, message, 1);
    else if (in_emergency_mode)
	emergency_notify(player, message);
}

void
boot_player(Objid player)
{
    shandle *h = find_shandle(player);

    if (h)
	h->disconnect_me = 1;
}

void
write_active_connections(void)
{
    int count = 0;
    shandle *h;

    for (h = all_shandles; h; h = h->next)
	count++;

    dbio_printf("%d active connections with listeners\n", count);

    for (h = all_shandles; h; h = h->next)
	dbio_printf("%d %d\n", h->player, h->listener);
}

int
read_active_connections(void)
{
    int count, i, have_listeners = 0;
    char c;

    i = dbio_scanf("%d active connections%c", &count, &c);
    if (i == EOF) {		/* older database format */
	checkpointed_connections = new_list(0);
	return 1;
    } else if (i != 2) {
	errlog("READ_ACTIVE_CONNECTIONS: Bad active connections count.\n");
	return 0;
    } else if (c == ' ') {
	if (strcmp(dbio_read_string(), "with listeners") != 0) {
	    errlog("READ_ACTIVE_CONNECTIONS: Bad listeners tag.\n");
	    return 0;
	} else
	    have_listeners = 1;
    } else if (c != '\n') {
	errlog("READ_ACTIVE_CONNECTIONS: Bad EOL.\n");
	return 0;
    }
    checkpointed_connections = new_list(count);
    for (i = 1; i <= count; i++) {
	Objid who, listener;
	Var v;

	if (have_listeners) {
	    if (dbio_scanf("%d %d\n", &who, &listener) != 2) {
		errlog("READ_ACTIVE_CONNECTIONS: Bad conn/listener pair.\n");
		return 0;
	    }
	} else {
	    who = dbio_read_num();
	    listener = SYSTEM_OBJECT;
	}
	checkpointed_connections.v.list[i] = v = new_list(2);
	v.v.list[1].type = v.v.list[2].type = TYPE_OBJ;
	v.v.list[1].v.obj = who;
	v.v.list[2].v.obj = listener;
    }

    return 1;
}

int
main(int argc, char **argv)
{
    char *this_program = str_dup(argv[0]);
    const char *log_file = 0;
    int emergency = 0;
    Var desc;
    slistener *l;

    init_cmdline(argc, argv);

    argc--;
    argv++;
    while (argc > 0 && argv[0][0] == '-') {
	/* Deal with any command-line options */
	switch (argv[0][1]) {
	case 'e':		/* Emergency wizard mode */
	    emergency = 1;
	    break;
	case 'l':		/* Specified log file */
	    if (argc > 1) {
		log_file = argv[1];
		argc--;
		argv++;
	    } else
		argc = 0;	/* Provoke usage message below */
	    break;
	default:
	    argc = 0;		/* Provoke usage message below */
	}
	argc--;
	argv++;
    }

    if (log_file) {
	FILE *f = fopen(log_file, "a");

	if (f)
	    set_log_file(f);
	else {
	    perror("Error opening specified log file");
	    exit(1);
	}
    } else
	set_log_file(stderr);

    if (!db_initialize(&argc, &argv)
	|| !network_initialize(argc, argv, &desc)) {
	fprintf(stderr, "Usage: %s [-e] [-l log-file] %s %s\n",
		this_program, db_usage_string(), network_usage_string());
	exit(1);
    }
#if NETWORK_PROTOCOL != NP_SINGLE
    if (!emergency)
	fclose(stdout);
#endif
    if (log_file)
	fclose(stderr);

    oklog("STARTING: Version %s of the LambdaMOO server\n", server_version);
    oklog("          (Using %s protocol)\n", network_protocol_name());
    oklog("          (Task timeouts measured in %s seconds.)\n",
	  virtual_timer_available()? "server CPU" : "wall-clock");

    register_bi_functions();

    l = new_slistener(SYSTEM_OBJECT, desc, 1, 0);
    if (!l) {
	errlog("Can't create initial connection point!\n");
	exit(1);
    }
    free_var(desc);

    if (!db_load())
	exit(1);

    load_server_options();

    SRANDOM(time(0));

    parent_pid = getpid();
    setup_signals();
    reset_command_history();

    if (!emergency || emergency_mode()) {
	if (!start_listener(l))
	    exit(1);

	main_loop();
	network_shutdown();
    }
    db_shutdown();
    free_str(this_program);

    return 0;
}


/**** built in functions ****/

static package
bf_server_version(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    r.type = TYPE_STR;
    r.v.str = str_dup(server_version);
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_renumber(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    Objid o = arglist.v.list[1].v.obj;
    free_var(arglist);

    if (!valid(o))
	return make_error_pack(E_INVARG);
    else if (!is_wizard(progr))
	return make_error_pack(E_PERM);

    r.type = TYPE_OBJ;
    r.v.obj = db_renumber_object(o);
    return make_var_pack(r);
}

static package
bf_reset_max_object(Var arglist, Byte next, void *vdata, Objid progr)
{
    free_var(arglist);

    if (!is_wizard(progr))
	return make_error_pack(E_PERM);

    db_reset_last_used_objid();
    return no_var_pack();
}

static package
bf_memory_usage(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    r = memory_usage();
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_shutdown(Var arglist, Byte next, void *vdata, Objid progr)
{
    /*
     * The stream 's' and its contents will leak, but we're shutting down,
     * so it doesn't really matter.
     */

    Stream *s;
    int nargs = arglist.v.list[0].v.num;
    const char *msg = (nargs >= 1 ? arglist.v.list[1].v.str : 0);

    if (!is_wizard(progr)) {
	free_var(arglist);
	return make_error_pack(E_PERM);
    }
    s = new_stream(100);
    stream_printf(s, "shutdown() called by %s", object_name(progr));
    if (msg)
	stream_printf(s, ": %s", msg);
    shutdown_message = stream_contents(s);

    free_var(arglist);
    return no_var_pack();
}

static package
bf_dump_database(Var arglist, Byte next, void *vdata, Objid progr)
{
    free_var(arglist);
    if (!is_wizard(progr))
	return make_error_pack(E_PERM);

    checkpoint_requested = CHKPT_FUNC;
    return no_var_pack();
}

static package
bf_db_disk_size(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var v;

    free_var(arglist);
    v.type = TYPE_INT;
    if ((v.v.num = db_disk_size()) < 0)
	return make_raise_pack(E_QUOTA, "No database file(s) available", zero);
    else
	return make_var_pack(v);
}

#ifdef OUTBOUND_NETWORK
static slistener *
find_slistener_by_oid(Objid obj)
{
    slistener *l;

    for (l = all_slisteners; l; l = l->next)
	if (l->oid == obj)
	    return l;

    return 0;
}
#endif /* OUTBOUND_NETWORK */

static package
bf_open_network_connection(Var arglist, Byte next, void *vdata, Objid progr)
{
#ifdef OUTBOUND_NETWORK

    Var r;
    enum error e;
    server_listener sl;
    slistener l;

    if (!is_wizard(progr)) {
        free_var(arglist);
        return make_error_pack(E_PERM);
    }

    if (arglist.v.list[0].v.num == 3) {
	Objid oid;

	if (arglist.v.list[3].type != TYPE_OBJ) {
	    return make_error_pack(E_TYPE);
	}
	oid = arglist.v.list[3].v.obj;
	arglist = listdelete(arglist, 3);

	sl.ptr = find_slistener_by_oid(oid);
	if (!sl.ptr) {
	    /* Create a temporary */
	    l.print_messages = 0;
	    l.name = "open_network_connection";
	    l.desc = zero;
	    l.oid = oid;
	    sl.ptr = &l;
	}
    } else {
	sl.ptr = NULL;
    }

    e = network_open_connection(arglist, sl);
    free_var(arglist);
    if (e == E_NONE) {
	/* The connection was successfully opened, implying that
	 * server_new_connection was called, implying and a new negative
	 * player number was allocated for the connection.  Thus, the old
	 * value of next_unconnected_player is the number of our connection.
	 */
	r.type = TYPE_OBJ;
	r.v.obj = next_unconnected_player + 1;
    } else {
	r.type = TYPE_ERR;
	r.v.err = e;
    }
    if (r.type == TYPE_ERR)
	return make_error_pack(r.v.err);
    else
	return make_var_pack(r);

#else				/* !OUTBOUND_NETWORK */

    /* This function is disabled in this server. */

    free_var(arglist);
    return make_error_pack(E_PERM);

#endif
}

static package
bf_connected_players(Var arglist, Byte next, void *vdata, Objid progr)
{
    shandle *h;
    int nargs = arglist.v.list[0].v.num;
    int show_all = (nargs >= 1 && is_true(arglist.v.list[1]));
    int count = 0;
    Var result;

    free_var(arglist);
    for (h = all_shandles; h; h = h->next)
	if ((show_all || h->connection_time != 0) && !h->disconnect_me)
	    count++;

    result = new_list(count);
    count = 0;

    for (h = all_shandles; h; h = h->next) {
	if ((show_all || h->connection_time != 0) && !h->disconnect_me) {
	    count++;
	    result.v.list[count].type = TYPE_OBJ;
	    result.v.list[count].v.obj = h->player;
	}
    }

    return make_var_pack(result);
}

static package
bf_connected_seconds(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (player) */
    Var r;
    shandle *h = find_shandle(arglist.v.list[1].v.obj);

    r.type = TYPE_INT;
    if (h && h->connection_time != 0 && !h->disconnect_me)
	r.v.num = time(0) - h->connection_time;
    else
	r.v.num = -1;
    free_var(arglist);
    if (r.v.num < 0)
	return make_error_pack(E_INVARG);
    else
	return make_var_pack(r);
}

static package
bf_idle_seconds(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (player) */
    Var r;
    shandle *h = find_shandle(arglist.v.list[1].v.obj);

    r.type = TYPE_INT;
    if (h && !h->disconnect_me)
	r.v.num = time(0) - h->last_activity_time;
    else
	r.v.num = -1;
    free_var(arglist);
    if (r.v.num < 0)
	return make_error_pack(E_INVARG);
    else
	return make_var_pack(r);
}

static package
bf_connection_name(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (player) */
    Objid who = arglist.v.list[1].v.obj;
    shandle *h = find_shandle(who);
    const char *conn_name;
    Var r;

    if (h && !h->disconnect_me)
	conn_name = network_connection_name(h->nhandle);
    else
	conn_name = 0;

    free_var(arglist);
    if (!is_wizard(progr) && progr != who)
	return make_error_pack(E_PERM);
    else if (!conn_name)
	return make_error_pack(E_INVARG);
    else {
	r.type = TYPE_STR;
	r.v.str = str_dup(conn_name);
	return make_var_pack(r);
    }
}

static package
bf_notify(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (player, string [, no_flush]) */
    Objid conn = arglist.v.list[1].v.obj;
    const char *line = arglist.v.list[2].v.str;
    int no_flush = (arglist.v.list[0].v.num > 2
		    ? is_true(arglist.v.list[3])
		    : 0);
    shandle *h = find_shandle(conn);
    Var r;

    if (!is_wizard(progr) && progr != conn) {
	free_var(arglist);
	return make_error_pack(E_PERM);
    }
    r.type = TYPE_INT;
    if (h && !h->disconnect_me) {
	if (h->binary) {
	    int length;

	    line = binary_to_raw_bytes(line, &length);
	    if (!line) {
		free_var(arglist);
		return make_error_pack(E_INVARG);
	    }
	    r.v.num = network_send_bytes(h->nhandle, line, length, !no_flush);
	} else
	    r.v.num = network_send_line(h->nhandle, line, !no_flush);
    } else {
	if (in_emergency_mode)
	    emergency_notify(conn, line);
	r.v.num = 1;
    }
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_boot_player(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (object) */
    Objid oid = arglist.v.list[1].v.obj;

    free_var(arglist);

    if (oid != progr && !is_wizard(progr))
	return make_error_pack(E_PERM);

    boot_player(oid);
    return no_var_pack();
}

static package
bf_set_connection_option(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (conn, option, value) */
    Objid oid = arglist.v.list[1].v.obj;
    const char *option = arglist.v.list[2].v.str;
    Var value = arglist.v.list[3];
    shandle *h = find_shandle(oid);
    enum error e = E_NONE;

    if (oid != progr && !is_wizard(progr))
	e = E_PERM;
    else if (!h || h->disconnect_me
	     || (!server_set_connection_option(h, option, value)
		 && !tasks_set_connection_option(h->tasks, option, value)
		 && !network_set_connection_option(h->nhandle, option, value)))
	e = E_INVARG;

    free_var(arglist);
    if (e == E_NONE)
	return no_var_pack();
    else
	return make_error_pack(e);
}

static package
bf_connection_options(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (conn [, opt-name]) */
    Objid oid = arglist.v.list[1].v.obj;
    int nargs = arglist.v.list[0].v.num;
    const char *oname = (nargs >= 2 ? arglist.v.list[2].v.str : 0);
    shandle *h = find_shandle(oid);
    Var ans;

    if (!h || h->disconnect_me) {
	free_var(arglist);
	return make_error_pack(E_INVARG);
    } else if (oid != progr && !is_wizard(progr)) {
	free_var(arglist);
	return make_error_pack(E_PERM);
    }
    if (oname) {
	if (!server_connection_option(h, oname, &ans)
	    && !tasks_connection_option(h->tasks, oname, &ans)
	    && !network_connection_option(h->nhandle, oname, &ans)) {
	    free_var(arglist);
	    return make_error_pack(E_INVARG);
	}
    } else {
	ans = new_list(0);
	ans = server_connection_options(h, ans);
	ans = tasks_connection_options(h->tasks, ans);
	ans = network_connection_options(h->nhandle, ans);
    }

    free_var(arglist);
    return make_var_pack(ans);
}

static slistener *
find_slistener(Var desc)
{
    slistener *l;

    for (l = all_slisteners; l; l = l->next)
	if (equality(desc, l->desc, 0))
	    return l;

    return 0;
}

static package
bf_listen(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (oid, desc) */
    Objid oid = arglist.v.list[1].v.obj;
    Var desc = arglist.v.list[2];
    int nargs = arglist.v.list[0].v.num;
    int print_messages = nargs >= 3 && is_true(arglist.v.list[3]);
    enum error e;
    slistener *l = 0;

    if (!is_wizard(progr))
	e = E_PERM;
    else if (!valid(oid) || find_slistener(desc))
	e = E_INVARG;
    else if (!(l = new_slistener(oid, desc, print_messages, &e)));	/* Do nothing; e is already set */
    else if (!start_listener(l))
	e = E_QUOTA;

    free_var(arglist);
    if (e == E_NONE)
	return make_var_pack(var_ref(l->desc));
    else
	return make_error_pack(e);
}

static package
bf_unlisten(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (desc) */
    Var desc = arglist.v.list[1];
    enum error e = E_NONE;
    slistener *l = 0;

    if (!is_wizard(progr))
	e = E_PERM;
    else if (!(l = find_slistener(desc)))
	e = E_INVARG;

    free_var(arglist);
    if (e == E_NONE) {
	free_slistener(l);
	return no_var_pack();
    } else
	return make_error_pack(e);
}

static package
bf_listeners(Var arglist, Byte next, void *vdata, Objid progr)
{				/* () */
    int i, count = 0;
    Var list, entry;
    slistener *l;

    free_var(arglist);
    for (l = all_slisteners; l; l = l->next)
	count++;
    list = new_list(count);
    for (i = 1, l = all_slisteners; l; i++, l = l->next) {
	list.v.list[i] = entry = new_list(3);
	entry.v.list[1].type = TYPE_OBJ;
	entry.v.list[1].v.obj = l->oid;
	entry.v.list[2] = var_ref(l->desc);
	entry.v.list[3].type = TYPE_INT;
	entry.v.list[3].v.num = l->print_messages;
    }

    return make_var_pack(list);
}

static package
bf_buffered_output_length(Var arglist, Byte next, void *vdata, Objid progr)
{				/* ([connection]) */
    int nargs = arglist.v.list[0].v.num;
    Objid conn = nargs >= 1 ? arglist.v.list[1].v.obj : 0;
    Var r;

    free_var(arglist);
    r.type = TYPE_INT;
    if (nargs == 0)
	r.v.num = MAX_QUEUED_OUTPUT;
    else {
	shandle *h = find_shandle(conn);

	if (!h)
	    return make_error_pack(E_INVARG);
	else if (progr != conn && !is_wizard(progr))
	    return make_error_pack(E_PERM);

	r.v.num = network_buffered_output_length(h->nhandle);
    }

    return make_var_pack(r);
}

void
register_server(void)
{
    register_function("server_version", 0, 0, bf_server_version);
    register_function("renumber", 1, 1, bf_renumber, TYPE_OBJ);
    register_function("reset_max_object", 0, 0, bf_reset_max_object);
    register_function("memory_usage", 0, 0, bf_memory_usage);
    register_function("shutdown", 0, 1, bf_shutdown, TYPE_STR);
    register_function("dump_database", 0, 0, bf_dump_database);
    register_function("db_disk_size", 0, 0, bf_db_disk_size);
    register_function("open_network_connection", 0, -1,
		      bf_open_network_connection);
    register_function("connected_players", 0, 1, bf_connected_players,
		      TYPE_ANY);
    register_function("connected_seconds", 1, 1, bf_connected_seconds,
		      TYPE_OBJ);
    register_function("idle_seconds", 1, 1, bf_idle_seconds, TYPE_OBJ);
    register_function("connection_name", 1, 1, bf_connection_name, TYPE_OBJ);
    register_function("notify", 2, 3, bf_notify, TYPE_OBJ, TYPE_STR, TYPE_ANY);
    register_function("boot_player", 1, 1, bf_boot_player, TYPE_OBJ);
    register_function("set_connection_option", 3, 3, bf_set_connection_option,
		      TYPE_OBJ, TYPE_STR, TYPE_ANY);
    register_function("connection_option", 2, 2, bf_connection_options,
		      TYPE_OBJ, TYPE_STR);
    register_function("connection_options", 1, 1, bf_connection_options,
		      TYPE_OBJ);
    register_function("listen", 2, 3, bf_listen, TYPE_OBJ, TYPE_ANY, TYPE_ANY);
    register_function("unlisten", 1, 1, bf_unlisten, TYPE_ANY);
    register_function("listeners", 0, 0, bf_listeners);
    register_function("buffered_output_length", 0, 1,
		      bf_buffered_output_length, TYPE_OBJ);
}

char rcsid_server[] = "$Id$";

/*
 * $Log$
 * Revision 1.12  2007/06/02 21:34:36  wrog
 * fix player_connect() so that the user_client_disconnected hook
 * sees a disconnected player, same as with server_close()
 *
 * Revision 1.11  2007/05/29 12:21:47  wrog
 * fixes server panic (or lost messages) caused by attempting to write to freed network handle during #0:user_reconnected; removes the one case where server and network handles were not being freed together
 *
 * Revision 1.10  2006/11/21 18:42:37  pschwan
 * b=1500775
 * fixes two use-after-free bugs that could lead very rarely to
 * calling the wrong functions during player connection
 *
 * Revision 1.9  2005/09/29 18:46:18  bjj
 * Add third argument to open_network_connection() that associates a specific listener object with the new connection.  This simplifies a lot of outbound connection management.
 *
 * Revision 1.8  2004/05/25 07:28:55  wrog
 * indentation fixes
 *
 * Revision 1.7  2004/05/22 01:25:44  wrog
 * merging in WROGUE changes (W_SRCIP, W_STARTUP, W_OOB)
 *
 * Revision 1.5.10.4  2004/05/21 23:02:56  wrog
 * NP_SINGLE needs to have stdout stay open
 *
 * Revision 1.5.10.3  2004/05/20 19:57:11  wrog
 * fixed flushing issues w.r.t. emergency mode;
 * close stdout and stderr when we are not using them
 *
 * Revision 1.6  2003/06/12 18:16:56  bjj
 * Suspend input on connection until :do_login_command() can run.
 *
 * Revision 1.5.10.2  2003/06/11 10:40:16  wrog
 * added binary argument to new_input_task()
 *
 * Revision 1.5.10.1  2003/06/07 12:59:04  wrog
 * introduced connection_option macros
 *
 * Revision 1.5  1998/12/29 06:56:32  nop
 * Fixed leak in onc().
 *
 * Revision 1.4  1998/12/14 13:18:57  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.3  1997/03/03 05:03:52  nop
 * steak2: move protectedness into builtin struct, load_server_options()
 * now required for $server_options updates.
 *
 * Revision 1.2  1997/03/03 04:19:24  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:01  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.11  1996/05/12  21:26:10  pavel
 * Slightly cleaned up some code in bf_shutdown().  Release 1.8.0p5.
 *
 * Revision 2.10  1996/04/08  01:01:19  pavel
 * Made get_server_option() public.  Added `-l LOGFILE' option.  Slightly
 * reordered the start-up sequence to more frequently avoid writing into the
 * log when the server is about to abort anyway.  Release 1.8.0p3.
 *
 * Revision 2.9  1996/03/19  07:10:10  pavel
 * Changed connect timeout handling to use an in-DB server option.  Fixed
 * task_id() in emergency-mode evals.  Moved built-in function registration to
 * happen after the start-up messages.  Release 1.8.0p2.
 *
 * Revision 2.8  1996/03/10  01:10:50  pavel
 * Added `connection_option()' built-in function.  Made argument to
 * shutdown() optional.  Release 1.8.0.
 *
 * Revision 2.7  1996/02/11  00:49:03  pavel
 * Made connection messages come from properties on LISTENER.server_options,
 * if defined.  Enhanced the robustness of the system-option-fetching code in
 * the face of invalid objects.  Release 1.8.0beta2.
 *
 * Revision 2.6  1996/02/08  06:53:21  pavel
 * Added `list' and `disassemble' commands to emergency wizard mode.  Now
 * ignore SIGFPE.  Added support for tasks-module connection options.  Added
 * connection_options() function.  Renamed err/logf() to errlog/oklog() and
 * TYPE_NUM to TYPE_INT.  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.5  1996/01/16  07:25:25  pavel
 * Fixed uninitialized-variable bug in `program' in emergency wizard mode.
 * Release 1.8.0alpha6.
 *
 * Revision 2.4  1996/01/11  07:36:41  pavel
 * Added CHKPT_FUNC reason, missing forever.  Added support for binary I/O.
 * Added calls to $user_disconnected() on timeout and for booted un-logged-in
 * connections.  Added buffered_output_length() built-in.
 * Release 1.8.0alpha5.
 *
 * Revision 2.3  1995/12/31  00:06:00  pavel
 * Added support for the new multiple-listening-points interface, including
 * listen(), unlisten(), and listeners().  Allowed DB-configurable messages to
 * be given as lists of strings.  Suppressed DB-configurable messages to
 * outbound connections.  Moved server-full handling to here from net_multi.c,
 * also making the message DB-configurable.    Release 1.8.0alpha4.
 *
 * Revision 2.2  1995/12/28  00:33:08  pavel
 * Added db_disk_size() built-in.  Release 1.8.0alpha3.
 *
 * Revision 2.1  1995/12/11  07:41:12  pavel
 * Fixed checkpoint timing to test for overflow when adding $dump_interval to
 * the current time.  Added an optional argument to `connected_players()' to
 * include even un-logged-in or outbound connections.
 *
 * Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  04:31:19  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.19  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.18  1992/10/23  22:22:24  pavel
 * Change to #include "my-wait.h" instead of <sys/wait.h>.
 *
 * Revision 1.17  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.16  1992/10/17  20:52:15  pavel
 * Global rename of strdup->str_dup, strref->str_ref, vardup->var_dup, and
 * varref->var_ref.
 *
 * Revision 1.15  1992/10/02  00:01:28  pavel
 * Fixed a bug in the main loop whereby external shutdown requests (e.g., the
 * interrupt signal) might not be noticed until some user I/O or a task
 * execution occurred.
 *
 * Revision 1.14  1992/09/26  02:26:20  pavel
 * Added support for printing the network protocol name on server start-up.
 *
 * Revision 1.13  1992/09/15  15:53:00  pjames
 * Renamed FLUSH choices.
 *
 * Revision 1.12  1992/09/14  17:46:16  pjames
 * Moved db_modification code to db modules.
 * Added set up for disk based data base updating.
 *
 * Revision 1.11  1992/09/08  22:08:56  pjames
 * Added all code from bf_server.c.
 *
 * Revision 1.10  1992/09/03  23:52:33  pavel
 * Added support for multiple complete network implementations.
 *
 * Revision 1.9  1992/08/28  16:01:04  pjames
 * Changed myfree(*, M_STRING) to free_str(*).
 *
 * Revision 1.8  1992/08/26  18:30:45  pavel
 * Dump interval and retry interval are now local integers.
 * Changed logic in regards to using POSIX.
 *
 * Revision 1.7  1992/08/13  21:28:11  pjames
 * Now only calls one procedure to register built in functions.
 *
 * Revision 1.6  1992/08/12  01:50:42  pjames
 * run_ready_tasks() was moved to before logoff actions, so that logoffs
 * would happen faster.
 *
 * Revision 1.5  1992/08/10  16:55:31  pjames
 * Updated #includes.  Added register_bf_log().
 *
 * Revision 1.4  1992/07/30  23:22:26  pavel
 * Added a remote checkpoint-request signal (SIGUSR2), analogous to the existing
 * shutdown-requestion signal (SIGUSR1).
 *
 * Revision 1.3  1992/07/30  00:40:59  pavel
 * Add support for compiling on RISC/os 4.52 and NonStop-UX A22.
 *
 * Revision 1.2  1992/07/21  00:06:27  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS ident. string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
