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
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <rtems.h>
#include <bsp/milkymist_pfpu.h>
#include <bsp/milkymist_tmu.h>

#include "../pixbuf/pixbuf.h"
#include "../compiler/compiler.h"
#include "framedescriptor.h"
#include "renderer.h"

#include "eval.h"

static float read_pfv(struct patch *p, int pfv)
{
	if(p->pfv_allocation[pfv] < 0)
		return p->pfv_initial[pfv];
	else
		return p->perframe_regs[p->pfv_allocation[pfv]];
}

static void write_pfv(struct patch *p, int pfv, float x)
{
	if(p->pfv_allocation[pfv] >= 0)
		p->perframe_regs[p->pfv_allocation[pfv]] = x;
	else
		p->pfv_initial[pfv] = x;
}

static void write_pvv(struct patch *p, int pvv, float x)
{
	if(p->pvv_allocation[pvv] >= 0)
		p->pervertex_regs[p->pvv_allocation[pvv]] = x;
}

static void transfer_pvv_regs(struct patch *p)
{
	write_pvv(p, pvv_texsize, renderer_texsize << TMU_FIXEDPOINT_SHIFT);
	write_pvv(p, pvv_hmeshsize, 1.0/(float)renderer_hmeshlast);
	write_pvv(p, pvv_vmeshsize, 1.0/(float)renderer_vmeshlast);

	write_pvv(p, pvv_sx, read_pfv(p, pfv_sx));
	write_pvv(p, pvv_sy, read_pfv(p, pfv_sy));
	write_pvv(p, pvv_cx, read_pfv(p, pfv_cx));
	write_pvv(p, pvv_cy, read_pfv(p, pfv_cy));
	write_pvv(p, pvv_rot, read_pfv(p, pfv_rot));
	write_pvv(p, pvv_dx, read_pfv(p, pfv_dx));
	write_pvv(p, pvv_dy, read_pfv(p, pfv_dy));
	write_pvv(p, pvv_zoom, read_pfv(p, pfv_zoom));

	write_pvv(p, pvv_time, read_pfv(p, pfv_time));
	write_pvv(p, pvv_frame, read_pfv(p, pfv_frame));
	write_pvv(p, pvv_bass, read_pfv(p, pfv_bass));
	write_pvv(p, pvv_mid, read_pfv(p, pfv_mid));
	write_pvv(p, pvv_treb, read_pfv(p, pfv_treb));
	write_pvv(p, pvv_bass_att, read_pfv(p, pfv_bass_att));
	write_pvv(p, pvv_mid_att, read_pfv(p, pfv_mid_att));
	write_pvv(p, pvv_treb_att, read_pfv(p, pfv_treb_att));

	write_pvv(p, pvv_warp, read_pfv(p, pfv_warp));
	write_pvv(p, pvv_warp_anim_speed, read_pfv(p, pfv_warp_anim_speed));
	write_pvv(p, pvv_warp_scale, read_pfv(p, pfv_warp_scale));

	write_pvv(p, pvv_q1, read_pfv(p, pfv_q1));
	write_pvv(p, pvv_q2, read_pfv(p, pfv_q2));
	write_pvv(p, pvv_q3, read_pfv(p, pfv_q3));
	write_pvv(p, pvv_q4, read_pfv(p, pfv_q4));
	write_pvv(p, pvv_q5, read_pfv(p, pfv_q5));
	write_pvv(p, pvv_q6, read_pfv(p, pfv_q6));
	write_pvv(p, pvv_q7, read_pfv(p, pfv_q7));
	write_pvv(p, pvv_q8, read_pfv(p, pfv_q8));

	write_pvv(p, pvv_idmx1, read_pfv(p, pfv_idmx1));
	write_pvv(p, pvv_idmx2, read_pfv(p, pfv_idmx2));
	write_pvv(p, pvv_idmx3, read_pfv(p, pfv_idmx3));
	write_pvv(p, pvv_idmx4, read_pfv(p, pfv_idmx4));
	write_pvv(p, pvv_idmx5, read_pfv(p, pfv_idmx5));
	write_pvv(p, pvv_idmx6, read_pfv(p, pfv_idmx6));
	write_pvv(p, pvv_idmx7, read_pfv(p, pfv_idmx7));
	write_pvv(p, pvv_idmx8, read_pfv(p, pfv_idmx8));

	write_pvv(p, pvv_osc1, read_pfv(p, pfv_osc1));
	write_pvv(p, pvv_osc2, read_pfv(p, pfv_osc2));
	write_pvv(p, pvv_osc3, read_pfv(p, pfv_osc3));
	write_pvv(p, pvv_osc4, read_pfv(p, pfv_osc4));
	
	write_pvv(p, pvv_midi1, read_pfv(p, pfv_midi1));
	write_pvv(p, pvv_midi2, read_pfv(p, pfv_midi2));
	write_pvv(p, pvv_midi3, read_pfv(p, pfv_midi3));
	write_pvv(p, pvv_midi4, read_pfv(p, pfv_midi4));
	write_pvv(p, pvv_midi5, read_pfv(p, pfv_midi5));
	write_pvv(p, pvv_midi6, read_pfv(p, pfv_midi6));
	write_pvv(p, pvv_midi7, read_pfv(p, pfv_midi7));
	write_pvv(p, pvv_midi8, read_pfv(p, pfv_midi8));
}

static void reinit_pfv(struct patch *p, int pfv)
{
	int r;

	r = p->pfv_allocation[pfv];
	if(r < 0) return;
	p->perframe_regs[r] = p->pfv_initial[pfv];
}

static void reinit_all_pfv(struct patch *p)
{
	int i;

	for(i=0;i<COMP_PFV_COUNT;i++)
		reinit_pfv(p, i);
}

static void set_pfv_from_frd(struct patch *p, struct frame_descriptor *frd)
{
	write_pfv(p, pfv_time, frd->time);
	write_pfv(p, pfv_frame, frd->frame);
	write_pfv(p, pfv_bass, frd->bass);
	write_pfv(p, pfv_mid, frd->mid);
	write_pfv(p, pfv_treb, frd->treb);
	write_pfv(p, pfv_bass_att, frd->bass_att);
	write_pfv(p, pfv_mid_att, frd->mid_att);
	write_pfv(p, pfv_treb_att, frd->treb_att);

	write_pfv(p, pfv_idmx1, frd->idmx[0]);
	write_pfv(p, pfv_idmx2, frd->idmx[1]);
	write_pfv(p, pfv_idmx3, frd->idmx[2]);
	write_pfv(p, pfv_idmx4, frd->idmx[3]);
	write_pfv(p, pfv_idmx5, frd->idmx[4]);
	write_pfv(p, pfv_idmx6, frd->idmx[5]);
	write_pfv(p, pfv_idmx7, frd->idmx[6]);
	write_pfv(p, pfv_idmx8, frd->idmx[7]);

	write_pfv(p, pfv_osc1, frd->osc[0]);
	write_pfv(p, pfv_osc2, frd->osc[1]);
	write_pfv(p, pfv_osc3, frd->osc[2]);
	write_pfv(p, pfv_osc4, frd->osc[3]);
	
	write_pfv(p, pfv_midi1, frd->midi[0]);
	write_pfv(p, pfv_midi2, frd->midi[1]);
	write_pfv(p, pfv_midi3, frd->midi[2]);
	write_pfv(p, pfv_midi4, frd->midi[3]);
	write_pfv(p, pfv_midi5, frd->midi[4]);
	write_pfv(p, pfv_midi6, frd->midi[5]);
	write_pfv(p, pfv_midi7, frd->midi[6]);
	write_pfv(p, pfv_midi8, frd->midi[7]);
}

static void set_frd_from_pfv(struct patch *p, struct frame_descriptor *frd)
{
	frd->decay = read_pfv(p, pfv_decay);

	frd->wave_mode = read_pfv(p, pfv_wave_mode);
	frd->wave_scale = read_pfv(p, pfv_wave_scale);
	frd->wave_additive = read_pfv(p, pfv_wave_additive);
	frd->wave_usedots = read_pfv(p, pfv_wave_usedots);
	frd->wave_brighten = read_pfv(p, pfv_wave_brighten);
	frd->wave_thick = read_pfv(p, pfv_wave_thick);

	frd->wave_x = read_pfv(p, pfv_wave_x);
	frd->wave_y = 1.0 - read_pfv(p, pfv_wave_y);
	frd->wave_r = read_pfv(p, pfv_wave_r);
	frd->wave_g = read_pfv(p, pfv_wave_g);
	frd->wave_b = read_pfv(p, pfv_wave_b);
	frd->wave_a = read_pfv(p, pfv_wave_a);

	frd->ob_size = read_pfv(p, pfv_ob_size);
	frd->ob_r = read_pfv(p, pfv_ob_r);
	frd->ob_g = read_pfv(p, pfv_ob_g);
	frd->ob_b = read_pfv(p, pfv_ob_b);
	frd->ob_a = read_pfv(p, pfv_ob_a);

	frd->ib_size = read_pfv(p, pfv_ib_size);
	frd->ib_r = read_pfv(p, pfv_ib_r);
	frd->ib_g = read_pfv(p, pfv_ib_g);
	frd->ib_b = read_pfv(p, pfv_ib_b);
	frd->ib_a = read_pfv(p, pfv_ib_a);

	frd->mv_x = read_pfv(p, pfv_mv_x);
	frd->mv_y = read_pfv(p, pfv_mv_y);
	frd->mv_dx = read_pfv(p, pfv_mv_dx);
	frd->mv_dy = read_pfv(p, pfv_mv_dy);
	frd->mv_l = read_pfv(p, pfv_mv_l);
	frd->mv_r = read_pfv(p, pfv_mv_r);
	frd->mv_g = read_pfv(p, pfv_mv_g);
	frd->mv_b = read_pfv(p, pfv_mv_b);
	frd->mv_a = read_pfv(p, pfv_mv_a);

	frd->tex_wrap = read_pfv(p, pfv_tex_wrap);

	frd->vecho_alpha = read_pfv(p, pfv_video_echo_alpha);
	frd->vecho_zoom = read_pfv(p, pfv_video_echo_zoom);
	frd->vecho_orientation = read_pfv(p, pfv_video_echo_orientation);

	frd->dmx[0] = read_pfv(p, pfv_dmx1);
	frd->dmx[1] = read_pfv(p, pfv_dmx2);
	frd->dmx[2] = read_pfv(p, pfv_dmx3);
	frd->dmx[3] = read_pfv(p, pfv_dmx4);
	frd->dmx[4] = read_pfv(p, pfv_dmx5);
	frd->dmx[5] = read_pfv(p, pfv_dmx6);
	frd->dmx[6] = read_pfv(p, pfv_dmx7);
	frd->dmx[7] = read_pfv(p, pfv_dmx8);

	frd->video_a = read_pfv(p, pfv_video_a);
	
	frd->image_a[0] = read_pfv(p, pfv_image1_a);
	frd->image_x[0] = read_pfv(p, pfv_image1_x);
	frd->image_y[0] = read_pfv(p, pfv_image1_y);
	frd->image_zoom[0] = read_pfv(p, pfv_image1_zoom);
	frd->image_index[0] = read_pfv(p, pfv_image1_index);
	frd->image_a[1] = read_pfv(p, pfv_image2_a);
	frd->image_x[1] = read_pfv(p, pfv_image2_x);
	frd->image_y[1] = read_pfv(p, pfv_image2_y);
	frd->image_zoom[1] = read_pfv(p, pfv_image2_zoom);
	frd->image_index[1] = read_pfv(p, pfv_image2_index);
}

static unsigned int pfpudummy[2] __attribute__((aligned(sizeof(struct tmu_vertex))));

static void eval_pfv(struct patch *p, int fd)
{
	struct pfpu_td td;

	td.output = pfpudummy;
	td.hmeshlast = 0;
	td.vmeshlast = 0;
	td.program = p->perframe_prog;
	td.progsize = p->perframe_prog_length;
	td.registers = p->perframe_regs;
	td.update = true;
	td.invalidate = false;

	ioctl(fd, PFPU_EXECUTE, &td);
}

static void eval_pvv(struct patch *p, struct tmu_vertex *output, int fd)
{
	struct pfpu_td td;

	td.output = (unsigned int *)output;
	td.hmeshlast = renderer_hmeshlast;
	td.vmeshlast = renderer_vmeshlast;
	td.program = p->pervertex_prog;
	td.progsize = p->pervertex_prog_length;
	td.registers = p->pervertex_regs;
	td.update = false;
	/* We send the vertices directly to the TMU.
	 * WARNING: set invalidate to true when printing them for debugging!
	 */
	td.invalidate = false;

	ioctl(fd, PFPU_EXECUTE, &td);
}

static rtems_id eval_q;
static rtems_id eval_terminated;

static rtems_task eval_task(rtems_task_argument argument)
{
	frd_callback callback = (frd_callback)argument;
	struct frame_descriptor *frd;
	size_t s;
	int pfpu_fd;

	pfpu_fd = open("/dev/pfpu", O_RDWR);
	if(pfpu_fd == -1) {
		perror("Unable to open PFPU device");
		goto end;
	}

	while(1) {
		struct patch *p;
		int i;

		rtems_message_queue_receive(
			eval_q,
			&frd,
			&s,
			RTEMS_WAIT,
			RTEMS_NO_TIMEOUT
		);
		/* Task termination is requested by sending a NULL frd */
		if(frd == NULL)
			break;
		assert(frd->status == FRD_STATUS_SAMPLED);

		renderer_lock_patch();

		p = renderer_get_patch(1);
		
		/* NB: we do not increment reference count and assume pixbufs
		 * will be valid until the renderer has fully stopped.
		 */
		for(i=0;i<IMAGE_COUNT;i++) {
			int n = frd->image_index[i];

			frd->images[i] = n >= 0 && n < p->n_images ?
			    p->images[n].pixbuf : NULL;
		}
		
		reinit_all_pfv(p);
		set_pfv_from_frd(p, frd);
		eval_pfv(p, pfpu_fd);
		set_frd_from_pfv(p, frd);
		transfer_pvv_regs(p);
		eval_pvv(p, frd->vertices, pfpu_fd);

		renderer_unlock_patch();

		frd->status = FRD_STATUS_EVALUATED;
		callback(frd);
	}

	close(pfpu_fd);

end:
	rtems_semaphore_release(eval_terminated);
	rtems_task_delete(RTEMS_SELF);
}

static rtems_id eval_task_id;

void eval_start(frd_callback callback)
{
	rtems_status_code sc;

	sc = rtems_message_queue_create(
		rtems_build_name('E', 'V', 'A', 'L'),
		FRD_COUNT,
		sizeof(void *),
		0,
		&eval_q);
	assert(sc == RTEMS_SUCCESSFUL);

	sc = rtems_semaphore_create(
		rtems_build_name('E', 'V', 'A', 'L'),
		0,
		RTEMS_SIMPLE_BINARY_SEMAPHORE,
		0,
		&eval_terminated);
	assert(sc == RTEMS_SUCCESSFUL);

	sc = rtems_task_create(rtems_build_name('E', 'V', 'A', 'L'), 10, RTEMS_MINIMUM_STACK_SIZE,
		RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR,
		0, &eval_task_id);
	assert(sc == RTEMS_SUCCESSFUL);
	sc = rtems_task_start(eval_task_id, eval_task, (rtems_task_argument)callback);
	assert(sc == RTEMS_SUCCESSFUL);
}

void eval_input(struct frame_descriptor *frd)
{
	rtems_message_queue_send(eval_q, &frd, sizeof(void *));
}

void eval_stop(void)
{
	void *dummy;

	dummy = NULL;
	rtems_message_queue_send(eval_q, &dummy, sizeof(void *));

	rtems_semaphore_obtain(eval_terminated, RTEMS_WAIT, RTEMS_NO_TIMEOUT);

	/* task self-deleted */
	rtems_semaphore_delete(eval_terminated);
	rtems_message_queue_delete(eval_q);
}
