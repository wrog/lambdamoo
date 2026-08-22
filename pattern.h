/******************************************************************************
  Copyright (c) 1995, 1996 Xerox Corporation.  All rights reserved.
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

#ifndef Pattern_H
#define Pattern_H 1

#include "config.h"

typedef struct {
    int start, end;
} Match_Indices;

/*
 * Number of match index entries produced by match_pattern(), including the
 * whole match at index zero.  This controls matching storage only; supporting
 * more groups would also require changes to the pattern translators and their
 * backreference syntax, among other things.
 */
#define MATCH_GROUP_LIMIT 10

typedef struct {
    void *ptr;
} Pattern;

typedef enum {
    MATCH_SUCCEEDED, MATCH_FAILED, MATCH_ABORTED
} Match_Result;

extern Pattern new_pattern(const char *pattern, int case_matters);
extern Match_Result match_pattern(Pattern p, const char *string,
				Match_Indices * indices, int is_reverse);
extern void free_pattern(Pattern p);

#endif		/* !Pattern_H */

/*
 * $Log$
 * Revision 2.1  1996/02/08  06:14:35  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  05:07:35  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.1  1995/11/30  05:07:25  pavel
 * Initial revision
 */
