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

void init_fb_mtk()
{
	struct fb_fix_screeninfo fb_fix;
	struct fb_var_screeninfo fb_var;

	framebuffer_fd = open("/dev/fb", O_RDWR);
	assert(framebuffer_fd != -1);
	ioctl(framebuffer_fd, FBIOSETVIDEOMODE, sysconfig_get_resolution());
	ioctl(framebuffer_fd, FBIOGET_FSCREENINFO, &fb_fix);
	ioctl(framebuffer_fd, FBIOGET_VSCREENINFO, &fb_var);
	
	mtk_init((void *)fb_fix.smem_start, fb_var.xres, fb_var.yres);
}
