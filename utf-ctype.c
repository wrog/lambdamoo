/*
 * utf-ctype.c
 *
 * Interface to unicode library, one of
 *   UD_GNU = GNU's libunistring
 *   UD_ICU = IBM's libicu(uc)
 *   UD_UCD = HPA's libucd
 */

#include "config.h"
#include "utf-ctype.h"

#include "storage.h"
#include "streams.h"
#include "exceptions.h"

#if   UNICODE_DATA == UD_GNU
#  include <uniname.h>
#  include <unistring/version.h>
#  include <unictype.h>
#  include <unicase.h>

#elif UNICODE_DATA == UD_ICU
#  include <unicode/uchar.h>

#elif UNICODE_DATA == UD_UCD
#  include <ucd.h>

#  define RETURN_UCD(_u,_ucd,_return_t,_rv_no_ucd,_rv_from_ucd)	\
    RETURN_UCDGET(_ucd, unicode_character_data(_u),		\
                  _return_t,_rv_no_ucd,_rv_from_ucd)

#  define RETURN_UCDGET(_ucd,_ucdget,				\
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

#endif	/* UNICODE_DATA */

/*------------*
 |   digits   |
 *------------*/

#if UNICODE_NUMBERS

int
my_isdigit(uint32_t x)
{
#  if   UNICODE_DATA == UD_GNU
    return uc_is_property_decimal_digit(x);

#  elif UNICODE_DATA == UD_ICU
    return u_getIntPropertyValue(x,UCHAR_NUMERIC_TYPE) == U_NT_DECIMAL;

#  elif UNICODE_DATA == UD_UCD
    RETURN_UCD(x, ucd, int, 0, ucd->numeric_type == UC_NT_Decimal);

#  endif
}

int
my_digitval(uint32_t x)
{
#  if   UNICODE_DATA == UD_GNU
    return uc_decimal_value(x);

#  elif UNICODE_DATA == UD_ICU
    return u_charDigitValue(x);

#  elif UNICODE_DATA == UD_UCD
    /* XXX assert(ucd->numeric_type == UC_NT_Decimal) */
    /* For digits, numeric_value_{den,exp} == 1 */
    RETURN_UCD(x, ucd, int, -1, ucd->numeric_value_num);

#  endif
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
#  if   UNICODE_DATA == UD_GNU
    return uc_is_property_xid_start(x);

#  elif UNICODE_DATA == UD_ICU
    return u_hasBinaryProperty(x,UCHAR_XID_START);

#  elif UNICODE_DATA == UD_UCD
    RETURN_UCD(x, ucd, int, 0, !!(ucd->fl & UC_FL_XID_START));

#  endif
}

int
my_is_xid_cont(uint32_t x)
{
#  if   UNICODE_DATA == UD_GNU
    return uc_is_property_xid_continue(x);

#  elif UNICODE_DATA == UD_ICU
    return u_hasBinaryProperty(x,UCHAR_XID_CONTINUE);

#  elif UNICODE_DATA == UD_UCD
    RETURN_UCD(x, ucd, int, 0, !!(ucd->fl & UC_FL_XID_CONTINUE));

#  endif
}

#endif /* UNICODE_IDENTIFIERS */

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
#if   UNICODE_DATA == UD_GNU
    return uc_tolower(x);

#elif UNICODE_DATA == UD_ICU
    return u_tolower(x);

#elif UNICODE_DATA == UD_UCD
    RETURN_UCD(x, ucd, uint32_t, x, ucd->simple_lowercase);

#endif
}

uint32_t
my_toupper(uint32_t x)
{
#if   UNICODE_DATA == UD_GNU
    return uc_toupper(x);

#elif UNICODE_DATA == UD_ICU
    return u_toupper(x);

#elif UNICODE_DATA == UD_UCD
    RETURN_UCD(x, ucd, uint32_t, x, ucd->simple_uppercase);

#endif
}

/*----------------*
 |   whitespace   |
 *----------------*/

int
my_isspace(uint32_t x)
{
#if   UNICODE_DATA == UD_ICU
    return u_isUWhiteSpace(x);

#elif UNICODE_DATA == UD_GNU
    return uc_is_property_white_space(x);

#elif UNICODE_DATA == UD_UCD
    RETURN_UCD(x, ucd, int, x, !!(ucd->fl & UC_FL_WHITE_SPACE));

#endif
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

#if   UNICODE_DATA == UD_UCD
    RETURN_UCD(x, ucd, int, 0, !(ucd->fl & UC_FL_NONCHARACTER_CODE_POINT));

#else  /* UNICODE_DATA != UD_UCD */
    return x <= 0x10ffff &&

#  if   UNICODE_DATA == UD_GNU
	!uc_is_property_not_a_character(x)

#  elif UNICODE_DATA == UD_ICU
        !u_hasBinaryProperty(x,UCHAR_NONCHARACTER_CODE_POINT)

#  endif
	;
#endif  /* UNICODE_DATA != UD_UCD */
}

/*----------------------------------*
 |   character name/number lookup   |
 *----------------------------------*/

const char *
my_char_name(uint32_t x)
{
    if (x > 0x10ffff)
	return NULL;

#if   UNICODE_DATA == UD_UCD
    RETURN_UCD(x, ucd, char *, NULL, str_dup(ucd->name));

#else  /* UNICODE_DATA != UD_UCD */
    Stream *s = new_stream(0);
    char *name;
    size_t nlen;

#  if   UNICODE_DATA == UD_GNU
    stream_beginfill(s, UNINAME_MAX-1, &name, &nlen);
    name = unicode_character_name(x, name);

#  elif UNICODE_DATA == UD_ICU
    UErrorCode pErr = U_ZERO_ERROR;
    stream_beginfill(s, 20, &name, &nlen);
    ssize_t l = u_charName(x, U_UNICODE_CHAR_NAME, name, nlen, &pErr);
    if (!l)
	name = NULL;
    else {
	if (l >= (ssize_t)nlen) {
	    stream_beginfill(s, l+1, &name, &nlen);
	    pErr = U_ZERO_ERROR;
	    l = u_charName(x, U_UNICODE_CHAR_NAME, name, nlen, &pErr);
	}
	if (U_FAILURE(pErr))
	    panic("ICU error charname");
	stream_endfill(s, nlen - l);
	name = reset_stream(s);
    }

#  endif  /* UNICODE_DATA == UD_ICU */

    if (name)
	name = str_dup(name);
    else if (x == '\t')
	name = str_dup("HORIZONTAL TAB");
    free_stream(s);
    return name;

#endif	/* UNICODE_DATA != UD_UCD */
}

uint32_t
my_char_lookup(const char *name)
{
#if   UNICODE_DATA == UD_GNU
    uint32_t ucs = unicode_name_character(name);
    return (ucs == UNINAME_INVALID) ? 0 : ucs;

#elif UNICODE_DATA == UD_ICU
    UErrorCode pErr = U_ZERO_ERROR;
    uint32_t ucs = u_charFromName(U_UNICODE_CHAR_NAME,name,&pErr);
    return (U_FAILURE(pErr)) ? 0 : ucs;

#elif UNICODE_DATA == UD_UCD
    RETURN_UCDGET(ucd, unicode_character_lookup(name),
		  uint32_t, 0, ucd->ucs);
#endif
}

/*-------------------------*
 |   version information   |
 *-------------------------*/

#ifdef UNICODE_DATA

uint32_t
my_unicode_version(void)
{
#  if   UNICODE_DATA == UD_GNU
    return (
#    if _LIBUNISTRING_VERSION >= 0x10200
	_libunistring_unicode_version
#    else
	_libunistring_version >= 0x10100 ? 0xf00 :
	_libunistring_version >= 0x10000 ? 0xe00 :
	_libunistring_version >=   0x908 ? 0x900 :
	_libunistring_version >=   0x906 ? 0x800 :
	_libunistring_version >=   0x905 ? 0x700 :
	_libunistring_version >=   0x904 ? 0x600 :
					   0x100
#    endif
	    ) << 8;

#  elif UNICODE_DATA == UD_ICU
    UVersionInfo uver;
    u_getUnicodeVersion(uver);
    return (((uver[0]<<8)+uver[1])<<8)+uver[2];

#  elif UNICODE_DATA == UD_UCD
    return unicode_database_version();

#  endif
}

uint32_t
my_unilib_version(void)
{
#  if   UNICODE_DATA == UD_GNU
    return _libunistring_version;

#  elif UNICODE_DATA == UD_ICU
    UVersionInfo ver;
    u_getVersion(ver);
    return (((ver[0]<<8)+ver[1])<<8)+ver[2];

#  elif UNICODE_DATA == UD_UCD
    return unicode_library_version();

#  endif
}

const char *my_unilib_name =
#  if   UNICODE_DATA == UD_GNU
    "unistring"

#  elif UNICODE_DATA == UD_ICU
    "ICU"

#  elif UNICODE_DATA == UD_UCD
    "UCD (hpa)"

#  endif
    ;

#endif /* UNICODE_DATA */
