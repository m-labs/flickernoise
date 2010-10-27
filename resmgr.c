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

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "messagebox.h"

#include "resmgr.h"

static char holders[N_RESOURCE][RESOURCE_HOLDER_MAXLEN];

static const char *getresname(int resource)
{
	switch(resource) {
		case RESOURCE_AUDIO: return "Audio";
		case RESOURCE_DMX_IN: return "DMX input";
		case RESOURCE_DMX_OUT: return "DMX output";
		case RESOURCE_MIDI: return "MIDI";
		case RESOURCE_VIDEOIN: return "Video input";
		case RESOURCE_SAMPLER: return "Sampler";
		default: return "<unknown>";
	}
}

int resmgr_acquire(const char *holder, int resource)
{
	if(holders[resource][0] != 0) {
		char msg[256];

		sprintf(msg, "%s is already used by %s.\n", getresname(resource), holders[resource]);
		messagebox("Error", msg);
		return 0;
	}
	strcpy(holders[resource], holder);
	return 1;
}

int resmgr_acquire_multiple(const char *holder, ...)
{
	va_list aq;
	int modified[N_RESOURCE];
	int i;
	int r;

	for(i=0;i<N_RESOURCE;i++)
		modified[i] = 0;

	r = 1;
	va_start(aq, holder);
	while(1) {
		int res;

		res = va_arg(aq, int);
		if(res == INVALID_RESOURCE)
			break;
		if(!resmgr_acquire(holder, res)) {
			for(i=0;i<N_RESOURCE;i++)
				if(modified[i])
					resmgr_release(i);
			r = 0;
			break;
		}
		modified[res] = 1;
	}
	va_end(aq);
	return r;
}

void resmgr_release(int resource)
{
	holders[resource][0] = 0;
}
