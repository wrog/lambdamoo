/******************************************************************************
  Copyright (c) 1995, 1996 Xerox Corporation.  All rights reserved.
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

/*****************************************************************************
 * Routines for use by non-DB modules with persistent state stored in the DB
 *****************************************************************************/

#include "config.h"
#include "my-ctype.h"
#include "my-math.h"
#include "my-stdarg.h"
#include "my-stdio.h"
#include "my-stdlib.h"
#include "my-string.h"
#include <errno.h>

#include "db_io.h"
#include "db_private.h"
#include "exceptions.h"
#include "list.h"
#include "log.h"
#include "numbers.h"
#include "parser.h"
#include "storage.h"
#include "streams.h"
#include "structures.h"
#include "str_intern.h"
#include "unparse.h"
#include "utils.h"
#include "version.h"


/*********** Input ***********/

static FILE *input;

static const char *dbio_last_error = NULL;

void
dbpriv_set_dbio_input(FILE * f)
{
    input = f;
}

int
dbio_peek_byte(void)
{
    int c = fgetc(input);
    if (EOF != c)
	ungetc(c, input);
    return c;
}

void
dbio_skip_lines(size_t n)
{
    int32_t c;
    while ((EOF != (c = getc(input)))
	   && (c != '\n' || --n));
}

/*------------------------*
 |  reading entire lines  |
 *------------------------*/

static Stream *dbio_line_stream = 0;

static void
dbio_free_line_stream(void)
{
    if (dbio_line_stream) {
	free_stream(dbio_line_stream);
	dbio_line_stream = 0;
    }
}

void
dbpriv_dbio_input_finished(void)
{
    dbio_free_line_stream();
}

static const char *
dbio_read_line(const char **end)
{
    char *buffer;
    size_t blen, len;

    if (!dbio_line_stream)
	dbio_line_stream = new_stream(0);

    do {
	stream_beginfill(dbio_line_stream, 2, &buffer, &blen);
	if (!fgets(buffer, blen + 1, input))
	    break;
	len = strlen(buffer);
	stream_endfill(dbio_line_stream, blen - len);
    } while (len == blen && buffer[len-1] != '\n');

    if (stream_last_byte(dbio_line_stream) != '\n') {
	if (end)
	    *end = NULL;
	return NULL;
    }
    stream_delete_char(dbio_line_stream);
    if (end) {
	size_t rlen = stream_length(dbio_line_stream);
	*end = stream_contents(dbio_line_stream) + rlen;
    }
    return reset_stream(dbio_line_stream);
}

int
dbio_scanf(const char *format,...)
{
    va_list args;
    int count;

    va_start(args, format);
    count = vfscanf(input, format, args);
    va_end(args);

    return count;
}

/*--------------------------*
 |  integer range checking  |
 *--------------------------*/

#define  DBIO_INT16_INIT  { INT16_MIN,  INT16_MAX, 0 }
#define DBIO_UINT16_INIT  {         0, UINT16_MAX, 0 }
#define DBIO_INTMAX_INIT  { .skip=1 }

#if NUM_MAX == INTMAX_MAX
#  define  DBIO_NUM_INIT  DBIO_INTMAX_INIT
#else
#  define  DBIO_NUM_INIT  { NUM_MIN, NUM_MAX, 0 }
#endif

#if INT_MAX < INTMAX_MAX
#  define  DBIO_INT_INIT  { INT_MIN,  INT_MAX, 0 }
#  define DBIO_UINT_INIT  {      0,  UINT_MAX, 0 }

#else  /* INT_MAX == INTMAX_MAX */
#  define  DBIO_INT_INIT  DBIO_INTMAX_INIT
#  define DBIO_UINT_INIT  { 0, INTMAX_MAX, 0 }
/* Why INTMAX_MAX instead of UINTMAX_MAX?
   Answer:
   In this situation, we would have to go to extra trouble to preserve
   that top bit, and having something declared as 'unsigned' rather
   than 'uintmax_t' means the author was never intending to put really
   large quantities there or be using that top bit as a flag.
   Forbidding this means we can read everything as intmax_t and then
   complain if we see anything going outside that range, which is most
   likely a mistake anyway.
*/
#endif

static struct intrange {
    intmax_t min;
    intmax_t max;
    uint8_t skip;
}
#define DBIO_DO_(INTXX,_2,_3,_4)   DBIO_##INTXX##_INIT,
    dbio_intranges[] = {
       DBIO_RANGE_SPEC_LIST(DBIO_DO_)
};
#undef DBIO_DO_


static intmax_t
dbio_string_to_integer(enum dbio_intrange range_id, const char *s, const char **end)
{
    errno = 0;
    intmax_t i = strtoimax(s, (char **)end, 10);
    const struct intrange *range = dbio_intranges + range_id;

    dbio_last_error = NULL;
    if (errno == ERANGE)
	dbio_last_error = "Integer overflow on read";
    else if (errno)
	dbio_last_error = "Some other strtoimax() error";
    else if (*end == s)
	dbio_last_error = "Integer expected";
    else if (range->skip)
	;
    else if (i < range->min)
	dbio_last_error = range->min ? "Integer too negative" : "Integer must be unsigned";
    else if (range->max < i)
	dbio_last_error = "Integer too large";
    return i;
}

/*-----------------------------*
 |  reading individual values  |
 *-----------------------------*/

intmax_t
dbio_read_integer(enum dbio_intrange range_id)
{
    const char *s, *p1, *p2;

    if (!(s = dbio_read_line(&p1))) {
	errlog("DBIO_READ_INTEGER: Unexpected end of file\n");
	dbio_last_error = "Unexpected end of file";
	return 0;
    }
    intmax_t i = dbio_string_to_integer(range_id, s, &p2);
    if (!dbio_last_error && (isspace(*s) || p1 != p2))
	dbio_last_error = "Did not read entire line";

    if (dbio_last_error)
	errlog("DBIO_READ_INTEGER: %s: \"%s\" at file pos. %ld\n",
	       dbio_last_error, s, ftell(input));
    return i;
}

double
dbio_read_float(void)
{
    const char *s, *p1, *p2;

    if (!(s = dbio_read_line(&p1))) {
	errlog("DBIO_READ_FLOAT: Unexpected end of file\n");
	dbio_last_error = "Unexpected end of file";
	return 0.0;
    }
    double d = strtod(s, (char **)&p2);
    dbio_last_error =
	(isspace(*s) || p1 != p2)
	? "Did not read entire line"
	: (!IS_REAL(d) ? "Magnitude too large or NaN" : NULL);

    if (dbio_last_error)
	errlog("DBIO_READ_FLOAT: %s: \"%s\" at file pos. %ld\n",
	       dbio_last_error, s, ftell(input));
    return d;
}

Objid
dbio_read_objid(void)
{
    return dbio_read_num();
}

const char *
dbio_read_string(void)
{
    return dbio_read_line(NULL);
}

const char *
dbio_read_string_intern(void)
{
    return str_intern(dbio_read_line(NULL));
}

Var
dbio_read_var(void)
{
    intmax_t vtype = dbio_read_intmax();
    if (dbio_last_error)
	return zero;
    return dbio_read_var_value(vtype);
}

Var
dbio_read_var_value(intmax_t vtype)
{
    Var r;
    if (vtype == TYPE_ANY && dbio_input_version == DBV_Prehistory)
	vtype = TYPE_NONE;  /* Old encoding for VM's empty temp register
			     * and any as-yet unassigned variables.
			     */
    r.type = (var_type) vtype;
    switch (vtype) {
    case TYPE_CLEAR:
    case TYPE_NONE:
	break;
    case _TYPE_STR:
	r.v.str = dbio_read_string_intern();
	r.type |= TYPE_COMPLEX_FLAG;
	break;
    case TYPE_OBJ:
    case TYPE_ERR:
    case TYPE_INT:
    case TYPE_CATCH:
    case TYPE_FINALLY:
	r.v.num = dbio_read_num();
	break;
    case _TYPE_FLOAT:
	r.v.fnum = dbio_read_float();
	break;
    case _TYPE_LIST: ;
	Num len = dbio_read_num();
	if (dbio_last_error)
	    return zero;
	r = new_list(len); /* overwrites r.type */
	Num i;
	for (i = 1; i <= len; ++i) {
	    r.v.list[i] = dbio_read_var();
	    if (dbio_last_error) {
		r.v.list[0].v.num = i-1;
		complex_free_var(r);
		return zero;
	    }
	}
	break;
    default:
	dbio_last_error = "Unknown Var type";
	errlog("DBIO_READ_VAR: Unknown type (%jd) at DB file pos. %ld\n",
	       vtype, ftell(input));
	r = zero;
	break;
    }
    return r;
}

/*---------------------*
 |  dbio_read_program  |
 *---------------------*/

struct state {
    char prev_char;
    const char *(*fmtr) (void *);
    void *data;
};

static const char *
program_name(struct state *s)
{
    if (!s->fmtr)
	return s->data;
    else
	return (*s->fmtr) (s->data);
}

static void
my_error(void *data, const char *msg)
{
    errlog("PARSER: Error in %s:\n", program_name(data));
    errlog("           %s\n", msg);
}

static void
my_warning(void *data, const char *msg)
{
    oklog("PARSER: Warning in %s:\n", program_name(data));
    oklog("           %s\n", msg);
}

static int
my_getc(void *data)
{
    struct state *s = data;
    int c;

    c = fgetc(input);
    if (c == '.' && s->prev_char == '\n') {
	/* end-of-verb marker in DB */
	c = fgetc(input);	/* skip next newline */
	return EOF;
    }
    if (c == EOF)
	my_error(data, "Unexpected EOF");
    s->prev_char = c;
    return c;
}

static Parser_Client parser_client =
{my_error, my_warning, my_getc};

Program *
dbio_read_program(DB_Version version, const char *(*fmtr) (void *), void *data)
{
    struct state s;

    s.prev_char = '\n';
    s.fmtr = fmtr;
    s.data = data;
    return parse_program(version, parser_client, &s);
}


/*********** Output ***********/

Exception dbpriv_dbio_failed;

static FILE *output;

void
dbpriv_set_dbio_output(FILE * f)
{
    output = f;
}

void
dbio_printf(const char *format,...)
{
    va_list args;

    va_start(args, format);
    if (vfprintf(output, format, args) < 0)
	RAISE(dbpriv_dbio_failed, 0);
    va_end(args);
}

void
dbio_write_intmax(intmax_t n)
{
    dbio_printf("%"PRIdMAX"\n", n);
}

void
dbio_write_float(double d)
{
    static const char *fmt = 0;
    static char buffer[10];

    if (!fmt) {
	sprintf(buffer, "%%.%dg\n", DBL_DIG + 4);
	fmt = buffer;
    }
    dbio_printf(fmt, d);
}

void
dbio_write_objid(Objid oid)
{
    dbio_write_intmax(oid);
}

void
dbio_write_string(const char *s)
{
    dbio_printf("%s\n", s ? s : "");
}

void
dbio_write_var(Var v)
{
    int i;

    dbio_write_intmax((intmax_t) v.type & TYPE_DB_MASK);
    switch ((int) v.type) {
    case TYPE_CLEAR:
    case TYPE_NONE:
	break;
    case TYPE_STR:
	dbio_write_string(v.v.str);
	break;
    case TYPE_OBJ:
    case TYPE_ERR:
    case TYPE_INT:
    case TYPE_CATCH:
    case TYPE_FINALLY:
	dbio_write_intmax(v.v.num);
	break;
    case TYPE_FLOAT:
	dbio_write_float(v.v.fnum);
	break;
    case TYPE_LIST:
	dbio_write_intmax(v.v.list[0].v.num);
	for (i = 0; i < v.v.list[0].v.num; i++)
	    dbio_write_var(v.v.list[i + 1]);
	break;
    }
}

static void
receiver(void *data UNUSED_, const char *line)
{
    dbio_printf("%s\n", line);
}

void
dbio_write_program(Program * program)
{
    unparse_program(program, receiver, 0, 1, 0, MAIN_VECTOR);
    dbio_printf(".\n");
}

void
dbio_write_forked_program(Program * program, int f_index)
{
    unparse_program(program, receiver, 0, 1, 0, f_index);
    dbio_printf(".\n");
}

char rcsid_db_io[] = "$Id$";

/*
 * $Log$
 * Revision 1.5  1998/12/14 13:17:34  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.4  1998/02/19 07:36:16  nop
 * Initial string interning during db load.
 *
 * Revision 1.3  1997/07/07 03:24:53  nop
 * Merge UNSAFE_OPTS (r5) after extensive testing.
 *
 * Revision 1.2.2.1  1997/03/20 18:07:51  bjj
 * Add a flag to the in-memory type identifier so that inlines can cheaply
 * identify Vars that need actual work done to ref/free/dup them.  Add the
 * appropriate inlines to utils.h and replace old functions in utils.c with
 * complex_* functions which only handle the types with external storage.
 *
 * Revision 1.2  1997/03/03 04:18:27  nop
 * GNU Indent normalization
 *
 * Revision 1.1.1.1  1997/03/03 03:44:59  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.5  1996/03/19  07:16:12  pavel
 * Increased precision of floating-point numbers printed in the DB file.
 * Release 1.8.0p2.
 *
 * Revision 2.4  1996/03/10  01:04:16  pavel
 * Increased the precision of printed floating-point numbers by two digits.
 * Release 1.8.0.
 *
 * Revision 2.3  1996/02/08  07:19:15  pavel
 * Renamed err/logf() to errlog/oklog() and TYPE_NUM to TYPE_INT.  Added
 * dbio_read/write_float().  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.2  1995/12/28  00:44:51  pavel
 * Added support for receiving MOO-compilation warnings during loading and for
 * printing useful error and warning messages in the log.
 * Release 1.8.0alpha3.
 *
 * Revision 2.1  1995/12/11  07:59:50  pavel
 * Fixed broken #includes.  Removed another silly use of `unsigned'.
 *
 * Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  04:20:10  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.1  1995/11/30  04:19:56  pavel
 * Initial revision
 */
