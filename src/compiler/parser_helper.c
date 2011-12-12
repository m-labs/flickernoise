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

#include <stdio.h>
#include <malloc.h>
#include <fpvm/ast.h>

#include "scanner.h"
#include "parser.h"
#include "parser_itf.h"
#include "parser_helper.h"

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

	for(p = s; *p > ' '; p++);
	return p-s;
}


int fpvm_parse(const char *expr, int start_token, union parser_comm *comm)
{
	struct scanner *s;
	struct parser_state state = {
		.comm = comm,
		.success = 0,
		.error_label = NULL,
		.id = NULL,
	};
	int tok;
	struct id *identifier;
	void *p;

	s = new_scanner((unsigned char *)expr);
	p = ParseAlloc(malloc);
	Parse(p, start_token, NULL, &state);
	tok = scan(s);
	while(tok != TOK_EOF) {
		identifier = malloc(sizeof(struct id));
		identifier->token = tok;
		identifier->lineno = s->lineno;
		if(tok == TOK_CONSTANT) {
			identifier->constant = get_constant(s);
			identifier->label = "";
		} else {
			identifier->label = get_token(s);
		}
		state.id = identifier;
		if(tok == TOK_ERROR) {
			printf("FPVM, line %d, near \"%c\": scan error\n",
			    s->lineno, printable_char(s->cursor[-1]));
			ParseFree(p, free);
			delete_scanner(s);
			return 0;
		}
		Parse(p, tok, identifier, &state);
		tok = scan(s);
	}
	Parse(p, TOK_EOF, NULL, &state);
	ParseFree(p, free);
	delete_scanner(s);

	if(!state.success)
		printf("FPVM, line %d, near \"%.*s\": parse error\n",
		    state.error_lineno, printable_label(state.error_label),
		    state.error_label);

	return state.success;
}

void fpvm_parse_free(struct ast_node *node)
{
	if(node == NULL) return;
	if(node->label[0] != 0) {
		fpvm_parse_free(node->contents.branches.a);
		fpvm_parse_free(node->contents.branches.b);
		fpvm_parse_free(node->contents.branches.c);
	}
	free(node);
}
