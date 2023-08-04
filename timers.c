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

#include "my-signal.h"
#include "my-stdlib.h"
#include "my-sys-time.h"
#include "my-time.h"
#include "my-unistd.h"

#include "config.h"
#include "timers.h"

#if (defined(MACH) && defined(CMU)) || !defined(SIGVTALRM)
/* Virtual interval timers are broken on Mach 3.0 */
#  undef ITIMER_VIRTUAL
#endif

typedef struct Timer_Entry Timer_Entry;
struct Timer_Entry {
    Timer_Entry *next;
    time_t when;
    Timer_Proc proc;
    Timer_Data data;
    Timer_ID id;
};

static Timer_Entry *active_timers = 0;
static Timer_Entry *free_timers = 0;
static Timer_Entry *virtual_timer = 0;
static Timer_ID next_id = 0;

static Timer_Entry *
allocate_timer(void)
{
    if (free_timers) {
	Timer_Entry *this = free_timers;

	free_timers = this->next;
	return this;
    } else
	return (Timer_Entry *) malloc(sizeof(Timer_Entry));
}

static void
free_timer(Timer_Entry * this)
{
    this->next = free_timers;
    free_timers = this;
}

static void restart_timers(void);

static void
wakeup_call(int signo)
{
    Timer_Entry *this = active_timers;
    Timer_Proc proc = this->proc;
    Timer_ID id = this->id;
    Timer_Data data = this->data;

    active_timers = active_timers->next;
    free_timer(this);
    restart_timers();
    if (proc)
	(*proc) (id, data);
}


#ifdef ITIMER_VIRTUAL
static void
virtual_wakeup_call(int signo)
{
    Timer_Entry *this = virtual_timer;
    Timer_Proc proc = this->proc;
    Timer_ID id = this->id;
    Timer_Data data = this->data;

    virtual_timer = 0;
    free_timer(this);
    if (proc)
	(*proc) (id, data);
}
#endif

static void
stop_timers()
{
    alarm(0);
    signal(SIGALRM, SIG_IGN);
    signal(SIGALRM, wakeup_call);

#ifdef ITIMER_VIRTUAL
    {
	struct itimerval itimer, oitimer;

	itimer.it_value.tv_sec = 0;
	itimer.it_value.tv_usec = 0;
	itimer.it_interval.tv_sec = 0;
	itimer.it_interval.tv_usec = 0;

	setitimer(ITIMER_VIRTUAL, &itimer, &oitimer);
	signal(SIGVTALRM, SIG_IGN);
	signal(SIGVTALRM, virtual_wakeup_call);
	if (virtual_timer)
	    virtual_timer->when = oitimer.it_value.tv_sec;
    }
#endif
}

static void
restart_timers()
{
    if (active_timers) {
	time_t now = time(0);

	signal(SIGALRM, wakeup_call);

	if (now < active_timers->when)	/* first timer is in the future */
	    alarm(active_timers->when - now);
	else
	    kill(getpid(), SIGALRM);	/* we're already late... */
    }
#ifdef ITIMER_VIRTUAL

    if (virtual_timer) {
	signal(SIGVTALRM, virtual_wakeup_call);

	if (virtual_timer->when > 0) {
	    struct itimerval itimer;

	    itimer.it_value.tv_sec = virtual_timer->when;
	    itimer.it_value.tv_usec = 0;
	    itimer.it_interval.tv_sec = 0;
	    itimer.it_interval.tv_usec = 0;

	    setitimer(ITIMER_VIRTUAL, &itimer, 0);
	} else
	    kill(getpid(), SIGVTALRM);
    }
#endif
}

Timer_ID
set_timer(unsigned seconds, Timer_Proc proc, Timer_Data data)
{
    Timer_Entry *this = allocate_timer();
    Timer_Entry **t;

    this->id = next_id++;
    this->when = time(0) + seconds;
    this->proc = proc;
    this->data = data;

    stop_timers();

    t = &active_timers;
    while (*t && this->when >= (*t)->when)
	t = &((*t)->next);
    this->next = *t;
    *t = this;

    restart_timers();

    return this->id;
}

Timer_ID
set_virtual_timer(unsigned seconds, Timer_Proc proc, Timer_Data data)
{
#ifdef ITIMER_VIRTUAL

    if (virtual_timer)
	return -1;

    stop_timers();

    virtual_timer = allocate_timer();
    virtual_timer->id = next_id++;
    virtual_timer->when = seconds;
    virtual_timer->proc = proc;
    virtual_timer->data = data;

    restart_timers();

    return virtual_timer->id;

#else				/* !ITIMER_VIRTUAL */

    return set_timer(seconds, proc, data);

#endif
}

int
virtual_timer_available()
{
#ifdef ITIMER_VIRTUAL
    return 1;
#else
    return 0;
#endif
}

unsigned
timer_wakeup_interval(Timer_ID id)
{
    Timer_Entry *t;

#ifdef ITIMER_VIRTUAL

    if (virtual_timer && virtual_timer->id == id) {
	struct itimerval itimer;

	getitimer(ITIMER_VIRTUAL, &itimer);
	return itimer.it_value.tv_sec;
    }
#endif

    for (t = active_timers; t; t = t->next)
	if (t->id == id)
	    return t->when - time(0);;

    return 0;
}

void
timer_sleep(unsigned seconds)
{
    set_timer(seconds, 0, 0);
    pause();
}

int
cancel_timer(Timer_ID id)
{
    Timer_Entry **t = &active_timers;
    int found = 0;

    stop_timers();

    if (virtual_timer && virtual_timer->id == id) {
	free_timer(virtual_timer);
	virtual_timer = 0;
	found = 1;
    } else {
	while (*t) {
	    if ((*t)->id == id) {
		Timer_Entry *tt = *t;

		*t = tt->next;
		found = 1;
		free_timer(tt);
		break;
	    }
	    t = &((*t)->next);
	}
    }

    restart_timers();

    return found;
}

void
reenable_timers(void)
{
#if HAVE_SIGEMPTYSET
    sigset_t sigs;

    sigemptyset(&sigs);
    sigaddset(&sigs, SIGALRM);
    sigprocmask(SIG_UNBLOCK, &sigs, 0);
#else
#if HAVE_SIGSETMASK
    int old_mask = sigsetmask(-1);	/* block everything, get old mask */

    old_mask &= ~sigmask(SIGALRM);	/* clear blocked bit for SIGALRM */
    sigsetmask(old_mask);	/* reset the signal mask */
#else
#if HAVE_SIGRELSE
    sigrelse(SIGALRM);		/* restore previous signal action */
#else
          #error I need some way to stop blocking SIGALRM!
#endif
#endif
#endif
}

char rcsid_timers[] = "$Id$";

/*
 * $Log$
 * Revision 1.4  2002/08/16 03:00:44  bjj
 * free cancelled timers with free_timer() instead of free() so they can
 * get re-used (typically there's only a handful) (harmless)
 *
 * Revision 1.3  1998/12/14 13:19:09  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:19:33  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:01  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:43:56  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:41:20  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.9  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.8  1992/10/23  22:24:08  pavel
 * Eliminated all uses of the useless macro NULL.
 *
 * Revision 1.7  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.6  1992/10/17  21:01:52  pavel
 * #undef'd ITIMER_VIRTUAL on CMU MACH because it apparently doesn't work on
 * that system.
 * Added an #ifdef of ITIMER_VIRTUAL around the virtual_wakeup_call()
 * signal-handler.
 *
 * Revision 1.5  1992/09/22  22:48:59  pavel
 * Added missing #include of "config.h".
 *
 * Revision 1.4  1992/08/21  00:45:29  pavel
 * Changed to conditionalize on the option AVOID_POSIX rather than USE_POSIX.
 *
 * Revision 1.3  1992/07/30  00:42:45  pavel
 * Add support for compiling on RISC/os 4.52 and NonStop-UX A22.
 *
 * Revision 1.2  1992/07/21  00:07:26  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS ident. string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
