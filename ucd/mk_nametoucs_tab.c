#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "gen/nametoucs_hash.h"

#define UCS_CNT 0x110000

const char *program;

void die(const char *msg)
{
  fprintf(stderr, "%s: %s\n", program, msg);
  exit(1);
}

static const char *int24str(int32_t n)
{
  static char str[17];

  sprintf(str, "{0x%02x,0x%02x,0x%02x}",
	  (n & 0xff),
	  ((n >> 8) & 0xff),
	  ((n >> 16) & 0xff));
  return str;
}

static uint32_t prehash(const char *str)
{
  uint32_t hash = PHASHSALT;
  const uint8_t *p = (const uint8_t *)str;

  while ( *p )
    hash = (hash ^ *p++) + ((hash << 27)+(hash >> 5));

  return hash;
}

static int32_t hash_to_ucs[PHASHNKEYS];
static void read_nametoucs_tab(void)
{
  char line[256], *p;
  FILE *f = fopen("gen/nametoucs.tab", "rt");
  if ( !f )
    die("could not open gen/nametoucs.tab");
  int32_t ucs;
  uint32_t hash;
  int i;

  for ( i = 0 ; i < PHASHNKEYS ; i++ )
    hash_to_ucs[i] = -1;

  while ( fgets(line, sizeof line, f) ) {
    if ( (p = strchr(line, '\n')) )
      *p = '\0';

    ucs = strtol(line, NULL, 16);
    hash = _libucd_nametoucs_hash(prehash(line+6));

    if ( hash >= PHASHNKEYS )
      die("hash not minimal");

    if ( hash_to_ucs[hash] != -1 )
      die("hash collision");

    hash_to_ucs[hash] = ucs;
  }

  fclose(f);
}

static void make_nametoucs_tab(void)
{
  FILE *f = fopen("gen/nametoucs_tab.c", "wt");
  if ( !f )
    die("could not create gen/nametoucs_tab.c");
  int i;
  int32_t ucs;

  fprintf(f,
	  "#include \"libucd_int.h\"\n"
	  "const struct _libucd_nametoucs_tab _libucd_nametoucs_tab[] =\n"
	  "{\n");

  for ( i = 0 ; i < PHASHNKEYS ; i++ ) {
    ucs = hash_to_ucs[i];
    fprintf(f, "\t{ %s },\n", int24str(ucs));
  }
  fprintf(f, "};\n");
  fclose(f);
}


int main(void)
{
  read_nametoucs_tab();
  make_nametoucs_tab();

  return 0;
}

