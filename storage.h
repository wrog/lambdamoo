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

#ifndef Storage_h
#define Storage_h 1

#include "my-string.h"

#include "structures.h"
#include "ref_count.h"

typedef enum Memory_Type {
    M_AST_POOL, M_AST, M_PROGRAM, M_PVAL, M_NETWORK, M_STRING, M_VERBDEF,
    M_LIST, M_PREP, M_PROPDEF, M_OBJECT_TABLE, M_OBJECT, M_FLOAT,
    M_STREAM, M_NAMES, M_ENV, M_TASK, M_PATTERN,

    M_BYTECODES, M_FORK_VECTORS, M_LIT_LIST,
    M_PROTOTYPE, M_CODE_GEN, M_DISASSEMBLE, M_DECOMPILE,

    M_RT_STACK, M_RT_ENV, M_BI_FUNC_DATA, M_VM,

    M_REF_ENTRY, M_REF_TABLE, M_VC_ENTRY, M_VC_TABLE, M_STRING_PTRS,
    M_INTERN_POINTER, M_INTERN_ENTRY, M_INTERN_HUNK,

    Sizeof_Memory_Type

} Memory_Type;

extern char *str_dup(const char *);
extern const char *str_ref(const char *);
extern Var memory_usage(void);

extern void myfree(void *where, Memory_Type type);
extern void *mymalloc(unsigned size, Memory_Type type);
extern void *myrealloc(void *where, unsigned size, Memory_Type type);

static inline void		/* XXX was extern, fix for non-gcc compilers */
free_str(const char *s)
{
    if (delref(s) == 0)
	myfree((void *) s, M_STRING);
}

#ifdef MEMO_STRLEN
/*
 * Using the same mechanism as ref_count.h uses to hide Value ref counts,
 * keep a memozied strlen in the storage with the string.
 */
#define memo_strlen(X)		((void)0, (((int *)(X))[-2]))
#else
#define memo_strlen(X)		strlen(X)

#endif /* MEMO_STRLEN */

#endif				/* Storage_h */

/*
 * $Log$
 * Revision 1.7  2006/12/06 23:44:56  wrog
 * Fix compiler warnings about redefining strlen/strcmp
 *
 * Revision 1.6  2006/09/07 00:55:02  bjj
 * Add new MEMO_STRLEN option which uses the refcounting mechanism to
 * store strlen with strings.  This is basically free, since most string
 * allocations are rounded up by malloc anyway.  This saves lots of cycles
 * computing strlen.  (The change is originally from jitmoo, where I wanted
 * inline range checks for string ops).
 *
 * Revision 1.5  1998/12/14 13:19:00  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.4  1998/02/19 07:36:17  nop
 * Initial string interning during db load.
 *
 * Revision 1.3  1997/07/07 03:24:55  nop
 * Merge UNSAFE_OPTS (r5) after extensive testing.
 *
 * Revision 1.2.2.4  1997/05/29 20:47:33  nop
 * Stupid hack to allow non-gcc compilers to use -Dinline= to make the server
 * compile.
 *
 * Revision 1.2.2.3  1997/05/20 03:01:34  nop
 * parse_into_words was allocating pointers to strings as strings.  Predictably,
 * the refcount prepend code was not prepared for this, causing unaligned memory
 * access on the Alpha.  Added new M_STRING_PTRS allocation class that could
 * be renamed to something better, perhaps.
 *
 * Revision 1.2.2.2  1997/03/21 15:19:24  bjj
 * add myrealloc interface, inline free_str
 *
 * Revision 1.2.2.1  1997/03/20 07:26:04  nop
 * First pass at the new verb cache.  Some ugly code inside.
 *
 * Revision 1.2  1997/03/03 04:19:27  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:04  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:13:09  pavel
 * Added M_FLOAT, removed unused M_PI.  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:55:11  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.7  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.6  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.5  1992/10/17  20:53:28  pavel
 * Global rename of strdup->str_dup, strref->str_ref, vardup->var_dup, and
 * varref->var_ref, to avoid conflict on systems that provide their own
 * definition of strdup().
 *
 * Revision 1.4  1992/09/14  17:39:30  pjames
 * Moved db_modification code to db modules.
 *
 * Revision 1.3  1992/08/31  22:23:29  pjames
 * Changed some `char *'s to `const char *'
 *
 * Revision 1.2  1992/08/28  16:25:17  pjames
 * Added memory types for refcount module.  Added `strref()' and `free_str()'.
 *
 * Revision 1.1  1992/08/10  16:20:00  pjames
 * Initial RCS-controlled version
 */
