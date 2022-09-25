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

#include "my-ctype.h"
#include <errno.h>
#include "my-fcntl.h"
#include "my-ioctl.h"
#include "my-signal.h"
#include "my-stdio.h"
#include "my-stdlib.h"
#include "my-string.h"
#include "my-unistd.h"

#include "config.h"
#include "exceptions.h"
#include "list.h"
#include "log.h"
#include "net_mplex.h"
#include "net_multi.h"
#include "net_proto.h"
#include "network.h"
#include "options.h"
#include "server.h"
#include "streams.h"
#include "structures.h"
#include "storage.h"
#include "timers.h"
#include "utf.h"
#include "utils.h"

static struct proto proto;
static int eol_length;		/* == strlen(proto.eol_out_string) */

#ifdef EAGAIN
static int eagain = EAGAIN;
#else
static int eagain = -1;
#endif

#ifdef EWOULDBLOCK
static int ewouldblock = EWOULDBLOCK;
#else
static int ewouldblock = -1;
#endif

static int *pocket_descriptors = 0;	/* fds we keep around in case we need
					 * one and no others are left... */

typedef struct text_block {
    struct text_block *next;
    int length;
    char *buffer;
    char *start;
} text_block;

typedef struct nhandle {
    struct nhandle *next, **prev;
    server_handle shandle;
    int rfd, wfd;
    char *name;
    Stream *input;
    int last_input_was_CR;
    int input_suspended;
    text_block *output_head;
    text_block **output_tail;
    int output_length;
    int output_lines_flushed;
    int outbound, binary;
    char excess_utf[4];
    int excess_utf_count;
#if NETWORK_PROTOCOL == NP_TCP
    int client_echo;
#endif
} nhandle;

static nhandle *all_nhandles = 0;

typedef struct nlistener {
    struct nlistener *next, **prev;
    server_listener slistener;
    int fd;
    const char *name;
} nlistener;

static nlistener *all_nlisteners = 0;


typedef struct {
    int fd;
    network_fd_callback readable;
    network_fd_callback writable;
    void *data;
} fd_reg;

static fd_reg *reg_fds = 0;
static int max_reg_fds = 0;

void
network_register_fd(int fd, network_fd_callback readable,
		    network_fd_callback writable, void *data)
{
    int i;

    if (!reg_fds) {
	max_reg_fds = 5;
	reg_fds = mymalloc(max_reg_fds * sizeof(fd_reg), M_NETWORK);
	for (i = 0; i < max_reg_fds; i++)
	    reg_fds[i].fd = -1;
    }
    /* Find an empty slot */
    for (i = 0; i < max_reg_fds; i++)
	if (reg_fds[i].fd == -1)
	    break;
    if (i >= max_reg_fds) {	/* No free slots */
	int new_max = 2 * max_reg_fds;
	fd_reg *new = mymalloc(new_max * sizeof(fd_reg), M_NETWORK);

	for (i = 0; i < new_max; i++)
	    if (i < max_reg_fds)
		new[i] = reg_fds[i];
	    else
		new[i].fd = -1;

	myfree(reg_fds, M_NETWORK);
	i = max_reg_fds;	/* first free slot */
	max_reg_fds = new_max;
	reg_fds = new;
    }
    reg_fds[i].fd = fd;
    reg_fds[i].readable = readable;
    reg_fds[i].writable = writable;
    reg_fds[i].data = data;
}

void
network_unregister_fd(int fd)
{
    int i;

    for (i = 0; i < max_reg_fds; i++)
	if (reg_fds[i].fd == fd)
	    reg_fds[i].fd = -1;
}

static void
add_registered_fds(void)
{
    fd_reg *reg;

    for (reg = reg_fds; reg < reg_fds + max_reg_fds; reg++)
	if (reg->fd != -1) {
	    if (reg->readable)
		mplex_add_reader(reg->fd);
	    if (reg->writable)
		mplex_add_writer(reg->fd);
	}
}

static void
check_registered_fds(void)
{
    fd_reg *reg;

    for (reg = reg_fds; reg < reg_fds + max_reg_fds; reg++)
	if (reg->fd != -1) {
	    if (reg->readable && mplex_is_readable(reg->fd))
		(*reg->readable) (reg->fd, reg->data);
	    if (reg->writable && mplex_is_writable(reg->fd))
		(*reg->writable) (reg->fd, reg->data);
	}
}


static void
free_text_block(text_block * b)
{
    myfree(b->buffer, M_NETWORK);
    myfree(b, M_NETWORK);
}

int
network_set_nonblocking(int fd)
{
#ifdef FIONBIO
    /* Prefer this implementation, since the second one fails on some SysV
     * platforms, including HP/UX.
     */
    int yes = 1;

    if (ioctl(fd, FIONBIO, &yes) < 0)
	return 0;
    else
	return 1;
#else
    int flags;

    if ((flags = fcntl(fd, F_GETFL, 0)) < 0
	|| fcntl(fd, F_SETFL, flags | NONBLOCK_FLAG) < 0)
	return 0;
    else
	return 1;
#endif
}

static int
push_output(nhandle * h)
{
    text_block *b;
    int count;

    if (h->output_lines_flushed > 0) {
	char buf[100];
	int length;

	sprintf(buf,
		"%s>> Network buffer overflow: %u line%s of output to you %s been lost <<%s",
		proto.eol_out_string,
		h->output_lines_flushed,
		h->output_lines_flushed == 1 ? "" : "s",
		h->output_lines_flushed == 1 ? "has" : "have",
		proto.eol_out_string);
	length = strlen(buf);
	count = write(h->wfd, buf, length);
	if (count == length)
	    h->output_lines_flushed = 0;
	else
	    return count >= 0 || errno == eagain || errno == ewouldblock;
    }
    while ((b = h->output_head) != 0) {
	count = write(h->wfd, b->start, b->length);
	if (count < 0)
	    return (errno == eagain || errno == ewouldblock);
	h->output_length -= count;
	if (count == b->length) {
	    h->output_head = b->next;
	    free_text_block(b);
	} else {
	    b->start += count;
	    b->length -= count;
	}
    }
    if (h->output_head == 0)
	h->output_tail = &(h->output_head);
    return 1;
}

static int
pull_input(nhandle * h)
{
    Stream *s = h->input;
    int count;
    char buffer[1024];
    char *ptr, *end;

    ptr = buffer;
    if (h->excess_utf_count) {
        memcpy(buffer, h->excess_utf, h->excess_utf_count);
        ptr += h->excess_utf_count;
    }

    if ((count = read(h->rfd, ptr, sizeof(buffer) - h->excess_utf_count)) > 0) {
	if (h->binary) {
	    stream_add_string(s, raw_bytes_to_binary(buffer, count));
	    server_receive_line(h->shandle, reset_stream(s));
	    h->last_input_was_CR = 0;
            h->excess_utf_count = 0;
	} else {
	    for (ptr = buffer, end = buffer + count; ptr < end && ptr + clearance_utf(*ptr) <= end;) {
		int c = get_utf(&ptr);

		if (my_is_printable(c))
		    stream_add_utf(s, c);
#ifdef INPUT_APPLY_BACKSPACE
		else if (c == 0x08 || c == 0x7F)
		    stream_delete_utf(s);
#endif
		else if (c == '\r' || (c == '\n' && !h->last_input_was_CR))
		    server_receive_line(h->shandle, reset_stream(s));

		h->last_input_was_CR = (c == '\r');
	    }
            if (ptr < end)
            {
                h->excess_utf_count = end - ptr;
                memcpy(h->excess_utf, ptr, end - ptr);
            }
            h->excess_utf_count = end - ptr;
	}
	return 1;
    } else
	return (count == 0 && !proto.believe_eof)
	    || (count < 0 && (errno == eagain || errno == ewouldblock));
}

static nhandle *
new_nhandle(int rfd, int wfd, const char *local_name, const char *remote_name,
	    int outbound)
{
    nhandle *h;
    static Stream *s = 0;

    if (s == 0)
	s = new_stream(100);

    if (!network_set_nonblocking(rfd)
	|| (rfd != wfd && !network_set_nonblocking(wfd)))
	log_perror("Setting connection non-blocking");

    h = mymalloc(sizeof(nhandle), M_NETWORK);

    if (all_nhandles)
	all_nhandles->prev = &(h->next);
    h->next = all_nhandles;
    h->prev = &all_nhandles;
    all_nhandles = h;

    h->rfd = rfd;
    h->wfd = wfd;
    h->input = new_stream(100);
    h->last_input_was_CR = 0;
    h->input_suspended = 0;
    h->output_head = 0;
    h->output_tail = &(h->output_head);
    h->output_length = 0;
    h->output_lines_flushed = 0;
    h->outbound = outbound;
    h->binary = 0;
    h->excess_utf_count = 0;
#if NETWORK_PROTOCOL == NP_TCP
    h->client_echo = 1;
#endif

    stream_printf(s, "%s %s %s",
		  local_name, outbound ? "to" : "from", remote_name);
    h->name = str_dup(reset_stream(s));

    return h;
}

static void
close_nhandle(nhandle * h)
{
    text_block *b, *bb;

    (void) push_output(h);
    *(h->prev) = h->next;
    if (h->next)
	h->next->prev = h->prev;
    b = h->output_head;
    while (b) {
	bb = b->next;
	free_text_block(b);
	b = bb;
    }
    free_stream(h->input);
    proto_close_connection(h->rfd, h->wfd);
    free_str(h->name);
    myfree(h, M_NETWORK);
}

static void
close_nlistener(nlistener * l)
{
    *(l->prev) = l->next;
    if (l->next)
	l->next->prev = l->prev;
    proto_close_listener(l->fd);
    free_str(l->name);
    myfree(l, M_NETWORK);
}

static void
make_new_connection(server_listener sl, int rfd, int wfd,
		    const char *local_name, const char *remote_name,
		    int outbound)
{
    nhandle *h;
    network_handle nh;

    nh.ptr = h = new_nhandle(rfd, wfd, local_name, remote_name, outbound);
    h->shandle = server_new_connection(sl, nh, outbound);
}

static void
get_pocket_descriptors()
{
    int i;

    if (!pocket_descriptors)
	pocket_descriptors =
	    (int *) mymalloc(proto.pocket_size * sizeof(int), M_NETWORK);

    for (i = 0; i < proto.pocket_size; i++) {
	pocket_descriptors[i] = dup(0);
	if (!pocket_descriptors[i]) {
	    log_perror("Can't get a pocket descriptor");
	    panic("Need pocket descriptors to continue");
	}
    }
}

static void
accept_new_connection(nlistener * l)
{
    network_handle nh;
    nhandle *h;
    int rfd, wfd, i;
    const char *host_name;

    switch (proto_accept_connection(l->fd, &rfd, &wfd, &host_name)) {
    case PA_OKAY:
	make_new_connection(l->slistener, rfd, wfd, l->name, host_name, 0);
	break;

    case PA_FULL:
	for (i = 0; i < proto.pocket_size; i++)
	    close(pocket_descriptors[i]);
	if (proto_accept_connection(l->fd, &rfd, &wfd, &host_name) != PA_OKAY)
	    errlog("Can't accept connection even by emptying pockets!\n");
	else {
	    nh.ptr = h = new_nhandle(rfd, wfd, l->name, host_name, 0);
	    server_refuse_connection(l->slistener, nh);
	    close_nhandle(h);
	}
	get_pocket_descriptors();
	break;

    case PA_OTHER:
	/* Do nothing.  The protocol implementation has already logged it. */
	break;
    }
}

static int
enqueue_output(network_handle nh, const char *line, int line_length,
	       int add_eol, int flush_ok)
{
    nhandle *h = nh.ptr;
    int length = line_length + (add_eol ? eol_length : 0);
    char *buffer;
    text_block *block;

    if (h->output_length != 0
	&& h->output_length + length > MAX_QUEUED_OUTPUT) {	/* must flush... */
	int to_flush;
	text_block *b;

	(void) push_output(h);
	to_flush = h->output_length + length - MAX_QUEUED_OUTPUT;
	if (to_flush > 0 && !flush_ok)
	    return 0;
	while (to_flush > 0 && (b = h->output_head)) {
	    h->output_length -= b->length;
	    to_flush -= b->length;
	    h->output_lines_flushed++;
	    h->output_head = b->next;
	    free_text_block(b);
	}
	if (h->output_head == 0)
	    h->output_tail = &(h->output_head);
    }
    buffer = (char *) mymalloc(length * sizeof(char), M_NETWORK);
    block = (text_block *) mymalloc(sizeof(text_block), M_NETWORK);
    memcpy(buffer, line, line_length);
    if (add_eol)
	memcpy(buffer + line_length, proto.eol_out_string, eol_length);
    block->buffer = block->start = buffer;
    block->length = length;
    block->next = 0;
    *(h->output_tail) = block;
    h->output_tail = &(block->next);
    h->output_length += length;

    return 1;
}


/*************************
 * External entry points *
 *************************/

const char *
network_protocol_name(void)
{
    return proto_name();
}

const char *
network_usage_string(void)
{
    return proto_usage_string();
}

int
network_initialize(int argc, char **argv, Var * desc)
{
    if (!proto_initialize(&proto, desc, argc, argv))
	return 0;

    eol_length = strlen(proto.eol_out_string);
    get_pocket_descriptors();

    /* we don't care about SIGPIPE, we notice it in mplex_wait() and write() */
    signal(SIGPIPE, SIG_IGN);

    return 1;
}

enum error
network_make_listener(server_listener sl, Var desc,
		   network_listener * nl, Var * canon, const char **name)
{
    int fd;
    enum error e = proto_make_listener(desc, &fd, canon, name);
    nlistener *l;

    if (e == E_NONE) {
	nl->ptr = l = mymalloc(sizeof(nlistener), M_NETWORK);
	l->fd = fd;
	l->slistener = sl;
	l->name = str_dup(*name);
	if (all_nlisteners)
	    all_nlisteners->prev = &(l->next);
	l->next = all_nlisteners;
	l->prev = &all_nlisteners;
	all_nlisteners = l;
    }
    return e;
}

int
network_listen(network_listener nl)
{
    nlistener *l = nl.ptr;

    return proto_listen(l->fd);
}

int
network_send_line(network_handle nh, const char *line, int flush_ok)
{
    return enqueue_output(nh, line, strlen(line), 1, flush_ok);
}

int
network_send_bytes(network_handle nh, const char *buffer, int buflen,
		   int flush_ok)
{
    return enqueue_output(nh, buffer, buflen, 0, flush_ok);
}

int
network_buffered_output_length(network_handle nh)
{
    nhandle *h = nh.ptr;

    return h->output_length;
}

void
network_suspend_input(network_handle nh)
{
    nhandle *h = nh.ptr;

    h->input_suspended = 1;
}

void
network_resume_input(network_handle nh)
{
    nhandle *h = nh.ptr;

    h->input_suspended = 0;
}

int
network_process_io(int timeout)
{
    nhandle *h, *hnext;
    nlistener *l;

    mplex_clear();
    for (l = all_nlisteners; l; l = l->next)
	mplex_add_reader(l->fd);
    for (h = all_nhandles; h; h = h->next) {
	if (!h->input_suspended)
	    mplex_add_reader(h->rfd);
	if (h->output_head)
	    mplex_add_writer(h->wfd);
    }
    add_registered_fds();

    if (mplex_wait(timeout))
	return 0;
    else {
	for (l = all_nlisteners; l; l = l->next)
	    if (mplex_is_readable(l->fd))
		accept_new_connection(l);
	for (h = all_nhandles; h; h = hnext) {
	    hnext = h->next;
	    if ((mplex_is_readable(h->rfd) && !pull_input(h))
		|| (mplex_is_writable(h->wfd) && !push_output(h))) {
		server_close(h->shandle);
		close_nhandle(h);
	    }
	}
	check_registered_fds();
	return 1;
    }
}

const char *
network_connection_name(network_handle nh)
{
    nhandle *h = (nhandle *) nh.ptr;

    return h->name;
}

void
network_set_connection_binary(network_handle nh, int do_binary)
{
    nhandle *h = nh.ptr;

    h->binary = do_binary;
}

#if NETWORK_PROTOCOL == NP_LOCAL
#  define NETWORK_CO_TABLE(DEFINE, nh, value, _)
       /* No network-specific connection options */

#elif NETWORK_PROTOCOL == NP_TCP
#  define NETWORK_CO_TABLE(DEFINE, nh, value, _)		\
       DEFINE(client-echo, _, TYPE_INT, num,			\
	      ((nhandle *)nh.ptr)->client_echo,			\
	      network_set_client_echo(nh, is_true(value));)	\

void
network_set_client_echo(network_handle nh, int is_on)
{
    nhandle *h = nh.ptr;

    /* These values taken from RFC 854 and RFC 857. */
#define TN_IAC	255		/* Interpret As Command */
#define TN_WILL	251
#define TN_WONT	252
#define TN_ECHO	1

    static char telnet_cmd[4] =
	{TN_IAC, 0, TN_ECHO, 0};

    h->client_echo = is_on;
    if (is_on)
	telnet_cmd[1] = TN_WONT;
    else
	telnet_cmd[1] = TN_WILL;
    enqueue_output(nh, telnet_cmd, 3, 0, 1);
}

#else /* NETWORK_PROTOCOL == NP_SINGLE */

#  error "NP_SINGLE ???"

#endif /* NETWORK_PROTOCOL */


#ifdef OUTBOUND_NETWORK

enum error
network_open_connection(Var arglist, server_listener sl)
{
    int rfd, wfd;
    const char *local_name, *remote_name;
    enum error e;

    e = proto_open_connection(arglist, &rfd, &wfd, &local_name, &remote_name);
    if (e == E_NONE)
	make_new_connection(sl, rfd, wfd,
			    local_name, remote_name, 1);

    return e;
}
#endif

void
network_close(network_handle h)
{
    close_nhandle(h.ptr);
}

void
network_close_listener(network_listener nl)
{
    close_nlistener(nl.ptr);
}

void
network_shutdown(void)
{
    while (all_nhandles)
	close_nhandle(all_nhandles);
    while (all_nlisteners)
	close_nlistener(all_nlisteners);
}

char rcsid_net_multi[] = "$Id$";

/*
 * $Log$
 * Revision 1.6  2006/12/06 23:57:51  wrog
 * New INPUT_APPLY_BACKSPACE option to process backspace/delete characters on nonbinary connections (patch 1571939)
 *
 * Revision 1.5  2005/09/29 18:46:17  bjj
 * Add third argument to open_network_connection() that associates a specific listener object with the new connection.  This simplifies a lot of outbound connection management.
 *
 * Revision 1.4  2004/05/22 01:25:43  wrog
 * merging in WROGUE changes (W_SRCIP, W_STARTUP, W_OOB)
 *
 * Revision 1.3.10.1  2003/06/07 12:59:04  wrog
 * introduced connection_option macros
 *
 * Revision 1.3  1998/12/14 13:18:31  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:19:05  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:02  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.6  1996/05/12  21:29:09  pavel
 * Fixed mis-initialization of "client-echo" option.  Release 1.8.0p5.
 *
 * Revision 2.5  1996/03/10  01:24:18  pavel
 * Added support for `connection_option()'.  Fixed `unused variable'
 * warnings for non-TCP configurations.  Release 1.8.0.
 *
 * Revision 2.4  1996/02/08  06:38:05  pavel
 * Renamed err/logf() to errlog/oklog().  Added memory of client_echo setting
 * for connection_options().  Added network_set_connection_binary() and
 * network_connection_options().  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.3  1996/01/11  07:38:58  pavel
 * Added support for binary I/O.  Added network_buffered_output_length().
 * Removed a few more `unsigned' declarations.  Release 1.8.0alpha5.
 *
 * Revision 2.2  1995/12/31  03:24:08  pavel
 * Moved server-full handling to server.c.  Added support for multiple
 * listening points.  Release 1.8.0alpha4.
 *
 * Revision 2.1  1995/12/28  00:36:05  pavel
 * Changed input-side EOL handling to include CR, LR, and CRLF.
 * Release 1.8.0alpha3.
 *
 * Revision 2.0  1995/11/30  04:45:36  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.16  1993/08/12  21:10:13  pavel
 * Fix long-standing denial-of-service attack vulnerability due to a connection
 * sending an essentially infinite stream of input to the server.
 *
 * Revision 1.15  1993/08/11  03:11:17  pavel
 * -- Fixed a syntax error in the new support for outbound connections.
 * -- Changed some bogus %d's to %u's in calls to *scanf() and *printf(),
 *    guided by warnings from GCC...
 *
 * Revision 1.14  1993/08/04  02:21:48  pavel
 * -- Added support for distinguishing between inbound and outbound
 *    connections.
 * -- Added support for better logging of outbound connections.
 * -- Added check to connection-timeout code to exempt outbound connections.
 *
 * Revision 1.13  1993/08/04  01:33:04  pavel
 * -- Added support for the BSD-style ioctl(fd, FIONBIO, ...) non-blocking I/O
 *    protocol.  The SysV/POSIX-style fcntl(fd, F_GETFL, ...) doesn't work on
 *    BSD sockets on some hybrid systems, including HP/UX.
 * -- Vastly improved the clarity of the `output flushed' message users see
 *    when their output-side network buffer overflows.
 * -- Now accepts tab as a normal input character mapping to itself, necessary
 *    for communicating with Gopher from the MOO.
 * -- Fixed a =/== bug in pull_input().
 * -- Added a log message printed when the server has to refuse a connection
 *    because there aren't any file descriptors left.
 * -- Added support for the new network_listen() protocol.
 *
 * Revision 1.12  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.11  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.10  1992/10/17  20:44:14  pavel
 * Changed to use NONBLOCK_FLAG instead of O_NDELAY to allow of using
 * POSIX-style non-blocking on systems where it is available.
 *
 * Revision 1.9  1992/10/06  18:16:36  pavel
 * Moved non-blocking code to here from individual protocol implementations.
 *
 * Revision 1.8  1992/09/30  06:18:08  pavel
 * Fixed small bug in the handling of the case where even emptying our pockets
 * of extra file descriptors doesn't make it possible to accept a connection.
 *
 * Revision 1.7  1992/09/26  02:22:07  pavel
 * Added support for printing the network protocol name on server start-up.
 *
 * Revision 1.6  1992/09/23  17:17:36  pavel
 * Stripped out all BSD-specific code, instead relying on the net_proto.h
 * and net_mplex.h interfaces.
 *
 * Revision 1.5  1992/09/11  21:21:54  pavel
 * Tracked change to network.h.
 *
 * Revision 1.4  1992/09/04  22:41:47  pavel
 * Fixed some picky ANSI C problems with (const char *)'s.
 *
 * Revision 1.3  1992/08/10  17:22:45  pjames
 * Updated #includes.
 *
 * Revision 1.2  1992/07/20  23:56:16  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS ident. string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
