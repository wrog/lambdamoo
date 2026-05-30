# DESCRIPTION
#
#   Framework for declaring LambdaMOO server extensions.
#   (slightly friendlier, thin front end for AX_XT (ax_xt.m4))
#
# EXPORTS
#   Autoconf macros:
#     MOO_XT_DECLARE_EXTENSIONS()
#     MOO_XT_EXTENSION_ARGS
#     MOO_XT_CONFIGURE_EXTENSIONS
#
#   Shell variables:
#     moo_xt_active_xts
#     moo_xt_do_<%%extension name>
#     moo_xt_rquse_<%require name>
#     moo_xt_rqtry_<%require name>
#     moo_d_<cpp identifier for config.h or options.h>
#
# IMPORTS
#     _MOO_OPTIONS_SET
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


#-----------------------------------------------------
#  MOO_XT_DECLARE_EXTENSIONS([
#     <extension declaration script>
#     ],,,,,,,,[ end of extension declarations ])
#
#    Parses the 1st argument into extension declarations
#    For parsing diagnostics to produce the correct line numbers,
#    there should be no linebreaks between the open paren
#    and the 1st argument.  Literal 9th argument is required.
#    Other arguments are ignored (aside from being expanded in the
#    usual manner for M4 macro arguments).
#    Multiple instances of this are allowed.
#
AC_DEFUN([MOO_XT_DECLARE_EXTENSIONS],
  [AC_REQUIRE([AX_XT_INIT])dnl
m4_if([$9],[ end of extension declarations ],[],
    [m4_fatal([bad trailer for $0 ($9)])])dnl
_moo_xt_set_global_state([1], [2])dnl
AX_LP_PARSE_SCRIPT([XT], [
    [g_args],      [_MOO_XT_EXTENSION_ARGS],
    [g_configure], [_MOO_XT_CONFIGURE_EXTENSIONS],
    [g_epilogue],  [_MOO_XT_CONFIGURE_EPILOGUE],
    [g_cdef_set],  [_MOO_OPTIONS_SET],
    [g_sh_var_],   [moo_xt_],
    [g_sh_cdef_],  [moo_d_],
], [$1])])


# These get m4_append()ed as we process the scripts:
m4_define([_MOO_XT_EXTENSION_ARGS])
m4_define([_MOO_XT_CONFIGURE_EXTENSIONS])

# This is m4_defined()ed by each script, but since
# they all overwrite it, only the last one matters
m4_define([_MOO_XT_CONFIGURE_EPILOGUE])



#-----------------------------------------------------
#  MOO_XT_EXTENSION_ARGS
#    outputs accumulated AC_ARG_ENABLE/WITH()s for
#      extension command line arguments, including shell code
#      to determine which extensions are active and
#      do preliminary settings of $moo_d_ variables
#    AC_PRESERVE_HELP_ORDER (intermixed --enables/--withs) is assumed;
#    must occur after the final MOO_XT_DECLARE_EXTENSIONS;
#    must occur before MOO_OPTION_ARG_ENABLES
#      (sets $moo_d_ variables from --enable-def arguments
#       and issues AC_DEFINE()s for options.h symbols)
#
AC_DEFUN([MOO_XT_EXTENSION_ARGS],
  [_moo_xt_set_global_state([2], [0])dnl
# ------------- extension arguments
_$0()
#
# ------------- end of extension arguments
])

#-----------------------------------------------------
#  MOO_XT_CONFIGURE_EXTENSIONS
#    outputs shell code to set makefile variables,
#    set cpp #define symbols for config.h and options.h,
#    and do whichever library searches are needed by the active extensions;
#    must occur after MOO_XT_EXTENSION_ARGS;
#    should occur after all of the mandatory requirements
#    are checked/satisfied;
#    must occur before AC_OUTPUT.
#
AC_DEFUN([MOO_XT_CONFIGURE_EXTENSIONS],
  [_moo_xt_set_global_state([2], [0])dnl
# ------------- extension configurations
_$0()
_MOO_XT_CONFIGURE_EPILOGUE()
#
# ------------- end of extension configurations
])


#-----------
# utilities
#-----------

# A vaguely useful debugging tool (output of this will inevitably
# be better-formatted than what ends up in ./configure)
#
m4_define([MOO_XT_DUMP_EVERYTHING],
[m4_errprint([-------
_MOO_XT_EXTENSION_ARGS:
-------
]m4_defn([_MOO_XT_EXTENSION_ARGS])[
-------
_MOO_XT_CONFIGURE_EXTENSIONS:
-------
]m4_defn([_MOO_XT_CONFIGURE_EXTENSIONS])[
-------
_MOO_XT_CONFIGURE_EPILOGUE:
-------
]m4_defn([_MOO_XT_CONFIGURE_EPILOGUE])
)])


#--------------
# global_state
#--------------

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
