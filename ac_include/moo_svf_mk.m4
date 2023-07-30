dnl  -*- autoconf -*-

# --------------------------------------------------------------------
#  MOO_SVF_ARG_MAKE
#
#  --(enable|disable)-svf-make
#  whether server_version_full includes a 'make' group
#  default is yes iff GNU make is in use.
#
AC_DEFUN([MOO_SVF_ARG_MAKE],
[AC_REQUIRE([MOO_CHECK_GNU_MAKE])
AC_ARG_ENABLE([svf-make],
[AS_HELP_STRING([--(en|dis)able-svf-make],[include/exclude 'make' group],[30])],
[AS_CASE([$enableval],[[yes]],
AS_CASE([[$moo_cv_make_is_gnumake]],[[no]],
[AC_MSG_ERROR([--enable-svf-make requires GNU make])]))],
[AS_VAR_COPY([enable_svf_make],[moo_cv_make_is_gnumake])])])


# --------------------------------------------------------------------
#  MOO_CHECK_GNU_MAKE
#
#  determines whether $MAKE understands GNU features needed
#  for server_version_full to include a 'make' group
#  (needs to understand ${subst...} and MAKEOVERRIDES)
#
AC_DEFUN([MOO_CHECK_GNU_MAKE],
[AC_REQUIRE([AC_PROG_MAKE_SET])
AC_CACHE_CHECK([make having GNU features],[moo_cv_make_is_gnumake],[[
if "][$][{MAKE-make}" a=b=c=d -f - <<'END' | grep axbxcxd >/dev/null 2>&1
b=][$][{subst =,x,][$][{MAKEOVERRIDES}}
t:
	@echo $(b)
END
then
  moo_cv_make_is_gnumake=yes
else
  moo_cv_make_is_gnumake=no
fi
]])])

# --------------------------------------------------------------------
#  MOO_SVF_SUBST_VERSION_MAKE
#
#  set the Makefile substitution for collecting
#  the version 'make' group, (or make it a no-op
#  if --disable-svf-make is specified)
#

AC_DEFUN([MOO_SVF_SUBST_VERSION_MAKE],
[AC_SUBST_FILE([version_make_frag])
AS_CASE([$enable_svf_make],
[[no]],[[
version_make_frag="$srcdir/ac_include/moo_svf_mk0.mk"]],
[[yes]],[[
version_make_frag="$srcdir/ac_include/moo_svf_mk1.mk"]])])
