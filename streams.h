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

#ifndef Stream_h
#define Stream_h 1

#include "config.h"

typedef struct {
    char *buffer;
    int buflen;
    int current;
} Stream;

extern Stream *new_stream(int size);
extern void stream_add_char(Stream *, char);
extern int stream_add_utf(Stream *, int);
extern void stream_delete_char(Stream *);
extern void stream_delete_utf(Stream *);
extern void stream_add_string(Stream *, const char *);
extern void stream_add_bytes(Stream *, const char *, int);
extern void stream_printf(Stream *, const char *,...) FORMAT(printf,2,3);
extern void free_stream(Stream *);
extern char *stream_contents(Stream *);
extern char *reset_stream(Stream *);
extern int stream_length(Stream *);

#endif

/*
 * $Log$
 * Revision 1.7  2010/04/23 05:10:55  wrog
 * remove max=0 meaning no limit
 *
 * Revision 1.4  2006/12/06 23:57:51  wrog
 * New INPUT_APPLY_BACKSPACE option to process backspace/delete characters on nonbinary connections (patch 1571939)
 *
 * Revision 1.3  1998/12/14 13:19:02  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:19:28  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:12:33  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:55:31  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.3  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.2  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
