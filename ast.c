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

#include "list.h"
#include "log.h"
#include "program.h"
#include "structures.h"
#include "sym_table.h"
#include "storage.h"
#include "utils.h"

struct entry {
    void *ptr;
    Memory_Type type;
};

static int pool_size, next_pool_slot;
static struct entry *pool;

void
begin_code_allocation(void)
{
    pool_size = 10;
    next_pool_slot = 0;
    pool = mymalloc(pool_size * sizeof(struct entry), M_AST_POOL);
}

void
end_code_allocation(int aborted)
{
    if (aborted) {
	int i;

	for (i = 0; i < next_pool_slot; i++) {
	    if (pool[i].ptr != 0)
		myfree(pool[i].ptr, pool[i].type);
	}
    }
    myfree(pool, M_AST_POOL);
}

static void *
astpool_ref(void *ptr, Memory_Type type)
{
    if (next_pool_slot >= pool_size) {	/* enlarge the pool */
	struct entry *new_pool;
	int i;

	pool_size *= 2;
	new_pool = mymalloc(pool_size * sizeof(struct entry), M_AST_POOL);
	for (i = 0; i < next_pool_slot; i++) {
	    new_pool[i] = pool[i];
	}
	myfree(pool, M_AST_POOL);
	pool = new_pool;
    }
    pool[next_pool_slot].type = type;
    return pool[next_pool_slot++].ptr = ptr;
}

static inline void *
allocate(int size, Memory_Type type)
{
    return astpool_ref(mymalloc(size, type), type);
}

static void
deallocate(void *ptr)
{
    int i;

    for (i = 0; i < next_pool_slot; i++) {
	if (ptr == pool[i].ptr) {
	    myfree(ptr, pool[i].type);
	    pool[i].ptr = 0;
	    return;
	}
    }

    errlog("DEALLOCATE: Unknown pointer deallocated\n");
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
    deallocate(str);
}

#if FLOATS_ARE_BOXED

FlBox
astpool_recv_float(FlBox fnump)
{
    return astpool_ref(fnump, M_FLOAT);
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

void
dealloc_node(void *node)
{
    deallocate(node);
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

static void assign_expr_resume_ids(Expr *, unsigned *, unsigned *);

static void
assign_arg_list_resume_ids(Arg_List * args, unsigned *current_site,
			   unsigned *next_code_unit)
{
    for (; args; args = args->next)
	assign_expr_resume_ids(args->expr, current_site, next_code_unit);
}

static void
assign_expr_resume_ids(Expr * expr, unsigned *current_site,
		       unsigned *next_code_unit)
{
    if (!expr)
	return;

    switch (expr->kind) {
    case EXPR_VAR:
    case EXPR_ID:
    case EXPR_LENGTH:
	break;
    case EXPR_PROP:
    case EXPR_INDEX:
    case EXPR_EQ:
    case EXPR_NE:
    case EXPR_LT:
    case EXPR_LE:
    case EXPR_GT:
    case EXPR_GE:
    case EXPR_IN:
    case EXPR_PLUS:
    case EXPR_MINUS:
    case EXPR_TIMES:
    case EXPR_DIVIDE:
    case EXPR_MOD:
    case EXPR_EXP:
    case EXPR_AND:
    case EXPR_OR:
    case EXPR_BITOR:
    case EXPR_BITXOR:
    case EXPR_BITAND:
    case EXPR_SHL:
    case EXPR_SHR:
    case EXPR_LSHR:
	assign_expr_resume_ids(expr->e.bin.lhs, current_site,
			       next_code_unit);
	assign_expr_resume_ids(expr->e.bin.rhs, current_site,
			       next_code_unit);
	break;
    case EXPR_RANGE:
	assign_expr_resume_ids(expr->e.range.base, current_site,
			       next_code_unit);
	assign_expr_resume_ids(expr->e.range.from, current_site,
			       next_code_unit);
	assign_expr_resume_ids(expr->e.range.to, current_site,
			       next_code_unit);
	break;
    case EXPR_ASGN:
	if (expr->e.bin.lhs->kind == EXPR_SCATTER) {
	    Scatter *sc;

	    assign_expr_resume_ids(expr->e.bin.rhs, current_site,
				   next_code_unit);
	    for (sc = expr->e.bin.lhs->e.scatter; sc; sc = sc->next)
		if (sc->expr)
		    assign_expr_resume_ids(sc->expr, current_site,
					   next_code_unit);
	} else {
	    assign_expr_resume_ids(expr->e.bin.lhs, current_site,
				   next_code_unit);
	    assign_expr_resume_ids(expr->e.bin.rhs, current_site,
				   next_code_unit);
	}
	break;
    case EXPR_CALL:
	assign_arg_list_resume_ids(expr->e.call.args, current_site,
				   next_code_unit);
	expr->e.call.resume_site = (*current_site)++;
	break;
    case EXPR_VERB:
	assign_expr_resume_ids(expr->e.verb.obj, current_site,
			       next_code_unit);
	assign_expr_resume_ids(expr->e.verb.verb, current_site,
			       next_code_unit);
	assign_arg_list_resume_ids(expr->e.verb.args, current_site,
				   next_code_unit);
	expr->e.verb.resume_site = (*current_site)++;
	break;
    case EXPR_NEGATE:
    case EXPR_NOT:
    case EXPR_COMPLEMENT:
	assign_expr_resume_ids(expr->e.expr, current_site, next_code_unit);
	break;
    case EXPR_LIST:
	assign_arg_list_resume_ids(expr->e.list, current_site, next_code_unit);
	break;
    case EXPR_COND:
	assign_expr_resume_ids(expr->e.cond.condition, current_site,
			       next_code_unit);
	assign_expr_resume_ids(expr->e.cond.consequent, current_site,
			       next_code_unit);
	assign_expr_resume_ids(expr->e.cond.alternate, current_site,
			       next_code_unit);
	break;
    case EXPR_CATCH:
	assign_expr_resume_ids(expr->e.catch.try, current_site,
			       next_code_unit);
	assign_arg_list_resume_ids(expr->e.catch.codes, current_site,
				   next_code_unit);
	assign_expr_resume_ids(expr->e.catch.except, current_site,
			       next_code_unit);
	break;
    case EXPR_SCATTER:
	/* Handled as the left-hand side of EXPR_ASGN. */
	break;
    default:
	panic("Unknown expression kind in ASSIGN_EXPR_RESUME_IDS()");
    }
}

static void
assign_stmt_resume_ids(Stmt * stmt, unsigned *current_site,
		       unsigned *next_code_unit, unsigned *current_loop)
{
    for (; stmt; stmt = stmt->next) {
	switch (stmt->kind) {
	case STMT_COND:
	    {
		Cond_Arm *arm;

		for (arm = stmt->s.cond.arms; arm; arm = arm->next) {
		    assign_expr_resume_ids(arm->condition, current_site,
					   next_code_unit);
		    assign_stmt_resume_ids(arm->stmt, current_site,
					   next_code_unit, current_loop);
		}
		assign_stmt_resume_ids(stmt->s.cond.otherwise, current_site,
				       next_code_unit, current_loop);
	    }
	    break;
	case STMT_LIST:
	    stmt->loop_ordinal = (*current_loop)++;
	    assign_expr_resume_ids(stmt->s.list.expr, current_site,
				   next_code_unit);
	    assign_stmt_resume_ids(stmt->s.list.body, current_site,
				   next_code_unit, current_loop);
	    break;
	case STMT_RANGE:
	    stmt->loop_ordinal = (*current_loop)++;
	    assign_expr_resume_ids(stmt->s.range.from, current_site,
				   next_code_unit);
	    assign_expr_resume_ids(stmt->s.range.to, current_site,
				   next_code_unit);
	    assign_stmt_resume_ids(stmt->s.range.body, current_site,
				   next_code_unit, current_loop);
	    break;
	case STMT_WHILE:
	    stmt->loop_ordinal = (*current_loop)++;
	    assign_expr_resume_ids(stmt->s.loop.condition, current_site,
				   next_code_unit);
	    assign_stmt_resume_ids(stmt->s.loop.body, current_site,
				   next_code_unit, current_loop);
	    break;
	case STMT_FORK:
	    {
		unsigned fork_site = 1, fork_loop = 1;

		assign_expr_resume_ids(stmt->s.fork.time, current_site,
				       next_code_unit);
		stmt->s.fork.code_unit = (*next_code_unit)++;
		assign_stmt_resume_ids(stmt->s.fork.body, &fork_site,
				       next_code_unit, &fork_loop);
	    }
	    break;
	case STMT_EXPR:
	case STMT_RETURN:
	    assign_expr_resume_ids(stmt->s.expr, current_site,
				   next_code_unit);
	    break;
	case STMT_TRY_EXCEPT:
	    {
		Except_Arm *except;

		assign_stmt_resume_ids(stmt->s.catch.body, current_site,
				       next_code_unit, current_loop);
		for (except = stmt->s.catch.excepts; except;
		     except = except->next) {
		    assign_arg_list_resume_ids(except->codes, current_site,
					       next_code_unit);
		    assign_stmt_resume_ids(except->stmt, current_site,
					   next_code_unit, current_loop);
		}
	    }
	    break;
	case STMT_TRY_FINALLY:
	    assign_stmt_resume_ids(stmt->s.finally.body, current_site,
				   next_code_unit, current_loop);
	    assign_stmt_resume_ids(stmt->s.finally.handler, current_site,
				   next_code_unit, current_loop);
	    break;
	case STMT_BREAK:
	case STMT_CONTINUE:
	    break;
	default:
	    panic("Unknown statement kind in ASSIGN_STMT_RESUME_IDS()");
	}
    }
}

void
assign_resume_ids(Stmt * stmt)
{
    unsigned current_site = 1, next_code_unit = 1, current_loop = 1;

    assign_stmt_resume_ids(stmt, &current_site, &next_code_unit, &current_loop);
}

static void free_expr(Expr *);

static void
free_arg_list(Arg_List * args)
{
    Arg_List *arg, *next_arg;

    for (arg = args; arg; arg = next_arg) {
	next_arg = arg->next;
	free_expr(arg->expr);
	myfree(arg, M_AST);
    }
}

static void
free_scatter(Scatter * sc)
{
    Scatter *next_sc;

    for (; sc; sc = next_sc) {
	next_sc = sc->next;
	if (sc->expr)
	    free_expr(sc->expr);
	myfree(sc, M_AST);
    }
}

static void
free_expr(Expr * expr)
{
    switch (expr->kind) {

    case EXPR_VAR:
	free_var(expr->e.var);
	break;

    case EXPR_ID:
    case EXPR_LENGTH:
	/* Do nothing. */
	break;

    case EXPR_PROP:
    case EXPR_INDEX:
    case EXPR_PLUS:
    case EXPR_MINUS:
    case EXPR_TIMES:
    case EXPR_DIVIDE:
    case EXPR_MOD:
    case EXPR_AND:
    case EXPR_OR:
    case EXPR_EQ:
    case EXPR_NE:
    case EXPR_LT:
    case EXPR_LE:
    case EXPR_GT:
    case EXPR_GE:
    case EXPR_IN:
    case EXPR_ASGN:
    case EXPR_EXP:
    case EXPR_BITAND:
    case EXPR_BITXOR:
    case EXPR_BITOR:
    case EXPR_SHL:
    case EXPR_SHR:
    case EXPR_LSHR:
	free_expr(expr->e.bin.lhs);
	free_expr(expr->e.bin.rhs);
	break;

    case EXPR_COND:
	free_expr(expr->e.cond.condition);
	free_expr(expr->e.cond.consequent);
	free_expr(expr->e.cond.alternate);
	break;

    case EXPR_VERB:
	free_expr(expr->e.verb.obj);
	free_expr(expr->e.verb.verb);
	free_arg_list(expr->e.verb.args);
	break;

    case EXPR_RANGE:
	free_expr(expr->e.range.base);
	free_expr(expr->e.range.from);
	free_expr(expr->e.range.to);
	break;

    case EXPR_CALL:
	free_arg_list(expr->e.call.args);
	break;

    case EXPR_NEGATE:
    case EXPR_NOT:
    case EXPR_COMPLEMENT:
	free_expr(expr->e.expr);
	break;

    case EXPR_LIST:
	free_arg_list(expr->e.list);
	break;

    case EXPR_CATCH:
	free_expr(expr->e.catch.try);
	free_arg_list(expr->e.catch.codes);
	if (expr->e.catch.except)
	    free_expr(expr->e.catch.except);
	break;

    case EXPR_SCATTER:
	free_scatter(expr->e.scatter);
	break;

    default:
	errlog("FREE_EXPR: Unknown Expr_Kind: %d\n", expr->kind);
	break;
    }

    myfree(expr, M_AST);
}

void
free_stmt(Stmt * stmt)
{
    Stmt *next_stmt;
    Cond_Arm *arm, *next_arm;
    Except_Arm *except, *next_e;

    for (; stmt; stmt = next_stmt) {
	next_stmt = stmt->next;

	switch (stmt->kind) {

	case STMT_COND:
	    for (arm = stmt->s.cond.arms; arm; arm = next_arm) {
		next_arm = arm->next;
		free_expr(arm->condition);
		free_stmt(arm->stmt);
		myfree(arm, M_AST);
	    }
	    if (stmt->s.cond.otherwise)
		free_stmt(stmt->s.cond.otherwise);
	    break;

	case STMT_LIST:
	    free_expr(stmt->s.list.expr);
	    free_stmt(stmt->s.list.body);
	    break;

	case STMT_RANGE:
	    free_expr(stmt->s.range.from);
	    free_expr(stmt->s.range.to);
	    free_stmt(stmt->s.range.body);
	    break;

	case STMT_WHILE:
	    free_expr(stmt->s.loop.condition);
	    free_stmt(stmt->s.loop.body);
	    break;

	case STMT_FORK:
	    free_expr(stmt->s.fork.time);
	    free_stmt(stmt->s.fork.body);
	    break;

	case STMT_EXPR:
	case STMT_RETURN:
	    if (stmt->s.expr)
		free_expr(stmt->s.expr);
	    break;

	case STMT_TRY_EXCEPT:
	    free_stmt(stmt->s.catch.body);
	    for (except = stmt->s.catch.excepts; except; except = next_e) {
		next_e = except->next;
		free_arg_list(except->codes);
		free_stmt(except->stmt);
		myfree(except, M_AST);
	    }
	    break;

	case STMT_TRY_FINALLY:
	    free_stmt(stmt->s.finally.body);
	    free_stmt(stmt->s.finally.handler);
	    break;

	case STMT_BREAK:
	case STMT_CONTINUE:
	    break;		/* Nothing extra to free */

	default:
	    errlog("FREE_STMT: unknown Stmt_Kind: %d\n", stmt->kind);
	    break;
	}

	myfree(stmt, M_AST);
    }
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
