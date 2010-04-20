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

#include "patcheditor.h"

static long appid;

static void close_callback(dope_event *e, void *arg)
{
	dope_cmd(appid, "w.close()");
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
"[preset00]\n"
"fRating=3.000000\n"
"fGammaAdj=2.000000\n"
"fDecay=0.960000\n"
"fVideoEchoZoom=1.006596\n"
"fVideoEchoAlpha=0.000000\n"
"nVideoEchoOrientation=3\n"
"nWaveMode=2\n"
"bAdditiveWaves=1\n"
"bWaveDots=0\n"
"bModWaveAlphaByVolume=1\n"
"bMaximizeWaveColor=1\n"
"bTexWrap=1\n"
"bDarkenCenter=0\n"
"bMotionVectorsOn=0\n"
"bRedBlueStereo=0\n"
"nMotionVectorsX=12\n"
"nMotionVectorsY=9\n"
"bBrighten=0\n"
"bDarken=0\n"
"bSolarize=0\n"
"bInvert=0\n"
"fWaveAlpha=4.099998\n"
"fWaveScale=1.421896\n"
"fWaveSmoothing=0.900000\n"
"fWaveParam=-0.500000\n"
"fModWaveAlphaStart=0.710000\n"
"fModWaveAlphaEnd=1.300000\n"
"fWarpAnimSpeed=1.000000\n"
"fWarpScale=1.331000\n"
"fZoomExponent=1.000000\n"
"fShader=0.000000\n"
"zoom=1.990548\n"
"rot=0.020000\n"
"cx=0.500000\n"
"cy=0.500000\n"
"dx=0.000000\n"
"dy=0.000000\n"
"warp=0.198054\n"
"sx=1.000000\n"
"sy=1.000000\n"
"wave_r=0.650000\n"
"wave_g=0.650000\n"
"wave_b=0.650000\n"
"wave_x=0.500000\n"
"wave_y=0.550000\n"
"ob_size=0.010000\n"
"ob_r=0.000000\n"
"ob_g=0.000000\n"
"ob_b=0.000000\n"
"ob_a=0.000000\n"
"ib_size=0.010000\n"
"ib_r=0.250000\n"
"ib_g=0.250000\n"
"ib_b=0.250000\n"
"ib_a=0.000000\n"
"per_frame_1=wave_r = wave_r + 0.350*( 0.60*sin(0.980*time) + 0.40*sin(1.047*time) );\n"
"per_frame_2=wave_g = wave_g + 0.350*( 0.60*sin(0.835*time) + 0.40*sin(1.081*time) );\n"
"per_frame_3=wave_b = wave_b + 0.350*( 0.60*sin(0.814*time) + 0.40*sin(1.011*time) );\n"
"per_frame_4=rot = rot + 0.030*( 0.60*sin(0.381*time) + 0.40*sin(0.479*time) );\n"
"per_frame_5=cx = cx + 0.110*( 0.60*sin(0.374*time) + 0.40*sin(0.294*time) );\n"
"per_frame_6=cy = cy + 0.110*( 0.60*sin(0.393*time) + 0.40*sin(0.223*time) );\n"
"per_frame_7=zoom=zoom+0.05+0.05*sin(time*0.133);\n"
"per_frame_8=\n"
"per_pixel_1=zoom=(zoom-1.0)*rad+1.0;\n"
		"\")",
		"g.place(ed, -column 1 -row 2)",
		"w = new Window(-content g -title \"Patch editor [untitled]\")",
		0);

	dope_bind(appid, "w", "close", close_callback, NULL);
}

void open_patcheditor_window()
{
	dope_cmd(appid, "w.open()");
}
