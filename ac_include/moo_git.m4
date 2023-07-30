dnl  -*- autoconf -*-
dnl
# -----------------------------------------
# MOO_PROG_GIT
#   Checks for git being installed (moo_prog_cv_git = 'yes|no')
#   Checks for source in git repo  (moo_git_cv_repo = 'yes|no')
AC_DEFUN([MOO_PROG_GIT],
[AC_CACHE_CHECK([git],[moo_prog_cv_git],[[
moo_git_zerohash=e69de29bb2d1d6434b8b29ae775ad8c2e48c5391
moo_git_cv_repo=no
if test `git hash-object --stdin </dev/null 2<&1` = "$moo_git_zerohash"; then
  moo_prog_cv_git=yes
  if (cd $srcdir ; git status --short -uno 1>/dev/null 2>&1) ; then
    moo_git_cv_repo=yes
  fi
else
  moo_prog_cv_git=no
fi
]])
AC_CACHE_CHECK([for git repository],[moo_git_cv_repo],[])])

# -----------------------------------------------------------
# MOO_PROG_SHA1GIT_HASH
#   Find a file hashing function, preferring 'git hash-object'
#   Sets moo_prog_cv_sha1git_hash =
#     git      => use 'git hash-object'
#     sha1?sum => use that
#     no       => nothing available
#   In the first two cases, sets moo_git_hash_body to
#   something that produces the same result as 'git hash-object'
#
AC_DEFUN([MOO_PROG_SHA1GIT_HASH],
[AC_REQUIRE([MOO_PROG_GIT])dnl
AC_CACHE_CHECK([how to do git hash],[moo_prog_cv_sha1git_hash],[[
if test "$moo_prog_cv_git" = yes ; then
  moo_prog_cv_sha1git_hash=git
elif test `printf "blob 0\000" | shasum  | tr -d ' -' 2<&1` = "$moo_git_zerohash"; then
  moo_prog_cv_sha1git_hash=shasum
elif test `printf "blob 0\000" | sha1sum | tr -d ' -' 2<&1` = "$moo_git_zerohash"; then
  moo_prog_cv_sha1git_hash=sha1sum
else
  moo_prog_cv_sha1git_hash=no
fi]])
AS_CASE([$moo_prog_cv_sha1git_hash],[[
  git]],[[
    moo_git_hash_body='git hash-object "$][1"'
]],[[
  shasum | sha1sum]],[[
    moo_git_hash_body='_moo_a=`cat $][1`]
    AS_VAR_ARITH([_moo_b],[[ 1 + ${@%:@_moo_a} ]])[
    printf "blob %d\000%s\n" $_moo_b "${_moo_a}" | '$moo_prog_cv_sha1git_hash' | tr -d '"' -'"
]],[[
    moo_git_hash_body=']AC_MSG_ERROR([no sha1 hash avaiable])['
]])])

# -----------------------------------------
# MOO_GIT_TREE_HASH(FILE_HASH)
#   given that 'FILE_HASH file' hashes a single file,
#   use git to compute a hash of all differences in the working tree
#   (or '?unmerged?' if a merge is unresolved,
#   or nothing if there are no changes from the most recent commit)
AC_DEFUN([MOO_GIT_TREE_HASH],
[[  moo_dfile=$tmp/moo_vdiff.tmp
  rm -f "$moo_dfile"
  (cd $ac_top_srcdir ; git diff-index --no-abbrev --no-renames HEAD ) | {
    moo_changed=false
    while : ; do
      while read _ _ _ moo_hash moo_UMAD moo_file ; do
	moo_changed=:]
	AS_CASE([$moo_UMAD],[[
          U]],[[
            # unfinished merge; this make is destined to fail
            printf "?unmerged?"
	    break 2
	    ]],[[
          D]],[[
            # deleted file; no contents to hash
	    printf "%s\n" "|$moo_file| deleted" >>"$moo_dfile"
          ]],[[
            if test "$moo_hash" = 0000000000000000000000000000000000000000 ; then
    	      # not yet hashed; need to do it ourselves
	      ]$1[ "$srcdir/$moo_file" >>"$moo_dfile"
	    else
	      printf "%s\n" "$moo_hash" >>"$moo_dfile"
	    fi
        ]])[
      done
      if $moo_changed ; then
	]$1[ "$moo_dfile"
      fi
      break
    done
  }
  rm -f "$moo_dfile"]])
