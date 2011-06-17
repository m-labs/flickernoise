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

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <rtems.h>
#include <mtklib.h>
#include <mtkeycodes.h>
#include <bsp/milkymist_video.h>

#include "compiler.h"
#include "renderer.h"
#include "resmgr.h"
#include "fb.h"
#include "input.h"
#include "config.h"
#include "cp.h"
#include "videoinreconf.h"

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

static int guirender_running;
static guirender_stop_callback callback;

void guirender_stop()
{
	if(!guirender_running) return;
	guirender_running = 0;

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

static void adjust_brightness(int amount)
{
	int brightness;
	
	brightness = config_read_int("vin_brightness", 0);
	brightness += amount;
	if(brightness < -128)
		brightness = -128;
	else if(brightness > 127)
		brightness = 127;
	config_write_int("vin_brightness", brightness);
	videoinreconf_request(VIDEO_SET_BRIGHTNESS, brightness);
	cp_notify_changed();
}

static void adjust_contrast(int amount)
{
	int contrast;

	contrast = config_read_int("vin_contrast", 0x80);
	contrast += amount;
	if(contrast < 0)
		contrast = 0;
	else if(contrast > 255)
		contrast = 255;
	config_write_int("vin_contrast", contrast);
	videoinreconf_request(VIDEO_SET_CONTRAST, contrast);
	cp_notify_changed();
}

static void input_cb(mtk_event *e, int count)
{
	int i;

	for(i=0;i<count;i++) {
		if((e[i].type == EVENT_TYPE_PRESS) && 
		  ((e[i].press.code == MTK_KEY_ENTER)
		    || (e[i].press.code == MTK_KEY_F2)
		    || (e[i].press.code == MTK_BTN_LEFT)
		    || (e[i].press.code == MTK_BTN_RIGHT))) {
			guirender_stop();
			mtk_input(&e[i+1], count-i-1);
		}
		if(e[i].type == EVENT_TYPE_PRESS) {
			switch(e[i].press.code) {
				case MTK_KEY_F5:
					adjust_brightness(5);
					break;
				case MTK_KEY_F6:
					adjust_brightness(-5);
					break;
				case MTK_KEY_F7:
					adjust_contrast(5);
					break;
				case MTK_KEY_F8:
					adjust_contrast(-5);
					break;
			}
		}
	}
}

int guirender(int appid, struct patch *p, guirender_stop_callback cb)
{
	if(guirender_running) return 0;

	if(!resmgr_acquire_multiple("renderer",
	  RESOURCE_AUDIO,
	  RESOURCE_DMX_IN,
	  RESOURCE_DMX_OUT,
	  RESOURCE_VIDEOIN,
	  RESOURCE_SAMPLER,
	  INVALID_RESOURCE))
		return 0;
	
	guirender_running = 1;

	renderer_start(framebuffer_fd, p);

	stop_appid = appid;
	input_delete_callback(mtk_input);
	input_add_callback(input_cb);
	set_led('1');

	callback = cb;

	return 1;
}
