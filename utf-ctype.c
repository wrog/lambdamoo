/*
 * utf-ctype.c
 *
 * Invoke libucd to test for specific character classes.
 */

#include "config.h"
#include "ucd.h"
#include "utf-ctype.h"

/*------------*
 |   digits   |
 *------------*/

int my_isdigit(int x)
{
    const struct unicode_character_data *ucd;
    int rv;

    ucd = unicode_character_data(x);
    if (!ucd)
	return 0;

    rv = ucd->numeric_type == UC_NT_Decimal;
    unicode_character_put(ucd);

    return rv;
}

int my_digitval(int x)
{
    const struct unicode_character_data *ucd;
    int rv;

    ucd = unicode_character_data(x);
    if (!ucd || ucd->numeric_type != UC_NT_Decimal)
	return 0;

    /* For digits, den == exp == 1 */
    rv = ucd->numeric_value_num;
    unicode_character_put(ucd);

    return rv;
}

/*---------------------------*
 |   identifier characters   |
 *---------------------------*/

/*
 * The XID categories are the best thing in Unicode to what
 * characters should allow to begin and continue identifiers,
 * respectively.
 */
int my_is_xid_start(int x)
{
    const struct unicode_character_data *ucd;
    int rv;

    ucd = unicode_character_data(x);
    if (!ucd)
	return 0;

    rv = !!(ucd->fl & UC_FL_XID_START);
    unicode_character_put(ucd);

    return rv;
}

int my_is_xid_cont(int x)
{
    const struct unicode_character_data *ucd;
    int rv;

    ucd = unicode_character_data(x);
    if (!ucd)
	return 0;

    rv = !!(ucd->fl & UC_FL_XID_CONTINUE);
    unicode_character_put(ucd);

    return rv;
}

/*-------------------------*
 |   simple case folding   |
 *-------------------------*/

/*
 * Yes, this is inadequate in general, but we knew that already.
 * The goal here is to have the MOO programming environment continue to
 * make sense, to have it be deterministic/easy/fast to know when two
 * identifiers are equivalent, not to accomodate every last quirk of
 * world language case folding.  Presumably, future designers of
 * actual international programming languages will now know not to
 * include case-insensitivity in their creations.  --wrog
 */

int my_tolower(int x)
{
    const struct unicode_character_data *ucd;
    int rv;

    ucd = unicode_character_data(x);
    if (!ucd)
	return x;

    rv = ucd->simple_lowercase;
    unicode_character_put(ucd);

    return rv;
}

int my_toupper(int x)
{
    const struct unicode_character_data *ucd;
    int rv;

    ucd = unicode_character_data(x);
    if (!ucd)
	return x;

    rv = ucd->simple_uppercase;
    unicode_character_put(ucd);

    return rv;
}

/*----------------*
 |   whitespace   |
 *----------------*/

int my_isspace(int x)
{
    const struct unicode_character_data *ucd;
    int rv;

    ucd = unicode_character_data(x);
    if (!ucd)
	return x;

    rv = !!(ucd->fl & UC_FL_WHITE_SPACE);
    unicode_character_put(ucd);

    return rv;
}

/*------------------------------*
 |   MOO-string character set   |
 *------------------------------*/

int my_is_printable(int x)
{
    const struct unicode_character_data *ucd;
    int rv;

    if (x == 0x09)
        return 1;
    if ((x <= 0xff && ((x & 0x60) == 0x00 || x == 0x7f)) ||
	(x >= 0xd800 && x <= 0xdfff))
        return 0;

    ucd = unicode_character_data(x);
    if (!ucd)
        return 0;

    rv = !(ucd->fl & UC_FL_NONCHARACTER_CODE_POINT);
    unicode_character_put(ucd);

    return rv;
}
