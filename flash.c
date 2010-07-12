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
static long file_dialog_id;
static long current_file_to_choose;

static void cancel_callback(dope_event *e, void *arg)
{
	dope_cmd(appid, "w.close()");
}

void flash_filedialog_ok_callback()
{
	char dope_cmd_str [384];
	char filepath[384];

	get_filedialog_selection(file_dialog_id, filepath, sizeof(filepath));

	switch(current_file_to_choose){
		case 1:
			sprintf(dope_cmd_str, "e1.set(-text \"%s\")", filepath);
			break;
		case 2:
			sprintf(dope_cmd_str, "e2.set(-text \"%s\")", filepath);
			break;
		case 3:
			sprintf(dope_cmd_str, "e3.set(-text \"%s\")", filepath);
			break;
		default:
			sprintf(dope_cmd_str, " ");
			break;
		}

	dope_cmd(appid, dope_cmd_str);
	close_filedialog(file_dialog_id);
}

void flash_filedialog_cancel_callback()
{
	close_filedialog(file_dialog_id);
}

static void flash_callback(dope_event *e, void *arg)
{		
	char name[384];

	switch ((int) arg){
		case 0:
			break;
	
		case 1:
			current_file_to_choose = 1;
			file_dialog_id = create_filedialog("Bitstream path", 0, flash_filedialog_ok_callback, NULL, flash_filedialog_cancel_callback, NULL);
			open_filedialog(file_dialog_id, "/");
			break;
		case 2:
			current_file_to_choose = 2;
			file_dialog_id = create_filedialog("Bios path", 0, flash_filedialog_ok_callback, NULL, flash_filedialog_cancel_callback, NULL);
			open_filedialog(file_dialog_id, "/");
			break;
		case 3:
			current_file_to_choose = 3;
			file_dialog_id = create_filedialog("Program path", 0, flash_filedialog_ok_callback, NULL, flash_filedialog_cancel_callback, NULL);
			open_filedialog(file_dialog_id, "/");
			break;
		case 4:
			// TODO : check files and Flash it into Milkyboard
			
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
