/*
 * utf.h
 *
 * Prototypes for UTF-8 handling
 */

#ifndef UTF_h
#define UTF_h 1

#include "config.h"
#include "options.h"
#include "my-string.h"

#include "storage.h"
#include "structures.h"

#define INVALID_RUNE	0xfffd

inline int is_utf8_cont_byte(uint8_t c)
{ return (c & 0xc0) == 0x80; }

#if !UNICODE_STRINGS

/* ASCII World:
 * Inline stubs assume all characters are single-byte.
 */

/* Return the character starting at position *ppc,
 * advancing *ppc to the start of the next character.
 */
inline uint32_t get_utf(const char **ppc)
{ return *(*ppc)++; }

/* Write c to **pp and advance *pp as necessary
 * unless c is outside of the legal range for (Unicode|ASCII)
 * in which case do not and return nonzero
 * (Note that we *still* allow non-MOO-character-set here).
 */
inline int
put_utf(char **ppc, uint32_t c)
{
    if (c <= 0x7f) {
	*(*ppc)++ = c;
	return 0;
    }
    else
	return -1;
}

/* Given a string s and a (1-based) character index ci,
 * Return the (1-based) byte index corresponding to
 * where that character starts.
 * ci may be ANY integer, i.e., this is safe to call *before* any
 * actual range checking.  For this purpose, imagine s being a
 * substring of something that, outside of s, is all single-byte
 * characters, meaning:
 *
 *    for all ci <= 1:
 *       byte index is ci
 *    for all ci > (# of chars in s)
 *       byte index is ci - (# of chars in s) + strlen(s)
 */
inline Num
utf_byte_index(const char *s UNUSED_, Num ci) {
    return ci;
}

/* Translate a pair of 1-based character indices to 1-based byte indices;
 *     utf_byte_range(s, cis);
 * is equivalent to
 *     cis[0] = utf_byte_index(s, cis[0]);
 *     cis[1] = utf_byte_index(s, cis[1]);
 * except that, in certain cases, we can do this **WAY** more efficiently.
 *
 * Note that, starting with a character range [ci0 .. ci1] (inclusive)
 * in a string s, the proper way to get the corresponding byte range
 * is actually:
 *     Num bi[2] = { ci0, ci1 + 1 };
 *     utf_byte_range(s, bi);
 *
 * and bi will similarly be 'start' and 'after' byte positions,
 * not 'start' and 'end'.
 */
inline void
utf_byte_range(const char *s UNUSED_, Num cis[2] UNUSED_)
{ }

/*  Given a 1-based byte index into s of a character start, return the
 *  corresponding 1-based character index.  Unlike with byte_index()
 *  and byte_range() the argument needs to be range-checked in advance.
 *     1 <= bi <= strlen(s)+2
 *  is required.
 */
inline Num
utf_char_index(const char *s UNUSED_, Num bi) {
    return bi;
}

/*  Returns the number of characters in s, but uses the cached length
 *  if we happen to be in ASCII World where there are only byte-strings.
 *  The 'memo_' part is mainly to remind that this is analogous to
 *  memo_strlen() and thus can only be used on the beginnings of
 *  allocated/interned strings (i.e., do NOT try to use this in the
 *  *middle* of a string).  (... and who knows?  Maybe some day
 *  it *will* turn out to be worth caching character lengths...)
 */
inline size_t
memo_strlen_utf(const char *s) {
    return memo_strlen(s);
}

/* Given the first byte of a character in UTF-8 representation,
 * how many bytes long is the full character?  (will be 1-4 in general)
 */
inline size_t
clearance_utf(const uint8_t c UNUSED_) {
    return 1;
}

#else  /* UNICODE_STRINGS */

/* Unicode World:
 * Use the real versions of these functions.
 */

extern uint32_t get_utf(const char **);
extern int put_utf(char **, uint32_t);      /* -> true if failed */
extern Num  utf_byte_index(const char *, Num);
extern void utf_byte_range(const char *, Num [2]);
extern Num  utf_char_index(const char *, Num);

extern size_t memo_strlen_utf(const char *);
extern size_t clearance_utf(const uint8_t);

/* Use
 *    get_byte, state1
 *      where get_byte(state1) returns either a next byte or EOF,
 *      updating state1 accordingly, and
 *    state2
 *      which is additional state needed, to be initialized to -1
 *      before the first call to get_utf_call
 * as a stream from which this function will retrieve (and
 * return) successive characters, and then EOF after the end is
 * reached.
 */
extern int32_t get_utf_call(int32_t (*get_byte)(void *), void *state1, int32_t *state2);

#endif  /* UNICODE_STRINGS */

#endif		/* !UTF_h */
