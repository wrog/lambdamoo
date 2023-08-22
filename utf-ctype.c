/*
 * utf-ctype.c
 *
 * Invoke libucd to test for specific character classes.
 */

#include "config.h"
#include "ucd.h"
#include "utf-ctype.h"

#include "storage.h"

#define RETURN_UCD(_u,_ucd,_return_t,_rv_no_ucd,_rv_from_ucd)	\
    RETURN_UCDGET(_ucd, unicode_character_data(_u),		\
                  _return_t,_rv_no_ucd,_rv_from_ucd)

#define RETURN_UCDGET(_ucd,_ucdget,				\
		      _return_t,_rv_no_ucd,_rv_from_ucd)	\
    const struct unicode_character_data *_ucd;			\
								\
    _ucd = (_ucdget);						\
    if (!_ucd)							\
	return (_rv_no_ucd);					\
								\
    _return_t rv = (_rv_from_ucd);				\
    unicode_character_put(_ucd);				\
								\
    do { return rv; } while(0)


/*------------*
 |   digits   |
 *------------*/

int
my_isdigit(uint32_t x)
{
    RETURN_UCD(x, ucd, int, 0, ucd->numeric_type == UC_NT_Decimal);
}

int
my_digitval(uint32_t x)
{
    /* XXX assert(ucd->numeric_type == UC_NT_Decimal) */
    /* For digits, numeric_value_{den,exp} == 1 */
    RETURN_UCD(x, ucd, int, -1, ucd->numeric_value_num);
}

/*---------------------------*
 |   identifier characters   |
 *---------------------------*/

/*
 * The XID categories are the best thing in Unicode to what
 * characters should allow to begin and continue identifiers,
 * respectively.
 */
int
my_is_xid_start(uint32_t x)
{
    RETURN_UCD(x, ucd, int, 0, !!(ucd->fl & UC_FL_XID_START));
}

int
my_is_xid_cont(uint32_t x)
{
    RETURN_UCD(x, ucd, int, 0, !!(ucd->fl & UC_FL_XID_CONTINUE));
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

uint32_t
my_tolower(uint32_t x)
{
    RETURN_UCD(x, ucd, uint32_t, x, ucd->simple_lowercase);
}

uint32_t
my_toupper(uint32_t x)
{
    RETURN_UCD(x, ucd, uint32_t, x, ucd->simple_uppercase);
}

/*----------------*
 |   whitespace   |
 *----------------*/

int
my_isspace(uint32_t x)
{
    RETURN_UCD(x, ucd, int, x, !!(ucd->fl & UC_FL_WHITE_SPACE));
}

/*------------------------------*
 |   MOO-string character set   |
 *------------------------------*/

int
my_is_printable(uint32_t x)
{
    if (x == 0x09)
        return 1;
    if ((x <= 0xff && ((x & 0x60) == 0x00 || x == 0x7f)) ||
	(x >= 0xd800 && x <= 0xdfff))
        return 0;

    RETURN_UCD(x, ucd, int, 0, !(ucd->fl & UC_FL_NONCHARACTER_CODE_POINT));
}

/*----------------------------------*
 |   character name/number lookup   |
 *----------------------------------*/

const char *
my_char_name(uint32_t x)
{
    RETURN_UCD(x, ucd, char *, NULL, str_dup(ucd->name));
}

uint32_t
my_char_lookup(const char *name)
{
    RETURN_UCDGET(ucd, unicode_character_lookup(name),
		  uint32_t, 0, ucd->ucs);
}
