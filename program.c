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

#include "ast.h"
#include "exceptions.h"
#include "list.h"
#include "parser.h"
#include "program.h"
#include "storage.h"
#include "structures.h"
#include "utils.h"

Program *
new_program(void)
{
    Program *p = (Program *) mymalloc(sizeof(Program), M_PROGRAM);

    p->ref_count = 1;
    p->first_lineno = 1;
    p->cached_lineno = 1;
    p->cached_lineno_pc = 0;
    return p;
}

Program *
null_program(void)
{
    static Program *p = 0;
    Var code, errors;

    if (!p) {
	code = new_list(0);
	p = parse_list_as_program(code, &errors);
	if (!p)
	    panic("Can't create the null program!");
	free_var(code);
	free_var(errors);
    }
    return p;
}

Program *
program_ref(Program * p)
{
    p->ref_count++;

    return p;
}

int
program_bytes(Program * p)
{
    int i, count;

    count = sizeof(Program);
    count += p->main_vector.size;

    for (i = 0; i < p->num_literals; i++)
	count += value_bytes(p->literals[i]);

    count += sizeof(Bytecodes) * p->fork_vectors_size;
    for (i = 0; i < p->fork_vectors_size; i++)
	count += p->fork_vectors[i].size;

    count += sizeof(const char *) * p->num_var_names;
    for (i = 0; i < p->num_var_names; i++)
	count += memo_strlen(p->var_names[i]) + 1;

    return count;
}

void
free_program(Program * p)
{
    unsigned i;

    p->ref_count--;
    if (p->ref_count == 0) {

	for (i = 0; i < p->num_literals; i++)
	    /* can't be a list--strings and floats need to be freed, though. */
	    free_var(p->literals[i]);
	if (p->literals)
	    myfree(p->literals, M_LIT_LIST);

	for (i = 0; i < p->fork_vectors_size; i++)
	    myfree(p->fork_vectors[i].vector, M_BYTECODES);
	if (p->fork_vectors_size)
	    myfree(p->fork_vectors, M_FORK_VECTORS);

	for (i = 0; i < p->num_var_names; i++)
	    free_str(p->var_names[i]);
	myfree(p->var_names, M_NAMES);

	myfree(p->main_vector.vector, M_BYTECODES);

	myfree(p, M_PROGRAM);
    }
}

char rcsid_program[] = "$Id$";

/*
 * $Log$
 * Revision 1.7  2010/04/22 21:32:01  wrog
 * Avoid using uninitialized Program.cached_lineno_vec (rob@mars.org)
 *
 * Revision 1.6  2006/09/07 00:55:02  bjj
 * Add new MEMO_STRLEN option which uses the refcounting mechanism to
 * store strlen with strings.  This is basically free, since most string
 * allocations are rounded up by malloc anyway.  This saves lots of cycles
 * computing strlen.  (The change is originally from jitmoo, where I wanted
 * inline range checks for string ops).
 *
 * Revision 1.5  1998/12/14 13:18:48  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.4  1997/07/07 03:24:54  nop
 * Merge UNSAFE_OPTS (r5) after extensive testing.
 *
 * Revision 1.3.2.1  1997/06/05 09:00:00  bjj
 * Cache one pc/lineno pair with each Program.  Hopefully most programs that
 * fail multiple times usually do it on the same line!
 *
 * Revision 1.3  1997/03/08 06:25:42  nop
 * 1.8.0p6 merge by hand.
 *
 * Revision 1.2  1997/03/03 04:19:17  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:01  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.4  1997/03/04 04:36:18  eostrom
 * Fixed memory leak in free_program().
 *
 * Revision 2.3  1996/04/08  00:41:16  pavel
 * Corrected an error in the computation of `program_bytes()'.
 * Release 1.8.0p3.
 *
 * Revision 2.2  1996/02/08  06:54:31  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.1  1995/12/11  08:04:20  pavel
 * Added `null_program()' and `program_bytes()'.  Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  04:29:58  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.6  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.5  1992/08/28  16:17:41  pjames
 * Changed `ak_dealloc_string()' to `free_str()'.
 *
 * Revision 1.4  1992/08/10  16:54:22  pjames
 * Updated #includes.
 *
 * Revision 1.3  1992/07/27  18:16:27  pjames
 * Changed name of ct_env to var_names, const_env to literals, and
 * f_vectors to fork_vectors.
 *
 * Revision 1.2  1992/07/21  00:05:45  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS ident. string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
