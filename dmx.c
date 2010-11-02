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

#include <mtklib.h>

#include "config.h"
#include "cp.h"
#include "util.h"
#include "dmxtable.h"
#include "dmx.h"

static long appid;

static void load_config()
{
	int i, value;
	char confname[12];

	for(i=0;i<4;i++) {
		sprintf(confname, "idmx%d", i+1);
		value = config_read_int(confname, i+1);
		mtk_cmdf(appid, "e_idmx%d.set(-text \"%d\")", i, value);
		sprintf(confname, "dmx%d", i+1);
		value = config_read_int(confname, i+1);
		mtk_cmdf(appid, "e_dmx%d.set(-text \"%d\")", i, value);
	}
}

static void set_config()
{
	int i, value;
	char confname[16];

	for(i=0;i<4;i++) {
		sprintf(confname, "e_idmx%d.text", i);
		value = mtk_req_l(appid, confname);
		if((value < 1) || (value > 512))
			value = i;
		sprintf(confname, "idmx%d", i+1);
		config_write_int(confname, value);
		sprintf(confname, "e_dmx%d.text", i);
		value = mtk_req_l(appid, confname);
		if((value < 1) || (value > 512))
			value = i;
		sprintf(confname, "dmx%d", i+1);
		config_write_int(confname, value);
	}
	cp_notify_changed();
}

static void table_callback(mtk_event *e, void *arg)
{
	open_dmxtable_window();
}

static int w_open;

static void ok_callback(mtk_event *e, void *arg)
{
	w_open = 0;
	close_dmxtable_window();
	mtk_cmd(appid, "w.close()");
	set_config();
}

static void close_callback(mtk_event *e, void *arg)
{
	w_open = 0;
	close_dmxtable_window();
	mtk_cmd(appid, "w.close()");
}

void init_dmx()
{
	int i;

	appid = mtk_init_app("DMX settings");

	mtk_cmd_seq(appid,
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

	for(i=0;i<4;i++) {
		mtk_cmdf(appid, "l_idmx%d = new Label(-text \"idmx%d\")", i, i+1);
		mtk_cmdf(appid, "e_idmx%d = new Entry()", i);
		mtk_cmdf(appid, "g_in.place(l_idmx%d, -column 1 -row %d)", i, i);
		mtk_cmdf(appid, "g_in.place(e_idmx%d, -column 2 -row %d)", i, i);
		mtk_cmdf(appid, "l_dmx%d = new Label(-text \"dmx%d\")", i, i+1);
		mtk_cmdf(appid, "e_dmx%d = new Entry()", i);
		mtk_cmdf(appid, "g_out.place(l_dmx%d, -column 1 -row %d)", i, i);
		mtk_cmdf(appid, "g_out.place(e_dmx%d, -column 2 -row %d)", i, i);
	}

	mtk_bind(appid, "b_table", "commit", table_callback, NULL);

	mtk_bind(appid, "b_ok", "commit", ok_callback, NULL);
	mtk_bind(appid, "b_cancel", "commit", close_callback, NULL);
	mtk_bind(appid, "w", "close", close_callback, NULL);
}

void open_dmx_window()
{
	if(w_open) return;
	w_open = 1;
	load_config();
	mtk_cmd(appid, "w.open()");
}
