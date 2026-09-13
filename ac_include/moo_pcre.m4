# MOO_LIB_PCRE_XT( <LIBNAME>, <IF_FOUND>, <FAIL_VAR> )
#   for use in extensions.ac
#   <LIBNAME> is one of 'pcre' or 'pcre2'
#   <IF_FOUND> is expanded if library is found and usable
#   <FAIL_VAR> gets assigned otherwise
#
AC_DEFUN([MOO_LIB_PCRE_XT], [moo_pcre_subst([$1],
  [m4_ifdef([_moo_$1_ldname], [],
    [m4_fatal([$0: unknown PCRE library '$1'])])
    AC_CHECK_LIB([%ldname%], [%entryfn%],
      [_moo_fail=:
      AC_CHECK_HEADERS([$1.h pcre/$1.h],
        [_moo_fail=false ; break],,[%preinclude%])
      AS_IF([[$_moo_fail]],
[       $3[="header file $1.h not found."]],
        [AC_CACHE_CHECK(
          [for [-l%ldname% required features]],
          [moo_lib_cv_$1_unicode],
          [[_moo_save_libs=$LIBS
          LIBS="-l%ldname% $LIBS"]
          AC_RUN_IFELSE([AC_LANG_PROGRAM(
            [%preinclude%][[
@%%:@include <$ac_header>]], [%body%])],
            [[moo_lib_cv_$1_unicode=yes]],
            [[moo_lib_cv_$1_unicode=no]],
            [[moo_lib_cv_$1_unicode="guessing yes"]])[
          LIBS=$_moo_save_libs]])
        AS_VAR_IF([[moo_lib_cv_$1_unicode]], [[no]],
[         $3[="-l%ldname% lacks required Unicode support%minv%."]],
dnl     else
[[        LIBS="-l%ldname% $LIBS"]
          $2])])],

      [[moo_lib_cv_$1_unicode=no]
      $3[="-l%ldname% library not found;
   use 'apt-get install %debian%' in Debian or Ubuntu;
   Fedora, Red Hat, MacOS homebrew all have '$1' packages."]])])])


m4_define([moo_pcre_subst],
  [AC_REQUIRE([AX_SUBST_PP_INIT])dnl
ax_subst_pp([$2],[m4_defn([_moo_$1_]],[)])])

m4_define([_moo_pcre_ldname],  [pcre])
m4_define([_moo_pcre_entryfn], [pcre_exec])
m4_define([_moo_pcre_debian],  [libpcre3-dev])
m4_define([_moo_pcre_minv],    [,
   which was not available before version 8])
m4_define([_moo_pcre_preinclude], [])

m4_define([_moo_pcre_body], [[
  int have_utf8, have_props;
  pcre_config(PCRE_CONFIG_UTF8, &have_utf8);
  pcre_config(PCRE_CONFIG_UNICODE_PROPERTIES, &have_props);
  return (have_utf8 && have_props) ? 0 : 1;
]])

m4_define([_moo_pcre2_ldname],  [pcre2-8])
m4_define([_moo_pcre2_entryfn], [pcre2_match_8])
m4_define([_moo_pcre2_debian],  [libpcre2-dev])
m4_define([_moo_pcre2_minv],    [])
m4_define([_moo_pcre2_preinclude], [[
@%:@define PCRE2_CODE_UNIT_WIDTH 8]])

m4_define([_moo_pcre2_body], [[
  uint32_t have_unicode;
  pcre2_config(PCRE2_CONFIG_UNICODE, &have_unicode);
  return have_unicode ? 0 : 1;
]])

# how to make %%->%
m4_define([_moo_pcre_],        [%])
m4_define([_moo_pcre2_],       [%])
