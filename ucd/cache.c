#include <stdlib.h>
#include <errno.h>
#include "libucd_int.h"
#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

#include "gen/cache.c"

#ifdef HAVE_PTHREAD_H
static void lock_cache(struct cache_row *r)
{
  pthread_mutex_lock(&r->mutex);
}
static void unlock_cache(struct cache_row *r)
{
  pthread_mutex_unlock(&r->mutex);
}
#else
/* Single-threaded execution only */
static void lock_cache(struct cache_row *r)
{
  (void)r;
}
static void unlock_cache(struct cache_row *r)
{
  (void)r;
}
#endif

#if defined(HAVE_PTHREAD_H) && (defined(__i386__) || defined(__x86_64__))

/* Specially optimized versions for i386 and x86-64 -- similar
   things can be done for any architecture which has atomic
   increment and decrement-with-test. */

const struct unicode_character_data *
unicode_character_get(const struct unicode_character_data *ucd)
{
  struct libucd_private *pvt = (struct libucd_private *)(ucd+1);
  asm volatile("lock ; incl %0" : "+m" (pvt->usage_ctr));
  return ucd;
}

void
unicode_character_put(const struct unicode_character_data *ucd)
{
  struct libucd_private *pvt = (struct libucd_private *)(ucd+1);
  unsigned char zero;

  asm volatile("lock ; decl %0 ; setz %1"
	       : "+m" (pvt->usage_ctr), "=r" (zero));
  if ( zero )
    free((void *)ucd);
}

#else

# ifdef HAVE_PTHREAD_H
static void lock(struct libucd_private *pvt)
{
  pthread_mutex_lock(&pvt->mutex);
}
static void unlock(struct libucd_private *pvt)
{
  pthread_mutex_unlock(&pvt->mutex);
}
# else
static void lock(struct libucd_private *pvt)
{
}
static void unlock(struct libucd_private *pvt)
{
}
# endif

const struct unicode_character_data *
unicode_character_get(const struct unicode_character_data *ucd)
{
  struct libucd_private *pvt = (struct libucd_private *)(ucd+1);
  lock(pvt);
  pvt->usage_ctr++;
  unlock(pvt);
  return ucd;
}

void
unicode_character_put(const struct unicode_character_data *ucd)
{
  struct libucd_private *pvt = (struct libucd_private *)(ucd+1);
  unsigned int cnt;
  lock(pvt);
  cnt = --pvt->usage_ctr;
  unlock(pvt);
  if ( !cnt )
    free((void *)ucd);
}

#endif

const struct unicode_character_data *
unicode_character_data(int32_t ucs)
{
  const struct unicode_character_data *ucd;
  struct cache_row *row;

  if ( unlikely((uint32_t)ucs > UCS_MAX) ) {
    errno = EINVAL;
    return NULL;
  }

  row = &libucd_cache[(uint32_t)ucs % CACHE_ROWS];

  RETURN_ENTRY(ucs, row);
}
