/******************************************************************************
  Copyright (c) 1995, 1996 Xerox Corporation.  All rights reserved.
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

#ifndef My_Ctype_H
#define My_Ctype_H 1

#include "config.h"

extern int my_tolower(int);
extern int my_toupper(int);

extern int my_isdigit(int);
extern int my_digitval(int);

extern int my_isspace(int);

extern int my_is_xid_start(int);
extern int my_is_xid_cont(int);

extern int my_is_printable(int);

#define my_isascii(x) ((unsigned int)(x) < 127)

#endif				/* !My_Ctype_H */

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:18:07  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:52  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:05:23  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  05:08:25  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.1  1995/11/30  05:08:19  pavel
 * Initial revision
 */
