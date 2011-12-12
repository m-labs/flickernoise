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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <rtems/fb.h>
#include <mtklib.h>

#include "sysconfig.h"
#include "fb.h"

int framebuffer_fd;

static int blanked;
static int g_render_mode = 0;

static void get_resolution(int mode, int *hres, int *vres)
{
	*hres = 0;
	*vres = 0;
	switch(mode) {
		case SC_RESOLUTION_640_480:
			*hres = 640;
			*vres = 480;
			break;
		case SC_RESOLUTION_800_600:
			*hres = 800;
			*vres = 600;
			break;
		case SC_RESOLUTION_1024_768:
			*hres = 1024;
			*vres = 768;
			break;
		default:
			assert(0);
			break;
	}
}

static int current_mode = -1;

static void set_mode(int mode)
{
	if(current_mode != mode) {
		ioctl(framebuffer_fd, FBIOSETVIDEOMODE, mode);
		current_mode = mode;
	}
}

void init_fb_mtk(int quiet)
{
	struct fb_fix_screeninfo fb_fix;
	int mode, hres, vres;

	blanked = quiet;

	framebuffer_fd = open("/dev/fb", O_RDWR);
	assert(framebuffer_fd != -1);
	mode = sysconfig_get_resolution();
	get_resolution(mode, &hres, &vres);
	if(quiet)
		/* Assume we will go into rendering mode, and prevent screen blinking. */
		set_mode(SC_RESOLUTION_640_480);
	else
		set_mode(mode);
	ioctl(framebuffer_fd, FBIOGET_FSCREENINFO, &fb_fix);
	
	mtk_init((void *)fb_fix.smem_start, hres, vres);
	
	if(quiet) {
		ioctl(framebuffer_fd, FBIOSETBUFFERMODE, FB_TRIPLE_BUFFERED);
		ioctl(framebuffer_fd, FBIOSWAPBUFFERS);
	}
}

void fb_unblank(void)
{
	if(blanked) {
		set_mode(sysconfig_get_resolution());
		ioctl(framebuffer_fd, FBIOSETBUFFERMODE, FB_SINGLE_BUFFERED);
		blanked = 0;
		/* FIXME: work around "black screen" bug in MTK */
		mtk_cmd(1, "screen.refresh()");
	}
}

void fb_render_mode(void)
{
	set_mode(SC_RESOLUTION_640_480);
	ioctl(framebuffer_fd, FBIOSETBUFFERMODE, FB_TRIPLE_BUFFERED);
	blanked = 0;
	g_render_mode = 1;
}

void fb_gui_mode(void)
{
	set_mode(sysconfig_get_resolution());
	ioctl(framebuffer_fd, FBIOSETBUFFERMODE, FB_SINGLE_BUFFERED);
	blanked = 0;
	g_render_mode = 0;
}

int fb_get_mode(void)
{
	return g_render_mode;
}

void fb_resize_gui(void)
{
	struct fb_fix_screeninfo fb_fix;
	int mode, hres, vres;
	
	mode = sysconfig_get_resolution();
	set_mode(mode);
	get_resolution(mode, &hres, &vres);
	ioctl(framebuffer_fd, FBIOGET_FSCREENINFO, &fb_fix);
	mtk_resize((void *)fb_fix.smem_start, hres, vres);
}
