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
extern int compare_integers(Num, Num);
extern Var compare_numbers(Var, Var);

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
 *   getch() - advance, return next character or EOF if done
 *   ungetch(c) - may be NULL; if not, push c (!=EOF) to input
 *
 * flags are as follows:
 */
#define PN_FLOAT_OK    0x01   /* include decimals + exponents */
#define PN_REQ_FLOAT   0x02   /* force returning a float      */
#define PN_REQ_INT     0x04   /* force returning an integer   */
#define PN_NONNEG      0x08   /* non-negative numbers only,   */
          /*  i.e., do not accept leading minus signs; this
           *  also changes the range of acceptable integers
	   *  and *may* make it possible for them to overflow
	   *  to NUM_MIN -- what the yacc parser needs.
           *  See Very Long Comment in numbers.c for details. */

/* The following should only be used when ungetch == NULL     */
#define PN_OCTOTHORPE  0x10   /* a leading # is allowed       */
#define PN_LDSPACE     0x20   /* allow leading whitespace     */
#define PN_TRSPACE     0x40   /* allow trailing whitespace    */
#define PN_OCTOSPACE   0x80   /* allow whitespace after #     */
#define PN_MUST_EOF   0x100   /* must use all of the input    */
#define PN_SPACE      PN_LDSPACE|PN_TRSPACE|PN_OCTOSPACE
/*
 * The return value will be a Var with .type =
 *    TYPE_INT   if no decimal./exponent was seen or
 *                 PN_REQ_INT was set (float value was truncated)
 *    TYPE_FLOAT if a decimal./exponent was seen or
 *                 PN_REQ_FLOAT was set (integer value was floated)
 *    TYPE_ERROR if the parse was unsuccessful for some reason
 *      .v.err = E_INVARG for syntax errors
 *      .v.err = E_RANGE if the parse saw a syntactically correct
 *               number outside of the representable range
 *               (integers must be in [NUM_MIN..NUM_MAX], or,
 *                if PN_NONNEG is set,   in [0..NUM_MAX+1];
 *                floats must have magnitude < HUGE_VAL)
 *      .v.err = E_NONE if ungetch != NULL and no digits were seen
 *
 * If ungetch() is non-NULL:
 *  (1)  E_NONE returned means the parser has fully backtracked,
 *       i.e., the next getch() would return c_first.
 *  (2)  The parser will backtrack out of '..',
 *       meaning '..' will always be treated as a distinct token
 *       and not included in any number (so that
 *         "1.." parses as:     [integer 1] ['..']
 *         "1...2" parses as:   [integer 1] ['..'] [float 0.2]
 *         "1....2" parses as:  [integer 1] ['..'] ['..']
 *       with that last one likely being a syntax error)
 */
extern Var
parse_number(unsigned flags, int32_t c_first, int32_t (*getch)(void), void (*ungetch)(int32_t));


#endif		/* !Numbers_H */

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:18:38  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:19:11  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 1.1  1996/02/08  07:28:25  pavel
 * Initial revision
 *
 */
