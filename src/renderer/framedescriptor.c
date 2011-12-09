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

#include <stdlib.h>
#include <rtems.h>
#include <bsp/milkymist_tmu.h>

#include "framedescriptor.h"

struct frame_descriptor *new_frame_descriptor(void)
{
	struct frame_descriptor *frd;

	frd = malloc(sizeof(struct frame_descriptor));
	if(frd == NULL)
		return NULL;

	frd->status = FRD_STATUS_NEW;

	frd->snd_buf = malloc(sizeof(struct snd_buffer)+4*FRD_AUDIO_NSAMPLES);
	if(frd->snd_buf == NULL) {
		free(frd);
		return NULL;
	}
	frd->snd_buf->nsamples = FRD_AUDIO_NSAMPLES;
	frd->snd_buf->user = frd;

	if(posix_memalign((void **)&frd->vertices, sizeof(struct tmu_vertex),
		sizeof(struct tmu_vertex)*TMU_MESH_MAXSIZE*TMU_MESH_MAXSIZE) != 0) {
		free(frd->snd_buf);
		free(frd);
		return NULL;
	}

	return frd;
}

void free_frame_descriptor(struct frame_descriptor *frd)
{
	free(frd->vertices);
	free(frd->snd_buf);
	free(frd);
}
