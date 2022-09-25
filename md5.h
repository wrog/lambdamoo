/******************************************************************************
  Copyright (c) 1996 Xerox Corporation.  All rights reserved.
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

/* md5.h
   MD5 Message Digest Algorithm implementation.
   Ron Frederick

   Derived from RSA Data Security, Inc. MD5 Message-Digest Algorithm
   See below for original license notice.
 */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
   rights reserved.

   License to copy and use this software is granted provided that it
   is identified as the "RSA Data Security, Inc. MD5 Message-Digest
   Algorithm" in all material mentioning or referencing this software
   or this function.

   License is also granted to make and use derivative works provided
   that such works are identified as "derived from the RSA Data
   Security, Inc. MD5 Message-Digest Algorithm" in all material
   mentioning or referencing the derived work.

   RSA Data Security, Inc. makes no representations concerning either
   the merchantability of this software or the suitability of this
   software for any particular purpose. It is provided "as is"
   without express or implied warranty of any kind.

   These notices must be retained in any copies of any part of this
   documentation and/or software.
 */

#ifndef _MD5_H_
#define _MD5_H_

#include "config.h"

typedef unsigned char uint8;
typedef unsigned32 uint32;

/* MD5 context. */
typedef struct {
    uint32 state[4];		/* state (ABCD) */
    uint32 count[2];		/* number of bits, modulo 2^64 (lsb first) */
    uint8 buffer[64];		/* input buffer */
} md5ctx_t;

void md5_Init(md5ctx_t * context);
void md5_Update(md5ctx_t * context, uint8 * buf, int len);
void md5_Final(md5ctx_t * context, uint8 digest[16]);

#endif

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:18:05  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:52  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 1.1  1996/02/18  23:18:53  pavel
 * Initial revision
 *
 */
