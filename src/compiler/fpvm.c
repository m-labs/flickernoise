/*
 * Milkymist SoC (Software)
 * Copyright (C) 2007, 2008, 2009, 2010, 2011 Sebastien Bourdeauducq
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fpvm/fpvm.h>
#include <fpvm/ast.h>

#include "unique.h"
#include "parser.h"
#include "parser_helper.h"
#include "fpvm.h"


void fpvm_init(struct fpvm_fragment *fragment, int vector_mode)
{
	/*
	 * We need to pass these through unique() because fpvm_assign does
	 * the same. Once things are in Flickernoise, we can get rid of these
	 * calls to unique().
	 */

	_Xi = unique("_Xi");
	_Xo = unique("_Xo");
	_Yi = unique("_Yi");
	_Yo = unique("_Yo");
	fpvm_do_init(fragment, vector_mode);
}


static const char *assign_default(struct parser_comm *comm,
    const char *label, struct ast_node *node)
{
	if(fpvm_do_assign(comm->u.fragment, label, node))
		return NULL;
	else
		return strdup(fpvm_get_last_error(comm->u.fragment));
}


static const char *assign_unsupported(struct parser_comm *comm,
    const char *label, struct ast_node *node)
{
	return strdup("assignment mode not supported yet");
}


int fpvm_chunk(struct fpvm_fragment *fragment, const char *chunk)
{
	struct parser_comm comm = {
		.u.fragment = fragment,
		.assign_default = assign_default,
		.assign_per_frame = assign_unsupported,
		.assign_per_vertex = assign_unsupported,
	};
	const char *error;

	error = fpvm_parse(chunk, TOK_START_ASSIGN, &comm);
	if(error) {
		snprintf(fragment->last_error, FPVM_MAXERRLEN, "%s", error);
		free((void *) error);
	}
	return !error;
}
