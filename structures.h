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

#include "config.h"
#include "options.h"

#include "my-math.h"
#include "my-stdio.h"
#include "my-stdlib.h"


/***********
 * Integers
 */

/* The definition of NUM_MAX lives in options_epilog.h
 * for reasons that are explained there.
 */

#if INT_TYPE_BITSIZE == 64

typedef  int64_t    Num;
typedef uint64_t   UNum;
#  define PRIdN	   PRId64
#  define PRIuN	   PRIu64
#  define SCNdN_   SCNd64
#  define SCNuN_   SCNu64
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
#  define PRIuN	   PRIu32
#  define SCNdN_   SCNd32
#  define SCNuN_   SCNu32
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
#  define PRIuN	   PRIu16
#  define SCNdN_   SCNd16
#  define SCNuN_   SCNu16
#  define NUM_MIN  INT16_MIN

#  if HAVE_INT32_T
#    define HAVE_UNUMNUM_T 1
typedef  int32_t   NumNum;
typedef uint32_t  UNumNum;
#  endif

#else
#  error "?? bad INT_TYPE_BITSIZE not handled in options_epilog.h ??"
#endif        /* INT_TYPE_BITSIZE */

#define SCNdN  SCNdN_"\a"
#define SCNuN  SCNuN_"\a"
/* For use in dbio_scxnf() only.  (scanf() is never used.)
 * Extra character following conversion spec allows these
 * to be distinguished from other uses of SCN### in a way
 * that won't screw up the FORMAT(scanf...) typechecking.
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


/***************
 * Floats
 */

#if FLOATING_TYPE == FT_QUAD

typedef  __float128  FlNum;
#  define FLOAT_FN(name)  name##q
#  define FLOAT_DEF(name) name##Q
#  define FLOAT_DIGITS    FLT128_DIG
#  define strtoflnum      strtoflt128
#  define PRIeR "Qe"
#  define PRIfR "Qf"
#  define PRIgR "Qg"

#elif FLOATING_TYPE == FT_LONG

typedef  long double  FlNum;
#  define FLOAT_FN(name)  name##l
#  define FLOAT_DEF(name) name##L
#  define FLOAT_DIGITS    LDBL_DIG
#  define strtoflnum      strtold
#  define PRIeR "Le"
#  define PRIfR "Lf"
#  define PRIgR "Lg"

#else  /* FLOATING_TYPE not in (FT_QUAD, FT_LONG) */

/* Elements common to FT_DOUBLE and FT_FLOAT
 * (thanks, default argument promotions).
 */
#  define PRIeR "e"
#  define PRIfR "f"
#  define PRIgR "g"

#  if FLOATING_TYPE == FT_FLOAT

typedef  float  FlNum;
#    define FLOAT_FN(name)  name##f
#    define FLOAT_DEF(name) name##F
#    define FLOAT_DIGITS    FLT_DIG
#    define strtoflnum      strtof

#  elif FLOATING_TYPE  == FT_DOUBLE

typedef  double  FlNum;
#    define FLOAT_FN(name)  name
#    define FLOAT_DEF(name) name
#    define FLOAT_DIGITS    DBL_DIG
#    define strtoflnum      strtod

#  else
#    error "options_epilog.h was supposed to catch this."
#  endif

#endif	/* FLOATING_TYPE not in (FT_QUAD, FT_LONG) */

#if !FLOATS_ARE_BOXED

typedef FlNum  FlBox;
inline  FlNum  fl_unbox(FlBox p) { return p; }
inline  FlBox  box_fl(  FlNum f) { return f; }

#else   /* FLOATS_ARE_BOXED */

typedef FlNum *FlBox;
inline  FlNum  fl_unbox(FlBox p) { return *p; }
extern  FlBox  box_fl(  FlNum f);

#endif	/* FLOATS_ARE_BOXED */

#define BQM_DESCRIBE_FlNum(B,F,V,X)     (2 * F)


/***********
 * Errors
 */

/* Do not reorder or otherwise modify this list, except to add new elements at
 * the end, since the order here defines the numeric equivalents of the error
 * values, and those equivalents are both DB-accessible knowledge and stored in
 * raw form in the DB.
 */
#define ERR_SPEC_LIST(DEF)				\
    DEF(E_NONE,    "No error")				\
    DEF(E_TYPE,    "Type mismatch")			\
    DEF(E_DIV,     "Division by zero")			\
    DEF(E_PERM,    "Permission denied")			\
    DEF(E_PROPNF,  "Property not found")		\
    DEF(E_VERBNF,  "Verb not found")			\
    DEF(E_VARNF,   "Variable not found")		\
    DEF(E_INVIND,  "Invalid indirection")		\
    DEF(E_RECMOVE, "Recursive move")			\
    DEF(E_MAXREC,  "Too many verb calls")		\
    DEF(E_RANGE,   "Range error")			\
    DEF(E_ARGS,    "Incorrect number of arguments")	\
    DEF(E_NACC,    "Move refused by destination")	\
    DEF(E_INVARG,  "Invalid argument")			\
    DEF(E_QUOTA,   "Resource limit exceeded")		\
    DEF(E_FLOAT,   "Floating-point arithmetic error")	\

#define ERROR_DO_(E_NAME,_2)  E_NAME,
enum error {
    ERR_SPEC_LIST(ERROR_DO_)
    ERR_COUNT,
    ERR_MAX = ERR_COUNT - 1,
    ERR_MIN = E_NONE
};
#undef ERROR_DO_


/***********
 * Task IDs
 *
 * Since task ID numbers occur in MOO-code,
 * this datatype must either be Num or a subrange.
 */

/* (negative taskIDs are deemed Bad but i don't know why. --wrog) */
#define TASK_MIN 0

#if INT32_MAX < NUM_MAX

/* Admittedly, the extra space taken up by int64_t task ids
 * would not kill us, but are we ever really going to get
 * to a billion+ tasks in a single server run?  Survey sez NO.
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
 *
 * Extension types not used in the base server/language should remain
 * visible here as placeholders in order that extensions can agree
 * on who uses which type numbers and thus can play well together
 * (in case you're wondering why TYPE_WAIF is not #ifdef'ed out).
 */
typedef enum {
    TYPE_INT, TYPE_OBJ, _TYPE_STR, TYPE_ERR, _TYPE_LIST, /* user-visible */
    TYPE_CLEAR,			/* in clear properties' value slot */
    TYPE_NONE,			/* in uninitialized MOO variables */
    TYPE_CATCH,			/* on-stack marker for an exception handler */
    TYPE_FINALLY,		/* on-stack marker for a TRY-FINALLY clause */
    _TYPE_FLOAT,		/* floating-point number; user-visible */
    _TYPE_WAIF,			/* lightweight object; user-visible */
    /* add new elements here */

    TYPE_STR   = (_TYPE_STR   | TYPE_COMPLEX_FLAG),
    TYPE_LIST  = (_TYPE_LIST  | TYPE_COMPLEX_FLAG),
    TYPE_WAIF  = (_TYPE_WAIF  | TYPE_COMPLEX_FLAG),
    TYPE_FLOAT = (_TYPE_FLOAT
#if FLOATS_ARE_BOXED
		  | TYPE_COMPLEX_FLAG
#endif
		  ),

    TYPE_ANY     = -1,	/* wildcard for use in declaring built-ins */
    TYPE_NUMERIC = -2	/* wildcard for (integer or float) */
} var_type;


/***********
 * Experimental.  On the Alpha, DEC cc allows us to specify certain
 * pointers to be 32 bits, but only if we compile and link with "-taso
 * -xtaso" in CFLAGS, which limits us to a 31-bit address space.  This
 * could be a win if your server is thrashing.  Running JHM's db, SIZE
 * went from 50M to 42M.  No doubt these pragmas could be applied
 * elsewhere as well, but I know this at least manages to load and run
 * a non-trivial db.
 *
 * ( History update:  Previous appears to be from Jay, circa 1997.
 *
 *   In the early 2000s, DEC Alpha was killed off.  It also seems the
 *   pointer_size pragma never caught on outside of DEC.  However,
 *   OpenVMS is evidently A Thing now, or at least as recently as
 *   2019, and its gcc/clang seems to know about pointer_size, so
 *   there remains a remote possibility that this still works
 *   (or, at least, can be made to work) somewhere.
 *
 *   pragmas were originally placed immediately surrounding struct
 *   Var, but they also included struct Waif on the waif branch,
 *   presumably deliberately.  To get this out of the way, I have
 *   moved it backwards into its own section... presumably safe
 *   since it should only affect struct/pointer declarations.
 *        --wrog      )
 *
  ................................
  :  begin DEC ALPHA experiment  :
  :
  :   #define SHORT_ALPHA_VAR_POINTERS 1
  */
#ifdef SHORT_ALPHA_VAR_POINTERS
#pragma pointer_size save
#pragma pointer_size short
#endif


/*********
 * Vars
 */

typedef struct Var Var;

/* insert forward declarations for extensions here */
typedef struct Waif Waif;

struct Var {
    union {
	const char *str;	/* STR */
	Num num;		/* NUM, CATCH, FINALLY */
	Objid obj;		/* OBJ */
	enum error err;		/* ERR */
	Var *list;		/* LIST */
	FlBox fnum;		/* FLOAT */
	Waif *waif;		/* WAIF */
    } v;
    var_type type;
};
#define BQM_DESCRIBE_Var(B,F,V,X)       (2 * V)

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
#  define MAX_LIST   (INT32_MAX/(Num)sizeof(Var) - 2)
#  define MAX_STRING (INT32_MAX - 9)
#else

/* In 16-bit land, length having to be a Num is the biggest
 * constraint.  Subtract 1 to shut up the compiler about
 * certain comparisons always evaluating true.
 */
#  define MAX_LIST   (NUM_MAX - 1)
#  define MAX_STRING (NUM_MAX - 1)
#endif


#ifdef SHORT_ALPHA_VAR_POINTERS
#pragma pointer_size restore
#endif
/*
 :  end of DEC ALPHA experiment  :
 :...............................:*/


/*********
 * Byte Quota Model
 *
 *   For use by value_bytes(), object_bytes(), and supporting routines.
 */

#if BYTE_QUOTA_MODEL == BQM_HW

/* Use actual-hardware numbers */
#  define BQM_SIZEOF(type)			sizeof(type)
#  define BQM_SIZEOF_PTR_TO(type)		sizeof(type *)
#  define BQM_SIZEOF_PTR_TO_CONST(type)		sizeof(const type *)

#else  /* BQM_64, BQM_64B, BQM_32 */
/*
 * Use one of the fixed-size legacy models, for which we need
 * each of the <type>s involved to have a correponding
 *
 *   #define BQM_DESCRIBE_<type>(B,F,V,X)  <expression>
 *
 * where <expression> is an integer expression to compute a size in bytes
 * that may include any of the following:
 *
 *    B(<base>)  size of type <base> (for when <type> bodily includes <base>)
 *    F          size of something that is always 4 bytes (like int32_t)
 *    V          size of something (e.g., a pointer) that is
 *        4 bytes in the BQM_32 world and
 *        8 bytes in the BQM_64(B) world
 *    X(<ext>,<expression>)
 *        where <ext> = cpp #define name for an extension and
 *        <expression> is another integer expression, possibly
 *        with B,F,V,X terms.  Evaluates to <expression> if
 *        the extension is considered active for the purposes
 *        of this byte quota model and otherwise 0.
 *        (see struct activation or struct Object for examples).
 *
 * See below for implementation details if you want to add a new
 * datatype, a new extension, or a new byte quota model.
 */

#  define BQM_SIZEOF_PTR_TO(type)        (BQM_VAR_)
#  define BQM_SIZEOF_PTR_TO_CONST(type)  (BQM_SIZEOF_PTR_TO(type))

/*  Want, but cannot have because the C preprocessor disables recursion (details below)
 *    #define BQM_SIZEOF(type)      BQM_DESCRIBE_##type(BQM_SIZEOF,     BQM_FOUR_,BQM_VAR_,BQM_EXT_)
 */
#  define BQM_SIZEOF(type)          BQM_DESCRIBE_##type(BQM_BASE_,      BQM_FOUR_,BQM_VAR_,BQM_EXT_)
#  define BQM_BASE_(type)           BQM_DESCRIBE_##type(BQM_BASE_##type,BQM_FOUR_,BQM_VAR_,BQM_EXT_)
#  define BQM_BASE_activation(type) BQM_DESCRIBE_##type(BQM_BASE_##type,BQM_FOUR_,BQM_VAR_,BQM_EXT_)

#  define BQM_FOUR_			((size_t)4)

#  if BYTE_QUOTA_MODEL == BQM_32
#    define BQM_VAR_		  	(BQM_FOUR_)

#  else /* BYTE_QUOTA_MODEL != BQM_32 */
#    define BQM_VAR_			(BQM_FOUR_ * 2)

#  endif

#  define BQM_EXT_(ext,adjust) 	BQM_##ext##_IFELSE((adjust),0)

#endif	 /* BYTE_QUOTA_MODEL != BQM_HW */

#endif		/* !Structures_H */


/* More on BYTE_QUOTA_MODEL:
 *
 * (1) If you are needing to add a new model,
 *     please make sure that, whatever you do, the prior models
 *     continue to produce the same numbers as before:
 *
 *       ./configure --enable-sz=bq32 (... whatever model you want)
 *       rm -f byte_quota_test
 *       make byte_quota_test && ./byte_quota_test
 *
 * (2) If you are adding a new type, it perhaps does not truly matter
 *     how big you declare it to be, since, presumably, none of the
 *     legacy dbs will have instances of it, but best practice would
 *     be to guess how big it *would* have been on typical early-2000s
 *     32- and 64- bit hardware, respectively.
 *
 * extensions that mess with structure sizes should
 * create a boolean option in options.ac and options.h.in
 *   BQM_<ext>
 *     (where <ext> is the #define name for when the the extension is
 *     active), which indicates whether the extension is active for
 *     the purposes of the byte quota model (regardless of whether the
 *     extension itself is indeed active),
 *
 * then make it default to the setting of <ext> (in options_epilog.h)
 *
 * and then add
 *
 *   #if BQM_<ext>
 *   # define BQM_<ext>_IFELSE(then,else)   then
 *   #else
 *   # define BQM_<ext>_IFELSE(then,else)   else
 *   #endif
 *
 * ------------------------------------
 * Re: wtf is going on with BQM_SIZEOF?
 * ------------------------------------
 * In case you were wondering, the problem here is that the C
 * preprocessor goes out of its way to disable recursion.
 * What we **want** is
 *
 *    #define BQM_SIZEOF(type)     \
 *       BQM_DESCRIBE_##type(BQM_SIZEOF, BQM_FOUR_,BQM_VAR_,BQM_EXT_)
 *
 * which does not work because, in any expansion of BQM_SIZEOF,
 * 'BQM_SIZEOF' will be on the list of identifiers not to be further
 * expanded, so that symbol comes out as itself, unexpanded, and
 * simply left there for the compiler to trip over.
 *
 * To make this work we need to give BQM_SIZEOF a different name
 * ('BQM_BASE_') in the expansion and then give that identifier the
 * *same* definition with yet another name in *its* expansion
 * ('BQM_BASE_##type'), and so on... As it happens, using
 * BQM_BASE_##type for the 2nd..nth time around gives us a wide
 * variety of names, and, furthermore, C forbids cycles of structs
 * including each other (you can have cycles of *pointers* but that's
 * different), so once we have definitions of BQM_BASE_<type> for all
 * types that this encounters, we're done.  But we don't even have to
 * do that since most types don't use B().  The only cases we have to
 * worry about are the length>=2 chains, the following being the only
 * one we have at the moment:
 *
 *     forked_task  uses B(activation)
 *     activation   uses B(Var)         (if #defined(WAIF_CORE))
 *     Var          does not use B()
 *
 * so this ends.  So, since activation is both referred to and
 * uses B() itself, we need a definition for BQM_BASE_activation()
 *
 * You can see this in action by commenting out the
 * BQM_BASE_activation #define and attempting a compile.
 * The error will be something like
 *
 *    "expected ‘)’ before ‘BQM_BASE_activation’",
 *
 * which tells you what needs to be #defined.  So if you should manage
 * to create more of these situations, now you know what to do.
 */


/*
 * $Log$
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
