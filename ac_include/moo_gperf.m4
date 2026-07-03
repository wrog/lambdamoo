
# ------------------------------------------------------------------------
# MOO_PROG_GPERF
#
# checks for gperf.
# Result is cached in the ‘ac_cv_prog_GPERF’ shell variable.
#
AC_DEFUN([MOO_PROG_GPERF],
[AC_CHECK_PROGS([GPERF], [gperf])
AC_ARG_VAR([GPERF],
[The perfect hash function generator to use.  Defaults to 'gperf'.])])


# --------------------------------------------------------------------
# MOO_TYPE_GPERF_KWDSIZE_T
#
# Define gperf_kwdsize_t to be a suitable type for gperf's
# lookup-function keyword size parameter (this changed between 3.0 and 3.1
# and Apple refuses to upgrade even though 3.0 is 10+ years old).
# Result is cached in the ‘moo_cv_gperf_kwdsize_t’ shell variable.
#
AC_DEFUN([MOO_TYPE_GPERF_KWDSIZE_T],
[AC_REQUIRE([MOO_PROG_GPERF])
AC_CACHE_CHECK([[for gperf_kwdsize_t]], [[moo_cv_gperf_kwdsize_t]], [[
    moo_cv_gperf_kwdsize_t=no
    for _moo_type in size_t 'unsigned int'; do]
      AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([[
@%:@include <string.h>
const char *in_word_set(const char *, $_moo_type);
`echo foo,bar | $GPERF --language=ANSI-C`]])], [[
        moo_cv_gperf_kwdsize_t=$_moo_type
        break]])[
    done]])
AS_VAR_IF([[moo_cv_gperf_kwdsize_t]],[[no]],
  [AC_MSG_ERROR([[unable to determine gperf_kwdsize_t]])],
  [AC_DEFINE_UNQUOTED([[gperf_kwdsize_t]],
    [[$moo_cv_gperf_kwdsize_t]])])])
