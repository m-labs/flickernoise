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

#ifndef __PARSER_HELPER_H
#define __PARSER_HELPER_H

#include <stdarg.h>

#include <fpvm/ast.h>
#include <fpvm/fpvm.h>

#include "symtab.h"


struct compiler_sc;
struct parser_state;

struct parser_comm {
	union {
		struct ast_node *parseout;
		struct fpvm_fragment *fragment;
		struct compiler_sc *sc;
	} u;
	const char *(*assign_default)(struct parser_comm *comm,
	    struct sym *sym, struct ast_node *node);
	const char *(*assign_per_frame)(struct parser_comm *comm,
	    struct sym *sym, struct ast_node *node);
	const char *(*assign_per_vertex)(struct parser_comm *comm,
	    struct sym *sym, struct ast_node *node);
	const char *(*assign_image_name)(struct parser_comm *comm,
	    int number, const char *name);
	const char *msg; /* NULL if neither error nor warning */
};

extern int warn_section;
extern int warn_undefined;

void error(struct parser_state *state, const char *fmt, ...);
void warn(struct parser_state *state, const char *fmt, ...);

int parse(const char *expr, int start_token, struct parser_comm *comm);
void parse_free_one(struct ast_node *node);
void parse_free(struct ast_node *node);

#endif /* __PARSER_HELPER_H */
