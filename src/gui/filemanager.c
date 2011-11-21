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

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <mtklib.h>

#include "filemanager.h"

/* Ugly APIs need ugly hacks. */
int rtems_shell_main_cp(int argc, char *argv[]);
int rtems_shell_main_rm(int argc, char *argv[]);
int rtems_shell_main_mv(int argc, char *argv[]);

static void call_cp(char *from, char *to)
{
	char *cp_argv[6] = { "cp", "-PRp", "--", from, to, NULL };
	int r;
	
	r = rtems_shell_main_cp(5, cp_argv);
	#ifdef DEBUG
	printf("CP: '%s' -> '%s': %d\n", from, to, r);
	#endif
}

static void call_rm(char *from)
{
	char *rm_argv[5] = { "rm", "-r", "--", from, NULL };
	int r;
	
	r = rtems_shell_main_rm(4, rm_argv);
	#ifdef DEBUG
	printf("RM: '%s': %d\n", from, r);
	#endif
}

static void call_mv(char *from, char *to)
{
	char *mv_argv[5] = { "mv", "--", from, to, NULL };
	int r;
	
	r = rtems_shell_main_mv(4, mv_argv);
	#ifdef DEBUG
	printf("MV: '%s' -> '%s': %d\n", from, to, r);
	#endif
}
/* */

static int appid;

static int move_mode;

static void display_folder(int panel, const char *folder);
static void refresh(int panel);

static void mode_callback(mtk_event *e, void *arg)
{
	move_mode = (int)arg;
	mtk_cmdf(appid, "pc_copy.set(-state %s)", !move_mode ? "on" : "off");
	mtk_cmdf(appid, "pc_move.set(-state %s)", move_mode ? "on" : "off");
}

static char *get_selection(char *list, int n)
{
	int i;
	char *s;

	s = list;
	i = 0;
	while(*list != 0) {
		if(*list == '\n') {
			*list = 0;
			if(i == n) break;
			i++;
			s = list+1;
		}
		list++;
	}
	return s;
}

static char *get_selection_panel(int panel)
{
	char filelist[16384];
	char num[8];
	int nselection;
	char *selection;

	mtk_reqf(appid, filelist, sizeof(filelist), "p%d_list.text", panel);
	if(strlen(filelist) == 0) return NULL;
	mtk_reqf(appid, num, sizeof(num), "p%d_list.selection", panel);
	nselection = atoi(num);
	selection = strdup(get_selection(filelist, nselection));
	assert(selection != NULL);
	return selection;
}

static void get_lfolder(char *buf, int size, int sel)
{
	mtk_reqf(appid, buf, size, "p%d_lfolder.text", sel);
	memmove(buf, buf+1, size-1); /* remove \e */
}

static void copymove(int src, int dst)
{
	char orig[384];
	char copied[384];
	char basename[384];
	int last;
	
	get_lfolder(orig, sizeof(orig), src);
	get_lfolder(copied, sizeof(copied), dst);
	mtk_reqf(appid, basename, sizeof(basename), "p%d_name.text", src);
	last = strlen(basename)-1;
	if(basename[last] == '/')
		basename[last] = 0;
	strncat(orig, basename, sizeof(orig));
	if(move_mode)
		call_mv(orig, copied);
	else {
		strncat(copied, basename, sizeof(orig));
		call_cp(orig, copied);
	}
	refresh(src);
	refresh(dst);
}

static void to_callback(mtk_event *e, void *arg)
{
	copymove(0, 1);
}

static void from_callback(mtk_event *e, void *arg)
{
	copymove(1, 0);
}

static void update_filename(int panel)
{
	char *selection;

	selection = get_selection_panel(panel);
	if(selection != NULL) {
		mtk_cmdf(appid, "p%d_name.set(-text \"%s\")", panel, selection);
		free(selection);
	}
}

static void selchange_callback(mtk_event *e, void *arg)
{
	int panel = (int)arg;
	update_filename(panel);
}

static void selcommit_callback(mtk_event *e, void *arg)
{
	int panel = (int)arg;
	char curfolder[384];
	char sel[384];
	
	update_filename(panel);
	mtk_reqf(appid, sel, sizeof(sel), "p%d_name.text", panel);
	if(sel[strlen(sel)-1] == '/') {
		get_lfolder(curfolder, sizeof(curfolder), panel);
		if(strcmp(sel, "../") == 0) {
			char *c;
			if(strcmp(curfolder, "/") == 0) return;
			*(curfolder+strlen(curfolder)-1) = 0;
			c = strrchr(curfolder, '/');
			c++;
			*c = 0;
		} else
			strncat(curfolder, sel, sizeof(curfolder));

		display_folder(panel, curfolder);
	}
}

static void clear_callback(mtk_event *e, void *arg)
{
	int panel = (int)arg;
	mtk_cmdf(appid, "p%d_name.set(-text \"\")", panel);
}

static void refresh(int panel)
{
	char curfolder[384];
	get_lfolder(curfolder, sizeof(curfolder), panel);
	display_folder(panel, curfolder);
}

static void rename_callback(mtk_event *e, void *arg)
{
	int panel = (int)arg;
	char orig[384];
	char renamed[384];
	char *selection;
	
	get_lfolder(orig, sizeof(orig), panel);
	strcpy(renamed, orig);
	selection = get_selection_panel(panel);
	if(selection == NULL) return;
	strncat(orig, selection, sizeof(orig));
	free(selection);
	mtk_reqf(appid, renamed+strlen(renamed), sizeof(renamed)-strlen(renamed), "p%d_name.text", panel);
	if(renamed[strlen(renamed)-1] == '/')
		renamed[strlen(renamed)-1] = 0;
	call_mv(orig, renamed);
	refresh(panel);
}

static int delete_confirm[2];

static void delete_callback(mtk_event *e, void *arg)
{
	int panel = (int)arg;
	char target[384];
	int last;
	
	if(delete_confirm[panel]) {
		get_lfolder(target, sizeof(target), panel);
		mtk_reqf(appid, target+strlen(target), sizeof(target)-strlen(target), "p%d_name.text", panel);
		last = strlen(target)-1;
		if(target[last] == '/')
			target[last] = 0;
		call_rm(target);
		refresh(panel);
		mtk_cmdf(appid, "p%d_delete.set(-text \"Delete\")", panel);
		delete_confirm[panel] = 0;
	} else {
		mtk_cmdf(appid, "p%d_delete.set(-text \"Sure?\")", panel);
		delete_confirm[panel] = 1;
	}
}

static void delete_leave_callback(mtk_event *e, void *arg)
{
	int panel = (int)arg;
	mtk_cmdf(appid, "p%d_delete.set(-text \"Delete\")", panel);
	delete_confirm[panel] = 0;
}

static void mkdir_callback(mtk_event *e, void *arg)
{
	int panel = (int)arg;
	char dirname[384];
	
	get_lfolder(dirname, sizeof(dirname), panel);
	mtk_reqf(appid, dirname+strlen(dirname), sizeof(dirname)-strlen(dirname), "p%d_name.text", panel);
	mkdir(dirname, 0777);
	refresh(panel);
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
		
		"p0_g = new Grid()",
		"p0_lfolder = new Label()",
		"p0_list = new List()",
		"p0_listf = new Frame(-content p0_list -scrollx yes -scrolly yes)",
		"p0_ig = new Grid()",
		"p0_lname = new Label(-text \"Name:\")",
		"p0_name = new Entry()",
		"p0_ig.place(p0_lname, -column 1 -row 1)",
		"p0_ig.place(p0_name, -column 2 -row 1)",
		"p0_bg = new Grid()",
		"p0_clear = new Button(-text \"Clear\")",
		"p0_rename = new Button(-text \"Rename\")",
		"p0_delete = new Button(-text \"Delete\")",
		"p0_mkdir = new Button(-text \"Mkdir\")",
		"p0_bg.place(p0_clear, -column 1 -row 1)",
		"p0_bg.place(p0_rename, -column 2 -row 1)",
		"p0_bg.place(p0_delete, -column 3 -row 1)",
		"p0_bg.place(p0_mkdir, -column 4 -row 1)",
		"p0_g.place(p0_lfolder, -column 1 -row 1 -align \"w\")",
		"p0_g.rowconfig(1, -size 0)",
		"p0_g.place(p0_listf, -column 1 -row 2)",
		"p0_g.place(p0_ig, -column 1 -row 3)",
		"p0_g.place(p0_bg, -column 1 -row 4)",
		
		"pc_g = new Grid()",
		"pc_copy = new Button(-text \"Copy\" -state on)",
		"pc_move = new Button(-text \"Move\")",
		"pc_to = new Button(-text \"\e->\")",
		"pc_from = new Button(-text \"\e<-\")",
		"pc_g.rowconfig(1, -weight 1)",
		"pc_g.place(pc_copy, -row 2 -column 1)",
		"pc_g.place(pc_move, -row 3 -column 1)",
		"pc_g.rowconfig(4, -size 40)",
		"pc_g.place(pc_to, -row 5 -column 1)",
		"pc_g.place(pc_from, -row 6 -column 1)",
		"pc_g.rowconfig(7, -weight 1)",
		
		"p1_g = new Grid()",
		"p1_lfolder = new Label()",
		"p1_list = new List()",
		"p1_listf = new Frame(-content p1_list -scrollx yes -scrolly yes)",
		"p1_ig = new Grid()",
		"p1_lname = new Label(-text \"Name:\")",
		"p1_name = new Entry()",
		"p1_ig.place(p1_lname, -column 1 -row 1)",
		"p1_ig.place(p1_name, -column 2 -row 1)",
		"p1_bg = new Grid()",
		"p1_clear = new Button(-text \"Clear\")",
		"p1_rename = new Button(-text \"Rename\")",
		"p1_delete = new Button(-text \"Delete\")",
		"p1_mkdir = new Button(-text \"Mkdir\")",
		"p1_bg.place(p1_clear, -column 1 -row 1)",
		"p1_bg.place(p1_rename, -column 2 -row 1)",
		"p1_bg.place(p1_delete, -column 3 -row 1)",
		"p1_bg.place(p1_mkdir, -column 4 -row 1)",
		"p1_g.place(p1_lfolder, -column 1 -row 1 -align \"w\")",
		"p1_g.rowconfig(1, -size 0)",
		"p1_g.place(p1_listf, -column 1 -row 2)",
		"p1_g.place(p1_ig, -column 1 -row 3)",
		"p1_g.place(p1_bg, -column 1 -row 4)",
		
		"g.place(p0_g, -column 1 -row 1)",
		"g.place(s1, -column 2 -row 1)",
		"g.place(pc_g, -column 3 -row 1)",
		"g.place(s2, -column 4 -row 1)",
		"g.place(p1_g, -column 5 -row 1)",
		
		"g.columnconfig(3, -size 0)",
		
		"w = new Window(-content g -title \"File manager\" -workx 20 -workh 350)",
	0);
	
	mtk_bind(appid, "pc_copy", "press", mode_callback, (void *)0);
	mtk_bind(appid, "pc_move", "press", mode_callback, (void *)1);
	mtk_bind(appid, "pc_to", "press", to_callback, NULL);
	mtk_bind(appid, "pc_from", "press", from_callback, NULL);
	
	mtk_bind(appid, "p0_list", "selchange", selchange_callback, (void *)0);
	mtk_bind(appid, "p0_list", "selcommit", selcommit_callback, (void *)0);
	mtk_bind(appid, "p1_list", "selchange", selchange_callback, (void *)1);
	mtk_bind(appid, "p1_list", "selcommit", selcommit_callback, (void *)1);

	mtk_bind(appid, "p0_clear", "commit", clear_callback, (void *)0);
	mtk_bind(appid, "p1_clear", "commit", clear_callback, (void *)1);
	mtk_bind(appid, "p0_rename", "commit", rename_callback, (void *)0);
	mtk_bind(appid, "p1_rename", "commit", rename_callback, (void *)1);
	mtk_bind(appid, "p0_delete", "commit", delete_callback, (void *)0);
	mtk_bind(appid, "p1_delete", "commit", delete_callback, (void *)1);
	mtk_bind(appid, "p0_delete", "leave", delete_leave_callback, (void *)0);
	mtk_bind(appid, "p1_delete", "leave", delete_leave_callback, (void *)1);
	mtk_bind(appid, "p0_mkdir", "commit", mkdir_callback, (void *)0);
	mtk_bind(appid, "p1_mkdir", "commit", mkdir_callback, (void *)1);

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

	mtk_cmdf(appid, "p%d_lfolder.set(-text \"\e%s\")", panel, folder);
	mtk_cmdf(appid, "p%d_list.set(-text \"%s\" -selection 0)", panel, fmt_list);
	mtk_cmdf(appid, "p%d_listf.expose(0, 0)", panel);
}

void open_filemanager_window()
{
	display_folder(0, "/");
	display_folder(1, "/");
	mtk_cmd(appid, "w.open()");
}
