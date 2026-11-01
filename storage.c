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

#include "storage.h"

#include "config.h"
#include "options.h"

#include "my-stdlib.h"
#include "my-string.h"

#include "exceptions.h"
#include "list.h"
#include "ref_count.h"
#include "structures.h"
#include "utils.h"

static unsigned alloc_num[Sizeof_Memory_Type];

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
	return MAX(sizeof(int), sizeof(FlNum));
    case M_STRING:
#ifdef MEMO_STRLEN
	/* yes, we depend on unsigned and int being the
	 * same size; this will need to be Different if
	 * we ever want to use size_t here (we probably won't)
	 */
	return sizeof(unsigned) + sizeof(int);
#else
	return sizeof(int);
#endif /* MEMO_STRLEN */
    case M_LIST:
	/* for systems with picky pointer alignment */
	return MAX(sizeof(int), sizeof(Var *));

#ifdef WAIF_CORE
    case M_WAIF:
	/* for systems with picky pointer alignment */
	return MAX(sizeof(int), sizeof(void *));
#endif

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

    if (offs) {
	memptr += offs;
	((int *) memptr)[-1] = 1;
#ifdef MEMO_STRLEN
	if (type == M_STRING)
	    ((unsigned *) memptr)[-2] = size - 1;
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

    ptr = realloc((char *) ptr - offs, size + offs);
    if (!ptr) {
	sprintf(msg, "memory re-allocation (size %u) failed!", size);
	panic(msg);
    }
    return (char *) ptr + offs;
}

void
myfree(const void *ptr, Memory_Type type)
{
    alloc_num[type]--;
    free((char *) ptr - refcount_overhead(type));
}

Var
memory_usage(void)
{
    return new_list(0);
}


/*
 * $Log$
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
