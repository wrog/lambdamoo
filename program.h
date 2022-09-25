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
#define Program_H

#include "structures.h"
#include "version.h"

typedef unsigned char Byte;

typedef struct {
    Byte numbytes_label, numbytes_literal, numbytes_fork, numbytes_var_name,
     numbytes_stack;
    Byte *vector;
    unsigned size;
    unsigned max_stack;
} Bytecodes;

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
    unsigned cached_lineno_vec;
} Program;

#define MAIN_VECTOR 	-1	/* As opposed to an index into fork_vectors */

extern Program *new_program(void);
extern Program *null_program(void);
extern Program *program_ref(Program *);
extern int program_bytes(Program *);
extern void free_program(Program *);

#endif				/* !Program_H */

/*
 * $Log$
 * Revision 1.5  1999/08/16 04:15:13  bjj
 * Lineno computation could be wrong for forked vectors.
 *
 * Revision 1.4  1998/12/14 13:18:49  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.3  1997/07/07 03:24:55  nop
 * Merge UNSAFE_OPTS (r5) after extensive testing.
 *
 * Revision 1.2.2.1  1997/06/05 09:00:01  bjj
 * Cache one pc/lineno pair with each Program.  Hopefully most programs that
 * fail multiple times usually do it on the same line!
 *
 * Revision 1.2  1997/03/03 04:19:18  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
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
