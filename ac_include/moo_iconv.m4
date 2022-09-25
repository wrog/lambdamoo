# ************************************************* -*- autoconf -*-
#    MOO_ICONV_LIBS
#  sometimes -liconv is needed, sometimes not.
#  Adjusts LIBS as needed.
#  Sets moo_cv_iconv_lib to one of 'none needed', '-liconv', 'fail'
#
AC_DEFUN([MOO_ICONV_LIBS],
[AC_CACHE_CHECK([[for iconv needing additional flags]],
		[moo_cv_iconv_lib],[[
  _moo_save_libs="$LIBS"
  moo_cv_iconv_lib=
  for _moo_lib in '' -liconv ; do
    LIBS="$_moo_lib $_moo_save_libs"]
    AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[@%:@include <iconv.h>]], [[(void)iconv_open("eat","me");]])],[[
      moo_cv_iconv_lib="$_moo_lib"
      test "x$_moo_lib" = x && moo_cv_iconv_lib="none needed"
      break
    ]])[
  done
  test "x$moo_cv_iconv_lib" = x && moo_cv_iconv_lib=fail
  LIBS="$_moo_save_libs"
]])[
test "x$moo_cv_iconv_lib" = "x-liconv" &&
  LIBS="$moo_cv_iconv_lib $LIBS"
]])dnl
