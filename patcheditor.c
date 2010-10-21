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
#include <stdlib.h>

#include <dopelib.h>
#include <vscreen.h>

#include "filedialog.h"
#include "patcheditor.h"
#include "framedescriptor.h"
#include "compiler.h"
#include "renderer.h"

static long appid;
static long fileopen_appid;

static void close_callback(dope_event *e, void *arg)
{
	dope_cmd(appid, "w.close()");
}

static void openbtn_callback(dope_event *e, void *arg)
{
	open_filedialog(fileopen_appid, "/");
}

static void openok_callback(dope_event *e, void *arg)
{
	char buf[384];
	get_filedialog_selection(fileopen_appid, buf, sizeof(buf));
	printf("file: %s\n", buf);
	close_filedialog(fileopen_appid);
}

static void opencancel_callback(dope_event *e, void *arg)
{
	close_filedialog(fileopen_appid);
}

static void saveasok_callback(dope_event *e, void *arg)
{
}

static void saveascancel_callback(dope_event *e, void *arg)
{
}

static void rmc(const char *m)
{
	puts(m);
}

extern int dope_rtems_framebuffer_fd;

static bool test_running;
static void run_callback(dope_event *e, void *arg)
{
	if(test_running)
		renderer_stop();
	else {
		struct patch *p;
		p = patch_compile(
"fDecay=0.9\n"
"fVideoEchoZoom=1\n"
"fVideoEchoAlpha=0\n"
"nVideoEchoOrientation=3\n"
"nWaveMode=3\n"
"bAdditiveWaves=1\n"
"bWaveDots=0\n"
"bWaveThick=0\n"
"bModWaveAlphaByVolume=0\n"
"bMaximizeWaveColor=1\n"
"bTexWrap=1\n"
"bDarkenCenter=0\n"
"bBrighten=0\n"
"bDarken=0\n"
"bSolarize=0\n"
"bInvert=0\n"
"fWaveAlpha=0.001645\n"
"fWaveScale=0.430333\n"
"fWaveSmoothing=0.63\n"
"fWaveParam=1\n"
"fModWaveAlphaStart=2\n"
"fModWaveAlphaEnd=2\n"
"fWarpAnimSpeed=1\n"
"fWarpScale=1\n"
"fZoomExponent=1\n"
"fShader=0\n"
"zoom=1\n"
"rot=0\n"
"cx=0.5\n"
"cy=0.5\n"
"dx=0\n"
"dy=0\n"
"warp=0.001\n"
"sx=1\n"
"sy=1\n"
"wave_r=0.65\n"
"wave_g=0.65\n"
"wave_b=0.65\n"
"wave_x=-1\n"
"wave_y=0.5\n"
"ob_size=0\n"
"ob_r=0\n"
"ob_g=0\n"
"ob_b=0.3\n"
"ob_a=1\n"
"ib_size=0.1\n"
"ib_r=1\n"
"ib_g=0.25\n"
"ib_b=0.25\n"
"ib_a=1\n"
"per_frame_1=bv = bass*.01+.99*bv;\n"
"per_frame_2=tt=tt+bass*.01;\n"
"per_frame_3=tt = if(above(bass*bass_att,4.5),abs(32768*sin(time)),tt);\n"
"per_frame_5=dx = .3*sin(tt*.12)+10*sin(tt*.015);\n"
"per_frame_6=dy = .39*sin(tt*.21)+20*sin(tt*.041);\n"
"per_frame_7=rot = 1*sin(tt*.15);\n"
"per_frame_8=cx = sin(tt*.16)*.5+.5;\n"
"per_frame_9=cy = cos(tt*.46)*.5+.5;\n"
"per_frame_10=ib_r = sin(tt*.51)*.5+.5;\n"
"per_frame_11=ib_g = sin(tt*.71)*.5+.5;\n"
"per_frame_12=ib_b = sin(tt*.81)*.5+.5;\n"
"per_frame_13=monitor = tt;\n"
"per_pixel_1=zoom = .8-.2*(1-rad);\n"
, rmc);
		if(p == NULL) {
			printf("Patch compilation failed\n");
			return;
		}
		printf("Patch compilation OK\n");
		renderer_start(dope_rtems_framebuffer_fd, p);
		printf("Started!\n");
		patch_free(p);
	}
	test_running = !test_running;
}

void init_patcheditor()
{
	appid = dope_init_app("Patch editor");
	dope_cmd_seq(appid,
		"g = new Grid()",
		"g_btn = new Grid()",
		"b_new = new Button(-text \"New\")",
		"b_open = new Button(-text \"Open\")",
		"b_save = new Button(-text \"Save\")",
		"b_saveas = new Button(-text \"Save As\")",
		"sep1 = new Separator(-vertical yes)",
		"b_run = new Button(-text \"Run!\")",
		"g_btn.place(b_new, -column 1 -row 1)",
		"g_btn.place(b_open, -column 2 -row 1)",
		"g_btn.place(b_save, -column 3 -row 1)",
		"g_btn.place(b_saveas, -column 4 -row 1)",
		"g_btn.place(sep1, -column 5 -row 1)",
		"g_btn.place(b_run, -column 6 -row 1)",
		"g.place(g_btn, -column 1 -row 1)",
		"ed = new Edit(-text \""
"decay=0.96\n"
"nWaveMode=6\n"
"rot=0.02\n"
"cx=0.5\n"
"cy=0.5\n"
"warp=0.198054\n"
"wave_r=0.65\n"
"wave_g=0.65\n"
"wave_b=0.65\n"
"wave_x=0.5\n"
"wave_y=0.55\n"
"per_frame=zoom = 1.01 + 0.05*bass\n"
		"\")",
		"g.place(ed, -column 1 -row 2)",
		"status = new Label(-text \"Ready.\" -font \"title\")",
		"g.place(status, -column 1 -row 3 -align \"nw\")",
		"g.rowconfig(3, -size 0)",
		"w = new Window(-content g -title \"Patch editor [untitled]\")",
		0);

	fileopen_appid = create_filedialog("Open patch", 0, openok_callback, NULL, opencancel_callback, NULL);

	dope_bind(appid, "b_open", "commit", openbtn_callback, NULL);
	dope_bind(appid, "b_run", "commit", run_callback, NULL);

	dope_bind(appid, "w", "close", close_callback, NULL);
}

void open_patcheditor_window()
{
	dope_cmd(appid, "w.open()");
}
