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

#ifndef __RENDERER_H
#define __RENDERER_H

#include "../compiler/compiler.h"

extern int renderer_texsize;
extern int renderer_hmeshlast;
extern int renderer_vmeshlast;
extern int renderer_squarew;
extern int renderer_squareh;

/*
 * Synchronization:
 * 1. call renderer_lock_patch()
 * 2. call renderer_get_patch()
 * 3. use the returned patch
 * 4. call renderer_unlock_patch()
 */

void renderer_lock_patch(void);
void renderer_unlock_patch(void);

void renderer_pulse_patch(struct patch *p);
void renderer_add_patch(struct patch *p);
void renderer_del_patch(struct patch *p);
struct patch *renderer_get_patch(int spin);

void renderer_start(int framebuffer_fd, struct patch *p);
void renderer_stop(void);

#endif /* __RENDERER_H */
