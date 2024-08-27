# ax_lp.m4  -- parser for autoconf/m4-based domain-specific language
# ===================================================================
#
# SYNOPSIS
#
#   AX_LP_DEFINE_LANGUAGE(<LANGUAGE>, <DEFINE_CMD>)
#
#   <DEFINE_CMD>( <CMD1>,
#      [:args,    <ARGPARSE_BODY>,]
#      [:allow,   <ALLOW_BODY>,   ]
#      [:parent,  <CMD0>,         |
#       :subcmds, <SUBCMD_LIST>,  |
#       :vars,    <VAR_LIST>,     |
#       :var,     <VAR+VALUE>,    |
#       :sets,    <SET_LIST>,     |
#       :hashes,  <HASH_LIST>,    |
#       :fn,      <FN_INIT_BODY>, |
#       :fnend,   <FN_END_BODY>,  ]* )
#     ...
#   AX_LP_PARSE_SCRIPT( <LANGUAGE>, [<INITARGS>...], <SCRIPT> )
#
#   Macros for use within :allow/:args/:fn/:fnend BODY hooks:
#
#     ax_lp_get                 ax_lp_set_empty
#     ax_lp_put                 ax_lp_set_add
#     ax_lp_append              ax_lp_set_add_all
#     ax_lp_ifdef               ax_lp_set_contains
#                               ax_lp_set_size
#     ax_lp_hash_empty          ax_lp_set_map_sep
#     ax_lp_hash_size
#     ax_lp_hash_has_key        ax_lp_cmd
#     ax_lp_hash_get            ax_lp_parent_cmd
#     ax_lp_hash_put            ax_lp_level
#     ax_lp_hash_append         ax_lp_fatal
#     ax_lp_hash_map_keys_sep   ax_lp_fatal2
#
# DESCRIPTION
#
#   This provides a parser for a simple, user-defined, domain-specific
#   command/directive/whatever (hereafter, "cmd") language, a means by
#   which small scripts in such languages can be incorporated into a
#   configure.ac, thus providing (we hope) a more readable/editable DSL
#   format for aspects of the configuration that someone lacking deep
#   knowledge of m4/autoconf syntax/internals may find easier to deal
#   with.
#
#   The parser is fully re-entrant, i.e., multiple parsing instances
#   in the same or multiple languages may be active at the same time
#   with no ill effects.
#
#   AX_LP_DEFINE_LANGUAGE( <LANGUAGE>, <DEFINE_CMD>, <GLOBAL_HOOKS>... )
#   establishes a language definition with <DEFINE_CMD> as a macro to
#   define individual cmds within the language, including a "root" cmd
#   (named []) that is deemed to encompass the entire script.
#
#   AX_LP_PARSE_SCRIPT( <LANGUAGE>, <INITARGS>, <SCRIPT>)
#   with <LANGUAGE> having been defined, []:fn(<INITARGS>) is
#   invoked and then <SCRIPT> is parsed line by line, invoking the
#   various cmd hooks, and finally []:fnend is invoked.
#
#   Each line in a script is expected to contain a cmd word and an
#   argument string, except blank lines and #-comments are ignored.
#   Script lines are quoted internally so that no expansions of that
#   text will occur other than as may be called for in the
#   user-supplied hooks.
#
#   The expansion of an AX_LP_PARSE_SCRIPT instance is whatever the
#   various :fn and :fnend hooks expand to, in the order that the
#   respective cmds are encountered (the order of expansion is
#   deterministic but perhaps not the most convenient; you may find
#   it easier to have all hooks expand to nothing and produce all
#   output via diversions or side-effects)
#
#   Cmds in a script form a hierarchy given by their relative
#   indentation (like Python), and there are facilities for
#   establishing local environments of definitions that remain in
#   effect throughout the processing of (child) subcmds.
#
#   Script lines ending with a heredoc marker (<<ENDMARK) are taken to
#   include all subsequent lines up to (but not including) the next
#   ENDMARK (on a line by itself) as a single "logical line" that
#   is then part of the argument string for the cmd being interpreted.
#
#   The underlying parser has limitations inherited from M4,
#   most notably that each individual cmd line in a script must be
#   quote-balanced ([]) (but heredocs need only be quote-balanced
#   overall, i.e., there is no balance requirement on the individual
#   lines within a heredoc)
#
# COPYRIGHT/LICENSE
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


#-----------------------------------------------------------------
# Utilities for defining macros (that, hence, need to come first)

# ax_lp_NTSC(<DEFN>) -> [<translited DEFN>]
#
# for when one needs to make (N)ewline, (T)ab, (S)pace,
# and #(C)omment readable, and N,T,S,C are otherwise
# unused in a macro definition
m4_define([ax_lp_NTSC], [m4_translit([[$1]],[NTSC],[
	 # this is not a comment (this line begins with <tab><space>)
])])

# ax_lp_beta(<A>,<DEFN>,<ARGS>*)
#  -> <DEFN{A\$}>(<ARGS>*)
#
# Does beta substitution.  <A> must be a single character.
# Substitute $ for <A> in <DEFN>, use that to temporarily
# define a macro, then immediately invoke it on <ARGS>*
#
m4_define([ax_lp_beta],
  [m4_pushdef([_lambda_],m4_translit([[$2]],[$1],[$]))dnl
_lambda_(m4_shift2($@)m4_popdef([_lambda_]))])

#-------------------------------------------------------
# AX_LP_PARSE_SCRIPT( <LANGUAGE>, <INITARGS>, <SCRIPT>)

AC_DEFUN([AX_LP_PARSE_SCRIPT],
  [_$0(_ax_lp_new_context(), $@)])

# _AX_LP_PARSE_SCRIPT( <CTX>, <LANGUAGE>, <INITARGS>, <SCRIPT>)
#
m4_define([_AX_LP_PARSE_SCRIPT],
[_ax_lp_init([$1], _ax_lp_lang_prefix([$2]), [$4])dnl
_ax_lp_call([$1],[],[:fn],[ :fn], $3)[]dnl
_ax_lp_parse_lines([$1])dnl
_ax_lp_call([$1],[],[:fnend],[ :fnend])[]dnl
_ax_lp_finalize([$1])])

m4_define([_ax_lp_new_context],
  [m4_define([_ax_lp_context_counter],
     m4_incr(m4_defn([_ax_lp_context_counter])))dnl
[_ax_lp__pctx_]_ax_lp_context_counter()])

m4_define([_ax_lp_context_counter], [0])

#---------------------------------------------
# parser state

# _ax_lp_init(<CTX>,<LANGUAGE>,<LINES>)
#    initialize <CTX> for a particular <LANGUAGE>
#      (sets <CTX>__p_{lang,file,lnum,top,cmd0,ind0})
#    and load up <LINES> to be parsed
#      (see _ax_lp_parse_lines for additional settings)
#
m4_define([_ax_lp_init],
[m4_define([$1__p_lang],  [$2])dnl              = LANGUAGE prefix
m4_define( [$1__p_file],  __file__)dnl          = current file name
m4_define( [$1__p_lnum],  m4_decr(__line__))dnl = current line number
m4_define( [$1__p_top],   [0])dnl               = current level number
m4_define( [$1__p_cmd0],  [])dnl           cmdN = cmd at level N
m4_define( [$1__p_ind0],  [-1,0])dnl       indN = level N indentation
_ax_lp_next_heredoc([$1],m4_newline()[$3])dnl     load <LINES>
dnl sets $1__p_lrest, $1__p_ltail, $1__p_lskip
_ax_lp_nextline([$1])])dnl sets $1__p_line1

# _ax_lp_finalize(<CTX>)
#    (in which we try not to have memory leaks.)
#    clean up <CTX> when we are done.
#
m4_define([_ax_lp_finalize],
[m4_foreach([_ax_lp_pv],[[lang],[file],[lnum],
            [top],[cmd0],[ind0],[lrest],[ltail],[lskip],[line1]],
  [m4_format([m4_undefine([$1__p_%s])],m4_defn([_ax_lp_pv]))])])


#---------------------------------------------
# stack management

# _ax_lp_push(<CTX>,<INDENTATION>,<CMD>)
#   assuming the current frame is for the parent of <CMD>
#   push a frame for <CMD>
#
m4_define([_ax_lp_push],
[m4_pushdef([_ax_lp_n],m4_incr($1__p_top))dnl
m4_pushdef( [$1__p_ln1],$1__p_lnum)dnl
m4_define(  [$1__p_top],_ax_lp_n)dnl
m4_define(  [$1__p_ind]_ax_lp_n,[$2])dnl
m4_define(  [$1__p_cmd]_ax_lp_n,[$3])dnl
m4_popdef(  [_ax_lp_n])])

# _ax_lp_pop(<CTX>)
#   invoke the current cmd's :fnend hook
#   (as if we were on the previous line),
#   then pop the current frame
#
m4_define([_ax_lp_pop],
[m4_pushdef([$1__p_lnum],m4_decr($1__p_lnum))dnl
_ax_lp_call([$1],m4_defn([$1__p_cmd]$1__p_top),[:fnend],[ :fnend])dnl
m4_popdef([$1__p_lnum])dnl
m4_pushdef([_ax_lp_n],$1__p_top)dnl
m4_undefine([$1__p_ind]_ax_lp_n)dnl
m4_undefine([$1__p_cmd]_ax_lp_n)dnl
m4_define(  [$1__p_top],m4_decr(_ax_lp_n))dnl
m4_popdef(  [$1__p_ln1])dnl
m4_popdef(  [_ax_lp_n])])

# _ax_lp_unwind(<CTX>,<INDENTATION>,<PREV_INDENT_CMP>)
#   Pop frames until we get to one indented less than <INDENTATION>,
#   If we are starting at frames indented more than <INDENTATION>,
#   then it is an error for there not to be a prior sibling
#   with the *same* indentation.
#
m4_define([_ax_lp_unwind],
[m4_pushdef([_ax_lp_pc],
  m4_list_cmp([$2], m4_defn([$1__p_ind]$1__p_top)))dnl
m4_if(_ax_lp_pc,[1],
  [m4_popdef([_ax_lp_pc])m4_if([$3],[-1],
     [ax_lp_fatal2([$1],
       [indentation does not match ...],
       [... prior sibling?])])],
  [_ax_lp_pop([$1])$0([$1],[$2],
      m4_defn([_ax_lp_pc])m4_popdef([_ax_lp_pc]))])])

#---------------------------------------------
# line processing

# _ax_lp_doline(<CTX>,<INDENTATION>,<LINE>)
#   unwind to the parent frame,
#   parse for the cmd name
#   and then _ax_lp_docmd(...)
#
m4_define([_ax_lp_doline],
[_ax_lp_unwind([$1],[$2],[0])dnl
_ax_lp_docmd([$1],[$2],_ax_lp_cmdargs([$1],[$3]))])

# _ax_lp_cmdargs(<CTX>,<LINE>)
#   -> (<CMD>, <PRELIM_ARG>+)
#   extract <CMD> from <LINE>; usually we break at the
#   first whitespace and <PRELIM_ARG>+ is a single value with
#   everything beyond.   But, we special-case:
#      ID=rest -> (=, ID, rest)
#      --word-word2[=rest] -> (--word, word2[=rest])
#
m4_define([_ax_lp_cmdargs],ax_lp_NTSC(
[m4_if(m4_bregexp([$2],[^[a-zA-Z_][a-zA-Z_0-9]*[ST]*=]),[0],
       [m4_bpatsubst([$2],
         [^\([a-zA-Z_][a-zA-Z_0-9]*\)[ST]*=[ST]*\(.*\)],
         [[=],[\1],[\2]])],
       [m4_bpatsubst([$2],
         [^\(--[a-z]*-?\|-[^-TSN]*\|[^-N][^=TSN]*\)[TS]*N?\(.*\(N.*\)*\)\'],
         [[\1],[\2]])])]))

# _ax_lp_docmd(<CTX>,<INDENTATION>,<CMD>,<PRELIM_ARG>+)
#   (1) invoke <PARENT:allow>(<CMD>) which aborts if <CMD> is not a subcmd
#   (2) push a frame for <CMD>
#   (3) invoke <CMD:args>(<PRELIM_ARG>+) -> <ARG>+ (cmd-specific parse)
#   (4) invoke <CMD:fn>(<ARGS>+)
#
m4_define([_ax_lp_docmd],
[_ax_lp_call([$1],m4_defn([$1__p_cmd]$1__p_top),[:allow],[ :allow],[$3])dnl
_ax_lp_push([$1],[$2],[$3])dnl
_ax_lp_call([$1],[$3],[:fn],[ :fn],
   _ax_lp_call([$1],[$3],[:args],[ :args],
   m4_shift3($@)))])

# _ax_lp_call(<CTX>,<CMD>,<HOOK>,<FALLBACK>,<ARG>+)
#    how to invoke <CMD:HOOK>(<ARG>+):
#      either <CMD> has <HOOK> defined,
#      or we use the language's <FALLBACK>
#        with <CMD> in front of the <ARG>+
#
m4_define([_ax_lp_call],
[m4_ifdef(m4_defn([$1__p_lang])[|$2|$3],
   [m4_indir(m4_defn([$1__p_lang])[|$2|$3], [$1],       m4_shift2(m4_shift2($@)))],
   [m4_indir(m4_defn([$1__p_lang])[|$4],    [$1], [$2], m4_shift2(m4_shift2($@)))])])

#---------------------------------------
# the actual parser

#  _ax_lp_parse_lines(<CTX>)
#
# Given a context <CTX> initialized by _ax_lp_init,
# (1) parse all available lines, skipping blank lines and #-commentary as
#     well as incorporating heredocs into those lines that have them;
# (2) invoke _ax_lp_doline for each actual resulting directive line; and
# (3) after the final line, fully unwind the stack -- as if there were a
#     subsequent line with the same indentation as the first line.
#
m4_define([_ax_lp_parse_lines], ax_lp_NTSC(
  [_ax_lp_if_eof([$1],
     [m4_if($1__p_top,[0],[],
        [_ax_lp_unwind([$1],m4_defn([$1__p_ind1]),[0])])],
     [_ax_lp_nextline([$1])dnl
m4_if(m4_bregexp(_ax_lp_line([$1]), [^[ST]*[^STC]]), [-1],
        [],
        [m4_bpatsubst(_ax_lp_line([$1]),
           [^\([ST]*\)\([^C]*[^CST]\)[ST]*\(C.*\)?$],
           [_ax_lp_doline([$1],_ax_lp_tabspace([$1],[\1]),[\2])])])dnl
$0($@)])]))

# _ax_lp_tabspace(<CTX>,<WHITESPACE-STRING>)
#     -> [#tabs, #spaces] = <INDENTATION>
#
m4_define([_ax_lp_tabspace],ax_lp_NTSC(
  [m4_if(m4_bregexp([$2],[^T*S*$]),[-1],
     [ax_lp_fatal([$1],[bad indentation: spaces before tabs])],
     [m4_pushdef([_ax_lp_t],m4_index([$2S],[S]))dnl
m4_quote(_ax_lp_t,m4_eval(m4_len([$2])-_ax_lp_t))dnl
m4_popdef([_ax_lp_t])])]))

# _ax_lp_if_eof(<CTX>,<IF-EOF>,<IF-MORE-LINES>)
#   note that more_lines_case includes that of
#   there being only blank lines remaining
#
m4_define([_ax_lp_if_eof],
  [m4_ifval(m4_defn([$1__p_lrest]),[$3],[$2])])

# _ax_lp_line(<CTX>) -> current line
#
m4_define([_ax_lp_line], [m4_defn([$1__p_line1])])

# _ax_lp_nextline(<CTX>)
#   advance to the next line
#
m4_define([_ax_lp_nextline],
[_ax_lp_loadlines([$1],$1__p_lrest)])

# _ax_lp_loadlines(<CTX>, <LINE>[, <LINE>...])
#   helper: install first line in $1__p_line1,
#   remaining lines in $1__p_lrest,
#   advance $1__p_lnum assuming lines is previous lrest
#
m4_define([_ax_lp_loadlines],ax_lp_NTSC(
[m4_define([$1__p_line1],[$2])dnl
m4_define([$1__p_lnum],m4_incr($1__p_lnum))dnl
m4_if([$C],[2],
  [m4_define([$1__p_lnum],m4_eval($1__p_lnum + $1__p_lskip))dnl
_ax_lp_next_heredoc([$1], m4_defn([$1__p_ltail]))],
  [m4_define([$1__p_lrest], m4_dquote(m4_shift2($@)))])dnl
]))

# _ax_lp_next_heredoc(<CTX>,<STRING>)
#   grab the substring of <STRING> up to and including
#   the next heredoc (or the entire string if there is none),
#   split it into lines and assign it to ctx__p_lrest
#   and any remainder to ctx__p_ltail.  Set ctx__p_lskip
#   to the number of lines in heredoc
#
#   If string is nonempty, ctx__p_lrest is guaranteed
#   to have at least one line.
#
#   <STRING> is assumed to begin with a newline;
#   (in what follows, each line is bundled with its *preceding* newline)
#
#   STAGE ONE
#     (1) decompose [string] into
#          [\nbefore]\n[hdline <<endmark][\nafter]
#   where hdline has no newlines or comments (#.*)
#   and endmark is a proper identifier ([A-Z_][A-Z_0-9]*),
#   or, if no line with the <<endmark$ pattern is found
#   everything goes in [\nbefore]
#
m4_define([_ax_lp_next_heredoc],
  [m4_bregexp([$2],
    m4_defn([_ax_lp_heredoc_horrific_regexp]),
    [_ax_lp_nexthd2([$1],[\1],[\6],[\7],[\8])])])

# (Yes, this is complicated, but trying to reassemble a heredoc that
# has already been split into lines that may possibly not have
# balanced quoting would be worse.  Trust me on this.)
#
m4_define([_ax_lp_heredoc_horrific_regexp],
  ax_lp_NTSC( [\`\(\(N\(]dnl
dnl  \1 is all lines before the first line with a heredoc.
dnl  Here are the ways for a line NOT to start a heredoc:
dnl     (0) be empty
[\|]dnl
dnl     (1) contain a #-comment
dnl         (we do not allow comments after <<END_ID)
[.*C.*\|]dnl
dnl
dnl     (2) end with a non-id char
[[^NC]*[^NCA-Za-z_0-9]\|]dnl
dnl
dnl     (3) end with id chars immediately preceded by
dnl         nothing, a non-'<', or a singleton '<'
[\(\|[^NC]*[^<NCA-Za-z_0-9]\|<\|[^NC]*[^NC<]<\)[A-Za-z_0-9]+\|]dnl
dnl
dnl     (4) end with '<<'(id chars that do not form an identifier)
[[^NC]*<<[0-9][A-Za-z_0-9]*]dnl
dnl
dnl  and that is all
[\)\)*\)]dnl
dnl
dnl  \6 is the beginning of line to last non-whitespace before the <<;
dnl  \7 is the end marker id;
dnl  ? goes around \5 because overall pattern must always match
[\(N\(\|[^NC]*[^NCST]\)[ST]*<<\([A-Za-z_][A-Za-z_0-9]*\)\)?]dnl
dnl
dnl  \8 is everything beyond (empty if no heredoc found
dnl  because \1 will be greedy)
dnl
[\(\(N.*\)*\)\']))

# _ax_lp_nexthd2(<CTX>,<BEFORE>,<HD1>,<ENDMARK>,<AFTER>)
#    STAGE TWO
#      split [before] into lines. If a heredoc line was found,
#      find the next occurrence of [endmark] in [after], and
#      extract, pushing [hd1\nhdlines] onto the end of
#      the [before] list and leaving the rest of [after] alone.
#      Otherwise [hd1]=[endmark]=[after]=[].
#      Update [$1__p_ltail] = [after]
#             [$1__p_lrest] = cdr(split([before]))(,heredoc)?
#             [$1__p_lskip] = number of lines in heredoc
#      recalling that we had a fake blank line at the start
#
m4_define([_ax_lp_nexthd2], ax_lp_NTSC(
[m4_define([$1__p_ltail], [$5])dnl
m4_pushdef([_ax_lp_b], m4_split([$2]m4_ifval([$4],[[N$3]],[]),[N]))dnl
m4_ifval([$4],

      [m4_pushdef([_ax_lp_i],
         m4_bregexp(m4_defn([$1__p_ltail]), [^$4]m4_newline()))dnl
m4_if(_ax_lp_i, [-1],
       [m4_define([$1__p_lnum],
          m4_eval($1__p_lnum + m4_count(_ax_lp_b) - 1))dnl
ax_lp_fatal([$1],[end of heredoc '<<$4' not found])])dnl
dnl
m4_pushdef([_ax_lp_hd],
           ax_lp_strhead(m4_decr(_ax_lp_i),m4_defn([$1__p_ltail])))dnl
dnl
m4_define([$1__p_ltail],
          ax_lp_strtail(m4_eval(_ax_lp_i + m4_len([$4])),
            m4_defn([$1__p_ltail])))dnl
dnl
m4_define([$1__p_lskip],
          m4_incr(_ax_lp_newline_count(m4_defn([_ax_lp_hd]))))dnl
m4_popdef([_ax_lp_i])],

      m4_if(# no heredoc case:
      # here we need to make sure __p_ltail is being cleared
      # otherwise infinite loops will happen
      )[m4_ifval(m4_defn([$1__p_ltail]),
[m4_fatal([_ax_lp_next_heredoc ']m4_defn([$1__p_ltail])dnl
[' regexp fail should not happen])])dnl
m4_pushdef([_ax_lp_hd], [])dnl
m4_define([$1__p_lskip], [0])])dnl
dnl
m4_define([$1__p_lrest], m4_cdr(_ax_lp_b[]m4_defn([_ax_lp_hd])))dnl
m4_popdef([_ax_lp_b],[_ax_lp_hd])]))


#------------------------------------------------------
# AX_LP_DEFINE_LANGUAGE( <LANGUAGE>, <DEFINE_CMD>{, <:KWD>, <KWDEXP>}* )
#
#   defines <DEFINE_CMD>, and declares the global fallback hooks
#
# <DEFINE_CMD>( <CMD>{, <:KWD>, <KWDEXP>}* )
#
#   declares the hooks for a specific cmd
#
# In both cases the keyword expressions are used to compose hook macros
# that get invoked at various stages of the parse.  There are four hooks
#
#    :args(<CTX>{, <PRELIM_ARGS...>}) -> <BETTER_ARGS...>
#    :fn(<CTX>, <BETTER_ARGS...>)
#       These are both evaluated when the complete first line
#       (including heredoc) of the CMD instance is read,
#       with the result of :args being fed to :fn
#
#    :fnend(<CTX>)
#       Evaluated after the last child cmd (i.e., last line with
#       greater indentation) but before the next sibling cmd
#
#    :allow(<CTX>, <CHILD_CMD>) -> 1|0
#       Evaluated first (i.e., before any of :args/:fn/:fnend)
#       for each child of a CMD instance and expected to return 1
#       true (nonzero) iff CHILD_CMD is a valid child of CMD;
#       an error will be raised otherwise.
#
# When these are used under AX_LP_DEFINE_LANGUAGE, they define global
# "fallback" hooks that take the cmd name as the first argument, i.e.:
#    :args( <CTX>, <CMD>, <PRELIM_ARGS...>) -> <BETTER_ARGS...>
#    :allow(<CTX>, <CMD>, CHILD_CMD) -> 1|0
#
# (***FIX
#   Global "fallback" :fn/:fnend hooks should be deprecated.  They
#   will not do what you would expect and I cannot see them being
#   useful for anything in the current state of things.  Overriding
#   the default :args/:allow is doable but might cause confusion...
# ***)
#
# Every line is given an initial parse to extract the cmd word
# (CMD), usually the first whitespace delimited word, and a
# preliminary parameter list (PRELIMINARY_ARGS...), usually just the
# entire rest of the line as a single string.  See _ax_lp_cmdargs for
# exceptions.
#
# Within <DEFINE_CMD>, the following :KWDs have special meanings:
#
#     [:parent], <PARENTCMD>
#       Adds PARENTCMD as an allowed parent of CMD;
#       (Used by the global default :allow hook;
#       ignored if PARENTCMD has its own ":allow")
#
#     [:subcmds], [<CMD>{, <CMD>}*]
#       Appends to the list of allowed children of CMD;
#       (Used by the global default :allow hook;
#       ignored if this cmd has its own ":allow")
#
#     [:vars], [<ID>{, <ID>}*]
#     [:var], [<ID>, <VALUE>],
#       These declare local variables, :vars initializing
#       all of them to [] while :var initializes a single
#       <ID> to a particular value.
#       Use ax_lp_{get,put,append...} to retrieve and set values later.
#
#     [:sets],   [<ID>{, <ID>}*]
#       These declare the <ID>s to be sets.
#       Use ax_lp_set_* to add members and query them
#     [:hashes], [<ID>{, <ID>}*]
#       These declare the <ID>s to be hashes.
#       Use ax_lp_hash_{put,get} to store values at
#       particular keys and read them back
#
#     [:allow], [<BODY>]
#       Sets the expansion of the :allow hook to <BODY>,
#       $1=<CHILD_CMD>.
#     [:args], [<BODY>]
#       Sets the expansion of the :args hook to <BODY>,
#       parameter references ($1,...) in <BODY> refer to the
#       <PRELIM_ARGS...>
#
#     [:fn], [<BODY>]
#       Appends BODY to the :fn hook.  For the root cmd ([]),
#       parameter references refer to the <INITARGS...> passed to
#       AX_LP_PARSE_SCRIPT.  For other cmds, they refer to
#       the <BETTER_ARGS...> that :args expands to / returns.
#
#     [:fnend], [<BODY>]
#       Prefixes BODY to the :fnend hook.  Multiple :fnend
#       declarations for the same cmd will thus be invoked in
#       reverse order.
#
# Note that the ordering of :fn, :fnend, and :local* declarations for
# a given cmd matters.  A given variable declaration will only be
# visible in :fn/:fnend and :var value expressions that *follow* it.
#
# Variables are in their own namespace prefixed by the parsing context
# (CTX), and thus, their names should not interfere with anything external.
# However,
#
# (1) VAR names must begin with a letter (upper or lower case,
#     does not matter).  (There are particular choices of names
#     beginning with underscore that will screw up AX_LP internals.)
#
# (2) On any given level, a name can only be used once
#     (i.e., you cannot declare something to be *both*
#     a :var *and* a :set)  (***FIX we could check for this***)
#     and you should only use
#        ax_lp_get/put/append on ordinary :vars
#        ax_lp_set_*          on :sets
#        ax_lp_hash_*         on :hashes
#
# It is however completely legal and expected to re-use a given
# variable name in a descendant cmd (it will hide/shadow the ancestral
# declaration).
#
AC_DEFUN([AX_LP_DEFINE_LANGUAGE],
  [_ax_lp_cmd_definer([$2],[$1])dnl
_ax_lp_lang_globals(_ax_lp_lang_prefix([$1]),
  [:args],  [m4_shift2($][@)],
  [:allow], m4_translit([[m4_set_contains(_ax_lp_lang_prefix([$1])[| _ok_par], [A2|A3], [],
                          [ax_lp_fatal2([A1],
                            ['A3' not allowed],
                            [this is a 'A2' block])])]],
                        [A],[$]),
  [:fn],    [],
  [:fnend], [],
  m4_shift2($@))])

m4_define([_ax_lp_lang_prefix], [[ax_lp_%($1)]])

m4_define([_ax_lp_lang_globals], ax_lp_NTSC(
  [m4_if($C,[2],
    [m4_ifval([$2],[m4_fatal([odd number of arguments for $0 ($2)])])],
    [m4_define([$1| $2], [$3])$0([$1],m4_shift3($@))])]))

m4_define([_ax_lp_cmd_definer],
  [m4_define([$1],[_ax_lp_lang_define_cmd(_ax_lp_lang_prefix([$2]),$][@)])])


# _ax_lp_lang_define_cmd(<LANGUAGE_PREFIX>,<CMD>,[<KWD>,<KWDEXP>]*)
#    define the hook functions that implement <CMD> in <LANGUAGE>
#
m4_define([_ax_lp_lang_define_cmd],
  [m4_case([$3],

    [:parent],
      [m4_set_add([$1| _ok_par],[$4|$2])],
    [:subcmds],
      [m4_map_args_sep([m4_set_add([$1| _ok_par],[$2]|],[)], [], $4)],


    [:fn],
      [_ax_lp_fn_add([$1], [$2], [$4])],
    [:fnend],
      [_ax_lp_fnend_add([$1], [$2], [$4])],

    [:vars], [m4_do(
      _ax_lp_fn_add([$1], [$2],
        [_ax_lp_push_locals(]m4_dquote([$][1])[,$4)]),
      _ax_lp_fnend_add([$1], [$2],
        [_ax_lp_pop_locals(]m4_dquote([$][1])[,$4)]))],

    [:var], [m4_do(
      _ax_lp_fn_add([$1], [$2],
        [m4_pushdef(]m4_dquote([$][1_]$4)[)]),
      _ax_lp_fnend_add([$1], [$2],
        [m4_popdef(]m4_dquote([$][1_]m4_car($4))[)]))],

    [:sets], [m4_do(
      _ax_lp_fn_add([$1], [$2],
         m4_map_args_sep(
           [[_ax_lp_push_local_indir(]m4_dquote([$][1],],
           [)[)]], [],
           $4)),
      _ax_lp_fnend_add([$1], [$2],
         m4_map_args_sep(
           [[m4_set_delete(ax_lp_get(]m4_dquote([$][1])[,]],
           [[))]], [],
           m4_reverse(m4_dquote_elt($4)))dnl
[_ax_lp_pop_locals(]m4_dquote([$][1])[,$4)]))],

    [:hashes], [m4_do(
      _ax_lp_fn_add([$1], [$2],
         m4_map_args_sep(
           [[_ax_lp_push_local_indir(]m4_dquote([$][1],],
           [)[)]], [],
           $4)),
     _ax_lp_fnend_add([$1], [$2],
         m4_map_args_sep(
         [[_ax_lp_hash_delete(]m4_dquote([$][1])[,]],
         [[)]], [],
         m4_reverse(m4_dquote_elt($4)))dnl
[_ax_lp_pop_locals(]m4_dquote([$][1])[,$4)]))],

    [m4_define([$1|$2|$3],[$4])])]dnl
dnl
[m4_if(]ax_lp_NTSC([$C])[,[3],[],
   [$0([$1],[$2],m4_shift2(m4_shift2($@)))])])


m4_define([_ax_lp_fn_add],
  [m4_define([$1|$2|:fn],
    _ax_lp_defn_missing_ok([$1|$2|:fn])[[]][$3])])

m4_define([_ax_lp_fnend_add],
  [m4_define([$1|$2|:fnend],
    [$3][[]]_ax_lp_defn_missing_ok([$1|$2|:fnend]))])

m4_define([_ax_lp_push_locals],
[m4_map_args_sep([m4_pushdef([$1_]],[,[])],[],m4_shift($@))])

m4_define([_ax_lp_pop_locals],
[m4_map_args_sep([m4_popdef([$1_]],[)],[],m4_reverse(m4_shift($@)))])

m4_define([_ax_lp_push_local_indir],
  [m4_pushdef([$1_$2],[$1_$2!]$1__p_top)])


#=====================================
# utilities for use in language hooks

#-----------
# variables

# ax_lp_get(<CTX>,<VARNAME>+)  -> (<VAL>+)
#   where each <VAL> = m4_defn(<CTX>_<VARNAME>)
#
m4_define([ax_lp_get],
  [m4_map_args_sep([m4_defn([$1_]],[)],[,],m4_shift($@))])

# ax_lp_put(<CTX>,<VARNAME>,<VAL>)  -> ''
#   does assignment <VARNAME> <- <VAL>
#
m4_define([ax_lp_put],
  [m4_define([$1_$2], [$3])])

# ax_lp_append(<CTX>,<VARNAME>,<MOREVAL>,<SEP>)
#   appends <MOREVAL> to <VARNAME>'s value,
#   including <SEP> if value was previously ''
#
m4_define([ax_lp_append],
  [m4_append([$1_$2], [$3], m4_ifval(m4_quote(m4_defn([$1_$2])),[$4]))])

# ax_lp_ifdef(<CTX>, <VARNAME>, <IF-DEF>, <IF-UNDEF>)
# ax_lp_<VERB>(<CTX>, <VARNAME>, <ARGS>...)
#   -> m4_<VERB>(<CTX>_<VARNAME>, <ARGS>...)
#
m4_map_args_sep(
  [ax_lp_beta([&],
     [m4_define([ax_lp_&1],
                [m4_&1(]m4_dquote([$][1_])[m4_shift($][@))])],],
  [)], [],
  [ifdef])

#------
# sets
#    (implemented as an ordinary variable whose value
#     names an m4_set.  Do NOT use ax_lp_put on these.)

# ax_lp_set_add ax_lp_set_add_all
# ax_lp_set_contains ax_lp_set_empty ax_lp_set_size
# ax_lp_set_map_sep
#   ax_lp_<VERB>(<CTX>, <VARNAME>, <ARGS>...)
#   -> m4_<VERB>(ax_lp_get(<CTX>,<VARNAME>), <ARGS>...)
#
m4_map_args_sep(
  [ax_lp_beta([&],
     [m4_define([ax_lp_&1],
                [m4_&1(ax_lp_get(]m4_dquote([$][1],[$][2])[),
                       m4_shift2($][@))])],],
  [)], [],
  [set_add], [set_add_all],
  [set_contains], [set_empty], [set_size],
  [set_map_sep])

#-------------
# hash tables
#    (implemented as an ordinary variable whose value
#     names an m4_set (of valid keys).
#     Do NOT use ax_lp_put on these.)

# ax_lp_hash_empty(<CTX>, <HNAME>)
# ax_lp_hash_size(<CTX>, <HNAME>)
# ax_lp_hash_has_key(<CTX>, <HNAME>, <KEY>, <IF_PRESENT>, <IF_ABSENT>)
# ax_lp_hash_map_keys_sep(<CTX>,<HNAME>, <PRE>, <POST>, <SEP>, <ARG>+)
#
# ax_lp_hash_<VERB>(<CTX>,<HNAME>[,<ARG>*])
#   -> m4_set_<VERB>(ax_lp_get(<CTX>,<HNAME>)[,<ARG>*])
#
m4_map_args_sep(
  [ax_lp_beta([&],
     [m4_define([ax_lp_hash_&1],
         [m4_set_&1(ax_lp_get(]m4_dquote([$][1],[$][2])[),
                    m4_shift2($][@))])],], [)],
  [],
  [empty], [size], [contains], [map_sep])

m4_rename([ax_lp_hash_map_sep],[ax_lp_hash_map_keys_sep])
m4_rename([ax_lp_hash_contains],[ax_lp_hash_has_key])

# ax_lp_hash_get(<CTX>,<HNAME>,<KEY>,<DEFAULT>)
# ax_lp_hash_put(<CTX>,<HNAME>,<KEY>,<VALUE>)
# ax_lp_hash_append(<CTX>,<HNAME>,<KEY>,<MOREVALUE>,<SEP>)
#
m4_map_args_sep(
  [m4_define([ax_lp_hash_]],
  [, [_$0(ax_lp_get([$1],[$2]),m4_shift2($@))])],
  [],
  [get],[put],[append])

m4_define([_ax_lp_hash_get],
  [m4_set_contains([$1], [$2],
    [m4_defn(_ax_lp__hash_key([$1],[$2]))], [$3])])

m4_define([_ax_lp_hash_put],
  [m4_set_add([$1], [$2],
      [m4_pushdef],[m4_define])(
         _ax_lp__hash_key([$1],[$2]), [$3])])

m4_define([_ax_lp_hash_append],
  [m4_set_add([$1], [$2],
      [m4_pushdef(_ax_lp__hash_key([$1],[$2]), [$3])],
      [m4_append( _ax_lp__hash_key([$1],[$2]), [$3], [$4])])])

# internal only
m4_define([_ax_lp_hash_delete], [$0_1(ax_lp_get($@))])
m4_define([_ax_lp_hash_delete_1],
  [m4_set_map_sep([$1], [m4_popdef(_ax_lp__hash_key([$1],],[))])dnl
m4_set_delete([$1])])

m4_define([_ax_lp__hash_key],[[_ax_lp_H([$1],[$2])]])


#--------------------------------
# stack frame

# ax_lp_cmd(<CTX>)
#   -> current <CMD>
#
m4_define([ax_lp_cmd],   [m4_defn([$1__p_cmd]$1__p_top)])

# ax_lp_parent(<CTX>)
#   -> current parent <CMD> ([] if root)
#
m4_define([ax_lp_parent_cmd],[m4_defn([$1__p_cmd]m4_decr($1__p_top))])

# ax_lp_level(<CTX>)
#   -> current level number == height of stack
#
m4_define([ax_lp_level],   [m4_defn([$1__p_top])])

# ax_lp_fatal(<CTX>,<MESSAGE>)
#    print <MESSAGE> on stderr tagged with the current file position
#    and exit
m4_define([ax_lp_fatal],
[m4_errprintn($1__p_file:$1__p_lnum[: error: $2])m4_exit(1)])

# ax_lp_fatal2(<CTX>,<MESSAGE>,<MESSAGE2>)
#    print <MESSAGE> on stderr tagged with the current file position
#    print <MESSAGE2> tagged with start of the current directive
#    and exit
m4_define([ax_lp_fatal2],
[m4_errprintn($1__p_file:$1__p_lnum[: error: $2])dnl
m4_errprintn($1__p_file:$1__p_ln1[: note: $3])dnl
m4_exit(1)])


#=====================
# internal utilities


# _ax_lp_defn_missing_ok(<NAME>) -> <DEFN> or []
# same as m4_defn() but no error if undefined.
#
m4_define([_ax_lp_defn_missing_ok],
  [m4_ifdef([$1],[m4_defn([$1])],[])])


# _ax_lp_newline_count(<STRING>)
#   -> number of newlines in <STRING>
#
m4_define([_ax_lp_newline_count],
[m4_len(m4_bpatsubst([$1],[.],[]))])


# ax_lp_strhead(<N>,<STRING>)
#   -> first <N> chars of string, quoted
#
# ax_lp_strtail(<N>,<STRING>])
#   -> all chars except for the first <N>, quoted
#
# Think of these better versions of m4_substr(), which is mostly
# useless because it does not quote its results.
#
# Note that Bad Things happen if extracted substrings are not quote
# balanced, meaning you need to independently make sure that is
# already the case or otherwise make people promise not to do that.
# ax_lp_strtail also assumes ax_lp_strhead(n,[string]) is balanced.
# (And, surprise:  Use of m4_changequote() will undoubtedly screw
# these up, so that is something else to *NOT* do.)

m4_define([ax_lp_strhead],
  [m4_format([[%.*s]],[$1],[$2])])

m4_define([ax_lp_strtail], ax_lp_NTSC(
  [m4_bregexp([$2],[\`]m4_bpatsubst(ax_lp_strhead([$1],[$2]),
     [.],[.])[\(\(.\|N\)*\)],[[\1]])]))
