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

#include <stdlib.h>

#include <mtklib.h>

#include "messagebox.h"

static int appid;

static void close_callback(mtk_event *e, void *arg)
{
	mtk_cmd(appid, "w.close()");
}

void init_messagebox()
{
	appid = mtk_init_app("Messagebox");
	mtk_cmd_seq(appid,
		"g = new Grid()",
		"l = new Label(-text \"text\")",
		"b_ok = new Button(-text \"OK\")",
		"g.place(l, -column 1 -row 1)",
		"g.rowconfig(1, -size 50)",
		"g.place(b_ok, -column 1 -row 2)",
		"w = new Window(-content g -title \"title\")",
		0);

	mtk_bind(appid, "b_ok", "commit", close_callback, NULL);
	mtk_bind(appid, "w", "close", close_callback, NULL);
}

void messagebox(const char *title, const char *text)
{
	mtk_cmdf(appid, "w.set(-title \"%s\")", title);
	mtk_cmdf(appid, "l.set(-text \"%s\")", text);
	mtk_cmd(appid, "w.open()");
}
