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

#ifndef __FRAMEDESCRIPTOR_H
#define __FRAMEDESCRIPTOR_H

#include <rtems.h>
#include <bsp/milkymist_ac97.h>

#define FRD_AUDIO_NSAMPLES (48000/25)

enum {
	FRD_STATUS_NEW = 0,
	FRD_STATUS_SAMPLING,
	FRD_STATUS_SAMPLED
};

struct frame_descriptor {
	int status;
	struct snd_buffer *snd_buf;
	float bass, mid, treb;
	float bass_att, mid_att, treb_att;
};

struct frame_descriptor *new_frame_descriptor();
void free_frame_descriptor(struct frame_descriptor *frd);

#endif /* __FRAMEDESCRIPTOR_H */
