# SYNOPSIS
#
#   # Declare one or more extensions
#   # (best to have this in a separate file [extensions.ac]
#   #  to be m4_included near the top of configure.ac)
#   #
#   MOO_XT_DECLARE_EXTENSIONS([
#     %%extension one
#       <subcmds>...
#     %%extension two
#       <subcmds>...
#   ])
#
#   # Define all --enable/--with arguments associated with extensions
#   # (since this sets moo_d_OPTION shell variables,
#   #  this must be before MOO_OPTION_ARG_ENABLES)
#   #
#   MOO_XT_EXTENSION_ARGS()
#
#   # Do all actual configuration of chosen extensions, e.g.,
#   # directory/include/library checks, setting of makefile and
#   # preprocessor variables, etc...  (should be near the end, after
#   # all of the mandatory requirements are established, probably the
#   # last thing before AC_OUTPUT)
#   #
#   MOO_XT_CONFIGURE_EXTENSIONS()
#
# DESCRIPTION
#
#   This defines and implements the language for
#   specifying extensions in the LambdaMOO server build system.
#
# AUTHOR/COPYRIGHT/LICENSE
#
#   Copyright (C) 2023, 2024  Roger F. Crew
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


#-----------------------------------------------------
# world-visible definitions
#
AC_DEFUN([MOO_XT_DECLARE_EXTENSIONS],
  [_moo_xt_set_global_state([1], [2])dnl
AX_LP_PARSE_SCRIPT([XTL_MOO],
     [[_MOO_XT_EXTENSION_ARGS],[_MOO_XT_CONFIGURE_EXTENSIONS]],
     [$1])])

AC_DEFUN([MOO_XT_EXTENSION_ARGS],
  [_moo_xt_set_global_state([2], [0])
_$0()])

AC_DEFUN([MOO_XT_CONFIGURE_EXTENSIONS],
  [_moo_xt_set_global_state([2], [0])dnl
#
# Configure extensions
#
m4_set_map([_moo_xt_reserved_makevars],[AC_SUBST])
_$0()])

# These get m4_append()ed as we process the scripts:
m4_define([_MOO_XT_EXTENSION_ARGS])
m4_define([_MOO_XT_CONFIGURE_EXTENSIONS])

#--------------
# global_state

# We need to ensure that once either of MOO_XT_EXTENSION_ARGS or
# MOO_XT_CONFIGURE_EXTENSIONS get invoked, no further scripts can be
# parsed.  So, there are three states:
#     [0]  - nothing seen yet
#     [1]  - MOO_XT_DECLARE_EXTENSIONS seen, but nothing else
#     [2]  - after MOO_XT_EXTENSION_ARGS or MOO_XT_CONFIGURE_EXTENSIONS
#
m4_define([_moo_xt_global_state], [0])

# _moo_xt_set_global_state([NEW-STATE],[NOT-ALLOWED])
#
m4_define([_moo_xt_set_global_state],
  [m4_if([_moo_xt_global_state], [$2],
     [AC_MSG_ERROR(m4_case([$2],
       [0], [[[No extensions defined.]]],
       [1], [[[*** THIS SHOULD NOT HAPPEN, report me ***]]],
       [2], [[[Too late for further extension declarations.]]]))])dnl
m4_define([_moo_xt_global_state], [$1])])


#-------------------------------
# makevars

# makevars that are lists of source files for an extension
m4_define([_moo_xt_source_makevars], [[XT_CSRCS], [XT_HDRS]])

# _moo_xt_dist_makevar_<VAR>
#    -> corresponding distribution makevar (ALL_<VAR>)
#
m4_map_args_sep([ax_lp_beta([&],
   [m4_define([_moo_xt_dist_makevar_&1], [ALL_&1])],],
   [)], [], _moo_xt_source_makevars)

# _moo_xt_reserved_makevars =
#    The complete set of reserved makevars
#    that we do substitutions on
#
m4_set_add_all([_moo_xt_reserved_makevars],
  _moo_xt_source_makevars,
  [CPPFLAGS], [XT_LOBJS], [XT_DIRS], [XT_MAKEVARS], [XT_RULES],
  m4_map_args_sep([m4_defn([_moo_xt_dist_makevar_]],
                  [)], [, ],
                  _moo_xt_source_makevars))

# _moo_xt_makevar_sep_<VAR>
#    -> separator to use when adding to <VAR>
#
m4_define([_moo_xt_makevar_sep], ax_lp_NTSC(
  [m4_ifdef([_moo_xt_makevar_sep_$1],
     [m4_defn([_moo_xt_makevar_sep_$1])], [S])]))
m4_define([_moo_xt_makevar_sep_XT_RULES],    ax_lp_NTSC([N]))
m4_define([_moo_xt_makevar_sep_XT_MAKEVARS], ax_lp_NTSC([N]))


# _moo_xtl_add_makevar(<CTX>,<VAR>,<VALUE>)
#    for variables we substitute
#       append [sep][value] to [var]
#    for other variables
#       append [var]=[value] to XT_MAKEVARS
#
m4_define([_moo_xtl_add_makevar],
  [m4_set_contains([_moo_xt_reserved_makevars], [$2],
    [ax_lp_hash_append([$1], [makevars], [$2],
        [$3], _moo_xt_makevar_sep([$2]))],
    [$0([$1], [XT_MAKEVARS], [$2 = $3])])dnl
m4_ifdef([_moo_xt_dist_makevar_$2],
 [ax_lp_append([$1], [dist_$2], [$3], [[ ]])])])

# _moo_xtl_put_makevars(<CTX>,<INDENT>)
#   -> shell code to actually set all of the makevars
#      affected at this level
#
m4_define([_moo_xtl_put_makevars],
  [ax_lp_hash_map_keys_sep([$1], [makevars],
    [m4_pushdef([moo_v],],
    [)
m4_format([[%*s]],[$2])dnl
ax_lp_beta([&],m4_if(m4_defn([moo_v]),[XT_MAKEVARS],
                     [[[&1="$&1${&1+&2}&3"]]], [[[&1=$&1${&1+'&2'}'&3']]]),
       m4_defn([moo_v]),
       _moo_xt_makevar_sep(m4_defn([moo_v])),
       ax_lp_hash_get([$1], [makevars],
                      m4_defn([moo_v])))m4_popdef([moo_v])])])

#-------------------------------
# cdefines

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

# _moo_xtl_cdef_setup(<CTX>,<NAME_ARG>,<VALUE_ARG>)
#    process a %cdefine
m4_define([_moo_xtl_cdef_setup], [m4_do(
  m4_ifval(ax_lp_get([$1], [cdef_sym]),
    [ax_lp_fatal([$1],['%cdefine' again? ])]),

  ax_lp_set_empty([$1], [all_kwds], [],
    [ax_lp_fatal([$1], m4_quote(
      ['%cdefine' must come before any ],
      m4_if(ax_lp_get([$1], [lvl]), [xt],
        [[%option]], [[%lib]])))]),

  m4_ifval([$2],
    [m4_if(m4_bregexp([$2],[%s]),[-1],
      [ax_lp_put([$1], [cdef_sym], [$2])],
      [m4_do(
         ax_lp_put([$1], [cdef_sym], m4_dquote([$2])),
         ax_lp_put([$1], [cdef_mode], [1]))])],
    [ax_lp_fatal([$1],['%cdefine' name required])]),

  m4_ifval([$3],
    [m4_if(ax_lp_get([$1], [cdef_mode]),[1],
      [ax_lp_fatal([$1],['%cdefine name%s value' not allowed])],
      [m4_if(m4_bregexp([$3],[%s]),[-1],
        [ax_lp_put([$1], [cdef_val], [$3])],
        [m4_do(
           ax_lp_put([$1], [cdef_val], m4_dquote([$3])),
           ax_lp_put([$1], [cdef_mode], [2]))])])]),

  m4_if(ax_lp_get([$1], [cdef_mode]),[0],
    [ax_lp_set_add([$1], [all_cdefs], [$2])]))])

# when encountering   %option/%lib <NAME> <KEYWORD>?
# (<CTX>, <NAME>, <KEYWORD>)
#   -> _moo_xtl_add_cdef2(<CTX>, <CPPSYM>, <CPPVALUE>)
m4_define([_moo_xtl_add_cdef],
  [m4_case(ax_lp_get([$1], [cdef_mode]),
    [2], [_moo_xtl_add_cdef2([$1],
              ax_lp_get([$1], [cdef_sym]),
              m4_format(ax_lp_get([$1], [cdef_val]),
                 m4_default_quoted([$3], m4_toupper([$2]))))],
    [1], [_moo_xtl_add_cdef2([$1],
              m4_format(ax_lp_get([$1], [cdef_sym]),
                        m4_default_quoted([$3],m4_toupper([$2]))))],
    [0], [m4_ifval([$3], [_moo_xtl_add_cdef2([$1], [$3])])],
    [m4_fatal([cant happen (cdef_mode = ]ax_lp_get([$1], [cdef_mode])[)])])])

m4_define([_moo_xtl_add_cdef2],[m4_do(
  ax_lp_put([$1],     [this_cdef], [$2]),
  ax_lp_set_add([$1], [all_cdefs], [$2]),
  ax_lp_put([$1],     [this_cval], [$3]))])



#=======================
# The XTL language
#=======================

AX_LP_DEFINE_LANGUAGE([XTL_MOO],[MOO_XTL_DEFINE])

#----------
# root cmd
#----------

MOO_XTL_DEFINE([],
  [:var],  [[g_args],      [$2]],
  [:var],  [[g_configure], [$3]],
  [:vars], [m4_map_args_sep([[dist_]],[],[,],_moo_xt_source_makevars())],
  [:fnend],
    [m4_append(ax_lp_get([$1], [g_configure]),
      m4_map_args_sep(
       [ax_lp_beta([&],
          ax_lp_NTSC([[N[&1='&2']]]),
          ax_lp_beta([&],
            [m4_defn([_moo_xt_dist_makevar_&1]), ax_lp_get([$1], [dist_&1])],],
       [))],
       [], _moo_xt_source_makevars()))])

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

MOO_XTL_DEFINE([%%extension],
  [:parent], [],
  [:subcmds],[[%disabled], [--enable-], [%cdefine],
              [%option], [%option_set]],
  [:vars],   [[xt_acarg], [xt_cases], [xt_disabled],
              [ew], [ew_var], [ew_name], [ew_vdesc],
              [help], [cdef_sym], [cdef_val],
              [reqs], [req2s]],
  [:var],    [[cdef_mode], [0]],
  [:var],    [[xt_name],  [$2]],
  [:var],    [[lvl],      [xt]],
  [:sets],   [[all_kwds], [all_options], [all_cdefs]],

  [:subcmds],[[=]],
  [:hashes], [[makevars]])

MOO_XTL_DEFINE([%disabled],
  [:fn], [ax_lp_put([$1], [xt_disabled], [1])])


MOO_XTL_DEFINE([=],
  [:fn], [_moo_xtl_add_makevar([$1],[$2],[$3])])


MOO_XTL_DEFINE([--enable-],
  [:subcmds], [[%?], [%?-]],
  [:args],
    [m4_do(m4_split(m4_translit([[$2]],[=],[ ])))],
  [:fn],
    [_moo_xtl_ew_defns([$1], [enable], [$2], [$3])])

MOO_XTL_DEFINE([--with-],
  [:subcmds], [[%?], [%?*]],
  [:args],
    [m4_do(m4_split(m4_translit([[$2]],[=],[ ])))],
  [:fn],
    [_moo_xtl_ew_defns([$1], [with], [$2], [$3])])

m4_define([_moo_xtl_ew_defns],
  [ax_lp_put([$1], [ew], [$2])dnl
ax_lp_put([$1], [ew_name], [$3])dnl
ax_lp_put([$1], [ew_vdesc], [$4])dnl
ax_lp_put([$1], [ew_var], m4_translit([[$2-$3]],[-+.],[___]))dnl
ax_lp_ifdef([$1], ax_lp_get([$1], [lvl])[_ew_name],
  [ax_lp_put([$1], ax_lp_get([$1], [lvl])[_ew], ax_lp_get([$1], [ew]))dnl
ax_lp_put([$1], ax_lp_get([$1], [lvl])[_ew_name], ax_lp_get([$1], [ew_name]))dnl
ax_lp_put([$1], ax_lp_get([$1], [lvl])[_ew_var], ax_lp_get([$1], [ew_var]))dnl
])])



MOO_XTL_DEFINE([%?],[:fn], ax_lp_NTSC(
  [m4_ifval(
    ax_lp_ifdef([$1], [kwd_select],
                [m4_if(ax_lp_get([$1], [lvl]),[bld],[],[1])]),
    [ax_lp_append([$1], [help],
       m4_format([[[N%22s: . . %s]]],
         ax_lp_get([$1], [kwd_select]), [$2]))],
    [_moo_xtl_addhelp([$1],
       ax_lp_get([$1], [ew]),
       ax_lp_get([$1], [ew_vdesc]),
       [$2])])]))

MOO_XTL_DEFINE([%?-],[:fn],
  [_moo_xtl_addhelp([$1],
     m4_if(ax_lp_get([$1], [ew]),[enable],[[disable]],[[without]]),
     [],[$2])])

MOO_XTL_DEFINE([%?*],[:fn],
  [ax_lp_append([$1], [help],
    [m4_newline()AS_HELP_STRING([],[$2],[28])])])

m4_define([_moo_xtl_addhelp],
  [ax_lp_append([$1], [help],
    m4_format([[%sAS_HELP_STRING([--$2-%s%s],[%s])]],
      m4_ifval(ax_lp_get([$1], [help]), m4_newline()),
      ax_lp_get([$1], [ew_name]),m4_ifval([$3],[[=$3]]),[$4]))])



MOO_XTL_DEFINE([%cdefine],
  [:args], [m4_unquote(m4_split([$2]))],
  [:fn],
    [_moo_xtl_cdef_setup($@)])


MOO_XTL_DEFINE([%option],
  [:subcmds], [[%implies], [%?], [%alt]],

  [:args], [m4_unquote(m4_split([$2]))],
  [:sets], [[opt_implies]],
  [:var],  [[opt_name],  [$2]],
  [:var],  [[kwd_select],[$2]],

  [:fn], [m4_do(
    m4_ifval(ax_lp_get([$1], [ew_name]),[],
      [ax_lp_fatal([$1],
        [need an '--enable' to have '%option's])]),
    _moo_xtl_set_require_new([$1], [all_kwds], [$2]),
    ax_lp_set_add([$1], [all_options],[$2]))],

  [:vars], [[this_cdef], [this_cval]],
  [:fn], [_moo_xtl_add_cdef([$1],[$2],[$3])])


MOO_XTL_DEFINE([%alt],
  [:fn], [m4_do(
    _moo_xtl_set_require_new([$1], [all_kwds], [$2]),
    ax_lp_put([$1], [kwd_select],
      [$2|]ax_lp_get([$1], [kwd_select])))])


MOO_XTL_DEFINE([%implies],
  [:fn], [m4_do(
    _moo_xtl_set_require_prev([$1], [all_kwds], [$2]),
    ax_lp_set_add([$1], [opt_implies], [$2]))])


MOO_XTL_DEFINE([%option_set],
  [:args], ax_lp_NTSC(
    [m4_if(m4_bregexp([$2],[^[^TS]+[TS]*=]),[-1],
      [ax_lp_fatal([$1],[= expected])],
      [m4_bpatsubst([[$2]],
        [^.\([^TS]+\)[TS]*=[TS]*\(.*\).$],
        [[\1],m4_do(m4_split([\2],[,[TS]*]))])])]),

  [:sets], [[os_mems]],
  [:var],  [[os_name], [$2]],

  [:fn], [m4_do(
    m4_ifval(ax_lp_get([$1], [ew_name]),[],
      [ax_lp_fatal([$1],
         [need '--enable' to have '%option_set'])]),
    m4_map_args_sep(
      [_moo_xtl_set_require_new([$1], [all_kwds],], [)], [],
      m4_do(m4_split([$2],[|]))),
    m4_map_args_sep(
      [_moo_xtl_set_require_prev([$1], [all_kwds],], [)], [],
      m4_shift2($@)),
    ax_lp_set_add_all([$1], [os_mems], m4_shift2($@)))],

  [:fnend], [m4_do(
     ax_lp_append([$1], [help],
       m4_format(ax_lp_NTSC([[[N%22s: . . %s]]]),
         ax_lp_get([$1], [os_name]),
         ax_lp_set_map_sep([$1], [os_mems],[],[],[[+]]))),
     ax_lp_append([$1], [xt_cases],
       _moo_xtl_xtset_cases([$1],
          ax_lp_get([$1], [os_name]), [os_mems])))])

m4_define([_moo_xtl_xtset_cases],
  [ax_lp_beta([&], [[,[[
      $2]], [[
        moo_ll="&1&2"
        continue]]]],
                m4_if([$2],[yes],[[__done__]],[[$moo_ll]]),
                ax_lp_set_map_sep([$1], [$3], [[,]]))])


MOO_XTL_DEFINE([%require],
  [:parent],  [%%extension],
  [:subcmds], [[--with-], [%cdefine], [%ac_yes]],

  [:sets], [[all_kwds], [all_cdefs]],
  [:vars], [[ew_name],[ew_var],[ew_vdesc],[yescode],
            [rq_ew],[rq_ew_name],[rq_ew_var],[rq_acarg],
            [all_libs],[cdef_sym],[cdef_val],
            [icases],[scases],[ucases],
            [help],[blds]],
  [:var],  [[cdef_mode], [0]],
  [:var],  [[rq_name],  [$2]],
  [:var],  [[ew],     [with]],
  [:var],  [[lvl],      [rq]])


# _moo_xtl_with_arg([ctx])
#   -> AC_ARG_WITH for %require or %build
#
m4_define([_moo_xtl_with_arg],
  [m4_ifval(ax_lp_get([$1], [ew_name]),
     [ax_lp_beta([&], m4_format([[[
AC_ARG_WITH([&2],
  [&3],
[AS_CASE([$withval],[[
   no]],[%s],[
     AS_IF([[$_moo_xt_saw_no]],[
       AC_MSG_ERROR([[--with-&2 $_moo_xt_conflict]])])[
     moo_xt_do_&1=:
     _moo_xt_conflict="conflicts with --with-&2"]])])]]],

  m4_if(ax_lp_get([$1], [lvl]), [bld], [[
     AC_MSG_ERROR([[--without-&2 is meaningless]])]],
[[
     AS_IF([[$moo_xt_do_&1]],[
       AC_MSG_ERROR([[--without-&2 $_moo_xt_conflict]])])[
     _moo_xt_saw_no=:
     _moo_xt_conflict="conflicts with --without-&2"]]])),

                 ax_lp_get([$1], [xt_name], [ew_name], [help]))])])


MOO_XTL_DEFINE([%lib],
  [:parent], [%require],
  [:subcmds],[[%ac], [%alt]],
  [:args],   [m4_unquote(m4_split([$2]))],
  [:vars],   [[code]],
  [:var],    [[lvl],      [lib]],
  [:var],    [[lib_name],  [$2]],
  [:var],    [[kwd_select],[$2]],

  [:subcmds],[[=]],
  [:hashes], [[makevars]],

  [:fn], [m4_do(
    ax_lp_append([$1], [all_libs], [$2], [[ ]]),
    _moo_xtl_set_require_new([$1], [all_kwds], [$2]))],

  [:vars], [[this_cdef], [this_cval]],
  [:fn], [_moo_xtl_add_cdef([$1],[$2],[$3])])


MOO_XTL_DEFINE([%build],
  [:parent],  [%lib],
  [:subcmds], [[--with-]],

  [:vars],   [[ew_name],[ew_var],[ew_vdesc],[help]],
  [:var],    [[ew], [with]],
  [:var],    [[lvl], [bld]],

  [:subcmds],[[=], [%make], [%dirvar], [%path]],
  [:vars],   [[dirvar], [path]],
  [:hashes], [[makevars]])


MOO_XTL_DEFINE([%dirvar],
  [:fn], [m4_do(
     m4_ifval(ax_lp_get([$1], [dirvar]),
       [ax_lp_fatal([$1], [second %dirvar for this %build?])]),
     m4_set_contains([_moo_xt_reserved_makevars], [$2],
       [ax_lp_fatal([$1], [$2 cannot be used as %dirvar])]),
     ax_lp_put([$1], [dirvar], [$2]),
     _moo_xtl_add_makevar([$1], [$2], [$moo_xtdir]),
     _moo_xtl_add_makevar([$1], [XT_DIRS], [$($2)]))])

MOO_XTL_DEFINE([%path],
  [:fn], [m4_do(
    m4_ifval(ax_lp_get([$1], [path]),
       [ax_lp_fatal([$1],[second %path in this %build?])]),
    m4_ifval([$2],
       [ax_lp_put([$1], [path], [$2])],
       [ax_lp_fatal([$1],[%path directory expected])]))])

MOO_XTL_DEFINE([%make],
  [:args],
    [_moo_xtl_trimmake([$2])],
  [:fn],
    [_moo_xtl_add_makevar([$1],[XT_RULES],[$2])])

m4_define([_moo_xtl_trimmake], ax_lp_NTSC(
 [m4_bpatsubsts([[$1]],
  [\`\(..\)\([ST]*N\)+],[\1],
  [\([^N]\)\([ST]*N\)+\(..\)\'],[\1N\3],
  [^\(T.*\)N\(..\)\'],[\1NN\2])]))



MOO_XTL_DEFINE([%ac],
  [:fn],
    [ax_lp_put([$1], [code], [$2])])

MOO_XTL_DEFINE([%ac_yes],
  [:fn],
    [ax_lp_put([$1], [yescode], [$2])])


m4_define([_moo_xtl_set_require_new],
  [ax_lp_set_add([$1], [$2], [$3], [],
    [ax_lp_fatal([$1],[keyword '$3' already defined])])])

m4_define([_moo_xtl_set_require_prev],
  [ax_lp_set_contains([$1], [$2], [$3], [],
    [ax_lp_fatal([$1],[unknown keyword: '$3'])])])


MOO_XTL_DEFINE([%option],
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
        AS_CASE([$moo_v'|'],[[
          *"|&2|"*]],[[
            continue]],[[
            moo_v="$moo_v|&2"]])]]],
[[[[
        moo_d=&1]
        AS_VAR_SET_IF([moo_d_$moo_d],[[continue]],[[moo_v=&2]])]]]),

                ax_lp_get([$1], [this_cdef]),
                m4_default_quoted(ax_lp_get([$1], [this_cval]),[yes]))]),

          ax_lp_set_empty([$1], [opt_implies],
            [], [m4_format(
[[[
        moo_ll="$moo_ll%s"]]],
                   ax_lp_set_map_sep([$1], [opt_implies],[[,]]))]),

          m4_ifval(ax_lp_get([$1], [this_cdef]),
            [],
[[[
        continue]]])))))])


MOO_XTL_DEFINE([%build],
  [:fnend],
  [m4_ifval(ax_lp_get([$1], [dirvar]),[],
       [ax_lp_fatal2([$1],[%dirvar required],[... by %build])])dnl
ax_lp_beta([&],
  [m4_ifval([&1],
    [m4_ifval([&2],
      [ax_lp_fatal([cannot have --with- and %path in the same %build])])],
    [m4_ifval([&2],[],
      [ax_lp_fatal([%build requires one of --with- or %path])])])],
           ax_lp_get([$1],[ew_name],[path]))dnl
ax_lp_append([$1], [rq_acarg], _moo_xtl_with_arg([$1]))dnl
ax_lp_append([$1], [blds],
          ax_lp_beta([&], [[
AS_CASE([x$&6],[[
  x|xno]],[],[[
    moo_xt_rquse_&1=&5]
    AS_CASE([[x$&2]],[[
      x|xyes|x&5]],[],
      [AC_MSG_ERROR([[--&8-&7=DIR vs. --&4-&3=$&2]])])
    AS_SET_CATFILE([moo_xtdir],[$srcdir],[$&6])
    AS_IF([[test -d "$moo_xtdir"]],[],[
      AC_MSG_ERROR([[directory not found: $moo_xtdir]])])
    AS_SET_CATFILE([moo_xtdir],['$(abs_srcdir)'],[$&6])[&9]])]],

                     ax_lp_get([$1],[rq_name],[rq_ew_var],[rq_ew_name],[rq_ew],
                                    [lib_name],[ew_var],[ew_name],[ew]),
                     _moo_xtl_put_makevars([$1],[4])))])

MOO_XTL_DEFINE([%lib],
   [:fnend],
   [ax_lp_append([$1], [icases], ax_lp_beta([&],
[[,[[
      &1]],  [[
        moo_xt_rqtry_&2="$moo_xt_rqtry_&2 &3"]]]],

      ax_lp_get([$1], [kwd_select]), ax_lp_get([$1], [rq_name]),
      ax_lp_get([$1], [lib_name])))dnl
ax_lp_append([$1], [scases],
  ax_lp_beta([&],
    m4_ifval(ax_lp_get([$1], [code]),
[[[,[[
      &3]],  [
        &1
        AS_IF([[$_moo_xt_found_it_]],[[
          moo_xt_rquse_&2=&3
          break]])]]]],
[[[,[[
      &3]],  [
        _moo_xt_found_it_=:
        moo_xt_rquse_&2=&3
        break]]]]),

             m4_bpatsubsts(m4_dquote(ax_lp_get([$1], [code])),
               [%%],[_moo_xt_found_it_=:],
               [%!],[_moo_lfail1]),
             ax_lp_get([$1],[rq_name],[lib_name])))dnl
dnl
ax_lp_append([$1], [ucases],
  ax_lp_beta([&], m4_format(
[[[,[[
  x&1]],  [%s&4]]]], m4_ifval(ax_lp_get([$1], [this_cdef]),[[
    AC_DEFINE([&2], [&3])]])),

             ax_lp_get([$1],[lib_name],[this_cdef]),
             m4_default_quoted(ax_lp_get([$1], [this_cval]),[1]),
             _moo_xtl_put_makevars([$1],[4])))])


MOO_XTL_DEFINE([%require],
  [:fnend],
  [ax_lp_put([$1], [xt_acarg],
      ax_lp_get([$1], [xt_acarg])_moo_xtl_with_arg([$1])ax_lp_get([$1], [rq_acarg]))dnl
m4_ifval(ax_lp_get([$1], [yescode]),[],
  [ax_lp_put([$1], [yescode], ["]ax_lp_get([$1], [all_libs])["])])dnl
m4_if(m4_bregexp(ax_lp_get([$1], [yescode]),[%%]),[-1],
  [ax_lp_put([$1], [yescode], m4_dquote([%%=]ax_lp_get([$1], [yescode])))])dnl
ax_lp_put([$1], [yescode],
  m4_bpatsubst(m4_dquote(ax_lp_get([$1], [yescode])),[%%],[moo_xt_rqtry_]ax_lp_get([$1], [rq_name])))dnl
dnl
ax_lp_append([$1], [reqs],
  ax_lp_beta([&], m4_format(
[[[[
moo_xt_rquse_&2=]&6[
moo_l=%s
moo_xt_rqtry_&2=]
AS_IF([[test x$moo_xt_rquse_&2 = x]],[
  AS_IF([[test x$moo_l = x]],[
    AS_IF([[$moo_xt_do_&1]],[[moo_l=yes]],[[moo_l=no]])])[
  ac_save_IFS=$IFS
  IFS=,
  for moo_kwd in $moo_l ; do
    IFS=$ac_save_IFS]
    AS_CASE([$moo_kwd]&7,[[
      no]],  [[
        moo_xt_rqtry_&2=
        break]],[[
      yes]], [
        &8[
        break]],[
      AC_MSG_ERROR([[unknown --&5-&4 keyword: $moo_kwd]])])[
  done]])]]],
               m4_ifval(ax_lp_get([$1], [ew_name]),[[$&3]])),

             ax_lp_get([$1],[xt_name],[rq_name],
                            [ew_var],[ew_name],[ew],
                            [blds],[icases],[yescode])))dnl
dnl
ax_lp_append([$1], [req2s],
  ax_lp_beta([&],

[[
AS_IF([[test "x$moo_xt_rqtry_&2" != x]],[[
  _moo_xt_found_it_=false
  _moo_lfail_extras=
  for moo_lib in $moo_xt_rqtry_&2 ; do
    _moo_lfail1=]
    AS_CASE([$moo_lib]&4,[
        AC_MSG_ERROR([[?? \$moo_xt_rqtry_&2 kwd = $moo_lib]])])
    AS_IF([[test "x$_moo_lfail1" != x]],[[
      _moo_lfail_extras="$_moo_lfail_extras
  $moo_lib: $_moo_lfail1"]])[
  done]
  AS_IF([[$_moo_xt_found_it_]],[],[
    AC_MSG_ERROR([[--with-&3: no library found$_moo_lfail_extras]])])])
AS_CASE([x$moo_xt_rquse_&2],[[
  x]],[]&5,[
    AC_MSG_ERROR([[?? \$moo_xt_rquse_&2 = $moo_xt_rquse_&2]])])]],

             ax_lp_get([$1],[xt_name],[rq_name],[rq_ew_name],
                            [scases],[ucases])))])

MOO_XTL_DEFINE([%%extension],
  [:fnend],
  [m4_ifval(ax_lp_get([$1], [ew_name]),

    m4_if(
    # set up an --enable-NAME argument
    #
    )[ax_lp_set_contains([$1], [all_kwds], [yes], [],
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
            [_moo_xtl_xtset_cases([$1], [yes], [all_options])]))])dnl
dnl
dnl fix xt_cases:
dnl provide 'no' case
ax_lp_append([$1], [xt_cases], m4_format([[,[[
      no]], [[
        moo_ll=__done__%s
        break]]]],
        m4_quote(ax_lp_set_map_sep([$1], [all_cdefs], [
        moo_d_], [=no]))))dnl
dnl
dnl build xt_acarg from...:
ax_lp_put([$1], [xt_acarg],
  ax_lp_beta([&],
dnl
dnl   AC_ARG_ENABLE for this extension
[[[
moo_xt_do_&1=false
_moo_xt_saw_no=false
_moo_xt_conflict=]
AC_ARG_ENABLE([&2],
  [&4],
[AS_CASE([$enableval],[[
   no]],[[
     _moo_xt_saw_no=:
     _moo_xt_conflict="conflicts with --disable-&2"]],[[
     moo_xt_do_&1=:
     _moo_xt_conflict="conflicts with --enable-&2"]])])]],

             ax_lp_get([$1],[xt_name],[ew_name],[ew_var],[help]))dnl
dnl
dnl   Prior AC_ARG_WITHs from %require segments
dnl
ax_lp_get([$1],[xt_acarg])dnl
dnl
dnl   At this point explicit --enable/--disable/--with/--without
dnl   are agreed on yes vs. no.  But there might not be any at all.
dnl   Make sure moo_xt_do_NAME is true iff extension is active.
dnl
ax_lp_beta([&],
  m4_ifval(ax_lp_get([$1], [xt_disabled]),
    [[[
AS_IF([[$moo_xt_do_&1]],[[
  moo_l=$&3]
  AS_IF([[test "x$moo_l" = x]],[[
    moo_l=yes]])],[[
  moo_l=no]])]]],

    [[[
AS_IF([[$_moo_xt_saw_no]],[[
  moo_l=no]],[[
  moo_xt_do_&1=:
  moo_l=$&3]
  AS_IF([[test "x$moo_l" = x]],[[moo_l=yes]])])]]]),

           ax_lp_get([$1],[xt_name],[ew_name],[ew_var]))dnl
dnl
dnl   process xt_cases
dnl
ax_lp_beta([&],[[[&5
while test "$moo_l" != __done__ ; do
  ac_save_IFS=$IFS
  IFS=,
  moo_ll=__done__
  for moo_kwd in $moo_l ; do
    IFS=$ac_save_IFS]
    AS_CASE([$moo_kwd],[[
      __done__]],[[
        continue]]&4,[
        AC_MSG_ERROR([[unknown --enable-&2 keyword: $moo_kwd]])])&6[
  done
  moo_l=$moo_ll
done]]],

           ax_lp_get([$1],[xt_name],[ew_name],[ew_var],[xt_cases]),
           m4_if(ax_lp_get([$1], [cdef_mode]),[2],[[
moo_v=0]]),
           m4_if(ax_lp_get([$1], [cdef_mode]),[2],[],
              [ax_lp_beta([&],[[
    AS_VAR_SET([moo_d_$moo_d],[[$moo_v]])
    AC_MSG_NOTICE([[(--enable-&2:) $moo_d = $moo_v]])]],

                          ax_lp_get([$1],[xt_name],[ew_name]))]))dnl
dnl
dnl   remaining things to do if extension is active:
dnl     set global moo_d_OPTION csymbol
dnl     set make variables
dnl
ax_lp_beta([&],[[
AS_IF([[$moo_xt_do_&1]],[&2])
]],
        ax_lp_get([$1], [xt_name]),
        ax_lp_beta([&],

          m4_case(ax_lp_get([$1], [cdef_mode]),
            [0],[m4_ifval(ax_lp_get([$1], [cdef_sym]), [[[
  AS_VAR_SET([moo_d_&4],[[&5]])
  AC_MSG_NOTICE([[(--enable-&2:) &4 = &5]])]]])],
            [1], [],
            [2], [[[[
  moo_v="($moo_v)"]
  AS_VAR_SET([moo_d_&4],[[$moo_v]])
  AC_MSG_NOTICE([[(--enable-&2:) %s = $moo_v]])]]]),

          ax_lp_get([$1],[xt_name],[ew_name],[ew_var],[cdef_sym]),
          m4_default_quoted(ax_lp_get([$1], [cdef_val]),[yes]))dnl
m4_dquote(_moo_xtl_put_makevars([$1],[2]))))],

    m4_if(
    # what to do if no --enable-NAME argument is set up.
    #   extension is automatically active.
    #   --without'ing %required things is forbidden.
    #   no xt_cases, no moo_d_OPTION_NAME variables to set.
    #
    )[ax_lp_put([$1], [xt_acarg],
                ax_lp_beta([&],[[[
moo_xt_do_&1=:
_moo_xt_saw_no=false
_moo_xt_conflict="not allowed"]&2]],

                           ax_lp_get([$1],[xt_name],[xt_acarg])dnl
_moo_xtl_put_makevars([$1],[0])))])dnl
dnl
m4_append(ax_lp_get([$1], [g_args]),
  ax_lp_beta([&], [[
#
#  Arguments for extension &1
#
&2]], ax_lp_get([$1], [xt_name], [xt_acarg])))dnl
m4_append(ax_lp_get([$1], [g_configure]),
  ax_lp_beta([&], [[
#
#  Configure &1 extension
#
&2&3]], ax_lp_get([$1],[xt_name],[reqs],[req2s])))])
