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

#include "my-stdlib.h"

#include "config.h"
#include "exceptions.h"
#include "list.h"
#include "options.h"
#include "ref_count.h"
#include "storage.h"
#include "structures.h"
#include "utils.h"

static unsigned alloc_num[Sizeof_Memory_Type];
#ifdef USE_GNU_MALLOC
static unsigned alloc_size[Sizeof_Memory_Type], alloc_real_size[Sizeof_Memory_Type];
#endif

static inline int
refcount_overhead(Memory_Type type)
{
    /* These are the only allocation types that are addref()'d.
     * As long as we're living on the wild side, avoid getting the
     * refcount slot for allocations that won't need it.
     */
    switch (type) {
    case M_FLOAT:
	/* for systems with picky double alignment */
	return MAX(sizeof(int), sizeof(double));
    case M_STRING:
#ifdef MEMO_STRLEN
	return sizeof(int) + sizeof(int);
#else
	return sizeof(int);
#endif /* MEMO_STRLEN */
    case M_LIST:
	/* for systems with picky pointer alignment */
	return MAX(sizeof(int), sizeof(Var *));
    default:
	return 0;
    }
}

void *
mymalloc(unsigned size, Memory_Type type)
{
    char *memptr;
    char msg[100];
    int offs;

    if (size == 0)		/* For queasy systems */
	size = 1;

    offs = refcount_overhead(type);
    memptr = (char *) malloc(size + offs);
    if (!memptr) {
	sprintf(msg, "memory allocation (size %u) failed!", size);
	panic(msg);
    }
    alloc_num[type]++;
#ifdef USE_GNU_MALLOC
    {
	extern unsigned malloc_real_size(void *ptr);
	extern unsigned malloc_size(void *ptr);

	alloc_size[type] += malloc_size(memptr);
	alloc_real_size[type] += malloc_real_size(memptr);
    }
#endif

    if (offs) {
	memptr += offs;
	((int *) memptr)[-1] = 1;
#ifdef MEMO_STRLEN
	if (type == M_STRING)
	    ((int *) memptr)[-2] = size - 1;
#endif /* MEMO_STRLEN */
    }
    return memptr;
}

const char *
str_ref(const char *s)
{
    addref(s);
    return s;
}

char *
str_dup(const char *s)
{
    char *r;

    if (s == 0 || *s == '\0') {
	static char *emptystring;

	if (!emptystring) {
	    emptystring = (char *) mymalloc(1, M_STRING);
	    *emptystring = '\0';
	}
	addref(emptystring);
	return emptystring;
    } else {
	r = (char *) mymalloc(strlen(s) + 1, M_STRING);	/* NO MEMO HERE */
	strcpy(r, s);
    }
    return r;
}

void *
myrealloc(void *ptr, unsigned size, Memory_Type type)
{
    int offs = refcount_overhead(type);
    static char msg[100];

#ifdef USE_GNU_MALLOC
    {
	extern unsigned malloc_real_size(void *ptr);
	extern unsigned malloc_size(void *ptr);

	alloc_size[type] -= malloc_size(ptr);
	alloc_real_size[type] -= malloc_real_size(ptr);
#endif

	ptr = realloc((char *) ptr - offs, size + offs);
	if (!ptr) {
	    sprintf(msg, "memory re-allocation (size %u) failed!", size);
	    panic(msg);
	}
#ifdef USE_GNU_MALLOC
	alloc_size[type] += malloc_size(ptr);
	alloc_real_size[type] += malloc_real_size(ptr);
    }
#endif

    return (char *) ptr + offs;
}

void
myfree(void *ptr, Memory_Type type)
{
    alloc_num[type]--;
#ifdef USE_GNU_MALLOC
    {
	extern unsigned malloc_real_size(void *ptr);
	extern unsigned malloc_size(void *ptr);

	alloc_size[type] -= malloc_size(ptr);
	alloc_real_size[type] -= malloc_real_size(ptr);
    }
#endif

    free((char *) ptr - refcount_overhead(type));
}

#ifdef USE_GNU_MALLOC
struct mstats_value {
    int blocksize;
    int nfree;
    int nused;
};

extern struct mstats_value malloc_stats(int size);
#endif

/* XXX stupid fix for non-gcc compilers, already in storage.h */
#ifdef NEVER
void
free_str(const char *s)
{
    if (delref(s) == 0)
	myfree((void *) s, M_STRING);
}

#endif

Var
memory_usage(void)
{
    Var r;

#ifdef USE_GNU_MALLOC
    int nsizes, i;

    /* Discover how many block sizes there are. */
    for (nsizes = 0;; nsizes++) {
	struct mstats_value v;

	v = malloc_stats(nsizes);
	if (v.blocksize <= 0)
	    break;
    }

    /* Get all of the allocation out of the way before getting the stats. */
    r = new_list(nsizes);
    for (i = 1; i <= nsizes; i++)
	r.v.list[i] = new_list(3);

    for (i = 0; i < nsizes; i++) {
	struct mstats_value v;
	Var l;

	v = malloc_stats(i);
	l = r.v.list[i + 1];
	l.v.list[1].type = l.v.list[2].type = l.v.list[3].type = TYPE_INT;
	l.v.list[1].v.num = v.blocksize;
	l.v.list[2].v.num = v.nused;
	l.v.list[3].v.num = v.nfree;
    }
#else
    r = new_list(0);
#endif

    return r;
}

char rcsid_storage[] = "$Id$";

/*
 * $Log$
 * Revision 1.6  2006/09/07 00:55:02  bjj
 * Add new MEMO_STRLEN option which uses the refcounting mechanism to
 * store strlen with strings.  This is basically free, since most string
 * allocations are rounded up by malloc anyway.  This saves lots of cycles
 * computing strlen.  (The change is originally from jitmoo, where I wanted
 * inline range checks for string ops).
 *
 * Revision 1.5  1998/12/14 13:18:59  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.4  1997/07/07 03:24:55  nop
 * Merge UNSAFE_OPTS (r5) after extensive testing.
 *
 * Revision 1.3.2.3  1997/05/29 20:47:32  nop
 * Stupid hack to allow non-gcc compilers to use -Dinline= to make the server
 * compile.
 *
 * Revision 1.3.2.2  1997/03/21 15:19:23  bjj
 * add myrealloc interface, inline free_str
 *
 * Revision 1.3.2.1  1997/03/20 18:59:26  bjj
 * Allocate refcounts with objects that can be addref()'d (strings, lists,
 * floats).  Use macros to manipulate those counts.  This completely replaces
 * the external hash table addref and friends.
 *
 * Revision 1.3  1997/03/03 06:32:10  bjj
 * str_dup("") now returns the same empty string to every caller
 *
 * Revision 1.2  1997/03/03 04:19:26  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:01  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:51:20  pavel
 * Renamed TYPE_NUM to TYPE_INT.  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:31:30  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.16  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.15  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.14  1992/10/17  20:52:37  pavel
 * Global rename of strdup->str_dup, strref->str_ref, vardup->var_dup, and
 * varref->var_ref.
 *
 * Revision 1.13  1992/09/14  18:38:42  pjames
 * Updated #includes.  Moved rcsid to bottom.
 *
 * Revision 1.12  1992/09/14  17:41:16  pjames
 * Moved db_modification code to db modules.
 *
 * Revision 1.11  1992/09/03  16:26:29  pjames
 * Added TYPE_CLEAR handling.
 * Changed property definition manipulating to work with arrays.
 *
 * Revision 1.10  1992/08/31  22:25:27  pjames
 * Changed some `char *'s to `const char *'
 *
 * Revision 1.9  1992/08/28  16:21:13  pjames
 * Changed vardup to varref.
 * Changed myfree(*, M_STRING) to free_str(*).
 * Added `strref()' and `free_str()'
 *
 * Revision 1.8  1992/08/21  00:42:18  pavel
 * Renamed include file "parse_command.h" to "parse_cmd.h".
 *
 * Changed to conditionalize on the option USE_GNU_MALLOC instead of
 * USE_SYSTEM_MALLOC.
 *
 * Removed use of worthless constant DB_INITIAL_SIZE, defined in config.h.
 *
 * Revision 1.7  1992/08/14  00:00:45  pavel
 * Converted to a typedef of `var_type' = `enum var_type'.
 *
 * Revision 1.6  1992/08/10  16:52:45  pjames
 * Updated #includes.
 *
 * Revision 1.5  1992/07/30  21:23:10  pjames
 * Casted malloc to (void *) because of a problem with some system.
 *
 * Revision 1.4  1992/07/27  19:05:18  pjames
 * Removed a debugging statement.
 *
 * Revision 1.3  1992/07/27  18:17:41  pjames
 * Changed name of ct_env to var_names and M_CT_ENV to M_NAMES.
 *
 * Revision 1.2  1992/07/21  00:06:38  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS ident. string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
