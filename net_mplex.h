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

/* This describes the complete set of procedures that a multiplexing wait
 * implementation must provide.
 *
 * The `mplex' abstraction provides a way to wait until it is possible to
 * perform an I/O operation on any of a set of file descriptors without
 * blocking.  Uses of the abstraction always have the following form:
 *
 *      mplex_clear();
 *      { mplex_add_reader(fd)  or  mplex_add_writer(fd) }*
 *      timed_out = mplex_wait(timeout);
 *      { mplex_is_readable(fd)  or  mplex_is_writable(fd) }*
 *
 * The set of file descriptors maintained by the abstraction is referred to
 * below as the `wait set'.  Each file descriptor in the wait set is marked
 * with the kind of I/O (i.e., reading, writing, or both) desired.
 */

#ifndef Net_MPlex_H
#define Net_MPlex_H 1

extern void mplex_clear(void);
				/* Reset the wait set to be empty. */

extern void mplex_add_reader(int fd);
				/* Add the given file descriptor to the wait
				 * set, marked for reading.
				 */

extern void mplex_add_writer(int fd);
				/* Add the given file descriptor to the wait
				 * set, marked for writing.
				 */

extern int mplex_wait(unsigned timeout);
				/* Wait until it is possible either to do the
				 * appropriate kind of I/O on some descriptor
				 * in the wait set or until `timeout' seconds
				 * have elapsed.  Return true iff the timeout
				 * expired without any I/O becoming possible.
				 */

extern int mplex_is_readable(int fd);
				/* Return true iff the most recent mplex_wait()
				 * call terminated (in part) because reading
				 * had become possible on the given descriptor.
				 */

extern int mplex_is_writable(int fd);
				/* Return true iff the most recent mplex_wait()
				 * call terminated (in part) because reading
				 * had become possible on the given descriptor.
				 */

#endif				/* !Net_MPlex_H */

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:18:30  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:19:05  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:18:23  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:53:08  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.2  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.1  1992/09/23  17:14:17  pavel
 * Initial RCS-controlled version.
 */
