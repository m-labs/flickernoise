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
#include <mtklib.h>

#include "config.h"
#include "cp.h"
#include "messagebox.h"
#include "filedialog.h"

#include "keyboard.h"

static int appid;
static int browse_appid;

static int w_open;

static void ok_callback(mtk_event *e, void *arg)
{
	char filename[384];

	mtk_cmd(appid, "w.close()");
	w_open = 0;
	cp_notify_changed();
}

static void cancel_callback(mtk_event *e, void *arg)
{
	mtk_cmd(appid, "w.close()");
	w_open = 0;
}

void init_keyboard()
{
	appid = mtk_init_app("Keyboard");

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

		"g_addedit0 = new Grid()",
		"l_addedit = new Label(-text \"Add/edit\" -font \"title\")",
		"s_addedit1 = new Separator(-vertical no)",
		"s_addedit2 = new Separator(-vertical no)",
		"g_addedit0.place(s_addedit1, -column 1 -row 1)",
		"g_addedit0.place(l_addedit, -column 2 -row 1)",
		"g_addedit0.place(s_addedit2, -column 3 -row 1)",
		
		"g_addedit1 = new Grid()",
		"l_key = new Label(-text \"Key:\")",
		"e_key= new Entry()",
		"g_addedit1.place(l_key, -column 1 -row 1)",
		"g_addedit1.place(e_key, -column 2 -row 1)",
		"l_filename = new Label(-text \"Filename:\")",
		"e_filename = new Entry()",
		"b_filename = new Button(-text \"Browse\")",
		"g_addedit1.place(l_filename, -column 1 -row 2)",
		"g_addedit1.place(e_filename, -column 2 -row 2)",
		"g_addedit1.place(b_filename, -column 3 -row 2)",
		"b_addupdate = new Button(-text \"Add/update\")",

		"g_btn = new Grid()",
		"b_ok = new Button(-text \"OK\")",
		"b_cancel = new Button(-text \"Cancel\")",
		"g_btn.columnconfig(1, -size 200)",
		"g_btn.place(b_ok, -column 2 -row 1)",
		"g_btn.place(b_cancel, -column 3 -row 1)",

		"g.place(g_existing0, -column 1 -row 1)",
		"g.place(lst_existing, -column 1 -row 2)",
		"g.place(g_addedit0, -column 1 -row 3)",
		"g.place(g_addedit1, -column 1 -row 4)",
		"g.place(b_addupdate, -column 1 -row 5)",
		"g.rowconfig(6, -size 10)",
		"g.place(g_btn, -column 1 -row 7)",

		"w = new Window(-content g -title \"Keyboard settings\")",
		0);

	mtk_bind(appid, "b_ok", "commit", ok_callback, NULL);
	mtk_bind(appid, "b_cancel", "commit", cancel_callback, NULL);

	mtk_bind(appid, "w", "close", cancel_callback, NULL);
}

void open_keyboard_window()
{
	if(w_open) return;
	w_open = 1;
	mtk_cmd(appid, "w.open()");
}
