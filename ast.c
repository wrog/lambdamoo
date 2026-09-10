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

#include "my-string.h"

#include "arena.h"
#include "list.h"
#include "log.h"
#include "program.h"
#include "storage.h"
#include "structures.h"
#include "sym_table.h"
#include "utils.h"

static Arena *ast_arena = NULL;

static char **tracked_strings = NULL;
static int tracked_strings_count = 0;
static int tracked_strings_size = 0;

#if FLOATS_ARE_BOXED
static FlBox *tracked_floats = NULL;
static int tracked_floats_count = 0;
static int tracked_floats_size = 0;
#endif

static void
track_string(char *str)
{
    if (tracked_strings_count >= tracked_strings_size) {
	tracked_strings_size =
	    tracked_strings_size == 0 ? 16 : tracked_strings_size * 2;
	tracked_strings = myrealloc(tracked_strings,
				    tracked_strings_size * sizeof(char *),
				    M_AST_POOL);
    }
    tracked_strings[tracked_strings_count++] = str;
}

#if FLOATS_ARE_BOXED
static void
track_float(FlBox fnum)
{
    if (tracked_floats_count >= tracked_floats_size) {
	tracked_floats_size = tracked_floats_size == 0 ? 8 : tracked_floats_size * 2;
	tracked_floats = myrealloc(tracked_floats,
				   tracked_floats_size * sizeof(FlBox),
				   M_AST_POOL);
    }
    tracked_floats[tracked_floats_count++] = fnum;
}
#endif

static void
release_ast(void)
{
    int i;

    for (i = 0; i < tracked_strings_count; i++)
	free_str(tracked_strings[i]);

#if FLOATS_ARE_BOXED
    for (i = 0; i < tracked_floats_count; i++) {
	Var value;

	value.type = TYPE_FLOAT;
	value.v.fnum = tracked_floats[i];
	free_var(value);
    }
#endif

    if (tracked_strings) {
	myfree(tracked_strings, M_AST_POOL);
	tracked_strings = NULL;
    }
    tracked_strings_count = 0;
    tracked_strings_size = 0;

#if FLOATS_ARE_BOXED
    if (tracked_floats) {
	myfree(tracked_floats, M_AST_POOL);
	tracked_floats = NULL;
    }
    tracked_floats_count = 0;
    tracked_floats_size = 0;
#endif

    if (ast_arena) {
	arena_destroy(ast_arena);
	ast_arena = NULL;
    }
}

void
begin_code_allocation(void)
{
    ast_arena = arena_create(4096, M_AST_POOL);
}

void
end_code_allocation(int aborted)
{
    if (aborted)
	release_ast();
}

static inline void *
allocate(int size, Memory_Type type)
{
    if (type == M_STRING) {
	char *str = mymalloc(size, M_STRING);
	track_string(str);
	return str;
    }
    (void)type;
    return arena_alloc(ast_arena, size);
}

char *
alloc_string(const char *buffer)
{
    char *string = allocate(strlen(buffer) + 1, M_STRING);

    strcpy(string, buffer);
    return string;
}

void
dealloc_string(char *str)
{
    int i;
    /* Backwards; most strings are released right after they're added */
    for (i = tracked_strings_count; i-- > 0;) {
	if (tracked_strings[i] == str) {
	    free_str(str);
	    tracked_strings[i] = tracked_strings[--tracked_strings_count];
	    return;
	}
    }
    free_str(str);
}

#if FLOATS_ARE_BOXED

FlBox
astpool_recv_float(FlBox fnump)
{
    track_float(fnump);
    return fnump;
}

/*
 *  Theoretically, we could also use the rigorously correct version of
 *
    void flbox_negate_in_place(FlBox *fp)
    {
	FlBox fpnew = allocate(sizeof(FlNum), M_FLOAT);
	*fpnew = -fl_unbox(*fp);
	deallocate(*fp);
	*fp = fpnew;
    }
 *  except this is unbelievably painful behind the scenes and would
 *  *still* fail if floats were interned because end_code_allocation()
 *  does not check reference counts in the 'aborted' case.
 */
#endif	/* FLOATS_ARE_BOXED */

Var
astpool_ref_var(Var value)
{
    value = var_ref(value);

    if (value.type == TYPE_STR)
	track_string((char *) value.v.str);
#if FLOATS_ARE_BOXED
    else if (value.type == TYPE_FLOAT)
	track_float(value.v.fnum);
#endif

    return value;
}

Stmt *
alloc_stmt(enum Stmt_Kind kind)
{
    Stmt *result = allocate(sizeof(Stmt), M_AST);

    result->kind = kind;
    result->next = 0;
    return result;
}

Cond_Arm *
alloc_cond_arm(Expr * condition, Stmt * stmt)
{
    Cond_Arm *result = allocate(sizeof(Cond_Arm), M_AST);

    result->condition = condition;
    result->stmt = stmt;
    result->next = 0;
    return result;
}

Except_Arm *
alloc_except(int id, Arg_List * codes, Stmt * stmt)
{
    Except_Arm *result = allocate(sizeof(Except_Arm), M_AST);

    result->id = id;
    result->codes = codes;
    result->stmt = stmt;
    result->label = 0;
    result->next = 0;
    return result;
}

Expr *
alloc_expr(enum Expr_Kind kind)
{
    Expr *result = allocate(sizeof(Expr), M_AST);

    result->kind = kind;
    return result;
}

Expr *
alloc_var(var_type type)
{
    Expr *result = alloc_expr(EXPR_VAR);

    result->e.var.type = type;
    return result;
}

Expr *
alloc_binary(enum Expr_Kind kind, Expr * lhs, Expr * rhs)
{
    Expr *result = alloc_expr(kind);

    result->e.bin.lhs = lhs;
    result->e.bin.rhs = rhs;
    return result;
}

Expr *
alloc_verb(Expr * obj, Expr * verb, Arg_List * args)
{
    Expr *result = alloc_expr(EXPR_VERB);

    result->e.verb.obj = obj;
    result->e.verb.verb = verb;
    result->e.verb.args = args;
    return result;
}

Arg_List *
alloc_arg_list(enum Arg_Kind kind, Expr * expr)
{
    Arg_List *result = allocate(sizeof(Arg_List), M_AST);

    result->kind = kind;
    result->expr = expr;
    result->next = 0;
    return result;
}

Scatter *
alloc_scatter(enum Scatter_Kind kind, int id, Expr * expr)
{
    Scatter *sc = allocate(sizeof(Scatter), M_AST);

    sc->kind = kind;
    sc->id = id;
    sc->expr = expr;
    sc->next = 0;
    sc->label = sc->next_label = 0;

    return sc;
}

void
free_stmt(Stmt * stmt UNUSED_)
{
    release_ast();
}


/*
 * $Log$
 * Revision 2.4  1996/02/08  07:11:54  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1 (again).
 *
 * Revision 2.3  1996/02/08  05:49:08  pavel
 * Renamed err/logf() to errlog/oklog().  Added support for floating-point and
 * the X^Y expression.  Added named WHILE loops and the BREAK and CONTINUE
 * statements.  Release 1.8.0beta1.
 *
 * Revision 2.2  1996/01/16  07:11:46  pavel
 * Added alloc/free for scatters.  Release 1.8.0alpha6.
 *
 * Revision 2.1  1995/12/31  03:14:13  pavel
 * Added EXPR_LENGTH case to free_expr().  Release 1.8.0alpha4.
 *
 * Revision 2.0  1995/11/30  04:16:25  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.12  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.11  1992/10/23  19:16:20  pavel
 * Eliminated all uses of the useless macro NULL.
 *
 * Revision 1.10  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.9  1992/08/31  22:21:28  pjames
 * Changed some `char *'s to `const char *'
 *
 * Revision 1.8  1992/08/28  23:12:13  pjames
 * Added ASGN_RANGE arm of `free_expr()'.
 *
 * Revision 1.7  1992/08/28  16:11:25  pjames
 * Changed myfree(*, M_STRING) to free_str(*).
 * Removed free_ast_var.  Use free_var instead.
 * Removed ak_dealloc_string.  Use free_str instead.
 *
 * Revision 1.6  1992/08/14  00:01:17  pavel
 * Converted to a typedef of `var_type' = `enum var_type'.
 *
 * Revision 1.5  1992/08/10  16:53:07  pjames
 * Updated #includes.
 *
 * Revision 1.4  1992/07/30  21:20:25  pjames
 * Checks for NULL before freeing M_STRINGS.
 *
 * Revision 1.3  1992/07/27  17:57:06  pjames
 * Changed myfree(*, M_AST) to (*, M_STRING) when * was a (char *)
 * because that is how they are allocated.
 *
 * Revision 1.2  1992/07/20  23:48:48  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS identification
 * string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
