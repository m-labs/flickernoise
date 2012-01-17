/*
 * Milkymist SoC (Software)
 * Copyright (C) 2007, 2008, 2009, 2010 Sebastien Bourdeauducq
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

%include {

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>

#include <fpvm/ast.h>
#include <fpvm/fpvm.h>

#include "symtab.h"
#include "parser_itf.h"
#include "parser_helper.h"
#include "parser.h"


struct yyParser;
static void yy_parse_failed(struct yyParser *yypParser);

typedef const char *(*assign_callback)(struct parser_comm *comm,
	    struct sym *sym, struct ast_node *node);

#define	FAIL(msg)				\
	do {					\
		error(state, msg);		\
		yy_parse_failed(yypParser);	\
	} while (0)

#define	OTHER_STYLE_new_style	old_style
#define	OTHER_STYLE_old_style	new_style

#define	IS_STYLE(which)						\
	do {							\
		if(state->style == OTHER_STYLE_##which) {	\
			FAIL("style mismatch");			\
			return;					\
		}						\
		state->style = which;				\
	} while (0)

static const enum ast_op tok2op[] = {
	[TOK_IDENT]	= op_ident,
	[TOK_CONSTANT]	= op_constant,
	[TOK_PLUS]	= op_plus,
	[TOK_MINUS]	= op_minus,
	[TOK_MULTIPLY]	= op_multiply,
	[TOK_DIVIDE]	= op_divide,
	[TOK_PERCENT]	= op_percent,
	[TOK_ABS]	= op_abs,
	[TOK_ISIN]	= op_isin,
	[TOK_ICOS]	= op_icos,
	[TOK_SIN]	= op_sin,
	[TOK_COS]	= op_cos,
	[TOK_ABOVE]	= op_above,
	[TOK_BELOW]	= op_below,
	[TOK_EQUAL]	= op_equal,
	[TOK_I2F]	= op_i2f,
	[TOK_F2I]	= op_f2i,
	[TOK_IF]	= op_if,
	[TOK_TSIGN]	= op_tsign,
	[TOK_QUAKE]	= op_quake,
	[TOK_SQR]	= op_sqr,
	[TOK_SQRT]	= op_sqrt,
	[TOK_INVSQRT]	= op_invsqrt,
	[TOK_MIN]	= op_min,
	[TOK_MAX]	= op_max,
	[TOK_INT]	= op_int,
};

static struct ast_node *node_op(enum ast_op op,
    struct ast_node *a, struct ast_node *b, struct ast_node *c)
{
	struct ast_node *n;

	n = malloc(sizeof(struct ast_node));
	n->op = op;
	n->sym = NULL;
	n->contents.branches.a = a;
	n->contents.branches.b = b;
	n->contents.branches.c = c;
	return n;
}

static struct ast_node *node(int token, struct sym *sym,
    struct ast_node *a, struct ast_node *b, struct ast_node *c)
{
	struct ast_node *n;

	n = node_op(tok2op[token], a, b, c);
	n->sym = &sym->fpvm_sym;
	return n;
}

static struct ast_node *constant(float n)
{
	struct ast_node *node;

	node = node_op(op_constant, NULL, NULL, NULL);
	node->contents.constant = n;
	return node;
}

#define FOLD_UNARY(res, ast_op, arg, expr)			\
	do {							\
		if((arg)->op == op_constant) {			\
			float a = (arg)->contents.constant;	\
								\
			res = constant(expr);			\
			parse_free(arg);			\
		} else {					\
			res = node_op(ast_op, arg, NULL, NULL);	\
		}						\
	} while (0)

#define FOLD_BINARY(res, ast_op, arg_a, arg_b, expr)			\
	do {								\
		if((arg_a)->op == op_constant &&			\
		    (arg_b)->op == op_constant) {			\
			float a = (arg_a)->contents.constant;		\
			float b = (arg_b)->contents.constant;		\
									\
			res = constant(expr);				\
			parse_free(arg_a);				\
			parse_free(arg_b);				\
		} else {						\
			res = node_op(ast_op, arg_a, arg_b, NULL);	\
		}							\
	} while (0)

static struct ast_node *conditional(struct ast_node *a,
    struct ast_node *b, struct ast_node *c)
{
	if(a->op == op_bnot) {
		struct ast_node *next = a->contents.branches.a;

		parse_free_one(a);
		return node_op(op_if, next, c, b);
	}
	if(a->op != op_constant)
		return node_op(op_if, a, b, c);
	if(a->contents.constant) {
		parse_free(a);
		parse_free(c);
		return b;
	} else {
		parse_free(a);
		parse_free(b);
		return c;
	}
}

static struct id *symbolify(struct id *id)
{
	const char *p;

	for(p = id->label; isalnum(*p); p++);
	id->sym = unique_n(id->label, p-id->label);
	return id;
}

} /* %include */


%start_symbol start
%extra_argument {struct parser_state *state}
%token_type {struct id *}

%token_destructor {
	free($$);
	(void) state;	/* suppress unused variable warning */
}

%type expr {struct ast_node *}
%type cond_expr {struct ast_node *}
%type equal_expr {struct ast_node *}
%type rel_expr {struct ast_node *}
%type add_expr {struct ast_node *}
%type mult_expr {struct ast_node *}
%type unary_expr {struct ast_node *}
%type primary_expr {struct ast_node *}

%destructor expr { free($$); }
%destructor cond_expr { free($$); }
%destructor equal_expr { free($$); }
%destructor rel_expr { free($$); }
%destructor add_expr { free($$); }
%destructor multexpr { free($$); }
%destructor unary_expr { free($$); }
%destructor primary_expr { free($$); }

%type context {assign_callback}

%syntax_error {
	FAIL("parse error");
}

start ::= TOK_START_EXPR expr(N). {
	state->comm->u.parseout = N;
	state->success = 1;
}

start ::= TOK_START_ASSIGN sections. {
	if(warn_undefined && state->style == old_style) {
		const struct sym *sym;

		foreach_sym(sym)
			if(!(sym->flags & (SF_SYSTEM | SF_ASSIGNED)))
				warn(state,
				    "variable %s is only read, never set",
				    sym->fpvm_sym.name);
	}
	state->success = 1;
}


/* ----- Sections and assignments ------------------------------------------ */


sections ::= assignments.
sections ::= assignments per_frame_label assignments.
sections ::= assignments per_frame_label assignments per_vertex_label
    assignments.
sections ::= assignments per_vertex_label assignments.

per_frame_label ::= TOK_PER_FRAME TOK_COLON. {
	IS_STYLE(new_style);
	state->assign = state->comm->assign_per_frame;
}

per_vertex_label ::= TOK_PER_VERTEX TOK_COLON. {
	IS_STYLE(new_style);
	state->assign = state->comm->assign_per_vertex;
}

assignments ::= assignments assignment.

assignments ::= .

assignment ::= ident(I) TOK_ASSIGN expr(N) opt_semi. {
	I->sym->flags |= SF_ASSIGNED;
	/*
	 * The conditions are as follows:
	 * - we must be outside compile_chunk (has different rules)
	 * - must be in the initial section
	 * - must not assign to a per-frame system variable
	 */
	if(state->comm->assign_per_frame &&
	    state->assign != state->comm->assign_per_frame &&
	    state->assign != state->comm->assign_per_vertex &&
	    I->sym->pfv_idx == -1) {
		free(I);
		if(N->op != op_constant || N->contents.constant) {
			FAIL("can initialize non-system variables only "
			    "to zero");
			return;
		}
		IS_STYLE(new_style);
	} else {
		const char *msg;

		msg = state->assign(state->comm, I->sym, N);
		free(I);
		if(msg) {
			FAIL(msg);
			free((void *) msg);
			return;
		}
	}
	parse_free(N);
}

assignment ::= TOK_IMAGEFILE(I) TOK_ASSIGN TOK_FNAME(N). {
	const char *msg;

	msg = state->comm->assign_image_name(state->comm,
	    atoi(I->label+9), N->fname);
	if(msg) {
		FAIL(msg);
		free((void *) msg);
		return;
	}
	free(I);
	free((void *) N->fname);
	free(N);
}

assignment ::= context(C). {
	/*
	 * @@@ Vile madness ahead: a lot of patches have per_frame= or
	 * per_vertex= tags followed by nothing else. We work around the
	 * syntax issue by making these tags "sticky".
	 *
	 * This subtly changes the semantics.
	 */
	state->assign = C;
}

context(C) ::= old_per_frame TOK_ASSIGN. {
	IS_STYLE(old_style);
	C = state->comm->assign_per_frame;
}

context(C) ::= old_per_vertex TOK_ASSIGN. {
	IS_STYLE(old_style);
	C = state->comm->assign_per_vertex;
}

old_per_frame ::= TOK_PER_FRAME.
old_per_frame ::= TOK_PER_FRAME_UGLY.

old_per_vertex ::= TOK_PER_VERTEX.
old_per_vertex ::= TOK_PER_VERTEX_UGLY.
old_per_vertex ::= TOK_PER_PIXEL.
old_per_vertex ::= TOK_PER_PIXEL_UGLY.

opt_semi ::= opt_semi TOK_SEMI.

opt_semi ::= .


/* ----- Operators --------------------------------------------------------- */


expr(N) ::= cond_expr(A). {
	N = A;
}

cond_expr(N) ::= equal_expr(A). {
	N = A;
}

cond_expr(N) ::= equal_expr(A) TOK_QUESTION expr(B) TOK_COLON cond_expr(C). {
	N = conditional(A, B, C);
}

equal_expr(N) ::= rel_expr(A). {
	N = A;
}

equal_expr(N) ::= equal_expr(A) TOK_EQ rel_expr(B). {
	FOLD_BINARY(N, op_equal, A, B, a == b);
}

equal_expr(N) ::= equal_expr(A) TOK_NE rel_expr(B). {
	struct ast_node *tmp;

	FOLD_BINARY(tmp, op_equal, A, B, a == b);
	FOLD_UNARY(N, op_bnot, tmp, !a);
}

rel_expr(N) ::= add_expr(A). {
	N = A;
}

rel_expr(N) ::= rel_expr(A) TOK_LT add_expr(B). {
	FOLD_BINARY(N, op_below, A, B, a < b);
}

rel_expr(N) ::= rel_expr(A) TOK_GT add_expr(B). {
	FOLD_BINARY(N, op_above, A, B, a > b);
}

rel_expr(N) ::= rel_expr(A) TOK_LE add_expr(B). {
	struct ast_node *tmp;

	FOLD_BINARY(tmp, op_above, A, B, a > b);
	FOLD_UNARY(N, op_bnot, tmp, !a);
}

rel_expr(N) ::= rel_expr(A) TOK_GE add_expr(B). {
	struct ast_node *tmp;

	FOLD_BINARY(tmp, op_below, A, B, a < b);
	FOLD_UNARY(N, op_bnot, tmp, !a);
}

add_expr(N) ::= mult_expr(A). {
	N = A;
}

add_expr(N) ::= add_expr(A) TOK_PLUS mult_expr(B). {
	FOLD_BINARY(N, op_plus, A, B, a + b);
}

add_expr(N) ::= add_expr(A) TOK_MINUS mult_expr(B). {
	FOLD_BINARY(N, op_minus, A, B, a - b);
}

mult_expr(N) ::= unary_expr(A). {
	N = A;
}

mult_expr(N) ::= mult_expr(A) TOK_MULTIPLY unary_expr(B). {
	FOLD_BINARY(N, op_multiply, A, B, a * b);
}

mult_expr(N) ::= mult_expr(A) TOK_DIVIDE unary_expr(B). {
	FOLD_BINARY(N, op_divide, A, B, a / b);
}

mult_expr(N) ::= mult_expr(A) TOK_PERCENT unary_expr(B). {
	FOLD_BINARY(N, op_percent, A, B, a-b*(int) (a/b));
}

unary_expr(N) ::= primary_expr(A). {
	N = A;
}

unary_expr(N) ::= TOK_MINUS unary_expr(A). {
	FOLD_UNARY(N, op_negate, A, -a);
}

unary_expr(N) ::= TOK_NOT unary_expr(A). {
	FOLD_UNARY(N, op_bnot, A, !a);
}


/* ----- Unary functions --------------------------------------------------- */


primary_expr(N) ::= unary_misc(I) TOK_LPAREN expr(A) TOK_RPAREN. {
	N = node(I->token, NULL, A, NULL, NULL);
	free(I);
}

primary_expr(N) ::= TOK_SQR TOK_LPAREN expr(A) TOK_RPAREN. {
	FOLD_UNARY(N, op_sqr, A, a*a);
}

primary_expr(N) ::= TOK_SQRT TOK_LPAREN expr(A) TOK_RPAREN. {
	FOLD_UNARY(N, op_sqrt, A, sqrtf(a));
}


/* ----- Binary functions -------------------------------------------------- */


primary_expr(N) ::= binary_misc(I) TOK_LPAREN expr(A) TOK_COMMA expr(B)
    TOK_RPAREN. {
	N = node(I->token, NULL, A, B, NULL);
	free(I);
}

primary_expr(N) ::= TOK_ABOVE TOK_LPAREN expr(A) TOK_COMMA expr(B) TOK_RPAREN. {
	FOLD_BINARY(N, op_above, A, B, a > b);
}

primary_expr(N) ::= TOK_BELOW TOK_LPAREN expr(A) TOK_COMMA expr(B) TOK_RPAREN. {
	FOLD_BINARY(N, op_below, A, B, a < b);
}

primary_expr(N) ::= TOK_EQUAL TOK_LPAREN expr(A) TOK_COMMA expr(B) TOK_RPAREN. {
	FOLD_BINARY(N, op_equal, A, B, a == b);
}

primary_expr(N) ::= TOK_MAX TOK_LPAREN expr(A) TOK_COMMA expr(B) TOK_RPAREN. {
	FOLD_BINARY(N, op_max, A, B, a > b ? a : b);
}

primary_expr(N) ::= TOK_MIN TOK_LPAREN expr(A) TOK_COMMA expr(B) TOK_RPAREN. {
	FOLD_BINARY(N, op_min, A, B, a < b ? a : b);
}


/* ----- Trinary functions ------------------------------------------------- */


primary_expr(N) ::= TOK_IF TOK_LPAREN expr(A) TOK_COMMA expr(B) TOK_COMMA
    expr(C) TOK_RPAREN. {
	N = conditional(A, B, C);
}


/* ----- Primary expressions ----------------------------------------------- */


primary_expr(N) ::= TOK_LPAREN expr(A) TOK_RPAREN. {
	N = A;
}

primary_expr(N) ::= TOK_CONSTANT(C). {
	N = constant(C->constant);
	free(C);
}

primary_expr(N) ::= ident(I). {
	if(warn_undefined && state->style == new_style &&
	    !(I->sym->flags & (SF_SYSTEM | SF_ASSIGNED)))
		warn(state, "reading undefined variable %s",
		    I->sym->fpvm_sym.name);
	N = node(I->token, I->sym, NULL, NULL, NULL);
	free(I);
}


/* ----- Identifiers ------------------------------------------------------- */

/*
 * Function names are not reserved words. If not followed by an opening
 * parenthesis, they become regular identifiers.
 *
 * {u,bi,ter}nary are identifiers that have an individual rule, e.g., because
 * they have function-specific code for constant folding. {u,bi,ter}nary_misc
 * are identifiers the parser treats as generic functions, without knowing
 * anything about their semantics.
 *
 * The use of symbolify() is somewhat inefficient, but use of function names
 * for variables should be a rare condition anyway.
 */

ident(O) ::= TOK_IDENT(I). {
	if(warn_section) {
		if(state->assign == state->comm->assign_per_frame &&
		    I->sym->pfv_idx == -1 && I->sym->pvv_idx != -1)
			warn(state, "using per-vertex variable %s in "
			    "per-frame section", I->sym->fpvm_sym.name);
		if(state->assign == state->comm->assign_per_vertex &&
		    I->sym->pfv_idx != -1 && I->sym->pvv_idx == -1)
			warn(state, "using per-frame variable %s in "
			    "per-vertex section", I->sym->fpvm_sym.name);
	}
	O = I;
}

ident(O) ::= unary(I).		{ O = symbolify(I); }
ident(O) ::= unary_misc(I).	{ O = symbolify(I); }
ident(O) ::= binary(I).		{ O = symbolify(I); }
ident(O) ::= binary_misc(I).	{ O = symbolify(I); }
ident(O) ::= ternary(I).	{ O = symbolify(I); }

unary_misc(O) ::= TOK_ABS(I).	{ O = I; }
unary_misc(O) ::= TOK_COS(I).	{ O = I; }
unary_misc(O) ::= TOK_F2I(I).	{ O = I; }
unary_misc(O) ::= TOK_ICOS(I).	{ O = I; }
unary_misc(O) ::= TOK_I2F(I).	{ O = I; }
unary_misc(O) ::= TOK_INT(I).	{ O = I; }
unary_misc(O) ::= TOK_INVSQRT(I).	{ O = I; }
unary_misc(O) ::= TOK_ISIN(I).	{ O = I; }
unary_misc(O) ::= TOK_QUAKE(I).	{ O = I; }
unary_misc(O) ::= TOK_SIN(I).	{ O = I; }
unary(O) ::= TOK_SQR(I).	{ O = I; }
unary(O) ::= TOK_SQRT(I).	{ O = I; }

binary(O) ::= TOK_ABOVE(I).	{ O = I; }
binary(O) ::= TOK_BELOW(I).	{ O = I; }
binary(O) ::= TOK_EQUAL(I).	{ O = I; }
binary(O) ::= TOK_MAX(I).	{ O = I; }
binary(O) ::= TOK_MIN(I).	{ O = I; }
binary_misc(O) ::= TOK_TSIGN(I).	{ O = I; }

ternary(O) ::= TOK_IF(I).	{ O = I; }
