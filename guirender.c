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

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <rtems.h>
#include <bsp/milkymist_usbinput.h>
#include <mtklib.h>

#include "compiler.h"
#include "renderer.h"
#include "resmgr.h"
#include "fb.h"

#include "guirender.h"

extern int input_fd;

#define MOUSE_LEFT       0x01000000
#define MOUSE_RIGHT      0x02000000

void guirender(int appid, struct patch *p)
{
	rtems_interval timeout;

	if(!resmgr_acquire_multiple("renderer",
	  RESOURCE_AUDIO,
	  RESOURCE_DMX_IN,
	  RESOURCE_DMX_OUT,
	  RESOURCE_VIDEOIN,
	  RESOURCE_SAMPLER,
	  INVALID_RESOURCE))
		return;
#if 0
	renderer_start(framebuffer_fd, p);

	/* take over USB input events */
	while(1) {
		int r;
		unsigned int input_event;

		r = read(input_fd, &input_event, 4);
		if(r == 2) /* keyboard event */
			break;
		if(input_event & (MOUSE_LEFT|MOUSE_RIGHT)) {
			while(1) {
				r = read(input_fd, &input_event, 4);
				if((r == 4) && !(input_event & (MOUSE_LEFT|MOUSE_RIGHT)))
					break;
			}
			break;
		}
	}

	renderer_stop();
#endif

	resmgr_release(RESOURCE_SAMPLER);
	resmgr_release(RESOURCE_VIDEOIN);
	resmgr_release(RESOURCE_DMX_OUT);
	resmgr_release(RESOURCE_DMX_IN);
	resmgr_release(RESOURCE_AUDIO);

	mtk_cmd(appid, "screen.refresh()");
}
