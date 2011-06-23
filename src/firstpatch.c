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
#include <mtklib.h>

#include "config.h"
#include "cp.h"
#include "filedialog.h"

#include "firstpatch.h"

static int appid;
static struct filedialog *browse_dlg;

static int w_open;

static void ok_callback(mtk_event *e, void *arg)
{
	char filename[384];

	mtk_cmd(appid, "w.close()");
	w_open = 0;
	mtk_req(appid, filename, sizeof(filename), "e_filename.text");
	config_write_string("firstpatch", filename);
	cp_notify_changed();
	close_filedialog(browse_dlg);
}

static void cancel_callback(mtk_event *e, void *arg)
{
	mtk_cmd(appid, "w.close()");
	w_open = 0;
	close_filedialog(browse_dlg);
}

static void browse_callback(mtk_event *e, void *arg)
{
	open_filedialog(browse_dlg);
}

static void browse_ok_callback(void *arg)
{
	char filename[384];

	get_filedialog_selection(browse_dlg, filename, sizeof(filename));
	mtk_cmdf(appid, "e_filename.set(-text \"%s\")", filename);
}

void init_firstpatch()
{
	appid = mtk_init_app("First patch");

	mtk_cmd_seq(appid,
		"g = new Grid()",

		"g_filename = new Grid()",
		"l_filename = new Label(-text \"Filename:\")",
		"e_filename = new Entry()",
		"b_filename = new Button(-text \"Browse\")",
		"g_filename.place(l_filename, -column 1 -row 1)",
		"g_filename.place(e_filename, -column 2 -row 1)",
		"g_filename.place(b_filename, -column 3 -row 1)",
		"g_filename.columnconfig(3, -size 0)",

		"g_btn = new Grid()",
		"b_ok = new Button(-text \"OK\")",
		"b_cancel = new Button(-text \"Cancel\")",
		"g_btn.columnconfig(1, -size 350)",
		"g_btn.place(b_ok, -column 2 -row 1)",
		"g_btn.place(b_cancel, -column 3 -row 1)",

		"g.place(g_filename, -column 1 -row 1)",
		"g.rowconfig(2, -size 10)",
		"g.place(g_btn, -column 1 -row 3)",

		"w = new Window(-content g -title \"First patch\")",
		0);


	mtk_bind(appid, "b_filename", "commit", browse_callback, NULL);
	mtk_bind(appid, "b_ok", "commit", ok_callback, NULL);
	mtk_bind(appid, "b_cancel", "commit", cancel_callback, NULL);

	mtk_bind(appid, "w", "close", cancel_callback, NULL);

	browse_dlg = create_filedialog("Select first patch", 0, "fnp", browse_ok_callback, NULL, NULL, NULL);
}

void open_firstpatch_window()
{
	const char *firstpatch;
	
	if(w_open) return;
	w_open = 1;
	
	firstpatch = config_read_string("firstpatch");
	if(firstpatch == NULL) firstpatch = "";
	mtk_cmdf(appid, "e_filename.set(-text \"%s\")", firstpatch);

	mtk_cmd(appid, "w.open()");
}
