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
#include <errno.h>

#include <dopelib.h>

#include "dmxtable.h"
#include "dmx.h"

static long appid;

static void table_callback(dope_event *e, void *arg)
{
	open_dmxtable_window();
}

static void close_callback(dope_event *e, void *arg)
{
	close_dmxtable_window();
	dope_cmd(appid, "w.close()");
}

void init_dmx()
{
	int i;

	appid = dope_init_app("DMX settings");

	dope_cmd_seq(appid,
		"g = new Grid()",

		"g_inout = new Grid()",

		"g_in = new Grid()",

		"g_inout.place(g_in, -column 1 -row 1)",

		"sep_inout = new Separator(-vertical yes)",
		"g_inout.place(sep_inout, -column 2 -row 1)",

		"g_out = new Grid()",

		"g_inout.place(g_out, -column 3 -row 1)",

		"g.place(g_inout, -column 1 -row 1)",

		"g_btn = new Grid()",
		"b_chain = new Button(-text \"Chain\")",
		"sep1 = new Separator(-vertical yes)",
		"b_spy = new Button(-text \"DMX spy\")",
		"b_table = new Button(-text \"DMX table\")",
		"sep2 = new Separator(-vertical yes)",
		"b_ok = new Button(-text \"OK\")",
		"b_cancel = new Button(-text \"Cancel\")",
		"g_btn.columnconfig(1, -size 20)",
		"g_btn.place(b_chain, -column 2 -row 1)",
		"g_btn.place(sep1, -column 3 -row 1)",
		"g_btn.place(b_spy, -column 4 -row 1)",
		"g_btn.place(b_table, -column 5 -row 1)",
		"g_btn.place(sep2, -column 6 -row 1)",
		"g_btn.place(b_ok, -column 7 -row 1)",
		"g_btn.place(b_cancel, -column 8 -row 1)",

		"g.place(g_btn, -column 1 -row 2)",

		"w = new Window(-content g -title \"DMX settings\")",
		0);

	for(i=1;i<=4;i++) {
		dope_cmdf(appid, "l_idmx%d = new Label(-text \"idmx%d\")", i, i);
		dope_cmdf(appid, "e_idmx%d = new Entry()", i);
		dope_cmdf(appid, "g_in.place(l_idmx%d, -column 1 -row %d)", i, i);
		dope_cmdf(appid, "g_in.place(e_idmx%d, -column 2 -row %d)", i, i);
		dope_cmdf(appid, "l_dmx%d = new Label(-text \"dmx%d\")", i, i);
		dope_cmdf(appid, "e_dmx%d = new Entry()", i);
		dope_cmdf(appid, "g_out.place(l_dmx%d, -column 1 -row %d)", i, i);
		dope_cmdf(appid, "g_out.place(e_dmx%d, -column 2 -row %d)", i, i);
	}

	dope_bind(appid, "b_table", "commit", table_callback, NULL);

	dope_bind(appid, "b_cancel", "commit", close_callback, NULL);
	dope_bind(appid, "w", "close", close_callback, NULL);
}

void open_dmx_window()
{
	dope_cmd(appid, "w.open()");
}
