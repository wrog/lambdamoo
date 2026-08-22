/*
 * PCRE glue for LambdaMOO
 * Copyright (c) 2008 Robert Leslie
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies.  This software is provided
 * "as is" without express or implied warranty.
 */

#include "pattern.h"

#include "config.h"
#include "options.h"

#if HAVE_PCRE2
#  define PCRE2_CODE_UNIT_WIDTH 8
#  include <pcre2.h>
#else
#  include <pcre.h>
#endif

#include "my-stdio.h"
#include "my-string.h"

#include "streams.h"
#include "utf.h"
#include "storage.h"
#include "exceptions.h"


#define DEBUG       0
#define UTF8_CHECK  0

#define MATCH_LIMIT            100000
#define MATCH_LIMIT_RECURSION    5000

typedef struct {
#if HAVE_PCRE2
    pcre2_code *code;
    pcre2_match_context *match_context;
#else
    pcre *code;
    pcre_extra *extra;
#endif
} regexp_t;

typedef struct {
#if HAVE_PCRE2
    PCRE2_SIZE ovec[MATCH_GROUP_LIMIT * 2];
#else
    int ovec[MATCH_GROUP_LIMIT * 2];
#endif
    int valid;
} rmatch_data_t;

static
const char *translate(const char *moopat)
{
    static Stream *s = 0;
    int c;
    enum { st_base, st_esc,
	   st_cset_init, st_cset_init2, st_cset } state = st_base;

    if (!s)
	s = new_stream(100);

    /*
     * Translate MOO regular expression syntax into PCRE syntax
     *
     * Aside from changing % to \ and sundry tweaks we also address an
     * incompatibility between MOO %b, %B, %w, %W and PCRE \b, \B, \w, \W --
     * namely, the inclusion of _ in \w and its absence in %w.
     */

    /* wrap entire expression so we can add a callout at the end */
    stream_add_string(s, "(?:");

    while ((c = get_utf(&moopat))) {
	switch (state) {
	case st_base:
	    switch (c) {
	    case '\\':
	    case '|':
	    case '(':
	    case ')':
	    case '{':
		stream_add_char(s, '\\');
		/* FALLS THROUGH */
	    case '.':
	    case '*':
	    case '+':
	    case '?':
	    case '^':
	    case '$':
	    default:
		stream_add_utf(s, c);
		break;

	    case '[':
		stream_add_char(s, c);
		state = st_cset_init;
		break;

	    case '%':
		state = st_esc;
		break;
	    }
	    break;

	case st_cset_init:
	    switch (c) {
	    case '\\':
	    case '[':
		stream_add_char(s, '\\');
		/* FALLS THROUGH */
	    case '-':
	    case ']':
	    default:
		stream_add_utf(s, c);
		state = st_cset;
		break;

	    case '^':
		stream_add_char(s, c);
		state = st_cset_init2;
		break;
	    }
	    break;

	case st_cset_init2:
	    switch (c) {
	    case '\\':
	    case '[':
		stream_add_char(s, '\\');
		/* FALLS THROUGH */
	    case '^':
	    case '-':
	    case ']':
	    default:
		stream_add_utf(s, c);
		state = st_cset;
		break;
	    }
	    break;

	case st_cset:
	    switch (c) {
	    case '\\':
	    case '[':
		stream_add_char(s, '\\');
		/* FALLS THROUGH */
	    case '^':
	    case '-':
	    default:
		stream_add_utf(s, c);
		break;

	    case ']':
		stream_add_char(s, c);
		state = st_base;
		break;
	    }
	    break;

	case st_esc:
	    switch (c) {
	    case '\\':
	    case '^':
	    case '$':
	    case '.':
	    case '[':
	    case '?':
	    case '*':
	    case '+':
	    case '{':
		stream_add_char(s, '\\');
		/* FALLS THROUGH */
	    case '|':
	    case ')':
	    default:
		stream_add_utf(s, c);
		break;

	    case '(':
		stream_add_char(s, c);
		/* insert a null-op (comment) to prevent special sequences */
		stream_add_string(s, "(?#)");
		break;

	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		stream_printf(s, "\\%d(?#)", c - '0');
		break;

#define P_WORD          "[^\\W_]"
#define P_NONWORD       "[\\W_]"

#define P_ALT(a, b)     "(?:"a"|"b")"
#define P_LBEHIND(p)    "(?<="p")"
#define P_LAHEAD(p)     "(?="p")"
#define P_LOOKBA(b, a)  P_LBEHIND(b) P_LAHEAD(a)

#define P_WORD_BEGIN    P_ALT("^", P_LBEHIND(P_NONWORD)) P_LAHEAD(P_WORD)
#define P_WORD_END      P_LBEHIND(P_WORD) P_ALT("$", P_LAHEAD(P_NONWORD))

	    case 'b':
		stream_add_string(s, P_ALT(P_WORD_BEGIN, P_WORD_END));
		break;

	    case 'B':
		stream_add_string(s, P_ALT(P_LOOKBA(P_WORD, P_WORD),
					   P_LOOKBA(P_NONWORD, P_NONWORD)));
		break;

	    case '<':
		stream_add_string(s, P_WORD_BEGIN);
		break;

	    case '>':
		stream_add_string(s, P_WORD_END);
		break;

	    case 'w':
		stream_add_string(s, P_WORD);
		break;

	    case 'W':
		stream_add_string(s, P_NONWORD);
		break;
	    }
	    state = st_base;
	    break;
	}
    }

    /* add callout at end of pattern for rmatch */
    stream_add_string(s, ")(?C)");

    /* don't let a trailing % get away without a syntax error */
    if (state == st_esc)
	stream_add_char(s, '\\');

    return reset_stream(s);
}

#if HAVE_PCRE2

Pattern new_pattern(const char *pattern, int case_matters)
{
    uint32_t options = 0;
    int error_code;
    PCRE2_SIZE error_offset;
    const char *translated;
    pcre2_code *code;
    regexp_t *regexp = 0;
    Pattern p;

    options |= PCRE2_UTF;
# if !UTF8_CHECK
    options |= PCRE2_NO_UTF_CHECK;
# endif

    /* allow PCRE to optimize .* at beginning of pattern by implicit anchor */
    options |= PCRE2_DOTALL;

    if (!case_matters)
	options |= PCRE2_CASELESS;

    translated = translate(pattern);
# if DEBUG
    fprintf(stderr, __FILE__ ": \"%s\" => /%s/\n", pattern, translated);
# endif

    code = pcre2_compile((PCRE2_SPTR) translated, PCRE2_ZERO_TERMINATED,
			 options, &error_code, &error_offset, 0);
# if DEBUG
    if (!code) {
	PCRE2_UCHAR error[256];

	pcre2_get_error_message(error_code, error, sizeof(error));
	fprintf(stderr, __FILE__ ": pcre2_compile() failed: %s\n", error);
	fprintf(stderr, __FILE__ ":   /%s/\n", translated);
	fprintf(stderr, __FILE__ ":    ");
	while (error_offset--)
	    fputc(' ', stderr);
	fprintf(stderr, "^\n");
    }
# endif

    if (code) {
	regexp = mymalloc(sizeof(*regexp), M_PATTERN);
	regexp->code = code;
	regexp->match_context = pcre2_match_context_create(0);
	if (!regexp->match_context)
	    panic("pcre2_match_context_create() failed");

	pcre2_set_match_limit(regexp->match_context, MATCH_LIMIT);
	pcre2_set_recursion_limit(regexp->match_context, MATCH_LIMIT_RECURSION);
    }

    p.ptr = regexp;

    return p;
}

static
int rmatch_callout(pcre2_callout_block *block, void *callout_data)
{
    rmatch_data_t *rmatch = callout_data;
    int capture_top = block->capture_top > MATCH_GROUP_LIMIT
	? MATCH_GROUP_LIMIT : block->capture_top;

    if (!rmatch->valid || block->current_position > rmatch->ovec[1] ||
	(block->current_position == rmatch->ovec[1] &&
	 block->start_match < rmatch->ovec[0])) {
	/* make a copy of the offsets vector so the last such vector found can
	   be returned as the rightmost match */

	rmatch->ovec[0] = block->start_match;
	rmatch->ovec[1] = block->current_position;
	if (capture_top > 1)
	    memcpy(&rmatch->ovec[2], &block->offset_vector[2],
		   sizeof(rmatch->ovec[2]) * 2 * (capture_top - 1));

	rmatch->valid = capture_top;
    }

    return 1;  /* cause match failure at current point, but continue trying */
}

Match_Result match_pattern(Pattern p, const char *string,
			   Match_Indices *indices, int is_reverse)
{
    regexp_t *regexp = p.ptr;
    pcre2_match_data *match_data = pcre2_match_data_create(MATCH_GROUP_LIMIT, 0);
    PCRE2_SIZE *ov;
    int rc;
    uint32_t options = 0;
    int i;
    rmatch_data_t rmatch;

    if (!match_data)
	panic("pcre2_match_data_create() failed");

    if (is_reverse) {
	rmatch.valid = 0;
	pcre2_set_callout(regexp->match_context, rmatch_callout, &rmatch);
    }
    else
	pcre2_set_callout(regexp->match_context, 0, 0);

# if !UTF8_CHECK
    options |= PCRE2_NO_UTF_CHECK;
# endif

    rc = pcre2_match(regexp->code, (PCRE2_SPTR) string, memo_strlen(string),
		     0, options, match_data, regexp->match_context);
    ov = pcre2_get_ovector_pointer(match_data);
    if (rc < 0) {
	switch (rc) {
	case PCRE2_ERROR_NOMATCH:
	    if (is_reverse && rmatch.valid) {
		ov = rmatch.ovec;
		rc = rmatch.valid;
		break;
	    }
	    pcre2_match_data_free(match_data);
	    return MATCH_FAILED;

	default:
# if DEBUG
	    fprintf(stderr, __FILE__ ": pcre2_match() failed: %d\n", rc);
# endif
	case PCRE2_ERROR_MATCHLIMIT:
	case PCRE2_ERROR_RECURSIONLIMIT:
	    pcre2_match_data_free(match_data);
	    return MATCH_ABORTED;
	}
    }

    if (rc == 0 || rc > MATCH_GROUP_LIMIT)
	rc = MATCH_GROUP_LIMIT;  /* there were more subpatterns than output vectors */

    for (i = 0; i < rc; ++i) {
	/* convert from 0-based open interval to 1-based closed one */
	if (ov[i * 2] == PCRE2_UNSET) {
	    indices[i].start =  0;
	    indices[i].end   = -1;
	}
	else {
	    indices[i].start = 1 + ov[i * 2 + 0];
	    indices[i].end   =     ov[i * 2 + 1];
	}
    }
    for (i = rc; i < MATCH_GROUP_LIMIT; ++i) {
	indices[i].start =  0;
	indices[i].end   = -1;
    }

    pcre2_match_data_free(match_data);
    return MATCH_SUCCEEDED;
}

void free_pattern(Pattern p)
{
    regexp_t *regexp = p.ptr;

    if (regexp) {
	pcre2_match_context_free(regexp->match_context);
	pcre2_code_free(regexp->code);

	myfree(regexp, M_PATTERN);
    }
}

#else /* PCRE1 */

Pattern new_pattern(const char *pattern, int case_matters)
{
    int options = PCRE_UTF8;
    const char *error;
    int error_offset;
    const char *translated;
    pcre *code;
    regexp_t *regexp = 0;
    Pattern p;

# if !UTF8_CHECK
    options |= PCRE_NO_UTF8_CHECK;
# endif
    options |= PCRE_DOTALL;
    if (!case_matters)
	options |= PCRE_CASELESS;

    translated = translate(pattern);
# if DEBUG
    fprintf(stderr, __FILE__ ": \"%s\" => /%s/\n", pattern, translated);
# endif
    code = pcre_compile(translated, options, &error, &error_offset, 0);
# if DEBUG
    if (!code) {
	fprintf(stderr, __FILE__ ": pcre_compile() failed: %s\n", error);
	fprintf(stderr, __FILE__ ":   /%s/\n", translated);
	fprintf(stderr, __FILE__ ":    ");
	while (error_offset--)
	    fputc(' ', stderr);
	fprintf(stderr, "^\n");
    }
# endif

    if (code) {
	pcre_extra *extra;

	regexp = mymalloc(sizeof(*regexp), M_PATTERN);
	regexp->code = code;
	extra = pcre_study(code, 0, &error);
# if DEBUG
	if (error)
	    fprintf(stderr, __FILE__ ": pcre_study() failed: %s\n", error);
# endif
	if (!extra) {
	    extra = pcre_malloc(sizeof(*extra));
	    if (!extra)
		panic("pcre_malloc() failed");
	    extra->flags = 0;
	}
	extra->match_limit = MATCH_LIMIT;
	extra->flags |= PCRE_EXTRA_MATCH_LIMIT;
	extra->match_limit_recursion = MATCH_LIMIT_RECURSION;
	extra->flags |= PCRE_EXTRA_MATCH_LIMIT_RECURSION;
	regexp->extra = extra;
    }

    p.ptr = regexp;
    return p;
}

static
int rmatch_callout(pcre_callout_block *block)
{
    rmatch_data_t *rmatch = block->callout_data;
    int capture_top = block->capture_top > MATCH_GROUP_LIMIT
	? MATCH_GROUP_LIMIT : block->capture_top;

    if (!rmatch->valid || block->current_position > rmatch->ovec[1] ||
	(block->current_position == rmatch->ovec[1] &&
	 block->start_match < rmatch->ovec[0])) {
	rmatch->ovec[0] = block->start_match;
	rmatch->ovec[1] = block->current_position;
	if (capture_top > 1)
	    memcpy(&rmatch->ovec[2], &block->offset_vector[2],
		   sizeof(rmatch->ovec[2]) * 2 * (capture_top - 1));
	rmatch->valid = capture_top;
    }

    return 1;
}

Match_Result match_pattern(Pattern p, const char *string,
			   Match_Indices *indices, int is_reverse)
{
    regexp_t *regexp = p.ptr;
    pcre_extra *extra = regexp->extra;
    int rc, options = 0;
    int ovec[MATCH_GROUP_LIMIT * 3];
    int i, *ov = ovec;
    rmatch_data_t rmatch;

    if (is_reverse) {
	rmatch.valid = 0;
	extra->callout_data = &rmatch;
	extra->flags |= PCRE_EXTRA_CALLOUT_DATA;
	pcre_callout = rmatch_callout;
    }
    else {
	extra->flags &= ~PCRE_EXTRA_CALLOUT_DATA;
	pcre_callout = 0;
    }

# if !UTF8_CHECK
    options |= PCRE_NO_UTF8_CHECK;
# endif
    rc = pcre_exec(regexp->code, extra, string, memo_strlen(string), 0,
		   options, ovec, sizeof(ovec) / sizeof(ovec[0]));
    if (rc < 0) {
	switch (rc) {
	case PCRE_ERROR_NOMATCH:
	    if (is_reverse && rmatch.valid) {
		ov = rmatch.ovec;
		rc = rmatch.valid;
		break;
	    }
	    return MATCH_FAILED;
	default:
# if DEBUG
	    fprintf(stderr, __FILE__ ": pcre_exec() failed: %d\n", rc);
# endif
	case PCRE_ERROR_MATCHLIMIT:
	case PCRE_ERROR_RECURSIONLIMIT:
	    return MATCH_ABORTED;
	}
    }

    if (rc == 0 || rc > MATCH_GROUP_LIMIT)
	rc = MATCH_GROUP_LIMIT;
    for (i = 0; i < rc; ++i) {
	if (ov[i * 2] < 0) {
	    indices[i].start = 0;
	    indices[i].end = -1;
	}
	else {
	    indices[i].start = 1 + ov[i * 2];
	    indices[i].end = ov[i * 2 + 1];
	}
    }
    for (i = rc; i < MATCH_GROUP_LIMIT; ++i) {
	indices[i].start = 0;
	indices[i].end = -1;
    }
    return MATCH_SUCCEEDED;
}

void free_pattern(Pattern p)
{
    regexp_t *regexp = p.ptr;

    if (regexp) {
	pcre_free(regexp->extra);
	pcre_free(regexp->code);
	myfree(regexp, M_PATTERN);
    }
}

#endif    /* HAVE_PCRE2 */
