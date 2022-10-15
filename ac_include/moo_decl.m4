dnl -*- autoconf -*-

dnl ******************************************************************
dnl 	MOO_NDECL_FUNCS(header, func1 func2 ...[, extra-hdr])
dnl Defines NDECL_func1, NDECL_func2, ... if not declared in header.
dnl Sets ac_cv_have_decl_{func1,func2,...} if declared
dnl *** TODO: use AC_CHECK_DECLS but that entails changing
dnl ***   #ifdef NDECL_func to #if !HAVE_DECL_func everywhere
dnl *** ALSO if you use this twice for the same function (cf. bzero)
dnl ***   you need to reset ac_cv_have_decl_FUNC
dnl ***   otherwise the 2nd check does not happen.   --wrog
m4_define([_MOO_CHECK_NDECL],
[AC_CHECK_DECL([$1], [],
 [AC_DEFINE_UNQUOTED(AS_TR_CPP([NDECL_][$1]))], [$2])
])dnl -- this NL apparently matters in 2.71
dnl
AC_DEFUN([MOO_NDECL_FUNCS],
[m4_map_args_w([$2], [_MOO_CHECK_NDECL(], [,
[[@%:@include <$1>]
$3])],[
])])dnl
dnl
dnl ***************************************************************************
dnl 	MOO_NDECL_VARS(header, var1 var2 ...[, extra-hdr])
dnl Defines NDECL_var1, NDECL_var2, ... if they are not declared in header.
dnl Sets ac_cv_have_decl_{var1,var2,...} if declared
dnl (since AC_CHECK_DECL only checks that the name can be used as an
dnl  rvalue without causing link errors and so works equally well for
dnl  vars and funcs, so this is now identical to MOO_NDECL_FUNCS)
AC_DEFUN([MOO_NDECL_VARS],m4_defn([MOO_NDECL_FUNCS]))dnl


dnl ***************************************************************************
dnl    MOO_WHERE_THE_HELL_IS_BZERO
dnl It seems bzero() can be just about anywhere.  If we already know
dnl it will be in the cache (ac_cv_have_decl_bzero), otherwise search
dnl 'yes' is for the usual place (string.h) and otherwise we
dnl cdefine NDECL_BZERO and whichever BZERO_IN_<WHATEVER> applies
dnl
AC_DEFUN([MOO_WHERE_THE_HELL_IS_BZERO],
[AS_IF([[test x"$ac_cv_have_decl_bzero" = x]],
  [_MOO_WHERE_THE_HELL_IS_BZERO(
    [[yes]], [[@%:@if NEED_MEMORY_H
@%:@include <memory.h>
@%:@endif
@%:@include <string.h>]],
    [[strings]],[[@%:@include <strings.h>]],
    [[stdlib]], [[@%:@include <stdlib.h>]])])
AS_IF([[test "$ac_cv_have_decl_bzero" != yes]],
  [AC_DEFINE([NDECL_BZERO])
AS_CASE([[$ac_cv_have_decl_bzero]],[[
  strings]],[AC_DEFINE([BZERO_IN_STRINGS_H])],[[
  stdlib]], [AC_DEFINE([BZERO_IN_STDLIB_H])])])])

m4_define([_MOO_WHERE_THE_HELL_IS_BZERO],
[m4_ifval([$2],[AS_UNSET([ac_cv_have_decl_bzero])
AC_CHECK_DECL([bzero],
  [[ac_cv_have_decl_bzero=]$1],
  [$0(m4_shift2($@))], [$2])],
[[ac_cv_have_decl_bzero=no]])])


dnl ***************************************************************************
dnl 	MOO_HEADER_STANDS_ALONE(header [, extra-code])
dnl Sets moo_cc_cv_hso_HEADER to yes or no accordingly.
dnl Defines header_NEEDS_HELP if can't be compiled all by itself.
dnl
AC_DEFUN([MOO_HEADER_STANDS_ALONE],
[AS_VAR_PUSHDEF([_moo_Flag], [moo_cc_cv_hso_$1])dnl
AC_CACHE_CHECK([for self-sufficiency of $1], m4_dquote(_moo_Flag),
 	       [AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <$1>
$2
]],[[]])],[AS_VAR_SET(m4_dquote(_moo_Flag),[yes])],[AS_VAR_SET(m4_dquote(_moo_Flag),[no])])])
AS_VAR_IF(m4_dquote(_moo_Flag),[no],[AC_DEFINE_UNQUOTED(AS_TR_CPP([$1][_NEEDS_HELP]))])
AS_VAR_POPDEF([_moo_Flag])])dnl

dnl ***************************************************************************
dnl	MOO_HAVE_FUNC_LIBS(func1 func2 ..., lib1 "lib2a lib2b" lib3 ...)
dnl For each `func' in turn, if `func' is defined using the current LIBS value,
dnl leave LIBS alone.  Otherwise, try adding each of the given libs to LIBS in
dnl turn, stopping when one of them succeeds in providing `func'.  Define
dnl HAVE_func if `func' is eventually found.
dnl
m4_define([_MOO_HAVE_FUNC_LIBS],
 [AC_SEARCH_LIBS([$1], [$2],
   [AC_DEFINE_UNQUOTED(AS_TR_CPP([HAVE_$1]),[1])], [], [$3])])dnl

AC_DEFUN([MOO_HAVE_FUNC_LIBS],
 [m4_map_args_w([$1], [_$0(], [, [$2], [$3])])])dnl
