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
#include <dopelib.h>
#include <vscreen.h>

#include "version.h"
#include "flash.h"
#include "about.h"

static long appid;

enum {
	AB_ITEM_CLOSE,
	AB_ITEM_FLASH
};

static void cp_callback(dope_event *e, void *arg)
{
	switch((int)arg) {
		case AB_ITEM_CLOSE:
			dope_cmd(appid, "w.close()");
			break;
		case AB_ITEM_FLASH:
			open_flash_window();
			break;
	}
}

void init_about()
{
	appid = dope_init_app("About");

	dope_cmd_seq(appid,
		"g = new Grid()",

		"flickernoise = new Label(-text \"Flickernoise "VERSION" (built on "__DATE__")\")",
		"rtems = new Label(-text \"OS: RTEMS "RTEMS_VERSION"\")",
		"bios = new Label(-text \"BIOS: Milkymist BIOS 1.0rc1\")",
		"platform = new Label(-text \"Platform: Milkymist SoC 1.0rc1\")",
		"cpu = new Label(-text \"CPU: LatticeMico32 3.5\")",
		"hw = new Label(-text \"Hardware: Milkymist One (PCB rev. 0)\")",
		"sep1 = new Separator(-vertical no)",
		"mac = new Label(-text \"Ethernet MAC: 00:1e:87:da:5a:bc\")",
		"sep2 = new Separator(-vertical no)",

		"g.place(flickernoise, -column 1 -row 1)",
		"g.place(rtems, -column 1 -row 2)",
		"g.place(bios, -column 1 -row 3)",
		"g.place(platform, -column 1 -row 4)",
		"g.place(cpu, -column 1 -row 5)",
		"g.place(hw, -column 1 -row 6)",
		"g.place(sep1, -column 1 -row 7)",
		"g.place(mac, -column 1 -row 8)",
		"g.place(sep2, -column 1 -row 9)",

		"g_btn = new Grid()",

		"b_flash = new Button(-text \"Flash\")",
		"b_close = new Button(-text \"Close\")",

		"g_btn.place(b_flash, -column 1 -row 1)",
		"g_btn.place(b_close, -column 2 -row 1)",

		"g.place(g_btn, -column 1 -row 10)",

		"w = new Window(-content g -title \"About\")",
		0);


	dope_bind(appid, "b_flash", "commit", cp_callback, (void *)AB_ITEM_FLASH);
	dope_bind(appid, "b_close", "commit", cp_callback, (void *)AB_ITEM_CLOSE);

	dope_bind(appid, "w", "close", cp_callback, (void *)AB_ITEM_CLOSE);
}

void open_about_window()
{
	dope_cmd(appid, "w.open()");
}
