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
#include <dopelib.h>
#include <vscreen.h>

#include "version.h"
#include "flash.h"
#include "about.h"

static long appid;

static void ok_callback(dope_event *e, void *arg)
{
}

static void close_callback(dope_event *e, void *arg)
{
	dope_cmd(appid, "w.close()");
}

void init_audio()
{
	appid = dope_init_app("Audio");

	dope_cmd_seq(appid,
		"g = new Grid()",

		"gv = new Grid()",
		"l_linevol = new Label(-text \"Line volume\")",
		"s_linevol = new Scale(-orient vertical)",
		"l_micvol = new Label(-text \"Mic volume\")",
		"s_micvol = new Scale(-orient vertical)",

		"b_mutline = new Button(-text \"Mute\")",
		"b_mutmic = new Button(-text \"Mute\")",

		"gv.place(l_linevol, -column 1 -row 1)",
		"gv.place(s_linevol, -column 1 -row 2)",
		"gv.place(b_mutline, -column 1 -row 3)",
		"gv.place(l_micvol, -column 2 -row 1)",
		"gv.place(s_micvol, -column 2 -row 2)",
		"gv.place(b_mutmic, -column 2 -row 3)",

		"gv.rowconfig(2, -size 150)",

		"g.place(gv, -column 1 -row 1)",

		"g_btn = new Grid()",

		"b_ok = new Button(-text \"OK\")",
		"b_cancel = new Button(-text \"Cancel\")",

		"g_btn.place(b_ok, -column 2 -row 1)",
		"g_btn.place(b_cancel, -column 3 -row 1)",

		"g_btn.columnconfig(1, -size 120)",

		"g.place(g_btn, -column 1 -row 2)",

		"w = new Window(-content g -title \"Audio settings\")",
		0);


	dope_bind(appid, "b_ok", "commit", ok_callback, NULL);
	dope_bind(appid, "b_cancel", "commit", close_callback, NULL);

	dope_bind(appid, "w", "close", close_callback, NULL);
}

void open_audio_window()
{
	dope_cmd(appid, "w.open()");
}
