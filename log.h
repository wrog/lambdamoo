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

#include "my-stdio.h"

#include "config.h"
#include "structures.h"

extern void set_log_file(FILE *);

extern void oklog(const char *,...) FORMAT(printf,1,2);
extern void errlog(const char *,...) FORMAT(printf,1,2);
extern void log_perror(const char *);

extern void reset_command_history(void);
extern void log_command_history(void);
extern void add_command_to_history(Objid player, const char *command);


#define log_report_progress()  ((--log_pcount <= 0) && log_report_progress_cktime())

extern int log_pcount;
extern int log_report_progress_cktime(void);

/*
 * $Log$
 * Revision 1.4  2004/05/22 01:25:43  wrog
 * merging in WROGUE changes (W_SRCIP, W_STARTUP, W_OOB)
 *
 * Revision 1.3.10.1  2003/06/03 12:19:27  wrog
 * added log_report_progress()
 *
 * Revision 1.3  1998/12/14 13:18:00  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:18:48  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:03  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.2  1996/04/08  01:05:28  pavel
 * Added `set_log_file()' entry point.  Release 1.8.0p3.
 *
 * Revision 2.1  1996/02/08  06:23:32  pavel
 * Renamed err/logf() to errlog/oklog().  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:52:07  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.3  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.2  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
