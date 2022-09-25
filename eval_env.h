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

#ifndef Eval_env_h
#define Eval_env_h 1

#include "config.h"
#include "structures.h"
#include "version.h"

extern Var *new_rt_env(unsigned size);
extern void free_rt_env(Var * rt_env, unsigned size);
extern Var *copy_rt_env(Var * from, unsigned size);

void set_rt_env_obj(Var * env, int slot, Objid o);
void set_rt_env_str(Var * env, int slot, const char *s);
void set_rt_env_var(Var * env, int slot, Var v);

void fill_in_rt_consts(Var * env, DB_Version);

#endif

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:17:45  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:36  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:02  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:26:34  pavel
 * Made fill_in_rt_consts() version-dependent.  Updated copyright notice for
 * 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:50:38  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.4  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.3  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.2  1992/08/31  22:22:36  pjames
 * Changed some `char *'s to `const char *'
 *
 * Revision 1.1  1992/08/10  16:20:00  pjames
 * Initial RCS-controlled version
 */
