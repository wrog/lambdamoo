dnl -*- autoconf -*-
#
#  various tweaks for various ancient platforms.
#
# ***************************************************************************
#	MOO_AUX
#  Defines _POSIX_SOURCE if this machine is running A/UX; this appears to be
#  necessary in order to get declarations of POSIX functions.
AC_DEFUN([MOO_AUX],
  [AC_CACHE_CHECK([for A/UX],[moo_sys_cv_a_ux],
     [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if !defined(AUX)
#emote not seeing aux
#endif
]])],[moo_sys_cv_a_ux=yes],[moo_sys_cv_a_ux=no])])[
  test "$moo_sys_cv_a_ux" = yes &&]
    AC_DEFINE([_POSIX_SOURCE],[1])
])dnl
dnl
# ***************************************************************************
#	MOO_HPUX
#  Defines _HPUX_SOURCE if this machine is running HP/UX; this appears to be
#  necessary in order to get declarations of POSIX functions.
AC_DEFUN([MOO_HPUX],
 [AC_CACHE_CHECK([for HP/UX],[moo_sys_cv_hp_ux],
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if !defined(__hpux)
#emote not seeing hpux
#endif
]])],[moo_sys_cv_hp_ux=yes],[moo_sys_cv_hp_ux=no])])[
  test "$moo_sys_cv_hp_ux" = yes &&]
    AC_DEFINE([_HPUX_SOURCE],[1])
])dnl
dnl
dnl
# ***************************************************************************
#	MOO_ALPHA
#  If this machine is a DEC Alpha running OSF/1, and we're not using GCC, then
#  add `-Olimit 1000' to the `cc' switches, to allow the compiler to run the
#  optimizer over large functions, like the main MOO interpreter loop.
AC_DEFUN([MOO_ALPHA],
[AC_CACHE_CHECK([for the DEC Alpha running OSF/1],[moo_sys_cv_dec_alpha],
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if !(defined(__osf__) && defined(__alpha) && !defined(__GNUC__))
#emote not an alpha
#endif
]])],[moo_sys_cv_dec_alpha=yes],[moo_sys_cv_dec_alpha=no])])[
test "$moo_sys_cv_dec_alpha" = yes && CFLAGS="$CFLAGS -Olimit 2500"
]])dnl
dnl
# ***************************************************************************
#	MOO_SGI
#  If this machine is an SGI running IRIX, and we're not using GCC, then
#  add `-Olimit 2500' to the `cc' switches, to allow the compiler to run the
#  optimizer over large functions, like the main MOO interpreter loop.  Also,
#  even if we are using GCC, undefine __EXTENSIONS__ to keep from seeing a
#  bunch of interfering declarations.
AC_DEFUN([MOO_SGI],
  [AC_CACHE_CHECK([for the SGI compiler],[moo_sys_cv_sgi],
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if !(defined(sgi) && !defined(__GNUC__))
#emote not an sgi or running gnucc
#endif
]])], [[moo_sys_cv_sgi=yes]],
      [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if !defined(sgi)
#emote not an sgi
#endif
]])],  [[moo_sys_cv_sgi=gcc]],  [[moo_sys_cv_sgi=no]])])])[
  test "$moo_sys_cv_sgi" = yes && CFLAGS="$CFLAGS -Olimit 2500"
  test "$moo_sys_cv_sgi" = no || CFLAGS="$CFLAGS -U__EXTENSIONS__"
]])dnl
dnl
# ***************************************************************************
#	MOO_NEXT
#  On NeXT, we need to make sure that _NEXT_SOURCE and _POSIX_SOURCE are
#  defined.
AC_DEFUN([MOO_NEXT],
  [AC_CACHE_CHECK([for NeXT],[moo_sys_cv_next],
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#ifndef NeXT
#emote not a next
#endif
]])],[[moo_sys_cv_next=yes]],[[moo_sys_cv_next=no]])])[
  if test "x$moo_sys_cv_next" = xyes ; then]
    AC_DEFINE([_NEXT_SOURCE])
    AC_DEFINE([_POSIX_SOURCE])[
fi]])dnl
dnl
