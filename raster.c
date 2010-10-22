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
#include <math.h>
#include <rtems.h>
#include <rtems/fb.h>
#include <bsp/milkymist_tmu.h>

#include "framedescriptor.h"
#include "renderer.h"
#include "color.h"
#include "wave.h"
#include "line.h"

#include "raster.h"

static void get_screen_res(int framebuffer_fd, int *hres, int *vres)
{
	struct fb_var_screeninfo fb_var;

	ioctl(framebuffer_fd, FBIOGET_VSCREENINFO, &fb_var);
	*hres = fb_var.xres;
	*vres = fb_var.yres;
}

static unsigned int get_tmu_wrap_mask(unsigned int x)
{
	unsigned int s;

	s = 1 << TMU_FIXEDPOINT_SHIFT;
	return (x-1)*s+s-1;
}

static void warp(int tmu_fd, unsigned short *src, unsigned short *dest, struct tmu_vertex *vertices, bool tex_wrap, unsigned int brightness)
{
	struct tmu_td td;
	unsigned int mask;

	if(tex_wrap)
		mask = get_tmu_wrap_mask(renderer_texsize);
	else
		mask = TMU_MASK_FULL;

	td.flags = 0;
	td.hmeshlast = renderer_hmeshlast;
	td.vmeshlast = renderer_vmeshlast;
	td.brightness = brightness;
	td.chromakey = 0;
	td.vertices = vertices;
	td.texfbuf = src;
	td.texhres = renderer_texsize;
	td.texvres = renderer_texsize;
	td.texhmask = mask;
	td.texvmask = mask;
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

	ioctl(tmu_fd, TMU_EXECUTE_NONBLOCK, &td);
}

static void draw_dot(struct line_context *ctx, int x, int y, unsigned int l)
{
	// TODO: make them round and good looking
	l >>= 1;
	hline(ctx, y, x-l, x+l);
}

static void draw_motion_vectors(unsigned short *fb, struct frame_descriptor *frd)
{
	int x, y;
	struct line_context ctx;
	float offsetx;
	float intervalx;
	float offsety;
	float intervaly;
	int nx, ny;
	int alpha, l;
	int px, py;

	alpha = 64.0*frd->mv_a;
	if(alpha == 0) return;
	if(frd->mv_x == 0.0) return;
	if(frd->mv_y == 0.0) return;

	line_init_context(&ctx, fb, renderer_texsize, renderer_texsize);
	ctx.color = float_to_rgb565(frd->mv_r, frd->mv_g, frd->mv_b);
	ctx.alpha = alpha;
	l = frd->mv_l;
	if(l < 1) l = 1;
	if(l > 10) l = 10;
	ctx.thickness = l;

	offsetx = frd->mv_dx*(float)renderer_texsize;
	intervalx = (float)renderer_texsize/frd->mv_x;
	offsety = frd->mv_dy*renderer_texsize;
	intervaly = (float)renderer_texsize/frd->mv_y;

	nx = frd->mv_x+1.5;
	ny = frd->mv_y+1.5;
	for(y=0;y<ny;y++)
		for (x=0;x<nx;x++) {
			px = offsetx+x*intervalx;
			if(px < 0) px = 0;
			if(px >= renderer_texsize) px = renderer_texsize-1;
			py = offsety+y*intervaly;
			if(py < 0) py = 0;
			if(py >= renderer_texsize) py = renderer_texsize-1;
			draw_dot(&ctx, px, py, l);
		}
}

static void border_rect(unsigned short *fb, int x0, int y0, int x1, int y1, short int color, unsigned int alpha)
{
	int y;
	struct line_context ctx;

	line_init_context(&ctx, fb, renderer_texsize, renderer_texsize);
	ctx.color = color;
	ctx.alpha = alpha;
	for(y=y0;y<=y1;y++)
		hline(&ctx, y, x0, x1);
}

static void draw_borders(unsigned short *fb, struct frame_descriptor *frd)
{
	unsigned int of;
	unsigned int iff;
	unsigned int texof;
	short int ob_color, ib_color;
	unsigned int ob_alpha, ib_alpha;
	int cmax;

	of = renderer_texsize*frd->ob_size*.5;
	iff = renderer_texsize*frd->ib_size*.5;

	if(of > 30) of = 30;
	if(iff > 30) iff = 30;

	texof = renderer_texsize-of;
	cmax = renderer_texsize-1;

	ob_alpha = 80.0*frd->ob_a;
	if((of != 0) && (ob_alpha != 0)) {
		ob_color = float_to_rgb565(frd->ob_r, frd->ob_g, frd->ob_b);


		border_rect(fb, 0, 0, of, cmax, ob_color, ob_alpha);
		border_rect(fb, of, 0, texof, of, ob_color, ob_alpha);
		border_rect(fb, texof, 0, cmax, cmax, ob_color, ob_alpha);
		border_rect(fb, of, texof, texof, cmax, ob_color, ob_alpha);
	}

	ib_alpha = 80.0*frd->ib_a;
	if((iff != 0) && (ib_alpha != 0)) {
		ib_color = float_to_rgb565(frd->ib_r, frd->ib_g, frd->ib_b);

		border_rect(fb, of, of, of+iff-1, texof-1, ib_color, ib_alpha);
		border_rect(fb, of+iff, of, texof-iff-1, of+iff-1, ib_color, ib_alpha);
		border_rect(fb, texof-iff, of, texof-1, texof-1, ib_color, ib_alpha);
		border_rect(fb, of+iff, texof-iff, texof-iff-1, texof-1, ib_color, ib_alpha);
	}
}

/* TODO: implement missing wave modes */

static int wave_mode_0(struct frame_descriptor *frd, struct wave_vertex *vertices)
{
	return 0;
}

static int wave_mode_1(struct frame_descriptor *frd, struct wave_vertex *vertices)
{
	return 0;
}

static int wave_mode_23(struct frame_descriptor *frd, struct wave_vertex *vertices)
{
	int nvertices;
	int i;
	float s1, s2;
	short int *samples = (short int *)frd->snd_buf->samples;

	nvertices = 64-32;

	for(i=0;i<nvertices;i++) {
		s1 = samples[8*i     ]/32768.0;
		s2 = samples[8*i+32+1]/32768.0;

		vertices[i].x = (s1*frd->wave_scale*0.5 + frd->wave_x)*renderer_texsize;
		vertices[i].y = (s2*frd->wave_scale*0.5 + frd->wave_x)*renderer_texsize;
	}

	return nvertices;
}

static int wave_mode_4(struct frame_descriptor *frd, struct wave_vertex *vertices)
{
	int nvertices;
	float wave_x;
	int i;
	float dy_adj;
	float s1, s2;
	float scale;
	short int *samples = (short int *)frd->snd_buf->samples;

	nvertices = 64;

	// TODO: rotate using wave_mystery
	wave_x = frd->wave_x*.75 + .125;
	scale = 4.0*(float)renderer_texsize/505.0;

	for(i=1;i<=nvertices;i++) {
		s1 = samples[8*i]/32768.0;
		s2 = samples[8*i-2]/32768.0;

		dy_adj = s1*20.0*frd->wave_scale-s2*20.0*frd->wave_scale;
		// nb: x and y reversed to simulate default rotation from wave_mystery
		vertices[i-1].y = s1*20.0*frd->wave_scale+(float)renderer_texsize*frd->wave_x;
		vertices[i-1].x = (i*scale)+dy_adj;
	}

	return nvertices;
}

static int wave_mode_5(struct frame_descriptor *frd, struct wave_vertex *vertices)
{
	int nvertices;
	int i;
	float s1, s2;
	float x0, y0;
	float cos_rot, sin_rot;
	short int *samples = (short int *)frd->snd_buf->samples;

	nvertices = 64-32;

	cos_rot = cosf(frd->time*0.3);
	sin_rot = sinf(frd->time*0.3);

	for(i=0;i<nvertices;i++) {
		s1 = samples[8*i     ]/32768.0;
		s2 = samples[8*i+64+1]/32768.0;
		x0 = 2.0*s1*s2;
		y0 = s1*s1 - s2*s2;

		vertices[i].x = (float)renderer_texsize*((x0*cos_rot - y0*sin_rot)*frd->wave_scale*0.5 + frd->wave_x);
		vertices[i].y = (float)renderer_texsize*((x0*sin_rot + y0*cos_rot)*frd->wave_scale*0.5 + frd->wave_y);
	}

	return nvertices;
}

static int wave_mode_6(struct frame_descriptor *frd, struct wave_vertex *vertices)
{
	int nvertices;
	int i;
	float inc;
	float offset;
	float s;
	short int *samples = (short int *)frd->snd_buf->samples;

	nvertices = 64;

	// TODO: rotate/scale by wave_mystery

	inc = (float)renderer_texsize/(float)nvertices;
	offset = (float)renderer_texsize*(1.0-frd->wave_x);
	for(i=0;i<nvertices;i++) {
		s = samples[8*i]/32768.0;
		// nb: x and y reversed to simulate default rotation from wave_mystery
		vertices[i].y = s*20.0*frd->wave_scale+offset;
		vertices[i].x = i*inc;
	}

	return nvertices;
}

static int wave_mode_7(struct frame_descriptor *frd, struct wave_vertex *vertices)
{
	return 0;
}

static int wave_mode_8(struct frame_descriptor *frd, struct wave_vertex *vertices)
{
	return 0;
}

static void compute_wave_vertices(struct frame_descriptor *frd, struct wave_params *params, struct wave_vertex *vertices, int *nvertices)
{
	params->wave_mode = frd->wave_mode;
	params->wave_additive = frd->wave_additive;
	params->wave_dots = frd->wave_usedots;
	params->wave_brighten = frd->wave_brighten;
	params->wave_thick = frd->wave_thick;

	params->wave_r = frd->wave_r;
	params->wave_g = frd->wave_g;
	params->wave_b = frd->wave_b;
	params->wave_a = frd->wave_a;

	params->treb = frd->treb;

	switch((int)frd->wave_mode) {
		case 0:
			*nvertices = wave_mode_0(frd, vertices);
			break;
		case 1:
			*nvertices = wave_mode_1(frd, vertices);
			break;
		case 2:
		case 3:
			*nvertices = wave_mode_23(frd, vertices);
			break;
		case 4:
			*nvertices = wave_mode_4(frd, vertices);
			break;
		case 5:
			*nvertices = wave_mode_5(frd, vertices);
			break;
		case 6:
			*nvertices = wave_mode_6(frd, vertices);
			break;
		case 7:
			*nvertices = wave_mode_7(frd, vertices);
			break;
		case 8:
			*nvertices = wave_mode_8(frd, vertices);
			break;
		default:
			*nvertices = 0;
			break;
	}
}

static void draw(unsigned short *fb, struct frame_descriptor *frd, struct wave_params *params, struct wave_vertex *vertices, int nvertices)
{
	draw_motion_vectors(fb, frd);
	draw_borders(fb, frd);
	wave_draw(fb, renderer_texsize, renderer_texsize, params, vertices, nvertices);
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
	struct wave_params params;
	struct wave_vertex vertices[256];
	int nvertices;
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
		ibrightness--;
		if(ibrightness > 63) ibrightness = 63;
		if(ibrightness < 0) ibrightness = 0;

		/* Compute frame */
		warp(tmu_fd, tex_frontbuffer, tex_backbuffer, frd->vertices, frd->tex_wrap, ibrightness);
		compute_wave_vertices(frd, &params, vertices, &nvertices);
		ioctl(tmu_fd, TMU_EXECUTE_WAIT);
		draw(tex_backbuffer, frd, &params, vertices, nvertices);

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
	assert(rtems_task_create(rtems_build_name('R', 'A', 'S', 'T'), 10, 8*1024,
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
