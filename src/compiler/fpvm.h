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

/*
 * Floating Point Virtual Machine compiler.
 * This library takes a series of equations and turn them into
 * FPVM code that evaluates them.
 */

#ifndef __FPVM_H
#define __FPVM_H

#include <fpvm/fpvm.h>


void fpvm_init(struct fpvm_fragment *fragment, int vector_mode);

int fpvm_chunk(struct fpvm_fragment *fragment, const char *chunk);

#endif /* __FPVM_H */
