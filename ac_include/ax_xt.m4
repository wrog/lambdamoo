# ax_xt.m4 - language to specify optional variant builds in autoconf
# ===================================================================
#
# DESCRIPTION
#
#   XT is a small, simple domain-specific language for specifying
#   'extensions', optional variant builds that may be invoked by the
#   user of a ./configure script.  An extension may add any of the
#   following:
#
#   .  --enable argument to activate it / choose subfeatures
#   .  files/modules for one or more source groups
#   .  cpp symbols needing to be AC_DEFINEd
#   .  make variables or rules
#   .  shared libraries (with autoconf thunks to find/test them)
#   .  build (sub)directories for statically linked objs/libs
#   .  --with arguments to select libraries or builds
#
# SYNOPSIS
#
#   configure.ac:
#
#     AC_INIT...
#     AC_CONFIG_WHATEVER...
#
#     # better to have a separate file for this
#     m4_include([extensions.ac])
#
#     # ./configure argument processing,
#     AC_PRESERVE_HELP_ORDER  # required, for now, sorry
#
#     # ... for base build
#     AC_ARG_ENABLE(...)
#     ...
#     # ... for extensions
#     AX_XT_EXTENSION_ARGUMENTS
#
#     # base build configure tests
#     AC_CHECK_WALK
#     AC_CHECK_CHEW_GUM
#     ...
#     # configure extensions and finish
#     AX_XT_CONFIGURE_EXTENSIONS
#     AX_XT_CONFIGURE_EPILOGUE
#     AC_OUTPUT
#
#   some_file_findable_by_aclocal.m4:
#
#     AC_DEFUN( [MY_XT_DECLARE_EXTENSIONS],
#       [AC_REQUIRE([AX_XT_INIT])
#       AX_LP_PARSE_SCRIPT([XT], [], [$1])])
#
#   extensions.ac:
#
#     MY_XT_DECLARE_EXTENSIONS([
#     #
#     # This script is in the XT language
#     # '#' is the comment character here, too.
#     # Indentation of non-comment lines matters (like Python).
#
#       %%extension foobity
#         %disabled  # by default
#         --enable- foo
#           %?  make foobish things happen
#           %?- prevent foobish things from happening
#
#         %require bar
#          --with- foobarlib = LIB,..
#              %?   bar library for foo
#              %?*  (gnu=GNU libgnubar, obf=OBF libopenbar)
#           %lib gnu
#             %ac AC_SEARCH_LIBS([[bar_open]], [[gnubar]], [%USE%])
#           %lib obf
#             %ac AC_SEARCH_LIBS([[obf_bar_create]], [[openbar]], [%USE%])
#     ])
#
# REQUIREMENTS
#
#   (*) ax_lp.m4 (LP language parser)
#   (*) Keep out of the ^_?(AX|ax)_(LP|XT|Xt|xt)_.* namespaces)
#   (*) recent autoconf (2.68+)
#
# AUTHOR/COPYRIGHT/LICENSE
#
#   Copyright (C) 2023, 2024, 2025  Roger F. Crew
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License
#   as published by the Free Software Foundation; either version 2
#   of the License, or (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#

# AX_Xt macro name prefix has lowercase 't'
# so that we can use ax_lp_NTSC and not get surprised.

#-------------------------------
# makevar utilities

# each optional source for an extension
# belongs to a source group
m4_define([_AX_Xt_source_groups], [[CSRCS], [HDRS]])

# _AX_Xt_direct_makevars
#   makevars where <var>=<value> appends <value> directly
#   (for all others, append <var>=<value> to XT_MAKEVARS)
#
m4_set_add_all([_AX_Xt_direct_makevars],
  [CPPFLAGS], [XT_LOBJS])
  # +XT_<sourcegroup>  for all sourcegroups

# _AX_Xt_reserved_makevars
#   makevars for which <var>=<value> is not allowed
#
m4_set_add_all([_AX_Xt_reserved_makevars],
  [XT_DIRS], [XT_MAKEVARS], [XT_RULES])
  # +ALL_XT_<sourcegroup>  for all sourcegroups

# _AX_Xt_makevar_source_group_<makevar>
#    source group corresponding to <makevar> (if defined)
#
m4_map_args_sep(
  [ax_lp_beta([&], [m4_do(
       m4_define([_AX_Xt_makevar_source_group_XT_&1],[&1]),
       m4_set_add([_AX_Xt_direct_makevars], [XT_&1]),
       m4_set_add([_AX_Xt_reserved_makevars], [ALL_XT_&1]))],
     ], [)], [],
  _AX_Xt_source_groups)

# _AX_Xt_append_source_group(<MAKEVAR>,<VALUE>)
#   if <MAKEVAR> is XT_<sourcegroup>,
#   split <VALUE> into filenames and add them to that group
#
m4_define([_AX_Xt_append_source_group],
  [m4_ifdef([_AX_Xt_makevar_source_group_$2],
    [m4_set_add_all(
      ax_lp_get([$1],[g_srcgrp_])m4_defn([_AX_Xt_makevar_source_group_$2]),
      m4_unquote(m4_split([$3])))])])

# _AX_Xt_substed_makevars
#    all makevars that have substitutions due to extensions
#
m4_set_add_all([_AX_Xt_substed_makevars]dnl
[]m4_set_listc([_AX_Xt_direct_makevars])dnl
[]m4_set_listc([_AX_Xt_reserved_makevars]))

# AX_Xt_add_makevar(<CTX>,<VAR>,<VALUE>)
#    perform assignment for '=' subcmd
#    ('%dirvar' and '%make' use the _$0() version of this directly)
#
m4_define([AX_Xt_add_makevar],
  [m4_set_contains([_AX_Xt_reserved_makevars], [$2],
    [ax_lp_fatal([$1],['$2' cannot be assigned here])],
    [m4_set_contains([_AX_Xt_direct_makevars], [$2],
      [_$0($@)_AX_Xt_append_source_group($@)],
      [_$0([$1], [XT_MAKEVARS], [$2 = $3])])])])

# _AX_Xt_makevar_sep(<VAR>)
#    -> separator to use when adding a value to <VAR>
#
m4_define([_AX_Xt_makevar_sep], ax_lp_NTSC(
  [m4_ifdef([_AX_Xt_makevar_sep_$1],
     [m4_defn([_AX_Xt_makevar_sep_$1])], [S])]))
m4_define([_AX_Xt_makevar_sep_XT_RULES],    ax_lp_NTSC([N]))
m4_define([_AX_Xt_makevar_sep_XT_MAKEVARS], ax_lp_NTSC([N]))

# _AX_Xt_add_makevar(<CTX>,<VAR>,<VALUE>)
#    append [sep]?[value] to [var], no checking
#
m4_define([_AX_Xt_add_makevar],
  [ax_lp_hash_append([$1], [makevars], [$2],
     [$3], _AX_Xt_makevar_sep([$2]))])

# _AX_Xt_put_makevars(<CTX>,<INDENT>)
#   -> shell code to actually set all of the makevars
#      affected at this level
#
m4_define([_AX_Xt_put_makevars],
  [ax_lp_hash_map_keys_sep([$1], [makevars],
    [m4_pushdef([_ax_xt_v],],
    [)
m4_format([[%*s]],[$2])dnl
ax_lp_beta([&],m4_if(m4_defn([_ax_xt_v]),[XT_MAKEVARS],
                     [[[&1="$&1${&1+&2}&3"]]],
                     [[[&1=$&1${&1+'&2'}'&3']]]),
       m4_defn([_ax_xt_v]),
       _AX_Xt_makevar_sep(m4_defn([_ax_xt_v])),
       ax_lp_hash_get([$1], [makevars],
                      m4_defn([_ax_xt_v])))dnl
m4_popdef([_ax_xt_v])])])

#-------------------------------
# cdefine utilities

#   mode 0:  %cdefine name value?(default=yes)
#     #define [name] [value]/1 if this extension is active
#     %option/%lib optional second arg is a 2nd #define
#       that should be set if that option/lib is selected
#
#   mode 1:  %cdefine nameformat_%s
#     #define nameformat_NAME (yes/1) if option/lib NAME is selected
#     %option/%lib second arg substitutes for NAME
#
#   mode 2:  %cdefine name valueformat_%s
#     #define [name] is enumeration of valueformat_NAME values
#     %option/%lib second arg substitutes for NAME
#     for %%extension, value is bit-or(|) of selected options
#     for %require, value is that of selected lib

# _AX_Xt_cdef_setup(<CTX>,<NAME_ARG>,<VALUE_ARG>)
#    process a %cdefine
m4_define([_AX_Xt_cdef_setup], [m4_do(
  m4_ifval(ax_lp_get([$1], [cdef_sym]),
    [ax_lp_fatal([$1],['%cdefine' again? ])]),

  ax_lp_set_empty([$1], [all_kwds], [],
    [ax_lp_fatal([$1], m4_joinall([ ],
      ['%cdefine' must come before any],
      m4_if(ax_lp_parent_cmd([$1]), [%%extension],
        [['%option']], [['%lib']]))))]),

  m4_ifval([$2],
    [m4_if(m4_bregexp([$2],[%s]),[-1],
      [ax_lp_put([$1], [cdef_sym],  [$2])],
      [ax_lp_put([$1], [cdef_sym],  m4_dquote([$2]),
                       [cdef_mode], [1])])],
    [ax_lp_fatal([$1],['%cdefine' name required])]),

  m4_ifval([$3],
    [m4_if(ax_lp_get([$1], [cdef_mode]),[1],
      [ax_lp_fatal([$1],['%cdefine name%s value' not allowed])],
      [m4_if(m4_bregexp([$3],[%s]),[-1],
        [ax_lp_put([$1], [cdef_val],  [$3])],
        [ax_lp_put([$1], [cdef_val],  m4_dquote([$3]),
                         [cdef_mode], [2])])])]),

  m4_if(ax_lp_get([$1], [cdef_mode]),[0],
    [ax_lp_set_add([$1], [all_cdefs], [$2])]))])

# when encountering   %option/%lib <NAME> <KEYWORD>?
# (<CTX>, <NAME>, <KEYWORD>)
#   -> _AX_Xt_add_cdef2(<CTX>, <CPPSYM>, <CPPVALUE>)
m4_define([_AX_Xt_add_cdef],
  [m4_case(ax_lp_get([$1], [cdef_mode]),
    [2], [_AX_Xt_add_cdef2([$1],
              ax_lp_get([$1], [cdef_sym]),
              m4_format(ax_lp_get([$1], [cdef_val]),
                 m4_default_quoted([$3], m4_toupper([$2]))))],
    [1], [_AX_Xt_add_cdef2([$1],
              m4_format(ax_lp_get([$1], [cdef_sym]),
                        m4_default_quoted([$3],m4_toupper([$2]))))],
    [0], [m4_ifval([$3], [_AX_Xt_add_cdef2([$1], [$3])])],
    [m4_fatal([cant happen (cdef_mode = ]ax_lp_get([$1], [cdef_mode])[)])])])

m4_define([_AX_Xt_add_cdef2],[m4_do(
  ax_lp_set_add([$1], [all_cdefs], [$2]),
  ax_lp_put([$1],     [this_cdef], [$2],
                      [this_cval], [$3]))])


# _AX_Xt_put_nonoption_cdefs(<CTX>)
#   -> shell code to ensure all_cdefs get AC_DEFINED
#      even if they are *not* options
#   (there is an extra layer of quotes on these so that
#    they do not expand prior to MOO_XT_CONFIGURE_EXTENSIONS,
#    since the relevant MOO_DECLARE_OPTIONS may be after
#    the extension declaration)
m4_define([_AX_Xt_put_nonoption_cdefs],
  [ax_lp_set_map_sep([$1],[all_cdefs],
    [[_AX_Xt_put_cdef_unless_option(]m4_dquote(],
    [,ax_lp_get([$1],[g_cdef_set],[g_sh_cdef_]))[)]])])

m4_define([_AX_Xt_put_cdef_unless_option],
  [m4_set_contains([$2],[$1],[],[
AS_VAR_COPY([_ax_xt_v],[$3][$1])
AS_CASE([$_ax_xt_v],
  [no],[],
  [yes|''],[AC_DEFINE([$1],[1])],
  [AC_DEFINE_UNQUOTED([$1],[$_ax_xt_v])])])])


#=======================
# The XT language
#=======================

# dummy routine to ensure this file is loaded
AC_DEFUN([AX_XT_INIT],[])

AX_LP_DEFINE_LANGUAGE([XT],[AX_XT_DEFINE])

#----------
# root cmd
#----------

# INITARGS for AX_LP_PARSE_SCRIPT is an alternating list of
# keyword/value arguments.  Default values are set in the :var
# declarations below.  The recognized keywords are:
#
# g_args
#   name of macro to accumulate AC_ARG_ENABLE/WITH()s for
#   extension command line arguments and shell code
#   to determine which extensions are active and set
#   values for cdefine shell variables (see g_sh_cdef_) accordingly.
#
# g_configure
#   name of macro to accumulate shell code to do final extension
#   configurations (find %libs and %build directories,
#   issue AC_DEFINEs for cdefines not pre-empted by g_cdef_set)
#
# g_epilogue
#   name of macro to for final extension configuration actions
#   e.g., issue AC_SUBSTs for make variables whose values are
#   not known until all extensions have been processed
#
# g_sh_var_
#   shell variable prefix for certain public variables:
#     (g_sh_var_)do_(extension)  -- extension is active
#     (g_sh_var_)active_xts  -- list of active extensions
#     (g_sh_var_)rqtry_(requirement)  -- library search list
#     (g_sh_var_)rquse_(requirement)  -- library chosen
#
# g_sh_cdef_
#   shell variable prefix for %extension/%option cdefine values
#   as determined from ./configure arguments
#
# g_srcgrp_
#   name prefix for m4 sets that accumulate source group members,
#   i.e., all filenames specifed in 'XT_(CSRCS|HDRS|...) ='
#   declarations for all extensions.
#
# g_cdef_set
#   m4 set of %extension/%option cdefines pre-empted from being
#   AC_DEFINEd in g_configure and instead expected to be handled by
#   the caller elsewhere, perhaps using the g_sh_cdef_* values,
#   or perhaps not.
#   cdefines NOT in this set get AC_DEFINEd in g_configure
#   to whatever the g_sh_cdef_* value is
#
#   (note that cdefines created by %require/%lib are *always*
#    AC_DEFINEd in g_configure, since these values may depend on
#    library searches which have to wait until then anyway)
#


AX_XT_DEFINE([],
  [:var], [[g_args],      [AX_XT_DECLARE_EXTENSIONS]],
  [:var], [[g_configure], [AX_XT_EXTENSION_ARGUMENTS]],
  [:var], [[g_epilogue],  [AX_XT_CONFIGURE_EXTENSIONS]],
  [:var], [[g_sh_cdef_],  [ax_xt__d_]],
  [:var], [[g_sh_var_],   [ax_xt_]],
  [:var], [[g_srcgrp_],   [AX_Xt_SGROUP_]],
  [:var], [[g_cdef_set],  [AX_Xt_CDEF_SET]],

  [:fn],  [ax_lp_check_puts($@)ax_lp_put($@)],

  [:fnend],
  [ax_lp_beta([&],
     [m4_define([&1],
m4_set_map_sep(
    [_AX_Xt_substed_makevars],
    [[AC_SUBST(]m4_dquote(],[)[)]])dnl
m4_map_args_sep(
     [ax_lp_beta([@],[
[[ALL_XT_@1=]]m4_dquote('m4_set_map_sep([&2@1],[],[],[ ])')],],
     [)], [],
     _AX_Xt_source_groups))],

ax_lp_get([$1],[g_epilogue],[g_srcgrp_]))])


# purposefully screw things up (uncomment to test for underquoting)
# m4_define([extension],[detention])
# m4_define([LANGUAGE_BITWISE],[LANGUAGE_FRACKYOU])
# m4_define([identifiers],[identif4ckifiers])
# --- these do not work; autoconf internals are underquoted
# m4_define([yes],[maybe])
# m4_define([CFLAGS],[CFLAGS_CAN_BITE_ME])
# (end test)

#-------------
# %%extension
#-------------

AX_XT_DEFINE([%%extension],
  [:parent], [],
  [:subcmds],[[%disabled], [--enable-], [%cdefine],
              [%option], [%option_set], [%?]],
  [:vars],   [[xt_acarg], [xt_cases], [xt_disabled],
              [ew], [ew_var], [ew_name], [ew_vdesc],
              [help], [cdef_sym], [cdef_val],
              [reqs], [req2s]],
  [:var],    [[cdef_mode], [0]],
  [:var],    [[xt_name],  [$2]],
  [:sets],   [[all_kwds], [all_options], [all_cdefs]],

  [:subcmds],[[=]],
  [:hashes], [[makevars]])


AX_XT_DEFINE([%disabled],
  [:fn], [ax_lp_put([$1], [xt_disabled], [1])])


AX_XT_DEFINE([=],
  [:fn], [AX_Xt_add_makevar([$1],[$2],[$3])])


# (--enable-|--with-) *<NAME> *(= *<DESCRIPTION>)?
#     -> (<NAME>, <DESCRIPTION>)
m4_define([_AX_Xt_args_ew],
  [[:args], ax_lp_NTSC(
   [m4_bregexp(]m4_dquote([$][2])[,
               [\`\([^=ST]*\)[ST]*\(\|=[ST]*\(.*\)\)],
               [[\1],[\3]])])])

AX_XT_DEFINE([--enable-],
  [:subcmds], [[%?], [%?-]],
  _AX_Xt_args_ew,
  [:fn],
    [_AX_Xt_ew_defns([$1], [enable], [$2], [$3])])

AX_XT_DEFINE([--with-],
  [:subcmds], [[%?], [%?*]],
  _AX_Xt_args_ew,
  [:fn],
    [_AX_Xt_ew_defns([$1], [with], [$2], [$3])])

m4_define([_AX_Xt_ew_defns],
  [m4_ifval([$3],[],[ax_lp_fatal([$1],['--enable|with-' name expected])])dnl
ax_lp_put([$1], [ew],       [$2],
                [ew_name],  [$3],
                [ew_vdesc], [$4],
                [ew_var],   m4_translit([[$2-$3]],[-+.],[___]))])

m4_define([_AX_Xt_ew_errname],
  [ax_lp_beta([&],
    [m4_ifval([&2],
        [[--&3-&2]],
        [[%%extension &1]][m4_ifval([&4], [[ (%requires &4)]])])],
    ax_lp_get([$1],[xt_name],[ew_name],[ew]),
    ax_lp_ifdef([$1],[rq_name],[ax_lp_get([$1],[rq_name])]))])


AX_XT_DEFINE([%?],[:fn], ax_lp_NTSC(
  [m4_ifval(
    ax_lp_ifdef([$1], [kwd_select],
                [m4_if(ax_lp_parent_cmd([$1],[2]), [%build],[],[1])]),
    [ax_lp_append([$1], [help],
       m4_format([[[N%22s: . . %s]]],
         ax_lp_get([$1], [kwd_select]), [$2]))],
    [m4_if(ax_lp_parent_cmd([$1]),[%%extension],
           [ax_lp_append([$1], [help], [[NS$2]])],
           [_AX_Xt_addhelp([$1],
               ax_lp_get([$1], [ew]),
               ax_lp_get([$1], [ew_vdesc]),
               [$2])])])]))

AX_XT_DEFINE([%?-],[:fn],
  [_AX_Xt_addhelp([$1],
     m4_if(ax_lp_get([$1], [ew]),[enable],[[disable]],[[without]]),
     [],[$2])])

AX_XT_DEFINE([%?*],[:fn],
  [ax_lp_append([$1], [help], [
AS_HELP_STRING([],[$2],[28])])])

m4_define([_AX_Xt_addhelp],
  [ax_lp_append([$1], [help],
    m4_format([[%s][AS_HELP_STRING([--$2-%s%s],[%s])]],
      m4_ifval(ax_lp_get([$1], [help]), m4_newline()),
      ax_lp_get([$1], [ew_name]),m4_ifval([$3],[[=$3]]),[$4]))])


# common case of arguments being whitespace-separated words
m4_define([_AX_Xt_args_split_into_words],
  [[:args],
   [m4_unquote(m4_split(]m4_dquote([$][2])[))]])


AX_XT_DEFINE([%cdefine],
  _AX_Xt_args_split_into_words,
  [:fn],
    [_AX_Xt_cdef_setup($@)])


AX_XT_DEFINE([%option],
  [:subcmds], [[%implies], [%?], [%alt]],

  _AX_Xt_args_split_into_words,
  [:sets], [[opt_implies]],
  [:var],  [[opt_name],  [$2]],
  [:var],  [[kwd_select],[$2]],

  [:fn], [m4_do(
    _AX_Xt_set_require_new([$1], [all_kwds], [$2]),
    ax_lp_set_add([$1], [all_options],[$2]))],

  [:vars], [[this_cdef], [this_cval]],
  [:fn], [_AX_Xt_add_cdef([$1],[$2],[$3])])


AX_XT_DEFINE([%alt],
  [:fn], [m4_do(
    _AX_Xt_set_require_new([$1], [all_kwds], [$2]),
    ax_lp_ifdef([$1], [kwd_xselect],
      [ax_lp_prepend([$1], [kwd_xselect], [x$2], [|])]),
    ax_lp_prepend([$1],    [kwd_select],  [$2],  [|]))])


AX_XT_DEFINE([%implies],
  [:fn], [m4_do(
    _AX_Xt_set_require_prev([$1], [all_kwds], [$2]),
    ax_lp_set_add([$1], [opt_implies], [$2]))])


AX_XT_DEFINE([%option_set],
  [:args], ax_lp_NTSC(
    [m4_if(m4_bregexp([$2],[^[^TS]+[TS]*=]),[-1],
      [ax_lp_fatal([$1],[= expected])],
      [m4_bpatsubst([[$2]],
        [^.\([^TS]+\)[TS]*=[TS]*\(.*\).$],
        [[\1],m4_do(m4_split([\2],[,[TS]*]))])])]),

  [:sets], [[os_mems]],
  [:var],  [[os_name], [$2]],

  [:fn], [m4_do(
    m4_map_args_sep(
      [_AX_Xt_set_require_new([$1], [all_kwds],], [)], [],
      m4_do(m4_split([$2],[|]))),
    m4_map_args_sep(
      [_AX_Xt_set_require_prev([$1], [all_kwds],], [)], [],
      m4_shift2($@)),
    ax_lp_set_add_all([$1], [os_mems], m4_shift2($@)))],

  [:fnend], [m4_do(
     ax_lp_append([$1], [help],
       m4_format(ax_lp_NTSC([[[N%22s: . . %s]]]),
         ax_lp_get([$1], [os_name]),
         ax_lp_set_map_sep([$1], [os_mems],[],[],[[+]]))),
     ax_lp_append([$1], [xt_cases],
       _AX_Xt_xtset_cases([$1],
          ax_lp_get([$1], [os_name]), [os_mems])))])

m4_define([_AX_Xt_xtset_cases],
  [ax_lp_beta([&], [[,[[
      $2]], [[
        _ax_xt_ll="&1&2"
        continue]]]],
                m4_if([$2],[yes],[[__done__]],[[$_ax_xt_ll]]),
                ax_lp_set_map_sep([$1], [$3], [[,]]))])


AX_XT_DEFINE([%require],
  [:parent],  [%%extension],
  [:subcmds], [[--with-], [%cdefine], [%ac_yes]],

  [:sets], [[all_kwds], [all_cdefs]],
  [:vars], [[ew_name],[ew_var],[ew_vdesc],[yescode],
            [all_libs],[cdef_sym],[cdef_val],[rq_acarg],
            [icases],[scases],[ucases],
            [help],[blds]],
  [:var],  [[cdef_mode], [0]],
  [:var],  [[rq_name],  [$2]],
  [:var],  [[ew],     [with]])


# _AX_Xt_with_arg([ctx])
#   -> AC_ARG_WITH for %require or %build
#
m4_define([_AX_Xt_with_arg],
  [ax_lp_beta([&], [[
AC_ARG_WITH([&3],
  [&4],
[AS_CASE([[$withval]],[[
   no]],[
     AS_IF([[$&1do_&2]],[
       AC_MSG_ERROR([[--without-&3 $_ax_xt_conflict]])])[
     _ax_xt_saw_no=:
     _ax_xt_conflict="conflicts with --without-&3"]],
[
     AS_IF([[$_ax_xt_saw_no]],[
       AC_MSG_ERROR([[--with-&3 $_ax_xt_conflict]])])[
     &1do_&2=:
     _ax_xt_conflict="conflicts with --with-&3"]])])]],

         ax_lp_get([$1], [g_sh_var_], [xt_name], [ew_name], [help]))])


AX_XT_DEFINE([%lib],
  [:parent], [%require],
  [:subcmds],[[%ac], [%alt]],
  _AX_Xt_args_split_into_words,
  [:vars],   [[code]],
  [:var],    [[lib_name],   [$2]],
  [:var],    [[kwd_select], [$2]],
  [:var],    [[kwd_xselect],[x$2]],

  [:subcmds],[[=]],
  [:hashes], [[makevars]],

  [:fn], [m4_do(
    ax_lp_append([$1], [all_libs], [$2], [[ ]]),
    _AX_Xt_set_require_new([$1], [all_kwds], [$2]))],

  [:vars], [[this_cdef], [this_cval]],
  [:fn], [_AX_Xt_add_cdef([$1],[$2],[$3])])


AX_XT_DEFINE([%build],
  [:parent],  [%lib],
  [:subcmds], [[--with-],[%ac]],

  [:vars],   [[ew_name],[ew_var],[ew_vdesc],[help],[code]],
  [:var],    [[ew], [with]],

  [:subcmds],[[=], [%make], [%dirvar], [%path]],
  [:vars],   [[dirvar], [path]],
  [:hashes], [[makevars]])


AX_XT_DEFINE([%dirvar],
  [:fn], [m4_do(
     m4_ifval(ax_lp_get([$1], [dirvar]),
       [ax_lp_fatal([$1], [second %dirvar for this %build?])]),
     m4_set_contains([_AX_Xt_substed_makevars], [$2],
       [ax_lp_fatal([$1], [$2 cannot be used as %dirvar])]),
     ax_lp_put([$1], [dirvar], [$2]),
     _AX_Xt_add_makevar([$1], [XT_MAKEVARS], [$2 = $_ax_xt_dir]),
     _AX_Xt_add_makevar([$1], [XT_DIRS], [$($2)]))])

AX_XT_DEFINE([%path],
  [:fn], [m4_do(
    m4_ifval(ax_lp_get([$1], [path]),
       [ax_lp_fatal([$1],[second %path in this %build?])]),
    m4_ifval([$2],
       [ax_lp_put([$1], [path], [$2])],
       [ax_lp_fatal([$1],[%path directory expected])]))])

AX_XT_DEFINE([%make],
  [:args],
    [_AX_Xt_trimmake([$2])],
  [:fn],
    [_AX_Xt_add_makevar([$1],[XT_RULES],[$2])])

# Strip away unnecessary extra blank lines.
# Recall that m4_bpatsubsts adds extra []s,
# so `\(..\) is really `[[ and \(..\)' is really ]]'
m4_define([_AX_Xt_trimmake], ax_lp_NTSC(
 [m4_bpatsubsts([[$1]],
  [\`\(..\)\([ST]*N\)+],[\1],
  [\([^N]\)\([ST]*N\)*\(..\)\'],[\1N\3],
  [^\(T.*\)N\(..\)\'],[\1NN\2])]))


AX_XT_DEFINE([%ac],
  [:fn],
    [ax_lp_put([$1], [code], [$2])])

AX_XT_DEFINE([%ac_yes],
  [:fn],
    [ax_lp_put([$1], [yescode], [$2])])


m4_define([_AX_Xt_set_require_new],
  [ax_lp_set_add([$1], [$2], [$3], [],
    [ax_lp_fatal([$1],[keyword '$3' already defined])])])

m4_define([_AX_Xt_set_require_prev],
  [ax_lp_set_contains([$1], [$2], [$3], [],
    [ax_lp_fatal([$1],[unknown keyword: '$3'])])])


AX_XT_DEFINE([%option],
  [:fnend],
    [ax_lp_append([$1], [xt_cases],
      m4_format(ax_lp_NTSC([[,[[N%*s%s]], [%s]]]),
        [6],[],
        ax_lp_get([$1], [kwd_select]),
        m4_do(m4_dquote_elt(
          m4_ifval(ax_lp_get([$1], [this_cdef]),
            [ax_lp_beta([&],
              m4_if(ax_lp_get([$1], [cdef_mode]),[2],
[[[
        AS_CASE([[$_ax_xt_v'|']],[[
          *"|&3|"*]],[[
            continue]],[[
            _ax_xt_v="$_ax_xt_v|&3"]])]]],
[[[[
        _ax_xt_d=&2]
        AS_VAR_SET_IF([&1$_ax_xt_d],[[continue]],[[_ax_xt_v=&3]])]]]),

                ax_lp_get([$1], [g_sh_cdef_], [this_cdef]),
                m4_default_quoted(ax_lp_get([$1], [this_cval]),[yes]))]),

          ax_lp_set_empty([$1], [opt_implies],
            [], [m4_format(
[[[
        _ax_xt_ll="$_ax_xt_ll%s"]]],
                   ax_lp_set_map_sep([$1], [opt_implies],[[,]]))]),

          m4_ifval(ax_lp_get([$1], [this_cdef]),
            [],
[[[
        continue]]])))))])


AX_XT_DEFINE([%build],
  [:fnend],
  [m4_ifval(ax_lp_get([$1], [dirvar]),[],
    [ax_lp_fatal2([$1],[%dirvar required],[... by %build])])dnl
dnl
ax_lp_beta([&],
  [m4_ifval([&1],
    [ax_lp_append([$1], [rq_acarg], _AX_Xt_with_arg([$1])dnl
m4_ifval([&3],[],[[
AS_CASE([[$&2]],[[
  yes]], [
    AC_MSG_ERROR([[--with-&1 requires a directory (%path not set)]])])]]))],
    [m4_ifval([&3],[],
      [ax_lp_fatal2([$1],[one of --with- or %path required],[... by %build])])])],

           ax_lp_get([$1],[ew_name],[ew_var],[path]))dnl
dnl
ax_lp_append([$1], [blds],
          ax_lp_beta([&], [[
  AS_IF([[test "x$&1rquse_&2" = x]],]dnl
m4_ifval([&3],[[[
    AS_CASE([[x$&3]],[[
      x|xno]],[],[&6][&7])]]],
  [m4_ifval([&5],[[[
    AS_CASE([[x$&5]],[[
      xno]],[],[[
      x|xyes|&4]],[&7])]]],
    [[&7]])])[)]],

           ax_lp_get([$1], [g_sh_var_], [rq_name],
                           [ew_var], [kwd_xselect]),
           ax_lp_get_prior([$1],[ew_var]),

           ax_lp_beta([&], [m4_ifval([&1], [m4_ifval([&4], [[
      AS_CASE([[x$&1]],[[
        x|xyes|&7]],[],
        [AC_MSG_ERROR([[--&6-&5=DIR vs. --&3-&2=$&1]])])]])])],

             ax_lp_get_prior([$1],[ew_var],[ew_name],[ew]),
             ax_lp_get([$1],      [ew_var],[ew_name],[ew],
                                  [kwd_xselect])),

           ax_lp_beta([&], [[[
      &1rquse_&2=&3]&6
      AS_SET_CATFILE([[_ax_xt_dir1]], [[$srcdir]], [[$_ax_xt_dir0]])
      AS_IF([[test -d "$_ax_xt_dir1"]], [], [
        AC_MSG_ERROR([[directory not found: $_ax_xt_dir1]])])
      AS_SET_CATFILE([[_ax_xt_dir]], [['$(abs_srcdir)']], [[$_ax_xt_dir0]])[&5]]dnl
m4_ifval([&4], [m4_bpatsubst([[
      &4]], [%DIR%], [[$_ax_xt_dir1]])])],

          ax_lp_get([$1], [g_sh_var_], [rq_name], [lib_name], [code]),
          _AX_Xt_put_makevars([$1], [6]),
          ax_lp_beta([&], [m4_ifval([&1], [m4_ifval([&2],[[
      AS_IF([[test "x$&1" = xyes]],[[
        _ax_xt_dir0="&2"]],[[
        _ax_xt_dir0=$&1]])]],[[[
      _ax_xt_dir0=$&1]]])],[[[
      _ax_xt_dir0="&2"]]])],
               ax_lp_get([$1], [ew_var], [path])))))])

AX_XT_DEFINE([%lib],
   [:fnend],
   [ax_lp_append([$1], [icases], ax_lp_beta([&],
[[,[[
      &2]],  [[
        &1rqtry_&3="$&1rqtry_&3 &4"]]]],

      ax_lp_get([$1], [g_sh_var_], [kwd_select],
                      [rq_name], [lib_name])))dnl
ax_lp_append([$1], [scases],
  ax_lp_beta([&],
    m4_ifval(ax_lp_get([$1], [code]),
[[[,[[
      &3]],  [
        &4
        AS_IF([[$_ax_xt_found_it]],[[
          &1rquse_&2=&3
          break]])]]]],
[[[,[[
      &3]],  [[
        _ax_xt_found_it=:
        &1rquse_&2=&3
        break]]]]]),

             ax_lp_get([$1], [g_sh_var_], [rq_name], [lib_name]),
             m4_bpatsubsts(m4_dquote(ax_lp_get([$1], [code])),
               [%USE%],  [[_ax_xt_found_it=:]],
               [%FAIL%], [[_ax_xt_lfail1]])))dnl
dnl
ax_lp_append([$1], [ucases],
  ax_lp_beta([&], m4_format(
[[[,[[
  x&1]],  [%s&4]]]], m4_ifval(ax_lp_get([$1], [this_cdef]),[[
    AC_DEFINE([&2], [&3])]])),

             ax_lp_get([$1],[lib_name],[this_cdef]),
             m4_default_quoted(ax_lp_get([$1], [this_cval]),[1]),
             _AX_Xt_put_makevars([$1],[4])))])


AX_XT_DEFINE([%require],
  [:fnend],

dnl  xt.[xt_acarg] +=
dnl    AC_ARG_WITH for --with-<req> and each of
dnl      the various builds for <req>  ([rq_acarg])
dnl
[ax_lp_append([$1], [xt_acarg],
  m4_ifval(ax_lp_get([$1], [ew_name]),
    [_AX_Xt_with_arg([$1])])dnl
ax_lp_get([$1], [rq_acarg]))dnl
dnl
dnl  make [yescode] set <g_sh_var_>rqtry_<req>
dnl    prior value being either blank, "lib1,lib2,..."
dnl      or something that sets %LIBS%.
dnl
ax_lp_put([$1], [yescode],
  ax_lp_beta([&],
    [m4_bpatsubst(
      m4_cond([[&4]],                      [],   [[[%LIBS%[="&3"]]]],
              [m4_bregexp([&4],[%LIBS%])], [-1], [[[%LIBS%[=]&4]]],
                                                 [[[&4]]]),
      [%LIBS%], [[&1rqtry_&2]])],

    ax_lp_get([$1], [g_sh_var_], [rq_name], [all_libs], [yescode])))dnl
dnl
dnl  xt.[reqs] +=
dnl    if a %build was selected, set makevars/etc for it and also
dnl      set <g_sh_var_>rquse_<req>=corresponding lib (rq.[blds])
dnl    otherwise set <g_sh_var_>rqtry_<req> to library search order
dnl      (using rq.[icases] and rq.[yescode])
dnl
ax_lp_append([$1], [reqs],
  ax_lp_beta([&], [[[
&1rquse_&3=]]m4_ifval([&6], [[
AS_IF([[$&1do_&2]],[&6])]])[[
_ax_xt_l=]]m4_ifval([&5], [[[$&4]]])[[
&1rqtry_&3=]
AS_IF([[test x$&1rquse_&3 = x]],[
  AS_IF([[test x$_ax_xt_l = x]],[
    AS_IF([[$&1do_&2]],[[_ax_xt_l=yes]],[[_ax_xt_l=no]])])[
  ac_save_IFS=$IFS
  IFS=,
  for _ax_xt_kwd in $_ax_xt_l ; do
    IFS=$ac_save_IFS]
    AS_CASE([[$_ax_xt_kwd]]&7,[[
      no]],  [[
        &1rqtry_&3=
        break]],[[
      yes]], [
        &8[
        break]],[
      AC_MSG_ERROR([[unknown &9 keyword: $_ax_xt_kwd]])])[
  done]])]],
             ax_lp_get([$1], [g_sh_var_], [xt_name], [rq_name],
                             [ew_var], [ew_name],
                             [blds], [icases], [yescode]),
             _AX_Xt_ew_errname([$1])))dnl
dnl
dnl  xt.[req2s] +=
dnl    do the library search (using rq.[scases])
dnl    set moo_xt_rquse to library found
dnl    configure library (using rq.[ucases])
dnl
ax_lp_append([$1], [req2s],
  ax_lp_beta([&],
[[
AS_IF([[test "x$&1rqtry_&2" != x]],[[
  _ax_xt_found_it=false
  _ax_xt_lfail_extras=
  for _ax_xt_lib in $&1rqtry_&2 ; do
    _ax_xt_lfail1=]
    AS_CASE([[$_ax_xt_lib]]&3,[
      AC_MSG_ERROR([[?? \$&1rqtry_&2 kwd = $_ax_xt_lib]])])
    AS_IF([[test "x$_ax_xt_lfail1" != x]],[[
      _ax_xt_lfail_extras="$_ax_xt_lfail_extras
  $_ax_xt_lib: $_ax_xt_lfail1"]])[
  done]
  AS_IF([[$_ax_xt_found_it]],[],[
    AC_MSG_ERROR([[&5: no library found$_ax_xt_lfail_extras]])])])
AS_CASE([[x$&1rquse_&2]],[[
  x]],[]&4,[
  AC_MSG_ERROR([[?? \$&1rquse_&2 = $&1rquse_&2]])])]],

        ax_lp_get([$1], [g_sh_var_], [rq_name], [scases], [ucases]),
        _AX_Xt_ew_errname([$1])))])

AX_XT_DEFINE([%%extension],
  [:fnend],
  [ax_lp_beta([&],
    [m4_append([&1],
      [AS_IF([[$&2do_&3]], [[&2active_xts="$&2active_xts &3"][&6]])]dnl
[&7][&4][&5])],

        ax_lp_get([$1],[g_configure],[g_sh_var_],
                       [xt_name],[reqs],[req2s]),
        _AX_Xt_put_makevars([$1],[2]),
        _AX_Xt_put_nonoption_cdefs([$1]))])

AX_XT_DEFINE([%%extension],
  [:fnend],
  [ax_lp_set_contains([$1], [all_kwds], [yes], [],
    [dnl
dnl
dnl fix xt_cases:
dnl if no 'yes' case is provided (%option or %option_set)
dnl fill one in that sets all of the flags
ax_lp_append([$1], [xt_cases],
        m4_if(ax_lp_get([$1], [cdef_mode]),[0],
            [[,[[
      yes]], [[
        continue]]]],
            [_AX_Xt_xtset_cases([$1], [yes], [all_options])]))])dnl
dnl
dnl fix xt_cases:
dnl provide 'no' case
ax_lp_append([$1], [xt_cases], m4_format([[,[[
      no]], [[
        _ax_xt_ll=__done__%s
        break]]]],
        m4_quote(ax_lp_set_map_sep([$1], [all_cdefs], [
        ]ax_lp_get([$1],[g_sh_cdef_]), [=no]))))dnl
dnl
dnl build xt_acarg:
dnl
ax_lp_put([$1], [xt_acarg],
  ax_lp_beta([&],
[[[&1do_&2=false
_ax_xt_saw_no=false
_ax_xt_conflict=]]]
dnl
dnl   Either we have an --enable defined
dnl
[m4_ifval([&3],[[
AC_ARG_ENABLE([&3],
  [&4],
[AS_CASE([[$enableval]],[[
   no]],[[
     _ax_xt_saw_no=:
     _ax_xt_conflict="conflicts with --disable-&3"]],
   [[
     &1do_&2=:
     _ax_xt_conflict="conflicts with --enable-&3"]])])]],
dnl
dnl  ... or we do not.  There may still be a help header
dnl
[m4_ifval([&4],[[
m4_divert_text([HELP_ENABLE],[&4])]])])],

       ax_lp_get([$1], [g_sh_var_], [xt_name], [ew_name], [help]))dnl
dnl
dnl   include --with args defined by %require stanzas
dnl
ax_lp_get([$1],[xt_acarg])dnl
dnl
dnl   do default cases:
dnl   non-%disabled extension needs absense of --disable/--without;
dnl   %disabled extension needs an explicit --enable/--with
dnl     which we should have seen already
dnl
ax_lp_beta([&],
  [m4_ifval([&5],[],[[
AS_IF([[$_ax_xt_saw_no]],
  [AC_MSG_NOTICE([[(%%extension &2:) disabled]])],
  [[&1do_&2=:]])]])
dnl
dnl   _ax_xt_l <- 'yes', 'no', or keyword list
dnl
[AS_IF([[$&1do_&2]],
  ]m4_ifval([&3],
    [[[AS_IF([[test "x$&4" = x]],
      [[  _ax_xt_l=yes]],[[  _ax_xt_l=$&4]])]]],
    [[[[_ax_xt_l=yes]]]])[,
  [[_ax_xt_l=no]])]],

    ax_lp_get([$1], [g_sh_var_], [xt_name],
                    [ew_name], [ew_var], [xt_disabled]))dnl
dnl
dnl   xt_cases expand keywords
dnl
ax_lp_beta([&],[m4_ifval([&3],[[[
_ax_xt_v=0]]])[[
while test "$_ax_xt_l" != __done__ ; do
  ac_save_IFS=$IFS
  IFS=,
  _ax_xt_ll=__done__
  for _ax_xt_kwd in $_ax_xt_l ; do
    IFS=$ac_save_IFS]
    AS_CASE([[$_ax_xt_kwd]],[[
      __done__]],[[
        continue]]&2,[
      AC_MSG_ERROR([[unknown &4 keyword: $_ax_xt_kwd]])])]dnl
m4_ifval([&3],,[[
    AS_VAR_SET([&1$_ax_xt_d],[[$_ax_xt_v]])
    AC_MSG_NOTICE([[(&4:) $_ax_xt_d = $_ax_xt_v]])]]dnl
)[[
  done
  _ax_xt_l=$_ax_xt_ll
done]]],
           ax_lp_get([$1], [g_sh_cdef_], [xt_cases]),
           m4_if(ax_lp_get([$1], [cdef_mode]),[2],[1]),
           _AX_Xt_ew_errname([$1]))dnl
dnl
dnl  for cdefine modes 0 and 2,
dnl  we still need to set the global csymbol
dnl
ax_lp_beta([&],
           m4_case(ax_lp_get([$1], [cdef_mode]),
             [0],[[m4_ifval([&4], [[
AS_IF([[$&1do_&3]],[
  AS_VAR_SET([&2&4],[[&5]])
  AC_MSG_NOTICE([[(&6:) &4 = &5]])])]])]],
             [1], [],
             [2], [[[
AS_IF([[$&1do_&3]],[[
  _ax_xt_v="($_ax_xt_v)"]
  AS_VAR_SET([&2&4],[[$_ax_xt_v]])
  AC_MSG_NOTICE([[(&6:) &4 = $_ax_xt_v]])])]]]),

          ax_lp_get([$1], [g_sh_var_], [g_sh_cdef_],
                          [xt_name], [cdef_sym]),
          m4_default_quoted(ax_lp_get([$1], [cdef_val]),[yes]),
          _AX_Xt_ew_errname([$1])))dnl
dnl
m4_append(ax_lp_get([$1], [g_args], [xt_acarg]))])

AX_XT_DEFINE([%%extension],
  [:fnend],
[ax_lp_beta([&], [m4_append([&2], [[
#
#  Arguments for extension &1
#
]])dnl
dnl
m4_append([&3], [[
#
#  Configure &1 extension
#
]])], ax_lp_get([$1], [xt_name], [g_args], [g_configure]))])


# m4_errprint(m4_defn(_ax_lp_lang_prefix([XT])[|%%extension|:fnend]))
