# --------------------------------------------------------------------
# MOO_ADD_CFLAGS(-OPTION,[-OPTION2,...])
#
#  Add -OPTION to CFLAGS, but only if compilation does not break
#  If multiple -OPTIONs are given, adds the first one that works.
#  Sets cache variable(s)  moo_cc_cv_grok_OPTION.
#
AC_DEFUN([MOO_ADD_CFLAGS],
[m4_if([$2],[],
[_MOO_ADD_CFLAG([$1],[nobreak])],
[[while : ; do]m4_map_args([
  _MOO_ADD_CFLAG],$@)[
  break
done]])])dnl
#
#  _MOO_ADD_CFLAG(-OPTION,[nobreak])
#    does the actual work;
#    adds a 'break' if successful unless 'nobreak' is specified
#
#  (the mechanics are annoying because clang warns but otherwise
#   happily continues with -Wfoos that it doesn't recognize,
#   so we have to set -Werror to get it to fail.  But this
#   causes -Wstrict-prototypes to make 'int main() {...}' fail,
#   so we cannot use AC_LANG_PROGRAM, and we also have to deal
#   with -Wunused (enabled by -Wall and -Wextra (nee -W))
#   killing us unless argc and argv are given something to do).
#
m4_define([_MOO_ADD_CFLAG],
[AS_VAR_PUSHDEF([_moo_Flag], [moo_cc_cv_grok$1])dnl
AC_CACHE_CHECK([if $CC accepts $1],m4_dquote(_moo_Flag),[[
 _moo_save_cflags="$CFLAGS"
 CFLAGS="$CFLAGS -Werror $1"]
 AC_LINK_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
int
main (int argc, char **argv)
{
  printf("Hello %d, World %s!", argc, argv[0]);
  return 0;
}
]])],[AS_VAR_SET(m4_dquote(_moo_Flag),[yes])],[AS_VAR_SET(m4_dquote(_moo_Flag),[no])])[
 CFLAGS="$_moo_save_cflags"
]])
AS_VAR_IF(m4_dquote(_moo_Flag),[yes],
[[CFLAGS="$CFLAGS $1"]m4_if([$2],[],[[
  break]],[])])
AS_VAR_POPDEF([_moo_Flag])])
dnl
