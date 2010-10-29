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

#include <mtklib.h>

#include "monitor.h"

static long appid;

static void close_callback(mtk_event *e, void *arg)
{
	mtk_cmd(appid, "w.close()");
}

void init_monitor()
{
	int i;
	int column;
	int brow;

	appid = mtk_init_app("Monitor");

	mtk_cmd(appid, "g = new Grid()");
	for(i=0;i<8;i++) {
		column = i > 3 ? 3 : 1;
		brow = i & 3;
		mtk_cmdf(appid, "var%d = new Entry()", i);
		mtk_cmdf(appid, "val%d = new Label(-text \"N/A\")", i);
		mtk_cmdf(appid, "g.place(var%d, -column %d -row %d)", i, column, 3*brow+1);
		mtk_cmdf(appid, "g.place(val%d, -column %d -row %d)", i, column, 3*brow+2);
		/* Put a horizontal separator everywhere except for the last row */
		if(brow != 3)  {
			mtk_cmdf(appid, "sep%d = new Separator(-vertical no)", i);
			mtk_cmdf(appid, "g.place(sep%d, -column %d -row %d)", i, column, 3*brow+3);
		}
	}
	for(i=0;i<11;i++) {
		mtk_cmdf(appid, "vsep%d = new Separator(-vertical yes)", i);
		mtk_cmdf(appid, "g.place(vsep%d, -column 2 -row %d)", i, i+1);
	}
	mtk_cmd(appid, "g.columnconfig(1, -weight 1 -size 100)");
	mtk_cmd(appid, "g.columnconfig(3, -weight 1 -size 100)");
	mtk_cmd(appid, "w = new Window(-content g -title \"Variable monitor\")");

	mtk_bind(appid, "w", "close", close_callback, NULL);
}

void open_monitor_window()
{
	mtk_cmd(appid, "w.open()");
}
