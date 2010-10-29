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

#include "filedialog.h"
#include "patcheditor.h"
#include "framedescriptor.h"
#include "compiler.h"
#include "guirender.h"

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
	char *buf;
	FILE *fd;
	int r;

	buf = malloc(32768);
	if(!buf) return;
	get_filedialog_selection(fileopen_appid, buf, 32768);
	close_filedialog(fileopen_appid);

	fd = fopen(buf, "r");
	if(!fd) {
		dope_cmdf(appid, "status.set(-text \"Unable to open file (%s)\")", strerror(errno));
		free(buf);
		return;
	}
	r = fread(buf, 1, 32767, fd);
	fclose(fd);
	if(r < 0) {
		dope_cmd(appid, "status.set(-text \"Unable to read file\")");
		free(buf);
		return;
	}
	buf[r] = 0;

	dope_cmdf(appid, "ed.set(-text \"%s\")", buf);

	free(buf);
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
	dope_cmdf(appid, "status.set(-text \"%s\")", m);
}

static void run_callback(dope_event *e, void *arg)
{
	struct patch *p;
	char *code;

	code = malloc(32768);
	if(code == NULL)
		return;
	dope_req(appid, code, 32768, "ed.text");
	p = patch_compile(code, rmc);
	free(code);
	if(p == NULL)
		return;

	guirender(appid, p);

	patch_free(p);
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
	dope_bind(appid, "b_run", "release", run_callback, NULL);

	dope_bind(appid, "w", "close", close_callback, NULL);
}

void open_patcheditor_window()
{
	dope_cmd(appid, "w.open()");
}
