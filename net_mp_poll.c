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

/* Multiplexing wait implementation using the System V poll() system call. */

#include <errno.h>
#include "my-poll.h"

#include "log.h"
#include "net_mplex.h"
#include "storage.h"

typedef struct pollfd Port;

static Port *ports = 0;
static unsigned num_ports = 0;
static unsigned max_fd;

void
mplex_clear(void)
{
    int i;

    max_fd = 0;
    for (i = 0; i < num_ports; i++) {
	ports[i].fd = -1;
	ports[i].events = 0;
    }
}

static void
add_common(int fd, unsigned dir)
{
    if (fd >= num_ports) {	/* Grow ports array */
	int new_num = (fd + 9) / 10 * 10 + 1;
	Port *new_ports = mymalloc(new_num * sizeof(Port), M_NETWORK);
	int i;

	for (i = 0; i < num_ports; i++)
	    new_ports[i] = ports[i];

	if (ports != 0)
	    myfree(ports, M_NETWORK);

	ports = new_ports;
	num_ports = new_num;
    }
    ports[fd].fd = fd;
    ports[fd].events |= dir;
    if (fd > max_fd)
	max_fd = fd;
}

void
mplex_add_reader(int fd)
{
    add_common(fd, POLLIN);
}

void
mplex_add_writer(int fd)
{
    add_common(fd, POLLOUT);
}

int
mplex_wait(unsigned timeout)
{
    int result = poll(ports, max_fd + 1, timeout * 1000);

    if (result < 0) {
	if (errno != EINTR)
	    log_perror("Waiting for network I/O");
	return 1;
    } else
	return (result == 0);
}

int
mplex_is_readable(int fd)
{
    return fd <= max_fd && (ports[fd].revents & (POLLIN | POLLHUP)) != 0;
}

int
mplex_is_writable(int fd)
{
    return fd <= max_fd && (ports[fd].revents & POLLOUT) != 0;
}

char rcsid_net_mp_poll[] = "$Id$";

/*
 * $Log$
 * Revision 1.2  1997/03/03 04:19:04  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:02  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:36:15  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:46:12  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.2  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.1  1992/10/03  00:52:26  pavel
 * Initial RCS-controlled version.
 */
