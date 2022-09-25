#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>		/* For snprintf() */
#include <inttypes.h>
#include <errno.h>
#include "libucd_int.h"
#include "gen/ucstoname_hash.h"
#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

/*
 * This returns the name for a specific UCS in a user-provided buffer,
 * and returns the total length regardless of overrun, like snprintf().
 * This is used with names that *are* found in the hash table only.
 */
static void
libucd_mkname(char *buf, const unsigned char *nameslist_ptr)
{
  const unsigned char *p = nameslist_ptr;
  char *q = buf;
  const char *pp;
  char cc;
  int n = *p++;			/* Characters remaining */

  while ( n ) {
    pp = _libucd_nameslist_dict[*p++];
    while ( (cc = *pp++) ) {
      *q++ = cc;
      if ( --n == 0 )
	break;
    }
  }

  *q = '\0';
}

/*
 * Take a character in the range of the Hangul characters, and compute
 * its formal name.  Returns the length, or 0 if invalid.
 */
static size_t
hangul_name(char *buf, size_t n, int32_t codepoint)
{
  /* See the Unicode Standard, version 4.1, section 3.12 */
  const int32_t SBase = 0xAC00;
  const int32_t LCount = 19;
  const int32_t VCount = 21;
  const int32_t TCount = 28;
  const int32_t NCount = VCount * TCount; /* 588 */
  const int32_t SCount = NCount * LCount; /* 11172 */

  int32_t SIndex, L, V, T;

  SIndex = codepoint - SBase;
  if ( SIndex < 0 || SIndex >= SCount )
    return 0;

  L = SIndex/NCount;
  V = (SIndex % NCount)/TCount;
  T = SIndex % TCount;

  return snprintf(buf, n, "HANGUL SYLLABLE %s%s%s",
		  _libucd_hangul_jamo_l[L],
		  _libucd_hangul_jamo_v[V],
		  _libucd_hangul_jamo_t[T]);
}

/*
 * Binary search of the properties array (for non-hashed characters)
 */
static const struct _libucd_property_array *
search_prop_array(int32_t ucs)
{
  int l, h, m;
  const struct _libucd_property_array *pa;

  l = 0;
  h = _libucd_property_array_count-1;

  for (;;) {
    m = (l+h) >> 1;
    pa = &_libucd_property_array[m];
    if ( ucs >= pa[0].ucs ) {
      if ( ucs < pa[1].ucs )
	return pa;
      else
	l = m+1;
    } else {
      h = m-1;
    }
  }
}

/*
 * Allocate memory and copy properties
 */
static struct unicode_character_data *
alloc_copy_properties(const struct _libucd_property_array *prop,
		      int32_t ucs, size_t namelen)
{
  struct unicode_character_data *ucd;
  struct libucd_private *pvt;
  size_t size = sizeof(struct unicode_character_data)+
    sizeof(struct libucd_private)+namelen+1;


  ucd = malloc(size);
  if ( !ucd )
    return NULL;
  pvt = (struct libucd_private *)(ucd+1);
  ucd->name = (char *)(pvt+1);
  ucd->size = sizeof(struct unicode_character_data);
  ucd->alloc_size = size;

  ucd->fl = prop->flags_block & UINT64_C(0xffffffffffff);
  ucd->bidi_mirroring_glyph = NULL; /* NYS */
  ucd->uppercase_mapping = NULL; /* NYS */
  ucd->lowercase_mapping = NULL; /* NYS */
  ucd->titlecase_mapping = NULL; /* NYS */
  ucd->ucs = ucs;
  ucd->simple_uppercase = ucs + getint24(prop->simple_uppercase);
  ucd->simple_lowercase = ucs + getint24(prop->simple_lowercase);
  ucd->simple_titlecase = ucs + getint24(prop->simple_titlecase);
  ucd->numeric_value_num = prop->numeric_value_num;
  if ( prop->numeric_value_den_exp & 0x80 ) {
    ucd->numeric_value_exp = prop->numeric_value_den_exp & 0x7f;
    ucd->numeric_value_den = 1;
  } else {
    ucd->numeric_value_exp = 1;
    ucd->numeric_value_den = prop->numeric_value_den_exp;
  }
  ucd->age_ma = prop->age >> 3;
  ucd->age_mi = prop->age & 7;
  ucd->general_category = prop->general_category;
  ucd->block = (prop->flags_block >> 48) & 0xff;
  ucd->script = prop->script;
  ucd->joining_type = prop->joining_type;
  ucd->joining_group = prop->joining_group;
  ucd->east_asian_width = prop->east_asian_width;
  ucd->hangul_syllable_type = prop->hangul_syllable_type;
  ucd->numeric_type = prop->numeric_type;
  ucd->canonical_combining_class = prop->canonical_combining_class;
  ucd->bidi_class = prop->bidi_class;
  ucd->grapheme_cluster_break = prop->grapheme_cluster_break;
  ucd->sentence_break = prop->sentence_break;
  ucd->word_break = prop->word_break;
  ucd->line_break = prop->line_break;

#if defined(HAVE_PTHREAD_H) && !defined(HAVE_ATOMIC_CTR)
  if ( pthread_mutex_init(&pvt->mutex, NULL) ) {
    free(ucd);
    return NULL;
  }
#endif
  pvt->usage_ctr = 2;		/* cache plus end user */

  return ucd;
}

/*
 * Actual data-generating function.  ucs is required to be
 * in the valid range [0..UCS_MAX].
 */
const struct unicode_character_data *
_libucd_character_data_raw(int32_t ucs)
{
  uint32_t hash;
  const struct _libucd_ucstoname_tab  *unt;
  const struct _libucd_property_array *prop;
  size_t namelen;
  struct unicode_character_data *ucd;

  hash = _libucd_ucstoname_hash(ucs);

  if ( hash >= PHASHNKEYS ) {
    unt = NULL;
  } else {
    unt = &_libucd_ucstoname_tab[hash];
    if ( getint24(unt->ucs) != ucs )
      unt = NULL;
  }

  if ( unt ) {
    const unsigned char *nameptr =
      &_libucd_names_list[getuint24(unt->names_offset)];
    prop = &_libucd_property_array[unt->proparray_offset];
    namelen = *nameptr;

    ucd = alloc_copy_properties(prop, ucs, namelen);
    if ( !ucd )
      return NULL;
    libucd_mkname((char *)ucd->name, nameptr);
  } else {
    prop = search_prop_array(ucs);

    if ( ucs >= 0xAC00 && ucs < 0xAC00+19*21*28 ) {
      namelen = hangul_name(NULL, 0, ucs);
      ucd = alloc_copy_properties(prop, ucs, namelen);
      if ( !ucd )
	return NULL;
      hangul_name((char *)ucd->name, namelen+1, ucs);
    } else if ( prop->flags_block & UC_FL_UNIFIED_IDEOGRAPH ) {
      /* "CJK UNIFIED IDEOGRAPH-XXXX[X] */
      namelen = (ucs > 0xffff) ? 27 : 26;
      ucd = alloc_copy_properties(prop, ucs, namelen);
      if ( !ucd )
	return NULL;
      snprintf((char *)ucd->name, namelen+1, "CJK UNIFIED IDEOGRAPH-%04X", ucs);
    } else {
      /* Unnamed character */
      namelen = -1;
      ucd = alloc_copy_properties(prop, ucs, namelen);
      if ( !ucd )
	return NULL;
      ucd->name = NULL;
    }
  }

  return ucd;
}
