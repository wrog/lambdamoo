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

#include "my-ctype.h"
#include "my-stdio.h"
#include "my-string.h"

#include "config.h"
#include "db.h"
#include "db_io.h"
#include "exceptions.h"
#include "list.h"
#include "log.h"
#include "match.h"
#include "numbers.h"
#include "ref_count.h"
#include "server.h"
#include "storage.h"
#include "streams.h"
#include "structures.h"
#include "utils.h"

/*
 * These versions of strcasecmp() and strncasecmp() depend on ASCII.
 * We implement them here because neither one is in the ANSI standard.
 */

static const char cmap[] =
"\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017"
"\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037"
"\040\041\042\043\044\045\046\047\050\051\052\053\054\055\056\057"
"\060\061\062\063\064\065\066\067\070\071\072\073\074\075\076\077"
"\100\141\142\143\144\145\146\147\150\151\152\153\154\155\156\157"
"\160\161\162\163\164\165\166\167\170\171\172\133\134\135\136\137"
"\140\141\142\143\144\145\146\147\150\151\152\153\154\155\156\157"
"\160\161\162\163\164\165\166\167\170\171\172\173\174\175\176\177"
"\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217"
"\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237"
"\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257"
"\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277"
"\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317"
"\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337"
"\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357"
"\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377";

int
mystrcasecmp(const char *ss, const char *tt)
{
    register const unsigned char *s = (const unsigned char *) ss;
    register const unsigned char *t = (const unsigned char *) tt;

    if (s == t) {
	return 0;
    }
    while (cmap[*s] == cmap[*t++]) {
	if (!*s++)
	    return 0;
    }
    return (cmap[*s] - cmap[*--t]);
}

int
mystrncasecmp(const char *ss, const char *tt, int n)
{
    const unsigned char *s = (const unsigned char *) ss;
    const unsigned char *t = (const unsigned char *) tt;

    if (!n || ss == tt)
	return 0;
    while (cmap[*s] == cmap[*t++]) {
	if (!*s++ || !--n)
	    return 0;
    }
    return (cmap[*s] - cmap[*--t]);
}

int
verbcasecmp(const char *verb, const char *word)
{
    const unsigned char *w;
    const unsigned char *v = (const unsigned char *) verb;
    enum {
	none, inner, end
    } star;

    if (verb == word) {
	return 1;
    }
    while (*v) {
	w = (const unsigned char *) word;
	star = none;
	while (1) {
	    while (*v == '*') {
		v++;
		star = (!*v || *v == ' ') ? end : inner;
	    }
	    if (!*v || *v == ' ' || !*w || cmap[*w] != cmap[*v])
		break;
	    w++;
	    v++;
	}
	if (!*w ? (star != none || !*v || *v == ' ')
	    : (star == end))
	    return 1;
	while (*v && *v != ' ')
	    v++;
	while (*v == ' ')
	    v++;
    }
    return 0;
}

unsigned
str_hash(const char *s)
{
    register unsigned ans = 0;

    while (*s) {
	ans = (ans << 3) + (ans >> 28) + cmap[(unsigned char) *s++];
    }
    return ans;
}

void
complex_free_var(Var v)
{
    int i;

    switch ((int) v.type) {
    case TYPE_STR:
	if (v.v.str)
	    free_str(v.v.str);
	break;
    case TYPE_LIST:
	if (delref(v.v.list) == 0) {
	    Var *pv;

	    for (i = v.v.list[0].v.num, pv = v.v.list + 1; i > 0; i--, pv++)
		free_var(*pv);
	    myfree(v.v.list, M_LIST);
	}
	break;
    case TYPE_FLOAT:
	if (delref(v.v.fnum) == 0)
	    myfree(v.v.fnum, M_FLOAT);
	break;
    }
}

Var
complex_var_ref(Var v)
{
    switch ((int) v.type) {
    case TYPE_STR:
	addref(v.v.str);
	break;
    case TYPE_LIST:
	addref(v.v.list);
	break;
    case TYPE_FLOAT:
	addref(v.v.fnum);
	break;
    }
    return v;
}

Var
complex_var_dup(Var v)
{
    int i;
    Var newlist;

    switch ((int) v.type) {
    case TYPE_STR:
	v.v.str = str_dup(v.v.str);
	break;
    case TYPE_LIST:
	newlist = new_list(v.v.list[0].v.num);
	for (i = 1; i <= v.v.list[0].v.num; i++) {
	    newlist.v.list[i] = var_ref(v.v.list[i]);
	}
	v.v.list = newlist.v.list;
	break;
    case TYPE_FLOAT:
	v = new_float(*v.v.fnum);
	break;
    }
    return v;
}

/* could be inlined and use complex_etc like the others, but this should
 * usually be called in a context where we already konw the type.
 */
int
var_refcount(Var v)
{
    switch ((int) v.type) {
    case TYPE_STR:
	return refcount(v.v.str);
	break;
    case TYPE_LIST:
	return refcount(v.v.list);
	break;
    case TYPE_FLOAT:
	return refcount(v.v.fnum);
	break;
    }
    return 1;
}

int
is_true(Var v)
{
    return ((v.type == TYPE_INT && v.v.num != 0)
	    || (v.type == TYPE_FLOAT && *v.v.fnum != 0.0)
	    || (v.type == TYPE_STR && v.v.str && *v.v.str != '\0')
	    || (v.type == TYPE_LIST && v.v.list[0].v.num != 0));
}

int
equality(Var lhs, Var rhs, int case_matters)
{
    if (lhs.type == TYPE_FLOAT || rhs.type == TYPE_FLOAT)
	return do_equals(lhs, rhs);
    else if (lhs.type != rhs.type)
	return 0;
    else {
	switch (lhs.type) {
	case TYPE_INT:
	    return lhs.v.num == rhs.v.num;
	case TYPE_OBJ:
	    return lhs.v.obj == rhs.v.obj;
	case TYPE_ERR:
	    return lhs.v.err == rhs.v.err;
	case TYPE_STR:
	    if (case_matters)
		return !strcmp(lhs.v.str, rhs.v.str);
	    else
		return !mystrcasecmp(lhs.v.str, rhs.v.str);
	case TYPE_LIST:
	    if (lhs.v.list[0].v.num != rhs.v.list[0].v.num)
		return 0;
	    else {
		int i;

		if (lhs.v.list == rhs.v.list) {
		    return 1;
		}
		for (i = 1; i <= lhs.v.list[0].v.num; i++) {
		    if (!equality(lhs.v.list[i], rhs.v.list[i], case_matters))
			return 0;
		}
		return 1;
	    }
	default:
	    panic("EQUALITY: Unknown value type");
	}
    }
    return 0;
}

char *
strsub(const char *source, const char *what, const char *with, int case_counts)
{
    static Stream *str = 0;
    int lwhat = strlen(what);

    if (str == 0)
	str = new_stream(100);

    while (*source) {
	if (!(case_counts ? strncmp(source, what, lwhat)
	      : mystrncasecmp(source, what, lwhat))) {
	    stream_add_string(str, with);
	    source += lwhat;
	} else
	    stream_add_char(str, *source++);
    }

    return reset_stream(str);
}

int
strindex(const char *source, const char *what, int case_counts)
{
    const char *s, *e;
    int lwhat = strlen(what);

    for (s = source, e = source + strlen(source) - lwhat; s <= e; s++) {
	if (!(case_counts ? strncmp(s, what, lwhat)
	      : mystrncasecmp(s, what, lwhat))) {
	    return s - source + 1;
	}
    }
    return 0;
}

int
strrindex(const char *source, const char *what, int case_counts)
{
    const char *s;
    int lwhat = strlen(what);

    for (s = source + strlen(source) - lwhat; s >= source; s--) {
	if (!(case_counts ? strncmp(s, what, lwhat)
	      : mystrncasecmp(s, what, lwhat))) {
	    return s - source + 1;
	}
    }
    return 0;
}

Var
get_system_property(const char *name)
{
    Var value;
    db_prop_handle h;

    if (!valid(SYSTEM_OBJECT)) {
	value.type = TYPE_ERR;
	value.v.err = E_INVIND;
	return value;
    }
    h = db_find_property(SYSTEM_OBJECT, name, &value);
    if (!h.ptr) {
	value.type = TYPE_ERR;
	value.v.err = E_PROPNF;
    } else if (!h.built_in)	/* make two cases the same */
	value = var_ref(value);
    return value;
}

Objid
get_system_object(const char *name)
{
    Var value;

    value = get_system_property(name);
    if (value.type != TYPE_OBJ) {
	free_var(value);
	return NOTHING;
    } else
	return value.v.obj;
}

int
value_bytes(Var v)
{
    int i, len, size = sizeof(Var);

    switch (v.type) {
    case TYPE_STR:
	size += memo_strlen(v.v.str) + 1;
	break;
    case TYPE_FLOAT:
	size += sizeof(double);
	break;
    case TYPE_LIST:
	len = v.v.list[0].v.num;
	size += sizeof(Var);	/* for the `length' element */
	for (i = 1; i <= len; i++)
	    size += value_bytes(v.v.list[i]);
	break;
    default:
	break;
    }

    return size;
}

const char *
raw_bytes_to_binary(const char *buffer, int buflen)
{
    static Stream *s = 0;
    int i;

    if (!s)
	s = new_stream(100);

    for (i = 0; i < buflen; i++) {
	unsigned char c = buffer[i];

	if (c != '~' && (isgraph(c) || c == ' '))
	    stream_add_char(s, c);
	else
	    stream_printf(s, "~%02x", (int) c);
    }

    return reset_stream(s);
}

const char *
binary_to_raw_bytes(const char *binary, int *buflen)
{
    static Stream *s = 0;
    const char *ptr = binary;

    if (!s)
	s = new_stream(100);
    else
	reset_stream(s);

    while (*ptr) {
	unsigned char c = *ptr++;

	if (c != '~')
	    stream_add_char(s, c);
	else {
	    int i;
	    char cc = 0;

	    for (i = 1; i <= 2; i++) {
		c = toupper(*ptr++);
		if (('0' <= c && c <= '9') || ('A' <= c && c <= 'F'))
		    cc = cc * 16 + (c <= '9' ? c - '0' : c - 'A' + 10);
		else
		    return 0;
	    }

	    stream_add_char(s, cc);
	}
    }

    *buflen = stream_length(s);
    return reset_stream(s);
}

char rcsid_utils[] = "$Id$";

/*
 * $Log$
 * Revision 1.8  2006/09/07 00:55:02  bjj
 * Add new MEMO_STRLEN option which uses the refcounting mechanism to
 * store strlen with strings.  This is basically free, since most string
 * allocations are rounded up by malloc anyway.  This saves lots of cycles
 * computing strlen.  (The change is originally from jitmoo, where I wanted
 * inline range checks for string ops).
 *
 * Revision 1.7  2002/08/18 09:47:26  bjj
 * Finally made free_activation() take a pointer after noticing how !$%^&
 * much time it was taking in a particular profiling run.
 *
 * Revision 1.5  1999/08/09 02:36:33  nop
 * Shortcut various equality tests if we have pointer equality.
 *
 * Revision 1.4  1998/12/14 13:19:14  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.3  1997/07/07 03:24:55  nop
 * Merge UNSAFE_OPTS (r5) after extensive testing.
 *
 * Revision 1.2.2.3  1997/03/21 15:11:22  bjj
 * add var_refcount interface
 *
 * Revision 1.2.2.2  1997/03/21 14:29:03  bjj
 * Some code bumming in complex_free_var (3rd most expensive function!)
 *
 * Revision 1.2.2.1  1997/03/20 18:07:48  bjj
 * Add a flag to the in-memory type identifier so that inlines can cheaply
 * identify Vars that need actual work done to ref/free/dup them.  Add the
 * appropriate inlines to utils.h and replace old functions in utils.c with
 * complex_* functions which only handle the types with external storage.
 *
 * Revision 1.2  1997/03/03 04:19:36  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:01  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.8  1996/04/08  00:43:09  pavel
 * Changed definition of `value_bytes()' to add in `sizeof(Var)'.
 * Release 1.8.0p3.
 *
 * Revision 2.7  1996/03/11  23:34:41  pavel
 * Changed %X to %x in a stream_printf call, since I don't want to support
 * both upper- and lower-case.  Release 1.8.0p1.
 *
 * Revision 2.6  1996/03/10  01:14:16  pavel
 * Change the format of binary strings to use hex instead of octal.
 * Release 1.8.0.
 *
 * Revision 2.5  1996/02/08  06:41:04  pavel
 * Added support for floating-point.  Moved compare_ints() to numbers.c.
 * Renamed err/logf() to errlog/oklog() and TYPE_NUM to TYPE_INT.  Updated
 * copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.4  1996/01/16  07:24:48  pavel
 * Removed special format for `~' in binary strings.  Release 1.8.0alpha6.
 *
 * Revision 2.3  1996/01/11  07:40:01  pavel
 * Added raw_bytes_to_binary() and binary_to_raw_bytes(), in support of binary
 * I/O facilities.  Release 1.8.0alpha5.
 *
 * Revision 2.2  1995/12/28  00:38:54  pavel
 * Neatened up implementation of case-folding string comparison functions.
 * Release 1.8.0alpha3.
 *
 * Revision 2.1  1995/12/11  08:09:31  pavel
 * Account for newly-clarified reference-counting behavior for built-in
 * properties.  Add `value_bytes()' from elsewhere.  Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  04:43:24  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.13  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.12  1992/10/23  22:23:18  pavel
 * Eliminated all uses of the useless macro NULL.
 *
 * Revision 1.11  1992/10/17  20:57:08  pavel
 * Global rename of strdup->str_dup, strref->str_ref, vardup->var_dup, and
 * varref->var_ref.
 * Removed useless #ifdef of STRCASE.
 *
 * Revision 1.10  1992/09/14  17:39:53  pjames
 * Moved db_modification code to db modules.
 *
 * Revision 1.9  1992/09/08  21:55:47  pjames
 * Updated #includes.
 *
 * Revision 1.8  1992/09/03  16:23:49  pjames
 * Make cmap[] visible.  Added TYPE_CLEAR handling.
 *
 * Revision 1.7  1992/08/28  16:23:41  pjames
 * Changed vardup to varref.
 * Changed myfree(*, M_STRING) to free_str(*).
 * Added `varref()'.
 * Changed `free_var()' to check `delref()' before freeing.
 * Removed `copy_pi()'.
 *
 * Revision 1.6  1992/08/18  00:54:40  pavel
 * Fixed typo.
 *
 * Revision 1.5  1992/08/18  00:41:14  pavel
 * Fixed boundary-condition bugs in index() and rindex(), when the search string
 * is empty.
 *
 * Revision 1.4  1992/08/11  17:26:56  pjames
 * Removed read/write Parse_Info procedures.
 *
 * Revision 1.3  1992/08/10  16:40:20  pjames
 * Added is_true (from execute.c) and *_activ_as_pi routines.  Added a
 * check for null parse_info in write_pi, because parse_infos are no
 * longer stored in each activation.
 *
 * Revision 1.2  1992/07/21  00:07:48  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS ident. string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
