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

#ifndef __PARSER_ITF_H
#define __PARSER_ITF_H

#include <fpvm/fpvm.h>

#include "parser_helper.h"


#define NDEBUG

struct id {
	int token;
	const char *label;
	struct sym *sym;
	const char *fname;
	float constant;
	int lineno;
};

struct parser_state {
	int success;
	struct parser_comm *comm;
	const char *(*assign)(struct parser_comm *comm,
            struct sym *sym, struct ast_node *node);

	const char *msg;	/* malloc'ed diagnostic message or NULL */
	const char *msg_label;	/* details about the failing token */
	int msg_lineno;
	int is_error;		/* non-zero if error (not just warning) */

	const struct id *id;	/* input, for error handling */

	enum {
		unknown_style,	/* haven't seen any fragment selection yet */
		old_style,	/* patch uses per_frame=var=expr */
		new_style,	/* patch uses per_frame: var=expr ... */
	} style; 
};

extern int syntax_is_new_style;

void *ParseAlloc(void *(*mallocProc)(size_t));
void ParseFree(void *p, void (*freeProc)(void*));
void Parse(void *yyp, int yymajor, struct id *yyminor,
    struct parser_state *state);

#endif /* __PARSER_ITF_H */
