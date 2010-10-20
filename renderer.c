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

#include <assert.h>
#include <rtems.h>
#include <stdio.h>
#include <stdlib.h>

#include "sampler.h"
#include "compiler.h"
#include "eval.h"

#include "renderer.h"

int renderer_texsize;
int renderer_hmeshlast;
int renderer_vmeshlast;

static struct patch *current_patch;
static rtems_id patch_lock;

void renderer_lock_patch()
{
	rtems_semaphore_obtain(patch_lock, RTEMS_WAIT, RTEMS_NO_TIMEOUT);
}

void renderer_unlock_patch()
{
	rtems_semaphore_release(patch_lock);
}

void renderer_set_patch(struct patch *p)
{
	struct patch *new_patch;
	struct patch *old_patch;

	/* Copy memory as the patch will be modified
	 * by the evaluator (current regs).
	 */

	old_patch = current_patch;
	new_patch = malloc(sizeof(struct patch));
	assert(new_patch != NULL);
	memcpy(new_patch, p, sizeof(struct patch));

	renderer_lock_patch();
	current_patch = new_patch;
	renderer_unlock_patch();

	free(old_patch);
}

struct patch *renderer_get_patch()
{
	return current_patch;
}

static void mycallback(struct frame_descriptor *frd)
{
	static int i;

	if(i++ == 24) {
		printf("bass:%f decay:%f\n", frd->bass, frd->decay);
		i = 0;
	}
	sampler_return(frd);
}

void renderer_start(struct patch *p)
{
	assert(rtems_semaphore_create(
		rtems_build_name('P', 'T', 'C', 'H'),
		1,
		RTEMS_SIMPLE_BINARY_SEMAPHORE,
		0,
		&patch_lock
	) == RTEMS_SUCCESSFUL);

	renderer_set_patch(p);

	renderer_hmeshlast = 32;
	renderer_vmeshlast = 32;
	renderer_texsize = 512;

	eval_start(mycallback);
	sampler_start(eval_input);
}

void renderer_stop()
{
	sampler_stop();
	eval_stop();

	free(current_patch);
	current_patch = NULL;
	rtems_semaphore_delete(patch_lock);
}
