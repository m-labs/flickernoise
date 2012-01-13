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

#include <stdarg.h>
#define _GNU_SOURCE /* for asprintf */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <malloc.h>

#include <fpvm/ast.h>

#include "symtab.h"
#include "scanner.h"
#include "parser.h"
#include "parser_itf.h"
#include "parser_helper.h"


void verror(struct parser_state *state, const char *fmt, va_list ap)
{
	/*
	 * If "error" or "error_label" are already set, then we keep the
	 * previous value. There are two reasons for this:
	 *
	 * - "error" may have already been set before calling error()
	 *
 	 * - we may be in the process of exiting the parser and are running
	 *   into false or unrelated problems
	 */

	if(!state->error) {
		char *tmp;

		vasprintf(&tmp, fmt, ap);
		state->error = tmp;
	}
	if(!state->error_label) {
		state->error_label = state->id->label;
		state->error_lineno = state->id->lineno;
	}
}

void error(struct parser_state *state, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	verror(state, fmt, ap);
	va_end(ap);
}

void warn(struct parser_state *state, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
#ifdef STANDALONE
	vprintf(fmt, ap);
	putchar('\n');
#else
	verror(state, fmt, ap);
#endif
	va_end(ap);
}

static char printable_char(unsigned char c)
{
	return c < ' ' || c > '~' ? '?' : c;
}

/*
 * Since operators don't set "label" properly in unique(), we can't just print
 * the whole string, but need to cut it at the first non-printable character.
 */

static int printable_label(const char *s)
{
	const char *p;

	if(isalnum(*s)) {
		for(p = s; isalnum(*p); p++);
	} else {
		for(p = s; *p > ' '; p++);
	}
	return p-s;
}

const char *parse(const char *expr, int start_token, struct parser_comm *comm)
{
	struct scanner *s;
	struct parser_state state = {
		.comm = comm,
		.assign = comm->assign_default,
		.success = 0,
		.error = NULL,
		.error_label = NULL,
		.id = NULL,
		.style = unknown_style,
	};
	int tok;
	struct id *identifier;
	void *p;
	char *error = NULL;

	s = new_scanner((unsigned char *)expr);
	p = ParseAlloc(malloc);
	Parse(p, start_token, NULL, &state);
	tok = scan(s);
	while(tok != TOK_EOF) {
		if(tok == TOK_ERROR) {
			asprintf(&error,
			    "line %d: scan error near '%c'",
			    s->lineno, printable_char(s->cursor[-1]));
			ParseFree(p, free);
			delete_scanner(s);
			return error;
		}

		identifier = malloc(sizeof(struct id));
		identifier->token = tok;
		identifier->lineno = s->lineno;

		switch(tok) {
		case TOK_CONSTANT:
			identifier->constant = get_constant(s);
			identifier->label = "";
			break;
		case TOK_FNAME:
			identifier->fname = get_name(s);
			identifier->label = identifier->fname;
			break;
		case TOK_IDENT:
			identifier->sym = get_symbol(s);
			identifier->label = identifier->sym->fpvm_sym.name;
			break;
		default:
			identifier->label = (const char *) s->old_cursor;
			break;
		}

		state.id = identifier;
		Parse(p, tok, identifier, &state);
		tok = scan(s);
	}

	identifier = malloc(sizeof(struct id));
	identifier->token = TOK_EOF;
	identifier->lineno = s->lineno;
	identifier->label = "EOF";

	state.id = identifier;
	Parse(p, TOK_EOF, NULL, &state);
	ParseFree(p, free);
	delete_scanner(s);

	free(identifier);

	if(!state.success)
		asprintf(&error,
		    "line %d: %s near '%.*s'",
		    state.error_lineno,
		    state.error ? state.error : "parse error",
		    printable_label(state.error_label), state.error_label);
	if(state.success && state.error)
		asprintf(&error, "line %d: %s",
		    state.error_lineno, state.error);
	free((void *) state.error);

	return error;
}

void parse_free_one(struct ast_node *node)
{
	free(node);
}

void parse_free(struct ast_node *node)
{
	if(node == NULL) return;
	if(node_is_op(node)) {
		parse_free(node->contents.branches.a);
		parse_free(node->contents.branches.b);
		parse_free(node->contents.branches.c);
	}
	free(node);
}
