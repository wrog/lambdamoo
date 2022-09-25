/*
------------------------------------------------------------------------------
perfect.h: code to generate code for a hash for perfect hashing.
(c) Bob Jenkins, September 1996
You may use this code in any way you wish, and it is free.  No warranty.
I hereby place this in the public domain.
Source is http://burtleburtle.net/bob/c/perfect.h
------------------------------------------------------------------------------
*/

#ifndef STANDARD
#include "standard.h"
#endif

#ifndef PERFECT
#define PERFECT

#define MAXKEYLEN 256                             /* maximum length of a key */
#define USE_SCRAMBLE  4096           /* use scramble if blen >= USE_SCRAMBLE */
#define SCRAMBLE_LEN ((uint32_t)1<<16)                    /* length of *scramble* */
#define RETRY_INITKEY 2048  /* number of times to try to find distinct (a,b) */
#define RETRY_PERFECT 1     /* number of times to try to make a perfect hash */
#define RETRY_HEX     200               /* RETRY_PERFECT when hex keys given */

/* the generated code for the final hash, assumes initial hash is done */
struct gencode
{
  char **line;                       /* array of text lines, 80 bytes apiece */
  /*
   * The code placed here must declare "uint32_t rsl"
   * and assign it the value of the perfect hash using the function inputs.
   * Later code will be tacked on which returns rsl or manipulates it according
   * to the user directives.
   *
   * This code is at the top of the routine; it may and must declare any
   * local variables it needs.
   *
   * Each way of filling in **line should be given a comment that is a unique
   * tag.  A testcase named with that tag should also be found which tests
   * the generated code.
   */
  uint32_t    len;                    /* number of lines available for final hash */
  uint32_t    used;                         /* number of lines used by final hash */

  uint32_t    lowbit;                          /* for HEX, lowest interesting bit */
  uint32_t    highbit;                        /* for HEX, highest interesting bit */
  uint32_t    diffbits;                         /* bits which differ for some key */
  uint32_t    i,j,k,l,m,n,o;                      /* state machine used in hexn() */
};
typedef  struct gencode  gencode;

/* user directives: perfect hash? minimal perfect hash? input is an int? */
struct hashform
{
  enum {
    NORMAL_HM,                                            /* key is a string */
    INLINE_HM,    /* user will do initial hash, we must choose salt for them */
    HEX_HM,              /* key to be hashed is a hexidecimal 4-byte integer */
    DECIMAL_HM,              /* key to be hashed is a decimal 4-byte integer */
    AB_HM,      /* key to be hashed is "A B", where A and B are (A,B) in hex */
    ABDEC_HM                                   /* like AB_HM, but in decimal */
  } mode;
  enum {
    STRING_HT,                                            /* key is a string */
    INT_HT,                                             /* key is an integer */
    AB_HT             /* dunno what key is, but input is distinct (A,B) pair */
  } hashtype;
  enum {
    NORMAL_HP,                                   /* just find a perfect hash */
    MINIMAL_HP                                /* find a minimal perfect hash */
  } perfect;
  enum {
    FAST_HS,                                                    /* fast mode */
    SLOW_HS                                                     /* slow mode */
  } speed;
};
typedef  struct hashform  hashform;

/* representation of a key */
struct key
{
  uint8_t        *name_k;                                      /* the actual key */
  uint32_t         len_k;                         /* the length of the actual key */
  uint32_t         hash_k;                 /* the initial hash value for this key */
  struct key *next_k;                                            /* next key */
/* beyond this point is mapping-dependent */
  uint32_t         a_k;                            /* a, of the key maps to (a,b) */
  uint32_t         b_k;                            /* b, of the key maps to (a,b) */
  struct key *nextb_k;                               /* next key with this b */
};
typedef  struct key  key;

/* things indexed by b of original (a,b) pair */
struct bstuff
{
  uint16_t  val_b;                                        /* hash=a^tabb[b].val_b */
  key *list_b;                   /* tabb[i].list_b is list of keys with b==i */
  uint32_t  listlen_b;                                        /* length of list_b */
  uint32_t  water_b;           /* high watermark of who has visited this map node */
};
typedef  struct bstuff  bstuff;

/* things indexed by final hash value */
struct hstuff
{
  key *key_h;                   /* tabh[i].key_h is the key with a hash of i */
};
typedef  struct hstuff hstuff;

/* things indexed by queue position */
struct qstuff
{
  bstuff *b_q;                        /* b that currently occupies this hash */
  uint32_t     parent_q;     /* queue position of parent that could use this hash */
  uint16_t     newval_q;      /* what to change parent tab[b] to to use this hash */
  uint16_t     oldval_q;                              /* original value of tab[b] */
};
typedef  struct qstuff  qstuff;

/* return ceiling(log based 2 of x) */
uint32_t ilog2(uint32_t val);

/* Given the keys, scramble[], and hash mode, find the perfect hash */
void findhash(bstuff **tabb, uint32_t *alen, uint32_t *blen, uint32_t *salt, gencode *final, uint32_t *scramble, uint32_t *smax, key *keys, uint32_t nkeys, hashform *form);

/* private, but in a different file because it's excessively verbose */
int inithex(key *keys, uint32_t nkeys, uint32_t alen, uint32_t blen, uint32_t smax, uint32_t salt, gencode *final, hashform *form);

#endif /* PERFECT */
