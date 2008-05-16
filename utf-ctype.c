/*
 * utf-ctype.c
 *
 * Invoke libucd to test for specific character classes.
 */

#include "config.h"
#include "utf-ctype.h"

#include "storage.h"
#include "streams.h"

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

    return x <= 0x10ffff &&
	!uc_is_property_not_a_character(x);
}

#endif /* UNICODE_STRINGS */

/*----------------------------------*
 |   character name/number lookup   |
 *----------------------------------*/

#if !UNICODE_STRINGS
/* Build character tables for ASCII World */

static
struct ascii_char_entry {
    unsigned start;
    const char **names;
    const char *format;
}
    ascii_char_info[] = {
    { '\0',   NULL, NULL },
    { '\t', (const char *[]){ "HORIZONTAL TAB" }, NULL },
    { '\t'+1, NULL, NULL },
    { ' ', (const char *[]){
	    "SPACE", "EXCLAMATION MARK", "QUOTATION MARK", "NUMBER SIGN",
	    "DOLLAR SIGN", "PERCENT SIGN", "AMPERSAND", "APOSTROPHE",
	    "LEFT PARENTHESIS", "RIGHT PARENTHESIS", "ASTERISK", "PLUS SIGN",
	    "COMMA", "HYPHEN-MINUS", "FULL STOP", "SOLIDUS",
	},
      NULL },
    { '0', (const char *[]){
	    "ZERO", "ONE", "TWO", "THREE", "FOUR",
	    "FIVE", "SIX", "SEVEN", "EIGHT", "NINE",
	},
      "DIGIT %s" },
    { ':', (const char *[]){
	    "COLON", "SEMICOLON", "LESS-THAN SIGN", "EQUALS SIGN",
	    "GREATER-THAN SIGN", "QUESTION MARK", "COMMERCIAL AT",
	},
      NULL },
    { 'A', NULL, "LATIN CAPITAL LETTER %c", },
    { '[', (const char *[]){
	    "LEFT SQUARE BRACKET", "REVERSE SOLIDUS", "RIGHT SQUARE BRACKET",
	    "CIRCUMFLEX ACCENT", "LOW LINE", "GRAVE ACCENT",
	},
      NULL },
    { 'a', NULL, "LATIN SMALL LETTER %c", },
    { '{', (const char *[]){
	    "LEFT CURLY BRACKET", "VERTICAL LINE", "RIGHT CURLY BRACKET",
	    "TILDE",
	},
      NULL },
    { '~'+1, NULL, NULL },
};

static const char *
ascii_char_name(Stream *s, uint32_t c)
{
    struct ascii_char_entry *ace = ascii_char_info;
    while (ace->start <= '~' && c >= ace[1].start) ++ace;
    if (ace->format) {
	if (ace->names)
	    stream_printf(s, ace->format, ace->names[c - ace->start]);
	else
	    stream_printf(s, ace->format, (ace->start&~0x20) + c - ace->start);
	return reset_stream(s);
    }
    else
	return ace->names ? ace->names[c - ace->start] : NULL;
}

/* The following is from a gperf run, which I suppose ought to be part
 * of the build process, if it were, like, EVER going to change again...
 */
static unsigned int
hash (register const char *str, register size_t len)
{
  static const uint8_t asso_values[] =
    {
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
       73, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163,  10, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163,  46,  16,  32,  22,  18,
       91,  67,  86,  37, 104,   6,  11,  60,  20,  63,
       56,  53,  26,   8,  13, 102,  42,  50,  58,  82,
       72,   5,  15,  16, 163, 163, 163,  46,  16,  32,
       22,  18,  91,  67,  86,  37, 104,   6,  11,  60,
       20,  63,  56,  53,  26,   8,  13, 102,  42,  50,
       58,  82,  72,   5,  15,  16, 163, 163, 163, 163,
       16, 163, 163, 163, 163, 163
    };
  register unsigned int hval = 0;

  switch (len)
    {
      default:
        hval += asso_values[(unsigned char)str[6]+8];
      /*FALLTHROUGH*/
      case 6: case 5: case 4: case 3: case 2: case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

#define MIN_HASH_VALUE 21
#define MAX_HASH_VALUE 162
/* maximum key range = 142, duplicates = 0 */

static int hash_to_c[142] = {
     47, 107,  75, 115,  83,  32, 108,  76, 116,  84, 126, 98,
     66, 101,  69, 110,  78, 100,  68, 123,  59, 114,  82, 93,
    125,  -1,  55,  99,  67,  -1,  64,  58, 105,  73,  51, 40,
     42, 118,  86,  53,  33,  97,  65,  -1,  94, 119,  87, 52,
    113,  81,  95, 112,  80, 120,  88, 109,  77,  44, 111, 79,
     -1,  57, 103,  71,  54,  -1,  60, 122,  90,  49,  37, 41,
     -1,  92,  56,  -1, 124, 121,  89,  50,  48, 104,  72, 45,
     38,  91, 102,  70,  63,  -1,  61,  34,  35,  39,  36, -1,
     96, 117,  85, 106,  74,  -1,  -1,  -1,  -1,  -1,  -1, -1,
     43,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1,
     -1,  -1,  -1,   9,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1,
     -1,  -1,  -1,  -1,  -1,  -1,  62,  -1,  -1,  46
};

static uint32_t
ascii_char_lookup(const char* name)
{
    size_t len = strlen(name);
    int h = hash(name,len);
    if (h < MIN_HASH_VALUE || MAX_HASH_VALUE < h)
	return 0;
    int32_t c = hash_to_c[h - MIN_HASH_VALUE];
    if (c < 0)
	return 0;
    Stream *s = new_stream(0);
    if (0 != mystrncasecmp(ascii_char_name(s, c), name, len))
	c = 0;
    free_stream(s);
    return c;
}

#endif /* !UNICODE_STRINGS */

const char *
my_char_name(uint32_t x)
{
#if UNICODE_STRINGS
    if (x > 0x10ffff)
#else
    if (x > 0x7e)
#endif
	return NULL;

    Stream *s = new_stream(0);

#if !UNICODE_STRINGS
    const char *name;
    name = ascii_char_name(s, x);
#else
    char *name;
    size_t nlen;
    stream_beginfill(s, UNINAME_MAX-1, &name, &nlen);
    name = unicode_character_name(x, name);
#endif
    if (name)
	name = str_dup(name);
    else if (x == '\t')
	name = str_dup("HORIZONTAL TAB");
    free_stream(s);
    return name;
}

uint32_t
my_char_lookup(const char *name)
{
#if !UNICODE_STRINGS
    return ascii_char_lookup(name);
#else
    uint32_t ucs = unicode_name_character(name);
    return (ucs == UNINAME_INVALID) ? 0 : ucs;
#endif
}
