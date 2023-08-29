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

extern void register_db_tune(void);
extern void register_disassemble(void);
extern void register_extensions(void);
extern void register_execute(void);
extern void register_functions(void);
extern void register_list(void);
extern void register_log(void);
extern void register_numbers(void);
extern void register_objects(void);
extern void register_property(void);
extern void register_server(void);
extern void register_tasks(void);
extern void register_verbs(void);

/*
 * $Log$
 * Revision 1.2  1998/12/14 13:17:29  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.1.1.1  1997/03/03 03:45:02  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.2  1996/02/08  06:29:12  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.1  1996/01/16  07:28:09  pavel
 * Added `register_functions()'.  Release 1.8.0alpha6.
 *
 * Revision 2.0  1995/11/30  04:50:30  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.4  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.3  1992/09/08  22:04:15  pjames
 * Updated to have new procedure names.
 *
 * Revision 1.2  1992/08/10  17:22:22  pjames
 * Added register_bf_log(void);
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
