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

#include <limits.h>
#include <errno.h>
#include <float.h>
#include "my-math.h"
#include "my-stdlib.h"
#include "my-string.h"
#include "my-time.h"

#include "config.h"
#include "functions.h"
#include "log.h"
#include "random.h"
#include "storage.h"
#include "structures.h"
#include "utils.h"

static int
parse_number(const char *str, int *result, int try_floating_point)
{
    char *p;

    *result = strtol(str, &p, 10);
    if (try_floating_point &&
	(p == str || *p == '.' || *p == 'e' || *p == 'E'))
	*result = (int) strtod(str, &p);
    if (p == str)
	return 0;
    while (*p) {
	if (*p != ' ')
	    return 0;
	p++;
    }
    return 1;
}

static int
parse_object(const char *str, Objid * result)
{
    int number;

    while (*str && *str == ' ')
	str++;
    if (*str == '#')
	str++;
    if (parse_number(str, &number, 0)) {
	*result = number;
	return 1;
    } else
	return 0;
}

static int
parse_float(const char *str, double *result)
{
    char *p;
    int negative = 0;

    while (*str && *str == ' ')
	str++;
    if (*str == '-') {
	str++;
	negative = 1;
    }
    *result = strtod(str, &p);
    if (p == str)
	return 0;
    while (*p) {
	if (*p != ' ')
	    return 0;
	p++;
    }
    if (negative)
	*result = -*result;
    return 1;
}

enum error
become_integer(Var in, int *ret, int called_from_tonum)
{
    switch (in.type) {
    case TYPE_INT:
	*ret = in.v.num;
	break;
    case TYPE_STR:
	if (!(called_from_tonum
	      ? parse_number(in.v.str, ret, 1)
	      : parse_object(in.v.str, ret)))
	    *ret = 0;
	break;
    case TYPE_OBJ:
	*ret = in.v.obj;
	break;
    case TYPE_ERR:
	*ret = in.v.err;
	break;
    case TYPE_FLOAT:
	if (*in.v.fnum < (double) INT_MIN || *in.v.fnum > (double) INT_MAX)
	    return E_FLOAT;
	*ret = (int) *in.v.fnum;
	break;
    case TYPE_LIST:
	return E_TYPE;
    default:
	errlog("BECOME_INTEGER: Impossible var type: %d\n", (int) in.type);
    }
    return E_NONE;
}

static enum error
become_float(Var in, double *ret)
{
    switch (in.type) {
    case TYPE_INT:
	*ret = (double) in.v.num;
	break;
    case TYPE_STR:
	if (!parse_float(in.v.str, ret) || !IS_REAL(*ret))
	    return E_INVARG;
	break;
    case TYPE_OBJ:
	*ret = (double) in.v.obj;
	break;
    case TYPE_ERR:
	*ret = (double) in.v.err;
	break;
    case TYPE_FLOAT:
	*ret = *in.v.fnum;
	break;
    case TYPE_LIST:
	return E_TYPE;
    default:
	errlog("BECOME_FLOAT: Impossible var type: %d\n", (int) in.type);
    }
    return E_NONE;
}

Var
new_float(double d)
{
    Var v;

    v.type = TYPE_FLOAT;
    v.v.fnum = mymalloc(sizeof(double), M_FLOAT);
    *v.v.fnum = d;

    return v;
}

#if COERCION_IS_EVER_IMPLEMENTED_AND_DESIRED
static int
to_float(Var v, double *dp)
{
    switch (v.type) {
    case TYPE_INT:
	*dp = (double) v.v.num;
	break;
    case TYPE_FLOAT:
	*dp = *v.v.fnum;
	break;
    default:
	return 0;
    }

    return 1;
}
#endif

#if defined(HAVE_MATHERR) && defined(DOMAIN) && defined(SING) && defined(OVERFLOW) && defined(UNDERFLOW)
/* Required in order to properly handle FP exceptions on SVID3 systems */
int
matherr(struct exception *x)
{
    switch (x->type) {
    case DOMAIN:
    case SING:
	errno = EDOM;
	/* fall thru to... */
    case OVERFLOW:
	x->retval = HUGE_VAL;
	return 1;
    case UNDERFLOW:
	x->retval = 0.0;
	return 1;
    default:
	return 0;		/* Take default action */
    }
}
#endif


/**** opcode implementations ****/

/*
 * All of the following implementations are strict, not performing any
 * coercions between integer and floating-point operands.
 */

int
do_equals(Var lhs, Var rhs)
{				/* LHS == RHS */
    /* At least one of LHS and RHS is TYPE_FLOAT */

    if (lhs.type != rhs.type)
	return 0;
    else
	return *lhs.v.fnum == *rhs.v.fnum;
}

int
compare_integers(int a, int b)
{
    if (a < b)
	return -1;
    else if (a == b)
	return 0;
    else
	return 1;
}

Var
compare_numbers(Var a, Var b)
{
    Var ans;

    if (a.type != b.type) {
	ans.type = TYPE_ERR;
	ans.v.err = E_TYPE;
    } else if (a.type == TYPE_INT) {
	ans.type = TYPE_INT;
	ans.v.num = compare_integers(a.v.num, b.v.num);
    } else {
	double aa = *a.v.fnum, bb = *b.v.fnum;

	ans.type = TYPE_INT;
	if (aa < bb)
	    ans.v.num = -1;
	else if (aa == bb)
	    ans.v.num = 0;
	else
	    ans.v.num = 1;
    }

    return ans;
}

#define SIMPLE_BINARY(name, op)					\
		Var						\
		do_ ## name(Var a, Var b)			\
		{						\
		    Var	ans;					\
								\
		    if (a.type != b.type) {			\
			ans.type = TYPE_ERR;			\
			ans.v.err = E_TYPE;			\
		    } else if (a.type == TYPE_INT) {		\
			ans.type = TYPE_INT;			\
			ans.v.num = a.v.num op b.v.num;		\
		    } else {					\
			double d = *a.v.fnum op *b.v.fnum;	\
								\
			if (!IS_REAL(d)) {			\
			    ans.type = TYPE_ERR;		\
			    ans.v.err = E_FLOAT;		\
			} else					\
			    ans = new_float(d);			\
		    }						\
								\
		    return ans;					\
		}

SIMPLE_BINARY(add, +)
SIMPLE_BINARY(subtract, -)
SIMPLE_BINARY(multiply, *)
#define DIVISION_OP(name, iop, fexpr)				\
		Var						\
		do_ ## name(Var a, Var b)			\
		{						\
		    Var	ans;					\
								\
		    if (a.type != b.type) {			\
			ans.type = TYPE_ERR;			\
			ans.v.err = E_TYPE;			\
		    } else if (a.type == TYPE_INT		\
			       && b.v.num != 0) {		\
			ans.type = TYPE_INT;			\
			ans.v.num = a.v.num iop b.v.num;	\
		    } else if (a.type == TYPE_FLOAT		\
			       && *b.v.fnum != 0.0) {		\
			double d = fexpr;			\
								\
			if (!IS_REAL(d)) {			\
			    ans.type = TYPE_ERR;		\
			    ans.v.err = E_FLOAT;		\
			} else					\
			    ans = new_float(d);			\
		    } else {					\
		        ans.type = TYPE_ERR;			\
			ans.v.err = E_DIV;			\
		    }						\
								\
		    return ans;					\
		}

DIVISION_OP(divide, /, *a.v.fnum / *b.v.fnum)
DIVISION_OP(modulus, %, fmod(*a.v.fnum, *b.v.fnum))
Var
do_power(Var lhs, Var rhs)
{				/* LHS ^ RHS */
    Var ans;

    if (lhs.type == TYPE_INT) {	/* integer exponentiation */
	int a = lhs.v.num, b, r;

	if (rhs.type != TYPE_INT)
	    goto type_error;

	b = rhs.v.num;
	ans.type = TYPE_INT;
	if (b < 0)
	    switch (a) {
	    case -1:
		ans.v.num = (b % 2 == 0 ? 1 : -1);
		break;
	    case 0:
		ans.type = TYPE_ERR;
		ans.v.err = E_DIV;
		break;
	    case 1:
		ans.v.num = 1;
		break;
	    default:
		ans.v.num = 0;
	} else {
	    r = 1;
	    while (b != 0) {
		if (b % 2 != 0)
		    r *= a;
		a *= a;
		b >>= 1;
	    }
	    ans.v.num = r;
	}
    } else if (lhs.type == TYPE_FLOAT) {	/* floating-point exponentiation */
	double d;

	switch (rhs.type) {
	case TYPE_INT:
	    d = (double) rhs.v.num;
	    break;
	case TYPE_FLOAT:
	    d = *rhs.v.fnum;
	    break;
	default:
	    goto type_error;
	}
	errno = 0;
	d = pow(*lhs.v.fnum, d);
	if (errno != 0 || !IS_REAL(d)) {
	    ans.type = TYPE_ERR;
	    ans.v.err = E_FLOAT;
	} else
	    ans = new_float(d);
    } else
	goto type_error;

    return ans;

  type_error:
    ans.type = TYPE_ERR;
    ans.v.err = E_TYPE;
    return ans;
}

/**** built in functions ****/

static package
bf_toint(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    enum error e;

    r.type = TYPE_INT;
    e = become_integer(arglist.v.list[1], &(r.v.num), 1);

    free_var(arglist);
    if (e != E_NONE)
	return make_error_pack(e);

    return make_var_pack(r);
}

static package
bf_tofloat(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    enum error e;

    r = new_float(0.0);
    e = become_float(arglist.v.list[1], r.v.fnum);

    free_var(arglist);
    if (e == E_NONE)
	return make_var_pack(r);

    free_var(r);
    return make_error_pack(e);
}

static package
bf_min(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    int i, nargs = arglist.v.list[0].v.num;
    int bad_types = 0;

    r = arglist.v.list[1];
    if (r.type == TYPE_INT) {	/* integers */
	for (i = 2; i <= nargs; i++)
	    if (arglist.v.list[i].type != TYPE_INT)
		bad_types = 1;
	    else if (arglist.v.list[i].v.num < r.v.num)
		r = arglist.v.list[i];
    } else {			/* floats */
	for (i = 2; i <= nargs; i++)
	    if (arglist.v.list[i].type != TYPE_FLOAT)
		bad_types = 1;
	    else if (*arglist.v.list[i].v.fnum < *r.v.fnum)
		r = arglist.v.list[i];
    }

    r = var_ref(r);
    free_var(arglist);
    if (bad_types)
	return make_error_pack(E_TYPE);
    else
	return make_var_pack(r);
}

static package
bf_max(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    int i, nargs = arglist.v.list[0].v.num;
    int bad_types = 0;

    r = arglist.v.list[1];
    if (r.type == TYPE_INT) {	/* integers */
	for (i = 2; i <= nargs; i++)
	    if (arglist.v.list[i].type != TYPE_INT)
		bad_types = 1;
	    else if (arglist.v.list[i].v.num > r.v.num)
		r = arglist.v.list[i];
    } else {			/* floats */
	for (i = 2; i <= nargs; i++)
	    if (arglist.v.list[i].type != TYPE_FLOAT)
		bad_types = 1;
	    else if (*arglist.v.list[i].v.fnum > *r.v.fnum)
		r = arglist.v.list[i];
    }

    r = var_ref(r);
    free_var(arglist);
    if (bad_types)
	return make_error_pack(E_TYPE);
    else
	return make_var_pack(r);
}

static package
bf_abs(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;

    r = var_dup(arglist.v.list[1]);
    if (r.type == TYPE_INT) {
	if (r.v.num < 0)
	    r.v.num = -r.v.num;
    } else
	*r.v.fnum = fabs(*r.v.fnum);

    free_var(arglist);
    return make_var_pack(r);
}

#define MATH_FUNC(name)							      \
		static package						      \
		bf_ ## name(Var arglist, Byte next, void *vdata, Objid progr) \
		{							      \
		    double	d;					      \
									      \
		    d = *arglist.v.list[1].v.fnum;			      \
		    errno = 0;						      \
		    d = name(d);					      \
		    free_var(arglist);					      \
		    if (errno == EDOM)					      \
		        return make_error_pack(E_INVARG);		      \
		    else if (errno != 0  ||  !IS_REAL(d))		      \
			return make_error_pack(E_FLOAT);		      \
		    else						      \
			return make_var_pack(new_float(d));		      \
		}

MATH_FUNC(sqrt)
MATH_FUNC(sin)
MATH_FUNC(cos)
MATH_FUNC(tan)
MATH_FUNC(asin)
MATH_FUNC(acos)
MATH_FUNC(sinh)
MATH_FUNC(cosh)
MATH_FUNC(tanh)
MATH_FUNC(exp)
MATH_FUNC(log)
MATH_FUNC(log10)
MATH_FUNC(ceil)
MATH_FUNC(floor)
    static package
     bf_trunc(Var arglist, Byte next, void *vdata, Objid progr)
{
    double d;

    d = *arglist.v.list[1].v.fnum;
    errno = 0;
    if (d < 0.0)
	d = ceil(d);
    else
	d = floor(d);
    free_var(arglist);
    if (errno == EDOM)
	return make_error_pack(E_INVARG);
    else if (errno != 0 || !IS_REAL(d))
	return make_error_pack(E_FLOAT);
    else
	return make_var_pack(new_float(d));
}

static package
bf_atan(Var arglist, Byte next, void *vdata, Objid progr)
{
    double d, dd;

    d = *arglist.v.list[1].v.fnum;
    errno = 0;
    if (arglist.v.list[0].v.num >= 2) {
	dd = *arglist.v.list[2].v.fnum;
	d = atan2(d, dd);
    } else
	d = atan(d);
    free_var(arglist);
    if (errno == EDOM)
	return make_error_pack(E_INVARG);
    else if (errno != 0 || !IS_REAL(d))
	return make_error_pack(E_FLOAT);
    else
	return make_var_pack(new_float(d));
}

static package
bf_time(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    r.type = TYPE_INT;
    r.v.num = time(0);
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_ctime(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    time_t c;
    char buffer[50];

    if (arglist.v.list[0].v.num == 1) {
	c = arglist.v.list[1].v.num;
    } else {
	c = time(0);
    }

    {				/* Format the time, including a timezone name */
#if HAVE_STRFTIME
	strftime(buffer, 50, "%a %b %d %H:%M:%S %Y %Z", localtime(&c));
#else
#  if HAVE_TM_ZONE
	struct tm *t = localtime(&c);
	char *tzname = t->tm_zone;
#  else
#    if !HAVE_TZNAME
	const char *tzname = "XXX";
#    endif
#  endif

	strcpy(buffer, ctime(&c));
	buffer[24] = ' ';
	strncpy(buffer + 25, tzname, 3);
	buffer[28] = '\0';
#endif
    }

    if (buffer[8] == '0')
	buffer[8] = ' ';
    r.type = TYPE_STR;
    r.v.str = str_dup(buffer);

    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_random(Var arglist, Byte next, void *vdata, Objid progr)
{
    int nargs = arglist.v.list[0].v.num;
    int num = (nargs >= 1 ? arglist.v.list[1].v.num : 1);

    free_var(arglist);

    if (num <= 0)
	return make_error_pack(E_INVARG);
    else {
	Var r;

	r.type = TYPE_INT;
	if (nargs == 0)
	    r.v.num = RANDOM();
	else
	    r.v.num = RANDOM() % num + 1;
	return make_var_pack(r);
    }
}

static package
bf_floatstr(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (float, precision [, sci-notation]) */
    double d = *arglist.v.list[1].v.fnum;
    int prec = arglist.v.list[2].v.num;
    int use_sci = (arglist.v.list[0].v.num >= 3
		   && is_true(arglist.v.list[3]));
    char fmt[10], output[500];	/* enough for IEEE double */
    Var r;

    free_var(arglist);
    if (prec > DBL_DIG + 4)
	prec = DBL_DIG + 4;
    else if (prec < 0)
	return make_error_pack(E_INVARG);
    sprintf(fmt, "%%.%d%c", prec, use_sci ? 'e' : 'f');
    sprintf(output, fmt, d);

    r.type = TYPE_STR;
    r.v.str = str_dup(output);

    return make_var_pack(r);
}

Var zero;			/* useful constant */

void
register_numbers(void)
{
    zero.type = TYPE_INT;
    zero.v.num = 0;
    register_function("toint", 1, 1, bf_toint, TYPE_ANY);
    register_function("tonum", 1, 1, bf_toint, TYPE_ANY);
    register_function("tofloat", 1, 1, bf_tofloat, TYPE_ANY);
    register_function("min", 1, -1, bf_min, TYPE_NUMERIC);
    register_function("max", 1, -1, bf_max, TYPE_NUMERIC);
    register_function("abs", 1, 1, bf_abs, TYPE_NUMERIC);
    register_function("random", 0, 1, bf_random, TYPE_INT);
    register_function("time", 0, 0, bf_time);
    register_function("ctime", 0, 1, bf_ctime, TYPE_INT);
    register_function("floatstr", 2, 3, bf_floatstr,
		      TYPE_FLOAT, TYPE_INT, TYPE_ANY);

    register_function("sqrt", 1, 1, bf_sqrt, TYPE_FLOAT);
    register_function("sin", 1, 1, bf_sin, TYPE_FLOAT);
    register_function("cos", 1, 1, bf_cos, TYPE_FLOAT);
    register_function("tan", 1, 1, bf_tan, TYPE_FLOAT);
    register_function("asin", 1, 1, bf_asin, TYPE_FLOAT);
    register_function("acos", 1, 1, bf_acos, TYPE_FLOAT);
    register_function("atan", 1, 2, bf_atan, TYPE_FLOAT, TYPE_FLOAT);
    register_function("sinh", 1, 1, bf_sinh, TYPE_FLOAT);
    register_function("cosh", 1, 1, bf_cosh, TYPE_FLOAT);
    register_function("tanh", 1, 1, bf_tanh, TYPE_FLOAT);
    register_function("exp", 1, 1, bf_exp, TYPE_FLOAT);
    register_function("log", 1, 1, bf_log, TYPE_FLOAT);
    register_function("log10", 1, 1, bf_log10, TYPE_FLOAT);
    register_function("ceil", 1, 1, bf_ceil, TYPE_FLOAT);
    register_function("floor", 1, 1, bf_floor, TYPE_FLOAT);
    register_function("trunc", 1, 1, bf_trunc, TYPE_FLOAT);
}

char rcsid_numbers[] = "$Id$";

/*
 * $Log$
 * Revision 1.5  2010/04/22 21:37:16  wrog
 * Fix random(m) to be uniformly distributed for m!=2^k
 * Allow for num > RAND_MAX; plus beginnings of 64-bit support
 *
 * Revision 1.4  1998/12/14 13:18:37  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.3  1997/03/08 06:25:42  nop
 * 1.8.0p6 merge by hand.
 *
 * Revision 1.2  1997/03/03 04:19:11  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:00  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.6  1997/03/04 04:34:06  eostrom
 * parse_number() now trusts strtol() and strtod() more instead of
 * parsing for "-" itself, since a bug in that led to inputs like "--5"
 * and "-+5" being treated as valid.
 *
 * Revision 2.5  1996/03/19  07:15:27  pavel
 * Fixed floatstr() to allow DBL_DIG + 4 digits.  Release 1.8.0p2.
 *
 * Revision 2.4  1996/03/10  01:06:49  pavel
 * Increased the maximum precision acceptable to floatstr() by two digits.
 * Release 1.8.0.
 *
 * Revision 2.3  1996/02/18  23:16:22  pavel
 * Made toint() accept floating-point strings.  Made floatstr() reject a
 * negative precision argument.  Release 1.8.0beta3.
 *
 * Revision 2.2  1996/02/11  00:43:00  pavel
 * Added optional implementation of matherr(), to improve floating-point
 * exception handling on SVID3 systems.  Added `trunc()' built-in function.
 * Release 1.8.0beta2.
 *
 * Revision 2.1  1996/02/08  06:58:01  pavel
 * Added support for floating-point numbers and arithmetic and for the
 * standard math functions.  Renamed TYPE_NUM to TYPE_INT, become_number()
 * to become_integer().  Updated copyright notice for 1996.  Release
 * 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:28:59  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.9  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.8  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.7  1992/10/17  20:47:26  pavel
 * Global rename of strdup->str_dup, strref->str_ref, vardup->var_dup, and
 * varref->var_ref.
 *
 * Revision 1.6  1992/09/26  18:02:49  pavel
 * Fixed bug whereby passing negative numbers to random() failed to evoke
 * E_INVARG.
 *
 * Revision 1.5  1992/09/14  17:31:52  pjames
 * Updated #includes.
 *
 * Revision 1.4  1992/09/08  22:01:42  pjames
 * Renamed bf_num.c to numbers.c.  Added `become_number()' from bf_type.c
 *
 * Revision 1.3  1992/08/10  17:36:26  pjames
 * Updated #includes.  Used new regisration method.  Add bf_sqrt();
 *
 * Revision 1.2  1992/07/20  23:51:47  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS ident. string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
