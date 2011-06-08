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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <rtems.h>
#include <mtklib.h>
#include <mtkeycodes.h>

#include "compiler.h"
#include "renderer.h"
#include "resmgr.h"
#include "fb.h"
#include "input.h"

#include "guirender.h"

static void set_led(char c)
{
	int fd;

	fd = open("/dev/led2", O_RDWR);
	if(fd == -1) return;
	write(fd, &c, 1);
	close(fd);
}

static int stop_appid;

static void input_cb(mtk_event *e, int count);

static guirender_stop_callback callback;

static void stop()
{
	renderer_stop();

	resmgr_release(RESOURCE_SAMPLER);
	resmgr_release(RESOURCE_VIDEOIN);
	resmgr_release(RESOURCE_DMX_OUT);
	resmgr_release(RESOURCE_DMX_IN);
	resmgr_release(RESOURCE_AUDIO);

	mtk_cmd(stop_appid, "screen.refresh()");

	input_delete_callback(input_cb);
	input_add_callback(mtk_input);
	set_led('0');

	if(callback != NULL)
		callback();
}

static int wait_release;

static void input_cb(mtk_event *e, int count)
{
	int i;

	for(i=0;i<count;i++) {
		if(wait_release != -1) {
			if((e[i].type == EVENT_TYPE_RELEASE) && (e[i].press.code == wait_release)) {
				stop();
				mtk_input(&e[i+1], count-i-1);
				return;
			}
		} else {
			if(e[i].type == EVENT_TYPE_PRESS) {
				if((e[i].press.code == MTK_KEY_ENTER)
				|| (e[i].press.code == MTK_KEY_F2)
				|| (e[i].press.code == MTK_BTN_LEFT)
				|| (e[i].press.code == MTK_BTN_RIGHT))
					wait_release = e[i].press.code;
			}
		}
	}
}

int guirender(int appid, struct patch *p, guirender_stop_callback cb)
{
	if(!resmgr_acquire_multiple("renderer",
	  RESOURCE_AUDIO,
	  RESOURCE_DMX_IN,
	  RESOURCE_DMX_OUT,
	  RESOURCE_VIDEOIN,
	  RESOURCE_SAMPLER,
	  INVALID_RESOURCE))
		return 0;

	renderer_start(framebuffer_fd, p);

	wait_release = -1;
	stop_appid = appid;
	input_delete_callback(mtk_input);
	input_add_callback(input_cb);
	set_led('1');

	callback = cb;

	return 1;
}
