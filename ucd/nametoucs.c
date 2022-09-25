#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>

#include "libucd_int.h"
#include "gen/nametoucs_hash.h"

static uint32_t prehash(const char *str)
{
  uint32_t hash = PHASHSALT;
  const uint8_t *p = (const uint8_t *)str;

  while ( *p )
    hash = (hash ^ *p++) + ((hash << 27)+(hash >> 5));

  return hash;
}

/* This returns a candidate UCS for a given name. */
static int32_t name_lookup(const char *name)
{
  int32_t ucs;
  uint32_t hash;

  if ( !strncmp(name, "CJK UNIFIED IDEOGRAPH-", 22) ) {
    char *q;

    ucs = strtol(name+22, &q, 16);
    if ( *q || q < name+26 || q > name+27 )
      return -1;
  } else {
    hash = _libucd_nametoucs_hash(prehash(name));
    if ( hash > PHASHNKEYS )
      return -1;
    ucs = getint24(_libucd_nametoucs_tab[hash].ucs);
  }

  return ucs;
}

const struct unicode_character_data *
unicode_character_lookup(const char *name)
{
  int32_t ucs = name_lookup(name);
  const struct unicode_character_data *ucd =
    unicode_character_data(ucs);

  if ( !ucd )
    return NULL;

  if ( !ucd->name || strcmp(name, ucd->name) ) {
    unicode_character_put(ucd);
    errno = EINVAL;
    return NULL;
  }

  return ucd;
}
