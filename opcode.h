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

#ifndef Opcode_h
#define Opcode_h 1

#include "options.h"

#define NUM_READY_VARS 32

enum Extended_Opcode {
    EOP_RANGESET, EOP_LENGTH,
    EOP_PUSH_LABEL, EOP_END_CATCH, EOP_END_EXCEPT, EOP_END_FINALLY,
    EOP_CONTINUE,

    /* ops after this point cost one tick */
    EOP_CATCH, EOP_TRY_EXCEPT, EOP_TRY_FINALLY,
    EOP_WHILE_ID, EOP_EXIT, EOP_EXIT_ID,
    EOP_SCATTER, EOP_EXP,

    Last_Extended_Opcode = 255
};

enum Opcode {

    /* control/statement constructs with 1 tick: */
    OP_IF, OP_WHILE, OP_EIF, OP_FORK, OP_FORK_WITH_ID, OP_FOR_LIST,
    OP_FOR_RANGE,

    /* expr-related opcodes with 1 tick: */
    OP_INDEXSET, OP_PUSH_GET_PROP, OP_GET_PROP, OP_CALL_VERB, OP_PUT_PROP,
    OP_BI_FUNC_CALL, OP_IF_QUES, OP_REF, OP_RANGE_REF,

    /* arglist-related opcodes with 1 tick: */
    OP_MAKE_SINGLETON_LIST, OP_CHECK_LIST_FOR_SPLICE,

    /* arith binary ops -- 1 tick: */
    OP_MULT, OP_DIV, OP_MOD, OP_ADD, OP_MINUS,

    /* comparison binary ops -- 1 tick: */
    OP_EQ, OP_NE, OP_LT, OP_LE, OP_GT, OP_GE, OP_IN,

    /* logic binary ops -- 1 tick: */
    OP_AND, OP_OR,

    /* unary ops -- 1 tick: */
    OP_UNARY_MINUS, OP_NOT,

    /* assignments, 1 tick: */
    OP_PUT,
    OP_G_PUT = OP_PUT + NUM_READY_VARS,

    /* variable references, no tick: */
    OP_PUSH,
    OP_G_PUSH = OP_PUSH + NUM_READY_VARS,

#ifdef BYTECODE_REDUCE_REF
    /* final variable references, no tick: */
    OP_PUSH_CLEAR,
    OP_G_PUSH_CLEAR = OP_PUSH_CLEAR + NUM_READY_VARS,
#endif				/* BYTECODE_REDUCE_REF */

    /* expr-related opcodes with no tick: */
    OP_IMM, OP_MAKE_EMPTY_LIST, OP_LIST_ADD_TAIL, OP_LIST_APPEND,
    OP_PUSH_REF, OP_PUT_TEMP, OP_PUSH_TEMP,

    /* control/statement constructs with no ticks: */
    OP_JUMP, OP_RETURN, OP_RETURN0, OP_DONE, OP_POP,

    OP_EXTENDED,		/* Used to add more opcodes */

    OPTIM_NUM_START,
    /* storage optimized imm-numbers can occupy 113-255, for 143 of them */
    Last_Opcode = 255
};

#define OPTIM_NUM_LOW -10
#define OPTIM_NUM_HI  (Last_Opcode - OPTIM_NUM_START + OPTIM_NUM_LOW)

#define IS_PUSH_n(o)             ((o) >= (unsigned) OP_PUSH \
				  && (o) < (unsigned) OP_G_PUSH)
#ifdef BYTECODE_REDUCE_REF
#define IS_PUSH_CLEAR_n(o)             ((o) >= (unsigned) OP_PUSH_CLEAR \
				  && (o) < (unsigned) OP_G_PUSH_CLEAR)
#define PUSH_CLEAR_n_INDEX(o)          ((o) - OP_PUSH_CLEAR)
#endif				/* BYTECODE_REDUCE_REF */
#define IS_PUT_n(o)              ((o) >= (unsigned) OP_PUT \
				  && (o) < (unsigned) OP_G_PUT)
#define PUSH_n_INDEX(o)          ((o) - OP_PUSH)
#define PUT_n_INDEX(o)           ((o) - OP_PUT)

#define IS_OPTIM_NUM_OPCODE(o)   ((o) >= (unsigned) OPTIM_NUM_START)
#define OPCODE_TO_OPTIM_NUM(o)   ((Num)(unsigned)(o) - OPTIM_NUM_START + OPTIM_NUM_LOW)

#define OPTIM_NUM_TO_OPCODE(i)   (OPTIM_NUM_START + (i) - OPTIM_NUM_LOW)
#define IN_OPTIM_NUM_RANGE(i)    ((i) >= OPTIM_NUM_LOW && (i) <= OPTIM_NUM_HI)

/* ARITH_COMP_BIN_OP does not include AND, OR */
#define IS_ARITH_COMP_BIN_OP(o)  ((o) >= (unsigned) OP_MULT \
				  && (o) <= (unsigned) OP_IN)

/* whether the opcode needs one tick */
#define COUNT_TICK(o)      	 ((o) <= OP_G_PUT)
#define COUNT_EOP_TICK(eo)	 ((eo) >= EOP_CATCH)

typedef enum Opcode Opcode;
typedef enum Extended_Opcode Extended_Opcode;

#endif

/*
 * $Log$
 * Revision 1.4  2002/09/15 23:21:01  xplat
 * GNU indent normalization.
 *
 * Revision 1.3  1998/12/14 13:18:40  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2.2.1  1997/09/09 07:01:17  bjj
 * Change bytecode generation so that x=f(x) calls f() without holding a ref
 * to the value of x in the variable slot.  See the options.h comment for
 * BYTECODE_REDUCE_REF for more details.
 *
 * This checkin also makes x[y]=z (OP_INDEXSET) take advantage of that (that
 * new code is not conditional and still works either way).
 *
 * Revision 1.2  1997/03/03 04:19:13  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.3  1996/02/08  06:18:03  pavel
 * Removed unused NUM_BUILTIN_NAMES constant.  Rearranged EOPs to support tick
 * counting and added COUNT_EOP_TICK().  Added EOP_EXP, EOP_WHILE_ID,
 * EOP_EXIT, and EOP_EXIT_ID.  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.2  1996/01/16  07:22:07  pavel
 * Added EOP_SCATTER.  Release 1.8.0alpha6.
 *
 * Revision 2.1  1995/12/31  03:13:27  pavel
 * Added EOP_LENGTH.  Release 1.8.0alpha4.
 *
 * Revision 2.0  1995/11/30  04:53:27  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.4  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.3  1992/10/17  20:48:34  pavel
 * Added some (unsigned) casts to placate over-protective compilers.
 * Removed unused IS_EXPR_OP macro.
 *
 * Revision 1.2  1992/08/28  23:19:33  pjames
 * Added Extended_Opcode enumeration, and OP_RANGESET.
 * Replaced LABEL with OP_EXTENDED.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
