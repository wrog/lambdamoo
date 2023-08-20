/*
 * utf-ctype.h
 *
 * Invoke libucd to test for specific character classes.
 */

#ifndef UTF_CType_H
#define UTF_CType_H 1

#include "config.h"

extern int my_isdigit(int);
extern int my_digitval(int);

extern int my_is_xid_start(int);
extern int my_is_xid_cont(int);

extern int my_tolower(int);
extern int my_toupper(int);
extern int my_isspace(int);

extern int my_is_printable(int);
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
