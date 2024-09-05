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

extern void dbio_skip_lines(size_t n);
				/* Read and discard N lines of input */

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
 *---------------------------*/

extern intmax_t dbio_read_integer(enum dbio_intrange);
                /* a.k.a. dbio_read_intmax() dbio_read_num()
		 *        dbio_read_int16() dbio_read_uint16()
		 *        dbio_read_int() dbio_read_uint()
		 *
		 * Reads an integer, does format and range checking.
		 * (uses str_to_imax() to read all available digits
		 * even for (u)int16.
		 *
		 * Also calls errlog() on errors
		 * (so caller does not have to).
		 */

#   define DBIO_DO_(INTXX,intxx_t,intxx,_4)			\
								\
inline intxx_t dbio_read_##intxx(void) {			\
    return (intxx_t)dbio_read_integer(DBIO_RANGE_##INTXX);	\
}								\

    DBIO_RANGE_SPEC_LIST(DBIO_DO_)
#   undef DBIO_DO_


extern Objid dbio_read_objid(void);
extern double dbio_read_float(void);

extern const char *dbio_read_string(void);
				/* The returned string is in private storage of
				 * the DBIO module, so the caller should
				 * str_dup() it if it is to persist.
				 */

extern const char *dbio_read_string_intern(void);
				/* The returned string is duplicated
				 * and possibly interned in a db-load
				 * string intern table.
				 */

extern Var dbio_read_var(void);
extern Var dbio_read_var_value(intmax_t vtype);
                                /* dbio_read_var() reads the type first,
				 * then the value;  dbio_read_var_value(type)
				 * reads a value given the type.
				 *
				 * In both cases, the DBIO module retains no
				 * references to the returned value, so freeing
				 * it is the responsibility of the caller.
				 */

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
