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
#include "db.h"
#include "quota.h"
#include "structures.h"

static const char *quota_name = "ownership_quota";

int
decr_quota(Objid player)
{
    db_prop_handle h;
    Var v;

    if (!valid(player))
	return 1;

    h = db_find_property(player, quota_name, &v);
    if (!h.ptr)
	return 1;

    if (v.type != TYPE_INT)
	return 1;

    if (v.v.num <= 0)
	return 0;

    v.v.num--;
    db_set_property_value(h, v);
    return 1;
}

void
incr_quota(Objid player)
{
    db_prop_handle h;
    Var v;

    if (!valid(player))
	return;

    h = db_find_property(player, quota_name, &v);
    if (!h.ptr)
	return;

    if (v.type != TYPE_INT)
	return;

    v.v.num++;
    db_set_property_value(h, v);
}

char rcsid_quota[] = "$Id$";

/*
 * $Log$
 * Revision 1.3  1998/12/14 13:18:51  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.2  1997/03/03 04:19:19  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:01  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:54:04  pavel
 * Renamed TYPE_NUM to TYPE_INT.  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:30:55  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.6  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.5  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.4  1992/09/14  17:43:25  pjames
 * Moved db_modification code to db modules.
 *
 * Revision 1.3  1992/08/10  16:54:57  pjames
 * Updated #includes.
 *
 * Revision 1.2  1992/07/21  00:06:18  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS ident. string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
