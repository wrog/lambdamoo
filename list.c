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
#include "my-string.h"
#include "my-math.h"

#include "bf_register.h"
#include "config.h"
#include "exceptions.h"
#include "functions.h"
#include "list.h"
#include "log.h"
#include "md5.h"
#include "options.h"
#include "pattern.h"
#include "random.h"
#include "ref_count.h"
#include "streams.h"
#include "storage.h"
#include "structures.h"
#include "unparse.h"
#include "ucd/ucd.h"
#include "utf.h"
#include "utils.h"

Var
new_list(int size)
{
    Var new;

    if (size == 0) {
	static Var emptylist;

	if (emptylist.v.list == 0) {
	    emptylist.type = TYPE_LIST;
	    emptylist.v.list = mymalloc(1 * sizeof(Var), M_LIST);
	    emptylist.v.list[0].type = TYPE_INT;
	    emptylist.v.list[0].v.num = 0;
	}
	/* give the lucky winner a reference */
	addref(emptylist.v.list);
	return emptylist;
    }
    new.type = TYPE_LIST;
    new.v.list = (Var *) mymalloc((size + 1) * sizeof(Var), M_LIST);
    new.v.list[0].type = TYPE_INT;
    new.v.list[0].v.num = size;
    return new;
}

Var
setadd(Var list, Var value)
{
    if (ismember(value, list, 0)) {
	free_var(value);
	return list;
    }
    return listappend(list, value);
}

Var
setremove(Var list, Var value)
{
    int i;

    if ((i = ismember(value, list, 0)) != 0) {
	return listdelete(list, i);
    } else {
	return list;
    }
}

int
ismember(Var lhs, Var rhs, int case_matters)
{
    int i;

    for (i = 1; i <= rhs.v.list[0].v.num; i++) {
	if (equality(lhs, rhs.v.list[i], case_matters)) {
	    return i;
	}
    }
    return 0;
}

Var
listset(Var list, Var value, int pos)
{
    free_var(list.v.list[pos]);
    list.v.list[pos] = value;
    return list;
}

static Var
doinsert(Var list, Var value, int pos)
{
    Var new;
    int i;
    int size = list.v.list[0].v.num + 1;

    if (var_refcount(list) == 1 && pos == size) {
	list.v.list = (Var *) myrealloc(list.v.list, (size + 1) * sizeof(Var), M_LIST);
	list.v.list[0].v.num = size;
	list.v.list[pos] = value;
	return list;
    }
    new = new_list(size);
    for (i = 1; i < pos; i++)
	new.v.list[i] = var_ref(list.v.list[i]);
    new.v.list[pos] = value;
    for (i = pos; i <= list.v.list[0].v.num; i++)
	new.v.list[i + 1] = var_ref(list.v.list[i]);
    free_var(list);
    return new;
}

Var
listinsert(Var list, Var value, int pos)
{
    if (pos <= 0)
	pos = 1;
    else if (pos > list.v.list[0].v.num)
	pos = list.v.list[0].v.num + 1;
    return doinsert(list, value, pos);
}

Var
listappend(Var list, Var value)
{
    return doinsert(list, value, list.v.list[0].v.num + 1);
}

Var
listdelete(Var list, int pos)
{
    Var new;
    int i;

    new = new_list(list.v.list[0].v.num - 1);
    for (i = 1; i < pos; i++) {
	new.v.list[i] = var_ref(list.v.list[i]);
    }
    for (i = pos + 1; i <= list.v.list[0].v.num; i++)
	new.v.list[i - 1] = var_ref(list.v.list[i]);
    free_var(list);		/* free old list */
    return new;
}

Var
listconcat(Var first, Var second)
{
    int lsecond = second.v.list[0].v.num;
    int lfirst = first.v.list[0].v.num;
    Var new;
    int i;

    new = new_list(lsecond + lfirst);
    for (i = 1; i <= lfirst; i++)
	new.v.list[i] = var_ref(first.v.list[i]);
    for (i = 1; i <= lsecond; i++)
	new.v.list[i + lfirst] = var_ref(second.v.list[i]);
    free_var(first);
    free_var(second);

    return new;
}

Var
listrangeset(Var base, int from, int to, Var value)
{
    /* base and value are free'd */
    int index, offset = 0;
    int val_len = value.v.list[0].v.num;
    int base_len = base.v.list[0].v.num;
    int lenleft = (from > 1) ? from - 1 : 0;
    int lenmiddle = val_len;
    int lenright = (base_len > to) ? base_len - to : 0;
    int newsize = lenleft + lenmiddle + lenright;
    Var ans;

    ans = new_list(newsize);
    for (index = 1; index <= lenleft; index++)
	ans.v.list[++offset] = var_ref(base.v.list[index]);
    for (index = 1; index <= lenmiddle; index++)
	ans.v.list[++offset] = var_ref(value.v.list[index]);
    for (index = 1; index <= lenright; index++)
	ans.v.list[++offset] = var_ref(base.v.list[to + index]);
    free_var(base);
    free_var(value);
    return ans;
}

Var
sublist(Var list, int lower, int upper)
{
    if (lower > upper) {
	free_var(list);
	return new_list(0);
    } else {
	Var r;
	int i;

	r = new_list(upper - lower + 1);
	for (i = lower; i <= upper; i++)
	    r.v.list[i - lower + 1] = var_ref(list.v.list[i]);
	free_var(list);
	return r;
    }
}

/* could get away with a weird calling convention in lieu of malloc, but that
 * might cause Issues down the road, so malloc it is... remember to free it
 * yourself! */

static const char *
float2str(double n)
{
    char *buffer;

    buffer = mymalloc(40, M_STREAM);
    snprintf(buffer, 40, "%.*g", DBL_DIG, n);
    if (!strchr(buffer, '.') && !strchr(buffer, 'e'))
        strncat(buffer, ".0", 40);   /* make it look floating */
    return buffer;
}

static const char *
list2str(Var * args)
{
    static Stream *str = 0;
    int i;
    const char *s;

    if (!str)
	str = new_stream(100);

    for (i = 1; i <= args[0].v.num; i++) {
	switch (args[i].type) {
	case TYPE_INT:
	    stream_printf(str, "%"PRIdN, args[i].v.num);
	    break;
	case TYPE_OBJ:
	    stream_printf(str, "#%"PRIdN, args[i].v.obj);
	    break;
	case TYPE_STR:
	    stream_add_string(str, args[i].v.str);
	    break;
	case TYPE_ERR:
	    stream_add_string(str, unparse_error(args[i].v.err));
	    break;
	case TYPE_FLOAT:
            s = float2str(args[i].v.fnum);
            stream_add_string(str, s);
            myfree(s, M_STREAM);
	    break;
	case TYPE_LIST:
	    stream_add_string(str, "{list}");
	    break;
	default:
	    panic("LIST2STR: Impossible var type.\n");
	}
    }

    return reset_stream(str);
}

const char *
value2str(Var value)
{
    Var list;
    const char *str;

    list = new_list(1);
    list.v.list[1] = var_ref(value);
    str = list2str(list.v.list);
    free_var(list);
    return str;
}

static void
print_to_stream(Var v, Stream * s)
{
    const char *tmp;

    switch (v.type) {
    case TYPE_INT:
	stream_printf(s, "%"PRIdN, v.v.num);
	break;
    case TYPE_OBJ:
	stream_printf(s, "#%"PRIdN, v.v.obj);
	break;
    case TYPE_ERR:
	stream_add_string(s, error_name(v.v.num));
	break;
    case TYPE_FLOAT:
        tmp = float2str(v.v.fnum);
        stream_add_string(s, tmp);
        myfree(tmp, M_STREAM);
	break;
    case TYPE_STR:
	{
	    const char *str = v.v.str;

	    stream_add_char(s, '"');
	    while (*str) {
		switch (*str) {
		case '"':
		case '\\':
		    stream_add_char(s, '\\');
		    /* fall thru */
		default:
		    stream_add_char(s, *str++);
		}
	    }
	    stream_add_char(s, '"');
	}
	break;
    case TYPE_LIST:
	{
	    const char *sep = "";
	    int len, i;

	    stream_add_char(s, '{');
	    len = v.v.list[0].v.num;
	    for (i = 1; i <= len; i++) {
		stream_add_string(s, sep);
		sep = ", ";
		print_to_stream(v.v.list[i], s);
	    }
	    stream_add_char(s, '}');
	}
	break;
    default:
	errlog("PRINT_TO_STREAM: Unknown Var type = %d\n", v.type);
	stream_add_string(s, ">>Unknown value<<");
    }
}

const char *
value_to_literal(Var v)
{
    static Stream *s = 0;

    if (!s)
	s = new_stream(100);

    print_to_stream(v, s);

    return reset_stream(s);
}

Var
strrangeset(Var base, int from, int to, Var value)
{
    /* base and value are free'd */
    int index, offset = 0;
    int val_len = memo_strlen(value.v.str);
    int base_len = memo_strlen(base.v.str);
    int charsleft = (from > 1) ? from - 1 : 0;
    int lenleft = skip_utf(base.v.str, charsleft);
    int lenmiddle = val_len;
    int indright = lenleft + skip_utf(base.v.str + lenleft, to - from + 1);
    int lenright = base_len - indright;
    int newsize = lenleft + lenmiddle + lenright;

    Var ans;
    char *s;

    ans.type = TYPE_STR;
    s = mymalloc(sizeof(char) * (newsize + 1), M_STRING);

    for (index = 0; index < lenleft; index++)
	s[offset++] = base.v.str[index];
    for (index = 0; index < lenmiddle; index++)
	s[offset++] = value.v.str[index];
    for (index = 0; index < lenright; index++)
	s[offset++] = base.v.str[index + indright];
    s[offset] = '\0';
    ans.v.str = s;
    free_var(base);
    free_var(value);
    return ans;
}

Var
substr(Var str, int lower, int upper)
{
    Var r;

    r.type = TYPE_STR;
    if (lower > upper)
	r.v.str = str_dup("");
    else {
	int loop, index = 0;
        int lower_ind = skip_utf(str.v.str, lower - 1);
        int upper_ind = lower_ind + skip_utf(str.v.str + lower_ind, upper + 1 - lower);
	char *s = mymalloc(upper_ind - lower_ind + 1, M_STRING);

	for (loop = lower_ind; loop < upper_ind; loop++)
	    s[index++] = str.v.str[loop];
	s[index] = '\0';
	r.v.str = s;
    }
    free_var(str);
    return r;
}

Var
strget(Var str, Var i)
{
    Var r;
    char *s;
    int ind = skip_utf(str.v.str, i.v.num - 1);
    int n = clearance_utf(str.v.str[ind]);

    r.type = TYPE_STR;
    s = mymalloc(n + 1, M_STRING);
    strncpy(s, str.v.str + ind, n);
    s[n] = 0;
    r.v.str = s;
    return r;
}

/**** built in functions ****/

static package
bf_length(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    switch (arglist.v.list[1].type) {
    case TYPE_LIST:
	r.type = TYPE_INT;
	r.v.num = arglist.v.list[1].v.list[0].v.num;
	break;
    case TYPE_STR:
	r.type = TYPE_INT;
	r.v.num = strlen_utf(arglist.v.list[1].v.str);
	break;
    default:
	free_var(arglist);
	return make_error_pack(E_TYPE);
	break;
    }

    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_setadd(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;

    r = setadd(var_ref(arglist.v.list[1]), var_ref(arglist.v.list[2]));
    free_var(arglist);
    return make_var_pack(r);
}


static package
bf_setremove(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;

    r = setremove(var_ref(arglist.v.list[1]), arglist.v.list[2]);
    free_var(arglist);
    return make_var_pack(r);
}


static package
bf_listappend(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    if (arglist.v.list[0].v.num == 2)
	r = listappend(var_ref(arglist.v.list[1]), var_ref(arglist.v.list[2]));
    else
	r = listinsert(var_ref(arglist.v.list[1]), var_ref(arglist.v.list[2]),
		       arglist.v.list[3].v.num + 1);
    free_var(arglist);
    return make_var_pack(r);
}


static package
bf_listinsert(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    if (arglist.v.list[0].v.num == 2)
	r = listinsert(var_ref(arglist.v.list[1]), var_ref(arglist.v.list[2]), 1);
    else
	r = listinsert(var_ref(arglist.v.list[1]),
		    var_ref(arglist.v.list[2]), arglist.v.list[3].v.num);
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_listdelete(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    if (arglist.v.list[2].v.num <= 0
	|| arglist.v.list[2].v.num > arglist.v.list[1].v.list[0].v.num) {
	free_var(arglist);
	return make_error_pack(E_RANGE);
    } else {
	r = listdelete(var_ref(arglist.v.list[1]), arglist.v.list[2].v.num);
    }
    free_var(arglist);
    return make_var_pack(r);
}


static package
bf_listset(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    if (arglist.v.list[3].v.num <= 0
	|| arglist.v.list[3].v.num > arglist.v.list[1].v.list[0].v.num) {
	free_var(arglist);
	return make_error_pack(E_RANGE);
    } else {
	r = listset(var_dup(arglist.v.list[1]),
		    var_ref(arglist.v.list[2]), arglist.v.list[3].v.num);
    }
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_equal(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;

    r.type = TYPE_INT;
    r.v.num = equality(arglist.v.list[1], arglist.v.list[2], 1);
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_is_member(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;

    r.type = TYPE_INT;
    r.v.num = ismember(arglist.v.list[1], arglist.v.list[2], 1);
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_strsub(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (source, what, with [, case-matters]) */
    Var r;
    int case_matters = 0;

    if (arglist.v.list[0].v.num == 4)
	case_matters = is_true(arglist.v.list[4]);
    if (arglist.v.list[2].v.str[0] == '\0') {
	free_var(arglist);
	return make_error_pack(E_INVARG);
    } else {
	r.type = TYPE_STR;
	r.v.str = str_dup(strsub(arglist.v.list[1].v.str,
				 arglist.v.list[2].v.str,
				 arglist.v.list[3].v.str, case_matters));

	free_var(arglist);
	return make_var_pack(r);
    }
}

static package
bf_crypt(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (string, [salt]) */
    Var r;

#if HAVE_CRYPT
    char salt[3];
    const char *saltp;
    static char saltstuff[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";
    extern const char *crypt(const char *, const char *);

    if (arglist.v.list[0].v.num == 1 || memo_strlen(arglist.v.list[2].v.str) < 2) {
	/* provide a random 2-letter salt, works with old and new crypts */
	salt[0] = saltstuff[RANDOM() % (int) strlen(saltstuff)];
	salt[1] = saltstuff[RANDOM() % (int) strlen(saltstuff)];
	salt[2] = '\0';
	saltp = salt;
    } else {
	/* return the entire crypted password in the salt, this works
	 * for all crypt versions */
	saltp = arglist.v.list[2].v.str;
    }
    r.type = TYPE_STR;
    r.v.str = str_dup(crypt(arglist.v.list[1].v.str, saltp));
#else				/* !HAVE_CRYPT */
    r.type = TYPE_STR;
    r.v.str = str_ref(arglist.v.list[1].v.str);
#endif

    free_var(arglist);
    return make_var_pack(r);
}

static int
signum(int x)
{
    return x < 0 ? -1 : (x > 0 ? 1 : 0);
}

static package
bf_strcmp(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (string1, string2) */
    Var r;

    r.type = TYPE_INT;
    r.v.num = signum(strcmp(arglist.v.list[1].v.str, arglist.v.list[2].v.str));
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_index(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (source, what [, case-matters]) */
    Var r;
    int case_matters = 0;

    if (arglist.v.list[0].v.num == 3)
	case_matters = is_true(arglist.v.list[3]);
    r.type = TYPE_INT;
    r.v.num = strindex(arglist.v.list[1].v.str, arglist.v.list[2].v.str,
		       case_matters);

    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_rindex(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (source, what [, case-matters]) */
    Var r;

    int case_matters = 0;

    if (arglist.v.list[0].v.num == 3)
	case_matters = is_true(arglist.v.list[3]);
    r.type = TYPE_INT;
    r.v.num = strrindex(arglist.v.list[1].v.str, arglist.v.list[2].v.str,
			case_matters);

    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_tostr(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    r.type = TYPE_STR;
    r.v.str = str_dup(list2str(arglist.v.list));
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_toliteral(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;

    r.type = TYPE_STR;
    r.v.str = str_dup(value_to_literal(arglist.v.list[1]));
    free_var(arglist);
    return make_var_pack(r);
}

struct pat_cache_entry {
    char *string;
    int case_matters;
    Pattern pattern;
    struct pat_cache_entry *next;
};

static struct pat_cache_entry *pat_cache;
static struct pat_cache_entry pat_cache_entries[PATTERN_CACHE_SIZE];

static void
setup_pattern_cache()
{
    int i;

    for (i = 0; i < PATTERN_CACHE_SIZE; i++) {
	pat_cache_entries[i].string = 0;
	pat_cache_entries[i].pattern.ptr = 0;
    }

    for (i = 0; i < PATTERN_CACHE_SIZE - 1; i++)
	pat_cache_entries[i].next = &(pat_cache_entries[i + 1]);
    pat_cache_entries[PATTERN_CACHE_SIZE - 1].next = 0;

    pat_cache = &(pat_cache_entries[0]);
}

static Pattern
get_pattern(const char *string, int case_matters)
{
    struct pat_cache_entry *entry, **entry_ptr;

    entry = pat_cache;
    entry_ptr = &pat_cache;

    while (1) {
	if (entry->string && !strcmp(string, entry->string)
	    && case_matters == entry->case_matters) {
	    /* A cache hit; move this entry to the front of the cache. */
	    break;
	} else if (!entry->next) {
	    /* A cache miss; this is the last entry in the cache, so reuse that
	     * one for this pattern, moving it to the front of the cache iff
	     * the compilation succeeds.
	     */
	    if (entry->string) {
		free_str(entry->string);
		free_pattern(entry->pattern);
	    }
	    entry->pattern = new_pattern(string, case_matters);
	    entry->case_matters = case_matters;
	    if (!entry->pattern.ptr)
		entry->string = 0;
	    else
		entry->string = str_dup(string);
	    break;
	} else {
	    /* not done searching the cache... */
	    entry_ptr = &(entry->next);
	    entry = entry->next;
	}
    }

    *entry_ptr = entry->next;
    entry->next = pat_cache;
    pat_cache = entry;
    return entry->pattern;
}

#define match_rebase(x) (x == 0 ? 0 : (subject_len - strlen_utf(subject + (x) - 1) + 1))
Var
do_match(Var arglist, int reverse)
{
    const char *subject, *pattern;
    int i;
    Pattern pat;
    Var ans;
    Match_Indices regs[10];
    int subject_len;

    subject = arglist.v.list[1].v.str;
    pattern = arglist.v.list[2].v.str;
    pat = get_pattern(pattern, (arglist.v.list[0].v.num == 3
				&& is_true(arglist.v.list[3])));

    if (!pat.ptr) {
	ans.type = TYPE_ERR;
	ans.v.err = E_INVARG;
    } else
	switch (match_pattern(pat, subject, regs, reverse)) {
	case MATCH_SUCCEEDED:
            subject_len = strlen_utf(subject);
	    ans = new_list(4);
	    ans.v.list[1].type = TYPE_INT;
	    ans.v.list[2].type = TYPE_INT;
	    ans.v.list[4].type = TYPE_STR;
	    ans.v.list[1].v.num = match_rebase(regs[0].start);
	    ans.v.list[2].v.num = match_rebase(regs[0].end + 1) - 1;
	    ans.v.list[3] = new_list(9);
	    ans.v.list[4].v.str = str_ref(subject);
	    for (i = 1; i <= 9; i++) {
		ans.v.list[3].v.list[i] = new_list(2);
		ans.v.list[3].v.list[i].v.list[1].type = TYPE_INT;
		ans.v.list[3].v.list[i].v.list[1].v.num = match_rebase(regs[i].start);
		ans.v.list[3].v.list[i].v.list[2].type = TYPE_INT;
		ans.v.list[3].v.list[i].v.list[2].v.num = match_rebase(regs[i].end + 1) - 1;
	    }
	    break;
	case MATCH_FAILED:
	    ans = new_list(0);
	    break;
	case MATCH_ABORTED:
	    ans.type = TYPE_ERR;
	    ans.v.err = E_QUOTA;
	    break;
	}

    return ans;
}

static package
bf_match(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var ans;

    ans = do_match(arglist, 0);
    free_var(arglist);
    if (ans.type == TYPE_ERR)
	return make_error_pack(ans.v.err);
    else
	return make_var_pack(ans);
}

static package
bf_rmatch(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var ans;

    ans = do_match(arglist, 1);
    free_var(arglist);
    if (ans.type == TYPE_ERR)
	return make_error_pack(ans.v.err);
    else
	return make_var_pack(ans);
}

int
invalid_pair(int num1, int num2, int max)
{
    if ((num1 == 0 && num2 == -1)
	|| (num1 > 0 && num2 >= num1 - 1 && num2 <= max))
	return 0;
    else
	return 1;
}

int
check_subs_list(Var subs)
{
    const char *subj;
    int subj_length, loop;

    if (subs.type != TYPE_LIST || subs.v.list[0].v.num != 4
	|| subs.v.list[1].type != TYPE_INT
	|| subs.v.list[2].type != TYPE_INT
	|| subs.v.list[3].type != TYPE_LIST
	|| subs.v.list[3].v.list[0].v.num != 9
	|| subs.v.list[4].type != TYPE_STR)
	return 1;
    subj = subs.v.list[4].v.str;
    subj_length = strlen_utf(subj);
    if (invalid_pair(subs.v.list[1].v.num, subs.v.list[2].v.num,
		     subj_length))
	return 1;

    for (loop = 1; loop <= 9; loop++) {
	Var pair;
	pair = subs.v.list[3].v.list[loop];
	if (pair.type != TYPE_LIST
	    || pair.v.list[0].v.num != 2
	    || pair.v.list[1].type != TYPE_INT
	    || pair.v.list[2].type != TYPE_INT
	    || invalid_pair(pair.v.list[1].v.num, pair.v.list[2].v.num,
			    subj_length))
	    return 1;
    }
    return 0;
}

static package
bf_substitute(Var arglist, Byte next, void *vdata, Objid progr)
{
    int template_length, subject_length;
    const char *template, *subject;
    Var subs, ans;
    int invarg = 0;
    Stream *s;
    char c = '\0';

    template = arglist.v.list[1].v.str;
    template_length = memo_strlen(template);
    subs = arglist.v.list[2];

    if (check_subs_list(subs)) {
	free_var(arglist);
	return make_error_pack(E_INVARG);
    }
    subject = subs.v.list[4].v.str;
    subject_length = memo_strlen(subject);

    s = new_stream(template_length);
    ans.type = TYPE_STR;
    while ((c = *(template++)) != '\0') {
	switch (c) {
	case '%':
	    {
		Var pair;
		int start = 0, end = 0;
		c = *(template++);
		if (c == '%')
		    stream_add_char(s, '%');
		else {
		    if (c >= '1' && c <= '9') {
			pair = subs.v.list[3].v.list[c - '0'];
			start = pair.v.list[1].v.num - 1;
			end = pair.v.list[2].v.num - 1;
		    } else if (c == '0') {
			start = subs.v.list[1].v.num - 1;
			end = subs.v.list[2].v.num - 1;
		    } else
			invarg = 1;
		    if (!invarg) {
			int where = skip_utf(subject, start);

                        end = where + skip_utf(subject + where, end - start + 1);
			for (; where < end; where++)
			    stream_add_char(s, subject[where]);
		    }
		}
		break;
	    }
	default:
	    stream_add_char(s, c);
	}
	if (invarg)
	    break;
    }

    free_var(arglist);
    if (!invarg)
	ans.v.str = str_dup(reset_stream(s));
    free_stream(s);
    if (invarg)
	return make_error_pack(E_INVARG);
    else
	return make_var_pack(ans);
}

static package
bf_value_bytes(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;

    r.type = TYPE_INT;
    r.v.num = value_bytes(arglist.v.list[1]);
    free_var(arglist);
    return make_var_pack(r);
}

static const char *
hash_bytes(const char *input, int length)
{
    md5ctx_t context;
    uint8_t result[16];
    int i;
    const char digits[] = "0123456789ABCDEF";
    char *hex = str_dup("12345678901234567890123456789012");
    const char *answer = hex;

    md5_Init(&context);
    md5_Update(&context, (uint8_t *) input, length);
    md5_Final(&context, result);
    for (i = 0; i < 16; i++) {
	*hex++ = digits[result[i] >> 4];
	*hex++ = digits[result[i] & 0xF];
    }
    return answer;
}

static package
bf_binary_hash(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    int length;
    const char *bytes = binary_to_raw_bytes(arglist.v.list[1].v.str, &length);

    free_var(arglist);
    if (!bytes)
	return make_error_pack(E_INVARG);
    r.type = TYPE_STR;
    r.v.str = hash_bytes(bytes, length);
    return make_var_pack(r);
}

static package
bf_string_hash(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    const char *str = arglist.v.list[1].v.str;

    r.type = TYPE_STR;
    r.v.str = hash_bytes(str, memo_strlen(str));
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_value_hash(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    const char *lit = value_to_literal(arglist.v.list[1]);

    r.type = TYPE_STR;
    r.v.str = hash_bytes(lit, memo_strlen(lit));
    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_decode_binary(Var arglist, Byte next, void *vdata, Objid progr)
{
    int length;
    const char *bytes = binary_to_raw_bytes(arglist.v.list[1].v.str, &length);
    int nargs = arglist.v.list[0].v.num;
    int fully = (nargs >= 2 && is_true(arglist.v.list[2]));
    Var r;
    int i;

    free_var(arglist);
    if (!bytes)
	return make_error_pack(E_INVARG);

    if (fully) {
	r = new_list(length);
	for (i = 1; i <= length; i++) {
	    r.v.list[i].type = TYPE_INT;
	    r.v.list[i].v.num = (unsigned char) bytes[i - 1];
	}
    } else {
	static Stream *s = 0;
	int count, in_string;

	if (!s)
	    s = new_stream(50);

	for (count = in_string = 0, i = 0; i < length; i++) {
	    unsigned char c = bytes[i];

	    if (isgraph(c) || c == ' ' || c == '\t') {
		if (!in_string)
		    count++;
		in_string = 1;
	    } else {
		count++;
		in_string = 0;
	    }
	}

	r = new_list(count);
	for (count = 1, in_string = 0, i = 0; i < length; i++) {
	    unsigned char c = bytes[i];

	    if (isgraph(c) || c == ' ' || c == '\t') {
		stream_add_char(s, c);
		in_string = 1;
	    } else {
		if (in_string) {
		    r.v.list[count].type = TYPE_STR;
		    r.v.list[count].v.str = str_dup(reset_stream(s));
		    count++;
		}
		r.v.list[count].type = TYPE_INT;
		r.v.list[count].v.num = c;
		count++;
		in_string = 0;
	    }
	}

	if (in_string) {
	    r.v.list[count].type = TYPE_STR;
	    r.v.list[count].v.str = str_dup(reset_stream(s));
	}
    }

    return make_var_pack(r);
}

static int
encode_binary(Stream * s, Var v)
{
    int i;

    switch (v.type) {
    case TYPE_INT:
	if (v.v.num < 0 || v.v.num >= 256)
	    return 0;
	stream_add_char(s, (char) v.v.num);
	break;
    case TYPE_STR:
	stream_add_string(s, v.v.str);
	break;
    case TYPE_LIST:
	for (i = 1; i <= v.v.list[0].v.num; i++)
	    if (!encode_binary(s, v.v.list[i]))
		return 0;
	break;
    default:
	return 0;
    }

    return 1;
}

static package
bf_encode_binary(Var arglist, Byte next, void *vdata, Objid progr)
{
    static Stream *s = 0;
    int ok, length;
    Var r;
    const char *bytes;

    if (!s)
	s = new_stream(100);

    ok = encode_binary(s, arglist);
    free_var(arglist);
    length = stream_length(s);
    bytes = reset_stream(s);
    if (ok) {
	r.type = TYPE_STR;
	r.v.str = str_dup(raw_bytes_to_binary(bytes, length));
	return make_var_pack(r);
    } else
	return make_error_pack(E_INVARG);
}

static package
bf_tochar(Var arglist, Byte next, void *vdata, Objid progr)
{
    static Stream *s = 0;
    int ucs = 0;
    Var ans;
    Var v = arglist.v.list[1];
    const char *bytes;
    const struct unicode_character_data *ucd = 0;

    if (!s)
	s = new_stream(5);
    switch (v.type) {
    case TYPE_INT:
	if (v.v.num > 0 && v.v.num <= 0x10ffff)
	    ucs = v.v.num;
	break;
    case TYPE_STR:
        ucd = unicode_character_lookup(v.v.str);
        if (ucd) {
            ucs = ucd->ucs;
            unicode_character_put(ucd);
        }
	break;
    default:
        free_var(arglist);
        return make_error_pack(E_TYPE);
    }
    free_var(arglist);
    if (!my_is_printable(ucs))
        ucs = 0;
    if (ucs) {
        stream_add_utf(s, ucs);
        ans.type = TYPE_STR;
	ans.v.str = str_dup(reset_stream(s));
    }
    if (!ucs)
	return make_error_pack(E_INVARG);
    else
	return make_var_pack(ans);
}

static package
bf_charname(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r = arglist.v.list[1];
    const struct unicode_character_data *ucd = 0;
    const char *s = r.v.str;
    int ucs = get_utf(&s);
    Var ans;

    if ((ucs == 0) || (*s != 0)) {
        free_var(arglist);
        return make_error_pack(E_INVARG);
    }
    free_var(arglist);
    ucd = unicode_character_data(ucs);
    if (!ucd)
        return make_error_pack(E_INVARG);
    ans.type = TYPE_STR;
    ans.v.str = str_dup(ucd->name);
    unicode_character_put(ucd);
    return make_var_pack(ans);
}

static package
bf_ord(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r = arglist.v.list[1];
    const char *s = r.v.str;
    int ucs = get_utf(&s);
    Var ans;

    if ((ucs == 0) || (*s != 0)) {
        free_var(arglist);
        return make_error_pack(E_INVARG);
    }
    free_var(arglist);
    ans.type = TYPE_INT;
    ans.v.num = ucs;
    return make_var_pack(ans);
}

static int
encode_chars(Stream *s, Var v)
{
    int i;

    switch (v.type) {
    case TYPE_INT:
	if (stream_add_utf(s, v.v.num) == -1)
	    return 0;
	break;

    case TYPE_STR:
	stream_add_string(s, v.v.str);
	break;

    case TYPE_LIST:
	for (i = 1; i <= v.v.list[0].v.num; ++i) {
	    if (!encode_chars(s, v.v.list[i]))
		return 0;
	}
	break;

    default:
	return 0;
    }

    return 1;
}

static package
bf_encode_chars(Var arglist, Byte next, void *vdata, Objid progr)
{
    static Stream *s = 0;
    int ok, length;
    const char *bytes;

    if (!s)
	s = new_stream(100);

    ok = encode_chars(s, arglist.v.list[1]);

    length = stream_length(s);
    bytes = reset_stream(s);

    if (ok) {
	bytes = recode_chars(bytes, &length, "UTF-8", arglist.v.list[2].v.str);
	if (!bytes)
	    ok = 0;
    }

    free_var(arglist);

    if (ok) {
	Var r;

	r.type = TYPE_STR;
	r.v.str = str_dup(raw_bytes_to_binary(bytes, length));
	return make_var_pack(r);
    } else
	return make_error_pack(E_INVARG);
}

static package
bf_decode_chars(Var arglist, Byte next, void *vdata, Objid progr)
{
    const char *binary = arglist.v.list[1].v.str;
    int nargs = arglist.v.list[0].v.num;
    int fully = (nargs >= 3 && is_true(arglist.v.list[3]));
    const char *src, *dst;
    int length, ok = 0;
    Var r;

    src = binary_to_raw_bytes(binary, &length);
    if (src) {
	dst = recode_chars(src, &length, arglist.v.list[2].v.str, "UTF-32");
	if (dst) {
	    uint32_t *chars = (uint32_t *) dst;

	    length /= sizeof(uint32_t);

	    if (length && *chars == 0xFEFF /* BOM */)
		++chars, --length;

	    if (fully) {
		int i;

		r = new_list(length);

		for (i = 1; i <= length; ++i) {
		    r.v.list[i].type = TYPE_INT;
		    r.v.list[i].v.num = *chars++;
		}
	    }
	    else {
		Stream *s;
		Var elt;

		r = new_list(0);
		s = new_stream(length + length / 2);

		while (length--) {
		    int c = *chars++;

		    if (my_is_printable(c))
			stream_add_utf(s, c);
		    else {
			if (stream_length(s)) {
			    elt.type = TYPE_STR;
			    elt.v.str = str_dup(reset_stream(s));
			    r = listappend(r, elt);
			}

			elt.type = TYPE_INT;
			elt.v.num = c;
			r = listappend(r, elt);
		    }
		}

		if (stream_length(s)) {
		    elt.type = TYPE_STR;
		    elt.v.str = str_dup(reset_stream(s));
		    r = listappend(r, elt);
		}

		free_stream(s);
	    }

	    ok = 1;
	}
    }

    free_var(arglist);

    return ok ? make_var_pack(r) : make_error_pack(E_INVARG);
}

void
register_list(void)
{
    register_function("value_bytes", 1, 1, bf_value_bytes, TYPE_ANY);
    register_function("value_hash", 1, 1, bf_value_hash, TYPE_ANY);
    register_function("string_hash", 1, 1, bf_string_hash, TYPE_STR);
    register_function("binary_hash", 1, 1, bf_binary_hash, TYPE_STR);
    register_function("decode_binary", 1, 2, bf_decode_binary,
		      TYPE_STR, TYPE_ANY);
    register_function("encode_binary", 0, -1, bf_encode_binary);
    /* list */
    register_function("length", 1, 1, bf_length, TYPE_ANY);
    register_function("setadd", 2, 2, bf_setadd, TYPE_LIST, TYPE_ANY);
    register_function("setremove", 2, 2, bf_setremove, TYPE_LIST, TYPE_ANY);
    register_function("listappend", 2, 3, bf_listappend,
		      TYPE_LIST, TYPE_ANY, TYPE_INT);
    register_function("listinsert", 2, 3, bf_listinsert,
		      TYPE_LIST, TYPE_ANY, TYPE_INT);
    register_function("listdelete", 2, 2, bf_listdelete, TYPE_LIST, TYPE_INT);
    register_function("listset", 3, 3, bf_listset,
		      TYPE_LIST, TYPE_ANY, TYPE_INT);
    register_function("equal", 2, 2, bf_equal, TYPE_ANY, TYPE_ANY);
    register_function("is_member", 2, 2, bf_is_member, TYPE_ANY, TYPE_LIST);

    /* string */
    register_function("tostr", 0, -1, bf_tostr);
    register_function("toliteral", 1, 1, bf_toliteral, TYPE_ANY);
    setup_pattern_cache();
    register_function("match", 2, 3, bf_match, TYPE_STR, TYPE_STR, TYPE_ANY);
    register_function("rmatch", 2, 3, bf_rmatch, TYPE_STR, TYPE_STR, TYPE_ANY);
    register_function("substitute", 2, 2, bf_substitute, TYPE_STR, TYPE_LIST);
    register_function("crypt", 1, 2, bf_crypt, TYPE_STR, TYPE_STR);
    register_function("index", 2, 3, bf_index, TYPE_STR, TYPE_STR, TYPE_ANY);
    register_function("rindex", 2, 3, bf_rindex, TYPE_STR, TYPE_STR, TYPE_ANY);
    register_function("strcmp", 2, 2, bf_strcmp, TYPE_STR, TYPE_STR);
    register_function("strsub", 3, 4, bf_strsub,
		      TYPE_STR, TYPE_STR, TYPE_STR, TYPE_ANY);
    register_function("tochar", 1, 1, bf_tochar, TYPE_ANY);
    register_function("charname", 1, 1, bf_charname, TYPE_STR);
    register_function("ord", 1, 1, bf_ord, TYPE_STR);
    register_function("encode_chars", 2, 2, bf_encode_chars,
		      TYPE_ANY, TYPE_STR);
    register_function("decode_chars", 2, 3, bf_decode_chars,
		      TYPE_STR, TYPE_STR, TYPE_ANY);
}


char rcsid_list[] = "$Id$";

/*
 * $Log$
 * Revision 1.12  2010/04/23 05:01:20  wrog
 * Implement max_list_concat for list_insert,list_append,set_add
 *
 * Revision 1.11  2010/04/22 22:00:35  wrog
 * Fuller explanation of TRY_STREAM, unscramble bf_substitute,
 * stream usage cleanups in bf_tostr, bf_toliteral, bf_value_hash, bf_encode_binary
 *
 * Revision 1.10  2010/03/31 18:08:07  wrog
 * builtin functions can now explicitly abort task with out-of-seconds/ticks
 * using make_abort_pack()/BI_KILL rather than by setting task_timed_out
 *
 * Revision 1.9  2010/03/30 23:13:39  wrog
 * rewrote bf_strsub(), bf_tostr(), bf_toliteral(),
 * -  bf_substitute(), bf_value_hash(), bf_encode_binary()
 * -  to enable and watch for stream_too_big exceptions;
 * bf_decode_binary() now raises errors
 * -  instead of creating overly large lists
 * value2str() made safe (never allocates);
 * list2str cannibalized into stream_add_tostr();
 * print_to_stream() renamed to unparse_value(),
 * -  made public to replace value_to_literal() [removed],
 * -  now panicks on unknown Var type rather than logging;
 *
 * Revision 1.8  2010/03/26 23:28:29  wrog
 * Fix compiler warning about unassigned variable
 *
 * Revision 1.7  2006/09/07 00:55:02  bjj
 * Add new MEMO_STRLEN option which uses the refcounting mechanism to
 * store strlen with strings.  This is basically free, since most string
 * allocations are rounded up by malloc anyway.  This saves lots of cycles
 * computing strlen.  (The change is originally from jitmoo, where I wanted
 * inline range checks for string ops).
 *
 * Revision 1.6  2001/03/12 00:16:29  bjj
 * bf_crypt now passes the entire second argument as the salt to
 * the C crypt() routine.  This works fine for traditional DES crypts
 * and supports modern modular crypts like FreeBSD's.  This just makes
 * it possible to pass the entire salt:  the core still has to do it.
 *
 * Revision 1.5  1998/12/14 13:17:57  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.4  1997/07/07 03:24:54  nop
 * Merge UNSAFE_OPTS (r5) after extensive testing.
 *
 * Revision 1.3.2.3  1997/07/03 08:04:01  bjj
 * Pattern cache was not storing case_matters flag, causing many patterns to
 * be impossible to find in the cache.
 *
 * Revision 1.3.2.2  1997/05/20 14:55:52  nop
 * Include Jason's patch for bf_decode_binary losing on systems where
 * char is signed.
 *
 * Revision 1.3.2.1  1997/03/21 15:22:56  bjj
 * doinsert reallocs for appending to refcnt 1 lists.  note that this wins
 * because it avoids all the var_ref/free_var that's done in the general case,
 * not because it avoids malloc/free.  the general case could also benefit from
 * using memcpy when the refcnt is 1, rather than looping with var_ref.
 *
 * Revision 1.3  1997/03/03 06:20:04  bjj
 * new_list(0) now returns the same empty list to every caller
 *
 * Revision 1.2  1997/03/03 04:18:46  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:45:00  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.7  1996/03/11  23:35:17  pavel
 * Fixed bad use of possibly-signed characters in decode_binary().
 * Release 1.8.0p1.
 *
 * Revision 2.6  1996/02/18  23:17:24  pavel
 * Added value_hash(), string_hash(), and binary_hash().  Release 1.8.0beta3.
 *
 * Revision 2.5  1996/02/08  07:02:09  pavel
 * Added support for floating-point numbers.  Fixed registration of
 * decode_binary().  Renamed err/logf() to errlog/oklog() and TYPE_NUM to
 * TYPE_INT.  Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.4  1996/01/16  07:26:56  pavel
 * Fixed `case_matters' arguments to strsub(), index(), rindex(), match(), and
 * rmatch() to allow values of any type.  Release 1.8.0alpha6.
 *
 * Revision 2.3  1996/01/11  07:42:14  pavel
 * Added support for C's crypt() function being unavailable.  Fixed potential
 * string overread in case of a too-short salt argument to MOO's crypt()
 * function.  Added built-ins encode_binary() and decode_binary(), in support
 * of new binary I/O facilities.  Release 1.8.0alpha5.
 *
 * Revision 2.2  1995/12/31  03:25:04  pavel
 * Added missing #include "options.h".  Release 1.8.0alpha4.
 *
 * Revision 2.1  1995/12/11  07:43:20  pavel
 * Moved value_bytes built-in function to here.
 *
 * Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  04:24:03  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.12  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.11  1992/10/23  19:27:07  pavel
 * Removed a place where a local structure variable was initialized in its
 * declaration, since some compilers can't hack that.
 * Added the `%%' -> `%' transformation to substitute().
 *
 * Revision 1.10  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.9  1992/10/17  20:33:17  pavel
 * Global rename of strdup->str_dup, strref->str_ref, vardup->var_dup, and
 * varref->var_ref.
 * Added some (int) casts to placate over-protective compilers.
 *
 * Revision 1.8  1992/09/08  22:15:09  pjames
 * Updated strrangeset() and listrangeset() to use correct algorithm.
 *
 * Revision 1.7  1992/09/08  22:03:45  pjames
 * Added all code from bf_str_list.c, and some code from bf_type.c.
 *
 * Revision 1.6  1992/08/31  22:31:47  pjames
 * Changed some `char *'s to `const char *' and fixed code accordingly.
 *
 * Revision 1.5  1992/08/28  23:20:22  pjames
 * Added `listrangeset()' and `strrangeset()'.
 *
 * Revision 1.4  1992/08/28  16:29:10  pjames
 * Added some varref()'s.
 * Removed some free_var()'s, due to ref-counting.
 * Changed some my_free()'s to free_var().
 * Made `substr()' preserve the string it is passed.
 *
 * Revision 1.3  1992/08/10  16:53:46  pjames
 * Updated #includes.
 *
 * Revision 1.2  1992/07/21  00:04:04  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS ident. string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
