#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "gen/ucstoname_hash.h"

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

static int proparrayindex[UCS_CNT];
static void read_proparrayindex(void)
{
  FILE *f = fopen("gen/proparrayindex", "rt");
  if ( !f )
    die("could not open gen/proparrayindex");
  int last = 0, prev_ix = -1;
  int curr, i, ix;

  while ( fscanf(f, "%x %d\n", &curr, &ix) == 2 ) {
    for ( i = last ; i < curr ; i++ )
      proparrayindex[i] = prev_ix;
    last = curr;
    prev_ix = ix;
  }

  for ( i = last ; i < UCS_CNT ; i++ )
    proparrayindex[i] = prev_ix;

  fclose(f);
}


static int nameslistoffset[UCS_CNT];
static void read_nameslistoffset(void)
{
  FILE *f = fopen("gen/nameslist.offset", "rt");
  if ( !f )
    die("could not open gen/nameslist.offset");
  int curr, offset;
  int i;

  for ( i = 0 ; i < UCS_CNT ; i++ )
    nameslistoffset[i] = -1;

  while ( fscanf(f, "%x %d\n", &curr, &offset) == 2 )
    nameslistoffset[curr] = offset;

  fclose(f);
}


int32_t hash_to_ucs[PHASHNKEYS];
static void compute_hash_to_ucs(void)
{
  uint32_t hash;
  int i;

  for ( i = 0 ; i < PHASHNKEYS ; i++ )
    hash_to_ucs[i] = -1;

  for ( i = 0 ; i < UCS_CNT ; i++ ) {
    if ( nameslistoffset[i] != -1 ) {
      hash = _libucd_ucstoname_hash(i);

      if ( hash >= PHASHNKEYS )
	die("hash not minimal");

      if ( hash_to_ucs[hash] != -1 )
	die("hash collision");

      hash_to_ucs[hash] = i;
    }
  }
}


static void make_ucstoname_tab(void)
{
  FILE *f = fopen("gen/ucstoname_tab.c", "wt");
  if ( !f )
    die("could not create gen/ucstoname_tab.c");
  int i;
  int32_t ucs;

  fprintf(f,
	  "#include \"libucd_int.h\"\n"
	  "const struct _libucd_ucstoname_tab _libucd_ucstoname_tab[] =\n"
	  "{\n");

  for ( i = 0 ; i < PHASHNKEYS ; i++ ) {
    ucs = hash_to_ucs[i];
    fprintf(f, "\t{ %s, ", int24str(ucs));
    fprintf(f, "%s, ", int24str(nameslistoffset[ucs]));
    fprintf(f, "%d },\n", proparrayindex[ucs]);
  }
  fprintf(f, "};\n");
  fclose(f);
}


int main(void)
{
  read_proparrayindex();
  read_nameslistoffset();
  compute_hash_to_ucs();
  make_ucstoname_tab();

  return 0;
}

