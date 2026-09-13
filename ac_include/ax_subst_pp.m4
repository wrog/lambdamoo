# ax_subst_pp.m4 - m4sugar substitution primitive
# ====================================================================
#serial 1
#
# Copyright 2026 Roger F. Crew
#
# This file is free software; you have unlimited permission to copy
# and/or distribute it, with or without modifications, as long as this
# notice is preserved.  This file is offered as-is, without any warranty.
#
#
# SYNOPSIS
#
#   ax_subst_pp(<TEMPLATE>, <PRE>, <POST>)
#   ax_subst_pos(<TEMPLATE>, <VALUE>,...)
#   ax_subst_kwd(<TEMPLATE>, <KWD>, <VALUE>, ...)
#
#   AC_REQUIRE([AX_SUBST_PP_INIT])
#   # dummy for autoconf/aclocal ensures this file is m4_included
#
# EXAMPLES
#
#   ax_subst_pos([[Hello %2%, how are [%1%]?]], [you], [world])
#     -> Hello world, how are [you]?
#
#   ax_subst_kwd([[Hello %:W%, how are [%:Y%]; yet [more [[%:W]]].]],
#                [:Y], [you], [:W], [world])
#     -> Hello world, how are [you]; yet [more [world]].
#
#   m4_define([my_world],[Earth])
#   m4_define([my_you],  [things])
#   ax_subst_pp([[Hello %world%, how [are [%you%]]?]],
#               [m4_defn([my_]], [)])
#     -> Hello Earth, how [are [things]]?
#
# DESCRIPTION
#
#   This provides a simple, low-level substitution mechanism
#   It is similar to m4_split() and roughly as inexpensive/bulletproof.
#
#   All functions take a TEMPLATE argument containing zero or more
#   named slots of the form '%NAME%'.  A given name may appear
#   arbitrarily many times.  Remaining arguments determine what values
#   are substituted for which names.  As with macro parameters,
#   substitutions take place no matter how deeply nested the quoting.
#
#   Rules:
#
#   * NAME can be *any* quote-balanced sequence of
#     characters not including '%'.
#   * TEMPLATE must be quote-balanced overall.
#   * '%' must not be used other than to delimit names.
#      Equivalently, TEMPLATE must have an even number of '%'s.
#   * TEMPLATE must not contain '-=<{' or '}>=-',


# ------------------------------------------
# ax_subst_pp( <TEMPLATE>, <PRE>, <POST>)
#   replaces every instance of %NAME% with <PRE>[NAME]<POST>
#   (This is the fastest/lowest-level macro
#   on which all of the others depend).

m4_define([ax_subst_pp],
  [m4_format(_ax_subst_format([$1]),
    m4_map_args_sep([$2], [$3], [,]_ax_subst_words([$1])))])

# -> expands to (quoted) TEMPLATE with all '%NAME%'s replaced with '%s'
m4_define([_ax_subst_format],
  [m4_unquote([m4_changequote([-=<{(],[)}>=-])]dnl
[m4_bpatsubst(-=<{(-=<{($1)}>=-)}>=-,
  -=<{(%[^%]*%)}>=-, -=<{(%s)}>=-)m4_changequote([, ])])])

# -> expands to the sequence of (quoted) 'NAME's with an extra leading [].
m4_define([_ax_subst_words],
  [m4_reverse(m4_shift(m4_reverse(m4_unquote(m4_split([%$1%], [%[^%]*%])))))])



# ------------------------------------------
# ax_subst_pos( <TEMPLATE>, <VALUE1>[, <VALUE2>...])
#   (positional arguments:)
#   NAMEs that are integers n >= 1 are replaced by [$(n+1)]
#   all other NAMEs expand to the empty string

m4_define([ax_subst_pos],
  [m4_pushdef([_ax_subst_fn],[m4_argn(]m4_dquote([$][1],m4_shift($@))[)])dnl
ax_subst_pp([$1],[_ax_subst_fn(],[)])dnl
m4_popdef([_ax_subst_fn])])

# ------------------------------------------
# ax_subst_kwd( <TEMPLATE>, <NAME1>, <VALUE1>[, <NAME2>, <VALUE2>...])
#   (keyword arguments:)
#   Instances of <NAMEn> are replaced with <VALUEn>.
#   A name appearing in the template but not on the parameter list
#   raises an error.

m4_define([ax_subst_kwd],
 [m4_pushdef([_ax_subst_fn],
    m4_translit(m4_dquote(
      [m4_case(]m4_dquote([&1],m4_shift($@),
        [m4_fatal([$0: '&1' value missing])])[)]),[&],[$]))dnl
ax_subst_pp([$1],[_ax_subst_fn(],[)])dnl
m4_popdef([_ax_subst_fn])])]

# ax_subst_kwd_default( <TEMPLATE>, <DEFAULT>[,<NAME1>, <VALUE1>,...])

m4_define([ax_subst_kwd_default],
 [m4_pushdef([_ax_subst_fn],
    [m4_case(]m4_dquote([$][1],m4_shift2($@),[$2])[)])dnl
ax_subst_pp([$1],[_ax_subst_fn(],[)])dnl
m4_popdef([_ax_subst_fn])])]


AC_DEFUN([AX_SUBST_PP_INIT])
