dnl  -*- autoconf -*-
dnl

# --------------------------------------------------------------------
#  MOO_DECLARE_OPTIONS(
#    [NAME, TYPE, DEFAULT-VALUE, CONFIGURE-HELP],...)
#
#    declares a sequence of options.h-#defined identifiers
#    that ./configure can affect
#    via --(dis|en)able-def-NAME[=value]
#
#  NAME is of the C #define macro appearing in options.h
#  TYPE is one of 'bool','int','str',or a space separated
#    list of enumeration identifiers 'WORD1 WORD2 ...'
#  DEFAULT-VALUE is the verbatim value to appear in the
#    #define directive *unless* it is
#    'yes' (tranlates to '1') or
#    'no'  (translates to #undef)
#  CONFIGURE-HELP is a short description to appear next to
#    --enable-def-NAME in the ./configure --help message
#
#  Each invocation of this macro adds to the existing list.
#
AC_DEFUN([MOO_DECLARE_OPTIONS],
[m4_ifdef([_MOO_OPTIONS_frozen],
 [AC_MSG_ERROR([all instances of $0 must be before anything that references the options list])])dnl
m4_define([_MOO_OPTIONS_RAW],
 m4_dquote(_MOO_OPTIONS_RAW,$@))])

# the actual list
m4_define([_MOO_OPTIONS_RAW],[])

# -------------------------------------------------------------------
#  MOO_OPTIONS_FOREACH(MACRO)
#
#  invokes MACRO([NAME],[TYPE],[DEFAULT],[HELP])
#  for each declared options.h option
#  and prohibits any further option declarations.
#
AC_DEFUN([MOO_OPTIONS_FOREACH],
[m4_ifdef([_MOO_OPTIONS_frozen],,
          [m4_define([_MOO_OPTIONS_frozen],[1])])dnl
m4_map_sep([$1],[],[_MOO_OPTIONS_RAW])])

# --------------------------------------------------------------------
#  MOO_OPTION_ARG_ENABLES()
#
#  creates the block of --(disable|enable)-def-OPTION
#  arguments for ./configure.  The value of OPTION to be
#  written into options.h shall be (in increasing order
#  of priority):
#
#  (1) the default given in MOO_DECLARE_OPTIONS
#  (2) the value of the shell variable $moo_d_OPTION
#      if set by some other --enable-*= argument
#  (3) the value given in --enable-def-OPTION=
#
#  The values 'yes' and 'no' are, for options.h,
#  converted to '1' and <#undef>, respectively.
#
AC_DEFUN([MOO_OPTION_ARG_ENABLES],
  [MOO_PUT_HELP([ENABLE],
[
 Single-@%:@define settings for options.h:
      (these will override settings made above; use with care)]
[AS_HELP_STRING([--disable-def-<NAME>],[same as @%:@undef <NAME>],[41])])
MOO_OPTIONS_FOREACH([_MOO_ARG_ENABLE_DEF])])

#  _MOO_ARG_ENABLE_DEF( NAME, TYPE, VALUE, CONFIG_HELP )
#  sets up --(disable|enable)-def-NAME
#
m4_define([_MOO_ARG_ENABLE_DEF],
[AC_ARG_ENABLE([def-$1],
  [AS_HELP_STRING([--enable-def-$1],[$4],[41])],[],
  [AS_VAR_SET_IF([moo_d_$1],
    [AS_VAR_COPY([enableval],[moo_d_$1])],
    [AS_VAR_SET([enableval],[AS_ESCAPE([$3])])])
  AS_VAR_COPY([enable_def_$1],[enableval])])dnl
AS_CASE([$enableval],
  [no],[],
  [yes|''],[AC_DEFINE([$1],[1])],
  [AC_DEFINE_UNQUOTED([$1],[$enableval])])

])
