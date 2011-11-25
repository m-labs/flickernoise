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

#include "../config.h"
#include "resmgr.h"
#include "../util.h"
#include "cp.h"
#include "../input.h"
#include "videoin.h"

static int appid;

static int video_fd;

static int format;
static int brightness;
static int contrast;
static int hue;

static unsigned short preview_fb[180*144];

enum {
	CONTROL_BRIGHTNESS,
	CONTROL_CONTRAST,
	CONTROL_HUE
};

static void set_format(int f)
{
	ioctl(video_fd, VIDEO_SET_FORMAT, f);
	mtk_cmdf(appid, "b_cvbsg.set(-state %s)", f == VIDEO_FORMAT_CVBS6 ? "on" : "off");
	mtk_cmdf(appid, "b_cvbsb.set(-state %s)", f == VIDEO_FORMAT_CVBS5 ? "on" : "off");
	mtk_cmdf(appid, "b_cvbsr.set(-state %s)", f == VIDEO_FORMAT_CVBS4 ? "on" : "off");
	mtk_cmdf(appid, "b_svideo.set(-state %s)", f == VIDEO_FORMAT_SVIDEO ? "on" : "off");
	mtk_cmdf(appid, "b_component.set(-state %s)", f == VIDEO_FORMAT_COMPONENT ? "on" : "off");
	format = f;
}

static void set_value(int channel, unsigned int val)
{
	int cmd;
	
	switch(channel) {
		case CONTROL_BRIGHTNESS:
			brightness = val;
			cmd = VIDEO_SET_BRIGHTNESS;
			break;
		case CONTROL_CONTRAST:
			contrast = val;
			cmd = VIDEO_SET_CONTRAST;
			break;
		case CONTROL_HUE:
			hue = val;
			cmd = VIDEO_SET_HUE;
			break;
		default:
			return;
	}
	ioctl(video_fd, cmd, val);
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

static void format_callback(mtk_event *e, void *arg)
{
	set_format((int)arg);
}

static void load_config()
{
	set_format(config_read_int("vin_format", VIDEO_FORMAT_CVBS6));

	set_value(CONTROL_BRIGHTNESS, config_read_int("vin_brightness", 0));
	set_value(CONTROL_CONTRAST, config_read_int("vin_contrast", 0x80));
	set_value(CONTROL_HUE, config_read_int("vin_hue", 0));

	mtk_cmdf(appid, "s_brightness.set(-value %d)", brightness);
	mtk_cmdf(appid, "s_contrast.set(-value %d)", contrast);
	mtk_cmdf(appid, "s_hue.set(-value %d)", hue);
}

static void set_config()
{
	config_write_int("vin_format", format);
	config_write_int("vin_brightness", brightness);
	config_write_int("vin_contrast", contrast);
	config_write_int("vin_hue", hue);
	cp_notify_changed();
}

static int w_open;

#define UPDATE_PERIOD 10
static rtems_interval next_update;

static char *fmt_video_signal(unsigned int status)
{
	if(!(status & 0x01)) return "None";
	switch((status & 0x70) >> 4) {
		case 0: return "\eNTSM-MJ";
		case 1: return "\eNTSC-443";
		case 2: return "\ePAL-M";
		case 3: return "\ePAL-60";
		case 4: return "\ePAL-B/G/H/I/D";
		case 5: return "\eSECAM";
		case 6: return "\ePAL-Combination N";
		case 7: return "\eSECAM 525";
		default: return "Unknown";
	}
}

static void preview_update(mtk_event *e, int count)
{
	rtems_interval t;
	unsigned short *videoframe;
	int x, y;
	unsigned int status;

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

		ioctl(video_fd, VIDEO_GET_SIGNAL, &status);
		mtk_cmdf(appid, "l_detected.set(-text \"%s\")", fmt_video_signal(status));
		
		next_update = t + UPDATE_PERIOD;
	}
}

static void close_videoin_window()
{
	input_delete_callback(preview_update);
	mtk_cmd(appid, "w.close()");
	load_config();
	close(video_fd);
	w_open = 0;
	resmgr_release(RESOURCE_VIDEOIN);
}

static void ok_callback(mtk_event *e, void *arg)
{
	set_config();
	close_videoin_window();
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
		    
		"g_format = new Grid()",
		"l_format = new Label(-text \"Format\" -font \"title\")",
		"s_format1 = new Separator(-vertical no)",
		"s_format2 = new Separator(-vertical no)",
		"g_format.place(s_format1, -column 1 -row 1)",
		"g_format.place(l_format, -column 2 -row 1)",
		"g_format.place(s_format2, -column 3 -row 1)",
		"g.place(g_format, -column 1 -row 1)",

		"g_cvbs = new Grid()",
		"b_cvbsg = new Button(-text \"CVBS: Green\")",
		"b_cvbsb = new Button(-text \"CVBS: Blue\")",
		"b_cvbsr = new Button(-text \"CVBS: Red\")",
		"g_cvbs.place(b_cvbsg, -column 1 -row 1)",
		"g_cvbs.place(b_cvbsb, -column 2 -row 1)",
		"g_cvbs.place(b_cvbsr, -column 3 -row 1)",
		"g.place(g_cvbs, -column 1 -row 2)",
		"b_svideo = new Button(-text \"S-Video (Y: Green, C: Blue)\")",
		"g.place(b_svideo, -column 1 -row 3)",
		"b_component = new Button(-text \"Component (YPbPr)\")",
		"g.place(b_component, -column 1 -row 4)",
		
		"g_detected = new Grid()",
		"l0_detected = new Label(-text \"Detected signal:\")",
		"l_detected = new Label(-text \"None\")",
		"g_detected.place(l0_detected, -column 1 -row 1)",
		"g_detected.place(l_detected, -column 2 -row 1)",
		"g.place(g_detected, -column 1 -row 5)",
		
		"g_parameters = new Grid()",
		"l_parameters = new Label(-text \"Parameters\" -font \"title\")",
		"s_parameters1 = new Separator(-vertical no)",
		"s_parameters2 = new Separator(-vertical no)",
		"g_parameters.place(s_parameters1, -column 1 -row 1)",
		"g_parameters.place(l_parameters, -column 2 -row 1)",
		"g_parameters.place(s_parameters2, -column 3 -row 1)",
		"g.place(g_parameters, -column 1 -row 6)",
		
		"gc = new Grid()",
		"l_brightness = new Label(-text \"Brightness:\")",
		"s_brightness = new Scale(-from -128 -to 127 -value 0 -orient horizontal)",
		"l_contrast = new Label(-text \"Contrast:\")",
		"s_contrast = new Scale(-from 0 -to 255 -value 128 -orient horizontal)",
		"l_hue = new Label(-text \"Hue:\")",
		"s_hue = new Scale(-from -128 -to 127 -value 0 -orient horizontal)",
		"gc.place(l_brightness, -column 1 -row 1)",
		"gc.place(s_brightness, -column 2 -row 1)",
		"gc.place(l_contrast, -column 1 -row 2)",
		"gc.place(s_contrast, -column 2 -row 2)",
		"gc.place(l_hue, -column 1 -row 3)",
		"gc.place(s_hue, -column 2 -row 3)",
		"gc.columnconfig(2, -size 150)",
		"g.place(gc, -column 1 -row 7)",

		"g_preview = new Grid()",
		"l_preview = new Label(-text \"Preview\" -font \"title\")",
		"s_preview1 = new Separator(-vertical no)",
		"s_preview2 = new Separator(-vertical no)",
		"g_preview.place(s_preview1, -column 1 -row 1)",
		"g_preview.place(l_preview, -column 2 -row 1)",
		"g_preview.place(s_preview2, -column 3 -row 1)",
		"g.place(g_preview, -column 1 -row 8)",

		"p_preview = new Pixmap(-w 180 -h 144)",
		"g.place(p_preview, -column 1 -row 9)",

		"g.rowconfig(10, -size 10)",

		"g_btn = new Grid()",
		"b_ok = new Button(-text \"OK\")",
		"b_cancel = new Button(-text \"Cancel\")",
		"g_btn.columnconfig(1, -size 190)",
		"g_btn.place(b_ok, -column 2 -row 1)",
		"g_btn.place(b_cancel, -column 3 -row 1)",
		"g.place(g_btn, -column 1 -row 11)",

		"w = new Window(-content g -title \"Video input settings\" -worky 30)",
		0);

	mtk_cmdf(appid, "p_preview.set(-fb %d)", preview_fb);
	
	mtk_bind(appid, "b_cvbsg", "press", format_callback, (void *)VIDEO_FORMAT_CVBS6);
	mtk_bind(appid, "b_cvbsb", "press", format_callback, (void *)VIDEO_FORMAT_CVBS5);
	mtk_bind(appid, "b_cvbsr", "press", format_callback, (void *)VIDEO_FORMAT_CVBS4);
	mtk_bind(appid, "b_svideo", "press", format_callback, (void *)VIDEO_FORMAT_SVIDEO);
	mtk_bind(appid, "b_component", "press", format_callback, (void *)VIDEO_FORMAT_COMPONENT);
	
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
