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

    if (c <= 0x7f) {
	v = c;
    } else if (c <= 0xbf) {
	v = INVALID_RUNE;
    } else if (c <= 0xdf) {
	v = c & 0x1f;
	GET_CONT;
	if (v <= 0x7f)
	    v = INVALID_RUNE;
    } else if (c <= 0xef) {
	v = c & 0x0f;
	GET_CONT;
	GET_CONT;
	if (v <= 0x7ff || (v >= 0xd800 && v <= 0xdfff))
	    v = INVALID_RUNE;
    } else if (c <= 0xf7) {
	v = c & 0x07;
	GET_CONT;
	GET_CONT;
	GET_CONT;
	if (v <= 0xffff || v > 0x10ffff)
	    v = INVALID_RUNE;
    } else {
	v = INVALID_RUNE;
    }

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
#define GET_CONT	      \
    do {		      \
	c = c_getch(c_data);  \
	cc = c;		      \
	c -= 0x80;	      \
	if ( c > 0x3f )	      \
	    goto bad_cont;    \
	v = (v << 6) + c;     \
    } while (0)

int32_t
get_utf_call(int32_t (*c_getch) (void *), void *c_data, int32_t *state)
{
    int32_t c;
    int32_t cc = *state;
    int32_t v;

    if (cc != -1) {
	c = cc;
	cc = -1;
    } else {
	c = c_getch(c_data);
	if (c == EOF)
	  return EOF;
    }

    if (c <= 0x7f) {
	v = c;
    } else if (c <= 0xbf) {
	v = INVALID_RUNE;
    } else if (c <= 0xdf) {
	v = c & 0x1f;
	GET_CONT;
	if (v <= 0x7f)
	    v = INVALID_RUNE;
    } else if (c <= 0xef) {
	v = c & 0x0f;
	GET_CONT;
	GET_CONT;
	if (v <= 0x7ff || (v >= 0xd800 && v <= 0xdfff))
	    v = INVALID_RUNE;
    } else if (c <= 0xf7) {
	v = c & 0x07;
	GET_CONT;
	GET_CONT;
	GET_CONT;
	if (v <= 0xffff || v > 0x10ffff)
	    v = INVALID_RUNE;
    } else {
	v = INVALID_RUNE;
    }

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

    return s - s0 + ci;
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

    while (cis[o] <= 0) {
	cis += step;
	if (cis == cidone) return;
    }

    Num cn = 1;
    do {/* s == start of char #cn
	 * cn <= all remaining cis (cis[o] is smallest)
	 */
	while (cis[o] == cn) {
	    cis[o] = (s - s0) + 1;
	    cis += step;
	    if (cis == cidone) return;
	}
	cn++;
    } while (get_utf(&s));

    do {
	cis[o] += (s - s0) - cn + 1;
	cis += step;
    } while (cis != cidone);
}

/* requires 1 <= bi <= strlen(s0)+2;
   s0[bi] (bi as a 1-based byte index,
   meaning s0[bi-1] in C terms,
   is assumed to be a character start but never dereferenced.  */
Num
utf_char_index(const char *s0, Num bi)
{
    if (is_utf8_cont_byte(s0[0])) {
	/* protect against backwards overruns. */
	panic("UTF_CHAR_INDEX given malformed utf8");
    }
    Num ci = 1;
    const char *s = s0 + bi - 1;
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
skip_utf(const char *s0, int i)
{
    const char *s = s0;

    while (*s && i) {
        get_utf(&s);
        i--;
    }
    return s - s0;
}

size_t
strlen_utf(const char *s)
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
