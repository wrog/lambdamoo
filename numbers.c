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
#include "my-ctype.h"
#include "my-math.h"
#include "my-stdlib.h"
#include "my-string.h"
#include "my-time.h"

#include "config.h"

#include "numbers.h"

#include "functions.h"
#include "log.h"
#include "random.h"
#include "storage.h"
#include "structures.h"
#include "utf.h"
#include "utf-ctype.h"
#include "utils.h"

#include "bf_register.h"

inline int
inrange_for_float_to_int(FlNum d)
{
    return (((((FlNum)NUM_MIN)-1.0) < d || (((FlNum)NUM_MIN) <= d))
	    && d < (-(FlNum)NUM_MIN));
}


Var
parse_number(unsigned flags, int32_t c_first,
	     int32_t (*getch)(void),
	     void (*ungetch)(int32_t))
{
    int32_t c = c_first;
    Stream *ns = new_stream(30);

    Var ret = zero;
    /* ret.type = TYPE_INT minus compiler
       warnings about uninitialized .num */

#if UNICODE_NUMBERS
    uint32_t dfamily = 0;
#endif

    /* state flags */

/* sticky flags; they stay on, once set:           */
#   define F_INT    0x01     /* integer-only mode  */
#   define F_MINUS  0x02     /* '-' seen at all    */
#   define F_DOT    0x04     /* '.' seen at all    */
#   define F_EXP    0x08     /* we are in exponent */	\

/* these get reset when 'E'|'e' shows up:          */
#   define F_DIGIT  0x10     /* number is viable   */
#   define F_XSIGN  0x20     /* forbid '-' and '+' */

    if (flags & PN_NONNEG) {
	stream_add_char(ns, '-');
	/* Yes, we are being tricky.
	   See Very Long Comment below. */
    }

    unsigned state = F_INT*(!(flags & PN_FLOAT_OK));

    if (EOF == c) goto donechars;

#   define NEXTC(what_to_do_at_eof)		\
        do {					\
	   if (EOF == (c = getch())) {		\
	       what_to_do_at_eof;		\
	   }					\
	} while (0)				\

    if (flags & PN_LDSPACE)
	while (my_isspace(c))
	    NEXTC(goto donechars);

    if ((flags & PN_OCTOTHORPE) && (c == '#')) {
	NEXTC(goto donechars);
	if (flags & PN_OCTOSPACE)
	    while (my_isspace(c))
		NEXTC(goto donechars);
    }

    for (;;) {
	switch (c) {
	case '+':
	    if ((state & (F_EXP|F_XSIGN)) != F_EXP)
		goto badchar;
	    state |= F_XSIGN;
	    break;
	case '-':
	    if (state & (F_XSIGN))
		goto badchar;
	    state |= F_MINUS|F_XSIGN;
	    break;
	case '.':
	    if (state & (F_DOT|F_EXP|F_INT))
		goto badchar;
	    if (ungetch) {
		/* we can peek to make sure it is not '..' */
		int32_t cc = getch();
		ungetch(cc);
		if (cc == '.') {
		    /* treat '..' as if it were some other character */
		    goto badchar;
		}
	    }
	    ret.type = TYPE_FLOAT;
	    state |= F_DOT|F_XSIGN;
	    break;
	case 'e':
	case 'E':
	    if ((state & (F_DIGIT|F_EXP|F_INT)) != F_DIGIT)
		goto badchar;
	    ret.type = TYPE_FLOAT;
	    state |= F_EXP;
	    state &= ~(F_DIGIT|F_XSIGN);
	    break;
	default:
#if UNICODE_NUMBERS
	    if (!my_isdigit(c)) {
#endif
		if (flags & PN_TRSPACE) {
		    while (my_isspace(c))
			NEXTC(goto donechars);
		}
		goto badchar;
#if UNICODE_NUMBERS
	    }
	    int dval = my_digitval(c);
	    if (!dfamily) {
		dfamily = c - dval;
	    }
	    else if (dfamily + dval != (uint32_t)c) {
		goto badchar;
	    }
	    c = '0' + dval;
#else /* !UNICODE_NUMBERS */
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
#endif /* !UNICODE_NUMBERS */
	    state |= F_DIGIT|F_XSIGN;
	    break;
	}
	stream_add_char(ns, c);
	NEXTC(goto donechars);
    }
#   undef NEXTC

 badchar:
    if (ungetch) ungetch(c);
    if (flags & PN_MUST_EOF)
	goto parse_error;

 donechars:
    if (state & F_DIGIT)
	goto have_digits;
    if ((state & F_EXP) || !ungetch)
	goto parse_error;

    if (state & F_DOT)
	(*ungetch)('.');
    if (state & F_MINUS)
	(*ungetch)('-');

    /* "Dr. Corby ... was never here." */
    ret.v.err = E_NONE;
    goto return_error;

#   undef F_INT
#   undef F_MINUS
#   undef F_DOT
#   undef F_EXP
#   undef F_DIGIT
#   undef F_XSIGN

 have_digits: ;
    char *p;
    const char *s = stream_contents(ns);
    const char *p0 = s + stream_length(ns);
    errno = 0;

    if (ret.type == TYPE_INT && !(flags & PN_REQ_FLOAT)) {
	intmax_t i = strtoimax(stream_contents(ns), &p, 10);
	/* Recall that this is negated in the PN_NONNEG case.
         * See Very Long Comment below.
	 */
	if (errno == ERANGE
#if NUM_MAX < INTMAX_MAX
	    || i < NUM_MIN || i > NUM_MAX
#endif
	   )
	    goto range_error;
	if (p != p0)
	    goto parse_error;
	ret.v.num = ((flags & PN_NONNEG) ? -i : i);
    }
    else {
	FlNum d = strtoflnum(stream_contents(ns)
			     /* Extra negation is not needed here. */
			     + ((flags & PN_NONNEG) ? 1 : 0), &p);
	if (p != p0)
	    goto parse_error;
	if (!IS_REAL(d))
	    goto range_error;
	if (flags & PN_REQ_INT) {
	    if (!inrange_for_float_to_int(d))
		goto range_error;
	    ret.type = TYPE_INT;
	    ret.v.num = (Num)d;
	}
	else {
	    ret.type = TYPE_FLOAT;
	    ret.v.fnum = box_fl(d);
	}
    }
 return_value:
    free_stream(ns);
    return ret;

 range_error:
    ret.v.err = E_RANGE;
    goto return_error;

 parse_error:
    ret.v.err = E_INVARG;
 return_error:
    ret.type = TYPE_ERR;
    goto return_value;

}
/* The extra initial minus for the PN_NONNEG case is for the yacc
 * parser, which splits minus signs away from constants, and hence has
 * to be able to read a bare (NUM_MAX+1) -- which we would normally want
 * to flag as out-of-range -- the problem being that NUM_MIN, which can
 * show up in dumped databases, will essentially have been pre-parsed as
 *   '-' (NUM_MAX+1)
 * and there's nothing we can do about that, or at least, nothing
 * pleasant.
 *
 * However, for the yacc parser, we will be certain the number we are
 * reading is non-negative, so artifically adding a minus before
 * sending it off to strimax() will
 *   (1) not lose information or create anything unparseable, and
 *   (2) make strtoimax read something in the range [NUM_MIN..0]
 *       rather than [0..NUM_MAX+1], which is then guaranteed to
 *       succeed, and, upon re-negation a few lines later, these
 *       numbers will all become [0..(NUM_MAX+1)(**)],
 *
 * and anything at started out as NUM_MIN will be returned from
 * parse_number as NUM_MAX+1 but then will later get fed to a unary
 * negation op constant-folding, and then become the NUM_MIN
 * that it was originally supposed to be...
 *
 * (**)... the one weirdness being that if we're on a platform whose
 * INTMAX_MAX == NUM_MAX, then strtoimax() will produce NUM_MIN
 * directly and the two subsequent negations will actually be
 * no-ops,... but this will be completely invisible.
 *
 * The one downside to all of this is if someone tries to write a
 * *positive* (NUM_MAX+1) into verbcode (i.e., without any leading
 * '-'), this will get silently rewritten into NUM_MIN as well, *but*,
 * on a virtual machine using 2s-complement integers of that width,
 * the two numbers *will* be functionally equivalent, even if NUM_MIN
 * might not be what they're expecting to see when they list out the
 * verb again.  However given that, in the database format, the only
 * integers of that magnitude will all be NUM_MINs, the only way a
 * postive (NUM_MAX+1) can show up is if someone enters all of the
 * digits directly anew into a set_verb_ode()/.program session, at
 * which point we have to presume that if they're playing in this
 * world, they know what they're doing (and thus know that it's going
 * to get immediately negated anyway because that's how 2-s complement
 * integers work.)
 *
 * (... still sucks that we cannot even issue a warning about this
 * but I have yet to come up with a good way to do even that.
 * --wrog)
 */


static const char *snp_string;
static const char *snp_end;
static void
snp_init(const char *str)
{
    snp_string = str;
    snp_end = str + memo_strlen(str);
}
static int32_t
snp_getch(void)
{
    if (snp_string >= snp_end)
	return EOF;
    return get_utf(&snp_string);
}

static Var
parse_number_from_string(const char *str, unsigned flags)
{
    snp_init(str);
    return parse_number(flags|PN_MUST_EOF, snp_getch(), snp_getch, NULL );
}


enum error
become_integer(Var in, Num *ret, int called_from_tonum)
{
    switch (in.type) {
    case TYPE_INT:
	*ret = in.v.num;
	break;
    case TYPE_STR: {
	Var r = parse_number_from_string(
              in.v.str,
	      PN_SPACE|PN_REQ_INT|
	      (called_from_tonum
	       ? PN_FLOAT_OK : PN_OCTOTHORPE));
	if (r.type == TYPE_ERR) {
	    if (r.v.err == E_NONE)
		/* cannot happen but current gcc is
		   not smart enough to know that. */
		*ret = 12345;
	    return r.v.err;
	}
	*ret = r.v.num;
	free_var(r);  /* gratuitous */
	break;
    }
    case TYPE_OBJ:
	*ret = in.v.obj;
	break;
    case TYPE_ERR:
	*ret = in.v.err;
	break;
    case TYPE_FLOAT:
        if (!IS_REAL(fl_unbox(in.v.fnum)))
	    return E_FLOAT;
	FlNum d = fl_unbox(in.v.fnum);
	if (!inrange_for_float_to_int(d))
	    return E_RANGE;
	*ret = (Num)d;
	break;
    case TYPE_LIST:
	return E_TYPE;
    default:
	panic("BECOME_INTEGER: Impossible Var .type");
    }
    return E_NONE;
}


static enum error
become_float(Var in, FlNum *ret)
{
    switch (in.type) {
    case TYPE_INT:
	*ret = (FlNum)in.v.num;
	break;
    case TYPE_STR: {
	Var r = parse_number_from_string(
	     in.v.str,
	     PN_SPACE|PN_FLOAT_OK|PN_REQ_FLOAT);
	if (r.type == TYPE_ERR) {
	    if (r.v.err == E_NONE)
		/* cannot happen but current gcc is
		   not smart enough to know that. */
		*ret = 12345.0;
	    return r.v.err;
	}
	*ret = fl_unbox(r.v.fnum);
	free_var(r);  /* not gratuitous if FLOATS_ARE_BOXED */
	break;
    }
    case TYPE_OBJ:
	*ret = (FlNum)in.v.obj;
	break;
    case TYPE_ERR:
	*ret = (FlNum)in.v.err;
	break;
    case TYPE_FLOAT:
	*ret = fl_unbox(in.v.fnum);
	break;
    case TYPE_LIST:
	return E_TYPE;
    default:
	panic("BECOME_FLOAT: Impossible Var .type");
    }
    return E_NONE;
}

#if FLOATS_ARE_BOXED

FlBox
box_fl(FlNum f) {
    FlBox p = mymalloc(sizeof(FlNum), M_FLOAT);
    *p = f;
    return p;
}
/* unboxed version is in structures.h */

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
	/* FALLS THROUGH */
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
	return fl_unbox(lhs.v.fnum) == fl_unbox(rhs.v.fnum);
}

int
compare_integers(Num a, Num b)
{
    if (a < b)
	return -1;
    else if (a > b)
	return 1;
    else
	return 0;
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
	if (a.v.num < b.v.num)
	    ans.v.num = -1;
	else if (a.v.num > b.v.num)
	    ans.v.num = 1;
	else
	    ans.v.num = 0;
    } else {
	ans.type = TYPE_INT;
	if (fl_unbox(a.v.fnum) < fl_unbox(b.v.fnum))
	    ans.v.num = -1;
	else if (fl_unbox(a.v.fnum) > fl_unbox(b.v.fnum))
	    ans.v.num = 1;
	else
	    ans.v.num = 0;
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
			FlNum d =				\
			    fl_unbox(a.v.fnum)			\
			      op fl_unbox(b.v.fnum);		\
								\
			if (!IS_REAL(d)) {			\
			    ans.type = TYPE_ERR;		\
			    ans.v.err = E_FLOAT;		\
			} else {				\
			    ans.type = TYPE_FLOAT;		\
			    ans.v.fnum = box_fl(d);		\
			}					\
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
			       && fl_unbox(b.v.fnum) != 0.0) {	\
			FlNum d = fexpr;			\
								\
			if (!IS_REAL(d)) {			\
			    ans.type = TYPE_ERR;		\
			    ans.v.err = E_FLOAT;		\
			} else {				\
			    ans.type = TYPE_FLOAT;		\
			    ans.v.fnum = box_fl(d);		\
		        }					\
		    } else {					\
		        ans.type = TYPE_ERR;			\
			ans.v.err = E_DIV;			\
		    }						\
								\
		    return ans;					\
		}

DIVISION_OP(divide, /, fl_unbox(a.v.fnum) / fl_unbox(b.v.fnum))
DIVISION_OP(modulus, %, fmod(fl_unbox(a.v.fnum), fl_unbox(b.v.fnum)))
Var
do_power(Var lhs, Var rhs)
{				/* LHS ^ RHS */
    Var ans;

    if (lhs.type == TYPE_INT) {	/* integer exponentiation */
	Num a = lhs.v.num, b, r;

	if (rhs.type != TYPE_INT)
	    goto type_error;

	b = rhs.v.num;
	ans.type = TYPE_INT;
	if (b < 0) {
	    switch (a) {
	    case -1:
		ans.v.num = (b & 1) ? 1 : -1;
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
		break;
	    }
	} else {
	    r = 1;
	    while (b != 0) {
		if (b & 1)
		    r *= a;
		a *= a;
		b >>= 1;
	    }
	    ans.v.num = r;
	}
    } else if (lhs.type == TYPE_FLOAT) {	/* floating-point exponentiation */
	FlNum d;

	switch (rhs.type) {
	case TYPE_INT:
	    d = (FlNum)rhs.v.num;
	    break;
	case TYPE_FLOAT:
	    d = fl_unbox(rhs.v.fnum);
	    break;
	default:
	    goto type_error;
	}
	errno = 0;
	d = FLOAT_FN(pow)(fl_unbox(lhs.v.fnum), d);
	if (errno != 0 || !IS_REAL(d)) {
	    ans.type = TYPE_ERR;
	    ans.v.err = E_FLOAT;
	} else {
	    ans.type = TYPE_FLOAT;
	    ans.v.fnum = box_fl(d);
	}
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
bf_toint(Var arglist, Byte next UNUSED_, void *vdata UNUSED_, Objid progr UNUSED_)
{
    enum error e;

    Num n;
    e = become_integer(arglist.v.list[1], &n, 1);

    free_var(arglist);
    if (e != E_NONE)
	return make_error_pack(e);

    return make_int_pack(n);
}

static package
bf_tofloat(Var arglist, Byte next UNUSED_, void *vdata UNUSED_, Objid progr UNUSED_)
{
    enum error e;

    FlNum d;
    e = become_float(arglist.v.list[1], &d);

    free_var(arglist);
    if (e != E_NONE)
	return make_error_pack(e);

    return make_float_pack(d);
}

static package
bf_min(Var arglist, Byte next UNUSED_, void *vdata UNUSED_, Objid progr UNUSED_)
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
	    else if (arglist.v.list[i].v.fnum < r.v.fnum)
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
bf_max(Var arglist, Byte next UNUSED_, void *vdata UNUSED_, Objid progr UNUSED_)
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
	    else if (arglist.v.list[i].v.fnum > r.v.fnum)
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
bf_abs(Var arglist, Byte next UNUSED_, void *vdata UNUSED_, Objid progr UNUSED_)
{
    Var r = arglist.v.list[1];
    package p;

    if (r.type == TYPE_INT)
	p = make_int_pack(r.v.num < 0 ? - r.v.num : r.v.num);
    else
	p = make_float_pack(FLOAT_FN(fabs)(fl_unbox(r.v.fnum)));

    free_var(arglist);
    return p;
}

#define MATH_FUNC(name)						\
    static package						\
    bf_ ## name(Var arglist, Byte next UNUSED_,			\
		void *vdata UNUSED_, Objid progr UNUSED_)	\
    {								\
	FlNum d;						\
								\
	errno = 0;						\
	d = FLOAT_FN(name)(fl_unbox(arglist.v.list[1].v.fnum));	\
	free_var(arglist);					\
	if (errno == EDOM)					\
	    return make_error_pack(E_INVARG);			\
	else if (errno != 0 || !IS_REAL(d))			\
	    return make_error_pack(E_FLOAT);			\
	else							\
	    return make_float_pack(d);				\
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
     MATH_FUNC(asinh)
     MATH_FUNC(acosh)
     MATH_FUNC(atanh)
     MATH_FUNC(exp)
     MATH_FUNC(log)
     MATH_FUNC(log10)
     MATH_FUNC(ceil)
     MATH_FUNC(floor)
     MATH_FUNC(expm1)
     MATH_FUNC(log1p)
     MATH_FUNC(erf)
     MATH_FUNC(erfc)
     MATH_FUNC(lgamma)

     static package
     bf_trunc(Var arglist, Byte next UNUSED_, void *vdata UNUSED_, Objid progr UNUSED_)
{
    FlNum d;

    d = fl_unbox(arglist.v.list[1].v.fnum);
    errno = 0;
    if (d < 0.0)
	d = FLOAT_FN(ceil)(d);
    else
	d = FLOAT_FN(floor)(d);
    free_var(arglist);
    if (errno == EDOM)
	return make_error_pack(E_INVARG);
    else if (errno != 0 || !IS_REAL(d))
	return make_error_pack(E_FLOAT);
    else
	return make_float_pack(d);
}

static package
bf_atan(Var arglist, Byte next UNUSED_, void *vdata UNUSED_, Objid progr UNUSED_)
{
    FlNum d, dd;

    d = fl_unbox(arglist.v.list[1].v.fnum);
    errno = 0;
    if (arglist.v.list[0].v.num >= 2) {
	dd = fl_unbox(arglist.v.list[2].v.fnum);
	d = atan2(d, dd);
    }
    else
	d = atan(d);
    free_var(arglist);
    if (errno == EDOM)
	return make_error_pack(E_INVARG);
    else if (errno != 0 || !IS_REAL(d))
	return make_error_pack(E_FLOAT);
    else
	return make_float_pack(d);
}

static package
bf_j(Var arglist, Byte next UNUSED_, void *vdata UNUSED_, Objid progr UNUSED_)
{
    FlNum d;

    d = fl_unbox(arglist.v.list[2].v.fnum);
    errno = 0;
    switch ( arglist.v.list[1].v.num ) {
    case 0:
	d = j0(d);
	break;
    case 1:
	d = j1(d);
	break;
    default:
	d = jn(d, arglist.v.list[1].v.num);
	break;
    }
    free_var(arglist);
    if (errno == EDOM)
	return make_error_pack(E_INVARG);
    else if (errno != 0 || !IS_REAL(d))
	return make_error_pack(E_FLOAT);
    else
	return make_float_pack(d);
}

static package
bf_y(Var arglist, Byte next UNUSED_, void *vdata UNUSED_, Objid progr UNUSED_)
{
    FlNum d;

    d = fl_unbox(arglist.v.list[2].v.fnum);
    errno = 0;
    switch ( arglist.v.list[1].v.num ) {
    case 0:
	d = y0(d);
	break;
    case 1:
	d = y1(d);
	break;
    default:
	d = yn(d, arglist.v.list[1].v.num);
	break;
    }
    free_var(arglist);
    if (errno == EDOM)
	return make_error_pack(E_INVARG);
    else if (errno != 0 || !IS_REAL(d))
	return make_error_pack(E_FLOAT);
    else
	return make_float_pack(d);
}

static package
bf_time(Var arglist, Byte next UNUSED_, void *vdata UNUSED_, Objid progr UNUSED_)
{
    free_var(arglist);
    return make_int_pack(time(0));
}

static package
bf_ftime(Var arglist, Byte next UNUSED_, void *vdata UNUSED_, Objid progr UNUSED_)
{
    FlNum t;
    struct timeval tv;

    free_var(arglist);

    gettimeofday(&tv, NULL);

    /* Use division since 1.0e-6 isn't representable in exact binary */
    t = (FlNum)tv.tv_sec + (FlNum)tv.tv_usec/1.0e+6;

    return make_float_pack(t);
}

static package
bf_ctime(Var arglist, Byte next UNUSED_, void *vdata UNUSED_, Objid progr UNUSED_)
{
    Var r;
    time_t c;
    char buffer[128];
    int has_time     = (arglist.v.list[0].v.num >= 1);
    int has_timezone = (arglist.v.list[0].v.num >= 2);
    char *current_timezone = NULL;
    struct tm *t;

    c = has_time ? (time_t)arglist.v.list[1].v.num : time(0);

    if (has_timezone) {
	current_timezone = getenv("TZ");
	if ( current_timezone )
	    current_timezone = str_dup(current_timezone);
	setenv("TZ", arglist.v.list[2].v.str, 1);
	tzset();
    }

    t = localtime(&c);
    if (t == 0)
	*buffer = 0;
    else {			/* Format the time, including a timezone name */
#if HAVE_STRFTIME
	if (strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y %Z", t) == 0)
	    *buffer = 0;
#else
#  if HAVE_TM_ZONE
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

    if (has_timezone) {
	if (current_timezone) {
	    setenv("TZ", current_timezone, 1);
	    free_str(current_timezone);
	} else {
	    unsetenv("TZ");
	}
	tzset();
    }

    free_var(arglist);

    if (*buffer == 0)
	return make_error_pack(E_INVARG);

    if (buffer[8] == '0')
	buffer[8] = ' ';
    r.type = TYPE_STR;
    r.v.str = str_dup(buffer);

    return make_var_pack(r);
}


/* Find an unsigned type that can hold RANDOM() results and Nums */
#if RAND_MAX <= NUM_MAX
#  define URNUM_BITS INT_TYPE_BITSIZE
typedef UNum URNum;
#  if HAVE_UNUMNUM_T
#    define HAVE_URNUM2_T 1
typedef UNumNum URNum2;
#  endif

#elif RAND_MAX == INT32_MAX
#  define URNUM_BITS 32
typedef uint32_t URNum;
#  if HAVE_INT64_T
#    define HAVE_URNUM2_T 1
typedef uint64_t URNum2;
#  endif

#elif RAND_MAX == INT64_MAX
#  define URNUM_BITS 64
typedef uint64_t URNum;
#  if HAVE_INT128_T
#    define HAVE_URNUM2_T 1
typedef uint128_t URNum2;
#  endif

#else
#  error weird RAND_MAX that I cannot cope with
#endif



#ifndef HAVE_URNUM2_T
/* Number of bits to shift V left in order to make
 * the high-order bit be 1 (assume V nonzero) */
static inline char
rlg2 (URNum v)
{
    /* See "Using de Bruijn Sequences to Index 1 in a Computer Word"
     * by Leiserson, Prokop, Randall; MIT LCS, 1998
     */
    static const char evil[] = {
#  if URNUM_BITS == 64
	63, 5,62, 4,24,10,61, 3,32,15,23, 9,45,29,60, 2,
	12,34,31,14,52,50,22, 8,48,37,44,28,41,20,59, 1,
	6, 25,11,33,16,46,30,13,35,53,51,49,38,42,21, 7,
	26,17,47,36,54,39,43,27,18,55,40,19,56,57,58, 0,
#  elif URNUM_BITS == 32
	31,22,30,21,18,10,29, 2,20,17,15,13, 9, 6,28, 1,
	23,19,11, 3,16,14, 7,24,12, 4, 8,25, 5,26,27, 0,
#  elif URNUM_BITS == 16
	15, 3,14, 2, 8, 6,13, 1, 4, 9, 7, 5,10,11,12, 0,
#  endif
    };

    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
#  if URNUM_BITS > 16
    v |= v >> 16;
#    if URNUM_BITS > 32
    v |= v >> 32;
    return evil[(URNum)(v * 0x03F566ED27179461ULL) >> 58];
#    else
    return evil[(URNum)(v * 0x07C4ACDDULL) >> 27];
#    endif
#  else
    return evil[(URNum)(v * 0x0F59ULL) >> 12];
#  endif
}
#endif

/* (a * b + c) % m, guarding against overflow; assumes m > 0 */
static inline URNum
muladdmod(URNum a, URNum b, URNum c, URNum m)
{
#ifdef HAVE_URNUM2_T
    return (URNum)((a * (URNum2)b + c) % m);
#else
#  define HALFWORD (URNUM_BITS/2)
#  define LO(x) ((x) & ((1ULL<<HALFWORD)-1))
#  define HI(x) ((x)>>HALFWORD)

    URNum lo = LO(a) * LO(b) + LO(c);
    URNum hi;
    {
	URNum mi1 = HI(a) * LO(b) + HI(c) + HI(lo);
	URNum mi2 = LO(a) * HI(b) + LO(mi1);
	hi = HI(a) * HI(b) + HI(mi1) + HI(mi2);
	lo = (LO(lo) + (LO(mi2)<<HALFWORD)) % m;
    }
    if (hi != 0) {
	int d_sh = rlg2(hi);
	int sh;
	for (sh = 2*HALFWORD - d_sh;
	     hi <<= d_sh, hi %= m, hi != 0;
	     sh -= d_sh) {

	    d_sh = rlg2(hi);
	    if (d_sh >= sh) {
		hi <<= sh;
		hi %= m;
		break;
	    }
	}
    }
    return (lo + hi) % m;
#  undef HALFWORD
#  undef HI
#  undef LO
#endif
}

static package
bf_random(Var arglist, Byte next UNUSED_, void *vdata UNUSED_, Objid progr UNUSED_)
{
    Num nargs = arglist.v.list[0].v.num;
    Num num = (nargs >= 1 ? arglist.v.list[1].v.num : NUM_MAX);
    Var r;
    URNum e;
    URNum rnd;

    free_var(arglist);

    if (num <= 0)
	return make_error_pack(E_INVARG);

    const Num range_l = /* RANGE % num */
	((NUM_MAX > RAND_MAX ? RAND_MAX : (RAND_MAX - num)) + 1) % num;

    r.type = TYPE_INT;

#if ((RAND_MAX <= 0) || 0!=(RAND_MAX & (RAND_MAX+1)))
#   error RAND_MAX+1 is not a positive power of 2 ??
#endif

#if (NUM_MAX > RAND_MAX)
    /* num >= RANGE possible; launch general algorithm */

#   define RANGE       ((URNum)RAND_MAX+1)
#   define OR_ZERO(n)  (n)

    rnd = 0;
    e = 1;
#else
    /* num >= RANGE not possible (num/RANGE == 0 always);
       unroll first loop iteration */

#   define OR_ZERO(n)  0

    rnd = RANDOM();
    e = range_l;

    if (rnd >= e) {
	r.v.num = 1 + rnd % num;
	return make_var_pack(r);
    }
#endif

    for (;;) {
	/* INVARIANT: rnd is uniform over [0..e-1] */
	URNum rnd_next = RANDOM();

#if RAND_MAX < NUM_MAX
	/* while (e*RANGE < num) ... */
	while (e < (num/RANGE) ||
	       ((e == num/RANGE) && (num%RANGE != 0))) {
	    rnd = rnd*RANGE + rnd_next;
	    e *= RANGE;
	    rnd_next = RANDOM();
	    /* I expect the compiler to turn all of these
	     * [/%*]RANGE operations into bitwise ops.
	     */
	}
#endif
	/* INVARIANTS:
	 *   e*RANGE >= num
	 *   rnd is uniform over [0..e-1]
	 *   rnd*RANGE + rnd_next is uniform over [0..e*RANGE-1]
	 */
	if (rnd > OR_ZERO(num/RANGE)) {
	    /* (rnd-1)*RANGE >= (num/RANGE)*RANGE == num - num%RANGE
	     * rnd*RANGE >= num + (RANGE - num%RANGE) > num > (e*RANGE)%num
	     */
	    r.v.num = 1 + muladdmod(rnd, range_l, rnd_next, num);
	    break;
	}
	rnd = OR_ZERO(rnd*RANGE) + rnd_next;
	e = muladdmod(e, range_l, 0, num);

	if (rnd >= e) {
	    r.v.num = 1 + rnd % num;
	    break;
	}
    }
    return make_var_pack(r);
#undef RANGE
#undef OR_ZERO
}

static package
bf_floatstr(Var arglist, Byte next UNUSED_, void *vdata UNUSED_, Objid progr UNUSED_)
{				/* (float, precision [, sci-notation]) */
    FlNum d = fl_unbox(arglist.v.list[1].v.fnum);
    Num prec = arglist.v.list[2].v.num;
    int use_sci = (arglist.v.list[0].v.num >= 3
		   && is_true(arglist.v.list[3]));
    char fmt[10], output[500];	/* enough for IEEE double */
    Var r;

    free_var(arglist);
    if (prec > FLOAT_DIGITS+4)
	prec = FLOAT_DIGITS+4;
    else if (prec < 0)
	return make_error_pack(E_INVARG);
    sprintf(fmt, "%%.%"PRIdN"%s", prec, use_sci ? PRIeR : PRIfR);
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
    register_function("ftime", 0, 0, bf_ftime);
    register_function("ctime", 0, 2, bf_ctime, TYPE_INT, TYPE_STR);
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
    register_function("asinh", 1, 1, bf_asinh, TYPE_FLOAT);
    register_function("acosh", 1, 1, bf_acosh, TYPE_FLOAT);
    register_function("atanh", 1, 1, bf_atanh, TYPE_FLOAT);
    register_function("exp", 1, 1, bf_exp, TYPE_FLOAT);
    register_function("log", 1, 1, bf_log, TYPE_FLOAT);
    register_function("log10", 1, 1, bf_log10, TYPE_FLOAT);
    register_function("ceil", 1, 1, bf_ceil, TYPE_FLOAT);
    register_function("floor", 1, 1, bf_floor, TYPE_FLOAT);
    register_function("trunc", 1, 1, bf_trunc, TYPE_FLOAT);
    register_function("expm1", 1, 1, bf_expm1, TYPE_FLOAT);
    register_function("log1p", 1, 1, bf_log1p, TYPE_FLOAT);
    register_function("erf", 1, 1, bf_erf, TYPE_FLOAT);
    register_function("erfc", 1, 1, bf_erfc, TYPE_FLOAT);
    register_function("lgamma", 1, 1, bf_lgamma, TYPE_FLOAT);
    register_function("j", 2, 2, bf_j, TYPE_INT, TYPE_FLOAT);
    register_function("y", 2, 2, bf_y, TYPE_INT, TYPE_FLOAT);
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
