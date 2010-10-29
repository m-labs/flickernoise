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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <mtklib.h>

#include "filedialog.h"

static void display_folder(long appid, const char *folder)
{
	char fmt_folders[8192];
	char *c_folders;
	char fmt_files[8192];
	char *c_files;
	char fullname[384];
	DIR *d;
	struct dirent *entry;
	struct stat s;
	int len;

	// TODO: sort output
	d = opendir(folder);
	if(!d) return;
	strcpy(fmt_folders, "../");
	c_folders = fmt_folders + 3;
	fmt_files[0] = 0;
	c_files = fmt_files;
	while((entry = readdir(d))) {
		if(entry->d_name[0] == '.') continue;
		strncpy(fullname, folder, sizeof(fullname));
		strncat(fullname, entry->d_name, sizeof(fullname));
		lstat(fullname, &s);
		len = strlen(entry->d_name);
		if(S_ISDIR(s.st_mode)) {
			if((c_folders-fmt_folders)+len+3 > sizeof(fmt_folders)) break;
			*c_folders++ = '\n';
			strcpy(c_folders, entry->d_name);
			c_folders += len;
			*c_folders++ = '/';
			*c_folders = 0;
		} else {
			if((c_files-fmt_files)+len+2 > sizeof(fmt_files)) break;
			if(c_files != fmt_files)
				*c_files++ = '\n';
			strcpy(c_files, entry->d_name);
			c_files += len;
			*c_files = 0;
		}
	}
	closedir(d);


	mtk_cmdf(appid, "fd_g2_folders.set(-text \"%s\" -selection 0)", fmt_folders);
	mtk_cmdf(appid, "fd_g2_files.set(-text \"%s\" -selection 0)", fmt_files);
	mtk_cmdf(appid, "fd_g1_l.set(-text \"%s\")", folder);
	mtk_cmdf(appid, "fd_selection.set(-text \"Selection: %s\")", folder);
	mtk_cmd(appid, "fd_g2_foldersf.set(-xview 0 -yview 0)");
	mtk_cmd(appid, "fd_g2_filesf.set(-xview 0 -yview 0)");
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

static void folder_selcommit_callback(mtk_event *e, void *arg)
{
	long appid = (long)arg;
	char curfolder[384];
	char folderlist[8192];
	char num[8];
	int nselection;
	char *selection;

	mtk_req(appid, curfolder, sizeof(curfolder), "fd_g1_l.text");
	mtk_req(appid, folderlist, sizeof(folderlist), "fd_g2_folders.text");
	mtk_req(appid, num, sizeof(num), "fd_g2_folders.selection");
	nselection = atoi(num);
	selection = get_selection(folderlist, nselection);

	if(strcmp(selection, "../") == 0) {
		char *c;
		if(strcmp(curfolder, "/") == 0) return;
		*(curfolder+strlen(curfolder)-1) = 0;
		c = strrchr(curfolder, '/');
		c++;
		*c = 0;
	} else
		strncat(curfolder, selection, sizeof(curfolder));

	display_folder(appid, curfolder);
}

static void update_filename(long appid)
{
	char filelist[8192];
	char num[8];
	int nselection;
	char *selection;

	mtk_req(appid, filelist, sizeof(filelist), "fd_g2_files.text");
	if(strlen(filelist) == 0) return;
	mtk_req(appid, num, sizeof(num), "fd_g2_files.selection");
	nselection = atoi(num);
	selection = get_selection(filelist, nselection);

	mtk_cmdf(appid, "fd_filename.set(-text \"%s\")", selection);
}

static void file_selchange_callback(mtk_event *e, void *arg)
{
	long appid = (long)arg;

	update_filename(appid);
}

static void file_selcommit_callback(mtk_event *e, void *arg)
{
	long appid = (long)arg;

	update_filename(appid);
}

long create_filedialog(const char *name, int is_save, void (*ok_callback)(mtk_event *,void *), void *ok_callback_arg, void (*cancel_callback)(mtk_event *,void *), void *cancel_callback_arg)
{
	long appid;

	appid = mtk_init_app(name);

	mtk_cmd(appid, "fd_g = new Grid()");

	mtk_cmd(appid, "fd_g1 = new Grid()");
	mtk_cmd(appid, "fd_g1_s1 = new Separator(-vertical no)");
	mtk_cmd(appid, "fd_g1_l = new Label(-text \"/\")");
	mtk_cmd(appid, "fd_g1_s2 = new Separator(-vertical no)");
	mtk_cmd(appid, "fd_g1.place(fd_g1_s1, -column 1 -row 1)");
	mtk_cmd(appid, "fd_g1.place(fd_g1_l, -column 2 -row 1)");
	mtk_cmd(appid, "fd_g1.place(fd_g1_s2, -column 3 -row 1)");

	mtk_cmd(appid, "fd_g2 = new Grid()");
	mtk_cmd(appid, "fd_g2_folders = new List()");
	mtk_cmd(appid, "fd_g2_foldersf = new Frame(-content fd_g2_folders -scrollx yes -scrolly yes)");
	mtk_cmd(appid, "fd_g2_files = new List()");
	mtk_cmd(appid, "fd_g2_filesf = new Frame(-content fd_g2_files -scrollx yes -scrolly yes)");
	mtk_cmd(appid, "fd_g2.place(fd_g2_foldersf, -column 1 -row 1)");
	mtk_cmd(appid, "fd_g2.place(fd_g2_filesf, -column 2 -row 1)");
	mtk_cmd(appid, "fd_g2.rowconfig(1, -size 200)");

	mtk_cmd(appid, "fd_selection = new Label(-text \"Selection: /\")");

	mtk_cmd(appid, "fd_filename = new Entry()");

	mtk_cmd(appid, "fd_g3 = new Grid()");
	mtk_cmd(appid, "fd_g3_ok = new Button(-text \"OK\")");
	mtk_cmd(appid, "fd_g3_cancel = new Button(-text \"Cancel\")");
	mtk_cmd(appid, "fd_g3.columnconfig(1, -size 340)");
	mtk_cmd(appid, "fd_g3.place(fd_g3_ok, -column 2 -row 1)");
	mtk_cmd(appid, "fd_g3.place(fd_g3_cancel, -column 3 -row 1)");

	mtk_cmd(appid, "fd_g.place(fd_g1, -column 1 -row 1)");
	mtk_cmd(appid, "fd_g.place(fd_g2, -column 1 -row 2)");
	mtk_cmd(appid, "fd_g.place(fd_selection, -column 1 -row 3 -align \"w\")");
	mtk_cmd(appid, "fd_g.place(fd_filename, -column 1 -row 4)");
	mtk_cmd(appid, "fd_g.place(fd_g3, -column 1 -row 5)");
	mtk_cmd(appid, "fd_g.rowconfig(3, -size 0)");

	mtk_cmdf(appid, "fd = new Window(-content fd_g -title \"%s\")", is_save ? "Save As" : "Open");

	mtk_bind(appid, "fd_g2_folders", "selcommit", folder_selcommit_callback, (void *)appid);
	mtk_bind(appid, "fd_g2_files", "selchange", file_selchange_callback, (void *)appid);
	mtk_bind(appid, "fd_g2_files", "selcommit", file_selcommit_callback, (void *)appid);

	mtk_bind(appid, "fd_g3_ok", "commit", ok_callback, ok_callback_arg);
	mtk_bind(appid, "fd_g3_cancel", "commit", cancel_callback, cancel_callback_arg);
	mtk_bind(appid, "fd", "close", cancel_callback, cancel_callback_arg);

	return appid;
}

void open_filedialog(long appid, const char *folder)
{
	display_folder(appid, folder);

	mtk_cmd(appid, "fd_filename.set(-text \"\")");
	mtk_cmd(appid, "fd.open()");
}

void close_filedialog(long appid)
{
	mtk_cmd(appid, "fd.close()");
}

void get_filedialog_selection(long appid, char *buffer, int buflen)
{
	char file[384];

	mtk_req(appid, buffer, buflen, "fd_g1_l.text");
	mtk_req(appid, file, sizeof(file), "fd_filename.text");
	strncat(buffer, file, buflen);
}
