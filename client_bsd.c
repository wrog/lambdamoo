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

/* BSD/LOCAL MUD client */

#include <errno.h>
#include "my-socket.h"
#include "my-stdio.h"
#include "my-stdlib.h"
#include "my-string.h"
#include "my-sys-time.h"
#include <sys/un.h>
#include "my-types.h"
#include "my-unistd.h"

#include "config.h"
#include "options.h"

void
main(int argc, char **argv)
{
    const char *connect_file = DEFAULT_CONNECT_FILE;
    int s;
    struct sockaddr_un address;

    if (argc == 2)
	connect_file = argv[1];
    else if (argc != 1) {
	fprintf(stderr, "Usage: %s [server-connect-file]\n", argv[0]);
	exit(1);
    }
    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
	perror("socket");
	exit(1);
    }
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, connect_file);

    if (connect(s, (struct sockaddr *) &address,
		sizeof(address.sun_family) + strlen(connect_file)) < 0) {
	perror("connect");
	exit(1);
    }
    while (1) {
	fd_set input;
	char buffer[1024];

	FD_ZERO(&input);
	FD_SET(0, &input);
	FD_SET(s, &input);

	if (select(s + 1, (void *) &input, 0, 0, 0) < 0) {
	    if (errno != EINTR) {
		perror("select");
		exit(1);
	    }
	} else {
	    if (FD_ISSET(0, &input))
		write(s, buffer, read(0, buffer, sizeof(buffer)));
	    if (FD_ISSET(s, &input)) {
		int count = read(s, buffer, sizeof(buffer));

		if (count == 0)
		    break;
		write(1, buffer, count);
	    }
	}
    }

    exit(0);
}

char rcsid_client_bsd[] = "$Id$";

/*
 * $Log$
 * Revision 1.2  1997/03/03 04:18:23  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:02  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.2  1996/03/10  01:11:49  pavel
 * Moved definition of DEFAULT_CONNECT_FILE to options.h.  Release 1.8.0.
 *
 * Revision 2.1  1996/02/08  06:33:46  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:47:38  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.5  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.4  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.3  1992/10/17  20:17:13  pavel
 * Fix up connect_file defaulting, to eliminate warnings from naive compilers.
 *
 * Revision 1.2  1992/10/06  18:15:34  pavel
 * Added missing #include of my-stdlib.h, for exit().
 *
 * Revision 1.1  1992/09/23  22:35:05  pavel
 * Initial RCS-controlled version.
 */
