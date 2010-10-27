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

#ifndef __RESMGR_H
#define __RESMGR_H

enum {
	RESOURCE_AUDIO = 0,
	RESOURCE_DMX_IN,
	RESOURCE_DMX_OUT,
	RESOURCE_MIDI,
	RESOURCE_VIDEOIN,
	RESOURCE_SAMPLER,

	N_RESOURCE, /* must be immediately before last */
	INVALID_RESOURCE /* must be last */
};

#define RESOURCE_HOLDER_MAXLEN 32

int resmgr_acquire(const char *holder, int resource);
int resmgr_acquire_multiple(const char *holder, ...);
void resmgr_release(int resource);

#endif /* __RESMGR_H */
