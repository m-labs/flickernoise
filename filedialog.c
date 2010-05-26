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

#include <dopelib.h>

#include "filedialog.h"

void create_filedialog(long appid, const char *name, int is_save, void (*ok_callback)(dope_event *,void *), void (*cancel_callback)(dope_event *,void *))
{
	dope_cmdf(appid, "%s_g = new Grid()", name);
	
	dope_cmdf(appid, "%s_g1 = new Grid()", name);
	dope_cmdf(appid, "%s_g1_s1 = new Separator(-vertical no)", name);
	dope_cmdf(appid, "%s_g1_l = new Label(-text \"/\")", name);
	dope_cmdf(appid, "%s_g1_s2 = new Separator(-vertical no)", name);
	dope_cmdf(appid, "%s_g1.place(%s_g1_s1, -column 1 -row 1)", name, name);
	dope_cmdf(appid, "%s_g1.place(%s_g1_l, -column 2 -row 1)", name, name);
	dope_cmdf(appid, "%s_g1.place(%s_g1_s2, -column 3 -row 1)", name, name);

	dope_cmdf(appid, "%s_g2 = new Grid()", name);
	dope_cmdf(appid, "%s_g2_folders = new List(-text \"folders\nldkfjssssssssssssssddddddddddddddddddddddddddddddddfffffffffffffffffffffffffffffffffffffffffffgggggggggggggggh\nsjhfhf\nhdhfh\nhdkfhf\ndfggujudsh\ndhfgfghje\ndhfg\nhdgf\ndfgg\ndhfgf\ndhfht\ndhyyuf\ndhgfu\nidshyftg\nskeuryyf\ndsjufy\ndhryty\nurytfg\nsheyd\nhsgfry\")", name);
	dope_cmdf(appid, "%s_g2_foldersf = new Frame(-content %s_g2_folders -scrollx yes -scrolly yes)", name, name);
	dope_cmdf(appid, "%s_g2_files = new List(-text \"files\")", name);
	dope_cmdf(appid, "%s_g2_filesf = new Frame(-content %s_g2_files -scrollx yes -scrolly yes)", name, name);
	dope_cmdf(appid, "%s_g2.place(%s_g2_foldersf, -column 1 -row 1)", name, name);
	dope_cmdf(appid, "%s_g2.place(%s_g2_filesf, -column 2 -row 1)", name, name);
	dope_cmdf(appid, "%s_g2.rowconfig(1, -size 200)", name);

	dope_cmdf(appid, "%s_selection = new Label(-text \"Selection: /\")", name);

	dope_cmdf(appid, "%s_filename = new Entry()", name);

	dope_cmdf(appid, "%s_g3 = new Grid()", name);
	dope_cmdf(appid, "%s_g3_ok = new Button(-text \"OK\")", name);
	dope_cmdf(appid, "%s_g3_cancel = new Button(-text \"Cancel\")", name);
	dope_cmdf(appid, "%s_g3.columnconfig(1, -size 230)", name);
	dope_cmdf(appid, "%s_g3.place(%s_g3_ok, -column 2 -row 1)", name, name);
	dope_cmdf(appid, "%s_g3.place(%s_g3_cancel, -column 3 -row 1)", name, name);

	dope_cmdf(appid, "%s_g.place(%s_g1, -column 1 -row 1)", name, name);
	dope_cmdf(appid, "%s_g.place(%s_g2, -column 1 -row 2)", name, name);
	dope_cmdf(appid, "%s_g.place(%s_selection, -column 1 -row 3 -align \"w\")", name, name);
	dope_cmdf(appid, "%s_g.place(%s_filename, -column 1 -row 4)", name, name);
	dope_cmdf(appid, "%s_g.place(%s_g3, -column 1 -row 5)", name, name);
	dope_cmdf(appid, "%s_g.rowconfig(3, -size 0)", name);
	
	dope_cmdf(appid, "%s = new Window(-content %s_g -title \"%s\")", name, name, is_save ? "Save As" : "Open");

	//dope_bindf(appid, "%s_g3_ok", "commit", ok_callback, (void *)name, name);
	//dope_bindf(appid, "%s_g3_cancel", "commit", cancel_callback, (void *)name, name);
	//dope_bindf(appid, "%s", "close", cancel_callback, (void *)name, name);
}

void open_filedialog(long appid, const char *name)
{
	dope_cmdf(appid, "%s.open()", name);
}

void close_filedialog(long appid, const char *name)
{
	dope_cmdf(appid, "%s.close()", name);
}

void get_filedialog_selection(long appid, const char *name, char *buffer, int buflen)
{
}
