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
	#include <stdlib.h>
	#include <malloc.h>
	#include <math.h>
	#include <fpvm/ast.h>
	#include <fpvm/fpvm.h>
	#include "parser_itf.h"
	#include "parser_helper.h"
	#include "parser.h"


	struct yyParser;
	static void yy_parse_failed(struct yyParser *yypParser);

	typedef const char *(*assign_callback)(struct parser_comm *comm,
	    const char *label, struct ast_node *node);

	const enum ast_op tok2op[] = {
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
		[TOK_NOT]	= op_not,
		[TOK_SQR]	= op_sqr,
		[TOK_SQRT]	= op_sqrt,
		[TOK_INVSQRT]	= op_invsqrt,
		[TOK_MIN]	= op_min,
		[TOK_MAX]	= op_max,
		[TOK_INT]	= op_int,
	};

	static struct ast_node *node(int token, const char *id,
	    struct ast_node *a, struct ast_node *b, struct ast_node *c)
	{
		struct ast_node *n;

		n = malloc(sizeof(struct ast_node));
		n->op = tok2op[token];
		n->label = id;
		n->contents.branches.a = a;
		n->contents.branches.b = b;
		n->contents.branches.c = c;
		return n;
	}

	static void syntax_error(struct parser_state *state)
	{
		if(!state->error_label) {
			state->error_label = state->id->label;
			state->error_lineno = state->id->lineno;
		}
	}
}

%start_symbol start
%extra_argument {struct parser_state *state}
%token_type {struct id *}

%token_destructor {
	free($$);
	(void) state;	/* suppress unused variable warning */
}

%type node {struct ast_node *}
%destructor node { free($$); }

%type context {assign_callback}

%syntax_error {
	syntax_error(state);
	yy_parse_failed(yypParser);
}

start ::= TOK_START_EXPR node(N). {
	state->comm->u.parseout = N;
	state->success = 1;
}

start ::= TOK_START_ASSIGN sections. {
	state->success = 1;
}

sections ::= assignments.
sections ::= assignments per_frame_label assignments.
sections ::= assignments per_frame_label assignments per_vertex_label
    assignments.
sections ::= assignments per_vertex_label assignments.

per_frame_label ::= TOK_PER_FRAME TOK_COLON. {
	state->comm->assign_default = state->comm->assign_per_frame;
}

per_vertex_label ::= TOK_PER_VERTEX TOK_COLON. {
	state->comm->assign_default = state->comm->assign_per_vertex;
}

assignments ::= assignments assignment.

assignments ::= .

assignment ::= ident(I) TOK_ASSIGN node(N) opt_semi. {
	state->error = state->comm->assign_default(state->comm, I->label, N);
	free(I);
	if(state->error) {
		syntax_error(state);
		yy_parse_failed(yypParser);
		return;
	}
	parse_free(N);
}

assignment ::= TOK_IMAGEFILE(I) TOK_ASSIGN TOK_FNAME(N). {
	state->error = state->comm->assign_image_name(state->comm,
	    atoi(I->label+9), N->label);
	free(I);
	if(state->error) {
		syntax_error(state);
		yy_parse_failed(yypParser);
		free((void *) N->label);
		free(N);
		return;
	}
	free((void *) N->label);
	free(N);
}

assignment ::= context(C). {
	/*
	 * @@@ Vile madness ahead: a lot of patches have per_frame= or
	 * per_vertex= tags followed by nothing else. We work around the
	 * syntax issue by making these tags "sticky".
	 *
	 * This subtly changes the semantics. Also, changing assign_default
	 * is not a good idea, since the caller may rely on it staying the
	 * same.
	 */
	state->comm->assign_default = C;
}

context(C) ::= TOK_PER_FRAME TOK_ASSIGN. {
	C = state->comm->assign_per_frame;
}

context(C) ::= TOK_PER_VERTEX TOK_ASSIGN. {
	C = state->comm->assign_per_vertex;
}

context(C) ::= TOK_PER_PIXEL TOK_ASSIGN. {
	C = state->comm->assign_per_vertex;
}

opt_semi ::= opt_semi TOK_SEMI.

opt_semi ::= .

node(N) ::= TOK_CONSTANT(C). {
	N = node(TOK_CONSTANT, "", NULL, NULL, NULL);
	N->contents.constant = C->constant;
	free(C);
}

node(N) ::= ident(I). {
	N = node(I->token, I->label, NULL, NULL, NULL);
	free(I);
}

%left TOK_PLUS TOK_MINUS.
%left TOK_MULTIPLY TOK_DIVIDE TOK_PERCENT.
%left TOK_NOT.

node(N) ::= node(A) TOK_PLUS node(B). {
	N = node(TOK_PLUS, "+", A, B, NULL);
}

node(N) ::= node(A) TOK_MINUS node(B). {
	N = node(TOK_MINUS, "-", A, B, NULL);
}

node(N) ::= node(A) TOK_MULTIPLY node(B). {
	N = node(TOK_MULTIPLY, "*", A, B, NULL);
}

node(N) ::= node(A) TOK_DIVIDE node(B). {
	N = node(TOK_DIVIDE, "/", A, B, NULL);
}

node(N) ::= node(A) TOK_PERCENT node(B). {
	N = node(TOK_PERCENT, "%", A, B, NULL);
}

node(N) ::= TOK_MINUS node(A). [TOK_NOT] {
	N = node(TOK_NOT, "!", A, NULL, NULL);
}

node(N) ::= unary(I) TOK_LPAREN node(A) TOK_RPAREN. {
	N = node(I->token, I->label, A, NULL, NULL);
	free(I);
}

node(N) ::= binary(I) TOK_LPAREN node(A) TOK_COMMA node(B) TOK_RPAREN. {
	N = node(I->token, I->label, A, B, NULL);
	free(I);
}

node(N) ::= ternary(I) TOK_LPAREN node(A) TOK_COMMA node(B) TOK_COMMA node(C)
    TOK_RPAREN. {
	N = node(I->token, I->label, A, B, C);
	free(I);
}

node(N) ::= TOK_LPAREN node(A) TOK_RPAREN. {
	N = A;
}

ident(O) ::= TOK_IDENT(I).	{ O = I; }
ident(O) ::= unary(I).		{ O = I; }
ident(O) ::= binary(I).		{ O = I; }
ident(O) ::= ternary(I).	{ O = I; }

unary(O) ::= TOK_ABS(I).	{ O = I; }
unary(O) ::= TOK_COS(I).	{ O = I; }
unary(O) ::= TOK_F2I(I).	{ O = I; }
unary(O) ::= TOK_ICOS(I).	{ O = I; }
unary(O) ::= TOK_I2F(I).	{ O = I; }
unary(O) ::= TOK_INT(I).	{ O = I; }
unary(O) ::= TOK_INVSQRT(I).	{ O = I; }
unary(O) ::= TOK_ISIN(I).	{ O = I; }
unary(O) ::= TOK_QUAKE(I).	{ O = I; }
unary(O) ::= TOK_SIN(I).	{ O = I; }
unary(O) ::= TOK_SQR(I).	{ O = I; }
unary(O) ::= TOK_SQRT(I).	{ O = I; }

binary(O) ::= TOK_ABOVE(I).	{ O = I; }
binary(O) ::= TOK_BELOW(I).	{ O = I; }
binary(O) ::= TOK_EQUAL(I).	{ O = I; }
binary(O) ::= TOK_MAX(I).	{ O = I; }
binary(O) ::= TOK_MIN(I).	{ O = I; }
binary(O) ::= TOK_TSIGN(I).	{ O = I; }

ternary(O) ::= TOK_IF(I).	{ O = I; }
