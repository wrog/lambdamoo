/*
 * utf-ctype.h
 *
 * query Unicode Character Database for character attributes
 */

#ifndef UTF_CType_H
#define UTF_CType_H 1

#include "config.h"

#include "my-ctype.h"

inline int my_isdigit(uint32_t c)      { return isdigit(c); }
inline int my_digitval(uint32_t c)     { return c - '0';    }

inline int my_is_xid_start(uint32_t c) { return isalpha(c); }
inline int my_is_xid_cont(uint32_t c)  { return isalnum(c); }

inline uint32_t my_tolower(uint32_t c) { return tolower(c); }
inline uint32_t my_toupper(uint32_t c) { return toupper(c); }
inline int      my_isspace(uint32_t c) { return isspace(c); }

#define my_isascii(x) ((unsigned int)(x) < 127)

#endif		/* !UTF_CType_H */

/*
 * $Log$
 *
 */
