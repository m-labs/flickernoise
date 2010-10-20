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
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <rtems.h>
#include <rtems/fb.h>
#include <bsp/milkymist_tmu.h>

#include "framedescriptor.h"
#include "renderer.h"

#include "raster.h"

static void get_screen_res(int framebuffer_fd, int *hres, int *vres)
{
	struct fb_var_screeninfo fb_var;

	ioctl(framebuffer_fd, FBIOGET_VSCREENINFO, &fb_var);
	*hres = fb_var.xres;
	*vres = fb_var.yres;
}

static void warp(int tmu_fd, unsigned short *src, unsigned short *dest, struct tmu_vertex *vertices, unsigned int brightness)
{
	struct tmu_td td;

	td.flags = 0;
	td.hmeshlast = renderer_hmeshlast;
	td.vmeshlast = renderer_vmeshlast;
	td.brightness = brightness;
	td.chromakey = 0;
	td.vertices = vertices;
	td.texfbuf = src;
	td.texhres = renderer_texsize;
	td.texvres = renderer_texsize;
	td.texhmask = TMU_MASK_FULL;
	td.texvmask = TMU_MASK_FULL;
	td.dstfbuf = dest;
	td.dsthres = renderer_texsize;
	td.dstvres = renderer_texsize;
	td.dsthoffset = 0;
	td.dstvoffset = 0;
	td.dstsquarew = renderer_squarew;
	td.dstsquareh = renderer_squareh;
	td.alpha = TMU_ALPHA_MAX;
	td.invalidate_before = false;
	td.invalidate_after = true;

	ioctl(tmu_fd, TMU_EXECUTE, &td);
}

static void draw(unsigned short *fb, struct frame_descriptor *frd)
{
}

static unsigned short *get_screen_backbuffer(int framebuffer_fd)
{
	struct fb_fix_screeninfo fb_fix;

	ioctl(framebuffer_fd, FBIOGET_FSCREENINFO, &fb_fix);
	return (unsigned short *)fb_fix.smem_start;
}

static void init_scale_vertices(struct tmu_vertex *vertices)
{
	vertices[0].x = 0;
	vertices[0].y = 0;
	vertices[1].x = renderer_texsize << TMU_FIXEDPOINT_SHIFT;
	vertices[1].y = 0;
	vertices[TMU_MESH_MAXSIZE].x = 0;
	vertices[TMU_MESH_MAXSIZE].y = renderer_texsize << TMU_FIXEDPOINT_SHIFT;
	vertices[TMU_MESH_MAXSIZE+1].x = renderer_texsize << TMU_FIXEDPOINT_SHIFT;
	vertices[TMU_MESH_MAXSIZE+1].y = renderer_texsize << TMU_FIXEDPOINT_SHIFT;
}

static void init_vecho_vertices(struct tmu_vertex *vertices, struct frame_descriptor *frd)
{
	int a, b;

	a = (32.0-32.0/frd->vecho_zoom)*(float)renderer_texsize;
	b = renderer_texsize*64 - a;

	if((frd->vecho_orientation == 1) || (frd->vecho_orientation == 3)) {
		vertices[0].x = b;
		vertices[1].x = a;
		vertices[TMU_MESH_MAXSIZE].x = b;
		vertices[TMU_MESH_MAXSIZE+1].x = a;
	} else {
		vertices[0].x = a;
		vertices[1].x = b;
		vertices[TMU_MESH_MAXSIZE].x = a;
		vertices[TMU_MESH_MAXSIZE+1].x = b;
	}
	if((frd->vecho_orientation == 2) || (frd->vecho_orientation == 3)) {
		vertices[0].y = b;
		vertices[1].y = b;
		vertices[TMU_MESH_MAXSIZE].y = a;
		vertices[TMU_MESH_MAXSIZE+1].y = a;
	} else {
		vertices[0].y = a;
		vertices[1].y = a;
		vertices[TMU_MESH_MAXSIZE].y = b;
		vertices[TMU_MESH_MAXSIZE+1].y = b;
	}
}

static void scale(int tmu_fd, struct tmu_vertex *vertices,
	unsigned short *src, unsigned short *dest,
	int hres, int vres, int alpha, bool invalidate)
{
	struct tmu_td td;

	td.flags = 0;
	td.hmeshlast = 1;
	td.vmeshlast = 1;
	td.brightness = TMU_BRIGHTNESS_MAX;
	td.chromakey = 0;
	td.vertices = vertices;
	td.texfbuf = src;
	td.texhres = renderer_texsize;
	td.texvres = renderer_texsize;
	td.texhmask = TMU_MASK_FULL;
	td.texvmask = TMU_MASK_FULL;
	td.dstfbuf = dest;
	td.dsthres = hres;
	td.dstvres = vres;
	td.dsthoffset = 0;
	td.dstvoffset = 0;
	td.dstsquarew = hres;
	td.dstsquareh = vres;
	td.alpha = alpha;
	td.invalidate_before = invalidate;
	td.invalidate_after = false;

	ioctl(tmu_fd, TMU_EXECUTE, &td);
}

struct raster_task_param {
	int framebuffer_fd;
	frd_callback callback;
};

static rtems_id raster_q;
static rtems_id raster_terminated;

static rtems_task raster_task(rtems_task_argument argument)
{
	struct raster_task_param *param = (struct raster_task_param *)argument;
	struct frame_descriptor *frd;
	size_t s;
	unsigned short *tex_frontbuffer, *tex_backbuffer;
	unsigned short *p;
	struct tmu_vertex *scale_vertices;
	int tmu_fd;
	unsigned short *screen_backbuffer;
	int hres, vres;
	float brightness_error;
	int ibrightness;
	int vecho_alpha;

	assert(posix_memalign((void **)&tex_frontbuffer, 32,
		2*renderer_texsize*renderer_texsize) == 0);
	assert(posix_memalign((void **)&tex_backbuffer, 32,
		2*renderer_texsize*renderer_texsize) == 0);
	memset(tex_frontbuffer, 0, 2*renderer_texsize*renderer_texsize);
	memset(tex_backbuffer, 0, 2*renderer_texsize*renderer_texsize);

	assert(posix_memalign((void **)&scale_vertices, sizeof(struct tmu_vertex),
		sizeof(struct tmu_vertex)*TMU_MESH_MAXSIZE*TMU_MESH_MAXSIZE) == 0);

	tmu_fd = open("/dev/tmu", O_RDWR);
	assert(tmu_fd != -1);

	get_screen_res(param->framebuffer_fd, &hres, &vres);
	ioctl(param->framebuffer_fd, FBIOSETBUFFERMODE, FB_TRIPLE_BUFFERED);

	brightness_error = 0.0;

	while(1) {
		rtems_message_queue_receive(
			raster_q,
			&frd,
			&s,
			RTEMS_WAIT,
			RTEMS_NO_TIMEOUT
		);
		/* Task termination is requested by sending a NULL frd */
		if(frd == NULL)
			break;
		assert(frd->status == FRD_STATUS_EVALUATED);

		/* Update brightness */
		brightness_error += frd->decay;
		ibrightness = 64.0*brightness_error;
		brightness_error -= (float)ibrightness/64.0;
		if(ibrightness > 64) ibrightness = 64;

		/* Compute frame */
		warp(tmu_fd, tex_frontbuffer, tex_backbuffer, frd->vertices, ibrightness-1);
		draw(tex_backbuffer, frd);

		/* Scale and send to screen */
		screen_backbuffer = get_screen_backbuffer(param->framebuffer_fd);
		init_scale_vertices(scale_vertices);
		scale(tmu_fd, scale_vertices, tex_backbuffer, screen_backbuffer, hres, vres, TMU_ALPHA_MAX, true);
		vecho_alpha = 64.0*frd->vecho_alpha;
		vecho_alpha--;
		if(vecho_alpha > TMU_ALPHA_MAX)
			vecho_alpha = TMU_ALPHA_MAX;
		if(vecho_alpha > 0) {
			init_vecho_vertices(scale_vertices, frd);
			scale(tmu_fd, scale_vertices, tex_backbuffer, screen_backbuffer, hres, vres, vecho_alpha, false);
		}
		ioctl(param->framebuffer_fd, FBIOSWAPBUFFERS);

		/* Swap texture buffers */
		p = tex_frontbuffer;
		tex_frontbuffer = tex_backbuffer;
		tex_backbuffer = p;

		frd->status = FRD_STATUS_USED;
		param->callback(frd);
	}

	ioctl(param->framebuffer_fd, FBIOSETBUFFERMODE, FB_SINGLE_BUFFERED);
	close(tmu_fd);
	free(tex_backbuffer);
	free(tex_frontbuffer);
	free(param);
	rtems_semaphore_release(raster_terminated);
	rtems_task_delete(RTEMS_SELF);
}

static rtems_id raster_task_id;

void raster_start(int framebuffer_fd, frd_callback callback)
{
	struct raster_task_param *param;

	assert(rtems_message_queue_create(
		rtems_build_name('R', 'A', 'S', 'T'),
		FRD_COUNT,
		sizeof(void *),
		0,
		&raster_q) == RTEMS_SUCCESSFUL);

	assert(rtems_semaphore_create(
		rtems_build_name('R', 'A', 'S', 'T'),
		0,
		RTEMS_SIMPLE_BINARY_SEMAPHORE,
		0,
		&raster_terminated) == RTEMS_SUCCESSFUL);

	param = malloc(sizeof(struct raster_task_param));
	assert(param != NULL);
	param->framebuffer_fd = framebuffer_fd;
	param->callback = callback;
	assert(rtems_task_create(rtems_build_name('R', 'A', 'S', 'T'), 10, RTEMS_MINIMUM_STACK_SIZE,
		RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR,
		0, &raster_task_id) == RTEMS_SUCCESSFUL);
	assert(rtems_task_start(raster_task_id, raster_task, (rtems_task_argument)param) == RTEMS_SUCCESSFUL);
}

void raster_input(struct frame_descriptor *frd)
{
	rtems_message_queue_send(raster_q, &frd, sizeof(void *));
}

void raster_stop()
{
	void *dummy;

	dummy = NULL;
	rtems_message_queue_send(raster_q, &dummy, sizeof(void *));

	rtems_semaphore_obtain(raster_terminated, RTEMS_WAIT, RTEMS_NO_TIMEOUT);

	/* task self-deleted and freed raster_task_param */
	rtems_semaphore_delete(raster_terminated);
	rtems_message_queue_delete(raster_q);
}
