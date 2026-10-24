/*
 * utf.c
 *
 * Routines related to UTF-8 handing
 */

#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include "utf.h"

#include "exceptions.h"

#if UNICODE_STRINGS

/* assuming that GET_CONT assigns next byte into c
 * and will jump somewhere upon malformedness, assign
 * v = next utf8 char.
 */
#define GET_UTF8_CHAR(v,c,GET_CONT)			\
do {							\
    if (c < 0x80)					\
	v = c;						\
    else if (c < 0xc0)					\
	v = INVALID_RUNE;				\
    else if (c < 0xe0) {				\
	v =  c & 0x1f;					\
	GET_CONT;					\
	if (v < 0x80)					\
	    v = INVALID_RUNE;				\
    }							\
    else if (c < 0xf0) {				\
	v =  c & 0x0f;					\
	GET_CONT;					\
	GET_CONT;					\
	if (v < 0x800 || (0xd800 <= v && v < 0xe000))	\
	    v = INVALID_RUNE;				\
    }							\
    else if (c < 0xf8) {				\
	v =  c & 0x07;					\
	GET_CONT;					\
	GET_CONT;					\
	GET_CONT;					\
	if (!(0x10000 <= v && v < 0x110000))		\
	    v = INVALID_RUNE;				\
    }							\
    else						\
	v = INVALID_RUNE;				\
} while (0)						\


/*
 * get_utf():
 *
 * Gets a single UTF-8 character from a string.
 */
#define GET_CONT	      \
    do {		      \
	c = *p;		      \
	c -= 0x80;	      \
	if ( c > 0x3f )	      \
	    goto bad_cont;    \
	p++;		      \
	v = (v << 6) + c;     \
    } while (0)

uint32_t
get_utf(const char **pp)
{
    const char *p = *pp;
    uint8_t  c;
    uint32_t v;

    c = *p++;
    GET_UTF8_CHAR(v,c,GET_CONT);

  done:
    *pp = p;
    return v;

  bad_cont:
    v = INVALID_RUNE;
    goto done;
}

#undef GET_CONT

/*
 * get_utf_call():
 *
 * Get a single UTF-8 character from a generic client.
 * The "state" buffer is used to hold a single byte of input read overrun
 * which can happen if we read a too short sequence; for example,
 * the sequence:
 *
 *  E3 80 41
 *
 * ... can be interpreted as an invalid sequence followed by the letter
 * "A", however, the invalid sequence will not be detected until the 41
 * byte is read, so push it into the state.  The alternative would be to
 * require each client to have an ungetc method.
 *
 */
#define GET_CONT			\
    do {				\
	c = c_getch(c_data);		\
	cc = c;				\
	if (!(0x80 <= c && c < 0xc0))	\
	    goto bad_cont;		\
	c -= 0x80;			\
	v = (v << 6) + c;		\
    } while (0)				\

int32_t
get_utf_call(int32_t (*c_getch) (void *), void *c_data, int32_t *state)
{
    int32_t c;
    int32_t cc = *state;
    int32_t v;

    if (cc != -1) {
	c = cc;
	cc = -1;
    }
    else {
	c = c_getch(c_data);
	if (c == EOF)
	  return EOF;
    }
    GET_UTF8_CHAR(v,c,GET_CONT);
    *state = -1;

  done:
    return v;

  bad_cont:
    *state = cc;
    v = INVALID_RUNE;
    goto done;
}

int
put_utf(char **pp, uint32_t v)
{
    char *p = *pp;

    if (v <= 0x7f) {
	*p++ = v;
    } else if (v <= 0x7ff) {
	*p++ = 0xc0 | (v >> 6);
	*p++ = 0x80 | (v & 0x3f);
    } else if (v <= 0xffff) {
	if ((v - 0xd800) <= (0xdfff-0xd800))
	    return -1;		/* Invalid UCS (surrogate) */
	*p++ = 0xe0 | (v >> 12);
	*p++ = 0x80 | ((v >> 6) & 0x3f);
	*p++ = 0x80 | (v & 0x3f);
    } else if (v <= 0x10ffff) {
	*p++ = 0xf0 | (v >> 18);
	*p++ = 0x80 | ((v >> 12) & 0x3f);
	*p++ = 0x80 | ((v >> 6) & 0x3f);
	*p++ = 0x80 | (v & 0x3f);
    } else {
	return -1;		/* Invalid UCS (out of range) */
    }

    *pp = p;

    return 0;
}

Num
utf_byte_index(const char *s0, Num ci)
{
 /* Num ci0 = ci;       */
    const char *s = s0;
    if (ci <= 1)
	return ci;

    do {/*  s - s0 + 1 == (1-based) index of the
	 *  first byte of character (ci0 - ci + 1)
	 */
	--ci;
    } while (get_utf(&s) && ci > 1);

    return ci > NUM_MAX - (s - s0) ? NUM_MIN : s - s0 + ci;
}

void
utf_byte_range(const char *s0, Num cis[2])
{
    const char *s = s0;

    /* Visit cis in non-decreasing order */
    int o = -(cis[0] > cis[1]);   /*      0 or -1      */
    int step = o | 1;		  /*      1 or -1      */
    Num *cidone = cis + 1 + step; /*  cis+2 or  cis    */
    cis += 1 - step;		  /*  cis   or  cis+2  */

#define _NEXT_CI   if (cidone == (cis += step)) return

    while (cis[o] <= 0)
	_NEXT_CI;

    Num cn = 1;
    do {/* s == start of char #cn
	 * cis[o] == smallest remaining unresolved ci
	 * cn <= all unresolved cis
	 */
	while (cis[o] == cn) {
	    cis[o] = (s - s0) + 1;
	    _NEXT_CI;
	}
	cn++;
    } while (get_utf(&s));

    Num delta_c = (s - s0) + 1 - cn;
    while (cis[o] <= NUM_MAX - delta_c) {
	cis[o] += delta_c;
	_NEXT_CI;
    }
    for (;;) {
	cis[o] = NUM_MIN;
	_NEXT_CI;
    }

#undef _NEXT_CI
}


/* requires bi <= strlen(s0)+2 (caller must check/know)
   and bi != NUM_MIN unless the string really is that long
   (can happen for INT_TYPE_SIZE=16)
   bi, as a 1-based byte index, is assumed to be a character start
   s0[bi] (meaning s0[bi-1] in C terms) is not checked if bi>1.
   bi<=1 is treated as if s0 is preceded by arbitrarily many
   1-byte chars so that those byte and char "indices" are the same. */
Num
utf_char_index(const char *s0, Num bi)
{
    if (bi <= 1 && bi != NUM_MIN)
	return bi;
    if (is_utf8_cont_byte(s0[0]))
	/* protect against backwards overruns. */
	panic("UTF_CHAR_INDEX given malformed utf8");

    Num ci = 1;
    const char *s = s0 + (UNum)bi - 1;
    while (s > s0) {
	/* s is at the start of a character;
	   ci + number of chars before s
	   == char index of bi) */
	while (is_utf8_cont_byte(*--s));
	++ci;
    }
    return ci;
}

size_t
memo_strlen_utf(const char *s)
{
    size_t i = 0;
    while (get_utf(&s)) {
        i++;
    }
    return i;
}

size_t
clearance_utf(const uint8_t c)
{
    if (c <= 0x7f)
        return 1;
    if (c <= 0xbf)
        return 1;
    if (c <= 0xdf)
        return 2;
    if (c <= 0xef)
        return 3;
    if (c <= 0xf7)
        return 4;
    return 1;
}

#endif /* UNICODE_STRINGS */
