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

#ifndef List_H
#define List_H 1

#include "structures.h"
#include "streams.h"


/*---------------------*
 |  List constructors  |
 *---------------------*
 * These consume a refcount on every Var argument
 * (meaning that, post-call, the reference will be
 * attributed to something in the return value or
 * Var will have been deref()ed, maybe reclaiming storage)
 */
extern Var new_list(int size);
extern Var listappend(Var list, Var value);
extern Var listinsert(Var list, Var value, int pos);
extern Var listdelete(Var list, int pos);
extern Var listset(Var list, Var value, int pos);
extern Var listconcat(Var first, Var second);
extern Var setadd(Var list, Var value);
extern Var setremove(Var list, Var value);

/*------------------*
 |  List utilities  |
 *------------------*
 * These do not consume argument refcount
 */
extern int ismember(Var value, Var list, int case_matters);
extern const char *value2str(Var);
extern void unparse_value(Stream *, Var);

/*--------------------*
 |  Subrange get/set  |
 *--------------------*
 * These consume a refcount on every Var argument
 * String from|first and after are byte indices.
 *
 * General rules for From-After APIs:
 *   from|first and after are 1-based indices;
 *   after is the first slot beyond the end of the range;
 *   after == NUM_MIN is allowed as a **special case**
 *     indicating a range ending at NUM_MAX
 *     (equv. window for after is NUM_MIN+1..NUM_MAX+1)
 */

/* Get subrange:
 *   If after != NUM_MIN and first >= after, return empty.
 *   Otherwise, from and (UNum)after-1 must be in 1..length.
 *   list|str refcount is consumed
 */
extern Var sublist(Var list, Num first, Num after);
extern Var substr( Var str,  Num first, Num after);

/* Set subrange:
 *   from <= 1 + length(base) and can be zero or arbitrarily negative;
 *   after is NUM_MIN or positive (arbitrarily large);
 *   result is base[1..max(from-1,0)]+value+base[min(after,$+1)..$]
 *   base and value refcounts are consumed
 */
extern Var listrangeset(Var base_list, Num from, Num after, Var value);
extern Var strrangeset( Var base_str,  Num from, Num after, Var value);


#endif		/* !List_H */

/*
 * $Log$
 * Revision 2.1  1996/02/08  06:23:44  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:51:57  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.3  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.2  1992/08/28  23:20:04  pjames
 * Added `listrangeset()' and `strrangeset()'.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
