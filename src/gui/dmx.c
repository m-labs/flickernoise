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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <rtems.h>
#include <bsp/milkymist_dmx.h>

#include <mtklib.h>

#include "../config.h"
#include "../renderer/framedescriptor.h"
#include "cp.h"
#include "../util.h"
#include "resmgr.h"
#include "monitor.h"
#include "dmxspy.h"
#include "dmxdesk.h"
#include "dmx.h"

static int appid;

static int chain_mode;

static void set_chain_mode(int chain)
{
	int fd;
	
	if(!resmgr_acquire("DMX settings", RESOURCE_DMX_OUT)) return;

	chain_mode = chain;
	if(chain)
		mtk_cmd(appid, "b_chain.set(-state on)");
	else
		mtk_cmd(appid, "b_chain.set(-state off)");

	fd = open("/dev/dmx_out", O_RDWR);
	if(fd == -1) {
		resmgr_release(RESOURCE_DMX_OUT);
		return;
	}
	ioctl(fd, DMX_SET_THRU, chain);
	close(fd);
	resmgr_release(RESOURCE_DMX_OUT);
}

void load_dmx_config()
{
	int i, value;
	char confname[12];

	for(i=0;i<IDMX_COUNT;i++) {
		sprintf(confname, "idmx%d", i+1);
		value = config_read_int(confname, i+1);
		mtk_cmdf(appid, "e_idmx%d.set(-text \"%d\")", i, value);
	}
	for(i=0;i<DMX_COUNT;i++) {
		sprintf(confname, "dmx%d", i+1);
		value = config_read_int(confname, i+1);
		mtk_cmdf(appid, "e_dmx%d.set(-text \"%d\")", i, value);
	}
	set_chain_mode(config_read_int("dmx_chain", 1));
}

static void set_config()
{
	int i, value;
	char confname[16];

	for(i=0;i<IDMX_COUNT;i++) {
		sprintf(confname, "e_idmx%d.text", i);
		value = mtk_req_i(appid, confname);
		if((value < 1) || (value > 512))
			value = i+1;
		sprintf(confname, "idmx%d", i+1);
		config_write_int(confname, value);
	}
	for(i=0;i<DMX_COUNT;i++) {
		sprintf(confname, "e_dmx%d.text", i);
		value = mtk_req_i(appid, confname);
		if((value < 1) || (value > 512))
			value = i+1;
		sprintf(confname, "dmx%d", i+1);
		config_write_int(confname, value);
	}
	config_write_int("dmx_chain", chain_mode);
	cp_notify_changed();
	monitor_notify_changed();
}

static void chain_callback(mtk_event *e, void *arg)
{
	set_chain_mode(!chain_mode);
}

static void spy_callback(mtk_event *e, void *arg)
{
	open_dmxspy_window();
}

static void desk_callback(mtk_event *e, void *arg)
{
	open_dmxdesk_window();
}

static int w_open;

static void ok_callback(mtk_event *e, void *arg)
{
	w_open = 0;
	close_dmxspy_window();
	close_dmxdesk_window();
	mtk_cmd(appid, "w.close()");
	set_config();
}

static void close_callback(mtk_event *e, void *arg)
{
	w_open = 0;
	close_dmxspy_window();
	close_dmxdesk_window();
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
		"b_desk = new Button(-text \"DMX desk\")",
		"sep2 = new Separator(-vertical yes)",
		"b_ok = new Button(-text \"OK\")",
		"b_cancel = new Button(-text \"Cancel\")",
		"g_btn.columnconfig(1, -size 20)",
		"g_btn.place(b_chain, -column 2 -row 1)",
		"g_btn.place(sep1, -column 3 -row 1)",
		"g_btn.place(b_spy, -column 4 -row 1)",
		"g_btn.place(b_desk, -column 5 -row 1)",
		"g_btn.place(sep2, -column 6 -row 1)",
		"g_btn.place(b_ok, -column 7 -row 1)",
		"g_btn.place(b_cancel, -column 8 -row 1)",

		"g.place(g_btn, -column 1 -row 2)",

		"w = new Window(-content g -title \"DMX settings\")",
		0);

	for(i=0;i<IDMX_COUNT;i++) {
		mtk_cmdf(appid, "l_idmx%d = new Label(-text \"\eidmx%d\")", i, i+1);
		mtk_cmdf(appid, "e_idmx%d = new Entry()", i);
		mtk_cmdf(appid, "g_in.place(l_idmx%d, -column 1 -row %d)", i, i);
		mtk_cmdf(appid, "g_in.place(e_idmx%d, -column 2 -row %d)", i, i);
	}
	for(i=0;i<DMX_COUNT;i++) {
		mtk_cmdf(appid, "l_dmx%d = new Label(-text \"\edmx%d\")", i, i+1);
		mtk_cmdf(appid, "e_dmx%d = new Entry()", i);
		mtk_cmdf(appid, "g_out.place(l_dmx%d, -column 1 -row %d)", i, i);
		mtk_cmdf(appid, "g_out.place(e_dmx%d, -column 2 -row %d)", i, i);
	}

	mtk_bind(appid, "b_chain", "press", chain_callback, NULL);
	mtk_bind(appid, "b_spy", "commit", spy_callback, NULL);
	mtk_bind(appid, "b_desk", "commit", desk_callback, NULL);

	mtk_bind(appid, "b_ok", "commit", ok_callback, NULL);
	mtk_bind(appid, "b_cancel", "commit", close_callback, NULL);
	mtk_bind(appid, "w", "close", close_callback, NULL);

	set_chain_mode(1);
}

void open_dmx_window()
{
	if(w_open) return;
	w_open = 1;
	load_dmx_config();
	mtk_cmd(appid, "w.open()");
}
