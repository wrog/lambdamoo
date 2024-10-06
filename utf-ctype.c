/*
 * utf-ctype.c
 *
 * Invoke libucd to test for specific character classes.
 */

#include "config.h"
#include "utf-ctype.h"

#include <uniname.h>
#include <unistring/version.h>
#include <unictype.h>
#include <unicase.h>

/*------------*
 |   digits   |
 *------------*/

#if UNICODE_NUMBERS

int
my_isdigit(uint32_t x)
{
    return uc_is_property_decimal_digit(x);
}

int
my_digitval(uint32_t x)
{
    return uc_decimal_value(x);
}

#endif /* UNICODE_NUMBERS */

/*---------------------------*
 |   identifier characters   |
 *---------------------------*/

#if UNICODE_IDENTIFIERS
/*
 * The XID categories are the best thing in Unicode to what
 * characters should allow to begin and continue identifiers,
 * respectively.
 */
int
my_is_xid_start(uint32_t x)
{
    return uc_is_property_xid_start(x);
}

int
my_is_xid_cont(uint32_t x)
{
    return uc_is_property_xid_continue(x);
}

#endif /* UNICODE_IDENTIFIERS */

/*-------------------------*
 |   simple case folding   |
 *-------------------------*/

#if UNICODE_STRINGS
/*
 * Yes, this is inadequate in general, but we knew that already.
 * The goal here is to have the MOO programming environment continue to
 * make sense, to have it be deterministic/easy/fast to know when two
 * identifiers are equivalent, not to accomodate every last quirk of
 * world language case folding.  Presumably, future designers of
 * actual international programming languages will now know not to
 * include case-insensitivity in their creations.  --wrog
 */

uint32_t
my_tolower(uint32_t x)
{
    return uc_tolower(x);
}

uint32_t
my_toupper(uint32_t x)
{
    return uc_toupper(x);
}

/*----------------*
 |   whitespace   |
 *----------------*/

int
my_isspace(uint32_t x)
{
    return uc_is_property_white_space(x);
}

#endif /* UNICODE_STRINGS */
