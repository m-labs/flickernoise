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
#include <string.h>
#include <errno.h>

#include <mtklib.h>
#include <keycodes.h>

#include "filedialog.h"
#include "patcheditor.h"
#include "framedescriptor.h"
#include "compiler.h"
#include "input.h"
#include "guirender.h"

static int appid;
static int fileopen_appid;
static int filesave_appid;

static int modified;
static char current_filename[384];

static void update_wintitle()
{
	const char *short_filename;

	if(current_filename[0] == 0)
		short_filename = "untitled";
	else {
		short_filename = strrchr(current_filename, '/');
		if(short_filename == NULL)
			short_filename = current_filename;
		else
			short_filename++;
	}
	mtk_cmdf(appid, "w.set(-title \"Patch editor [%s]%s\")",
		short_filename,
		modified ? " [modified]" : "");
}

static void new_callback(mtk_event *e, void *arg)
{
	modified = 0;
	current_filename[0] = 0;
	mtk_cmd(appid, "ed.set(-text \"\")");
	update_wintitle();
}

static void openbtn_callback(mtk_event *e, void *arg)
{
	open_filedialog(fileopen_appid, "/");
}

static void openok_callback(mtk_event *e, void *arg)
{
	char buf[32768];
	FILE *fd;
	int r;

	get_filedialog_selection(fileopen_appid, current_filename, sizeof(current_filename));
	close_filedialog(fileopen_appid);
	modified = 0;
	update_wintitle();

	fd = fopen(current_filename, "r");
	if(!fd) {
		mtk_cmdf(appid, "status.set(-text \"Unable to open file (%s)\")", strerror(errno));
		return;
	}
	r = fread(buf, 1, 32767, fd);
	fclose(fd);
	if(r < 0) {
		mtk_cmd(appid, "status.set(-text \"Unable to read file\")");
		return;
	}
	buf[r] = 0;

	mtk_cmdf(appid, "ed.set(-text \"%s\")", buf);
}

static void opencancel_callback(mtk_event *e, void *arg)
{
	close_filedialog(fileopen_appid);
}

static void save_current()
{
	char buf[32768];
	FILE *fd;

	fd = fopen(current_filename, "w");
	if(!fd) {
		mtk_cmdf(appid, "status.set(-text \"Unable to open file (%s)\")", strerror(errno));
		return;
	}
	mtk_req(appid, buf, 32768, "ed.text");
	if(fwrite(buf, 1, strlen(buf), fd) < 0) {
		mtk_cmdf(appid, "status.set(-text \"Unable to write file (%s)\")", strerror(errno));
		fclose(fd);
		return;
	}
	if(fclose(fd) != 0)
		mtk_cmdf(appid, "status.set(-text \"Unable to close file (%s)\")", strerror(errno));
}

static void saveas_callback(mtk_event *e, void *arg)
{
	open_filedialog(filesave_appid, "/");
}

static void save_callback(mtk_event *e, void *arg)
{
	if(current_filename[0] == 0)
		saveas_callback(e, arg);
	else {
		modified = 0;
		update_wintitle();
		save_current();
	}
}

static void saveasok_callback(mtk_event *e, void *arg)
{
	get_filedialog_selection(filesave_appid, current_filename, sizeof(current_filename));
	close_filedialog(filesave_appid);
	modified = 0;
	update_wintitle();
	save_current();
}

static void saveascancel_callback(mtk_event *e, void *arg)
{
	close_filedialog(filesave_appid);
}

static void rmc(const char *m)
{
	mtk_cmdf(appid, "status.set(-text \"%s\")", m);
}

static void run_callback(mtk_event *e, void *arg)
{
	struct patch *p;
	char code[32768];

	mtk_cmd(appid, "status.set(-text \"Ready.\")");
	mtk_req(appid, code, 32768, "ed.text");
	p = patch_compile(code, rmc);
	if(p == NULL)
		return;

	guirender(appid, p, NULL);

	patch_free(p);
}

static void change_callback(mtk_event *e, void *arg)
{
	if(!modified) {
		modified = 1;
		update_wintitle();
	}
}

static void accel_callback(mtk_event *e, void *arg)
{
	if((e->type == EVENT_TYPE_PRESS) && (e->press.code == MTK_KEY_F8)) {
		mtk_event evt;

		/* hack to prevent "stuck key" bug */
		evt.type = EVENT_TYPE_RELEASE;
		evt.release.code = MTK_KEY_F8;
		mtk_input(&evt, 1);

		run_callback(e, arg);
	}
}

static void close_callback(mtk_event *e, void *arg);

void init_patcheditor()
{
	appid = mtk_init_app("Patch editor");
	mtk_cmd_seq(appid,
		"g = new Grid()",
		"g_btn = new Grid()",
		"b_new = new Button(-text \"New\")",
		"b_open = new Button(-text \"Open\")",
		"b_save = new Button(-text \"Save\")",
		"b_saveas = new Button(-text \"Save As\")",
		"sep1 = new Separator(-vertical yes)",
		"b_run = new Button(-text \"Run (F8)\")",
		"g_btn.place(b_new, -column 1 -row 1)",
		"g_btn.place(b_open, -column 2 -row 1)",
		"g_btn.place(b_save, -column 3 -row 1)",
		"g_btn.place(b_saveas, -column 4 -row 1)",
		"g_btn.place(sep1, -column 5 -row 1)",
		"g_btn.place(b_run, -column 6 -row 1)",
		"g.place(g_btn, -column 1 -row 1)",
		"ed = new Edit()",
		"g.place(ed, -column 1 -row 2)",
		"status = new Label(-text \"Ready.\" -font \"title\")",
		"g.place(status, -column 1 -row 3 -align \"nw\")",
		"g.rowconfig(3, -size 0)",
		"w = new Window(-content g -title \"Patch editor [untitled]\")",
		0);

	fileopen_appid = create_filedialog("Open patch", 0, openok_callback, NULL, opencancel_callback, NULL);
	filesave_appid = create_filedialog("Save patch", 1, saveasok_callback, NULL, saveascancel_callback, NULL);

	mtk_bind(appid, "b_new", "commit", new_callback, NULL);
	mtk_bind(appid, "b_open", "commit", openbtn_callback, NULL);
	mtk_bind(appid, "b_save", "commit", save_callback, NULL);
	mtk_bind(appid, "b_saveas", "commit", saveas_callback, NULL);
	mtk_bind(appid, "b_run", "release", run_callback, NULL);

	mtk_bind(appid, "ed", "change", change_callback, NULL);

	mtk_bind(appid, "w", "press", accel_callback, NULL);

	mtk_bind(appid, "w", "close", close_callback, NULL);
}

static int w_open;

static void close_callback(mtk_event *e, void *arg)
{
	w_open = 0;
	mtk_cmd(appid, "w.close()");
}

void open_patcheditor_window()
{
	if(w_open) return;
	w_open = 1;
	mtk_cmd(appid, "w.open()");
}
