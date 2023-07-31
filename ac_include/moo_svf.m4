dnl  -*- autoconf -*-
dnl  moo_svf - routines to support server_version_full()

# Synopsis:
#
#   AC_CONFIG_COMMANDS([version.lastck],
#     [[write_lastck]],[[$init_lastck]])
#
#   MOO_SVF_ARG_ENABLES
#   MOO_SVF_LASTCK_INIT([init_lastck],[write_lastck])


# -----------------------------------------
# MOO_SVF_ARG_ENABLES()
#  creates block of --(disable|enable)-svf-OPTION for ./configure
#
AC_DEFUN([MOO_SVF_ARG_ENABLES],
[MOO_PUT_HELP([ENABLE],[
 Settings for server_version_full:])
AC_ARG_ENABLE([svf-source],
[AS_HELP_STRING([--(en|dis)able-svf-source],[include/exclude 'source' group],[30])],
[],[enable_svf_source=yes])
MOO_SVF_ARG_MAKE
AC_ARG_ENABLE([svf-diff],
[AS_HELP_STRING([--enable-svf-diff=yes],[check for modified source],[30])
AS_HELP_STRING( [                 =?],[no check, assume modified],[30])
AS_HELP_STRING( [                 =!],[no check, assume not],[30])],
[],[enable_svf_diff=yex])])

# -----------------------------------------
# MOO_SVF_LASTCK_INIT(VAR, lastck, [tree_hash, [file_hash]])
#  accumulates INIT code in $VAR, expected to be a sequence of
#  function declarations but really can be anything,
#  including:
#  (1) a declaration for lastck() as the function to call
#      (from config.status) to remake version.lastck
#  (2) (optional) a name for the tree_hash function
#      if something needs to reference that explicitly
#  (3) (optional) a name for the file_hash function
#      if something needs to reference that explicitly
#
# expected usage:
#   AC_CONFIG_COMMANDS([version.lastck],
#     [[write_lastck]],[[$init_lastck]])
#
#   ...(various checks)...
#
#   MOO_SVF_LASTCK_INIT([init_lastck],[write_lastck])
#
# ordering does not matter since nothing specified by
# AC_CONFIG_* takes effect prior to AC_OUTPUT.
#
AC_DEFUN([MOO_SVF_LASTCK_INIT],
[AC_REQUIRE([MOO_PROG_SHA1GIT_HASH])dnl
AC_ARG_VAR([VERSION_EXT],[manually sets the version number 'extension' part])dnl
m4_pushdef([_moo_fhash],
  m4_ifval([$4],[[$4]],[[moo_fhash_]__line__]))dnl
m4_pushdef([_moo_trhash],
  m4_ifval([$3],[[$3]],[[moo_trhash_]__line__]))dnl
[
]$1[=
if test -f "$srcdir/version.fixed" ; then
  if test "$moo_prog_cv_sha1git_hash" != no &&
     test -f "$srcdir/MANIFEST"; then
  ]_MOO_SVF_ADDINIT([$1],
      [_moo_trhash [() {
]MOO_SVF_MANIFEST_TREE_HASH([_moo_fhash])[
}]])[
  fi
elif test "$moo_git_cv_repo" = yes ; then
  ]_MOO_SVF_ADDINIT([$1],
      [_moo_trhash [() {
]MOO_GIT_TREE_HASH([_moo_fhash])[
}]])[
fi
if test "x$]$1[" != x ; then
  ]$1[="
]_moo_fhash[ () {
  $moo_git_hash_body
}][$]$1["]
  AS_CASE([$enable_svf_diff],[[yex]], [[enable_svf_diff=yes]])[
else]
  AS_CASE([$enable_svf_diff],
    [[yex]], [[enable_svf_diff=?]],
    [[yes]], [AC_MSG_ERROR([enable-svf-diff=yes requires git repository or MANIFEST file])])[
fi
]_MOO_SVF_ADDINIT([$1],
[$2 [() {
  ]_MOO_SVF_WRITE_LASTCK([_moo_trhash])[
}]])[
$1="
VERSION_EXT=$VERSION_EXT
enable_svf_source=$enable_svf_source
enable_svf_diff=$enable_svf_diff
"][$]$1
m4_popdef([_moo_fhash])m4_popdef([_moo_trhash])])

# _MOO_SVF_ADDINIT(VAR, CODE)
#   appends CODE to var, completely quoted
m4_define([_MOO_SVF_ADDINIT],
[[{ $1=$][$1`cat`'
'; } <<\_MOO_END_ADDINIT]

$2
_MOO_END_ADDINIT
])

# -----------------------------------------
# MOO_SVF_MANIFEST_TREE_HASH(FILE_HASH)
#   given that FILE_HASH() hashes a single file,
#   compute a hash of all files listed in 'MANIFEST'
#   ((to be invoked from config.status))
#
AC_DEFUN([MOO_SVF_MANIFEST_TREE_HASH],
[[  moo_dfile=$tmp/moo_vdiff.tmp
  rm -f "$moo_dfile"
  for moo_f in `cat "$ac_top_srcdir/MANIFEST"` ; do]
    AS_CASE([$moo_f],[[
    keywords.c | parser.c | y.tab.h | configure]],[[
      continue
    ]],[[
      ]$1[ "$ac_top_srcdir/$moo_f" >>"$moo_dfile"
  ]])[
  done
  ]$1[ "$moo_dfile"
  rm -f "$moo_dfile"]])dnl

# _MOO_SVF_FORALL_UHASH(Nvar,[Hvar],BODY)
#   for Nvar = $moo_UFIRST .. (last) do
#     Hvar=${moo_UHASH$Nvar}
#     BODY
#   done
#   (Nvar is last+1)
#
m4_define([_MOO_SVF_FORALL_UHASH],[
  m4_pushdef([_moo_hash],m4_ifval([$2],[[$2]],[[moo_uh_]__line__]))dnl
  $1[=$moo_UFIRST
  while] AS_VAR_COPY([_moo_hash],[moo_UHASH][$]$1)[
    test "x$]_moo_hash[" != x
  do]
    $3
    AS_VAR_ARITH([$1], [$][$1 + 1])[
  done
  ]m4_popdef([_moo_hash])dnl
])

m4_define([_MOO_SVF_PUTVAR],
[AS_VAR_COPY([moo_PV],[moo_$1])
[printf "%s='%s'\n" "moo_$1" "$moo_PV" >>"$moo_vsrc_tmp"]])
dnl


# _MOO_SVF_WRITE_LASTCK(TREE_HASH)
#   given a TREE_HASH function
#   rewrite version.lastck
#   ((to be invoked from config.status))
#
m4_define([_MOO_SVF_WRITE_LASTCK],
[[  moo_UFIRST=1
  moo_UHASH1=
  moo_VCS=unknown
  moo_do_version_from_git=:
  if test -f "$srcdir/version.fixed"; then
    moo_do_version_from_git=false
    moo_VCS=release
  . "$srcdir/version.fixed" >/dev/null 2>&1
  fi
  if test "x$VERSION_EXT" = x; then
    moo_EXT0=$moo_EXT
  else
    moo_EXT0=$VERSION_EXT
  fi
  moo_no_write=yes
  if test -f ./version.lastck; then
    . ./version.lastck >/dev/null 2>&1
  fi
  moo_EXT=$moo_EXT0

  # write draft of new version.lastck
  moo_vsrc_tmp=$tmp/moo_vsrc.tmp
  rm -f "$moo_vsrc_tmp"
  cat >"$moo_vsrc_tmp" <<_MOO_END
@%:@!$SHELL
@%:@#### generated file; DO NOT EDIT #####
_MOO_END
  if $moo_do_version_from_git; then
    ]_MOO_SVF_VN_FROM_GIT[
    if test "x$enable_svf_source" = "xyes" ; then
      ]_MOO_SVF_SOURCE_GIT[
    fi
  fi
  moo_UHASH=
  ]AS_CASE([[$enable_svf_diff]],
   [['!']],[],
   [[no|'?']],[[
     moo_EXT=$moo_EXT? ]],
   [[yes]],[[
    moo_UHASH=`]$1[`
    test "x$moo_UHASH_RELEASE" != x &&
      test "x$moo_UHASH" = "x$moo_UHASH_RELEASE" &&
      moo_UHASH=
    ]])
  _MOO_SVF_PUT_DIFFHASHES
  _MOO_SVF_PUTVAR([EXT])[

  # produce contents of version_src.h
  cat >>"$moo_vsrc_tmp" <<\_MOO_END
if test x"$moo_no_write" = x ; then
  cat <<\_VSRC_END
_MOO_END
  if test "x$moo_RELEASE" != x ; then
    cat >>"$moo_vsrc_tmp" <<_MOO_END
@%:@define VERSION_RELEASE $moo_RELEASE
@%:@define VERSION_MINOR $moo_MINOR
@%:@define VERSION_MAJOR $moo_MAJOR
_MOO_END
  fi
  if test "x$enable_svf_source" = "xyes" ; then
    cat >>"$moo_vsrc_tmp" <<_MOO_END
@%:@define VERSION_SOURCE(DEF) DEF(vcs,"$moo_VCS") $moo_DEFSRC
_MOO_END
  fi
  cat >>"$moo_vsrc_tmp" <<_MOO_END
@%:@define VERSION_EXT "$moo_EXT"
_VSRC_END
fi
_MOO_END

  # replace if different
  moo_file=version.lastck
  if diff "$moo_file" "$moo_vsrc_tmp" >/dev/null 2>&1; then
    :
  else
    chmod -w+x "$moo_vsrc_tmp"
    rm -f "$moo_file"
    mv "$moo_vsrc_tmp" "$moo_file" \
      || ]AC_MSG_ERROR([could not create $moo_file])[
    if test "x$moo_RELEASE" = x; then
      ]AC_MSG_NOTICE([version ext = $moo_EXT])[
    else
      ]AC_MSG_NOTICE([version = $moo_MAJOR.$moo_MINOR.$moo_RELEASE$moo_EXT])[
    fi
  fi]])

#  _MOO_SVF_SOURCE_GIT
#    extract source group info from git
#    assumes moo_gdesc is already set from _MOO_SVF_VN_FROM_GIT
#    set and write
#      moo_VCS, moo_DEFSRC
#
m4_define([_MOO_SVF_SOURCE_GIT],
[[moo_VCS=git
  ]_MOO_SVF_PUTVAR([VCS])[

  # check for git upgrade
  moo_gversion=`git --version`
  moo_gversion=`expr "X$moo_gversion" : 'Xgit version \(.*\)'`
  moo_DEFSRC='DEF(vcs_version,"'$moo_gversion'")'

  # include commit id
  moo_DEFSRC=$moo_DEFSRC' DEF(commit,"'`git rev-list HEAD ^HEAD^`'")'

  if test "x$moo_gdesc" != x ; then
    moo_DEFSRC=$moo_DEFSRC' DEF(desc,"'$moo_gdesc'")'
  fi
  ]_MOO_SVF_PUTVAR([DEFSRC])])

#  _MOO_SVF_VN_FROM_GIT
#    save 'git describe' in moo_gdesc (for _MOO_SVF_SOURCE_GIT)
#    extract version number from moo_gdesc
#    set and write out
#      moo_{MAJOR, MINOR, RELEASE}
#    if current commit is not the version-tagged one
#      appends +n to moo_EXT if moo_EXT is not already set
#
m4_define([_MOO_SVF_VN_FROM_GIT],
[[moo_gdesc=`git describe --tags --match "v[0-9]*"`
  # extract MAJOR.MINOR.RELEASE-CCOUNT from git describe
  moo_ccount=`expr "X$moo_gdesc" : 'Xv[0-9][0-9]*[.][0-9][0-9]*[.][0-9][0-9]*-\([0-9][0-9]*\)'`
  moo_RELEASE=`expr "X$moo_gdesc" : 'Xv[0-9][0-9]*[.][0-9][0-9]*[.]\([0-9][0-9]*\)'`
  if test "x$moo_RELEASE" = x ; then
      # messed-up repo
      ]AC_MSG_ERROR([cannot parse version tag '$moo_gdesc'])[
  fi
  moo_MINOR=`expr "X$moo_gdesc" : 'Xv[0-9][0-9]*[.]\([0-9][0-9]*\)'`
  moo_MAJOR=`expr "X$moo_gdesc" : 'Xv\([0-9][0-9]*\)'`
  ]
  _MOO_SVF_PUTVAR([MAJOR])
  _MOO_SVF_PUTVAR([MINOR])
  _MOO_SVF_PUTVAR([RELEASE])[
  if test "x$moo_ccount" != x && test "x$moo_EXT" = x; then
      moo_EXT=+$moo_ccount
  fi]])dnl

# _MOO_SVF_PUT_DIFFHASHES
#   given UHASH calculated from current tree diff,
#   prior UFIRST and UHASH{UFIRST..}
#   set+write UFIRST,UHASH{UFIRST..},UCUR
#   append to moo_EXT
#
m4_define([_MOO_SVF_PUT_DIFFHASHES],
[[moo_UCUR=
  if test "x$moo_UHASH" != x; then
    while : ; do]
      _MOO_SVF_FORALL_UHASH([moo_UCUR],[moo_UH],
	 [[test "$moo_UH" = "$moo_UHASH" && break 2]])
      AS_VAR_SET([moo_UHASH$moo_UCUR],[$moo_UHASH])
      AS_VAR_ARITH([moo_U], [$moo_UCUR - 5])[
      if test "$moo_U" -gt "$moo_UFIRST" ; then]
	AS_VAR_ARITH([moo_UFIRST], [$moo_UFIRST + 1])[
      fi
      break
    done
  fi
  if test "x$moo_UCUR" != x || test "$moo_UFIRST" -ne 1 || test "x$moo_UHASH1" != x ; then]
    _MOO_SVF_PUTVAR([UFIRST])
    _MOO_SVF_FORALL_UHASH([moo_u],[],
      [_MOO_SVF_PUTVAR([UHASH$moo_u])])
    AS_VAR_SET([moo_UHASH$moo_u],[])
    _MOO_SVF_PUTVAR([UHASH$moo_u])
    _MOO_SVF_PUTVAR([UCUR])[
  fi
  if test "x$moo_UCUR" != x ; then
    moo_EXT=][$][{moo_EXT}?$moo_UCUR
  fi]])
