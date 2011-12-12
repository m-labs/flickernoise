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
#include <rtems.h>
#include <rtems/libio.h>

#include <mtklib.h>

#include "shutdown.h"

static int appid;

void clean_shutdown(int turnoff)
{
	unmount("/memcard");
	unmount("/ssd");
	rtems_shutdown_executive(turnoff);
}

static void cancel_callback(mtk_event *e, void *arg)
{
	mtk_cmd(appid, "w.close()");
}

static void shutdown_callback(mtk_event *e, void *arg)
{
	int turnoff = (int)arg;
	clean_shutdown(turnoff);
}

void init_shutdown(void)
{
	appid = mtk_init_app("Shutdown");
	mtk_cmd_seq(appid,
		"g = new Grid()",
		"g2 = new Grid()",
		"l = new Label(-text \"Are you sure?\")",
		"b_shutdown = new Button(-text \"Reboot\")",
		"b_cancel = new Button(-text \"Cancel\")",
		"g.place(l, -column 1 -row 1)",
		"g.rowconfig(1, -size 50)",
		"g2.place(b_shutdown, -column 1 -row 1)",
		"g2.place(b_cancel, -column 2 -row 1)",
		"g.place(g2, -column 1 -row 2)",
		"w = new Window(-content g -title \"Shutdown\")",
		0);

	mtk_bind(appid, "b_shutdown", "clack", shutdown_callback, (void *)1);

	mtk_bind(appid, "b_cancel", "clack", cancel_callback, NULL);
	mtk_bind(appid, "w", "close", cancel_callback, NULL);
}

void open_shutdown_window(void)
{
	mtk_cmd(appid, "w.open()");
}
