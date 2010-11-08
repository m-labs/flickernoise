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
#include <stdlib.h>
#include <string.h>
#include <mtklib.h>

#include "util.h"
#include "input.h"
#include "config.h"
#include "cp.h"
#include "messagebox.h"
#include "filedialog.h"

#include "ir.h"

static int appid;
static int browse_appid;

static int w_open;

static char key_bindings[64][384];

static void load_config()
{
	char config_key[6];
	int i;
	const char *val;

	strcpy(config_key, "ir_");
	for(i=0;i<64;i++) {
		sprintf(&config_key[3], "%02x", i);
		val = config_read_string(config_key);
		if(val != NULL)
			strcpy(key_bindings[i], val);
		else
			key_bindings[i][0] = 0;
	}
}

static void set_config()
{
	char config_key[6];
	int i;

	strcpy(config_key, "ir_");
	for(i=0;i<64;i++) {
		sprintf(&config_key[3], "%02x", i);
		if(key_bindings[i][0] != 0)
			config_write_string(config_key, key_bindings[i]);
		else
			config_delete(config_key);
	}
}

static void update_list()
{
	char str[32768];
	char *p;
	int i;

	str[0] = 0;
	p = str;
	for(i=0;i<64;i++) {
		if(key_bindings[i][0] != 0)
			p += sprintf(p, "0x%02x: %s\n", i, key_bindings[i]);
	}
	/* remove last \n */
	if(p != str) {
		p--;
		*p = 0;
	}
	mtk_cmdf(appid, "lst_existing.set(-text \"%s\" -selection 0)", str);
}

static void selchange_callback(mtk_event *e, void *arg)
{
	int sel;
	int count;
	int i;

	sel = mtk_req_i(appid, "lst_existing.selection");
	count = 0;
	for(i=0;i<64;i++) {
		if(key_bindings[i][0] != 0) {
			if(count == sel)
				break;
			count++;
		}
	}
	mtk_cmdf(appid, "e_key.set(-text \"0x%02x\")", i);
	mtk_cmdf(appid, "e_filename.set(-text \"%s\")", key_bindings[i]);
}

static int capturing;

static void ir_event(mtk_event *e, int count)
{
	int i;

	for(i=0;i<count;i++) {
		if(e[i].type == EVENT_TYPE_IR) {
			mtk_cmdf(appid, "e_key.set(-text \"0x%02x\")", e[i].press.code);
			mtk_cmd(appid, "b_key.set(-state off)");
			capturing = 0;
			input_delete_callback(ir_event);
			break;
		}
	}
}

static void capture_callback(mtk_event *e, void *arg)
{
	if(capturing) {
		input_delete_callback(ir_event);
		mtk_cmd(appid, "b_key.set(-state off)");
	} else {
		input_add_callback(ir_event);
		mtk_cmd(appid, "b_key.set(-state on)");
	}
	capturing = !capturing;
}

static void close_window()
{
	mtk_cmd(appid, "w.close()");
	if(capturing)
		capture_callback(NULL, NULL);
	w_open = 0;
	close_filedialog(browse_appid);
}

static void ok_callback(mtk_event *e, void *arg)
{
	set_config();
	cp_notify_changed();
	close_window();
}

static void cancel_callback(mtk_event *e, void *arg)
{
	close_window();
}

static void browse_ok_callback()
{
	char filename[384];

	get_filedialog_selection(browse_appid, filename, sizeof(filename));
	close_filedialog(browse_appid);
	mtk_cmdf(appid, "e_filename.set(-text \"%s\")", filename);
}

static void browse_cancel_callback()
{
	close_filedialog(browse_appid);
}

static void browse_callback(mtk_event *e, void *arg)
{
	open_filedialog(browse_appid, "/");
}

static void clear_callback(mtk_event *e, void *arg)
{
	mtk_cmd(appid, "e_filename.set(-text \"\")");
}

static void addupdate_callback(mtk_event *e, void *arg)
{
	char key[8];
	int index;
	char *c;
	char filename[384];

	mtk_req(appid, key, sizeof(key), "e_key.text");
	mtk_req(appid, filename, sizeof(filename), "e_filename.text");
	index = strtol(key, &c, 0);
	if((*c != 0x00) || (index < 0) || (index > 63)) {
		messagebox("Error", "Invalid key code.\nUse a decimal or hexadecimal (0x...) number between 0 and 63.");
		return;
	}
	strcpy(key_bindings[index], filename);
	update_list();
	mtk_cmd(appid, "e_key.set(-text \"\")");
	mtk_cmd(appid, "e_filename.set(-text \"\")");
}

void init_ir()
{
	appid = mtk_init_app("IR");

	mtk_cmd_seq(appid,
		"g = new Grid()",

		"g_existing0 = new Grid()",
		"l_existing = new Label(-text \"Existing bindings\" -font \"title\")",
		"s_existing1 = new Separator(-vertical no)",
		"s_existing2 = new Separator(-vertical no)",
		"g_existing0.place(s_existing1, -column 1 -row 1)",
		"g_existing0.place(l_existing, -column 2 -row 1)",
		"g_existing0.place(s_existing2, -column 3 -row 1)",
		"lst_existing = new List()",
		"lst_existingf = new Frame(-content lst_existing -scrollx no -scrolly yes)",

		"g_addedit0 = new Grid()",
		"l_addedit = new Label(-text \"Add/edit\" -font \"title\")",
		"s_addedit1 = new Separator(-vertical no)",
		"s_addedit2 = new Separator(-vertical no)",
		"g_addedit0.place(s_addedit1, -column 1 -row 1)",
		"g_addedit0.place(l_addedit, -column 2 -row 1)",
		"g_addedit0.place(s_addedit2, -column 3 -row 1)",
		
		"g_addedit1 = new Grid()",
		"l_key = new Label(-text \"Key code:\")",
		"e_key = new Entry()",
		"b_key = new Button(-text \"Capture\")",
		"g_addedit1.place(l_key, -column 1 -row 1)",
		"g_addedit1.place(e_key, -column 2 -row 1)",
		"g_addedit1.place(b_key, -column 3 -row 1)",
		"l_filename = new Label(-text \"Filename:\")",
		"e_filename = new Entry()",
		"b_filename = new Button(-text \"Browse\")",
		"b_filenameclear = new Button(-text \"Clear\")",
		"g_addedit1.place(l_filename, -column 1 -row 2)",
		"g_addedit1.place(e_filename, -column 2 -row 2)",
		"g_addedit1.place(b_filename, -column 3 -row 2)",
		"g_addedit1.place(b_filenameclear, -column 4 -row 2)",
		"g_addedit1.columnconfig(3, -size 0)",
		"g_addedit1.columnconfig(4, -size 0)",
		"b_addupdate = new Button(-text \"Add/update\")",

		"g_btn = new Grid()",
		"b_ok = new Button(-text \"OK\")",
		"b_cancel = new Button(-text \"Cancel\")",
		"g_btn.columnconfig(1, -size 200)",
		"g_btn.place(b_ok, -column 2 -row 1)",
		"g_btn.place(b_cancel, -column 3 -row 1)",

		"g.place(g_existing0, -column 1 -row 1)",
		"g.place(lst_existingf, -column 1 -row 2)",
		"g.rowconfig(2, -size 130)",
		"g.place(g_addedit0, -column 1 -row 3)",
		"g.place(g_addedit1, -column 1 -row 4)",
		"g.place(b_addupdate, -column 1 -row 5)",
		"g.rowconfig(6, -size 10)",
		"g.place(g_btn, -column 1 -row 7)",

		"w = new Window(-content g -title \"IR remote control settings\")",
		0);

	mtk_bind(appid, "lst_existing", "selchange", selchange_callback, NULL);
	mtk_bind(appid, "lst_existing", "selcommit", selchange_callback, NULL);
	mtk_bind(appid, "b_key", "press", capture_callback, NULL);
	mtk_bind(appid, "b_filename", "commit", browse_callback, NULL);
	mtk_bind(appid, "b_filenameclear", "commit", clear_callback, NULL);
	mtk_bind(appid, "b_addupdate", "commit", addupdate_callback, NULL);

	mtk_bind(appid, "b_ok", "commit", ok_callback, NULL);
	mtk_bind(appid, "b_cancel", "commit", cancel_callback, NULL);

	mtk_bind(appid, "w", "close", cancel_callback, NULL);

	browse_appid = create_filedialog("IR patch select", 0, browse_ok_callback, NULL, browse_cancel_callback, NULL);
}

void open_ir_window()
{
	if(w_open) return;
	w_open = 1;
	load_config();
	update_list();
	mtk_cmd(appid, "w.open()");
}
