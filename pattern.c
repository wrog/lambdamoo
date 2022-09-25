/*
 * PCRE glue for LambdaMOO
 * Copyright (c) 2008 Robert Leslie
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies.  This software is provided
 * "as is" without express or implied warranty.
 */

# include <pcre.h>
# include "my-stdio.h"
# include "my-string.h"

# include "config.h"
# include "pattern.h"
# include "streams.h"
# include "utf.h"
# include "storage.h"
# include "exceptions.h"

# define DEBUG       0
# define UTF8_CHECK  0

# define MATCH_LIMIT            100000
# define MATCH_LIMIT_RECURSION    5000

typedef struct {
    pcre *code;
    pcre_extra *extra;
} regexp_t;

typedef struct {
    int ovec[10 * 2];
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

# define P_WORD          "[^\\W_]"
# define P_NONWORD       "[\\W_]"

# define P_ALT(a, b)     "(?:"a"|"b")"
# define P_LBEHIND(p)    "(?<="p")"
# define P_LAHEAD(p)     "(?="p")"
# define P_LOOKBA(b, a)  P_LBEHIND(b) P_LAHEAD(a)

# define P_WORD_BEGIN    P_ALT("^", P_LBEHIND(P_NONWORD)) P_LAHEAD(P_WORD)
# define P_WORD_END      P_LBEHIND(P_WORD) P_ALT("$", P_LAHEAD(P_NONWORD))

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

Pattern new_pattern(const char *pattern, int case_matters)
{
    int options = 0;
    const char *error;
    int error_offset;
    const char *translated;
    pcre *code;
    regexp_t *regexp = 0;
    Pattern p;

    options |= PCRE_UTF8;
# if !UTF8_CHECK
    options |= PCRE_NO_UTF8_CHECK;
# endif

    /* allow PCRE to optimize .* at beginning of pattern by implicit anchor */
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

	/*
	 * It would be nice to call pcre_study() only if the pattern is used
	 * more than once, but we need the pcre_extra block in any case and
	 * it's difficult to merge the study data later.
	 */
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

    if (!rmatch->valid || block->current_position > rmatch->ovec[1] ||
	(block->current_position == rmatch->ovec[1] &&
	 block->start_match < rmatch->ovec[0])) {
	/* make a copy of the offsets vector so the last such vector found can
	   be returned as the rightmost match */

	rmatch->ovec[0] = block->start_match;
	rmatch->ovec[1] = block->current_position;
	memcpy(&rmatch->ovec[2], &block->offset_vector[2],
	       sizeof(rmatch->ovec[2]) * 2 * (block->capture_top - 1));

	rmatch->valid = block->capture_top;
    }

    return 1;  /* cause match failure at current point, but continue trying */
}

Match_Result match_pattern(Pattern p, const char *string,
			   Match_Indices *indices, int is_reverse)
{
    regexp_t *regexp = p.ptr;
    pcre_extra *extra = regexp->extra;
    int rc, options = 0;
    int ovec[10 * 3];  /* N.B. PCRE needs the top 1/3 for internal use */
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

    if (rc == 0 || rc > 10)
	rc = 10;  /* there were more subpatterns than output vectors */

    for (i = 0; i < rc; ++i) {
	/* convert from 0-based open interval to 1-based closed one */
	indices[i].start = 1 + ov[i * 2 + 0];
	indices[i].end   =     ov[i * 2 + 1];
    }
    for (i = rc; i < 10; ++i) {
	indices[i].start =  0;
	indices[i].end   = -1;
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
