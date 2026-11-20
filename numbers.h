/******************************************************************************
  Copyright (c) 1996 Xerox Corporation.  All rights reserved.
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

#ifndef Numbers_H
#define Numbers_H 1

#include "structures.h"

extern enum error become_integer(Var, Num *, int);

extern int do_equals(Var, Var);
extern Var numeric_lt_or_eq(int, Var, Var);

extern Var do_add(Var, Var);
extern Var do_subtract(Var, Var);
extern Var do_multiply(Var, Var);
extern Var do_divide(Var, Var);
extern Var do_modulus(Var, Var);
extern Var do_power(Var, Var);

/*
 * parse_number(flags, c_first, getch, ungetch|NULL )
 *
 * parses and returns the next integer or float value,
 * possibly allowing whitespace, decimal points, exponents
 * or a leading #, in accordance with the specified
 *   flags   - keep reading
 * from an input stream defined by the last three arguments
 *   c_first - the first character
 *   getch() - advance input, return next character or EOF
 *   ungetch(c) - NULL or routine that pushes c back to input
 *
 * flags are as follows:
 */
#define PN_FLOAT_OK     0x1   /* parse '.' and 'E'/'e'        */
#define PN_REQ_FLOAT    0x2   /* force returning a float      */
#define PN_REQ_INT      0x4   /* force returning an integer   */
#define PN_NONNEG       0x8   /* non-negative numbers only,   */
          /*  i.e., do not allow leading signs; this also
	   *  changes the range of acceptable integers
	   *  and *may* make it possible for them to overflow
	   *  to NUM_MIN -- what the yacc parser needs.
           *  See Very Long Comment in numbers.c for details. */

/* The following can only be used when ungetch == NULL:       */
#define PN_LDSPACE     0x10   /* allow leading whitespace     */
#define PN_OCTOTHORPE  0x20   /* allow '#'                    */
#define PN_OCTOSPACE   0x40   /*   allow whitespace after '#' */
#define PN_TRSPACE    0x100   /* allow trailing whitespace    */
#define PN_MUST_EOF   0x200   /* must use all of the input    */
#define PN_SPACE     (PN_LDSPACE|PN_TRSPACE|PN_OCTOSPACE)
/*
 * The return value will be a Var with .type =
 *    TYPE_INT or TYPE_FLOAT if the parse was successful *and*
 *      the resulting value was within the representable range
 *      (integers must be in [NUM_MIN..NUM_MAX], or,
 *       if PN_NONNEG is set,   in [0..NUM_MAX+1];
 *       floats must have magnitude < HUGE_VAL)
 *    TYPE_ERROR otherwise
 *      .v.err = E_NONE if ungetch != NULL and no digits were seen
 *      .v.err = E_INVARG for all other syntax errors
 *      .v.err = E_RANGE or E_FLOAT for unrepresentable values
 *
 * as per the following chart:
 *
 *                             Value encountered
 *    PN_REQ_flags  !PN_FLOAT_OK or        PN_FLOAT_OK and
 *     specified    No '.' and no 'E'/'e'  found '.' or 'E'/'e'
 *   _____________||_____________________|________________________
 *                ||                     |  TYPE_INT or E_FLOAT
 *    PN_REQ_INT  ||    TYPE_INT         |  (float is truncated)
 *   _____________|| _   or E_RANGE   _  |________________________
 *     neither    ||                     |
 *   -------------||---------------------+     TYPE_FLOAT
 *   PN_REQ_FLOAT ||  (int is floated)   .      or E_FLOAT
 *
 * If ungetch() is non-NULL:
 *  (1)  E_NONE returned means the parser has fully backtracked,
 *       i.e., the next getch() would return c_first.
 *  (2)  The parser will backtrack out of '..'
 *       and certain bitwise operator situations, meaning,
 *       that '..' will always be treated as a distinct token
 *       and not included in any number so that, e.g.,
 *         "1.." parses as:     [integer 1] ['..']
 *         "1...2" parses as:   [integer 1] ['..'] [float 0.2]
 *         "1....2" parses as:  [integer 1] ['..'] ['..']
 */
extern Var
parse_number(unsigned flags,
	     int32_t c_first,
	     int32_t (*getch)(void),
	     void (*ungetch)(int32_t));

/*
 * parse_number_from_string(string, flags)
 *
 * attempt to parse string as a number,
 * flags are as for parse_number
 * but with PN_MUST_EOF always set.
 */
extern Var parse_number_from_string(const char *, unsigned);


#endif		/* !Numbers_H */

/*
 * $Log$
 * Revision 1.1  1996/02/08  07:28:25  pavel
 * Initial revision
 *
 */
