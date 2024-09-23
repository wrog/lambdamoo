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
 * Routines for use by non-DB modules with persistent state stored in the DB
 *****************************************************************************/

#ifndef DB_IO_h
#define DB_IO_h 1

#include "program.h"
#include "structures.h"
#include "version.h"

/*********** Input ***********/

extern DB_Version dbio_input_version;
				/* What DB-format version are we reading? */

extern int dbio_peek_byte(void);
                                /* Does one-byte lookahead.
				 * Returns 0-255 or EOF.
				 */

extern int dbio_skip_lines(size_t n, const char *caller);
				/* Read and discard N lines of input.
				 * Return true iff successful.
				 * errlog() about failure in CALLER
				 * otherwise.
				 */

extern int dbio_scanf(const char *format,...) FORMAT(scanf,1,2);

/*--------------------------*
 |  integer range checking  |
 *--------------------------*
 *  DEF(<XX>, <xx_t>, <fn>, <SCNxx>)
 *    declares the enumeration constant 'DBIO_RANGE_<XX>'
 *    to refer to the integer type '<xx_t>',
 *    the corresponding reader function to be 'dbio_read_<fn>',
 *    and the dbio_scxnf() conversion to be %"<SCNxx>"
 *    (and if that value is quoted here, then it's unquoted
 *    in the actual format string and vice versa, e.g.,
 *    use '%jd' for intmax_t, but '%"SCNdN"' for Num).
 */
#define DBIO_RANGE_SPEC_LIST(DEF)		\
    DEF(INTMAX, intmax_t, intmax,    "jd")	\
    DEF(NUM,         Num, num,      SCNdN)	\
    DEF(INT16,   int16_t, int16,   SCNd16)	\
    DEF(UINT16, uint16_t, uint16,  SCNu16)	\
    DEF(INT,         int, int,        "d")	\
    DEF(UINT,   unsigned, uint,       "u")	\

enum dbio_intrange {
#   define DBIO_DO_(INTXX,_2,_3,_4)  DBIO_RANGE_##INTXX,

    DBIO_RANGE_SPEC_LIST(DBIO_DO_)
    DBIO_RANGE__LENGTH

#   undef DBIO_DO_
};

/*---------------------------*
 |  dbio_read_*() functions  |
 *---------------------------*
 *  The dbio_read_* functions (NOT including dbio_read_program) read
 *  individual data items, and, when successful, assign the value read
 *  to the designated lvalue (*P) argument and return non-zero.
 *
 *  When an error or premature EOF occurs, the return value is zero
 *  with an error message being written to the server log, in which
 *  case you should not count on (*P) to be anything in particular.
 */

extern int dbio_read_integer(enum dbio_intrange r, intmax_t *p);
                /* a.k.a. dbio_read_intmax(p)  dbio_read_num(p)
		 *        dbio_read_int16(p)   dbio_read_uint16(p)
		 *        dbio_read_int(p)     dbio_read_uint(p)
		 *
		 * Reads an integer, with range checking (uses
		 * str_to_imax() to read all available digits even for
		 * (u)int16); the value read is assigned via the last
		 * argument.
		 */

#   define DBIO_DO_(INTXX,intxx_t,intxx,_4)		\
							\
inline int dbio_read_##intxx(intxx_t *p) {		\
    intmax_t i = 0;					\
    int r = dbio_read_integer(DBIO_RANGE_##INTXX, &i);	\
    *p = (intxx_t)i;					\
    return r;						\
}							\

    DBIO_RANGE_SPEC_LIST(DBIO_DO_)
#   undef DBIO_DO_

extern int dbio_read_objid(Objid *);

/* For this function, any string assigned will always be in private
 * DBIO module storage, to be overwritten by the next dbio_read*()
 * call; caller must make its own copy if persistence is needed.
 */
extern int dbio_read_string(const char **);

/*  For the following functions, in the event of a succesful
 *  return (>0) the caller is responsible for eventually freeing
 *  any allocated data via free_str() or free_var(), depending.
 *  For an error return (==0), all such memory will have already
 *  been reclaimed and any Var or char* assigned here can
 *  (and must) be safely ignored.
 */
extern int dbio_read_float(Var *);
extern int dbio_read_string_intern(const char **);

extern int dbio_read_var(Var *v);
extern int dbio_read_var_value(intmax_t vtype, Var *v);
                                /* dbio_read_var() reads a type (integer) first,
				 * then the value;
				 * dbio_read_var_value(type) is for reading
				 * a value where the type is already known.
				 */

/*---------------------*
 |  dbio_read_program  |
 *---------------------*  is different */

extern Program *dbio_read_program(DB_Version version,
				  const char *(*fmtr) (void *),
				  void *data);
				/* FMTR is called with DATA to produce a human-
				 * understandable identifier for the program
				 * being read, for use in any error/warning
				 * messages.  If FMTR is null, then DATA should
				 * be the required string.
				 */


/*********** Output ***********/

/* NOTE: All output routines can raise a (private) exception if they are unable
 * to write all of the requested output (e.g., because there is no more space
 * on disk).  The DB module catches this exception and retries the DB dump
 * after performing appropriate notifications, waiting, and/or fixes.  Callers
 * should thus be prepared for any call to these routines to fail to return
 * normally, using TRY ... FINALLY ... if necessary to recover from such an
 * event.
 */

extern void dbio_printf(const char *format,...) FORMAT(printf,1,2);

extern void dbio_write_intmax(intmax_t);
extern void dbio_write_objid(Objid);
extern void dbio_write_float(double);

extern void dbio_write_string(const char *);
				/* The given string should not contain any
				 * newline characters.
				 */

extern void dbio_write_var(Var);

extern void dbio_write_program(Program *);
extern void dbio_write_forked_program(Program * prog, int f_index);

#endif		/* !DB_IO_h */

/*
 * $Log$
 * Revision 1.4  1998/12/14 13:17:35  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.3  1998/02/19 07:36:16  nop
 * Initial string interning during db load.
 *
 * Revision 1.2  1997/03/03 04:18:28  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:02  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.3  1996/02/08  06:28:21  pavel
 * Added dbio_input_version, dbio_read/write_float().  Made dbio_read_program
 * version-dependent.  Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.2  1995/12/28  00:46:52  pavel
 * Added support for printing location of MOO-compilation warnings and errors
 * during loading.  Release 1.8.0alpha3.
 *
 * Revision 2.1  1995/12/11  08:00:11  pavel
 * Removed another silly use of `unsigned'.
 *
 * Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  05:05:29  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.1  1995/11/30  05:05:21  pavel
 * Initial revision
 */
