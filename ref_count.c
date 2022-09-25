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
#include "exceptions.h"
#include "ref_count.h"
#include "storage.h"

#if 0

typedef struct reftab_entry {
    struct reftab_entry *next;
    const void *p;
    int count;
} reftab_entry;

static int table_size;
static int table_entries;
static int table_power;		/* table_power is the power of 2 */
static reftab_entry **ref_table;

static int init = 0;

static reftab_entry *free_list;

static void
init_ref_table()
{
    int index;

    table_size = 1024;
    table_entries = 0;
    table_power = 10;
    ref_table = mymalloc(table_size * sizeof(reftab_entry *), M_REF_TABLE);
    for (index = 0; index < table_size; index++)
	ref_table[index] = 0;
    free_list = 0;
    init = 1;
}

static int
key(const void *p)
{
    return ((long) p >> 4) & ((1 << table_power) - 1);
}

static reftab_entry *
new_entry()
{
    reftab_entry *ans;

    table_entries++;
    if (free_list) {
	ans = free_list;
	free_list = free_list->next;
    } else
	ans = mymalloc(sizeof(reftab_entry), M_REF_ENTRY);
    return ans;
}

static void
save_entry(reftab_entry * e)
{
    table_entries--;
    e->next = free_list;
    free_list = e;
}

static void
ll_insert_entry(reftab_entry ** list, reftab_entry * e)
{
    if ((*list) == 0) {
	e->next = 0;
	*list = e;
    } else if ((*list)->p > e->p) {
	e->next = *list;
	*list = e;
    } else if ((*list)->p == e->p)
	panic("Error encountered during rehashing");
    else
	ll_insert_entry(&((*list)->next), e);
}

static void
ll_insert_value(reftab_entry ** list, const void *p)
     /* returns 1 if added a new entry */
{
    reftab_entry *entry;

    if ((*list == 0) || (*list)->p > p) {
	entry = new_entry();
	entry->p = p;
	entry->count = 2;
	entry->next = *list;
	*list = entry;
    } else if ((*list)->p == p) {
	(*list)->count++;
    } else
	ll_insert_value(&((*list)->next), p);
}

static int
ll_delete_value(reftab_entry ** list, const void *p)
     /* return number of references to p left */
{
    reftab_entry **prev = list, *tmp;

    while ((*list) && (*list)->p < p) {
	prev = list;
	list = &((*list)->next);
    }

    /* not found, must be last reference */
    if (*list == 0 || (*list)->p > p)
	return 0;

    else {			/* found */

	/* not next to last reference */
	if ((*list)->count > 2)
	    return --((*list)->count);

	/* too few references */
	else if ((*list)->count < 2)
	    panic("Error when deleting reference: count too low");

	/* next to last reference: remove entry */
	else {			/* count == 2 */
	    tmp = *list;
	    (*prev)->next = (*list)->next;
	    if (list == prev)	/* head of list */
		*prev = (*list)->next;
	    save_entry(tmp);
	    return 1;
	}
    }
    return -1;			/* shouldn't reach here */
}

static reftab_entry **
rehash(reftab_entry ** old, reftab_entry ** new)
{
    reftab_entry *link, *next;
    int loop;

    for (loop = 0; loop < table_size / 2; loop++)
	for (link = old[loop]; link; link = next) {
	    int index = key(link->p);
	    next = link->next;
	    ll_insert_entry(&new[index], link);
	}
    myfree(old, M_REF_TABLE);
    return new;
}

void
addref(const void *p)
{
    int index;

    if (!init)
	init_ref_table();

    if (p == 0)
	panic("increasing ref count of 0x0");

    index = key(p);
    ll_insert_value(&(ref_table[index]), p);

    if (table_entries > table_size) {
	reftab_entry **new_table;
	table_power++;
	table_size *= 2;
	new_table = mymalloc(table_size * sizeof(reftab_entry *), M_REF_TABLE);
	for (index = 0; index < table_size; index++)
	    new_table[index] = 0;
	ref_table = rehash(ref_table, new_table);	/* frees old table */
    }
}

unsigned int
delref(const void *p)
{
    int index;

    if (!init)
	init_ref_table();

    if (p == 0)
	panic("decreasing ref count of 0x0");

    index = key(p);
    return ll_delete_value(&(ref_table[index]), p);
}
#endif

char rcsid_ref_count[] = "$Id$";

/*
 * $Log$
 * Revision 1.4  1998/12/14 13:18:54  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.3  1997/07/07 03:24:55  nop
 * Merge UNSAFE_OPTS (r5) after extensive testing.
 *
 * Revision 1.2.2.1  1997/03/20 18:59:24  bjj
 * Allocate refcounts with objects that can be addref()'d (strings, lists,
 * floats).  Use macros to manipulate those counts.  This completely replaces
 * the external hash table addref and friends.
 *
 * Revision 1.2  1997/03/03 04:19:21  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:01  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:53:33  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:31:09  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.6  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.5  1992/10/23  22:21:58  pavel
 * Eliminated all uses of the useless macro NULL.
 *
 * Revision 1.4  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.3  1992/09/08  21:54:40  pjames
 * Fixed #includes.
 *
 * Revision 1.2  1992/09/03  16:22:25  pjames
 * Cleaned up ll_insert code.  Changed when table will re-hash.
 *
 * Revision 1.1  1992/08/28  16:31:45  pjames
 * Initial RCS-controlled version.
 */
