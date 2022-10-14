/**********************************************************
 * config_epilog.h -- epilogue for config.h
 *
 * Additional preprocessing for the hooks defined in config.h
 * This needs to be a separate file, since we need to be
 * certain ./configure is not overwriting any of this.
 *
 * Resolution of defaults/conflicts in config parameters must be dealt
 * with HERE without outside help.  config.h is the first inclusion
 * for many modules; introducing dependencies on particular modules
 * here risks circularity.  Also, config.h-#defined identifiers must
 * not be re-#defined in any other file, since config.h could be
 * included without the other file, leading to inconsistencies.
 */

#if defined(Config_Epilog_H) || !defined(Config_H)
#  error config_epilog.h must only be included from config.h
#else
#  define Config_Epilog_H 1

/* Some sites have installed GCC improperly or incompletely, thereby requiring
 * the server to be compiled with the `-traditional' switch.  That disables the
 * `const', `volatile' or `signed' keywords, which we need.  Thus, for GCC, we
 * do these little substitutions to always refer to its `hidden' names for
 * these keywords.
 */

#if defined(__GNUC__) && !HAVE_SYS_CDEFS_H
#  define const __const__
#  define volatile __volatile__
#  define signed __signed__
#endif

/*
 * The following code figures out how to express 32- and 64-bit integer
 * types on your machine.
 */

#ifdef NEED_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef NEED_STDINT_H
# include <stdint.h>
#endif

#include <limits.h>

#ifndef HAVE_INT32_T
#  if INT_MAX == 2147483647
     typedef int	int32_t;
     typedef unsigned	uint32_t;
#    define INT32_MAX	INT_MAX
#    define PRId32 "d"
#    define PRIi32 "i"
#    define PRIo32 "o"
#    define PRIu32 "u"
#    define PRIx32 "x"
#    define PRIX32 "X"
#    define SCNd32 "d"
#    define SCNi32 "i"
#    define SCNo32 "o"
#    define SCNu32 "u"
#    define SCNx32 "x"
#    define HAVE_INT32_T 1
#  elif LONG_MAX == 2147483647
     typedef long int		int32_t;
     typedef unsigned long	uint32_t;
#    define INT32_MAX		LONG_MAX
#    define PRId32 "ld"
#    define PRIi32 "li"
#    define PRIo32 "lo"
#    define PRIu32 "lu"
#    define PRIx32 "lx"
#    define PRIX32 "lX"
#    define SCNd32 "ld"
#    define SCNi32 "li"
#    define SCNo32 "lo"
#    define SCNu32 "lu"
#    define SCNx32 "lx"
#    define HAVE_INT32_T 1
#  endif
#endif

#ifndef HAVE_INT32_T
#  error This platform does not have 32-bit integers?
#endif

#ifndef HAVE_INT64_T
#  if LONG_MAX == 9223372036854775807
     typedef long          int64_t;
     typedef unsigned long uint64_t;
#    define INT64_MAX      LONG_MAX
#    define PRId64 "ld"
#    define PRIi64 "li"
#    define PRIo64 "lo"
#    define PRIu64 "lu"
#    define PRIx64 "lx"
#    define PRIX64 "lX"
#    define SCNd64 "ld"
#    define SCNi64 "li"
#    define SCNo64 "lo"
#    define SCNu64 "lu"
#    define SCNx64 "lx"
#    define HAVE_INT64_T 1
#  elif defined(HAVE_LONG_LONG) && LONG_LONG_MAX == 9223372036854775807
     typedef long long     int64_t;
     typedef unsigned long long uint64_t;
#    define INT64_MAX      LONG_LONG_MAX
#    define PRId64 "lld"
#    define PRIi64 "lli"
#    define PRIo64 "llo"
#    define PRIu64 "llu"
#    define PRIx64 "llx"
#    define PRIX64 "llX"
#    define SCNd64 "lld"
#    define SCNi64 "lli"
#    define SCNo64 "llo"
#    define SCNu64 "llu"
#    define SCNx64 "llx"
#    define HAVE_INT64_T 1
#  endif
#endif

#ifndef HAVE_INT64_T
#  error This platform does not have 64-bit integers?
#endif

#ifndef HAVE_INT128_T
#  ifdef HAVE___INT128
     typedef __int128           int128_t;
     typedef unsigned __int128 uint128_t;
#    define HAVE_INT128_T 1
#  endif
#endif

#ifndef HAVE_STRTOIMAX
# ifdef HAVE_LONG_LONG
#  define strtoimax strtoll
#  define strtoumax strtoull
# else
#  define strtoimax strtol
#  define strtoumax strtoul
# endif
#endif

#ifndef HAVE_UINT8_T
typedef unsigned char uint8_t;
#endif

/*
 *  Cope with compilers that do not understand
 *  gcc function and variable attributes
 */
#if HAVE_FUNC_ATTRIBUTE_FORMAT
#  define FORMAT(x,y,z) __attribute__((format (x,y,z)))
#else
#  define FORMAT(x,y,z)
#endif

#if HAVE_FUNC_ATTRIBUTE_NORETURN
#  define NORETURN_ void __attribute__((noreturn))
#else
#  define NORETURN_ void
#endif

#if HAVE_VAR_ATTRIBUTE_UNUSED
#  define UNUSED_ __attribute__((__unused__))
#else
#  define UNUSED_
#endif

#endif		/* !Config_Epilog_H */
