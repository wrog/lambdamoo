# -*- autoconf -*-

# ********************************************************************
#  MOO_SYS_FIFO
#  check whether fifo is available as a file type
#    cache variable:  moo_sys_cv_fifo
#
AC_DEFUN([MOO_SYS_FIFO],
  [AC_CACHE_CHECK([whether FIFOs are available], [moo_sys_cv_fifo],
                  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/stat.h>
#if (!defined(S_IFIFO) || defined(NeXT))
/* The NeXT claims to have FIFOs, but using them panics the kernel... */
#emote no FIFOs
#endif
]])], [[moo_sys_cv_fifo=yes]], [[moo_sys_cv_fifo=no]])])])

m4_define([_MOO_NET_FUD],
  [AC_CACHE_CHECK([$3],[$2],
     [AC_RUN_IFELSE([AC_LANG_SOURCE([$4])],[$2[=yes]],[$2[=no]])])
  m4_ifval([$1],[[test "$]$2[" = yes &&]
    AC_DEFINE([$1])])
])
# *******************************************************************
#  MOO_NET_PREREQUISITES
#   check prerequisites for networking
#   needed for */NS_BSD
#     cache:  ac_cv_header_sys_socket_h
#     cache:  ac_cv_search_accept
#   needed for NP_TCP/*
#     cache:  ac_cv_search_gethostbyname
#   needed for NP_TCP/NS_SYSV
#     cache:  ac_cv_search_t_open
#   (amend LIBS only if network protocol/style needs it)
#
AC_DEFUN([MOO_NET_PREREQUISITES],
[AC_CHECK_HEADER([sys/socket.h])
[_moo_save_LIBS=$LIBS]
AC_SEARCH_LIBS([gethostbyname],[nsl nsl_s],
[AS_IF([[test x"$enable_def_NETWORK_PROTOCOL" = xNP_TCP]],[],[[LIBS=$_moo_save_LIBS]])])
[_moo_save_LIBS=$LIBS]
AC_SEARCH_LIBS([accept],[socket 'socket -lnsl' inet],
[AS_IF([[test x"$enable_def_NETWORK_STYLE" = xNS_BSD]],[],[[LIBS=$_moo_save_LIBS]])])
[_moo_save_LIBS=$LIBS]
AC_SEARCH_LIBS([t_open],[nsl nsl_s],
[AS_IF([[test x"$enable_def_NETWORK_PROTOCOL" = xNP_TCP && test x"$enable_def_NETWORK_STYLE" = xNS_SYSV]],[],[[LIBS=$_moo_save_LIBS]])])
])dnl
# *******************************************************************
#  MOO_NET_FIFO_WORKS
#  check the ways in which FIFOs can be used for sysv networking
#  (1) cache:  moo_net_cv_fifo_fstat
#      #define FSTAT_WORKS_ON_FIFOS
#  (2) cache:  moo_net_cv_fifo_select
#      #define SELECT_WORKS_ON_FIFOS
#  (3) cache:  moo_net_cv_fifo_poll
#      #define POLL_WORKS_ON_FIFOS
#
AC_DEFUN([MOO_NET_FIFO_WORKS],
  [AC_REQUIRE([MOO_SYS_FIFO])dnl
AS_VAR_IF([moo_sys_cv_fifo],[no],[[
  moo_net_cv_fifo_fstat=no
  moo_net_cv_fifo_select=no
  moo_net_cv_fifo_poll=no
]],[dnl
_MOO_NET_FUD([FSTAT_WORKS_ON_FIFOS],[moo_net_cv_fifo_fstat],
    [[whether or not fstat() can tell how much data is in a FIFO]],[[
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
main()
{
  int	rfd, wfd, result; struct stat st;
  unlink("/tmp/conftest-fifo");
  result = (mknod("/tmp/conftest-fifo", 0666 | S_IFIFO, 0) < 0
	    || (rfd = open("/tmp/conftest-fifo", O_RDONLY | O_NDELAY)) < 0
	    || (wfd = open("/tmp/conftest-fifo", O_WRONLY)) < 0
	    || write(wfd, "foo", 3) != 3
	    || fstat(rfd, &st) < 0
	    || st.st_size != 3);
  unlink("/tmp/conftest-fifo");
  exit(result);
}
]])
  _MOO_NET_FUD([SELECT_WORKS_ON_FIFOS],[moo_net_cv_fifo_select],
    [[whether or not select() can be used on FIFOs]],[[
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef FD_ZERO
#define	NFDBITS		(sizeof(fd_set)*8)
#define	FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))
#define	FD_SET(n, p)	((p)->fds_bits[0] |= (1L<<((n)%NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[0] &  (1L<<((n)%NFDBITS)))
#endif /* FD_ZERO */
main()
{
  int	rfd, wfd, result; fd_set input; struct timeval tv;
  tv.tv_sec = 2;
  tv.tv_usec = 0;
  unlink("/tmp/conftest-fifo");
  result = (mknod("/tmp/conftest-fifo", 0666 | S_IFIFO, 0) < 0
	    || (rfd = open("/tmp/conftest-fifo", O_RDONLY | O_NDELAY)) < 0
	    || (wfd = open("/tmp/conftest-fifo", O_WRONLY)) < 0
	    || (FD_ZERO(&input), FD_SET(rfd, &input),
		select(rfd + 1, &input, 0, 0, &tv) != 0)
	    || write(wfd, "foo", 3) != 3
	    || (FD_ZERO(&input), FD_SET(rfd, &input),
		select(rfd + 1, &input, 0, 0, &tv) != 1)
	    || !FD_ISSET(rfd, &input));
  unlink("/tmp/conftest-fifo");
  exit(result);
}
]])
  _MOO_NET_FUD([POLL_WORKS_ON_FIFOS],[moo_net_cv_fifo_poll],
    [[whether or not poll() can be used on FIFOs]],[[
#include <sys/types.h>
#include <poll.h>
#include <sys/stat.h>
#include <fcntl.h>
main()
{
  int	rfd, wfd, result; struct pollfd fds[1];
  unlink("/tmp/conftest-fifo");
  result = (mknod("/tmp/conftest-fifo", 0666 | S_IFIFO, 0) < 0
	    || (rfd = open("/tmp/conftest-fifo", O_RDONLY | O_NDELAY)) < 0
	    || (wfd = open("/tmp/conftest-fifo", O_WRONLY)) < 0
	    || write(wfd, "foo", 3) != 3
	    || (fds[0].fd = rfd, fds[0].events = POLLIN, poll(fds, 1, 1) != 1)
	    || (fds[0].revents & POLLIN) == 0);
  unlink("/tmp/conftest-fifo");
  exit(result);
}
]])])])dnl
dnl
# *******************************************************************
#  MOO_NET_POSIX_NONBLOCKING
#  check whether or not select() can be used on FIFOs
#    cache:  moo_net_cv_posix_nonblocking
#    #define POSIX_NONBLOCKING_WORKS
#
AC_DEFUN([MOO_NET_POSIX_NONBLOCKING],
  [AC_REQUIRE([MOO_SYS_FIFO])dnl
AS_VAR_IF([moo_sys_cv_fifo],[no],[[
  moo_net_cv_posix_nonblocking=no
]],[
  _MOO_NET_FUD([POSIX_NONBLOCKING_WORKS],[moo_net_cv_posix_nonblocking],
    [[whether POSIX-style non-blocking I/O works]],[[
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
handler(int sig) { }
main ()
{
  int	rfd, wfd, flags, result; char buffer[10];
  unlink("/tmp/conftest-fifo");
  signal(SIGALRM, handler);
  result = (mknod("/tmp/conftest-fifo", 0666 | S_IFIFO, 0) < 0
	    || (rfd = open("/tmp/conftest-fifo", O_RDONLY | O_NONBLOCK)) < 0
	    || (wfd = open("/tmp/conftest-fifo", O_WRONLY)) < 0
	    || (flags = fcntl(rfd, F_GETFL, 0)) < 0
	    || fcntl(rfd, F_SETFL, flags | O_NONBLOCK) < 0
	    || (alarm(3), read(rfd, buffer, 10) >= 0)
	    || (alarm(0), errno != EAGAIN));
  unlink("/tmp/conftest-fifo");
  exit(result);
}
]])])])
