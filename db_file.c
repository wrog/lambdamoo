/******************************************************************************
  Copyright (c) 1995, 1996 Xerox Corporation.  All rights reserved.
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

/*****************************************************************************
 * Routines for initializing, loading, dumping, and shutting down the database
 *****************************************************************************/

#include "my-stat.h"
#include "my-unistd.h"
#include "my-stdio.h"
#include "my-stdlib.h"
#include "my-string.h"

#include "config.h"
#include "db.h"
#include "db_io.h"
#include "db_private.h"
#include "exceptions.h"
#include "list.h"
#include "log.h"
#include "options.h"
#include "server.h"
#include "storage.h"
#include "streams.h"
#include "str_intern.h"
#include "tasks.h"
#include "timers.h"
#include "version.h"

static char *input_db_name, *dump_db_name;
static int dump_generation = 0;
static const char *header_format_string
= "** LambdaMOO Database, Format Version %u **\n";

DB_Version dbio_input_version;


/*********** Verb and property I/O ***********/

static void
read_verbdef(Verbdef * v)
{
    v->name = dbio_read_string_intern();
    v->owner = dbio_read_objid();
    v->perms = dbio_read_num();
    v->prep = dbio_read_num();
    v->next = 0;
    v->program = 0;
}

static void
write_verbdef(Verbdef * v)
{
    dbio_write_string(v->name);
    dbio_write_objid(v->owner);
    dbio_write_num(v->perms);
    dbio_write_num(v->prep);
}

static Propdef
read_propdef(void)
{
    const char *name = dbio_read_string_intern();
    return dbpriv_new_propdef(name);
}

static void
write_propdef(Propdef * p)
{
    dbio_write_string(p->name);
}

static void
read_propval(Pval * p)
{
    p->var = dbio_read_var();
    p->owner = dbio_read_objid();
    p->perms = dbio_read_num();
}

static void
write_propval(Pval * p)
{
    dbio_write_var(p->var);
    dbio_write_objid(p->owner);
    dbio_write_num(p->perms);
}


/*********** Object I/O ***********/

static int
read_object(void)
{
    Objid oid;
    Object *o;
    char s[20];
    int i;
    Verbdef *v, **prevv;
    int nprops;

    if (dbio_scanf("#%"SCNdN, &oid) != 1 || oid != db_last_used_objid() + 1)
	return 0;
    dbio_read_line(s, sizeof(s));

    if (strcmp(s, " recycled\n") == 0) {
	dbpriv_new_recycled_object();
	return 1;
    } else if (strcmp(s, "\n") != 0)
	return 0;

    o = dbpriv_new_object();
    o->name = dbio_read_string_intern();
    (void) dbio_read_string();	/* discard old handles string */
    o->flags = dbio_read_num();

    o->owner = dbio_read_objid();

    o->location = dbio_read_objid();
    o->contents = dbio_read_objid();
    o->next = dbio_read_objid();

    o->parent = dbio_read_objid();
    o->child = dbio_read_objid();
    o->sibling = dbio_read_objid();

    o->verbdefs = 0;
    prevv = &(o->verbdefs);
    for (i = dbio_read_num(); i > 0; i--) {
	v = mymalloc(sizeof(Verbdef), M_VERBDEF);
	read_verbdef(v);
	*prevv = v;
	prevv = &(v->next);
    }

    o->propdefs.cur_length = 0;
    o->propdefs.max_length = 0;
    o->propdefs.l = 0;
    if ((i = dbio_read_num()) != 0) {
	o->propdefs.l = mymalloc(i * sizeof(Propdef), M_PROPDEF);
	o->propdefs.cur_length = i;
	o->propdefs.max_length = i;
	for (i = 0; i < o->propdefs.cur_length; i++)
	    o->propdefs.l[i] = read_propdef();
    }
    nprops = dbio_read_num();
    if (nprops)
	o->propval = mymalloc(nprops * sizeof(Pval), M_PVAL);
    else
	o->propval = 0;

    for (i = 0; i < nprops; i++) {
	read_propval(o->propval + i);
    }

    return 1;
}

static void
write_object(Objid oid)
{
    Object *o;
    Verbdef *v;
    int i;
    int nverbdefs, nprops;

    if (!valid(oid)) {
	dbio_printf("#%"PRIdN" recycled\n", oid);
	return;
    }
    o = dbpriv_find_object(oid);

    dbio_printf("#%"PRIdN"\n", oid);
    dbio_write_string(o->name);
    dbio_write_string("");	/* placeholder for old handles string */
    dbio_write_num(o->flags);

    dbio_write_objid(o->owner);

    dbio_write_objid(o->location);
    dbio_write_objid(o->contents);
    dbio_write_objid(o->next);

    dbio_write_objid(o->parent);
    dbio_write_objid(o->child);
    dbio_write_objid(o->sibling);

    for (v = o->verbdefs, nverbdefs = 0; v; v = v->next)
	nverbdefs++;

    dbio_write_num(nverbdefs);
    for (v = o->verbdefs; v; v = v->next)
	write_verbdef(v);

    dbio_write_num(o->propdefs.cur_length);
    for (i = 0; i < o->propdefs.cur_length; i++)
	write_propdef(&o->propdefs.l[i]);

    nprops = dbpriv_count_properties(oid);

    dbio_write_num(nprops);
    for (i = 0; i < nprops; i++)
	write_propval(o->propval + i);
}


/*********** File-level Input ***********/

static int
validate_hierarchies(void)
{
    Objid oid;
    Objid size = db_last_used_objid() + 1;
    int broken = 0;
    int fixed_nexts = 0;

    oklog("VALIDATING the object hierarchies ...\n");

#   define MAYBE_LOG_PROGRESS					\
    {								\
        if (log_report_progress()) {				\
	    oklog("VALIDATE: Done through #%"PRIdN" ...\n", oid);	\
	}							\
    }

    oklog("VALIDATE: Phase 1: Check for invalid objects ...\n");
    for (oid = 0; oid < size; oid++) {
	Object *o = dbpriv_find_object(oid);

	MAYBE_LOG_PROGRESS;
	if (o) {
	    if (o->location == NOTHING && o->next != NOTHING) {
		o->next = NOTHING;
		fixed_nexts++;
	    }
#	    define CHECK(field, name) 					\
	    {								\
	        if (o->field != NOTHING					\
		    && !dbpriv_find_object(o->field)) {			\
		    errlog("VALIDATE: #%"PRIdN".%s = #%"PRIdN" <invalid> ... fixed.\n", \
			   oid, name, o->field);			\
		    o->field = NOTHING;				  	\
		}							\
	    }

	    CHECK(parent, "parent");
	    CHECK(child, "child");
	    CHECK(sibling, "sibling");
	    CHECK(location, "location");
	    CHECK(contents, "contents");
	    CHECK(next, "next");

#	    undef CHECK
	}
    }

    if (fixed_nexts != 0)
	errlog("VALIDATE: Fixed %d should-be-null next pointer(s) ...\n",
	       fixed_nexts);

    oklog("VALIDATE: Phase 2: Check for cycles ...\n");
    for (oid = 0; oid < size; oid++) {
	Object *o = dbpriv_find_object(oid);

	MAYBE_LOG_PROGRESS;
	if (o) {
#	    define CHECK(start, field, name)			\
	    {							\
		Objid slower = start;				\
		Objid faster = slower;				\
		while (faster != NOTHING) {			\
		    faster = dbpriv_find_object(faster)->field;	\
		    if (faster == NOTHING)			\
			break;					\
		    faster = dbpriv_find_object(faster)->field;	\
		    slower = dbpriv_find_object(slower)->field;	\
		    if (faster == slower) {			\
			errlog("VALIDATE: Cycle in `%s' chain of #%"PRIdN"\n", \
			       name, oid);			\
			broken = 1;				\
			break;					\
		    }						\
		}						\
	    }

	    CHECK(o->parent, parent, "parent");
	    CHECK(o->child, sibling, "child");
	    CHECK(o->location, location, "location");
	    CHECK(o->contents, next, "contents");

#	    undef CHECK

	    /* setup for phase 3:  set two temp flags on every object */
	    o->flags |= (3<<FLAG_FIRST_TEMP);
	}
    }

    if (broken)			/* Can't continue if cycles found */
	return 0;

    oklog("VALIDATE: Phase 3a: Finding delusional parents ...\n");
    for (oid = 0; oid < size; oid++) {
	Object *o = dbpriv_find_object(oid);

	MAYBE_LOG_PROGRESS;
	if (o) {
#	    define CHECK(up, down, down_name, across, FLAG)	\
	    {							\
		Objid	oidkid;					\
		Object *okid;					\
								\
		for (oidkid = o->down;				\
		     oidkid != NOTHING;				\
		     oidkid = okid->across) {			\
								\
		    okid = dbpriv_find_object(oidkid);		\
		    if (okid->up != oid) {			\
			errlog(					\
			    "VALIDATE: #%"PRIdN" erroneously on #%"PRIdN"'s %s list.\n", \
			    oidkid, oid, down_name);		\
			broken = 1;				\
		    }						\
		    else {					\
			/* mark okid as properly claimed */	\
			okid->flags &= ~(1<<(FLAG));		\
		    }						\
		}						\
	    }

	    CHECK(parent,   child,    "child",    sibling, FLAG_FIRST_TEMP);
	    CHECK(location, contents, "contents", next,    FLAG_FIRST_TEMP+1);

#	    undef CHECK
	}
    }

    oklog("VALIDATE: Phase 3b: Finding delusional children ...\n");
    for (oid = 0; oid < size; oid++) {
	Object *o = dbpriv_find_object(oid);

	MAYBE_LOG_PROGRESS;
	if (o) {
#	    define CHECK(up, up_name, down_name, FLAG)			\
	    {								\
		/* If oid is unclaimed, up must be NOTHING */		\
		if ((o->flags & (1<<(FLAG))) && o->up != NOTHING) {	\
		    errlog("VALIDATE: #%"PRIdN" not in %s (#%"PRIdN")'s %s list.\n", \
			   oid, up_name, o->up, down_name);		\
		    broken = 1;						\
		}							\
	    }

	    CHECK(parent,   "parent",   "child",    FLAG_FIRST_TEMP);
	    CHECK(location, "location", "contents", FLAG_FIRST_TEMP+1);

	    /* clear temp flags */
	    o->flags &= ~(3<<FLAG_FIRST_TEMP);

#	    undef CHECK
	}
    }

    oklog("VALIDATING the object hierarchies ... finished.\n");
    return !broken;
}

static const char *
fmt_verb_name(void *data)
{
    db_verb_handle *h = data;
    static Stream *s = 0;

    if (!s)
	s = new_stream(40);

    stream_printf(s, "#%"PRIdN":%s", db_verb_definer(*h), db_verb_names(*h));
    return reset_stream(s);
}

static int
read_db_file(void)
{
    Objid oid;
    Num i, nobjs, nprogs, nusers, vnum, dummy;
    Var user_list;
    db_verb_handle h;
    Program *program;

    if (dbio_scanf(header_format_string, &dbio_input_version) != 1)
	dbio_input_version = DBV_Prehistory;

    if (!check_version(dbio_input_version)) {
	errlog("READ_DB_FILE: Unknown DB version number: %d\n",
	       dbio_input_version);
	return 0;
    }
    /* I use a `dummy' variable here and elsewhere instead of the `*'
     * assignment-suppression syntax of `scanf' because it allows more
     * straightforward error checking; unfortunately, the standard says that
     * suppressed assignments are not counted in determining the returned value
     * of `scanf'...
     */
    if (dbio_scanf("%"SCNdN"\n%"SCNdN"\n%"SCNdN"\n%"SCNdN"\n",
		   &nobjs, &nprogs, &dummy, &nusers) != 4) {
	errlog("READ_DB_FILE: Bad header\n");
	return 0;
    }
    user_list = new_list(nusers);
    for (i = 1; i <= nusers; i++) {
	user_list.v.list[i].type = TYPE_OBJ;
	user_list.v.list[i].v.obj = dbio_read_objid();
    }
    dbpriv_set_all_users(user_list);

    oklog("LOADING: Reading %"PRIdN" objects...\n", nobjs);
    for (i = 1; i <= nobjs; i++) {
	if (!read_object()) {
	    errlog("READ_DB_FILE: Bad object #%"PRIdN".\n", i - 1);
	    return 0;
	}
	if (i == nobjs || log_report_progress())
	    oklog("LOADING: Done reading %"PRIdN" objects ...\n", i);
    }

    if (!validate_hierarchies()) {
	errlog("READ_DB_FILE: Errors in object hierarchies.\n");
	return 0;
    }
    oklog("LOADING: Reading %"PRIdN" MOO verb programs...\n", nprogs);
    for (i = 1; i <= nprogs; i++) {
	if (dbio_scanf("#%"SCNdN":%"SCNdN"\n", &oid, &vnum) != 2) {
	    errlog("READ_DB_FILE: Bad program header, i = %"PRIdN".\n", i);
	    return 0;
	}
	if (!valid(oid)) {
	    errlog("READ_DB_FILE: Verb for non-existant object: #%"PRIdN":%"PRIdN".\n",
		   oid, vnum);
	    return 0;
	}
	h = db_find_indexed_verb(oid, vnum + 1);	/* DB file is 0-based. */
	if (!h.ptr) {
	    errlog("READ_DB_FILE: Unknown verb index: #%"PRIdN":%"PRIdN".\n", oid, vnum);
	    return 0;
	}
	program = dbio_read_program(dbio_input_version, fmt_verb_name, &h);
	if (!program) {
	    errlog("READ_DB_FILE: Unparsable program #%"PRIdN":%"PRIdN".\n", oid, vnum);
	    return 0;
	}
	db_set_verb_program(h, program);
	if (i == nprogs || log_report_progress())
	    oklog("LOADING: Done reading %"PRIdN" verb programs...\n", i);
    }

    oklog("LOADING: Reading forked and suspended tasks...\n");
    if (!read_task_queue()) {
	errlog("READ_DB_FILE: Can't read task queue.\n");
	return 0;
    }
    oklog("LOADING: Reading list of formerly active connections...\n");
    if (!read_active_connections()) {
	errlog("DB_READ: Can't read active connections.\n");
	return 0;
    }
    return 1;
}


/*********** File-level Output ***********/

static int
write_db_file(const char *reason)
{
    Objid oid;
    Objid max_oid = db_last_used_objid();
    Verbdef *v;
    Var user_list;
    int i;
    volatile int nprogs = 0;
    volatile int success = 1;

    for (oid = 0; oid <= max_oid; oid++) {
	if (valid(oid))
	    for (v = dbpriv_find_object(oid)->verbdefs; v; v = v->next)
		if (v->program)
		    nprogs++;
    }

    user_list = db_all_users();

    TRY {
	dbio_printf(header_format_string, current_version);
	dbio_printf("%"PRIdN"\n%d\n%d\n%"PRIdN"\n",
		    max_oid + 1, nprogs, 0, user_list.v.list[0].v.num);
	for (i = 1; i <= user_list.v.list[0].v.num; i++)
	    dbio_write_objid(user_list.v.list[i].v.obj);
	oklog("%s: Writing %"PRIdN" objects...\n", reason, max_oid + 1);
	for (oid = 0; oid <= max_oid; oid++) {
	    write_object(oid);
	    if (oid == max_oid || log_report_progress())
		oklog("%s: Done writing %"PRIdN" objects...\n", reason, oid + 1);
	}
	oklog("%s: Writing %d MOO verb programs...\n", reason, nprogs);
	for (i = 0, oid = 0; oid <= max_oid; oid++)
	    if (valid(oid)) {
		int vcount = 0;

		for (v = dbpriv_find_object(oid)->verbdefs; v; v = v->next) {
		    if (v->program) {
			dbio_printf("#%"PRIdN":%d\n", oid, vcount);
			dbio_write_program(v->program);
			if (++i == nprogs || log_report_progress())
			    oklog("%s: Done writing %d verb programs...\n",
				  reason, i);
		    }
		    vcount++;
		}
	    }
	oklog("%s: Writing forked and suspended tasks...\n", reason);
	write_task_queue();
	oklog("%s: Writing list of formerly active connections...\n", reason);
	write_active_connections();
    }
    EXCEPT(dbpriv_dbio_failed)
	success = 0;
    ENDTRY;

    return success;
}

typedef enum {
    DUMP_SHUTDOWN, DUMP_CHECKPOINT, DUMP_PANIC
} Dump_Reason;
const char *reason_names[] =
{"DUMPING", "CHECKPOINTING", "PANIC-DUMPING"};

static int
dump_database(Dump_Reason reason)
{
    Stream *s = new_stream(100);
    char *temp_name;
    FILE *f;
    int success;

  retryDumping:

    stream_printf(s, "%s.#%d#", dump_db_name, dump_generation);
    remove(reset_stream(s));	/* Remove previous checkpoint */

    if (reason == DUMP_PANIC)
	stream_printf(s, "%s.PANIC", dump_db_name);
    else {
	dump_generation++;
	stream_printf(s, "%s.#%d#", dump_db_name, dump_generation);
    }
    temp_name = reset_stream(s);

    oklog("%s on %s ...\n", reason_names[reason], temp_name);

#ifdef UNFORKED_CHECKPOINTS
    reset_command_history();
#else
    if (reason == DUMP_CHECKPOINT) {
	switch (fork_server("checkpointer")) {
	case FORK_PARENT:
	    reset_command_history();
	    free_stream(s);
	    return 1;
	case FORK_ERROR:
	    free_stream(s);
	    return 0;
	case FORK_CHILD:
	    set_server_cmdline("(MOO checkpointer)");
	    break;
	}
    }
#endif

    success = 1;
    if ((f = fopen(temp_name, "w")) != 0) {
	dbpriv_set_dbio_output(f);
	if (!write_db_file(reason_names[reason])) {
	    log_perror("Trying to dump database");
	    fclose(f);
	    remove(temp_name);
	    if (reason == DUMP_CHECKPOINT) {
		errlog("Abandoning checkpoint attempt...\n");
		success = 0;
	    } else {
		int retry_interval = 60;

		errlog("Waiting %d seconds and retrying dump...\n",
		       retry_interval);
		timer_sleep(retry_interval);
		goto retryDumping;
	    }
	} else {
	    fflush(f);
	    fsync(fileno(f));
	    fclose(f);
	    oklog("%s on %s finished\n", reason_names[reason], temp_name);
	    if (reason != DUMP_PANIC) {
		remove(dump_db_name);
		if (rename(temp_name, dump_db_name) != 0) {
		    log_perror("Renaming temporary dump file");
		    success = 0;
		}
	    }
	}
    } else {
	log_perror("Opening temporary dump file");
	success = 0;
    }

    free_stream(s);

#ifndef UNFORKED_CHECKPOINTS
    if (reason == DUMP_CHECKPOINT)
	/* We're a child, so we'd better go away. */
	exit(!success);
#endif

    return success;
}


/*********** External interface ***********/

const char *
db_usage_string(void)
{
    return "input-db-file output-db-file";
}

static FILE *input_db;

int
db_initialize(int *pargc, char ***pargv)
{
    FILE *f;

    if (*pargc < 2)
	return 0;

    input_db_name = str_dup((*pargv)[0]);
    dump_db_name = str_dup((*pargv)[1]);
    *pargc -= 2;
    *pargv += 2;

    if (!(f = fopen(input_db_name, "r"))) {
	fprintf(stderr, "Cannot open input database file: %s\n",
		input_db_name);
	return 0;
    }
    input_db = f;
    dbpriv_build_prep_table();

    return 1;
}

int
db_load(void)
{
    dbpriv_set_dbio_input(input_db);

    str_intern_open(0);

    oklog("LOADING: %s\n", input_db_name);
    if (!read_db_file()) {
	errlog("DB_LOAD: Cannot load database!\n");
	return 0;
    }
    oklog("LOADING: %s done, will dump new database on %s\n",
	  input_db_name, dump_db_name);

    str_intern_close();

    fclose(input_db);
    return 1;
}

int
db_flush(enum db_flush_type type)
{
    int success = 0;

    switch (type) {
    case FLUSH_IF_FULL:
    case FLUSH_ONE_SECOND:
	success = 1;
	break;

    case FLUSH_ALL_NOW:
	success = dump_database(DUMP_CHECKPOINT);
	break;

    case FLUSH_PANIC:
	success = dump_database(DUMP_PANIC);
	break;
    }

    return success;
}

int64_t
db_disk_size(void)
{
    struct stat st;

    if ((dump_generation == 0 || stat(dump_db_name, &st) < 0)
	&& stat(input_db_name, &st) < 0)
	return -1;
    else
	return st.st_size;
}

void
db_shutdown()
{
    dump_database(DUMP_SHUTDOWN);

    free_str(input_db_name);
    free_str(dump_db_name);
}

char rcsid_db_file[] = "$Id$";

/*
 * $Log$
 * Revision 1.7  2010/04/22 21:29:18  wrog
 * Free database name strings on shutdown (rob@mars.org)
 * current_version -> current_db_version
 *
 * Revision 1.6  2007/11/12 11:17:03  wrog
 * sync so that checkpoint is physically written before prior checkpoint is unlinked
 *
 * Revision 1.5  2004/05/22 01:25:43  wrog
 * merging in WROGUE changes (W_SRCIP, W_STARTUP, W_OOB)
 *
 * Revision 1.4.8.2  2003/06/03 12:21:17  wrog
 * new validation algorithms for cycle-detection and hierarchy checking
 *
 * Revision 1.4.8.1  2003/06/01 12:27:35  wrog
 * added braces and fixed indentation on TRY
 *
 * Revision 1.4  1998/12/14 13:17:33  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.3  1998/02/19 07:36:16  nop
 * Initial string interning during db load.
 *
 * Revision 1.2  1997/03/03 04:18:27  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:44:59  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.5  1996/04/08  01:07:21  pavel
 * Changed a boot-time error message to go directly to stderr, instead of
 * through the logging package.  Release 1.8.0p3.
 *
 * Revision 2.4  1996/02/08  07:20:18  pavel
 * Renamed err/logf() to errlog/oklog().  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.3  1995/12/31  03:27:54  pavel
 * Added missing #include "options.h".  Release 1.8.0alpha4.
 *
 * Revision 2.2  1995/12/28  00:51:39  pavel
 * Added db_disk_size().  Added support for printing location of
 * MOO-compilation warnings and errors during loading.  More slight
 * improvements to load-time progress messages.  Added dump-time progress
 * messages.  Added init-time call to build preposition table.
 * Release 1.8.0alpha3.
 *
 * Revision 2.1  1995/12/11  07:55:01  pavel
 * Added missing #include of "my-stdlib.h".  Slightly improved clarity of the
 * progress messages during DB loading.
 *
 * Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  04:19:37  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.1  1995/11/30  04:19:11  pavel
 * Initial revision
 */
