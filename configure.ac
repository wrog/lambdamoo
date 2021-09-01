dnl Process this file with `autoconf' to create `configure'.
dnl
dnl Copyright (c) 1992, 1995, 1996 Xerox Corporation.  All rights reserved.
dnl Portions of this code were written by Stephen White, aka ghond.
dnl Use and copying of this software and preparation of derivative works based
dnl upon this software are permitted.  Any distribution of this software or
dnl derivative works must comply with all applicable United States export
dnl control laws.  This software is made available AS IS, and Xerox Corporation
dnl makes no warranty about the software, its performance or its conformity to
dnl any specification.  Any person obtaining a copy of this software is
dnl requested to send their name and post office or electronic mail address to:
dnl   Pavel Curtis
dnl   Xerox PARC
dnl   3333 Coyote Hill Rd.
dnl   Palo Alto, CA 94304
dnl   Pavel@Xerox.Com
dnl
AC_INIT
[if test "x$srcdir" != "x." && test -f "$srcdir/config.status" ; then]
  AC_MSG_ERROR([[clean up the source directory (make distclean) before attempting a VPATH build]])[
fi]

AC_CONFIG_MACRO_DIR([ac_include])
AC_CONFIG_SRCDIR([Minimal.db])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

dnl ***************************************************************************
dnl Start of actual configuration tests
dnl ***************************************************************************

AC_PROG_MAKE_SET
AC_PROG_YACC
AC_CHECK_PROGS([GPERF], [gperf])
[test "x$ac_cv_prog_GPERF" = x &&] AC_MSG_ERROR([[We need gperf]])dnl
AC_ARG_VAR([GPERF],
[The perfect hash function generator to use.  Defaults to 'gperf'.])

AC_PROG_CC
AC_PROG_GCC_TRADITIONAL
AC_USE_SYSTEM_EXTENSIONS
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if __GNUC__<2
#emote cant use -Wall
#endif
]])], [CC="$CC -Wall -Wwrite-strings"], [])
test -n "$GCC" && CC="$CC -g"
AC_PROG_CPP
MOO_NEXT
MOO_ALPHA
MOO_SGI
AC_SEARCH_LIBS([strerror],[cposix])
AC_FUNC_STRFTIME
MOO_AUX
MOO_HPUX
MOO_ANSI_C
MOO_ADD_CFLAGS([-Wall])
MOO_ADD_CFLAGS([-W])
MOO_ADD_CFLAGS([-Wwrite-strings])
MOO_ADD_CFLAGS([-Wpointer-arith])
MOO_ADD_CFLAGS([-Wbad-function-cast])
MOO_ADD_CFLAGS([-Wcast-equal])
MOO_ADD_CFLAGS([-Wstrict-prototypes])
MOO_ADD_CFLAGS([-Wmissing-prototypes])
MOO_ADD_CFLAGS([-Wmissing-declarations])
MOO_ADD_CFLAGS([-Wnested-externs])
MOO_ADD_CFLAGS([-Winline])
MOO_ADD_CFLAGS([-Wwrite-strings])
dnl MOO_ADD_CFLAGS([-Wundef])
dnl MOO_ADD_CFLAGS([-Wshadow])
dnl MOO_ADD_CFLAGS([-Wsign-compare])
MOO_ADD_CFLAGS([-pipe])
MOO_ADD_CFLAGS([-O3])
AC_C_CONST
AC_C_INLINE
AC_C_RESTRICT
AC_SYS_LARGEFILE
AC_CHECK_HEADER([malloc.h],[AC_DEFINE([NEED_MALLOC_H])])
AC_CHECK_HEADER([sys/bsdtypes.h],[AC_DEFINE([NEED_BSDTYPES_H])])
AC_CHECK_HEADER([sys/select.h],[AC_DEFINE([NEED_SELECT_H])])
AC_CHECK_HEADER([memory.h],[AC_DEFINE([NEED_MEMORY_H])])
AC_CHECK_HEADER([inttypes.h],[AC_DEFINE([NEED_INTTYPES_H])])
AC_CHECK_HEADER([stdint.h],[AC_DEFINE([NEED_STDINT_H])])
AC_TYPE_PID_T
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_STRUCT_TIMEZONE
MOO_HAVE_FUNC_LIBS([atan2 asinh expm1 erfc j1], [m])
dnl *** was -lm /lib/libm.a "-static -lm" (is this still needed?)
MOO_HAVE_FUNC_LIBS([mkfifo waitpid sigemptyset], [posix])
dnl *** was -lposix /lib/libposix.a (is this still needed?)
MOO_HAVE_FUNC_LIBS([accept], [socket inet], [nsl])
dnl *** was "-lsocket -lnsl" -lsocket -linet
dnl *** (does nsl screw up non-sysV builds?)
MOO_HAVE_FUNC_LIBS([t_open], [nsl nsl_s])
MOO_HAVE_FUNC_LIBS([crypt], [crypt crypt_d])
AC_CHECK_HEADERS([unistd.h sys/cdefs.h stdlib.h tiuser.h machine/endian.h])
AC_CHECK_FUNCS([remove rename poll select strerror strftime strtoul matherr])
AC_CHECK_FUNCS([random lrand48 wait3 wait2 sigsetmask sigprocmask sigrelse])
AC_CHECK_FUNCS([strtoimax])
MOO_NDECL_FUNCS([ctype.h], [tolower])
MOO_NDECL_FUNCS([fcntl.h], [fcntl])
MOO_NDECL_FUNCS([netinet/in.h], [htonl])
AC_CHECK_TYPES([long long])
AC_CHECK_TYPES([uint8_t])
AC_CHECK_TYPES([int32_t])
AC_CHECK_TYPES([int64_t])
AC_CHECK_TYPES([int128_t])
AC_CHECK_TYPES([__int128])
AC_CHECK_TYPE([in_addr_t], [], [AC_DEFINE([NDECL_IN_ADDR_T])],
  [[@%:@include <netinet/in.h>]])
MOO_NDECL_FUNCS([sys/ioctl.h], [ioctl])
MOO_NDECL_FUNCS([poll.h],      [poll])
MOO_NDECL_FUNCS([signal.h],    [kill sigemptyset sigprocmask sigrelse])
MOO_NDECL_FUNCS([sys/socket.h],[accept bind shutdown])
MOO_NDECL_FUNCS([sys/stat.h],  [fstat])
MOO_NDECL_FUNCS([stdio.h],  [fclose perror remove vfprintf])
MOO_NDECL_FUNCS([stdlib.h], [random srandom strtod strtol strtoul])
MOO_NDECL_FUNCS([string.h], [memcpy memset strerror], [[
#if NEED_MEMORY_H
#include <memory.h>
@%:@endif]])
MOO_WHERE_THE_HELL_IS_BZERO()
MOO_NDECL_FUNCS([sys/time.h], [getitimer setitimer select], [[
#if NEED_SELECT_H
#include <sys/types.h>
#include <sys/select.h>
@%:@endif]])
MOO_NDECL_FUNCS([time.h], [strftime time])
MOO_NDECL_VARS([time.h], [tzname])
MOO_NDECL_FUNCS([tiuser.h], [t_open])
MOO_NDECL_VARS([tiuser.h], [t_errlist])
MOO_NDECL_FUNCS([unistd.h], [fork])
MOO_NDECL_FUNCS([sys/wait.h], [waitpid])

MOO_HEADER_STANDS_ALONE([arpa/inet.h], [struct in_addr foo;])
MOO_HEADER_STANDS_ALONE([signal.h])
MOO_HEADER_STANDS_ALONE([sys/socket.h])
MOO_HEADER_STANDS_ALONE([sys/stat.h])
MOO_HEADER_STANDS_ALONE([time.h])

dnl ***************************************************************************
AC_MSG_CHECKING([[for string.h declaring some functions incorrectly]])[
cat >conftest.c <<'_MOOEOF'
#include <sys/types.h>
#include <string.h>
#if NEED_MEMORY_H
#include <memory.h>
#endif
_MOOEOF
$CC $DEFS conftest.c -o conftest $LIBS > conftest.out 2>&1
if grep "conflicting types for built-in function" conftest.out >/dev/null; then]
  AC_MSG_RESULT([yes])
  AC_DEFINE(USE_OWN_STRING_H)[
else]
  AC_MSG_RESULT([no])[
fi
rm -f conftest*]

dnl ***************************************************************************
AC_MSG_CHECKING([[for incompatibility between <sys/ioctl.h> and <stropts.h>]])
AC_PREPROC_IFELSE([AC_LANG_SOURCE([[
#include <sys/ioctl.h>
#include <stropts.h>
]])],[AC_MSG_RESULT([no])],[AC_MSG_RESULT([yes])
AC_DEFINE([UNDEF_IO_IN_STROPTS_H],[1])])

MOO_NET_FIFO_WORKS
MOO_NET_POSIX_NONBLOCKING
dnl ***************************************************************************
AC_MSG_CHECKING([which MOO networking configurations are likely to work])

[NETWORK_CONFIGURATIONS="NP_SINGLE"]
m4_define(MOO_ADD_NET_CONFIG,
  [[NETWORK_CONFIGURATIONS="$NETWORK_CONFIGURATIONS $1"]])dnl

AC_CHECK_HEADER([sys/socket.h],[
	MOO_ADD_NET_CONFIG(NS_BSD/NP_LOCAL)
	AC_CHECK_PROG([have_telnet],[telnet],[yes],[no])
        test x$have_telnet = xyes && MOO_ADD_NET_CONFIG(NS_BSD/NP_TCP)])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/stat.h>
@%:@if !(defined(S_IFIFO) && !defined(NeXT) && \
      (SELECT_WORKS_ON_FIFOS || POLL_WORKS_ON_FIFOS || FSTAT_WORKS_ON_FIFOS))
#emote no FIFOs
#endif
]])], [MOO_ADD_NET_CONFIG(NS_SYSV/NP_LOCAL)], [])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if !(HAVE_TIUSER_H && HAVE_T_OPEN && HAVE_POLL)
#emote no user+open+poll
#endif
]])],[test -r /dev/tcp && MOO_ADD_NET_CONFIG(NS_SYSV/NP_TCP)], [])
AC_MSG_RESULT([done])

dnl ***************************************************************************
dnl Find a usable PCRE library
AC_CHECK_LIB([pcre], [pcre_exec], [], [AC_MSG_ERROR([[missing -lpcre
*** The PCRE library cannot be found. (must be PCRE1, not PCRE2)
***   ('apt-get install libpcre3-dev' in Debian or Ubuntu)
***   (Fedora, Red Hat, MacOS homebrew all have 'pcre' packages)]])])
AC_CACHE_CHECK([[whether -lpcre was built with required features]],
  [moo_lib_cv_pcre_utf8],
[AC_RUN_IFELSE([AC_LANG_PROGRAM([[@%:@ include <pcre.h>]],[[
  int have_utf8, have_props;
  pcre_config(PCRE_CONFIG_UTF8, &have_utf8);
  pcre_config(PCRE_CONFIG_UNICODE_PROPERTIES, &have_props);
  return (have_utf8 && have_props) ? 0 : 1;
]])], [[moo_lib_cv_pcre_utf8=yes]], [[moo_lib_cv_pcre_utf8=no]])])[
if test "x$moo_lib_cv_pcre_utf8" = xno ; then]
  AC_MSG_ERROR([[-lpcre cannot be used
*** The installed PCRE library is missing either support for UTF-8 or support
*** for Unicode character properties.  We need at least version 8.]])[
fi]

AC_OUTPUT
[
cat <<_MOO_EOF
----------------------------------------------------------------------
| The following networking configurations will probably work on your
| system; any configuration *not* listed here almost certainly will
| *not* work on your system:
|
|   $NETWORK_CONFIGURATIONS
----------------------------------------------------------------------
_MOO_EOF
]

# $Log$
# Revision 1.2  2004/05/22 01:25:43  wrog
# merging in WROGUE changes (W_SRCIP, W_STARTUP, W_OOB)
#
# Revision 1.1.1.1.12.1  2003/06/01 12:39:27  wrog
# ensure definitions for in_addr_t, INADDR_NONE
#
# Revision 1.1.1.1  1997/03/03 03:45:05  nop
# LambdaMOO 1.8.0p5
#
# Revision 2.8  1996/03/19  07:16:46  pavel
# Added one more option for trying to enable ANSI C compilation.
# Release 1.8.0p2.
#
# Revision 2.7  1996/03/10  01:21:41  pavel
# Increased DEC Alpha -Olimit to 2500 from 1000.  Release 1.8.0.
#
# Revision 2.6  1996/02/11  00:43:22  pavel
# Added check for matherr().  Release 1.8.0beta2.
#
# Revision 2.5  1996/02/08  07:22:30  pavel
# Added checks for SGI machines and for strtod() being undeclared.  Updated
# copyright notice for 1996.  Release 1.8.0beta1.
#
# Revision 2.4  1996/01/11  07:47:54  pavel
# Added a handy form-feed between the macro definitions and the main script,
# to make it easier to navigate with Emacs.  Release 1.8.0alpha5.
#
# Revision 2.3  1995/12/31  03:17:07  pavel
# Added test for <sys/stat.h> needing help.  Release 1.8.0alpha4.
#
# Revision 2.2  1995/12/28  00:49:41  pavel
# On NeXT, we now define _POSIX_SOURCE too.  Release 1.8.0alpha3.
#
# Revision 2.1  1995/12/11  08:05:45  pavel
# Added support for finding `crypt()' in `-lcrypt'.  Release 1.8.0alpha2.
#
# Revision 2.0  1995/11/30  05:09:41  pavel
# New baseline version, corresponding to release 1.8.0alpha1.
#
# Revision 1.5  1992/10/28  01:56:41  pavel
# Fixed NDECL test to look for `vfprintf' instead of the unused `vprintf'...
#
# Revision 1.4  1992/10/23  23:13:18  pavel
# Added copyright notice.
#
# Revision 1.3  1992/10/23  19:23:52  pavel
# Added MOO_AUX, MOO_MALLOC_H, MOO_FUNC_DECL_CHECK.
# Generalized MOO_HAVE_FUNC_LIBS to take multiple function names.
# Refined MOO_CONST to check for warnings about `reserved' keywords.
# Added MOO_NDECL_FUNCS checks for sigemptyset, perror, remove, vprintf,
# bzero, memset, fork, fstat, and waitpid.
# Simplified test for conflicting built-in function declarations.
# Added SELECT_WORKS_ON_FIFOS and POSIX_NONBLOCKING_WORKS tests.
#
# Revision 1.2  1992/10/21  03:09:41  pavel
# Added this log.
