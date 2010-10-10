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
"fDecay=0.960000\n"
"nWaveMode=2\n"
"bAdditiveWaves=1\n"
"bWaveDots=0\n"
"bModWaveAlphaByVolume=1\n"
"bMaximizeWaveColor=1\n"
"bTexWrap=1\n"
"fWaveAlpha=4.099998\n"
"fWaveScale=1.421896\n"
"fModWaveAlphaStart=0.710000\n"
"fModWaveAlphaEnd=1.300000\n"
"zoom=1.990548\n"
"rot=0.020000\n"
"cx=0.500000\n"
"cy=0.500000\n"
"warp=0.198054\n"
"wave_r=0.650000\n"
"wave_g=0.650000\n"
"wave_b=0.650000\n"
"wave_x=0.500000\n"
"wave_y=0.550000\n"
"per_frame_1=wave_r = wave_r + 0.350*( 0.60*sin(0.980*time) + 0.40*sin(1.047*time) );\n"
"per_frame_2=wave_g = wave_g + 0.350*( 0.60*sin(0.835*time) + 0.40*sin(1.081*time) );\n"
		"\")",
		"g.place(ed, -column 1 -row 2)",
		"status = new Label(-text \"Ready.\" -font \"title\")",
		"g.place(status, -column 1 -row 3 -align \"nw\")",
		"g.rowconfig(3, -size 0)",
		"w = new Window(-content g -title \"Patch editor [untitled]\")",
		0);

	fileopen_appid = create_filedialog("Open patch", 0, openok_callback, NULL, opencancel_callback, NULL);

	dope_bind(appid, "b_open", "commit", openbtn_callback, NULL);

	dope_bind(appid, "w", "close", close_callback, NULL);
}

void open_patcheditor_window()
{
	dope_cmd(appid, "w.open()");
}
