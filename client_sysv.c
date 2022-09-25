/******************************************************************************
  Copyright (c) 1992, 1995, 1996 Xerox Corporation.  All rights reserved.
  Portions of this code were written by Stephen White, aka ghond.
  Use and copying of this software and preparation of derivative works based
  upon this software are permitted.  Any distribution of this software or
  derivative works must comply with all applicable United States export
  control laws.  This software is made available AS IS, and Xerox Corporation
  makes no warranty about the software, its performance or its conformity to
  any specification.  Any person obtaining a copy of this software is requested
  to send their name and post office or electronic mail address to:
    Pavel Curtis
    Xerox PARC
    3333 Coyote Hill Rd.
    Palo Alto, CA 94304
    Pavel@Xerox.Com
 *****************************************************************************/

/* SYSV/LOCAL MUD client */

#include <errno.h>
#include "my-fcntl.h"
#include "my-signal.h"
#include "my-stdio.h"
#include "my-stdlib.h"
#include "my-string.h"
#include "my-types.h"
#include "my-stat.h"
#include "my-unistd.h"

#include "config.h"
#include "options.h"

int
set_non_blocking(int fd)
{
    int flags;

    if ((flags = fcntl(fd, F_GETFL, 0)) < 0
	|| fcntl(fd, F_SETFL, flags | NONBLOCK_FLAG) < 0)
	return 0;
    else
	return 1;
}

int
set_blocking(int fd)
{
    int flags;

    if ((flags = fcntl(fd, F_GETFL, 0)) < 0
	|| fcntl(fd, F_SETFL, flags & ~NONBLOCK_FLAG) < 0)
	return 0;
    else
	return 1;
}

void
kill_signal(int sig)
{
    set_blocking(0);
    exit(0);
}

void
write_all(int fd, const char *buffer, int length)
{
    while (length) {
	int count = write(fd, buffer, length);

	buffer += count;
	length -= count;
    }
}

void
main(int argc, char **argv)
{
    const char *connect_file = DEFAULT_CONNECT_FILE;
    int server_fifo;
    const char *dir;
    char c2s[1000], s2c[1000], connect[2010];
    int pid = getpid();
    int rfd, wfd;
    int use_home_dir = 0;
    char *prog_name = argv[0];

    while (--argc && **(++argv) == '-') {
	switch ((*argv)[1]) {
	case 'h':
	    use_home_dir = 1;
	    break;

	default:
	    goto usage;
	}
    }

    if (argc == 1)
	connect_file = argv[0];
    else if (argc != 0)
	goto usage;

    signal(SIGTERM, kill_signal);
    signal(SIGINT, kill_signal);
    signal(SIGQUIT, kill_signal);

    if (!use_home_dir || !(dir = getenv("HOME")))
	dir = "/usr/tmp";

    sprintf(c2s, "%s/.c2s-%05d", dir, (int) pid);
    sprintf(s2c, "%s/.s2c-%05d", dir, (int) pid);

    if (mkfifo(c2s, 0000) < 0 || chmod(c2s, 0644) < 0
	|| mkfifo(s2c, 0000) < 0 || chmod(s2c, 0622) < 0) {
	perror("Creating personal FIFOs");
	goto die;
    }
    if ((rfd = open(s2c, O_RDONLY | NONBLOCK_FLAG)) < 0) {
	perror("Opening server-to-client FIFO");
	goto die;
    }
    if ((server_fifo = open(connect_file, O_WRONLY | NONBLOCK_FLAG)) < 0) {
	perror("Opening server FIFO");
	goto die;
    }
    sprintf(connect, "\n%s %s\n", c2s, s2c);
    write(server_fifo, connect, strlen(connect));
    close(server_fifo);

    if ((wfd = open(c2s, O_WRONLY)) < 0) {
	perror("Opening client-to-server FIFO");
	goto die;
    }
    remove(c2s);
    remove(s2c);

    fprintf(stderr, "[Connected to server]\n");

    if (!set_non_blocking(0) || !set_non_blocking(rfd)) {
	perror("Setting connection non-blocking");
	exit(1);
    }
    while (1) {
	char buffer[1024];
	int count, did_some = 0;

	count = read(0, buffer, sizeof(buffer));
	if (count > 0) {
	    did_some = 1;
	    write_all(wfd, buffer, count);
	}
	count = read(rfd, buffer, sizeof(buffer));
	if (count > 0) {
	    did_some = 1;
	    if (buffer[count - 1] == '\0') {
		write_all(1, buffer, count - 1);
		break;
	    } else
		write_all(1, buffer, count);
	}
	if (!did_some)
	    sleep(1);
    }

    set_blocking(0);
    exit(0);

  die:
    remove(c2s);
    remove(s2c);
    exit(1);

  usage:
    fprintf(stderr, "Usage: %s [-h] [server-connect-file]\n", prog_name);
    exit(1);
}

char rcsid_client_sysv[] = "$Id$";

/*
 * $Log$
 * Revision 1.2  1997/03/03 04:18:23  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:02  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.3  1996/03/10  01:11:56  pavel
 * Moved definition of DEFAULT_CONNECT_FILE to options.h.  Release 1.8.0.
 *
 * Revision 2.2  1996/02/08  06:33:33  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.1  1995/12/30  23:58:51  pavel
 * Fixed a longstanding bug where it never worked to specify a connect-file on
 * the command line.  Release 1.8.0alpha4.
 *
 * Revision 2.0  1995/11/30  04:47:54  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.10  1993/12/09  02:19:59  pavel
 * Fixed bug whereby short writes would cause us to lose data.
 *
 * Revision 1.9  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.8  1992/10/23  19:17:22  pavel
 * Moved macro definition of mkfifo() to my-stat.h.
 * Added a `connected' message, printed when the client has finished the
 * hand-shake with the server.
 *
 * Revision 1.7  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.6  1992/10/17  20:18:27  pavel
 * Replace all uses of O_NDELAY by NONBLOCK_FLAG, to allow for the use of
 * POSIX non-blocking where available.
 *
 * Revision 1.5  1992/10/06  20:33:57  pavel
 * Added -h switch to make it create the client FIFOs in the user's home
 * directory.
 *
 * Revision 1.4  1992/10/06  18:23:49  pavel
 * Added missing #include of my-string.h.
 *
 * Revision 1.3  1992/10/02  18:41:24  pavel
 * Now catches SIGINT, SIGTERM, and SIGQUIT in order to restore normal
 * blocking behavior on stdin.
 *
 * Revision 1.2  1992/10/01  17:29:18  pavel
 * Removed use of the BSD-specific select() call.
 *
 * Revision 1.1  1992/09/27  19:23:03  pavel
 * Initial RCS-controlled version.
 */
