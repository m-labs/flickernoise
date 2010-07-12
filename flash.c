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

#include <dopelib.h>
#include <vscreen.h>

#include "flash.h"
#include "filedialog.h"

static long appid;
static long fileDialog_id;

static void cancel_callback(dope_event *e, void *arg)
{
	dope_cmd(appid, "w.close()");
}

void flash_filedialog_ok_callback()
{
	char filepath[384];
	get_filedialog_selection(fileDialog_id, filepath, sizeof(filepath));

	char dope_cmd_str [384];
	sprintf(dope_cmd_str, "e1.set(-text \"%s\")", filepath);
	dope_cmd(appid, dope_cmd_str);

	char name[384];
	dope_req(appid, name, sizeof(name), "w.title");

	close_filedialog(fileDialog_id);
}

void flash_filedialog_cancel_callback()
{
	close_filedialog(fileDialog_id);
}

static void flash_callback(dope_event *e, void *arg)
{	
	switch ((int) arg)
	{
		case 0:
			break;
	
		case 1:
			fileDialog_id = create_filedialog("Bitstream path", 0, flash_filedialog_ok_callback, NULL, flash_filedialog_cancel_callback, NULL);
			open_filedialog(fileDialog_id, "/");
			break;
		case 2:
			fileDialog_id = create_filedialog("Bios path", 0, flash_filedialog_ok_callback, NULL, flash_filedialog_cancel_callback, NULL);
			open_filedialog(fileDialog_id, "/");
			break;
		case 3:
			fileDialog_id = create_filedialog("Program path", 0, flash_filedialog_ok_callback, NULL, flash_filedialog_cancel_callback, NULL);
			open_filedialog(fileDialog_id, "/");
			break;
		case 4:
			printf("Flash device\n");
			// TODO : check files and Flash it into Milkyboard

			char name[384];
			dope_req(appid, name, sizeof(name), "e1.text");
			printf("Bitstream :\t%s\n", name);
			dope_req(appid, name, sizeof(name), "e2.text");
			printf("Bios :\t%s\n", name);
			dope_req(appid, name, sizeof(name), "e3.text");
			printf("Program :\t%s\n", name);

			dope_cmd(appid, "w.close()");
			break;
		default:
			break;
	}
}

void init_flash()
{
	appid = dope_init_app("Flash");

	dope_cmd_seq(appid,
		"g = new Grid()",
		"g.columnconfig(2, -size 100)",
		"g.columnconfig(2, -size 250)",

		"l0 = new Label(-text \"Select binaries files.\")",
		"l1 = new Label(-text \"Bitstream Path : \")",
		"l2 = new Label(-text \"Bios Path : \")",
		"l3 = new Label(-text \"Program Path : \")",

		"e1 = new Entry()",
		"e2 = new Entry()",
		"e3 = new Entry()",

		"b_change1 = new Button(-text \"Change\")",
		"b_change2 = new Button(-text \"Change\")",
		"b_change3 = new Button(-text \"Change\")",
		"b_cancel = new Button(-text \"Cancel\")",
		"b_ok = new Button(-text \"Ok\")",

		"g.place(l0, -column 1 -row 1 -align w)",
		"g.place(l1, -column 1 -row 2 -align w)",
		"g.place(l2, -column 1 -row 3 -align w)",
		"g.place(l3, -column 1 -row 4 -align w)",

		"g.place(e1, -column 2 -row 2)",
		"g.place(e2, -column 2 -row 3)",
		"g.place(e3, -column 2 -row 4)",

		"g.place(b_change1, -column 3 -row 2)",
		"g.place(b_change2, -column 3 -row 3)",
		"g.place(b_change3, -column 3 -row 4)",

		"g.place(b_cancel, -column 2 -row 5)",
		"g.place(b_ok, -column 3 -row 5)",
		
		"w = new Window(-content g -title \"Flash milkymist\")",
		0);

	dope_bind(appid, "b_change1", "clack", flash_callback, (void *)1);
	dope_bind(appid, "b_change2", "clack", flash_callback, (void *)2);
	dope_bind(appid, "b_change3", "clack", flash_callback, (void *)3);
	dope_bind(appid, "b_cancel", "clack", cancel_callback, NULL);
	dope_bind(appid, "b_ok", "clack", flash_callback, (void *)4);

	dope_bind(appid, "w", "close", cancel_callback, NULL);
}

void open_flash_window()
{
	dope_cmd(appid, "w.open()");
}
