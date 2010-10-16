/*
 * Flickernoise
 * Copyright (C) 2010 Sebastien Bourdeauducq
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

#ifndef __SAMPLER_H
#define __SAMPLER_H

#include "framedescriptor.h"

typedef void (*sampler_callback)(struct frame_descriptor *);

void sampler_start(sampler_callback callback);
void sampler_return(struct frame_descriptor *frd);
void sampler_stop();

#endif /* __SAMPLER_H */
