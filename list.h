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

#include "structures.h"

extern Var listappend(Var list, Var value);
extern Var listinsert(Var list, Var value, int pos);
extern Var listdelete(Var list, int pos);
extern Var listset(Var list, Var value, int pos);
extern Var listrangeset(Var list, int from, int to, Var value);
extern Var listconcat(Var first, Var second);
extern int ismember(Var value, Var list, int case_matters);
extern Var setadd(Var list, Var value);
extern Var setremove(Var list, Var value);
extern Var sublist(Var list, int lower, int upper);
extern Var strrangeset(Var list, int from, int to, Var value);
extern Var substr(Var str, int lower, int upper);
extern Var strget(Var str, Var i);
extern Var new_list(int size);
extern const char *value2str(Var);
extern const char *value_to_literal(Var);

/*
 * $Log$
 * Revision 1.4  2010/03/30 23:06:51  wrog
 * value_to_literal() replaced by unparse_value()
 *
 * Revision 1.3  1998/12/14 13:17:58  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:47  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:03  nop
 * LambdaMOO 1.8.0p5
 *
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
