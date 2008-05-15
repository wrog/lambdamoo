/*
 * utf-ctype.h
 *
 * query Unicode Character Database for character attributes
 */

#ifndef UTF_CType_H
#define UTF_CType_H 1

#include "config.h"
#include "options.h"

#include "my-ctype.h"

#if !UNICODE_NUMBERS
inline int my_isdigit(uint32_t c)      { return isdigit(c); }
inline int my_digitval(uint32_t c)     { return c - '0';    }
#else
extern int my_isdigit(uint32_t);
extern int my_digitval(uint32_t);
#endif

#if !UNICODE_IDENTIFIERS
inline int my_is_xid_start(uint32_t c) { return isalpha(c); }
inline int my_is_xid_cont(uint32_t c)  { return isalnum(c); }
#else
extern int my_is_xid_start(uint32_t);
extern int my_is_xid_cont(uint32_t);
#endif

#if !UNICODE_STRINGS
inline uint32_t my_tolower(uint32_t c) { return tolower(c); }
inline uint32_t my_toupper(uint32_t c) { return toupper(c); }
inline int      my_isspace(uint32_t c) { return isspace(c); }
#else
extern uint32_t my_tolower(uint32_t);
extern uint32_t my_toupper(uint32_t);
extern int      my_isspace(uint32_t);
#endif

#if UNICODE_STRINGS
extern int my_is_printable(uint32_t);
#else
inline int
my_is_printable(uint32_t x)
{
    /* ::=  isgraph(x) || x == ' ' || x == '\t',
     *  except since we are defining our own thing we may
     *  as well NOT rely on the ctype.h definitions.
     */
    return ((x <= 0x7e)  /* excludes DELETE */
	    &&
	    ((x & 0x60) || (x == '\t')));
}
#endif
/*  Unlike with the other my_* functions here, which are intended to
 *  follow the respective Unicode classifications, "printable" here
 *  does NOT.  It actually refers to the set of code points (character
 *  values) that are part of the MOO language character set, i.e.,
 *  characters usable in MOO language string constants and on
 *  connections that are not in binary mode.
 *
 *  (XXX -- have thoughts about renaming, but if this comment is
 *  still here, then that has not happened yet.)
 */

#define my_isascii(x) ((unsigned int)(x) < 127)

#endif		/* !UTF_CType_H */

/*
 * $Log$
 *
 */
