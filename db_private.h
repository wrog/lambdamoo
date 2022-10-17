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
 * Private interface for internal communication in the DB implementation
 *****************************************************************************/

#ifndef DB_Private_H
#define DB_Private_H 1

#include "config.h"
#include "exceptions.h"
#include "program.h"
#include "structures.h"

typedef struct Verbdef Verbdef;

struct Verbdef {
    const char *name;
    Program *program;
    Objid owner;
    short perms;
    short prep;
    Verbdef *next;
};

typedef struct Proplist Proplist;
typedef struct Propdef Propdef;

struct Propdef {
    const char *name;
    int hash;
};

struct Proplist {
    int max_length;
    int cur_length;
    Propdef *l;
};

typedef struct Pval {
    Var var;
    Objid owner;
    short perms;
} Pval;

typedef struct Object {
    Objid id;
    Objid owner;
    Objid location;
    Objid contents;
    Objid next;

    Objid parent;
    Objid child;
    Objid sibling;


    const char *name;
    int flags;

    Verbdef *verbdefs;
    Proplist propdefs;
    Pval *propval;
} Object;

/*********** Verb cache support ***********/

#define VERB_CACHE 1

#ifdef VERB_CACHE

/* Whenever anything is modified that could influence callable verb
 * lookup, this function must be called.
 */

#ifdef RONG
#define db_priv_affected_callable_verb_lookup() (db_verb_generation++)
                                 /* The choice of a new generation. */
extern unsigned int db_verb_generation;
#endif

extern void db_priv_affected_callable_verb_lookup(void);

#else /* no cache */
#define db_priv_affected_callable_verb_lookup()
#endif

/*********** Objects ***********/

extern void dbpriv_set_all_users(Var);
				/* Initialize the list returned by
				 * db_all_users().
				 */

extern Object *dbpriv_new_object(void);
				/* Creates a new object, assigning it a number,
				 * but doesn't fill in any of the fields other
				 * than `id'.
				 */

extern void dbpriv_new_recycled_object(void);
				/* Does the equivalent of creating and
				 * destroying an object, with the net effect of
				 * using up the next available object number.
				 */

extern Object *dbpriv_find_object(Objid);
				/* Returns 0 if given object is not valid.
				 */

/*********** Properties ***********/

extern Propdef dbpriv_new_propdef(const char *name);

extern int dbpriv_count_properties(Objid);

extern int dbpriv_check_properties_for_chparent(Objid oid,
						Objid new_parent);
				/* Return true iff NEW_PARENT defines no
				 * properties that are also defined by either
				 * OID or any of OID's descendants.
				 */

extern void dbpriv_fix_properties_after_chparent(Objid oid,
						 Objid old_parent);
				/* OID has just had its parent changed away
				 * from OLD_PARENT.  Fix up the properties of
				 * OID and its descendants, removing obsolete
				 * ones and adding clear new ones, as
				 * appropriate for its new parent.
				 */

/*********** Verbs ***********/

extern void dbpriv_build_prep_table(void);
				/* Should be called once near the beginning of
				 * the world, to initialize the
				 * prepositional-phrase matching table.
				 */

/*********** DBIO ***********/

extern Exception dbpriv_dbio_failed;
				/* Raised by DBIO in case of failure (e.g.,
				 * running out of disk space for the dump).
				 */

extern void dbpriv_set_dbio_input(FILE *);
extern void dbpriv_set_dbio_output(FILE *);

#endif		/* !DB_Private_H */

/*
 * $Log$
 * Revision 1.4  1998/12/14 13:17:37  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.3  1997/07/07 03:24:53  nop
 * Merge UNSAFE_OPTS (r5) after extensive testing.
 *
 * Revision 1.2  1997/03/03 04:18:30  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:02  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.3  1996/02/08  06:27:28  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.2  1995/12/31  03:18:05  pavel
 * Removed a few more uses of `unsigned'.  Release 1.8.0alpha4.
 *
 * Revision 2.1  1995/12/28  00:57:51  pavel
 * Added dbpriv_build_prep_table().  Release 1.8.0alpha3.
 *
 * Revision 2.0  1995/11/30  05:05:53  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.1  1995/11/30  05:05:38  pavel
 * Initial revision
 */
