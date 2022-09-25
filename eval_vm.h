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
#include "execute.h"
#include "structures.h"

extern vm read_vm(int task_id);
extern void write_vm(vm);

extern vm new_vm(int task_id, int stack_size);
extern void free_vm(vm the_vm, int stack_too);

extern activation top_activ(vm);
extern Objid progr_of_cur_verb(vm);
extern unsigned suspended_lineno_of_vm(vm);

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:17:47  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:36  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:02  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:26:06  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:50:51  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.6  1993/12/21  23:41:30  pavel
 * Add new `outcome' result from the interpreter.
 *
 * Revision 1.5  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.4  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.3  1992/10/17  20:26:16  pavel
 * Changed return-type of read_vm() from `char' to `int', for systems that use
 * unsigned chars.
 *
 * Revision 1.2  1992/08/31  22:22:56  pjames
 * Changed some `char *'s to `const char *'
 *
 * Revision 1.1  1992/08/10  16:20:00  pjames
 * Initial RCS-controlled version
 */
