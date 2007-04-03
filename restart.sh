#!/bin/sh

# Copyright (c) 1992, 1994, 1995, 1996 Xerox Corporation.  All rights reserved.
# Portions of this code were written by Stephen White, aka ghond.
# Use and copying of this software and preparation of derivative works based
# upon this software are permitted.  Any distribution of this software or
# derivative works must comply with all applicable United States export
# control laws.  This software is made available AS IS, and Xerox Corporation
# makes no warranty about the software, its performance or its conformity to
# any specification.  Any person obtaining a copy of this software is requested
# to send their name and post office or electronic mail address to:
#   Pavel Curtis
#   Xerox PARC
#   3333 Coyote Hill Rd.
#   Palo Alto, CA 94304
#   Pavel@Xerox.Com

if [ $# -lt 1 -o $# -gt 2 ]; then
	echo 'Usage: restart dbase-prefix [port]'
	exit 1
fi

if [ ! -r $1.db ]; then
	echo "Unknown database: $1.db"
	exit 1
fi

if [ -r $1.db.new ]; then
	mv $1.db $1.db.old
	mv $1.db.new $1.db
	rm -f $1.db.old.gz
	gzip $1.db.old &
fi

if [ -f $1.log ]; then
	cat $1.log >> $1.log.old
	rm $1.log
fi

echo `date`: RESTARTED >> $1.log
nohup ./moo $1.db $1.db.new $2 >> $1.log 2>&1 &

###############################################################################
# $Log$
# Revision 1.1  1997/03/03 03:45:05  nop
# Initial revision
#
# Revision 2.1  1996/02/08  07:25:45  pavel
# Updated copyright notice for 1996.  Release 1.8.0beta1.
#
# Revision 2.0  1995/11/30  05:14:43  pavel
# New baseline version, corresponding to release 1.8.0alpha1.
#
# Revision 1.2  1994/05/26  16:43:13  pavel
# Fixed up copyright, RCS info; removed debugging switch.
#
# Revision 1.1  1994/05/26  16:41:32  pavel
# Initial revision
###############################################################################
