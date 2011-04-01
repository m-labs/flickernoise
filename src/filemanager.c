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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <mtklib.h>

#include "filemanager.h"

static int appid;

static int move_mode;

static void mode_callback(mtk_event *e, void *arg)
{
	move_mode = (int)arg;
	mtk_cmdf(appid, "pc_copy.set(-state %s)", !move_mode ? "on" : "off");
	mtk_cmdf(appid, "pc_move.set(-state %s)", move_mode ? "on" : "off");
}

static void close_callback(mtk_event *e, void *arg)
{
	mtk_cmd(appid, "w.close()");
}

void init_filemanager()
{
	appid = mtk_init_app("File manager");

	mtk_cmd_seq(appid,
		"g = new Grid()",
		"s1 = new Separator(-vertical yes)",
		"s2 = new Separator(-vertical yes)",
		
		"p1_g = new Grid()",
		"p1_list = new List()",
		"p1_listf = new Frame(-content p1_list -scrollx yes -scrolly yes)",
		"p1_ig = new Grid()",
		"p1_lname = new Label(-text \"Name:\")",
		"p1_name = new Entry()",
		"p1_lsize = new Label(-text \"Size:\")",
		"p1_size = new Label()",
		"p1_ig.place(p1_lname, -column 1 -row 1)",
		"p1_ig.place(p1_name, -column 2 -row 1)",
		"p1_ig.place(p1_lsize, -column 1 -row 2)",
		"p1_ig.place(p1_size, -column 2 -row 2 -align \"w\")",
		"p1_g.place(p1_listf, -column 1 -row 1)",
		"p1_g.place(p1_ig, -column 1 -row 2)",
		
		"pc_g = new Grid()",
		"pc_copy = new Button(-text \"Copy\" -state on)",
		"pc_move = new Button(-text \"Move\")",
		"pc_to = new Button(-text \"->\")",
		"pc_from = new Button(-text \"<-\")",
		"pc_g.rowconfig(1, -weight 1)",
		"pc_g.place(pc_copy, -row 2 -column 1)",
		"pc_g.place(pc_move, -row 3 -column 1)",
		"pc_g.rowconfig(4, -size 40)",
		"pc_g.place(pc_to, -row 5 -column 1)",
		"pc_g.place(pc_from, -row 6 -column 1)",
		"pc_g.rowconfig(7, -weight 1)",
		
		"p2_g = new Grid()",
		"p2_list = new List()",
		"p2_listf = new Frame(-content p2_list -scrollx yes -scrolly yes)",
		"p2_ig = new Grid()",
		"p2_lname = new Label(-text \"Name:\")",
		"p2_name = new Entry()",
		"p2_lsize = new Label(-text \"Size:\")",
		"p2_size = new Label()",
		"p2_ig.place(p2_lname, -column 1 -row 1)",
		"p2_ig.place(p2_name, -column 2 -row 1)",
		"p2_ig.place(p2_lsize, -column 1 -row 2)",
		"p2_ig.place(p2_size, -column 2 -row 2  -align \"w\")",
		"p2_g.place(p2_listf, -column 1 -row 1)",
		"p2_g.place(p2_ig, -column 1 -row 2)",
		
		"g.place(p1_g, -column 1 -row 1)",
		"g.place(s1, -column 2 -row 1)",
		"g.place(pc_g, -column 3 -row 1)",
		"g.place(s2, -column 4 -row 1)",
		"g.place(p2_g, -column 5 -row 1)",
		
		"g.columnconfig(3, -size 0)",
		
		"w = new Window(-content g -title \"File manager\" -workw 450 -workh 350)",
	0);
	
	mtk_bind(appid, "pc_copy", "press", mode_callback, (void *)0);
	mtk_bind(appid, "pc_move", "press", mode_callback, (void *)1);

	mtk_bind(appid, "w", "close", close_callback, NULL);
}

static int cmpstringp(const void *p1, const void *p2)
{
	return strcmp(*(char * const *)p1, *(char * const *)p2);
}

static void display_folder(int panel, const char *folder)
{
	char *folders[384];
	int n_folders;
	char *files[384];
	int n_files;
	int i;

	char fmt_list[16385];
	char *c_list;
	char fullname[384];
	DIR *d;
	struct dirent *entry;
	struct stat s;
	int len;

	d = opendir(folder);
	if(!d) return;
	n_folders = 0;
	n_files = 0;
	while((entry = readdir(d))) {
		if(entry->d_name[0] == '.') continue;
		strncpy(fullname, folder, sizeof(fullname));
		strncat(fullname, entry->d_name, sizeof(fullname));
		lstat(fullname, &s);
		if(S_ISDIR(s.st_mode)) {
			/* hide /dev */
			if((strcmp(folder, "/") != 0) || (strcmp(entry->d_name, "dev") != 0)) {
				if(n_folders < 384) {
					folders[n_folders] = strdup(entry->d_name);
					n_folders++;
				}
			}
		} else {
			files[n_files] = strdup(entry->d_name);
			n_files++;
		}
	}
	closedir(d);
	
	qsort(folders, n_folders, sizeof(char *), cmpstringp);
	qsort(files, n_files, sizeof(char *), cmpstringp);
	
	strcpy(fmt_list, "../");
	c_list = fmt_list + 3;
	for(i=0;i<n_folders;i++) {
		len = strlen(folders[i]);
		if((c_list-fmt_list)+len+3 > sizeof(fmt_list)) break;
		*c_list++ = '\n';
		strcpy(c_list, folders[i]);
		free(folders[i]);
		c_list += len;
		*c_list++ = '/';
		*c_list = 0;
	}
	for(i=0;i<n_files;i++) {
		len = strlen(files[i]);
		if((c_list-fmt_list)+len+2 > sizeof(fmt_list)) break;
		if(c_list != fmt_list)
			*c_list++ = '\n';
		strcpy(c_list, files[i]);
		free(files[i]);
		c_list += len;
		*c_list = 0;
	}

//	mtk_cmdf(appid, "p%d_lfolder.set(-text \"%s\")", panel, folder);
	mtk_cmdf(appid, "p%d_list.set(-text \"%s\" -selection 0)", panel, fmt_list);
	mtk_cmdf(appid, "p%d_listf.expose(0, 0)", panel);
}

void open_filemanager_window()
{
	display_folder(1, "/");
	display_folder(2, "/flash/");
	mtk_cmd(appid, "w.open()");
}
