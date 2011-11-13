/*
 * Flickernoise
 * Copyright (C) 2010, 2011 Sebastien Bourdeauducq
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
#include "raster.h"
#include "osd.h"
#include "rsswall.h"

#include "renderer.h"

int renderer_texsize;
int renderer_hmeshlast;
int renderer_vmeshlast;
int renderer_squarew;
int renderer_squareh;

static int mashup_en;
static struct patch *mashup_head;
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

static struct patch *copy_patch(struct patch *p)
{
	struct patch *new_patch;
	
	/* Copy memory as the patch will be modified
	 * by the evaluator (current regs).
	 */
	new_patch = malloc(sizeof(struct patch));
	assert(new_patch != NULL);
	memcpy(new_patch, p, sizeof(struct patch));
	new_patch->original = p;
	new_patch->next = NULL;
	return new_patch;
}

void renderer_pulse_patch(struct patch *p)
{
	struct patch *oldpatch;
	
	renderer_lock_patch();
	if(!mashup_en && (mashup_head->next == NULL)) {
		oldpatch = mashup_head;
		mashup_head = copy_patch(p);
		current_patch = mashup_head;
		free(oldpatch);
	}
	renderer_unlock_patch();
}

void renderer_add_patch(struct patch *p)
{
	struct patch *p1;
	struct patch *new_patch;
	
	renderer_lock_patch();
	p1 = mashup_head;
	while(p1 != NULL) {
		if(p1->original == p) {
			renderer_unlock_patch();
			return;
		}
		p1 = p1->next;
	}
	new_patch = copy_patch(p);
	if(!mashup_en) {
		p1 = mashup_head;
		mashup_head = new_patch;
		current_patch = mashup_head;
		free(p1);
	} else {
		new_patch->next = mashup_head;
		mashup_head = new_patch;
	}
	mashup_en++;
	renderer_unlock_patch();
}

void renderer_del_patch(struct patch *p)
{
	struct patch *p1, *p2;
	
	renderer_lock_patch();
	mashup_en--;
	if(mashup_en < 0)
		mashup_en = 0;
	/* Never delete the only patch */
	if(mashup_head->next == NULL) {
		renderer_unlock_patch();
		return;
	}
	if(mashup_head->original == p) {
		if(mashup_head == current_patch)
			renderer_get_patch(1);
		p1 = mashup_head->next;
		free(mashup_head);
		mashup_head = p1;
	} else {
		p1 = mashup_head;
		while((p1->next != NULL) && (p1->next->original != p))
			p1 = p1->next;
		if(p1->next == NULL) {
			/* not found */
			renderer_unlock_patch();
			return;
		}
		if(p1->next == current_patch)
			renderer_get_patch(1);
		p2 = p1->next->next;
		free(p1->next);
		p1->next = p2;
	}
	renderer_unlock_patch();
}

struct patch *renderer_get_patch(int spin)
{
	if(spin) {
		if(current_patch->next == NULL)
			current_patch = mashup_head;
		else
			current_patch = current_patch->next;
	}
	return current_patch;
}

void renderer_start(int framebuffer_fd, struct patch *p)
{
	rtems_status_code sc;

	sc = rtems_semaphore_create(
		rtems_build_name('P', 'T', 'C', 'H'),
		1,
		RTEMS_SIMPLE_BINARY_SEMAPHORE,
		0,
		&patch_lock
	);
	assert(sc == RTEMS_SUCCESSFUL);

	assert(mashup_head == NULL);
	mashup_head = copy_patch(p);
	current_patch = mashup_head;
	mashup_en = 0;

	renderer_texsize = 512;
	renderer_hmeshlast = 32;
	renderer_vmeshlast = 32;
	renderer_squarew = renderer_texsize/renderer_hmeshlast;
	renderer_squareh = renderer_texsize/renderer_vmeshlast;

	osd_init();
	raster_start(framebuffer_fd, sampler_return);
	eval_start(raster_input);
	sampler_start(eval_input);
	rsswall_start();
}

void renderer_stop()
{
	struct patch *p;
	
	rsswall_stop();
	sampler_stop();
	eval_stop();
	raster_stop();

	while(mashup_head != NULL) {
		p = mashup_head->next;
		free(mashup_head);
		mashup_head = p;
	}
	rtems_semaphore_delete(patch_lock);
}
