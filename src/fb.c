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

void init_fb_mtk(int quiet)
{
	struct fb_fix_screeninfo fb_fix;
	int mode, hres, vres;

	blanked = quiet;

	framebuffer_fd = open("/dev/fb", O_RDWR);
	assert(framebuffer_fd != -1);
	mode = sysconfig_get_resolution();
	get_resolution(mode, &hres, &vres);
	if(!quiet)
		ioctl(framebuffer_fd, FBIOSETVIDEOMODE, mode);
	ioctl(framebuffer_fd, FBIOGET_FSCREENINFO, &fb_fix);
	
	mtk_init((void *)fb_fix.smem_start, hres, vres);
}

void fb_unblank()
{
	if(blanked) {
		ioctl(framebuffer_fd, FBIOSETVIDEOMODE, sysconfig_get_resolution());
		blanked = 0;
	}
}

void fb_render_mode()
{
	if((sysconfig_get_resolution() != SC_RESOLUTION_640_480) || blanked)
		ioctl(framebuffer_fd, FBIOSETVIDEOMODE, SC_RESOLUTION_640_480);
	ioctl(framebuffer_fd, FBIOSETBUFFERMODE, FB_TRIPLE_BUFFERED);
	blanked = 0;
	g_render_mode = 1;
}

void fb_gui_mode()
{
	if(sysconfig_get_resolution() != SC_RESOLUTION_640_480)
		ioctl(framebuffer_fd, FBIOSETVIDEOMODE, sysconfig_get_resolution());
	ioctl(framebuffer_fd, FBIOSETBUFFERMODE, FB_SINGLE_BUFFERED);
	blanked = 0;
	g_render_mode = 0;
}

int fb_get_mode()
{
	return g_render_mode;
}

void fb_resize_gui()
{
	struct fb_fix_screeninfo fb_fix;
	int mode, hres, vres;
	
	mode = sysconfig_get_resolution();
	ioctl(framebuffer_fd, FBIOSETVIDEOMODE, mode);
	get_resolution(mode, &hres, &vres);
	ioctl(framebuffer_fd, FBIOGET_FSCREENINFO, &fb_fix);
	mtk_resize((void *)fb_fix.smem_start, hres, vres);
}
