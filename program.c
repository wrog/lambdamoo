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
#include "opcode.h"
#include "parser.h"
#include "program.h"
#include "storage.h"
#include "structures.h"
#include "utils.h"

Program *
new_program(void)
{
    Program *p = (Program *) mymalloc(sizeof(Program), M_PROGRAM);

    memset(p, 0, sizeof(Program));
    p->ref_count = 1;
    p->first_lineno = 1;
    p->cached_lineno = 1;
    p->cached_lineno_pc = 0;
    p->cached_lineno_vec = MAIN_VECTOR;
    return p;
}

ResumeKey
invalid_resume_key(void)
{
    ResumeKey key;

    key.code_unit = 0;
    key.site = 0;
    return key;
}

int
resume_key_is_valid(ResumeKey key)
{
    return key.site != 0;
}

static int
same_resume_key(ResumeKey a, ResumeKey b)
{
    return a.code_unit == b.code_unit && a.site == b.site;
}

const ResumePoint *
resume_point_for_key(Program * p, ResumeKey key)
{
    unsigned i;

    for (i = 0; i < p->num_resume_points; i++)
	if (same_resume_key(p->resume_points[i].key, key))
	    return &p->resume_points[i];

    return 0;
}

const ResumePoint *
resume_point_for_program_pc(Program * p, int vector, unsigned pc)
{
    const ResumePoint *result = 0;
    unsigned i;

    for (i = 0; i < p->num_resume_points; i++)
	if (p->resume_points[i].vector == vector
	    && p->resume_points[i].pc == pc) {
	    if (result)
		return 0;
	    result = &p->resume_points[i];
	}

    return result;
}

const ResumePoint *
resume_point_for_program_location(Program * p, int vector, unsigned pc,
				  unsigned error_pc)
{
    unsigned i;

    for (i = 0; i < p->num_resume_points; i++)
	if (p->resume_points[i].vector == vector
	    && p->resume_points[i].pc == pc
	    && p->resume_points[i].error_pc == error_pc)
	    return &p->resume_points[i];

    return 0;
}

int
validate_program_resume_points(Program * p)
{
    unsigned i, j;

    for (i = 0; i < p->num_resume_points; i++) {
	ResumePoint *point = &p->resume_points[i];
	Bytecodes *bc;
	Byte op;
	unsigned frame_slots = 1;

	if (!resume_key_is_valid(point->key)
	    || point->vector < MAIN_VECTOR
	    || (point->vector != MAIN_VECTOR
		&& (unsigned) point->vector >= p->fork_vectors_size))
	    return 0;

	bc = point->vector == MAIN_VECTOR
	    ? &p->main_vector : &p->fork_vectors[point->vector];
	if (point->pc >= bc->size || point->error_pc >= bc->size
	    || point->stack_depth > bc->max_stack
	    || point->flags != RESUME_PRESERVE_TEMP
	    || (point->stack_depth != 0 && point->stack_slots == 0))
	    return 0;

	for (j = 0; j < point->stack_depth; j++) {
	    ResumeStackSlot *slot = &point->stack_slots[j];

	    switch (slot->kind) {
	    case RSS_PENDING_REASON:
		if (j + 1 >= point->stack_depth
		    || point->stack_slots[j + 1].kind != RSS_PENDING_VALUE)
		    return 0;
		frame_slots++;
		break;
	    case RSS_PENDING_VALUE:
		if (j == 0 || point->stack_slots[j - 1].kind != RSS_PENDING_REASON)
		    return 0;
		frame_slots++;
		break;
	    case RSS_VALUE:
		frame_slots++;
		break;
	    case RSS_HANDLER_PC:
	    case RSS_FINALLY:
		if (slot->data >= bc->size)
		    return 0;
		break;
	    case RSS_CATCH:
		if (slot->data == 0)
		    return 0;
		break;
	    default:
		return 0;
	    }
	}
	if (point->frame_slots != frame_slots)
	    return 0;

	op = bc->vector[point->error_pc];
	if ((point->kind == RP_CALL
	     && (op != OP_CALL_VERB || point->pc != point->error_pc + 1))
	    || (point->kind == RP_BUILTIN
		&& (op != OP_BI_FUNC_CALL || point->pc != point->error_pc + 2)))
	    return 0;

	for (j = 0; j < i; j++)
	    if (same_resume_key(p->resume_points[j].key, point->key)
		|| (p->resume_points[j].vector == point->vector
		    && p->resume_points[j].pc == point->pc))
		return 0;
    }

    return 1;
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
    unsigned i, count;

    count = BQM_SIZEOF(Program);
    count += p->main_vector.size;

    for (i = 0; i < p->num_literals; i++)
	count += value_bytes(p->literals[i]);

    count += BQM_SIZEOF(Bytecodes) * p->fork_vectors_size;
    for (i = 0; i < p->fork_vectors_size; i++)
	count += p->fork_vectors[i].size;

    count += BQM_SIZEOF_PTR_TO_CONST(char) * p->num_var_names;
    for (i = 0; i < p->num_var_names; i++)
	count += memo_strlen(p->var_names[i]) + 1;

    count += BQM_SIZEOF(ResumeLoop) * p->num_resume_loops;
    count += BQM_SIZEOF(ResumePoint) * p->num_resume_points;
    for (i = 0; i < p->num_resume_points; i++)
	count += BQM_SIZEOF(ResumeStackSlot)
	    * p->resume_points[i].stack_depth;

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

	for (i = 0; i < p->num_resume_points; i++)
	    if (p->resume_points[i].stack_slots)
		myfree(p->resume_points[i].stack_slots, M_PROGRAM);
	if (p->resume_loops)
	    myfree(p->resume_loops, M_PROGRAM);
	if (p->resume_points)
	    myfree(p->resume_points, M_PROGRAM);

	myfree(p, M_PROGRAM);
    }
}


/*
 * $Log$
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
