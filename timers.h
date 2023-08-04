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

#ifndef Timers_H
#define Timers_H 1

#include "my-time.h"

typedef int Timer_ID;
typedef void *Timer_Data;
typedef void (*Timer_Proc) (Timer_ID, Timer_Data);

extern Timer_ID set_timer(unsigned, Timer_Proc, Timer_Data);
extern Timer_ID set_virtual_timer(unsigned, Timer_Proc, Timer_Data);
extern int cancel_timer(Timer_ID);
extern void reenable_timers(void);
extern unsigned timer_wakeup_interval(Timer_ID);
extern void timer_sleep(unsigned seconds);
extern int virtual_timer_available();

#endif				/* !Timers_H */

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:19:10  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:19:33  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:09:00  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:56:13  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.3  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.2  1992/09/23  17:12:25  pavel
 * Added protection against this file being included more than once.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
