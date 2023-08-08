/******************************************************************************
  Copyright (c) 1992, 1995, 1996 Xerox Corporation.  All rights reserved.
  Portions of this code were written by Stephen White, aka ghond.
  Use and copying of this software and preparation of derivative works based
  upon this software are permitted.  Any distribution of this software or
  derivative works must comply with all applicable United States export
  control laws.  This software is made available AS IS, and Xerox Corporation
  makes no warranty about the software, its performance or its conformity to
  any specification.  Any person obtaining a copy of this software is requested
  to send their name and post office or electronic mail address to:
    Pavel Curtis
    Xerox PARC
    3333 Coyote Hill Rd.
    Palo Alto, CA 94304
    Pavel@Xerox.Com
 *****************************************************************************/

#ifndef Options_h
#define Options_h 1

/******************************************************************************
 * The server is prepared to keep a log of every command entered by players
 * since the last checkpoint.  The log is flushed whenever a checkpoint is
 * successfully begun and is dumped into the server log when and if the server
 * panics.  Define LOG_COMMANDS to enable this logging.
 */

/* #define LOG_COMMANDS */

/******************************************************************************
 * The server normally forks a separate process to make database checkpoints;
 * the original process continues to service user commands as usual while the
 * new process writes out the contents of its copy of memory to a disk file.
 * This checkpointing process can take quite a while, depending on how big your
 * database is, so it's usually quite convenient that the server can continue
 * to be responsive while this is taking place.  On some systems, however,
 * there may not be enough memory to support two simultaneously running server
 * processes.  Define UNFORKED_CHECKPOINTS to disable server forking for
 * checkpoints.
 */

/* #define UNFORKED_CHECKPOINTS */

/******************************************************************************
 * If OUT_OF_BAND_PREFIX is defined as a non-empty string, then any lines of
 * input from any player that begin with that prefix will bypass both normal
 * command parsing and any pending read()ing task, instead spawning a server
 * task invoking #0:do_out_of_band_command(word-list).  This is intended for
 * use by fancy clients that need to send reliably-understood messages to the
 * server.
 */

#define OUT_OF_BAND_PREFIX "#$#"

/******************************************************************************
 * If OUT_OF_BAND_QUOTE_PREFIX is defined as a non-empty string, then any
 * lines of input from any player that begin with that prefix will be
 * stripped of that prefix and processed normally (whether to be parsed a
 * command or given to a pending read()ing task), even if the resulting line
 * begins with OUT_OF_BAND_PREFIX.  This provides a means of quoting lines
 * that would otherwise spawn #0:do_out_of_band_command tasks
 */

#define OUT_OF_BAND_QUOTE_PREFIX "#$\""

/******************************************************************************
 * The following constants define the execution limits placed on all MOO tasks.
 *
 * DEFAULT_MAX_STACK_DEPTH is the default maximum depth allowed for the MOO
 *	verb-call stack, the maximum number of dynamically-nested calls at any
 *	given time.  If defined in the database and larger than this default,
 *	$server_options.max_stack_depth overrides this default.
 * DEFAULT_FG_TICKS and DEFAULT_BG_TICKS are the default maximum numbers of
 *	`ticks' (basic operations) any task is allowed to use without
 *	suspending.  If defined in the database, $server_options.fg_ticks and
 *	$server_options.bg_ticks override these defaults.
 * DEFAULT_FG_SECONDS and DEFAULT_BG_SECONDS are the default maximum numbers of
 *	real-time seconds any task is allowed to use without suspending.  If
 *	defined in the database, $server_options.fg_seconds and
 *	$server_options.bg_seconds override these defaults.
 *
 * The *FG* constants are used only for `foreground' tasks (those started by
 * either player input or the server's initiative and that have never
 * suspended); the *BG* constants are used only for `background' tasks (forked
 * tasks and those of any kind that have suspended).
 *
 * The values given below are documented in the LambdaMOO Programmer's Manual,
 * so they should either be left as they are or else the manual should be
 * updated.
 */

#define DEFAULT_MAX_STACK_DEPTH	50

#define DEFAULT_FG_TICKS	30000
#define DEFAULT_BG_TICKS	15000

#define DEFAULT_FG_SECONDS	5
#define DEFAULT_BG_SECONDS	3

/******************************************************************************
 * Debug settings:
 *
 * DEBUG_LOG_TRACEBACKS prints all tracebacks to the server log.
 */

#define DEBUG_LOG_TRACEBACKS

/******************************************************************************
 * NETWORK_PROTOCOL must be defined as one of the following:
 *
 * NP_SINGLE	The server will accept only one user at a time, communicating
 *		with them using the standard input and output streams of the
 *		server itself.
 * NP_TCP	The server will use TCP/IP protocols, such as are used by the
 *		Internet `telnet' command.
 * NP_LOCAL	The server will use a non-networking mechanism, entirely local
 *		to the machine on which the server is running.  Depending on
 *		the value of NETWORK_STYLE, below, this will be either UNIX-
 *		domain sockets (NS_BSD) or named pipes (NS_SYSV).
 *
 * If NP_TCP is selected, then DEFAULT_PORT is the TCP port number on which the
 * server listens when no port argument is given on the command line.
 *
 * If NP_LOCAL is selected, then DEFAULT_CONNECT_FILE is the name of the UNIX
 * pseudo-file through which the server will listen for connections when no
 * file name is given on the command line.
 */

#define NETWORK_PROTOCOL 	NP_TCP

#define DEFAULT_PORT 		7777
#define DEFAULT_CONNECT_FILE	"/tmp/.MOO-server"

/******************************************************************************
 * If NETWORK_PROTOCOL is not defined as NP_SINGLE, then NETWORK_STYLE must be
 * defined as one of the following:
 *
 * NS_BSD	The server will use implementation techniques appropriate to a
 *		BSD-style UNIX system.
 * NS_SYSV	The server will use implementation techniques appropriate to an
 *		AT&T UNIX System V system.
 */

#define NETWORK_STYLE NS_BSD

/******************************************************************************
 * If NETWORK_PROTOCOL is not defined as NP_SINGLE, then MPLEX_STYLE must be
 * defined as one of the following:
 *
 * MP_SELECT	The server will assume that the select() system call exists.
 * MP_POLL	The server will assume that the poll() system call exists and
 *		that, if NETWORK_PROTOCOL is NP_LOCAL above, it works for
 *		named pipes (FIFOs).
 * MP_FAKE	The server will use a nasty trick that works only if you've
 *		defined NETWORK_PROTOCOL as NP_LOCAL and NETWORK_STYLE as
 *		NS_SYSV above.
 *
 * Usually, it works best to leave MPLEX_STYLE undefined and let the code at
 * the bottom of this file pick the right value.
 */

/* #define MPLEX_STYLE MP_POLL */

/******************************************************************************
 * The built-in MOO function open_network_connection(), when enabled,
 * allows (only) wizard-owned MOO code to make outbound network connections
 * from the server.  When disabled, it raises E_PERM whenever called.
 *
 * The +O and -O command line options can explicitly enable and disable this
 * function.  If neither option is supplied, the definition given to
 * OUTBOUND_NETWORK here determines the default behavior
 * (use 0 to disable by default, 1 or blank to enable by default).
 *
 * If OUTBOUND_NETWORK is not defined at all,
 * open_network_connection() is permanently disabled and +O is ignored.
 *
 * *** THINK VERY HARD BEFORE ENABLING THIS FUNCTION ***
 * In some contexts, this could represent a serious breach of security.
 *
 * Note: OUTBOUND_NETWORK may not be defined if NETWORK_PROTOCOL is either
 *	 NP_SINGLE or NP_LOCAL.
 */

/* disable by default, +O enables: */
/* #define OUTBOUND_NETWORK 0 */

/* enable by default, -O disables: */
/* #define OUTBOUND_NETWORK 1 */


/******************************************************************************
 * The following constants define certain aspects of the server's network
 * behavior if NETWORK_PROTOCOL is not defined as NP_SINGLE.
 *
 * MAX_QUEUED_OUTPUT is the maximum number of output characters the server is
 *		     willing to buffer for any given network connection before
 *		     discarding old output to make way for new.  The server
 *		     only discards output after attempting to send as much as
 *		     possible on the connection without blocking.
 * MAX_QUEUED_INPUT is the maximum number of input characters the server is
 *		    willing to buffer from any given network connection before
 *		    it stops reading from the connection at all.  The server
 *		    starts reading from the connection again once most of the
 *		    buffered input is consumed.
 * DEFAULT_CONNECT_TIMEOUT is the default number of seconds an un-logged-in
 *			   connection is allowed to remain idle without being
 *			   forcibly closed by the server; this can be
 *			   overridden by defining the `connect_timeout'
 *			   property on $server_options or on L, for connections
 *			   accepted by a given listener L.
 */

#define MAX_QUEUED_OUTPUT	65536
#define MAX_QUEUED_INPUT	MAX_QUEUED_OUTPUT
#define DEFAULT_CONNECT_TIMEOUT	300

/******************************************************************************
 * On connections that have not been set to binary mode, the server normally
 * discards incoming characters that are not printable ASCII, including
 * backspace (8) and delete(127).  If INPUT_APPLY_BACKSPACE is defined,
 * backspace and delete cause the preceding character (if any) to be removed
 * from the input stream.  (Comment this out to restore pre-1.8.3 behavior)
 */
#define INPUT_APPLY_BACKSPACE

/******************************************************************************
 * The server maintains a cache of the most recently used patterns from calls
 * to the match() and rmatch() built-in functions.  PATTERN_CACHE_SIZE controls
 * how many past patterns are remembered by the server.  Do not set it to a
 * number less than 1.
 */

#define PATTERN_CACHE_SIZE	20

/******************************************************************************
 * If you don't plan on using protecting built-in properties (like
 * .name and .location), define IGNORE_PROP_PROTECTED.  The extra
 * property lookups on every reference to a built-in property are
 * expensive.
 */

#define IGNORE_PROP_PROTECTED

/******************************************************************************
 * The code generator can now recognize situations where the code will not
 * refer to the value of a variable again and generate opcodes that will
 * keep the interpreter from holding references to the value in the runtime
 * environment variable slot.  Before, when doing something like x=f(x), the
 * interpreter was guaranteed to have a reference to the value of x while f()
 * was running, meaning that f() always had to copy x to modify it.  With
 * BYTECODE_REDUCE_REF enabled, f() could be called with the last reference
 * to the value of x.  So for example, x={@x,y} can (if there are no other
 * references to the value of x in variables or properties) just append to
 * x rather than make a copy and append to that.  If it *does* have to copy,
 * the next time (if it's in a loop) it will have the only reference to the
 * copy and then it can take advantage.
 *
 * NOTE WELL    NOTE WELL    NOTE WELL    NOTE WELL    NOTE WELL
 *
 * This option affects the length of certain bytecode sequences.
 * Suspended tasks in a database from a server built with this option
 * are not guaranteed to work with a server built without this option,
 * and vice versa.  It is safe to flip this switch only if there are
 * no suspended tasks in the database you are loading.  (It might work
 * anyway, but hey, it's your database.)  This restriction will be
 * lifted in a future version of the server software.  Consider this
 * option as being BETA QUALITY until then.
 *
 * NOTE WELL    NOTE WELL    NOTE WELL    NOTE WELL    NOTE WELL
 */
/* #define BYTECODE_REDUCE_REF */

#ifdef BYTECODE_REDUCE_REF
#error Think carefully before enabling BYTECODE_REDUCE_REF.  This feature is still beta.  Comment out this line if you are sure.
#endif

/******************************************************************************
 * The server can merge duplicate strings on load to conserve memory.  This
 * involves a rather expensive step at startup to dispose of the table used
 * to find the duplicates.  This should be improved eventually, but you may
 * want to trade off faster startup time for increased memory usage.
 *
 * You might want to turn this off if you see a large delay before the
 * INTERN: lines in the log at startup.
 */

#define STRING_INTERNING /* */

/******************************************************************************
 * Store the length of the string WITH the string rather than recomputing
 * it each time it is needed.
 */
/* #define MEMO_STRLEN */

/******************************************************************************
 * This package comes with a copy of the implementation of malloc() from GNU
 * Emacs.  This is a very nice and reasonably portable implementation, but some
 * systems, notably the NeXT machine, won't allow programs to provide their own
 * implementations of standard C functions.  Also, the GNU malloc()
 * implementation uses a fair amount more memory than some system-supplied
 * ones, though it does allow for better debugging support.  Define
 * USE_GNU_MALLOC to enable the use of the GNU malloc() implementation.
 ******************************************************************************
 * NOTE: This switch is now officially deprecated; it is recommended that you
 *	 avoid its use.  The GNU code is no longer as portable as once it might
 *	 have been, and it causes compilation errors on many systems.  If you
 *	 do enable it, and then experience compilation problems in the file
 *	 `gnu_malloc.c', your first step should be to re-disable its use.  This
 *	 option may be completely removed in a later release of the server.
 ******************************************************************************
 */

/* #define USE_GNU_MALLOC */

/*****************************************************************************
 ********** You shouldn't need to change anything below this point. **********
 *****************************************************************************/

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

#if NETWORK_STYLE != NS_BSD && NETWORK_STYLE != NS_SYSV
#  error Illegal value for "NETWORK_STYLE"
#endif

#if defined(MPLEX_STYLE) 	\
    && MPLEX_STYLE != MP_SELECT \
    && MPLEX_STYLE != MP_POLL \
    && MPLEX_STYLE != MP_FAKE
#  error Illegal value for "MPLEX_STYLE"
#endif

#endif				/* !Options_h */

/*
 * $Log$
 * Revision 1.16  2010/04/23 04:13:13  wrog
 * Define minima for .max_list_concat and .max_string_concat
 *
 * Revision 1.15  2010/04/22 21:44:57  wrog
 * Comment tweaks
 *
 * Revision 1.14  2010/03/29 21:59:49  wrog
 * Set better MAX_CONCAT_* limits; revised description
 *
 * Revision 1.13  2010/03/27 00:10:33  wrog
 * IGNORE_PROP_PROTECTED now off by default and deprecated;
 * Undo accidental uncommenting of OUTBOUND_NETWORK
 *
 * Revision 1.12  2010/03/26 23:51:42  wrog
 * New server options max_*_concat and max_concat_catchable
 *
 * Revision 1.11  2006/12/06 23:57:51  wrog
 * New INPUT_APPLY_BACKSPACE option to process backspace/delete characters on nonbinary connections (patch 1571939)
 *
 * Revision 1.10  2006/09/07 00:55:02  bjj
 * Add new MEMO_STRLEN option which uses the refcounting mechanism to
 * store strlen with strings.  This is basically free, since most string
 * allocations are rounded up by malloc anyway.  This saves lots of cycles
 * computing strlen.  (The change is originally from jitmoo, where I wanted
 * inline range checks for string ops).
 *
 * Revision 1.9  2004/05/22 01:25:44  wrog
 * merging in WROGUE changes (W_SRCIP, W_STARTUP, W_OOB)
 *
 * Revision 1.8.10.3  2004/05/21 00:02:59  wrog
 * allow for OUT_OF_BAND_QUOTE_PREFIX being undefined
 *
 * Revision 1.8.10.2  2003/06/11 10:36:45  wrog
 * added OUT_OF_BAND_QUOTE_PREFIX
 *
 * Revision 1.8.10.1  2003/06/01 12:42:30  wrog
 * added cmdline options -a (source address) +O/-O (enable/disable outbound network)
 *
 * Revision 1.8  2001/01/29 09:08:40  bjj
 * Made STRING_INTERNING optional via options.h.
 *
 * Revision 1.7  2000/01/11 02:05:27  nop
 * More doc tweaking, really warn about BYTECODE_REDUCE_REF.
 *
 * Revision 1.6  2000/01/09 22:20:15  nop
 * Round one of doc cleanup.
 *
 * Revision 1.5  1999/08/09 04:09:54  nop
 * Turn up the buffer sizes a notch.  They're still really too small...
 *
 * Revision 1.4  1998/12/14 13:18:41  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.3.2.1  1997/09/09 07:01:17  bjj
 * Change bytecode generation so that x=f(x) calls f() without holding a ref
 * to the value of x in the variable slot.  See the options.h comment for
 * BYTECODE_REDUCE_REF for more details.
 *
 * This checkin also makes x[y]=z (OP_INDEXSET) take advantage of that (that
 * new code is not conditional and still works either way).
 *
 * Revision 1.3  1997/03/03 06:14:45  nop
 * Nobody actually uses protected properties.  Make IGNORE_PROP_PROTECTED
 * the default.
 *
 * Revision 1.2  1997/03/03 04:19:13  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.5  1996/03/19  07:13:35  pavel
 * Changed CONNECT_TIMEOUT into DEFAULT_CONNECT_TIMEOUT.  Release 1.8.0p2.
 *
 * Revision 2.4  1996/03/10  01:12:55  pavel
 * Added definitions of DEFAULT_PORT and DEFAULT_CONNECT_FILE, moved here
 * from elsewhere.  Release 1.8.0.
 *
 * Revision 2.3  1996/02/08  06:15:51  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.2  1995/12/31  00:06:58  pavel
 * Clarified that OUTBOUND_NETWORK may not be defined for non-TCP
 * configurations.  Release 1.8.0alpha4.
 *
 * Revision 2.1  1995/12/11  08:07:02  pavel
 * Moved USE_GNU_MALLOC to a less prominent place in the file.
 * Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  04:53:59  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.11  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.10  1992/10/23  22:21:10  pavel
 * Fixed MPLEX_STYLE-defaulting code to avoid assuming that select() always
 * works on FIFOs by conditionalizing on SELECT_WORKS_ON_FIFOS rather than
 * HAVE_SELECT.
 *
 * Revision 1.9  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.8  1992/10/17  20:20:16  pavel
 * Generalize treatment of system-dependent replacements for rand().
 * Allow for systems that don't have rename() and/or remove().
 *
 * Revision 1.7  1992/10/06  18:23:07  pavel
 * Dyked out useless XNS support and fixed a bug in the default definition of
 * MPLEX_STYLE.
 *
 * Revision 1.6  1992/09/23  17:16:25  pavel
 * Added definitions of NETWORK_PROTOCOL etc. to support new networking
 * architecture.
 *
 * Revision 1.5  1992/09/11  21:17:54  pavel
 * Reset to refrain from defining SINGLE_USER in the distribution.
 *
 * Revision 1.4  1992/09/03  23:52:45  pavel
 * Added support for multiple complete network implementations.
 *
 * Revision 1.3  1992/08/21  01:01:22  pavel
 * Radically reorganized and verbosely commented to make it easy for people to
 * understand what all of the configuration options mean.
 *
 * Moved all of the -DFOO options out of the Makefile and into here.
 *
 * Revision 1.2  1992/08/20  17:33:54  pavel
 * Added #define for OUT_OF_BAND_PREFIX.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
