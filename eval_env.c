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

#include "config.h"
#include "eval_env.h"
#include "storage.h"
#include "structures.h"
#include "sym_table.h"
#include "utils.h"

/*
 * Keep a pool of rt_envs big enough to hold NUM_READY_VARS variables to
 * avoid lots of malloc/free.
 */
static Var *ready_size_rt_envs;

Var *
new_rt_env(unsigned size)
{
    Var *ret;
    unsigned i;

    if (size <= NUM_READY_VARS && ready_size_rt_envs) {
	ret = ready_size_rt_envs;
	ready_size_rt_envs = ret[0].v.list;
    } else
	ret = mymalloc(MAX(size, NUM_READY_VARS) * sizeof(Var), M_RT_ENV);

    for (i = 0; i < size; i++)
	ret[i].type = TYPE_NONE;

    return ret;
}

void
free_rt_env(Var * rt_env, unsigned size)
{
    register unsigned i;

    for (i = 0; i < size; i++)
	free_var(rt_env[i]);

    if (size <= NUM_READY_VARS) {
	rt_env[0].v.list = ready_size_rt_envs;
	ready_size_rt_envs = rt_env;
    } else
	myfree((void *) rt_env, M_RT_ENV);
}

Var *
copy_rt_env(Var * from, unsigned size)
{
    unsigned i;

    Var *ret = new_rt_env(size);
    for (i = 0; i < size; i++)
	ret[i] = var_ref(from[i]);
    return ret;
}

void
fill_in_rt_consts(Var * env, DB_Version version)
{
    Var v;

    v.type = TYPE_INT;
    v.v.num = (int) TYPE_ERR;
    env[SLOT_ERR] = var_ref(v);
    v.v.num = (int) TYPE_INT;
    env[SLOT_NUM] = var_ref(v);
    v.v.num = (int) _TYPE_STR;
    env[SLOT_STR] = var_ref(v);
    v.v.num = (int) TYPE_OBJ;
    env[SLOT_OBJ] = var_ref(v);
    v.v.num = (int) _TYPE_LIST;
    env[SLOT_LIST] = var_ref(v);

    if (version >= DBV_Float) {
	v.v.num = (int) TYPE_INT;
	env[SLOT_INT] = var_ref(v);
	v.v.num = (int) _TYPE_FLOAT;
	env[SLOT_FLOAT] = var_ref(v);
    }
}

void
set_rt_env_obj(Var * env, int slot, Objid o)
{
    Var v;
    v.type = TYPE_OBJ;
    v.v.obj = o;
    env[slot] = var_ref(v);
}

void
set_rt_env_str(Var * env, int slot, const char *s)
{
    Var v;
    v.type = TYPE_STR;
    v.v.str = s;
    env[slot] = v;
}

void
set_rt_env_var(Var * env, int slot, Var v)
{
    env[slot] = v;
}

char rcsid_rt_env[] = "$Id$";


/*
 * $Log$
 * Revision 1.5  1998/12/14 13:17:44  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.4  1997/07/07 03:24:53  nop
 * Merge UNSAFE_OPTS (r5) after extensive testing.
 *
 * Revision 1.3.2.1  1997/03/20 18:07:50  bjj
 * Add a flag to the in-memory type identifier so that inlines can cheaply
 * identify Vars that need actual work done to ref/free/dup them.  Add the
 * appropriate inlines to utils.h and replace old functions in utils.c with
 * complex_* functions which only handle the types with external storage.
 *
 * Revision 1.3  1997/03/05 08:41:50  bjj
 * A few malloc-friendly changes:  rt_stacks are now centrally allocated/freed
 * so that we can keep a pool of them handy.  rt_envs are similarly pooled.
 * Both revert to malloc/free for large requests.
 *
 * Revision 1.2  1997/03/03 04:18:35  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:44:59  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  07:13:03  pavel
 * Renamed TYPE_NUM to TYPE_INT.  Made fill_in_rt_consts() version-dependent.
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:21:51  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.6  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.5  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.4  1992/10/17  20:24:21  pavel
 * Global rename of strdup->str_dup, strref->str_ref, vardup->var_dup, and
 * varref->var_ref.
 *
 * Revision 1.3  1992/08/31  22:23:15  pjames
 * Changed some `char *'s to `const char *'
 *
 * Revision 1.2  1992/08/28  15:59:54  pjames
 * Changed vardup to varref.
 *
 * Revision 1.1  1992/08/10  16:20:00  pjames
 * Initial RCS-controlled version
 */
