%{
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

/*************************************************************************/
/* NOTE: If you add an #include here, make sure you properly update the  */
/*       parser.o dependency line in the Makefile.			 */
/*************************************************************************/

#include "my-ctype.h"
#include "my-math.h"
#include "my-stdlib.h"
#include "my-string.h"

#include "ast.h"
#include "code_gen.h"
#include "config.h"
#include "functions.h"
#include "keywords.h"
#include "list.h"
#include "log.h"
#include "numbers.h"
#include "opcode.h"
#include "parser.h"
#include "program.h"
#include "storage.h"
#include "streams.h"
#include "structures.h"
#include "sym_table.h"
#include "utils.h"
#include "version.h"

static Stmt    	       *prog_start;
static int		dollars_ok;
static DB_Version	language_version;

static void	error(const char *, const char *);
static void	warning(const char *, const char *);
static int	find_id(char *name);
static void	yyerror(const char *s);
static int	yylex(void);
static Scatter *scatter_from_arglist(Arg_List *);
static Scatter *add_scatter_item(Scatter *, Scatter *);
static void	vet_scatter(Scatter *);
static void	push_loop_name(const char *);
static void	pop_loop_name(void);
static void	suspend_loop_scope(void);
static void	resume_loop_scope(void);

enum loop_exit_kind { LOOP_BREAK, LOOP_CONTINUE };

static void	check_loop_name(const char *, enum loop_exit_kind);
%}

%union {
  Stmt	       *stmt;
  Expr	       *expr;
  int		integer;
  Objid		object;
  double       *real;
  char	       *string;
  enum error	error;
  Arg_List     *args;
  Cond_Arm     *arm;
  Except_Arm   *except;
  Scatter      *scatter;
}

%type	<stmt>   statements statement elsepart
%type	<arm>    elseifs
%type   <expr>   expr default
%type   <args>   arglist ne_arglist codes
%type	<except> except excepts
%type	<string> opt_id
%type	<scatter> scatter scatter_item

%token	<integer> tINTEGER
%token	<object> tOBJECT
%token	<real> tFLOAT
%token	<string> tSTRING tID
%token	<error> tERROR
%token	tIF tELSE tELSEIF tENDIF tFOR tIN tENDFOR tRETURN tFORK tENDFORK
%token  tWHILE tENDWHILE tTRY tENDTRY tEXCEPT tFINALLY tANY tBREAK tCONTINUE

%token	tTO tARROW

%right	'='
%nonassoc '?' '|'
%left	tOR tAND
%left   tEQ tNE '<' tLE '>' tGE tIN
%left	'+' '-'
%left	'*' '/' '%'
%right	'^'
%left	'!' tUNARYMINUS
%nonassoc '.' ':' '[' '$'

%%

program:  statements
		{ prog_start = $1; }
	;

statements:
	  /* NOTHING */
		{ $$ = 0; }
	| statements statement
		{
		    if ($1) {
			Stmt *tmp = $1;

			while (tmp->next)
			    tmp = tmp->next;
			tmp->next = $2;
			$$ = $1;
		    } else
			$$ = $2;
		}
	;

statement:
	  tIF '(' expr ')' statements elseifs elsepart tENDIF
		{

		    $$ = alloc_stmt(STMT_COND);
		    $$->s.cond.arms = alloc_cond_arm($3, $5);
		    $$->s.cond.arms->next = $6;
		    $$->s.cond.otherwise = $7;
		}
	| tFOR tID tIN '(' expr ')'
		{
		    push_loop_name($2);
		}
	  statements tENDFOR
		{
		    $$ = alloc_stmt(STMT_LIST);
		    $$->s.list.id = find_id($2);
		    $$->s.list.expr = $5;
		    $$->s.list.body = $8;
		    pop_loop_name();
		}
	| tFOR tID tIN '[' expr tTO expr ']'
		{
		    push_loop_name($2);
		}
	  statements tENDFOR
		{
		    $$ = alloc_stmt(STMT_RANGE);
		    $$->s.range.id = find_id($2);
		    $$->s.range.from = $5;
		    $$->s.range.to = $7;
		    $$->s.range.body = $10;
		    pop_loop_name();
		}
	| tWHILE '(' expr ')'
		{
		    push_loop_name(0);
		}
	  statements tENDWHILE
		{
		    $$ = alloc_stmt(STMT_WHILE);
		    $$->s.loop.id = -1;
		    $$->s.loop.condition = $3;
		    $$->s.loop.body = $6;
		    pop_loop_name();
		}
	| tWHILE tID '(' expr ')'
		{
		    push_loop_name($2);
		}
	  statements tENDWHILE
		{
		    $$ = alloc_stmt(STMT_WHILE);
		    $$->s.loop.id = find_id($2);
		    $$->s.loop.condition = $4;
		    $$->s.loop.body = $7;
		    pop_loop_name();
		}
	| tFORK '(' expr ')'
		{
		    suspend_loop_scope();
		}
	  statements tENDFORK
		{
		    $$ = alloc_stmt(STMT_FORK);
		    $$->s.fork.id = -1;
		    $$->s.fork.time = $3;
		    $$->s.fork.body = $6;
		    resume_loop_scope();
		}
	| tFORK tID '(' expr ')'
		{
		    suspend_loop_scope();
		}
	  statements tENDFORK
		{
		    $$ = alloc_stmt(STMT_FORK);
		    $$->s.fork.id = find_id($2);
		    $$->s.fork.time = $4;
		    $$->s.fork.body = $7;
		    resume_loop_scope();
		}
	| expr ';'
		{
		    $$ = alloc_stmt(STMT_EXPR);
		    $$->s.expr = $1;
		}
	| tBREAK ';'
		{
		    check_loop_name(0, LOOP_BREAK);
		    $$ = alloc_stmt(STMT_BREAK);
		    $$->s.exit = -1;
		}
	| tBREAK tID ';'
		{
		    check_loop_name($2, LOOP_BREAK);
		    $$ = alloc_stmt(STMT_BREAK);
		    $$->s.exit = find_id($2);
		}
	| tCONTINUE ';'
		{
		    check_loop_name(0, LOOP_CONTINUE);
		    $$ = alloc_stmt(STMT_CONTINUE);
		    $$->s.exit = -1;
		}
	| tCONTINUE tID ';'
		{
		    check_loop_name($2, LOOP_CONTINUE);
		    $$ = alloc_stmt(STMT_CONTINUE);
		    $$->s.exit = find_id($2);
		}
	| tRETURN expr ';'
		{
		    $$ = alloc_stmt(STMT_RETURN);
		    $$->s.expr = $2;
		}
	| tRETURN ';'
		{
		    $$ = alloc_stmt(STMT_RETURN);
		    $$->s.expr = 0;
		}
	| ';'
		{ $$ = 0; }
    	| tTRY statements excepts tENDTRY
		{
		    $$ = alloc_stmt(STMT_TRY_EXCEPT);
		    $$->s.catch.body = $2;
		    $$->s.catch.excepts = $3;
		}
    	| tTRY statements tFINALLY statements tENDTRY
		{
		    $$ = alloc_stmt(STMT_TRY_FINALLY);
		    $$->s.finally.body = $2;
		    $$->s.finally.handler = $4;
		}
	;

elseifs:
	  /* NOTHING */
		{ $$ = 0; }
	| elseifs tELSEIF '(' expr ')' statements
		{
		    Cond_Arm *this_arm = alloc_cond_arm($4, $6);

		    if ($1) {
		        Cond_Arm *tmp = $1;

			while (tmp->next)
			    tmp = tmp->next;
			tmp->next = this_arm;
			$$ = $1;
		    } else
			$$ = this_arm;
		}
	;

elsepart:
	  /* NOTHING */
		{ $$ = 0; }
	| tELSE statements
		{ $$ = $2; }
	;

excepts:
	  tEXCEPT except
		{ $$ = $2; }
	| excepts tEXCEPT
		{
		    Except_Arm *tmp = $1;
		    int		count = 1;

		    while (tmp->next) {
			tmp = tmp->next;
			count++;
		    }
		    if (!tmp->codes)
			yyerror("Unreachable EXCEPT clause");
		    else if (count > 255)
			yyerror("Too many EXCEPT clauses (max. 255)");
		}
	  except
		{
		    Except_Arm *tmp = $1;

		    while (tmp->next)
			tmp = tmp->next;

		    tmp->next = $4;
		    $$ = $1;
		}
	;

except:
	  opt_id '(' codes ')' statements
		{ $$ = alloc_except($1 ? find_id($1) : -1, $3, $5); }
	;

opt_id:
	  /* NOTHING */
		{ $$ = 0; }
	| tID
		{ $$ = $1; }
	;

expr:
	  tINTEGER
		{
		    $$ = alloc_var(TYPE_INT);
		    $$->e.var.v.num = $1;
		}
	| tFLOAT
		{
		    $$ = alloc_var(TYPE_FLOAT);
		    $$->e.var.v.fnum = $1;
		}
	| tSTRING
		{
		    $$ = alloc_var(TYPE_STR);
		    $$->e.var.v.str = $1;
		}
	| tOBJECT
		{
		    $$ = alloc_var(TYPE_OBJ);
		    $$->e.var.v.obj = $1;
		}
	| tERROR
		{
		    $$ = alloc_var(TYPE_ERR);
		    $$->e.var.v.err = $1;
		}
	| tID
		{
		    $$ = alloc_expr(EXPR_ID);
		    $$->e.id = find_id($1);
		}
	| '$' tID
		{
		    /* Treat $foo like #0.("foo") */
		    Expr *obj = alloc_var(TYPE_OBJ);
		    Expr *prop = alloc_var(TYPE_STR);
		    obj->e.var.v.obj = 0;
		    prop->e.var.v.str = $2;
		    $$ = alloc_binary(EXPR_PROP, obj, prop);
		}
	| expr '.' tID
		{
		    /* Treat foo.bar like foo.("bar") for simplicity */
		    Expr *prop = alloc_var(TYPE_STR);
		    prop->e.var.v.str = $3;
		    $$ = alloc_binary(EXPR_PROP, $1, prop);
		}
	| expr '.' '(' expr ')'
    		{
		    $$ = alloc_binary(EXPR_PROP, $1, $4);
		}
	| expr ':' tID '(' arglist ')'
		{
		    /* treat foo:bar(args) like foo:("bar")(args) */
		    Expr *verb = alloc_var(TYPE_STR);
		    verb->e.var.v.str = $3;
		    $$ = alloc_verb($1, verb, $5);
		}
	| '$' tID '(' arglist ')'
		{
		    /* treat $bar(args) like #0:("bar")(args) */
		    Expr *obj = alloc_var(TYPE_OBJ);
		    Expr *verb = alloc_var(TYPE_STR);
		    obj->e.var.v.obj = 0;
		    verb->e.var.v.str = $2;
		    $$ = alloc_verb(obj, verb, $4);
		}
	| expr ':' '(' expr ')' '(' arglist ')'
		{
		    $$ = alloc_verb($1, $4, $7);
		}
	| expr '[' dollars_up expr ']'
		{
		    dollars_ok--;
		    $$ = alloc_binary(EXPR_INDEX, $1, $4);
		}
	| expr '[' dollars_up expr tTO expr ']'
		{
		    dollars_ok--;
		    $$ = alloc_expr(EXPR_RANGE);
		    $$->e.range.base = $1;
		    $$->e.range.from = $4;
		    $$->e.range.to = $6;
		}
	| '$'
		{
		    if (!dollars_ok)
			yyerror("Illegal context for `$' expression.");
		    $$ = alloc_expr(EXPR_LENGTH);
		}
	| expr '=' expr
                {
		    Expr *e = $1;

		    if (e->kind == EXPR_LIST) {
			e->kind = EXPR_SCATTER;
			if (e->e.list) {
			    e->e.scatter = scatter_from_arglist(e->e.list);
			    vet_scatter(e->e.scatter);
			} else
			    yyerror("Empty list in scattering assignment.");
		    } else {
			if (e->kind == EXPR_RANGE)
			    e = e->e.range.base;
			while (e->kind == EXPR_INDEX)
			    e = e->e.bin.lhs;
			if (e->kind != EXPR_ID  &&  e->kind != EXPR_PROP)
			    yyerror("Illegal expression on left side of"
				    " assignment.");
		    }
		    $$ = alloc_binary(EXPR_ASGN, $1, $3);
	        }
	| '{' scatter '}' '=' expr
		{
		    Expr       *e = alloc_expr(EXPR_SCATTER);

		    e->e.scatter = $2;
		    vet_scatter($2);
		    $$ = alloc_binary(EXPR_ASGN, e, $5);
		}
	| tID '(' arglist ')'
		{
		    unsigned f_no;

		    $$ = alloc_expr(EXPR_CALL);
		    if ((f_no = number_func_by_name($1)) == FUNC_NOT_FOUND) {
			/* Replace with call_function("$1", @args) */
			Expr	       *fname = alloc_var(TYPE_STR);
			Arg_List       *a = alloc_arg_list(ARG_NORMAL, fname);

			fname->e.var.v.str = $1;
			a->next = $3;
			warning("Unknown built-in function: ", $1);
			$$->e.call.func = number_func_by_name("call_function");
			$$->e.call.args = a;
		    } else {
			$$->e.call.func = f_no;
			$$->e.call.args = $3;
			dealloc_string($1);
		    }
		}
	| expr '+' expr
		{
		    $$ = alloc_binary(EXPR_PLUS, $1, $3);
		}
	| expr '-' expr
		{
		    $$ = alloc_binary(EXPR_MINUS, $1, $3);
		}
	| expr '*' expr
		{
		    $$ = alloc_binary(EXPR_TIMES, $1, $3);
		}
	| expr '/' expr
		{
		    $$ = alloc_binary(EXPR_DIVIDE, $1, $3);
		}
	| expr '%' expr
		{
		    $$ = alloc_binary(EXPR_MOD, $1, $3);
		}
	| expr '^' expr
		{
		    $$ = alloc_binary(EXPR_EXP, $1, $3);
		}
	| expr tAND expr
		{
		    $$ = alloc_binary(EXPR_AND, $1, $3);
		}
	| expr tOR expr
		{
		    $$ = alloc_binary(EXPR_OR, $1, $3);
		}
	| expr tEQ expr
		{
		    $$ = alloc_binary(EXPR_EQ, $1, $3);
		}
	| expr tNE expr
		{
		    $$ = alloc_binary(EXPR_NE, $1, $3);
		}
	| expr '<' expr
		{
		    $$ = alloc_binary(EXPR_LT, $1, $3);
		}
	| expr tLE expr
		{
		    $$ = alloc_binary(EXPR_LE, $1, $3);
		}
	| expr '>' expr
		{
		    $$ = alloc_binary(EXPR_GT, $1, $3);
		}
	| expr tGE expr
		{
		    $$ = alloc_binary(EXPR_GE, $1, $3);
		}
	| expr tIN expr
		{
		    $$ = alloc_binary(EXPR_IN, $1, $3);
		}
	| '-' expr  %prec tUNARYMINUS
		{
		    if ($2->kind == EXPR_VAR
			&& ($2->e.var.type == TYPE_INT
			    || $2->e.var.type == TYPE_FLOAT)) {
			switch ($2->e.var.type) {
			  case TYPE_INT:
			    $2->e.var.v.num = -$2->e.var.v.num;
			    break;
			  case TYPE_FLOAT:
			    *($2->e.var.v.fnum) = - (*($2->e.var.v.fnum));
			    break;
			  default:
			    break;
			}
		        $$ = $2;
		    } else {
			$$ = alloc_expr(EXPR_NEGATE);
			$$->e.expr = $2;
		    }
		}
	| '!' expr
		{
		    $$ = alloc_expr(EXPR_NOT);
		    $$->e.expr = $2;
		}
	| '(' expr ')'
		{ $$ = $2; }
	| '{' arglist '}'
		{
		    $$ = alloc_expr(EXPR_LIST);
		    $$->e.list = $2;
		}
	| expr '?' expr '|' expr
		{
		    $$ = alloc_expr(EXPR_COND);
		    $$->e.cond.condition = $1;
		    $$->e.cond.consequent = $3;
		    $$->e.cond.alternate = $5;
		}
	| '`' expr '!' codes default '\''
		{
		    $$ = alloc_expr(EXPR_CATCH);
		    $$->e.catch.try = $2;
		    $$->e.catch.codes = $4;
		    $$->e.catch.except = $5;
		}
	;

dollars_up:
	  /* NOTHING */
		{ dollars_ok++; }
	;

codes:
	  tANY
		{ $$ = 0; }
	| ne_arglist
		{ $$ = $1; }
	;

default:
	  /* NOTHING */
		{ $$ = 0; }
	| tARROW expr
		{ $$ = $2; }
	;

arglist:
	  /* NOTHING */
		{ $$ = 0; }
        | ne_arglist
                { $$ = $1; }
	;

ne_arglist:
	  expr
		{ $$ = alloc_arg_list(ARG_NORMAL, $1); }
	| '@' expr
		{ $$ = alloc_arg_list(ARG_SPLICE, $2); }
	| ne_arglist ',' expr
		{
		    Arg_List *this_arg = alloc_arg_list(ARG_NORMAL, $3);

		    if ($1) {
			Arg_List *tmp = $1;

			while (tmp->next)
			    tmp = tmp->next;
			tmp->next = this_arg;
			$$ = $1;
		    } else
			$$ = this_arg;
		}
	| ne_arglist ',' '@' expr
		{
		    Arg_List *this_arg = alloc_arg_list(ARG_SPLICE, $4);

		    if ($1) {
			Arg_List *tmp = $1;

			while (tmp->next)
			    tmp = tmp->next;
			tmp->next = this_arg;
			$$ = $1;
		    } else
			$$ = this_arg;
		}
	;

scatter:
	  ne_arglist ',' scatter_item
		{
		    Scatter    *sc = scatter_from_arglist($1);

		    if (sc)
			$$ = add_scatter_item(sc, $3);
		    else
			$$ = $3;
		}
	| scatter ',' scatter_item
		{
		    $$ = add_scatter_item($1, $3);
		}
	| scatter ',' tID
		{
		    $$ = add_scatter_item($1, alloc_scatter(SCAT_REQUIRED,
							    find_id($3), 0));
		}
	| scatter ',' '@' tID
		{
		    $$ = add_scatter_item($1, alloc_scatter(SCAT_REST,
							    find_id($4), 0));
		}
	| scatter_item
		{   $$ = $1;  }
	;

scatter_item:
	  '?' tID
		{
		    $$ = alloc_scatter(SCAT_OPTIONAL, find_id($2), 0);
		}
	| '?' tID '=' expr
		{
		    $$ = alloc_scatter(SCAT_OPTIONAL, find_id($2), $4);
		}
	;

%%

static int		lineno, nerrors, must_rename_keywords;
static Parser_Client	client;
static void	       *client_data;
static Names	       *local_names;

static int
find_id(char *name)
{
    int slot = find_or_add_name(&local_names, name);

    dealloc_string(name);
    return slot;
}

static void
yyerror(const char *s)
{
    error(s, 0);
}

static const char *
fmt_error(const char *s, const char *t)
{
    static Stream      *str = 0;

    if (str == 0)
	str = new_stream(100);
    if (t)
	stream_printf(str, "Line %d:  %s%s", lineno, s, t);
    else
	stream_printf(str, "Line %d:  %s", lineno, s);
    return reset_stream(str);
}

static void
error(const char *s, const char *t)
{
    nerrors++;
    (*(client.error))(client_data, fmt_error(s, t));
}

static void
warning(const char *s, const char *t)
{
    if (client.warning)
	(*(client.warning))(client_data, fmt_error(s, t));
    else
	error(s, t);
}

static int unget_buffer[5], unget_count;

static int
lex_getc(void)
{
    if (unget_count > 0)
	return unget_buffer[--unget_count];
    else
	return (*(client.getch))(client_data);
}

static void
lex_ungetc(int c)
{
    unget_buffer[unget_count++] = c;
}

static int
follow(int expect, int ifyes, int ifno)     /* look ahead for >=, etc. */
{
    int c = lex_getc();

    if (c == expect)
        return ifyes;
    lex_ungetc(c);
    return ifno;
}

static Stream  *token_stream = 0;

static int
yylex(void)
{
    int		c;

    reset_stream(token_stream);

start_over:

    do {
	c = lex_getc();
	if (c == '\n') lineno++;
    } while (isspace(c));

    if (c == '/') {
	c = lex_getc();
	if (c == '*') {
	    for (;;) {
		c = lex_getc();
		if (c == '*') {
		    c = lex_getc();
		    if (c == '/')
			goto start_over;
		}
		if (c == EOF) {
		    yyerror("End of program while in a comment");
		    return c;
		}
	    }
	} else {
	    lex_ungetc(c);
	    return '/';
	}
    }

    if (c == '#') {
	int            negative = 0;
	Objid          oid = 0;

	c = lex_getc();
	if (c == '-') {
	    negative = 1;
	    c = lex_getc();
	}
	if (!isdigit(c)) {
	    yyerror("Malformed object number");
	    lex_ungetc(c);
	    return 0;
	}
	do {
	    oid = oid * 10 + (c - '0');
	    c = lex_getc();
	} while (isdigit(c));
	lex_ungetc(c);

	yylval.object = negative ? -oid : oid;
	return tOBJECT;
    }

    if (isdigit(c) || (c == '.'  &&  language_version >= DBV_Float)) {
	int	n = 0;
	int	type = tINTEGER;

	while (isdigit(c)) {
	    n = n * 10 + (c - '0');
	    stream_add_char(token_stream, c);
	    c = lex_getc();
	}

	if (language_version >= DBV_Float && c == '.') {
	    /* maybe floating-point (but maybe `..') */
	    int cc;

	    lex_ungetc(cc = lex_getc()); /* peek ahead */
	    if (isdigit(cc)) {	/* definitely floating-point */
		type = tFLOAT;
		do {
		    stream_add_char(token_stream, c);
		    c = lex_getc();
		} while (isdigit(c));
	    } else if (stream_length(token_stream) == 0)
		/* no digits before or after `.'; not a number at all */
		goto normal_dot;
	    else if (cc != '.') {
		/* Some digits before dot, not `..' */
		type = tFLOAT;
		stream_add_char(token_stream, c);
		c = lex_getc();
	    }
	}

	if (language_version >= DBV_Float && (c == 'e' || c == 'E')) {
	    /* better be an exponent */
	    type = tFLOAT;
	    stream_add_char(token_stream, c);
	    c = lex_getc();
	    if (c == '+' || c == '-') {
		stream_add_char(token_stream, c);
		c = lex_getc();
	    }
	    if (!isdigit(c)) {
		yyerror("Malformed floating-point literal");
		lex_ungetc(c);
		return 0;
	    }
	    do {
		stream_add_char(token_stream, c);
		c = lex_getc();
	    } while (isdigit(c));
	}

	lex_ungetc(c);

	if (type == tINTEGER)
	    yylval.integer = n;
	else {
	    double	d;

	    d = strtod(reset_stream(token_stream), 0);
	    if (!IS_REAL(d)) {
		yyerror("Floating-point literal out of range");
		d = 0.0;
	    }
	    yylval.real = alloc_float(d);
	}
	return type;
    }

    if (isalpha(c) || c == '_') {
	char	       *buf;
	Keyword	       *k;

	stream_add_char(token_stream, c);
	while (isalnum(c = lex_getc()) || c == '_')
	    stream_add_char(token_stream, c);
	lex_ungetc(c);
	buf = reset_stream(token_stream);

	k = find_keyword(buf);
	if (k) {
	    if (k->version <= language_version) {
		int	t = k->token;

		if (t == tERROR)
		    yylval.error = k->error;
		return t;
	    } else {		/* New keyword being used as an identifier */
		if (!must_rename_keywords)
		    warning("Renaming old use of new keyword: ", buf);
		must_rename_keywords = 1;
	    }
	}

	yylval.string = alloc_string(buf);
	return tID;
    }

    if (c == '"') {
	while(1) {
	    c = lex_getc();
	    if (c == '"')
		break;
	    if (c == '\\')
		c = lex_getc();
	    if (c == '\n' || c == EOF) {
		yyerror("Missing quote");
		break;
	    }
	    stream_add_char(token_stream, c);
	}
	yylval.string = alloc_string(reset_stream(token_stream));
	return tSTRING;
    }

    switch(c) {
    case '>':
	return follow('=', tGE, '>');
    case '<':
	return follow('=', tLE, '<');
    case '=':
	return ((c = follow('=', tEQ, 0))
		? c
		: follow('>', tARROW, '='));
    case '!':
	return follow('=', tNE, '!');
    case '|':
	return follow('|', tOR, '|');
    case '&':
	return follow('&', tAND, '&');
    normal_dot:
    case '.':
	return follow('.', tTO, '.');
    default:
	return c;
    }
}

static Scatter *
add_scatter_item(Scatter *first, Scatter *last)
{
    Scatter    *tmp = first;

    while (tmp->next)
	tmp = tmp->next;
    tmp->next = last;

    return first;
}

static Scatter *
scatter_from_arglist(Arg_List *a)
{
    Scatter    *sc = 0, **scp;
    Arg_List   *anext;

    for (scp = &sc; a; a = anext, scp = &((*scp)->next)) {
	if (a->expr->kind == EXPR_ID) {
	    *scp = alloc_scatter(a->kind == ARG_NORMAL ? SCAT_REQUIRED
						       : SCAT_REST,
				 a->expr->e.id, 0);
	    anext = a->next;
	    dealloc_node(a->expr);
	    dealloc_node(a);
	} else {
	    yyerror("Scattering assignment targets must be simple variables.");
	    return 0;
	}
    }

    return sc;
}

static void
vet_scatter(Scatter *sc)
{
    int	seen_rest = 0, count = 0;

    for (; sc; sc = sc->next) {
	if (sc->kind == SCAT_REST) {
	    if (seen_rest)
		yyerror("More than one `@' target in scattering assignment.");
	    else
		seen_rest = 1;
	}
	count++;
    }

    if (count > 255)
	yyerror("Too many targets in scattering assignment.");
}

struct loop_entry {
    struct loop_entry  *next;
    const char	       *name;
    int			is_barrier;
};

static struct loop_entry *loop_stack;

static void
push_loop_name(const char *name)
{
    struct loop_entry  *entry = mymalloc(sizeof(struct loop_entry), M_AST);

    entry->next = loop_stack;
    entry->name = (name ? str_dup(name) : 0);
    entry->is_barrier = 0;
    loop_stack = entry;
}

static void
pop_loop_name(void)
{
    if (!loop_stack)
	errlog("PARSER: Empty loop stack in POP_LOOP_NAME!\n");
    else if (loop_stack->is_barrier)
	errlog("PARSER: Tried to pop loop-scope barrier!\n");
    else {
	struct loop_entry      *entry = loop_stack;

	loop_stack = loop_stack->next;
	if (entry->name)
	    free_str(entry->name);
	myfree(entry, M_AST);
    }
}

static void
suspend_loop_scope(void)
{
    struct loop_entry  *entry = mymalloc(sizeof(struct loop_entry), M_AST);

    entry->next = loop_stack;
    entry->name = 0;
    entry->is_barrier = 1;
    loop_stack = entry;
}

static void
resume_loop_scope(void)
{
    if (!loop_stack)
	errlog("PARSER: Empty loop stack in RESUME_LOOP_SCOPE!\n");
    else if (!loop_stack->is_barrier)
	errlog("PARSER: Tried to resume non-loop-scope barrier!\n");
    else {
	struct loop_entry      *entry = loop_stack;

	loop_stack = loop_stack->next;
	myfree(entry, M_AST);
    }
}

static void
check_loop_name(const char *name, enum loop_exit_kind kind)
{
    struct loop_entry  *entry;

    if (!name) {
	if (!loop_stack  ||  loop_stack->is_barrier) {
	    if (kind == LOOP_BREAK)
		yyerror("No enclosing loop for `break' statement");
	    else
		yyerror("No enclosing loop for `continue' statement");
	}
	return;
    }

    for (entry = loop_stack; entry && !entry->is_barrier; entry = entry->next)
	if (entry->name  &&  mystrcasecmp(entry->name, name) == 0)
	    return;

    if (kind == LOOP_BREAK)
	error("Invalid loop name in `break' statement: ", name);
    else
	error("Invalid loop name in `continue' statement: ", name);
}

extern int yyparse(void);

Program *
parse_program(DB_Version version, Parser_Client c, void *data)
{
    Program    *prog;

    if (token_stream == 0)
	token_stream = new_stream(1024);
    unget_count = 0;
    nerrors = 0;
    must_rename_keywords = 0;
    lineno = 1;
    client = c;
    client_data = data;
    local_names = new_builtin_names(version);
    dollars_ok = 0;
    loop_stack = 0;
    language_version = version;

    begin_code_allocation();
    yyparse();
    end_code_allocation(nerrors > 0);
    if (loop_stack) {
	if (nerrors == 0)
	    errlog("PARSER: Non-empty loop-scope stack!\n");
	while (loop_stack) {
	    struct loop_entry *entry = loop_stack;

	    loop_stack = loop_stack->next;
	    if (entry->name)
		free_str(entry->name);
	    myfree(entry, M_AST);
	}
    }

    if (nerrors == 0) {
	if (must_rename_keywords) {
	    /* One or more new keywords were used as identifiers in this code,
	     * possibly as local variable names (but maybe only as property or
	     * verb names).  Such local variables must be renamed to avoid a
	     * conflict in the new world.  We just add underscores to the end
	     * until it stops conflicting with any other local variable.
	     */
	    unsigned i;

	    for (i = first_user_slot(version); i < local_names->size; i++) {
		const char	*name = local_names->names[i];

		if (find_keyword(name)) { /* Got one... */
		    stream_add_string(token_stream, name);
		    do {
			stream_add_char(token_stream, '_');
		    } while (find_name(local_names,
				       stream_contents(token_stream)) >= 0);
		    free_str(name);
		    local_names->names[i] =
			str_dup(reset_stream(token_stream));
		}
	    }
	}

	prog = generate_code(prog_start, version);
	prog->num_var_names = local_names->size;
	prog->var_names = local_names->names;

	myfree(local_names, M_NAMES);
	free_stmt(prog_start);

	return prog;
    } else {
	free_names(local_names);
	return 0;
    }
}

struct parser_state {
    Var		code;		/* a list of strings */
    int		cur_string;	/* which string? */
    int		cur_char;	/* which character in that string? */
    Var		errors;		/* a list of strings */
};

static void
my_error(void *data, const char *msg)
{
    struct parser_state	*state = (struct parser_state *) data;
    Var			v;

    v.type = TYPE_STR;
    v.v.str = str_dup(msg);
    state->errors = listappend(state->errors, v);
}

static int
my_getc(void *data)
{
    struct parser_state	*state = (struct parser_state *) data;
    Var			code;
    char		c;

    code = state->code;
    if (task_timed_out  ||  state->cur_string > code.v.list[0].v.num)
	return EOF;
    else if (!(c = code.v.list[state->cur_string].v.str[state->cur_char])) {
	state->cur_string++;
	state->cur_char = 0;
	return '\n';
    } else {
	state->cur_char++;
	return c;
    }
}

static Parser_Client list_parser_client = { my_error, 0, my_getc };

Program *
parse_list_as_program(Var code, Var *errors)
{
    struct parser_state	state;
    Program	       *program;

    state.code = code;
    state.cur_string = 1;
    state.cur_char = 0;
    state.errors = new_list(0);
    program = parse_program(current_db_version, list_parser_client, &state);
    *errors = state.errors;

    return program;
}

char rcsid_parser[] = "$Id$";

/*
 * $Log$
 * Revision 1.5  2010/04/22 21:30:18  wrog
 * current_version -> current_db_version
 *
 * Revision 1.4  2004/05/22 01:25:44  wrog
 * merging in WROGUE changes (W_SRCIP, W_STARTUP, W_OOB)
 *
 * Revision 1.3  2004/04/09 19:52:52  bjj
 * == Revision 1.2.8.1  2003/06/07 20:16:24 wrog
 * fixed 6 rules that were missing final semicolons to make newer bison happy.
 *
 * Revision 1.2  1998/12/14 13:18:45  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.1.1.1  1997/03/03 03:45:02  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.11  1996/05/12  21:28:37  pavel
 * Removed non-backward-compatible parse error for negating a non-numeric
 * literal.  Made certain aspects of floating-point literal parsing be
 * language-version-dependent, to maintain backward compatibility with DBs
 * written by pre-float servers.  Release 1.8.0p5.
 *
 * Revision 2.10  1996/04/19  01:26:38  pavel
 * Fixed potential memory smash when `$' expression occurs in an illegal
 * context.  Release 1.8.0p4.
 *
 * Revision 2.9  1996/03/10  01:08:01  pavel
 * Removed bogus `call_function("foo", ...)' --> `foo(...)' conversion.
 * Fixed line numbers of renamed-variable warnings.  Release 1.8.0.
 *
 * Revision 2.8  1996/02/18  23:14:54  pavel
 * Liberalized parsing of floating-point numbers to accept `3.' and `3.e1'.
 * Changed parsing to replace call_function("foo", @args) with foo(@args) if
 * foo is a known function.  Release 1.8.0beta3.
 *
 * Revision 2.7  1996/02/11  00:46:01  pavel
 * Liberalized parsing of floating-point literals to accept `.02' and the
 * like.  Fixed a bug where `1E6' and the like would be parsed as an integer.
 * Release 1.8.0beta2.
 *
 * Revision 2.6  1996/02/08  06:32:48  pavel
 * Added support for floating-point literals, exponentiation expression, named
 * WHILE loops, and BREAK and CONTINUE statements.  Generalized support for
 * version-dependent compilation.  Renamed TYPE_NUM to TYPE_INT.  Removed use
 * of ungetch() method on Parser_Client.  Updated copyright notice for 1996.
 * Release 1.8.0beta1.
 *
 * Revision 2.5  1996/01/16  07:17:05  pavel
 * Add support for scattering assignment.  Release 1.8.0alpha6.
 *
 * Revision 2.4  1996/01/11  07:50:49  pavel
 * Added missing #include "my-string.h".  Release 1.8.0alpha5.
 *
 * Revision 2.3  1995/12/31  00:09:40  pavel
 * Removed unused tokens tRAISE and tFORLIST (huh?).  Added `$' expression.
 * Release 1.8.0alpha4.
 *
 * Revision 2.2  1995/12/28  00:43:25  pavel
 * Added some support for MOO-compilation warnings.  Changed `unknown built-in
 * function' error into a warning with recovery.  Changed identifier-renaming
 * log message into a warning.  Release 1.8.0alpha3.
 *
 * Revision 2.1  1995/12/11  08:01:26  pavel
 * Added support for `$foo(...)' syntax.  Release 1.8.0alpha2.
 *
 * Revision 2.0  1995/11/30  04:48:11  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.13  1992/10/23  23:03:47  pavel
 * Added copyright notice.
 *
 * Revision 1.12  1992/10/23  22:21:43  pavel
 * Eliminated all uses of the useless macro NULL.
 *
 * Revision 1.11  1992/10/21  03:02:35  pavel
 * Converted to use new automatic configuration system.
 *
 * Revision 1.10  1992/08/31  22:25:51  pjames
 * Changed some `char *'s to `const char *'
 *
 * Revision 1.9  1992/08/28  23:18:38  pjames
 * Added ASGN_RANGE parsing code.  Fixed tiny memory leak.
 *
 * Revision 1.8  1992/08/28  16:16:52  pjames
 * Changed `ak_dealloc_string()' to `free_str()'.
 *
 * Revision 1.7  1992/08/12  01:49:58  pjames
 * Negative literals are now accepted (instead of negated positive literals).
 *
 * Revision 1.6  1992/08/10  16:55:59  pjames
 * Updated #includes.
 *
 * Revision 1.5  1992/07/30  21:22:08  pjames
 * Removed max_stack calculations (moved to vector.c).
 *
 * Revision 1.4  1992/07/29  18:05:12  pjames
 * $$ no longer = NULL when there is an illegal left hand side of an
 * expression.
 *
 * Revision 1.3  1992/07/27  18:12:41  pjames
 * Changed name of ct_env to var_names and const_env to literals.
 * Modified call to vectorize to use new argument format.
 * Assignment statements now check left hand side properly for allowable
 * expressions, and now frees memory from reused expressions.
 *
 * Revision 1.2  1992/07/21  00:05:16  pavel
 * Added rcsid_<filename-root> declaration to hold the RCS ident. string.
 *
 * Revision 1.1  1992/07/20  23:23:12  pavel
 * Initial RCS-controlled version.
 */
