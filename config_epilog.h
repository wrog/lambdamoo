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

/* The following code figures out how to express a 32-bit integer type on your
 * machine.
 */

#include <limits.h>

#if INT_MAX == 2147483647
   typedef int		int32;
   typedef unsigned	unsigned32;
#  define INT32_MAX	INT_MAX
#else
#  if LONG_MAX == 2147483647
     typedef long int		int32;
     typedef unsigned long	unsigned32;
#    define INT32_MAX		LONG_MAX
#  else
#    error I cannot figure out how to express a 32-bit integer on your machine.
#  endif
#endif

#endif		/* !Config_Epilog_H */
