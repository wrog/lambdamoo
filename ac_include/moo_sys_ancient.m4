dnl ***************************************************************************
dnl	MOO_AUX
dnl Defines _POSIX_SOURCE if this machine is running A/UX; this appears to be
dnl necessary in order to get declarations of POSIX functions.
AC_DEFUN([MOO_AUX], [
echo checking for A/UX
AC_PROGRAM_EGREP(yes, [
#if defined(AUX)
  yes
#endif
], AC_DEFINE(_POSIX_SOURCE))])

dnl ***************************************************************************
dnl	MOO_HPUX
dnl Defines _HPUX_SOURCE if this machine is running HP/UX; this appears to be
dnl necessary in order to get declarations of POSIX functions.
AC_DEFUN([MOO_HPUX], [
echo checking for HP/UX
AC_PROGRAM_EGREP(yes, [
#if defined(__hpux)
  yes
#endif
], AC_DEFINE(_HPUX_SOURCE))])

dnl ***************************************************************************
dnl	MOO_ALPHA
dnl If this machine is a DEC Alpha running OSF/1, and we're not using GCC, then
dnl add `-Olimit 1000' to the `cc' switches, to allow the compiler to run the
dnl optimizer over large functions, like the main MOO interpreter loop.
AC_DEFUN([MOO_ALPHA], [
echo checking for the DEC Alpha running OSF/1
AC_PROGRAM_EGREP(yes, [
#if defined(__osf__) && defined(__alpha) && !defined(__GNUC__)
  yes
#endif
], CC="$CC -Olimit 2500")])

dnl ***************************************************************************
dnl	MOO_SGI
dnl If this machine is an SGI running IRIX, and we're not using GCC, then
dnl add `-Olimit 2500' to the `cc' switches, to allow the compiler to run the
dnl optimizer over large functions, like the main MOO interpreter loop.  Also,
dnl even if we are using GCC, undefine __EXTENSIONS__ to keep from seeing a
dnl bunch of interfering declarations.
AC_DEFUN([MOO_SGI], [
echo checking for the SGI compiler
AC_PROGRAM_EGREP(yes, [
#if defined(sgi) && !defined(__GNUC__)
  yes
#endif
], CC="$CC -Olimit 2500")
AC_PROGRAM_EGREP(yes, [
#if defined(sgi)
  yes
#endif
], CC="$CC -U__EXTENSIONS__")])

dnl ***************************************************************************
dnl	MOO_NEXT
dnl On NeXT, we need to make sure that _NEXT_SOURCE and _POSIX_SOURCE are
dnl defined.
AC_DEFUN([MOO_NEXT], [
echo checking for NeXT
AC_PROGRAM_EGREP(yes, [
#ifdef NeXT
  yes
#endif
], AC_DEFINE(_NEXT_SOURCE)
   AC_DEFINE(_POSIX_SOURCE))])
