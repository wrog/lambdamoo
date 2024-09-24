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

#ifndef Structures_H
#define Structures_H 1

#include "my-stdio.h"

#include "config.h"
#include "options.h"


/***********
 * Numbers
 */

#if INT_TYPE_BITSIZE == 64

typedef  int64_t    Num;
typedef uint64_t   UNum;
#  define PRIdN	   PRId64
#  define SCNdN_   SCNd64
#  define NUM_MAX  INT64_MAX
#  define NUM_MIN  INT64_MIN

#  if HAVE_INT128_T
/*
 *  I was originally going to insist 'Num' be called something else,
 *  but I decided to let that go.  This is your pennance:    --wrog
 */
#    define HAVE_UNUMNUM_T 1
typedef  int128_t   NumNum;
typedef uint128_t  UNumNum;
#  endif

#elif INT_TYPE_BITSIZE == 32

typedef  int32_t    Num;
typedef uint32_t   UNum;
#  define PRIdN	   PRId32
#  define SCNdN_   SCNd32
#  define NUM_MAX  INT32_MAX
#  define NUM_MIN  INT32_MIN

#  if HAVE_INT64_T
#    define HAVE_UNUMNUM_T 1
typedef  int64_t   NumNum;
typedef uint64_t  UNumNum;
#  endif

#elif INT_TYPE_BITSIZE == 16

/* Oh sure, why not.  Probably NOT something anyone will want
 * for production, but possibly useful for debugging/testing
 * ... and pranks.
 */
typedef  int16_t    Num;
typedef uint16_t   UNum;
#  define PRIdN	   PRId16
#  define SCNdN_   SCNd16
#  define NUM_MAX  INT16_MAX
#  define NUM_MIN  INT16_MIN

#  if HAVE_INT32_T
#    define HAVE_UNUMNUM_T 1
typedef  int64_t   NumNum;
typedef uint64_t  UNumNum;
#  endif

#else
#  error "?? bad INT_TYPE_BITSIZE not handled in options_epilog.h ??"
#endif        /* INT_TYPE_BITSIZE */

#define SCNdN  SCNdN_"\a"
/* For dbio_scxnf() only.  (scanf() is no longer used here.)
 * Extra character distinguishes this from other uses of SCNd##
 * in a way that won't screw up typechecking.
 */


/***********
 * Objects
 *
 * Note:  It's a pretty hard assumption in MOO that integers and objects
 * are the same data type.
 */

typedef Num     Objid;
#define OBJ_MAX	NUM_MAX

/*
 * Special Objid's
 */
#define SYSTEM_OBJECT	0
#define NOTHING		-1
#define AMBIGUOUS	-2
#define FAILED_MATCH	-3


/***********
 * Errors
 */

/* Do not reorder or otherwise modify this list, except to add new elements at
 * the end, since the order here defines the numeric equivalents of the error
 * values, and those equivalents are both DB-accessible knowledge and stored in
 * raw form in the DB.
 */
enum error {
    E_NONE, E_TYPE, E_DIV, E_PERM, E_PROPNF, E_VERBNF, E_VARNF, E_INVIND,
    E_RECMOVE, E_MAXREC, E_RANGE, E_ARGS, E_NACC, E_INVARG, E_QUOTA, E_FLOAT
};


/***********
 * Task IDs
 *
 * Note that this datatype must be either Num or a subrange.
 */

#define TASK_MIN 0

#if INT32_MAX < NUM_MAX

/* Admittedly, the extra space taken up by int64_t task ids
 * would not kill us, but are we ever really going to get
 * to a billion+ tasks in a single server run?
 * Survey sez NO.
 */
typedef  int32_t    TaskID;
#  define PRIdT	    PRId32
#  define SCNdT_    SCNd32
#  define TASK_MAX  INT32_MAX

/* This is only used for searching */
inline TaskID task_id_from_num(Num n) {
    return (TaskID)(n < 0 || n > INT32_MAX ? 0 : n);
}

#else
typedef   Num       TaskID;
#  define PRIdT	    PRIdN
#  define SCNdT_    SCNdN_
#  define TASK_MAX  NUM_MAX

inline TaskID task_id_from_num(Num n) {
    return n < 0 ? 0 : n;
}
#endif

#define SCNdT  SCNdT_"\b"

inline Num num_from_task_id(TaskID t) { return t; }

/* typedef struct task_id_ *TaskID;
inline TaskID task_id_from_num(Num n) { return (TaskID)(void *)(uintmax_t)n; }
inline Num num_from_task_id(TaskID t) { return (Num)(uintmax_t)(void *)t; }
 */

/****************
 * General Types
 */

/* Types which have external data should be marked with the TYPE_COMPLEX_FLAG
 * so that free_var/var_ref/var_dup can recognize them easily.  This flag is
 * only set in memory.  The original _TYPE values are used in the database
 * file and returned to verbs calling typeof().  This allows the inlines to
 * be extremely cheap (both in space and time) for simple types like oids
 * and ints.
 */
#define TYPE_DB_MASK		0x7f
#define TYPE_COMPLEX_FLAG	0x80

/* Do not reorder or otherwise modify the first part of this list
 * (up to "add new elements here"), since the order here defines the
 * numeric equivalents of the type values, and those equivalents are both
 * DB-accessible knowledge and stored in raw form in the DB.
 */
typedef enum {
    TYPE_INT, TYPE_OBJ, _TYPE_STR, TYPE_ERR, _TYPE_LIST, /* user-visible */
    TYPE_CLEAR,			/* in clear properties' value slot */
    TYPE_NONE,			/* in uninitialized MOO variables */
    TYPE_CATCH,			/* on-stack marker for an exception handler */
    TYPE_FINALLY,		/* on-stack marker for a TRY-FINALLY clause */
    _TYPE_FLOAT,		/* floating-point number; user-visible */
    /* add new elements here */

    TYPE_STR   = (_TYPE_STR   | TYPE_COMPLEX_FLAG),
    TYPE_LIST  = (_TYPE_LIST  | TYPE_COMPLEX_FLAG),
    TYPE_FLOAT = (_TYPE_FLOAT),

    TYPE_ANY     = -1,	/* wildcard for use in declaring built-ins */
    TYPE_NUMERIC = -2	/* wildcard for (integer or float) */
} var_type;


/*********
 * Vars
 */

typedef struct Var Var;

/* Experimental.  On the Alpha, DEC cc allows us to specify certain
 * pointers to be 32 bits, but only if we compile and link with "-taso
 * -xtaso" in CFLAGS, which limits us to a 31-bit address space.  This
 * could be a win if your server is thrashing.  Running JHM's db, SIZE
 * went from 50M to 42M.  No doubt these pragmas could be applied
 * elsewhere as well, but I know this at least manages to load and run
 * a non-trivial db.
 */

/* #define SHORT_ALPHA_VAR_POINTERS 1 */

#ifdef SHORT_ALPHA_VAR_POINTERS
#pragma pointer_size save
#pragma pointer_size short
#endif

struct Var {
    union {
	const char *str;	/* STR */
	Num num;		/* NUM, CATCH, FINALLY */
	Objid obj;		/* OBJ */
	enum error err;		/* ERR */
	Var *list;		/* LIST */
	double fnum;		/* FLOAT */
    } v;
    var_type type;
};

#ifdef SHORT_ALPHA_VAR_POINTERS
#pragma pointer_size restore
#endif

extern Var zero;		/* useful constant */

/*
 * Hard limits on string and list sizes are imposed mainly to keep
 * malloc calculations from rolling over, and thus preventing the
 * ensuing buffer overruns.  Sizes allow space for reference counts
 * and cached length values.  Actual limits imposed on
 * user-constructed lists and strings should generally be smaller
 * (see DEFAULT_MAX_LIST_CONCAT and DEFAULT_MAX_STRING_CONCAT
 *  in options.h)
 */
#if NUM_MAX > INT16_MAX
#  define MAX_LIST   (INT32_MAX/sizeof(Var) - 2)
#  define MAX_STRING (INT32_MAX - 9)
#else

/* In 16-bit land, length having to be a Num is the biggest
 * constraint.  Subtract 1 to shut up the compiler about
 * certain comparisons always evaluating true.
 */
#  define MAX_LIST   (NUM_MAX - 1)
#  define MAX_STRING (NUM_MAX - 1)
#endif

#endif		/* !Structures_H */

/*
 * $Log$
 * Revision 1.5  2010/04/22 21:56:28  wrog
 * Fix for-statement infinite loop bug (rob@mars.org)
 * add MAX_LIST and MAX_STRING
 *
 * Revision 1.4  1998/12/14 13:19:04  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.3  1997/07/07 03:24:55  nop
 * Merge UNSAFE_OPTS (r5) after extensive testing.
 *
 * Revision 1.2.2.2  1997/05/23 07:01:30  nop
 * Added experimental support for 32-bit pointer model on Alpha with DEC cc.
 *
 * Revision 1.2.2.1  1997/03/20 18:07:52  bjj
 * Add a flag to the in-memory type identifier so that inlines can cheaply
 * identify Vars that need actual work done to ref/free/dup them.  Add the
 * appropriate inlines to utils.h and replace old functions in utils.c with
 * complex_* functions which only handle the types with external storage.
 *
 * Revision 1.2  1997/03/03 04:19:29  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:12:21  pavel
 * Added E_FLOAT, TYPE_FLOAT, and TYPE_NUMERIC.  Renamed TYPE_NUM to TYPE_INT.
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:55:46  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.12  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.11  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.10  1992/09/14  17:40:51  pjames
 * Moved db_modification code to db modules.
 *
 * Revision 1.9  1992/09/04  01:17:29  pavel
 * Added support for the `f' (for `fertile') bit on objects.
 *
 * Revision 1.8  1992/09/03  16:25:12  pjames
 * Added TYPE_CLEAR for Var.
 * Changed Property definition lists to be arrays instead of linked lists.
 *
 * Revision 1.7  1992/08/31  22:25:04  pjames
 * Changed some `char *'s to `const char *'
 *
 * Revision 1.6  1992/08/14  00:00:36  pavel
 * Converted to a typedef of `var_type' = `enum var_type'.
 *
 * Revision 1.5  1992/08/10  16:52:00  pjames
 * Moved several types/procedure declarations to storage.h
 *
 * Revision 1.4  1992/07/30  21:24:31  pjames
 * Added M_COND_ARM_STACK and M_STMT_STACK for vector.c
 *
 * Revision 1.3  1992/07/28  17:18:48  pjames
 * Added M_COND_ARM_STACK for unparse.c
 *
 * Revision 1.2  1992/07/27  18:21:34  pjames
 * Changed name of ct_env to var_names, const_env to literals and
 * f_vectors to fork_vectors, removed M_CT_ENV, M_LOCAL_ENV, and
 * M_LABEL_MAPS, changed M_CONST_ENV to M_LITERALS, M_IM_STACK to
 * M_EXPR_STACK, M_F_VECTORS to M_FORK_VECTORS, M_ID_LIST to M_VL_LIST
 * and M_ID_VALUE to M_VL_VALUE.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
