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

#ifndef Program_H
#define Program_H 1

#include "structures.h"
#include "version.h"

typedef uint8_t Byte;

#define RESUME_SCHEMA 1

/*
 * ResumeKey is a serialized language-level continuation identifier.  The
 * schema-1 assignment order is a database compatibility contract: code unit
 * zero is the main body, fork bodies are numbered in fork-statement encounter
 * preorder, and sites are numbered by the canonical AST traversal within each
 * code unit.  Code-unit numbering is independent of fork-vector allocation
 * order.
 */
typedef struct {
    unsigned code_unit;
    unsigned site;
} ResumeKey;

typedef struct {
    Byte numbytes_label, numbytes_literal, numbytes_fork, numbytes_var_name,
     numbytes_stack;
    Byte *vector;
    unsigned size;
    unsigned max_stack;
} Bytecodes;
#define BQM_DESCRIBE_Bytecodes(B,F,V,X)   ((4 * F) + V)

typedef enum {
    RP_CALL,
    RP_BUILTIN
} ResumePointKind;

#define RESUME_PRESERVE_TEMP 1

typedef enum {
    RSS_VALUE,
    RSS_PENDING_REASON,
    RSS_PENDING_VALUE,	/* data: innermost enclosing loop ordinal, or zero */
    RSS_HANDLER_PC,
    RSS_CATCH,
    RSS_FINALLY
} ResumeStackSlotKind;

typedef struct {
    ResumeStackSlotKind kind;
    unsigned data;
} ResumeStackSlot;
#define BQM_DESCRIBE_ResumeStackSlot(B,F,V,X)   (2 * V)

typedef struct {
    ResumeKey key;
    int vector;
    unsigned pc;
    unsigned error_pc;
    unsigned stack_depth;
    unsigned flags;
    ResumePointKind kind;
    unsigned frame_slots;
    ResumeStackSlot *stack_slots;
} ResumePoint;
#define BQM_DESCRIBE_ResumePoint(B,F,V,X)   ((9 * V) + F)

/* Engine-private destinations, indexed by semantic code unit and ordinal. */
typedef struct {
    unsigned code_unit, ordinal, parent; /* parent is an ordinal, or zero */
    unsigned break_pc, continue_pc, break_stack, continue_stack;
} ResumeLoop;
#define BQM_DESCRIBE_ResumeLoop(B,F,V,X)   (7 * V)

typedef struct {
    DB_Version version;
    unsigned first_lineno;
    unsigned ref_count;

    Bytecodes main_vector;

    unsigned num_literals;
    Var *literals;

    unsigned fork_vectors_size;
    Bytecodes *fork_vectors;

    unsigned num_var_names;
    const char **var_names;

    unsigned cached_lineno;
    unsigned cached_lineno_pc;
    int cached_lineno_vec;

    unsigned num_resume_points;
    ResumePoint *resume_points;
    unsigned num_resume_loops;
    ResumeLoop *resume_loops;
} Program;
#define BQM_DESCRIBE_Program(B,F,V,X)   ((9 * F) + (12 * V))

#define MAIN_VECTOR 	-1	/* As opposed to an index into fork_vectors */

extern Program *new_program(void);
extern Program *null_program(void);
extern Program *program_ref(Program *);
extern ResumeKey invalid_resume_key(void);
extern int resume_key_is_valid(ResumeKey);
extern const ResumePoint *resume_point_for_key(Program *, ResumeKey);
extern const ResumePoint *resume_point_for_program_pc(Program *, int,
						       unsigned);
extern const ResumePoint *resume_point_for_program_location(Program *, int,
						     unsigned, unsigned);
extern int validate_program_resume_points(Program *);
extern int program_bytes(Program *);
extern void free_program(Program *);

#endif		/* !Program_H */

/*
 * $Log$
 * Revision 2.3  1996/02/08  06:14:19  pavel
 * Added version number on programs.  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.2  1995/12/31  03:13:02  pavel
 * Added numbytes_stack field to Bytecodes values.  Release 1.8.0alpha4.
 *
 * Revision 2.1  1995/12/11  08:03:22  pavel
 * Removed a useless macro definition.  Added `null_program()' and
 * `program_bytes()'.  Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  04:54:29  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.2  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
