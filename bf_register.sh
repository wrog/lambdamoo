#!/bin/sh
#     Copyright (C) 2025  Roger F. Crew
#
#     This file is fully redistributable+modifiable free software
#     with zero warranty made available under the same terms as
#     autoconf itself, which is to say, essentially public domain.  Enjoy.
#
################
#   bf_register.sh srcdir COMMON_CSRCS... -- ALL_XT_CSRCS... -- XT_CSRCS...
#
# COMMON_CSRCS..., ALL_XT_CSRCS..., XT_CSRCS...
#   are lists of .c files from Makefile.
# Read all files, collecting all void-return and (void)-args
#   function definition names of the form 'register_<FOO>'
# Copy bf_register.{h,c}.in to bf_register.{h,c},
#   replace the filename comment block
#   replace @@EXTERNS@@   with extern declarations (for the .h)
#   replace @@FUNCTIONS@@ with function references (for the .c)
#   but only write the files if they actually change.
#
# The function list is sorted with COMMON_CSRCS functions coming first
# but otherwise lexicographically.  Functions for inactive extensions,
# i.e., appearing in ALL_XT_CSRCS but not XT_CSRCS, are commented out.

#------------#
#  Prologue  |
#------------#
#
# shamelessly steal abbreviated boilerplate from Autoconf World:

cleanup() {
    rm -rf $TMPDIR
}

fail() {
    printf "*** %s\n" "$1" >&2
    exit 1
}

{
  TMPDIR=`(umask 077 && mktemp -d ./bf_tmpXXXXXX) 2>/dev/null` &&
  test -d "$TMPDIR"
}  ||
{
  TMPDIR=./bf_tmp$$-$RANDOM
  (umask 077 && mkdir "$TMPDIR")
} ||
{
    printf "cannot create a temporary directory\n" >&2
    exit $?
}

# :; preserves exit status in pre-2.05 versions of bash (?)
trap ':; cleanup' 0

# Avoid depending upon Character Ranges.
as_cr_letters='abcdefghijklmnopqrstuvwxyz'
as_cr_LETTERS='ABCDEFGHIJKLMNOPQRSTUVWXYZ'
as_cr_Letters=$as_cr_letters$as_cr_LETTERS
as_cr_digits='0123456789'
as_cr_alnum=$as_cr_Letters$as_cr_digits
as_sed_sh="y%*+%pp%;s%[^_$as_cr_alnum]%_%g"


#--------------------#
#  Build file lists  |
#--------------------#

# An FKEY is the legal-shell-variable version of a filename
ALL_FKEYS=

# lookup FILENAME
#   generate FKEY for FILENAME
#   ensure FKEY was not previously in use for another file,
#   set found=<FKEY>
#   set FN_<FKEY>=<FILENAME>
#   append <FKEY> to ALL_FKEYS
#
lookup () {
  found=`printf "%s\n" "$1" | sed "$as_sed_sh"`
  while : ; do
      if eval test \${FN_$found+y}; then
	  if eval 'test x"$FN_'"$found"'" != x"$1"'; then
	      found="${found}x"
	      continue
	  fi
      else
	  eval "FN_$found"='"'"$1"'"'
	  ALL_FKEYS="$ALL_FKEYS $found"
      fi
      break
  done
}

srcdir="$1"
shift

# ${COM_<FKEY>} == this file is in COMMON_CSRCS
# ${USE_<FKEY>} == this file is in COMMON_CSRCS or an active extension

# do COMMON_CSRCS
while test $# -gt 0; do
    if test "$1" = "--"; then
	shift
	break
    fi
    lookup "$1"
    eval "COM_$found"=:
    eval "USE_$found"=:
    shift
done

# do ALL_XT_CSRCS
while test $# -gt 0; do
    if test "$1" = "--"; then
	shift
	break
    fi
    lookup "$1"
    if eval test \${COM_$found+y}; then
	fail "file $1 in both COMMON_CSRCS and ALL_XT_CSRCS ?"
    fi
    eval "COM_$found"=false
    eval "USE_$found"=false
    shift
done

# do XT_CSRCS
while test $# -gt 0; do
    lookup "$1"
    eval test \${COM_$found+y} || \
	fail "file $1 in XT_CSRCS but not ALL_XT_CSRCS ?"
    eval '${COM_'"$found"'}' && \
	fail "file $1 in XT_CSRCS and COMMON_CSRCS ?"

    eval "USE_$found"=:
    shift
done

#-------------------------------#
#  Find registration functions  |
#-------------------------------#

# append function names to SYMS_COM or SYMS_XT as appropriate
SYMS_COM=
SYMS_XT=
for FK in $ALL_FKEYS; do
    eval 'F="$FN_'"$FK"'"'
    for SYM in `sed -n -e '/^void *$/{s/.*//;N;s%(void) *%%;=;p;}' "$srcdir/$F" |
           grep -E "^([$as_cr_digits][$as_cr_digits]*|register_[$as_cr_Letters][_$as_cr_alnum]*)$"` ; do
	case $SYM in  #(
	    [$as_cr_digits]*)
		L=$SYM
		;; #(
	    register_*)
		if eval test \${LOC_$SYM+y} ; then
		    eval LOC_$SYM='"${LOC_'$SYM'}, $F:$L"'
		else
		    eval LOC_$SYM='"$F:$L"'
		fi

		if eval '${COM_'"$FK"'}'; then
		    SYMS_COM="$SYMS_COM $SYM"
		else
		    SYMS_XT="$SYMS_XT $SYM"
		fi
		if eval '${USE_'"$FK"'}'; then
		    eval "DO_$SYM"=:
		elif eval test \${DO_$SYM+y} ; then
		    :
		else
		    eval "DO_$SYM"=false
		fi
		;;
	esac
    done
done


#----------------------#
#  sort SYMS_(COM|XT)  |
#----------------------#

SYMS=`printf "%s\n" $SYMS_COM | sort -u`
test "x$SYMS_XT" = x || {
    SYMS="$SYMS -- "`printf "%s\n" $SYMS_XT | sort -u`
}

#---------------------------#
#  write bf_register.{c,h}  |
#---------------------------#

# put FILE VAR OUT INDENT
#   substitute OUT for VAR in FILE with extra INDENTation in places,
#   but leave FILE untouched (i.e., no lastmod time update)
#   if there are no changes
#
put () {
    FILE="$1"
    VAR="$2"
    OUT="$3"
    INDENT="$4"
    INFILE="$srcdir/$FILE.in"
    grep -E "^${VAR}"'$' "$INFILE" >/dev/null 2>&1 || \
	fail "${FILE}.in is expected to have '${VAR}' on a line by itself."
    {
	printf "/*  %s  --  Generated from %s.in by %s\n" "$1" "$1" "$0"
	printf " * _____________________ DO NOT EDIT ___________________________________ */\n"
	sed -e "0,/[*]\// d ; /${VAR}/,"'$ d' "$INFILE"
	printf "%s%s\n" \
	       "$INDENT" '/*----------------------*' \
	       "$INDENT" " |   ${VAR} ==" \
	       "$INDENT" ' */'

	printf "%s" "$OUT"
	printf "\n"
	printf "%s%s\n" \
	       "$INDENT" '/*    END of generated content.' \
	       "$INDENT" ' *-------------------------------*/'

	sed -e "1,/${VAR}/ d" "$INFILE"
    } >"$TMPDIR/$FILE"

    if test ! -f "$TMPDIR/$FILE" ; then
	fail "could not write $TMPDIR/$FILE"
    elif test -f "$FILE" &&
	 diff -q "$TMPDIR/$FILE" "$FILE" >/dev/null 2>&1; then
	    :
    else
	mv -f "$TMPDIR/$FILE" "$FILE"
	printf "writing %s\n" "$FILE"
    fi
    chmod -w "$FILE"
}

set xx $SYMS
shift
while test $# -gt 0; do
    if test "$1" = "--"; then
	shift
	COUT="$COUT
"
	HOUT="$HOUT
"
	continue
    fi
    eval 'LOC="${LOC_'"$1"'}"';
    if eval '${DO_'"$1"'}'; then
	PRE='  '
	MID='/*'
    else
	PRE='/*'
	MID=' *'
    fi
    COUT=`printf "%s%s  %-24s %s %-18s */" "$COUT" "$PRE" "$1," "$MID" "$LOC"`'
'
    HOUT=`printf "%s%s extern void %-30s %s %-18s */" "$HOUT" "$PRE" "$1(void);" "$MID" "$LOC"`'
'
    shift
done

put bf_register.h '@@EXTERNS@@'   "$HOUT" ""
put bf_register.c '@@FUNCTIONS@@' "$COUT" "    "


#-----------------------------#
#  write bf_register.lastrun  |
#-----------------------------#

echo timestamp > bf_register.lastrun
exit 0
