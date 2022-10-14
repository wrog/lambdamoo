dnl ***************************************************************************
dnl	MOO_ANSI_C
dnl Check whether or not the C compiler handles ANSI C (i.e., allows function
dnl prototypes and the `void *' type) and try to make it do so by adding
dnl command-line options like -Aa and -Xa, which some compilers require.  If
dnl nothing works, abort configuration.
define(MOO_ANSI_C, [
echo "checking that the C compiler handles important ANSI C constructs"
for opt in "" -Aa -Xa -ansi
do
SAVECC="$CC"
CC="$CC $opt"
AC_TEST_PROGRAM([
int main(int argc, char **argv) { void *ptr; return 0; }
],
[have_ansi=1
break],
[CC="$SAVECC"])
done
if test -z "$have_ansi"; then
echo ""
echo "*** Sorry, but I can't figure out how to find an ANSI C compiler here."
echo "*** Compiling this program requires such a compiler."
exit 1
fi
])

dnl --------------------------------------------------------------------------
dnl MOO_ADD_CFLAGS()
dnl
dnl Attempt to add the given option to CFLAGS, if it doesn't break compilation
dnl --------------------------------------------------------------------------
AC_DEFUN([MOO_ADD_CFLAGS],
[AC_MSG_CHECKING([if $CC accepts $1])
 pa_add_cflags__old_cflags="$CFLAGS"
 CFLAGS="$CFLAGS $1"
 AC_TRY_LINK([#include <stdio.h>],
 [printf("Hello, World!\n");],
 AC_MSG_RESULT([yes]),
 AC_MSG_RESULT([no])
 CFLAGS="$pa_add_cflags__old_cflags")])
