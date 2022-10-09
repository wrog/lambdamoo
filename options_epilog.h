/**********************************************************
 * options_epilog.h -- epilogue for options.h
 *
 * Sanity-check and ensure defaults for various settings.
 * This needs to be a separate file, since we need to be
 * certain ./configure is not overwriting any of this.
 *
 * Resolution of defaults/conflicts in option values must be
 * done HERE and using AT MOST information from config.h.
 * Violation of this rule risks introducing circular
 * dependencies since options.h is usually included
 * early elsewhere.
 *
 * Options must NOT be re-#defined in any other file.
 * Violation of this rule risks introducing inconsistencies
 * since options.h may be included without the other file.
 */

#if defined(Options_Epilog_H) || !defined(Options_H)
#  error options_epilog.h must only be included from options.h
#else
#  define Options_Epilog_H 1


#ifdef BYTECODE_REDUCE_REF
#error Think carefully before enabling BYTECODE_REDUCE_REF.  This feature is still beta.  Comment out this line if you are sure.
#endif

/**********************************************************
 * You shouldn't need to change anything below this point.
 **********************************************************/

#define OBN_OFF		0
#define OBN_ON		1

#ifndef OUT_OF_BAND_PREFIX
#define OUT_OF_BAND_PREFIX ""
#endif
#ifndef OUT_OF_BAND_QUOTE_PREFIX
#define OUT_OF_BAND_QUOTE_PREFIX ""
#endif

#if PATTERN_CACHE_SIZE < 1
#  error Illegal match() pattern cache size!
#endif

#define NP_SINGLE	1
#define NP_TCP		2
#define NP_LOCAL	3

#define NS_BSD		1
#define NS_SYSV		2

#define MP_SELECT	1
#define MP_POLL		2
#define MP_FAKE		3

#include "config.h"

#if NETWORK_PROTOCOL != NP_SINGLE  &&  !defined(MPLEX_STYLE)
#  if NETWORK_STYLE == NS_BSD
#    if HAVE_SELECT
#      define MPLEX_STYLE MP_SELECT
#    else
       #error You cannot use BSD sockets without having select()!
#    endif
#  else				/* NETWORK_STYLE == NS_SYSV */
#    if NETWORK_PROTOCOL == NP_LOCAL
#      if SELECT_WORKS_ON_FIFOS
#        define MPLEX_STYLE MP_SELECT
#      else
#        if POLL_WORKS_ON_FIFOS
#	   define MPLEX_STYLE MP_POLL
#	 else
#	   if FSTAT_WORKS_ON_FIFOS
#	     define MPLEX_STYLE MP_FAKE
#	   else
	     #error I need to be able to do a multiplexing wait on FIFOs!
#	   endif
#	 endif
#      endif
#    else			/* It's a TLI-based networking protocol */
#      if HAVE_POLL
#        define MPLEX_STYLE MP_POLL
#      else
         #error You cannot use TLI without having poll()!
#      endif
#    endif
#  endif
#endif

#if (NETWORK_PROTOCOL == NP_LOCAL || NETWORK_PROTOCOL == NP_SINGLE) && defined(OUTBOUND_NETWORK)
#  error You cannot define "OUTBOUND_NETWORK" with that "NETWORK_PROTOCOL"
#endif

/* make sure OUTBOUND_NETWORK has a value;
   for backward compatibility, use 1 if none given */
#if defined(OUTBOUND_NETWORK) && (( 0 * OUTBOUND_NETWORK - 1 ) == 0)
#undef OUTBOUND_NETWORK
#define OUTBOUND_NETWORK 1
#endif


#if NETWORK_PROTOCOL != NP_LOCAL && NETWORK_PROTOCOL != NP_SINGLE && NETWORK_PROTOCOL != NP_TCP
#  error Illegal value for "NETWORK_PROTOCOL"
#endif

#if NETWORK_STYLE != NS_BSD && NETWORK_STYLE != NS_SYSV && !(NETWORK_PROTOCOL == NP_SINGLE && !defined(NETWORK_STYLE))
#  error Illegal value for "NETWORK_STYLE"
#endif

#if defined(MPLEX_STYLE) 	\
    && MPLEX_STYLE != MP_SELECT \
    && MPLEX_STYLE != MP_POLL \
    && MPLEX_STYLE != MP_FAKE
#  error Illegal value for "MPLEX_STYLE"
#endif

#endif		/* !Options_Epilog_H */
