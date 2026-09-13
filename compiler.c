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

#include "compiler.h"

#include "code_gen.h"
#include "storage.h"

Program *
compile_ast_to_program(Stmt * ast, Names * var_names, DB_Version version)
{
    Program *program;

    program = generate_code(ast, version);

    program->num_var_names = var_names->size;
    program->var_names = var_names->names;

    myfree(var_names, M_NAMES);
    free_stmt(ast);

    return program;
}
