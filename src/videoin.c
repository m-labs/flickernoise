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

#include <bsp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <mtklib.h>
#include <bsp/milkymist_video.h>

#include "config.h"
#include "resmgr.h"
#include "util.h"
#include "cp.h"
#include "input.h"
#include "videoin.h"

static int appid;

static int video_fd;

static int brightness;
static int contrast;
static int hue;

static unsigned short preview_fb[180*144];

enum {
	CONTROL_BRIGHTNESS,
	CONTROL_CONTRAST,
	CONTROL_HUE
};

static void set_value(int channel, unsigned int val)
{
	int cmd;
	
	switch(channel) {
		case CONTROL_BRIGHTNESS:
			brightness = val;
			cmd = 1; // TODO
			break;
		case CONTROL_CONTRAST:
			contrast = val;
			cmd = 1; // TODO
			break;
		case CONTROL_HUE:
			hue = val;
			cmd = 2; // TODO
			break;
		default:
			return;
	}
	// TODO ioctl(video_fd, cmd, val);
}

static void slide_callback(mtk_event *e, void *arg)
{
	unsigned int control = (unsigned int)arg;
	unsigned int val;
	char *guiname;

	guiname = NULL;
	switch(control) {
		case CONTROL_BRIGHTNESS:
			guiname = "s_brightness.value";
			break;
		case CONTROL_CONTRAST:
			guiname = "s_contrast.value";
			break;
		case CONTROL_HUE:
			guiname = "s_hue.value";
			break;
		default:
			return;
	}
	
	val = mtk_req_i(appid, guiname);
	set_value(control, val);
}

static void load_config()
{
	set_value(CONTROL_BRIGHTNESS, config_read_int("vin_brightness", 50));
	set_value(CONTROL_CONTRAST, config_read_int("vin_contrast", 50));
	set_value(CONTROL_HUE, config_read_int("vin_hue", 50));

	mtk_cmdf(appid, "s_brightness.set(-value %d)", brightness);
	mtk_cmdf(appid, "s_contrast.set(-value %d)", contrast);
	mtk_cmdf(appid, "s_hue.set(-value %d)", hue);
}

static void set_config()
{
	config_write_int("vin_brightness", brightness);
	config_write_int("vin_contrast", contrast);
	config_write_int("vin_hue", hue);
	cp_notify_changed();
}

static int w_open;

#define UPDATE_PERIOD 10
static rtems_interval next_update;

static void preview_update(mtk_event *e, int count)
{
	rtems_interval t;
	unsigned short *videoframe;
	int x, y;

	t = rtems_clock_get_ticks_since_boot();
	if(t >= next_update) {
		videoframe = (unsigned short *)1; /* invalidate */
		ioctl(video_fd, VIDEO_BUFFER_LOCK, &videoframe);
		if(videoframe != NULL) {
			for(y=0;y<144;y++)
				for(x=0;x<180;x++)
					preview_fb[180*y+x] = videoframe[720*2*y+4*x];
			ioctl(video_fd, VIDEO_BUFFER_UNLOCK, videoframe);
			mtk_cmd(appid, "p_preview.refresh()");
		}
		
		next_update = t + UPDATE_PERIOD;
	}
}

static void close_videoin_window()
{
	mtk_cmd(appid, "w.close()");
	close(video_fd);
	w_open = 0;
	resmgr_release(RESOURCE_VIDEOIN);
	input_delete_callback(preview_update);
}

static void ok_callback(mtk_event *e, void *arg)
{
	close_videoin_window();
	set_config();
}

static void close_callback(mtk_event *e, void *arg)
{
	close_videoin_window();
}

void init_videoin()
{
	appid = mtk_init_app("Video in");

	mtk_cmd_seq(appid,
		"g = new Grid()",

		"gc = new Grid()",
		"l_brightness = new Label(-text \"Brightness:\")",
		"s_brightness = new Scale(-from 0 -to 100 -value 0 -orient horizontal)",
		"l_contrast = new Label(-text \"Contrast:\")",
		"s_contrast = new Scale(-from 0 -to 100 -value 0 -orient horizontal)",
		"l_hue = new Label(-text \"Hue:\")",
		"s_hue = new Scale(-from 0 -to 100 -value 0 -orient horizontal)",
		"l0_detected = new Label(-text \"Detected signal:\")",
		"l_detected = new Label(-text \"None\")",

		"gc.place(l_brightness, -column 1 -row 1)",
		"gc.place(s_brightness, -column 2 -row 1)",
		"gc.place(l_contrast, -column 1 -row 2)",
		"gc.place(s_contrast, -column 2 -row 2)",
		"gc.place(l_hue, -column 1 -row 3)",
		"gc.place(s_hue, -column 2 -row 3)",
		"gc.place(l0_detected, -column 1 -row 4)",
		"gc.place(l_detected, -column 2 -row 4)",
		"gc.columnconfig(2, -size 150)",

		"g.place(gc, -column 1 -row 1)",

		"g_preview = new Grid()",
		"l_preview = new Label(-text \"Preview\" -font \"title\")",
		"s_preview1 = new Separator(-vertical no)",
		"s_preview2 = new Separator(-vertical no)",
		"g_preview.place(s_preview1, -column 1 -row 1)",
		"g_preview.place(l_preview, -column 2 -row 1)",
		"g_preview.place(s_preview2, -column 3 -row 1)",

		"g.place(g_preview, -column 1 -row 2)",

		"p_preview = new Pixmap(-w 180 -h 144)",
		"g.place(p_preview, -column 1 -row 3)",

		"g.rowconfig(4, -size 10)",

		"g_btn = new Grid()",
		"b_ok = new Button(-text \"OK\")",
		"b_cancel = new Button(-text \"Cancel\")",
		"g_btn.columnconfig(1, -size 190)",
		"g_btn.place(b_ok, -column 2 -row 1)",
		"g_btn.place(b_cancel, -column 3 -row 1)",
		"g.place(g_btn, -column 1 -row 5)",

		"w = new Window(-content g -title \"Video input settings\")",
		0);

	mtk_cmdf(appid, "p_preview.set(-fb %d)", preview_fb);
	
	mtk_bind(appid, "s_brightness", "change", slide_callback, (void *)CONTROL_BRIGHTNESS);
	mtk_bind(appid, "s_contrast", "change", slide_callback, (void *)CONTROL_CONTRAST);
	mtk_bind(appid, "s_hue", "change", slide_callback, (void *)CONTROL_HUE);

	mtk_bind(appid, "b_ok", "commit", ok_callback, NULL);
	mtk_bind(appid, "b_cancel", "commit", close_callback, NULL);

	mtk_bind(appid, "w", "close", close_callback, NULL);
}

void open_videoin_window()
{
	if(w_open) return;

	if(!resmgr_acquire("Video in settings", RESOURCE_VIDEOIN))
		return;

	video_fd = open("/dev/video", O_RDWR);
	if(video_fd == -1) {
		perror("Unable to open video device");
		resmgr_release(RESOURCE_VIDEOIN);
		return;
	}
	
	w_open = 1;
	load_config();
	next_update = rtems_clock_get_ticks_since_boot() + UPDATE_PERIOD;
	input_add_callback(preview_update);
	mtk_cmd(appid, "w.open()");
}
