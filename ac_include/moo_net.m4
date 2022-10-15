# *******************************************************************
#  MOO_NET_FIFO_WORKS
#  check the ways in which FIFOs can be used for sysv networking
#  (1) #define FSTAT_WORKS_ON_FIFOS
#  (2) #define SELECT_WORKS_ON_FIFOS
#  (3) #define POLL_WORKS_ON_FIFOS
#
AC_DEFUN([MOO_NET_FIFO_WORKS],[dnl
dnl ***************************************************************************
echo "checking whether or not fstat() can tell how much data is in a FIFO"
AC_RUN_IFELSE([AC_LANG_SOURCE([[#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
main()
{
#ifdef NeXT
/* The NeXT claims to have FIFOs, but using them panics the kernel... */
  exit(-1);
#endif
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
]])], [AC_DEFINE([FSTAT_WORKS_ON_FIFOS],[1])], [], [])

dnl ***************************************************************************
echo "checking whether or not select() can be used on FIFOs"
AC_RUN_IFELSE([AC_LANG_SOURCE([[#include <sys/types.h>
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
#ifdef NeXT
/* The NeXT claims to have FIFOs, but using them panics the kernel... */
  exit(-1);
#endif
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
]])], [AC_DEFINE([SELECT_WORKS_ON_FIFOS],[1])],[],[])

dnl ***************************************************************************
echo "checking whether or not poll() can be used on FIFOs"
AC_RUN_IFELSE([AC_LANG_SOURCE([[#include <sys/types.h>
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
]])],[AC_DEFINE([POLL_WORKS_ON_FIFOS],[1])],[],[])
])

# *******************************************************************
#  MOO_NET_POSIX_NONBLOCKING
#  check whether or not select() can be used on FIFOs
#    #define POSIX_NONBLOCKING_WORKS
#
AC_DEFUN([MOO_NET_POSIX_NONBLOCKING],[dnl
dnl ***************************************************************************
echo checking whether POSIX-style non-blocking I/O works
AC_RUN_IFELSE([AC_LANG_SOURCE([[#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
handler(int sig) { }
main ()
{ /* Testing a POSIX feature, so assume FIFOs */
#ifdef NeXT
/* The NeXT claims to have FIFOs, but using them panics the kernel... */
  exit(-1);
#endif
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
]])],[AC_DEFINE([POSIX_NONBLOCKING_WORKS],[1])],[],[])
])
