/******************************************************************************
  Copyright (c) 1994, 1995, 1996 Xerox Corporation.  All rights reserved.
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

#include "code_gen.h"

#include <limits.h>

#include "ast.h"
#include "exceptions.h"
#include "opcode.h"
#include "program.h"
#include "storage.h"
#include "structures.h"
#include "str_intern.h"
#include "utils.h"
#include "version.h"
#include "my-stdlib.h"

/*** The reader will likely find it useful to consult the file
 *** `MOOCodeSequences.txt' in this directory while reading the code in this
 *** file.
 ***/

enum fixup_kind {
    FIXUP_LITERAL, FIXUP_FORK, FIXUP_LABEL, FIXUP_VAR_REF, FIXUP_STACK
};

struct fixup {
    enum fixup_kind kind;
    unsigned pc;
    unsigned value;
    unsigned prev_literals, prev_forks, prev_var_refs, prev_labels,
     prev_stacks;
    int next;			/* chain for compiling IF/ELSEIF arms */
};
typedef struct fixup Fixup;

struct gstate {
    unsigned total_var_refs;	/* For duplicating an old bug... */
    unsigned num_literals, max_literals;
    Var *literals;
    unsigned num_fork_vectors, max_fork_vectors;
    Bytecodes *fork_vectors;
    unsigned num_resume_points, max_resume_points;
    ResumePoint *resume_points;
    unsigned num_resume_loops, max_resume_loops;
    ResumeLoop *resume_loops;
};
typedef struct gstate GState;

struct loop {
    int id;
    unsigned target;
    Fixup top_label;
    unsigned top_stack;
    int bottom_label;
    unsigned bottom_stack;
};
typedef struct loop Loop;

struct state {
    unsigned max_literal, max_fork, max_var_ref;
    /* For telling how big the refs must be */
    unsigned num_literals, num_forks, num_var_refs, num_labels, num_stacks;
    /* For computing the final vector length */
    unsigned num_fixups, max_fixups;
    Fixup *fixups;
    unsigned num_bytes, max_bytes;
    Byte *bytes;
#ifdef BYTECODE_REDUCE_REF
    Byte *pushmap;
    Byte *trymap;
    unsigned try_depth;
#endif				/* BYTECODE_REDUCE_REF */
    unsigned cur_stack, max_stack;
    unsigned max_stack_slots;
    ResumeStackSlot *stack_slots;
    unsigned saved_stack;
    unsigned num_loops, max_loops;
    Loop *loops;
    unsigned code_unit;
    int vector;
    GState *gstate;
};
typedef struct state State;

#define UNBOUND_RESUME_VECTOR -2

#ifdef BYTECODE_REDUCE_REF
#define INCR_TRY_DEPTH(SSS)	(++(SSS)->try_depth)
#define DECR_TRY_DEPTH(SSS)	(--(SSS)->try_depth)
#define NON_VR_VAR_MASK	      ~((1 << SLOT_ARGSTR) | \
				(1 << SLOT_DOBJ) | \
				(1 << SLOT_DOBJSTR) | \
				(1 << SLOT_PREPSTR) | \
				(1 << SLOT_IOBJ) | \
				(1 << SLOT_IOBJSTR) | \
				(1 << SLOT_PLAYER))
#else				/* no BYTECODE_REDUCE_REF */
#define INCR_TRY_DEPTH(SSS)
#define DECR_TRY_DEPTH(SSS)
#endif				/* BYTECODE_REDUCE_REF */

static void
init_gstate(GState * gstate)
{
    gstate->total_var_refs = 0;
    gstate->num_literals = gstate->num_fork_vectors = 0;
    gstate->max_literals = gstate->max_fork_vectors = 0;
    gstate->fork_vectors = 0;
    gstate->literals = 0;
    gstate->num_resume_points = gstate->max_resume_points = 0;
    gstate->resume_points = 0;
    gstate->num_resume_loops = gstate->max_resume_loops = 0;
    gstate->resume_loops = 0;
}

static void
free_gstate(GState gstate)
{
    unsigned i;

    if (gstate.literals)
	myfree(gstate.literals, M_CODE_GEN);
    if (gstate.fork_vectors)
	myfree(gstate.fork_vectors, M_CODE_GEN);

    for (i = 0; i < gstate.num_resume_points; i++)
	if (gstate.resume_points[i].stack_slots)
	    myfree(gstate.resume_points[i].stack_slots, M_PROGRAM);
    if (gstate.resume_points)
	myfree(gstate.resume_points, M_CODE_GEN);
    if (gstate.resume_loops)
	myfree(gstate.resume_loops, M_CODE_GEN);
}

static void
init_state(State * state, GState * gstate, unsigned code_unit, int vector)
{
    state->num_literals = state->num_forks = state->num_labels = 0;
    state->num_var_refs = state->num_stacks = 0;

    state->max_literal = state->max_fork = state->max_var_ref = 0;

    state->num_fixups = 0;
    state->max_fixups = 10;
    state->fixups = mymalloc(sizeof(Fixup) * state->max_fixups, M_CODE_GEN);

    state->num_bytes = 0;
    state->max_bytes = 50;
    state->bytes = mymalloc(sizeof(Byte) * state->max_bytes, M_BYTECODES);
#ifdef BYTECODE_REDUCE_REF
    state->pushmap = mymalloc(sizeof(Byte) * state->max_bytes, M_BYTECODES);
    state->trymap = mymalloc(sizeof(Byte) * state->max_bytes, M_BYTECODES);
    state->try_depth = 0;
#endif				/* BYTECODE_REDUCE_REF */

    state->cur_stack = state->max_stack = 0;
    state->max_stack_slots = 16;
    state->stack_slots = mymalloc(sizeof(ResumeStackSlot)
				  * state->max_stack_slots, M_CODE_GEN);
    state->saved_stack = UINT_MAX;

    state->num_loops = 0;
    state->max_loops = 5;
    state->loops = mymalloc(sizeof(Loop) * state->max_loops, M_CODE_GEN);

    state->code_unit = code_unit;
    state->vector = vector;
    state->gstate = gstate;
}

static void
add_resume_point(State * state, unsigned pc, unsigned error_pc,
		 unsigned stack_depth, ResumePointKind kind, unsigned site)
{
    GState *gstate = state->gstate;
    ResumePoint *point;
    unsigned i;

    if (gstate->num_resume_points == gstate->max_resume_points) {
	unsigned new_max = gstate->max_resume_points == 0
	    ? 16 : 2 * gstate->max_resume_points;
	ResumePoint *new_points = mymalloc(sizeof(ResumePoint) * new_max,
					   M_CODE_GEN);
	for (i = 0; i < gstate->num_resume_points; i++)
	    new_points[i] = gstate->resume_points[i];
	if (gstate->resume_points)
	    myfree(gstate->resume_points, M_CODE_GEN);
	gstate->resume_points = new_points;
	gstate->max_resume_points = new_max;
    }

    /* This encounter order is the persistent RESUME_SCHEMA 1 contract. */
    point = &gstate->resume_points[gstate->num_resume_points++];
    point->key.code_unit = state->code_unit;
    point->key.site = site;
    point->vector = state->vector;
    point->pc = pc;
    point->error_pc = error_pc;
    point->stack_depth = stack_depth;
    point->flags = RESUME_PRESERVE_TEMP;
    point->kind = kind;
    point->frame_slots = 1;	/* The canonical assignment temporary. */
    point->stack_slots = stack_depth
	? mymalloc(sizeof(ResumeStackSlot) * stack_depth, M_PROGRAM) : 0;
    for (i = 0; i < stack_depth; i++) {
	point->stack_slots[i] = state->stack_slots[i];
	if (point->stack_slots[i].kind == RSS_VALUE
	    || point->stack_slots[i].kind == RSS_PENDING_REASON
	    || point->stack_slots[i].kind == RSS_PENDING_VALUE)
	    point->frame_slots++;
    }
}

static void
bind_resume_points(GState * gstate, unsigned code_unit, int vector)
{
    unsigned i;

    for (i = 0; i < gstate->num_resume_points; i++)
	if (gstate->resume_points[i].key.code_unit == code_unit) {
	    if (gstate->resume_points[i].vector != UNBOUND_RESUME_VECTOR)
		panic("ResumePoint code unit bound more than once");
	    gstate->resume_points[i].vector = vector;
	}
}

static void
free_state(State state)
{
    myfree(state.fixups, M_CODE_GEN);
    myfree(state.bytes, M_BYTECODES);
    myfree(state.stack_slots, M_CODE_GEN);
#ifdef BYTECODE_REDUCE_REF
    myfree(state.pushmap, M_BYTECODES);
    myfree(state.trymap, M_BYTECODES);
#endif				/* BYTECODE_REDUCE_REF */
    myfree(state.loops, M_CODE_GEN);
}

static void
emit_byte(Byte b, State * state)
{
    if (state->num_bytes == state->max_bytes) {
	unsigned new_max = 2 * state->max_bytes;
	state->bytes = myrealloc(state->bytes, sizeof(Byte) * new_max,
				 M_BYTECODES);
#ifdef BYTECODE_REDUCE_REF
	state->pushmap = myrealloc(state->pushmap, sizeof(Byte) * new_max,
				   M_BYTECODES);
	state->trymap = myrealloc(state->trymap, sizeof(Byte) * new_max,
				  M_BYTECODES);
#endif				/* BYTECODE_REDUCE_REF */
	state->max_bytes = new_max;
    }
#ifdef BYTECODE_REDUCE_REF
    state->pushmap[state->num_bytes] = 0;
    state->trymap[state->num_bytes] = state->try_depth;
#endif				/* BYTECODE_REDUCE_REF */
    state->bytes[state->num_bytes++] = b;
}

static void
emit_extended_byte(Byte b, State * state)
{
    emit_byte(OP_EXTENDED, state);
    emit_byte(b, state);
}

static int
add_known_fixup(Fixup f, State * state)
{
    unsigned int i;

    if (state->num_fixups == state->max_fixups) {
	unsigned new_max = 2 * state->max_fixups;
	Fixup *new_fixups = mymalloc(sizeof(Fixup) * new_max,
				     M_CODE_GEN);

	for (i = 0; i < state->num_fixups; i++)
	    new_fixups[i] = state->fixups[i];

	myfree(state->fixups, M_CODE_GEN);
	state->fixups = new_fixups;
	state->max_fixups = new_max;
    }
    f.pc = state->num_bytes;
    state->fixups[i = state->num_fixups++] = f;

    emit_byte(0, state);	/* a placeholder for the eventual value */

    return i;
}

static int
add_linked_fixup(enum fixup_kind kind, unsigned value, int next, State * state)
{
    Fixup f;

    f.kind = kind;
    f.value = value;
    f.prev_literals = state->num_literals;
    f.prev_forks = state->num_forks;
    f.prev_var_refs = state->num_var_refs;
    f.prev_labels = state->num_labels;
    f.prev_stacks = state->num_stacks;
    f.next = next;
    return add_known_fixup(f, state);
}

static int
add_fixup(enum fixup_kind kind, unsigned value, State * state)
{
    return add_linked_fixup(kind, value, -1, state);
}

static void
add_literal(Var v, State * state)
{
    GState *gstate = state->gstate;
    Var *literals = gstate->literals;
    unsigned i;

    for (i = 0; i < gstate->num_literals; i++)
	if (v.type == literals[i].type	/* no int/float coercion here */
	    && equality(v, literals[i], 1))
	    break;

    if (i == gstate->num_literals) {
	/* New literal to intern */
	if (gstate->num_literals == gstate->max_literals) {
	    unsigned new_max = gstate->max_literals == 0
	    ? 5 : 2 * gstate->max_literals;
	    Var *new_literals = mymalloc(sizeof(Var) * new_max,
					 M_CODE_GEN);

	    if (gstate->literals) {
		for (i = 0; i < gstate->num_literals; i++)
		    new_literals[i] = literals[i];

		myfree(literals, M_CODE_GEN);
	    }
	    gstate->literals = new_literals;
	    gstate->max_literals = new_max;
	}
	if (v.type == TYPE_STR) {
	    /* intern string if we can */
	    Var nv;

	    nv.type = TYPE_STR;
	    nv.v.str = str_intern(v.v.str);
	    gstate->literals[i = gstate->num_literals++] = nv;
	} else {
	    gstate->literals[i = gstate->num_literals++] = var_ref(v);
	}
    }
    add_fixup(FIXUP_LITERAL, i, state);
    state->num_literals++;
    if (i > state->max_literal)
	state->max_literal = i;
}

static unsigned
add_fork(Bytecodes b, State * state)
{
    unsigned i;
    GState *gstate = state->gstate;

    if (gstate->num_fork_vectors == gstate->max_fork_vectors) {
	unsigned new_max = gstate->max_fork_vectors == 0
	? 1 : 2 * gstate->max_fork_vectors;
	Bytecodes *new_fv = mymalloc(sizeof(Bytecodes) * new_max,
				     M_CODE_GEN);

	if (gstate->fork_vectors) {
	    for (i = 0; i < gstate->num_fork_vectors; i++)
		new_fv[i] = gstate->fork_vectors[i];

	    myfree(gstate->fork_vectors, M_CODE_GEN);
	}
	gstate->fork_vectors = new_fv;
	gstate->max_fork_vectors = new_max;
    }
    gstate->fork_vectors[i = gstate->num_fork_vectors++] = b;

    add_fixup(FIXUP_FORK, i, state);
    state->num_forks++;
    if (i > state->max_fork)
	state->max_fork = i;

    return i;
}

static void
add_var_ref(unsigned slot, State * state)
{
    add_fixup(FIXUP_VAR_REF, slot, state);
    state->num_var_refs++;
    if (slot > state->max_var_ref)
	state->max_var_ref = slot;
    state->gstate->total_var_refs++;
}

static int
add_linked_label(int next, State * state)
{
    int label = add_linked_fixup(FIXUP_LABEL, 0, next, state);

    state->num_labels++;
    return label;
}

static int
add_label(State * state)
{
    return add_linked_label(-1, state);
}

static void
add_pseudo_label(unsigned value, State * state)
{
    Fixup f;

    f.kind = FIXUP_LABEL;
    f.value = value;
    f.prev_literals = f.prev_forks = 0;
    f.prev_var_refs = f.prev_labels = 0;

    f.prev_stacks = 0;

    f.next = -1;

    add_known_fixup(f, state);
    state->num_labels++;
}

static int
add_known_label(Fixup f, State * state)
{
    int label = add_known_fixup(f, state);

    state->num_labels++;
    return label;
}

static Fixup
capture_label(State * state)
{
    Fixup f;

    f.kind = FIXUP_LABEL;
    f.value = state->num_bytes;
    f.prev_literals = state->num_literals;
    f.prev_forks = state->num_forks;
    f.prev_var_refs = state->num_var_refs;
    f.prev_labels = state->num_labels;
    f.prev_stacks = state->num_stacks;
    f.next = -1;

    /* silence compiler warning;
     * capture_label() is always followed by add_known_label()
     */
    f.pc = 0xdefeca7e;
    return f;
}

static void
define_label(int label, State * state)
{
    unsigned value = state->num_bytes;

    while (label != -1) {
	Fixup *fixup = &(state->fixups[label]);

	fixup->value = value;
	fixup->prev_literals = state->num_literals;
	fixup->prev_forks = state->num_forks;
	fixup->prev_var_refs = state->num_var_refs;
	fixup->prev_labels = state->num_labels;
	fixup->prev_stacks = state->num_stacks;
	label = fixup->next;
    }
}

static void
add_stack_ref(unsigned index, State * state)
{
    add_fixup(FIXUP_STACK, index, state);
}

static void
push_stack_slot(ResumeStackSlotKind kind, unsigned data, State * state)
{
    if (state->cur_stack == state->max_stack_slots) {
	state->max_stack_slots *= 2;
	state->stack_slots = myrealloc(state->stack_slots,
				       sizeof(ResumeStackSlot)
				       * state->max_stack_slots, M_CODE_GEN);
    }
    state->stack_slots[state->cur_stack].kind = kind;
    state->stack_slots[state->cur_stack].data = data;
    state->cur_stack++;
    if (state->cur_stack > state->max_stack)
	state->max_stack = state->cur_stack;
}

static void
push_stack(unsigned n, State * state)
{
    while (n--)
	push_stack_slot(RSS_VALUE, 0, state);
}

static void
pop_stack(unsigned n, State * state)
{
    state->cur_stack -= n;
}

static unsigned
save_stack_top(State * state)
{
    unsigned old = state->saved_stack;

    state->saved_stack = state->cur_stack - 1;

    return old;
}

static unsigned
saved_stack_top(State * state)
{
    return state->saved_stack;
}

static void
restore_stack_top(unsigned old, State * state)
{
    state->saved_stack = old;
}

static void
enter_loop(int id, unsigned ordinal, Fixup top_label, unsigned top_stack,
	   int bottom_label, unsigned bottom_stack, State * state)
{
    unsigned int i;
    Loop *loop;

    if (state->num_loops == state->max_loops) {
	unsigned new_max = 2 * state->max_loops;
	Loop *new_loops = mymalloc(sizeof(Loop) * new_max,
				   M_CODE_GEN);

	for (i = 0; i < state->num_loops; i++)
	    new_loops[i] = state->loops[i];

	myfree(state->loops, M_CODE_GEN);
	state->loops = new_loops;
	state->max_loops = new_max;
    }
    {
	GState *g = state->gstate;
	ResumeLoop *target;

	if (g->num_resume_loops == g->max_resume_loops) {
	    unsigned new_max = g->max_resume_loops ? 2 * g->max_resume_loops : 16;
	    ResumeLoop *targets = mymalloc(sizeof(ResumeLoop) * new_max,
					   M_CODE_GEN);

	    for (i = 0; i < g->num_resume_loops; i++)
		targets[i] = g->resume_loops[i];
	    if (g->resume_loops)
		myfree(g->resume_loops, M_CODE_GEN);
	    g->resume_loops = targets;
	    g->max_resume_loops = new_max;
	}
	target = &g->resume_loops[g->num_resume_loops];
	target->code_unit = state->code_unit;
	target->ordinal = ordinal;
	target->parent = state->num_loops
	    ? g->resume_loops[state->loops[state->num_loops - 1].target].ordinal : 0;
	target->continue_pc = top_label.value;
	target->break_pc = bottom_label; /* Relocated from the label fixup. */
	target->continue_stack = top_stack;
	target->break_stack = bottom_stack;
	loop = &(state->loops[state->num_loops++]);
	loop->target = g->num_resume_loops++;
    }
    loop->id = id;
    loop->top_label = top_label;
    loop->top_stack = top_stack;
    loop->bottom_label = bottom_label;
    loop->bottom_stack = bottom_stack;
}

static int
exit_loop(State * state)
{
    return state->loops[--state->num_loops].bottom_label;
}


static void
emit_call_verb_op(Opcode op, State * state)
{
    emit_byte(op, state);
#ifdef BYTECODE_REDUCE_REF
    state->pushmap[state->num_bytes - 1] = OP_CALL_VERB;
#endif				/* BYTECODE_REDUCE_REF */
}

static void
emit_ending_op(Opcode op, State * state)
{
    emit_byte(op, state);
#ifdef BYTECODE_REDUCE_REF
    state->pushmap[state->num_bytes - 1] = OP_DONE;
#endif				/* BYTECODE_REDUCE_REF */
}

static void
emit_var_op(Opcode op, unsigned slot, State * state)
{
    if (slot >= NUM_READY_VARS) {
	emit_byte(op + NUM_READY_VARS, state);
	add_var_ref(slot, state);
    } else {
	emit_byte(op + slot, state);
#ifdef BYTECODE_REDUCE_REF
	state->pushmap[state->num_bytes - 1] = op;
#endif				/* BYTECODE_REDUCE_REF */
    }
}

static void generate_expr(Expr *, State *);

static void
generate_arg_list(Arg_List * args, State * state)
{
    if (!args) {
	emit_byte(OP_MAKE_EMPTY_LIST, state);
	push_stack(1, state);
    } else {
	Opcode normal_op = OP_MAKE_SINGLETON_LIST, splice_op = OP_CHECK_LIST_FOR_SPLICE;
	unsigned pop = 0;

	for (; args; args = args->next) {
	    generate_expr(args->expr, state);
	    emit_byte(args->kind == ARG_NORMAL ? normal_op : splice_op, state);
	    pop_stack(pop, state);
	    normal_op = OP_LIST_ADD_TAIL;
	    splice_op = OP_LIST_APPEND;
	    pop = 1;
	}
    }
}

static void
push_lvalue(Expr * expr, int indexed_above, State * state)
{
    unsigned old;

    switch (expr->kind) {
    case EXPR_RANGE:
	push_lvalue(expr->e.range.base, 1, state);
	old = save_stack_top(state);
	generate_expr(expr->e.range.from, state);
	generate_expr(expr->e.range.to, state);
	restore_stack_top(old, state);
	break;
    case EXPR_INDEX:
	push_lvalue(expr->e.bin.lhs, 1, state);
	old = save_stack_top(state);
	generate_expr(expr->e.bin.rhs, state);
	restore_stack_top(old, state);
	if (indexed_above) {
	    emit_byte(OP_PUSH_REF, state);
	    push_stack(1, state);
	}
	break;
    case EXPR_ID:
	if (indexed_above) {
	    emit_var_op(OP_PUSH, expr->e.id, state);
	    push_stack(1, state);
	}
	break;
    case EXPR_PROP:
	generate_expr(expr->e.bin.lhs, state);
	generate_expr(expr->e.bin.rhs, state);
	if (indexed_above) {
	    emit_byte(OP_PUSH_GET_PROP, state);
	    push_stack(1, state);
	}
	break;
    default:
	panic("Bad lvalue in PUSH_LVALUE()");
    }
}

static void
generate_codes(Arg_List * codes, State * state)
{
    if (codes)
	generate_arg_list(codes, state);
    else {
	emit_byte(OPTIM_NUM_TO_OPCODE(0), state);
	push_stack(1, state);
    }
}

static void
generate_expr(Expr * expr, State * state)
{
    switch (expr->kind) {
    case EXPR_VAR:
	{
	    Var v;

	    v = expr->e.var;
	    if (v.type == TYPE_INT && IN_OPTIM_NUM_RANGE(v.v.num))
		emit_byte(OPTIM_NUM_TO_OPCODE(v.v.num), state);
	    else {
		emit_byte(OP_IMM, state);
		add_literal(v, state);
	    }
	    push_stack(1, state);
	}
	break;
    case EXPR_ID:
	emit_var_op(OP_PUSH, expr->e.id, state);
	push_stack(1, state);
	break;
    case EXPR_AND:
    case EXPR_OR:
	{
	    int end_label;

	    generate_expr(expr->e.bin.lhs, state);
	    emit_byte(expr->kind == EXPR_AND ? OP_AND : OP_OR, state);
	    end_label = add_label(state);
	    pop_stack(1, state);
	    generate_expr(expr->e.bin.rhs, state);
	    define_label(end_label, state);
	}
	break;
    case EXPR_NEGATE:
    case EXPR_NOT:
	generate_expr(expr->e.expr, state);
	emit_byte(expr->kind == EXPR_NOT ? OP_NOT : OP_UNARY_MINUS, state);
	break;
    case EXPR_COMPLEMENT:
	generate_expr(expr->e.expr, state);
	emit_extended_byte(EOP_COMPLEMENT, state);
	break;
    case EXPR_EQ:
    case EXPR_NE:
    case EXPR_GE:
    case EXPR_GT:
    case EXPR_LE:
    case EXPR_LT:
    case EXPR_IN:
    case EXPR_PLUS:
    case EXPR_MINUS:
    case EXPR_TIMES:
    case EXPR_DIVIDE:
    case EXPR_MOD:
    case EXPR_PROP:
	{
	    Opcode op = OP_ADD;	/* initialize to silence warning */

	    generate_expr(expr->e.bin.lhs, state);
	    generate_expr(expr->e.bin.rhs, state);
	    switch (expr->kind) {
	    case EXPR_EQ:
		op = OP_EQ;
		break;
	    case EXPR_NE:
		op = OP_NE;
		break;
	    case EXPR_GE:
		op = OP_GE;
		break;
	    case EXPR_GT:
		op = OP_GT;
		break;
	    case EXPR_LE:
		op = OP_LE;
		break;
	    case EXPR_LT:
		op = OP_LT;
		break;
	    case EXPR_IN:
		op = OP_IN;
		break;
	    case EXPR_PLUS:
		op = OP_ADD;
		break;
	    case EXPR_MINUS:
		op = OP_MINUS;
		break;
	    case EXPR_TIMES:
		op = OP_MULT;
		break;
	    case EXPR_DIVIDE:
		op = OP_DIV;
		break;
	    case EXPR_MOD:
		op = OP_MOD;
		break;
	    case EXPR_PROP:
		op = OP_GET_PROP;
		break;
	    default:
		panic("Not a binary operator in GENERATE_EXPR()");
	    }
	    emit_byte(op, state);
	    pop_stack(1, state);
	}
	break;
    case EXPR_BITAND:
    case EXPR_BITXOR:
    case EXPR_BITOR:
    case EXPR_SHL:
    case EXPR_SHR:
    case EXPR_LSHR:
	{
	    Extended_Opcode op = EOP_BITAND;	/* initialize to silence warning */

	    generate_expr(expr->e.bin.lhs, state);
	    generate_expr(expr->e.bin.rhs, state);
	    switch (expr->kind) {
	    case EXPR_BITAND:
		op = EOP_BITAND;
		break;
	    case EXPR_BITXOR:
		op = EOP_BITXOR;
		break;
	    case EXPR_BITOR:
		op = EOP_BITOR;
		break;
	    case EXPR_SHL:
		op = EOP_SHL;
		break;
	    case EXPR_SHR:
		op = EOP_SHR;
		break;
	    case EXPR_LSHR:
		op = EOP_LSHR;
		break;
	    default:
		panic("Not a binary operator in GENERATE_EXPR()");
	    }
	    emit_extended_byte(op, state);
	    pop_stack(1, state);
	}
	break;
    case EXPR_EXP:
	generate_expr(expr->e.bin.lhs, state);
	generate_expr(expr->e.bin.rhs, state);
	emit_extended_byte(EOP_EXP, state);
	pop_stack(1, state);
	break;
    case EXPR_INDEX:
	{
	    unsigned old;

	    generate_expr(expr->e.bin.lhs, state);
	    old = save_stack_top(state);
	    generate_expr(expr->e.bin.rhs, state);
	    restore_stack_top(old, state);
	    emit_byte(OP_REF, state);
	    pop_stack(1, state);
	}
	break;
    case EXPR_RANGE:
	{
	    unsigned old;

	    generate_expr(expr->e.range.base, state);
	    old = save_stack_top(state);
	    generate_expr(expr->e.range.from, state);
	    generate_expr(expr->e.range.to, state);
	    restore_stack_top(old, state);
	    emit_byte(OP_RANGE_REF, state);
	    pop_stack(2, state);
	}
	break;
    case EXPR_LENGTH:
	{
	    unsigned saved = saved_stack_top(state);

	    if (saved != UINT_MAX) {
		emit_extended_byte(EOP_LENGTH, state);
		add_stack_ref(saved, state);
		push_stack(1, state);
	    } else
		panic("Missing saved stack for `$' in GENERATE_EXPR()");
	}
	break;
    case EXPR_LIST:
	generate_arg_list(expr->e.list, state);
	break;
    case EXPR_CALL:
	generate_arg_list(expr->e.call.args, state);
	emit_byte(OP_BI_FUNC_CALL, state);
	emit_byte(expr->e.call.func, state);
	if (state->cur_stack == 0)
	    panic("Bad built-in stack depth in GENERATE_EXPR()");
	add_resume_point(state, state->num_bytes, state->num_bytes - 2,
			 state->cur_stack - 1, RP_BUILTIN, expr->e.call.resume_site);
	break;
    case EXPR_VERB:
	generate_expr(expr->e.verb.obj, state);
	generate_expr(expr->e.verb.verb, state);
	generate_arg_list(expr->e.verb.args, state);
	emit_call_verb_op(OP_CALL_VERB, state);
	if (state->cur_stack < 3)
	    panic("Bad verb-call stack depth in GENERATE_EXPR()");
	add_resume_point(state, state->num_bytes, state->num_bytes - 1,
			 state->cur_stack - 3, RP_CALL, expr->e.verb.resume_site);
	pop_stack(2, state);
	break;
    case EXPR_COND:
	{
	    int else_label, end_label;

	    generate_expr(expr->e.cond.condition, state);
	    emit_byte(OP_IF_QUES, state);
	    else_label = add_label(state);
	    pop_stack(1, state);
	    generate_expr(expr->e.cond.consequent, state);
	    emit_byte(OP_JUMP, state);
	    end_label = add_label(state);
	    pop_stack(1, state);
	    define_label(else_label, state);
	    generate_expr(expr->e.cond.alternate, state);
	    define_label(end_label, state);
	}
	break;
    case EXPR_ASGN:
	{
	    Expr *e = expr->e.bin.lhs;

	    if (e->kind == EXPR_SCATTER) {
		int nargs = 0, nreq = 0, rest = -1;
		unsigned done;
		Scatter *sc;

		generate_expr(expr->e.bin.rhs, state);
		for (sc = e->e.scatter; sc; sc = sc->next) {
		    nargs++;
		    if (sc->kind == SCAT_REQUIRED)
			nreq++;
		    else if (sc->kind == SCAT_REST)
			rest = nargs;
		}
		if (rest == -1)
		    rest = nargs + 1;
		emit_extended_byte(EOP_SCATTER, state);
		emit_byte(nargs, state);
		emit_byte(nreq, state);
		emit_byte(rest, state);
		for (sc = e->e.scatter; sc; sc = sc->next) {
		    add_var_ref(sc->id, state);
		    if (sc->kind != SCAT_OPTIONAL)
			add_pseudo_label(0, state);
		    else if (!sc->expr)
			add_pseudo_label(1, state);
		    else
			sc->label = add_label(state);
		}
		done = add_label(state);
		for (sc = e->e.scatter; sc; sc = sc->next)
		    if (sc->kind == SCAT_OPTIONAL && sc->expr) {
			define_label(sc->label, state);
			generate_expr(sc->expr, state);
			emit_var_op(OP_PUT, sc->id, state);
			emit_byte(OP_POP, state);
			pop_stack(1, state);
		    }
		define_label(done, state);
	    } else {
		int is_indexed = 0;

		push_lvalue(e, 0, state);
		generate_expr(expr->e.bin.rhs, state);
		if (e->kind == EXPR_RANGE || e->kind == EXPR_INDEX)
		    emit_byte(OP_PUT_TEMP, state);
		while (1) {
		    switch (e->kind) {
		    case EXPR_RANGE:
			emit_extended_byte(EOP_RANGESET, state);
			pop_stack(3, state);
			e = e->e.range.base;
			is_indexed = 1;
			continue;
		    case EXPR_INDEX:
			emit_byte(OP_INDEXSET, state);
			pop_stack(2, state);
			e = e->e.bin.lhs;
			is_indexed = 1;
			continue;
		    case EXPR_ID:
			emit_var_op(OP_PUT, e->e.id, state);
			break;
		    case EXPR_PROP:
			emit_byte(OP_PUT_PROP, state);
			pop_stack(2, state);
			break;
		    default:
			panic("Bad lvalue in GENERATE_EXPR()");
		    }
		    break;
		}
		if (is_indexed) {
		    emit_byte(OP_POP, state);
		    emit_byte(OP_PUSH_TEMP, state);
		}
	    }
	}
	break;
    case EXPR_CATCH:
	{
	    int handler_label, end_label;

	    generate_codes(expr->e.catch.codes, state);
	    emit_extended_byte(EOP_PUSH_LABEL, state);
	    handler_label = add_label(state);
	    push_stack_slot(RSS_HANDLER_PC, handler_label, state);
	    emit_extended_byte(EOP_CATCH, state);
	    push_stack_slot(RSS_CATCH, 1, state);
	    INCR_TRY_DEPTH(state);
	    generate_expr(expr->e.expr, state);
	    DECR_TRY_DEPTH(state);
	    emit_extended_byte(EOP_END_CATCH, state);
	    end_label = add_label(state);
	    pop_stack(3, state);	/* codes, label, catch */
	    define_label(handler_label, state);
	    /* After this label, we still have a value on the stack, but now,
	     * instead of it being the value of the main expression, we have
	     * the exception tuple pushed before entering the handler.
	     */
	    if (expr->e.catch.except) {
		emit_byte(OP_POP, state);
		pop_stack(1, state);
		generate_expr(expr->e.catch.except, state);
	    } else {
		/* Select code from tuple */
		emit_byte(OPTIM_NUM_TO_OPCODE(1), state);
		emit_byte(OP_REF, state);
	    }
	    define_label(end_label, state);
	}
	break;
    default:
	panic("Can't happen in GENERATE_EXPR()");
    }
}

static Bytecodes stmt_to_code(Stmt *, GState *, unsigned, int);

static void
generate_stmt(Stmt * stmt, State * state)
{
    for (; stmt; stmt = stmt->next) {
	switch (stmt->kind) {
	case STMT_COND:
	    {
		Opcode if_op = OP_IF;
		int end_label = -1;
		Cond_Arm *arms;

		for (arms = stmt->s.cond.arms; arms; arms = arms->next) {
		    int else_label;

		    generate_expr(arms->condition, state);
		    emit_byte(if_op, state);
		    else_label = add_label(state);
		    pop_stack(1, state);
		    generate_stmt(arms->stmt, state);
		    emit_byte(OP_JUMP, state);
		    end_label = add_linked_label(end_label, state);
		    define_label(else_label, state);
		    if_op = OP_EIF;
		}

		if (stmt->s.cond.otherwise)
		    generate_stmt(stmt->s.cond.otherwise, state);
		define_label(end_label, state);
	    }
	    break;
	case STMT_LIST:
	    {
		Fixup loop_top;
		int end_label;

		generate_expr(stmt->s.list.expr, state);
		emit_byte(OPTIM_NUM_TO_OPCODE(1), state);	/* loop list index */
		push_stack(1, state);
		loop_top = capture_label(state);
		emit_byte(OP_FOR_LIST, state);
		add_var_ref(stmt->s.list.id, state);
		end_label = add_label(state);
		enter_loop(stmt->s.list.id, stmt->loop_ordinal, loop_top,
			   state->cur_stack, end_label, state->cur_stack - 2, state);
		generate_stmt(stmt->s.list.body, state);
		end_label = exit_loop(state);
		emit_byte(OP_JUMP, state);
		add_known_label(loop_top, state);
		define_label(end_label, state);
		pop_stack(2, state);
	    }
	    break;
	case STMT_RANGE:
	    {
		Fixup loop_top;
		int end_label;

		generate_expr(stmt->s.range.from, state);
		generate_expr(stmt->s.range.to, state);
		loop_top = capture_label(state);
		emit_byte(OP_FOR_RANGE, state);
		add_var_ref(stmt->s.range.id, state);
		end_label = add_label(state);
		enter_loop(stmt->s.range.id, stmt->loop_ordinal, loop_top,
			   state->cur_stack, end_label, state->cur_stack - 2, state);
		generate_stmt(stmt->s.range.body, state);
		end_label = exit_loop(state);
		emit_byte(OP_JUMP, state);
		add_known_label(loop_top, state);
		define_label(end_label, state);
		pop_stack(2, state);
	    }
	    break;
	case STMT_WHILE:
	    {
		Fixup loop_top;
		int end_label;

		loop_top = capture_label(state);
		generate_expr(stmt->s.loop.condition, state);
		if (stmt->s.loop.id == -1)
		    emit_byte(OP_WHILE, state);
		else {
		    emit_extended_byte(EOP_WHILE_ID, state);
		    add_var_ref(stmt->s.loop.id, state);
		}
		end_label = add_label(state);
		pop_stack(1, state);
		enter_loop(stmt->s.loop.id, stmt->loop_ordinal, loop_top,
			   state->cur_stack, end_label, state->cur_stack, state);
		generate_stmt(stmt->s.loop.body, state);
		end_label = exit_loop(state);
		emit_byte(OP_JUMP, state);
		add_known_label(loop_top, state);
		define_label(end_label, state);
	    }
	    break;
	case STMT_FORK:
	    {
		Bytecodes fork_code;
		unsigned code_unit, fork_index;

		generate_expr(stmt->s.fork.time, state);
		if (stmt->s.fork.id >= 0)
		    emit_byte(OP_FORK_WITH_ID, state);
		else
		    emit_byte(OP_FORK, state);
		/* Fork code-unit preorder is part of RESUME_SCHEMA 1. */
		code_unit = stmt->s.fork.code_unit;
		fork_code = stmt_to_code(stmt->s.fork.body, state->gstate,
					 code_unit, UNBOUND_RESUME_VECTOR);
		fork_index = add_fork(fork_code, state);
		bind_resume_points(state->gstate, code_unit, fork_index);
		if (stmt->s.fork.id >= 0)
		    add_var_ref(stmt->s.fork.id, state);
		pop_stack(1, state);
	    }
	    break;
	case STMT_EXPR:
	    generate_expr(stmt->s.expr, state);
	    emit_byte(OP_POP, state);
	    pop_stack(1, state);
	    break;
	case STMT_RETURN:
	    if (stmt->s.expr) {
		generate_expr(stmt->s.expr, state);
		emit_ending_op(OP_RETURN, state);
		pop_stack(1, state);
	    } else
		emit_ending_op(OP_RETURN0, state);
	    break;
	case STMT_TRY_EXCEPT:
	    {
		int end_label, arm_count = 0;
		Except_Arm *ex;

		for (ex = stmt->s.catch.excepts; ex; ex = ex->next) {
		    generate_codes(ex->codes, state);
		    emit_extended_byte(EOP_PUSH_LABEL, state);
		    ex->label = add_label(state);
		    push_stack_slot(RSS_HANDLER_PC, ex->label, state);
		    arm_count++;
		}
		emit_extended_byte(EOP_TRY_EXCEPT, state);
		emit_byte(arm_count, state);
		push_stack_slot(RSS_CATCH, arm_count, state);
		INCR_TRY_DEPTH(state);
		generate_stmt(stmt->s.catch.body, state);
		DECR_TRY_DEPTH(state);
		emit_extended_byte(EOP_END_EXCEPT, state);
		end_label = add_label(state);
		pop_stack(2 * arm_count + 1, state);	/* 2(codes,pc) + catch */
		for (ex = stmt->s.catch.excepts; ex; ex = ex->next) {
		    define_label(ex->label, state);
		    push_stack(1, state);	/* exception tuple */
		    if (ex->id >= 0)
			emit_var_op(OP_PUT, ex->id, state);
		    emit_byte(OP_POP, state);
		    pop_stack(1, state);
		    generate_stmt(ex->stmt, state);
		    if (ex->next) {
			emit_byte(OP_JUMP, state);
			end_label = add_linked_label(end_label, state);
		    }
		}
		define_label(end_label, state);
	    }
	    break;
	case STMT_TRY_FINALLY:
	    {
		int handler_label;

		emit_extended_byte(EOP_TRY_FINALLY, state);
		handler_label = add_label(state);
		push_stack_slot(RSS_FINALLY, handler_label, state);
		INCR_TRY_DEPTH(state);
		generate_stmt(stmt->s.finally.body, state);
		DECR_TRY_DEPTH(state);
		emit_extended_byte(EOP_END_FINALLY, state);
		pop_stack(1, state);	/* FINALLY marker */
		define_label(handler_label, state);
		push_stack_slot(RSS_PENDING_REASON, 0, state);
		push_stack_slot(RSS_PENDING_VALUE, state->num_loops
		    ? state->gstate->resume_loops[
			state->loops[state->num_loops - 1].target].ordinal : 0,
		    state);
		generate_stmt(stmt->s.finally.handler, state);
		emit_extended_byte(EOP_CONTINUE, state);
		pop_stack(2, state);
	    }
	    break;
	case STMT_BREAK:
	case STMT_CONTINUE:
	    {
		int i;
		Loop *loop = 0;	/* silence warnings */

		if (stmt->s.exit == -1) {
		    emit_extended_byte(EOP_EXIT, state);
		    if (state->num_loops == 0)
			panic("No loop to exit, in CODE_GEN!");
		    loop = &(state->loops[state->num_loops - 1]);
		} else {
		    emit_extended_byte(EOP_EXIT_ID, state);
		    add_var_ref(stmt->s.exit, state);
		    for (i = state->num_loops - 1; i >= 0; i--)
			if (state->loops[i].id == stmt->s.exit) {
			    loop = &(state->loops[i]);
			    break;
			}
		    if (i < 0)
			panic("Can't find loop in CONTINUE_LOOP!");
		}

		if (stmt->kind == STMT_CONTINUE) {
		    add_stack_ref(loop->top_stack, state);
		    add_known_label(loop->top_label, state);
		} else {
		    add_stack_ref(loop->bottom_stack, state);
		    loop->bottom_label = add_linked_label(loop->bottom_label,
							  state);
		}
	    }
	    break;
	default:
	    panic("Can't happen in GENERATE_STMT()");
	}
    }
}

static unsigned
max(unsigned a, unsigned b)
{
    return a > b ? a : b;
}

static unsigned
ref_size(unsigned rmax)
{
    if (rmax <= 256)
	return 1;
    else if (rmax <= 256 * 256)
	return 2;
    else
	return 4;
}

static unsigned
fixup_width(enum fixup_kind kind, Bytecodes bc)
{
    switch (kind) {
    case FIXUP_LITERAL:
	return bc.numbytes_literal;
    case FIXUP_FORK:
	return bc.numbytes_fork;
    case FIXUP_VAR_REF:
	return bc.numbytes_var_name;
    case FIXUP_STACK:
	return bc.numbytes_stack;
    case FIXUP_LABEL:
	return bc.numbytes_label;
    default:
	panic("Can't happen in FIXUP_WIDTH()");
    }
}

static unsigned
expanded_pc(State * state, Bytecodes bc, unsigned pc)
{
    Fixup *fixup;
    unsigned i, result = pc;

    for (fixup = state->fixups, i = 0; i < state->num_fixups; i++, fixup++) {
	if (fixup->pc >= pc)
	    continue;

	result += fixup_width(fixup->kind, bc) - 1;
    }

    return result;
}

static void
relocate_resume_points(State * state, Bytecodes bc)
{
    GState *gstate = state->gstate;
    unsigned i, j;

    for (i = 0; i < gstate->num_resume_loops; i++) {
	ResumeLoop *loop = &gstate->resume_loops[i];

	if (loop->code_unit == state->code_unit) {
	    loop->break_pc = expanded_pc(state, bc,
		state->fixups[loop->break_pc].value);
	    loop->continue_pc = expanded_pc(state, bc, loop->continue_pc);
	}
    }
    for (i = 0; i < gstate->num_resume_points; i++)
	if (gstate->resume_points[i].key.code_unit == state->code_unit) {
	    ResumePoint *point = &gstate->resume_points[i];

	    point->pc = expanded_pc(state, bc, point->pc);
	    point->error_pc = expanded_pc(state, bc, point->error_pc);
	    for (j = 0; j < point->stack_depth; j++)
		if (point->stack_slots[j].kind == RSS_HANDLER_PC
		    || point->stack_slots[j].kind == RSS_FINALLY) {
		    unsigned label = point->stack_slots[j].data;

		    if (label >= state->num_fixups
			|| state->fixups[label].kind != FIXUP_LABEL)
			panic("Bad handler label in RELOCATE_RESUME_POINTS()");
		    point->stack_slots[j].data =
			expanded_pc(state, bc, state->fixups[label].value);
		}
	}
}

#ifdef BYTECODE_REDUCE_REF
static int
bbd_cmp(int *a, int *b)
{
    return *a - *b;
}
#endif				/* BYTECODE_REDUCE_REF */

static Bytecodes
stmt_to_code(Stmt * stmt, GState * gstate, unsigned code_unit, int vector)
{
    State state;
    Bytecodes bc;
    int old_i, new_i, fix_i;
#ifdef BYTECODE_REDUCE_REF
    int *bbd, n_bbd;		/* basic block delimiters */
    unsigned varbits;		/* variables we've seen */
#if NUM_READY_VARS > 32
#error assumed NUM_READY_VARS was 32
#endif
#endif				/* BYTECODE_REDUCE_REF */
    Fixup *fixup;

    init_state(&state, gstate, code_unit, vector);

    generate_stmt(stmt, &state);
    emit_ending_op(OP_DONE, &state);

    if (state.cur_stack != 0)
	panic("Stack not entirely popped in STMT_TO_CODE()");
    if (state.saved_stack != UINT_MAX)
	panic("Still a saved stack index in STMT_TO_CODE()");

    /* The max()ing here with gstate->* is wrong (since that's a global
     * cumulative count, and thus unrelated to the local maximum), but required
     * in order to maintain the validity of old program counters stored for
     * suspended tasks... */
    bc.numbytes_literal = ref_size(max(state.max_literal,
				       gstate->num_literals));
    bc.numbytes_fork = ref_size(max(state.max_fork,
				    gstate->num_fork_vectors));
    bc.numbytes_var_name = ref_size(max(state.max_var_ref,
					gstate->total_var_refs));

    bc.size = state.num_bytes
	+ (bc.numbytes_literal - 1) * state.num_literals
	+ (bc.numbytes_fork - 1) * state.num_forks
	+ (bc.numbytes_var_name - 1) * state.num_var_refs;

    if (bc.size <= 256)
	bc.numbytes_label = 1;
    else if (bc.size + state.num_labels <= 256 * 256)
	bc.numbytes_label = 2;
    else
	bc.numbytes_label = 4;
    bc.size += (bc.numbytes_label - 1) * state.num_labels;

    bc.max_stack = state.max_stack;
    bc.numbytes_stack = ref_size(state.max_stack);

    bc.vector = mymalloc(sizeof(Byte) * bc.size, M_BYTECODES);

#ifdef BYTECODE_REDUCE_REF
    /*
     * Create a sorted array filled with the bytecode offsets of
     * beginnings of each basic block of code.  These are sequences
     * of bytecodes which are guaranteed to execute in order (so if
     * you start at the top, you will reach the bottom).  As such they
     * are delimited by conditional and unconditional jump operations,
     * each of which has an associated fixup.  If you also want to
     * limit the blocks to those which have the property "if you get to
     * the bottom you had to have started at the top", include the
     * *destinations* of the jumps (hence the qsort).
     */
    bbd = mymalloc(sizeof(*bbd) * (state.num_fixups + 2), M_CODE_GEN);
    n_bbd = 0;
    bbd[n_bbd++] = 0;
    bbd[n_bbd++] = state.num_bytes;
    for (fixup = state.fixups, fix_i = 0; fix_i < state.num_fixups; ++fix_i, ++fixup)
	if (fixup->kind == FIXUP_LABEL || fixup->kind == FIXUP_FORK)
	    bbd[n_bbd++] = fixup->pc;
    qsort(bbd, n_bbd, sizeof(*bbd), bbd_cmp);

    /*
     * For every basic block, search backwards for PUT ops.  The first
     * PUSH we find for each variable slot (looking backwards, remember)
     * after each PUT becomes a PUSH_CLEAR, while the rest remain PUSHs.
     * In other words, the last use of a variable before it is replaced
     * is identified, so that during interpretation the code can avoid
     * holding spurious references to it.
     */
    while (n_bbd-- > 1) {
	varbits = 0;

	for (old_i = bbd[n_bbd] - 1; old_i >= bbd[n_bbd - 1]; --old_i) {
	    if (state.pushmap[old_i] == OP_PUSH) {
		int id = PUSH_n_INDEX(state.bytes[old_i]);

		if (varbits & (1 << id)) {
		    varbits &= ~(1 << id);
		    state.bytes[old_i] += OP_PUSH_CLEAR - OP_PUSH;
		}
	    } else if (state.trymap[old_i] > 0) {
		/*
		 * Operations inside of exception handling blocks might not
		 * execute, so they can't set any bits.
		 */ ;
	    } else if (state.pushmap[old_i] == OP_PUT) {
		int id = PUT_n_INDEX(state.bytes[old_i]);
		varbits |= 1 << id;
	    } else if (state.pushmap[old_i] == OP_DONE) {
		/*
		 * If the verb ends, all variables are unneeded.  This
		 * means things like `return pass(@args)' will not hold
		 * a ref to `args' during the called verb.
		 */
		varbits = ~0U;
	    } else if (state.pushmap[old_i] == OP_CALL_VERB) {
		/*
		 * Verb calls implicitly pass the VR variables (dobj,
		 * dobjstr, player, etc).  They can't be clear at the
		 * time of a verbcall.
		 */
		varbits &= NON_VR_VAR_MASK;
	    }
	}
    }
    myfree(bbd, M_CODE_GEN);
#endif				/* BYTECODE_REDUCE_REF */

    fixup = state.fixups;
    fix_i = 0;
    /* For this loop, old_i and fix_i start at 0
     * and are always incremented, so casting to unsigned
     * (to silence vs-signed warnings) will be safe.
     * Not so in the previous loop. */
    for (old_i = new_i = 0; (unsigned)old_i < state.num_bytes; old_i++) {
	if ((unsigned)fix_i < state.num_fixups && fixup->pc == (unsigned)old_i) {
	    unsigned value, size;

	    value = fixup->value;
	    if (fixup->kind == FIXUP_LABEL)
		value += fixup->prev_literals * (bc.numbytes_literal - 1)
		    + fixup->prev_forks * (bc.numbytes_fork - 1)
		    + fixup->prev_var_refs * (bc.numbytes_var_name - 1)
		    + fixup->prev_labels * (bc.numbytes_label - 1)
		    + fixup->prev_stacks * (bc.numbytes_stack - 1);
	    size = fixup_width(fixup->kind, bc);

	    switch (size) {
	    case 4:
		bc.vector[new_i++] = value >> 24;
		bc.vector[new_i++] = value >> 16;
		/* FALLS THROUGH */
	    case 2:
		bc.vector[new_i++] = value >> 8;
		/* FALLS THROUGH */
	    case 1:
		bc.vector[new_i++] = value;
		break;
	    default:
		panic("Can't happen #2 in STMT_TO_CODE()");
	    }

	    fixup++;
	    fix_i++;
	} else
	    bc.vector[new_i++] = state.bytes[old_i];
    }

    relocate_resume_points(&state, bc);

    free_state(state);

    return bc;
}

Program *
generate_code(Stmt * stmt, DB_Version version)
{
    Program *prog = new_program();
    GState gstate;

    assign_resume_ids(stmt);
    init_gstate(&gstate);

    prog->main_vector = stmt_to_code(stmt, &gstate, 0, MAIN_VECTOR);
    prog->version = version;

    if (gstate.literals) {
	unsigned i;

	prog->literals = mymalloc(sizeof(Var) * gstate.num_literals,
				  M_LIT_LIST);
	prog->num_literals = gstate.num_literals;
	for (i = 0; i < gstate.num_literals; i++)
	    prog->literals[i] = gstate.literals[i];
    } else {
	prog->literals = 0;
	prog->num_literals = 0;
    }

    if (gstate.fork_vectors) {
	unsigned i;

	prog->fork_vectors =
	    mymalloc(sizeof(Bytecodes) * gstate.num_fork_vectors,
		     M_FORK_VECTORS);
	prog->fork_vectors_size = gstate.num_fork_vectors;
	for (i = 0; i < gstate.num_fork_vectors; i++)
	    prog->fork_vectors[i] = gstate.fork_vectors[i];
    } else {
	prog->fork_vectors = 0;
	prog->fork_vectors_size = 0;
    }

    if (gstate.resume_points) {
	unsigned i;

	prog->resume_points =
	    mymalloc(sizeof(ResumePoint) * gstate.num_resume_points,
		     M_PROGRAM);
	prog->num_resume_points = gstate.num_resume_points;
	for (i = 0; i < gstate.num_resume_points; i++) {
	    prog->resume_points[i] = gstate.resume_points[i];
	    gstate.resume_points[i].stack_slots = 0;
	}
    } else {
	prog->resume_points = 0;
	prog->num_resume_points = 0;
    }

    prog->num_resume_loops = gstate.num_resume_loops;
    if (gstate.num_resume_loops) {
	unsigned i;

	prog->resume_loops = mymalloc(sizeof(ResumeLoop) * gstate.num_resume_loops,
				    M_PROGRAM);
	for (i = 0; i < gstate.num_resume_loops; i++)
	    prog->resume_loops[i] = gstate.resume_loops[i];
    }
    free_gstate(gstate);

    if (!validate_program_resume_points(prog))
	panic("Invalid ResumePoint table in GENERATE_CODE()");

    return prog;
}


/*
 * $Log$
 * Revision 2.4  1996/02/08  07:21:08  pavel
 * Renamed TYPE_NUM to TYPE_INT.  Added support for exponentiation expression,
 * named WHILE loop and BREAK and CONTINUE statement.  Updated copyright
 * notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.3  1996/01/16  07:17:36  pavel
 * Add support for scattering assignment.  Release 1.8.0alpha6.
 *
 * Revision 2.2  1995/12/31  03:10:47  pavel
 * Added general support for managing stack references as another kind of
 * fixup and for a single stack of remembered stack positions.  Used that
 * stack for remembering the positions of indexed/subranged values and for
 * implementing the `$' expression.  Release 1.8.0alpha4.
 *
 * Revision 2.1  1995/11/30  04:18:56  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 2.0  1995/11/30  04:16:54  pavel
 * Initial RCS-controlled version.
 */
