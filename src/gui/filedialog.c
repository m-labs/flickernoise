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
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#include <mtklib.h>

#include "filedialog.h"

#define ROOT_FOLDER "/ssd/"

static int cmpstringp(const void *p1, const void *p2)
{
	return strcmp(*(char * const *)p1, *(char * const *)p2);
}

static void display_folder(struct filedialog *dlg, const char *folder)
{
	char *folders[384];
	int n_folders;
	char *files[384];
	int n_files;
	int i;

	char fmt_folders[8192];
	char *c_folders;
	char fmt_files[8192];
	char *c_files;
	char fullname[384];
	char quickfind[384];
	DIR *d;
	struct dirent *entry;
	struct stat s;
	int len;
	char *c;

	mtk_req(dlg->appid, quickfind, sizeof(quickfind), "fd_gqf_e.text");

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
			if(n_folders < 384) {
				folders[n_folders] = strdup(entry->d_name);
				n_folders++;
			}
		} else {
			/* hide files without the extensions we are looking for
			 * and those which do not match the quickfind entry (if any).
			 */
			c = strrchr(entry->d_name, '.');
			if(
			  (((dlg->extfilter[0] == 0) || ((c != NULL) && (strcasecmp(dlg->extfilter, c+1) == 0)))
			   || ((dlg->extfilter2[0] != 0) && (c != NULL) && (strcasecmp(dlg->extfilter2, c+1) == 0))
			   || ((dlg->extfilter3[0] != 0) && (c != NULL) && (strcasecmp(dlg->extfilter3, c+1) == 0)))
			  && ((quickfind[0] == 0) || (strcasestr(entry->d_name, quickfind) != NULL))
			) {
				if(n_files < 384) {
					files[n_files] = strdup(entry->d_name);
					n_files++;
				}
			}
		}
	}
	closedir(d);
	
	qsort(folders, n_folders, sizeof(char *), cmpstringp);
	qsort(files, n_files, sizeof(char *), cmpstringp);

	strcpy(fmt_folders, "../");
	c_folders = fmt_folders + 3;
	fmt_files[0] = 0;
	c_files = fmt_files;
	for(i=0;i<n_folders;i++) {
		len = strlen(folders[i]);
		if((c_folders-fmt_folders)+len+3 > sizeof(fmt_folders)) break;
		*c_folders++ = '\n';
		strcpy(c_folders, folders[i]);
		free(folders[i]);
		c_folders += len;
		*c_folders++ = '/';
		*c_folders = 0;
	}
	for(i=0;i<n_files;i++) {
		len = strlen(files[i]);
		if((c_files-fmt_files)+len+2 > sizeof(fmt_files)) break;
		if(c_files != fmt_files)
			*c_files++ = '\n';
		strcpy(c_files, files[i]);
		free(files[i]);
		c_files += len;
		*c_files = 0;
	}

	mtk_cmdf(dlg->appid, "fd_g2_folders.set(-text \"%s\" -selection 0)", fmt_folders);
	mtk_cmdf(dlg->appid, "fd_g2_files.set(-text \"%s\" -selection 0)", fmt_files);
	mtk_cmdf(dlg->appid, "fd_g1_l.set(-text \"\e%s\")", folder);
	mtk_cmdf(dlg->appid, "fd_selection.set(-text \"\e%s\")", folder);
	mtk_cmd(dlg->appid, "fd_g2_foldersf.expose(0, 0)");
	mtk_cmd(dlg->appid, "fd_g2_filesf.expose(0, 0)");
}

static void refresh(struct filedialog *dlg)
{
	char curfolder[384];

	mtk_req(dlg->appid, curfolder, sizeof(curfolder), "fd_g1_l.text");
	memmove(curfolder, curfolder+1, sizeof(curfolder)-1);
	display_folder(dlg, curfolder);
	mtk_cmd(dlg->appid, "fd_filename.set(-text \"\")");
}

static void unlock_callback(mtk_event *e, void *arg)
{
	struct filedialog *dlg = arg;
	dlg->unlock++;
}

static void gqf_change_callback(mtk_event *e, void *arg)
{
	struct filedialog *dlg = arg;
	refresh(dlg);
}

static void gqf_clear_callback(mtk_event *e, void *arg)
{
	struct filedialog *dlg = arg;

	mtk_cmd(dlg->appid, "fd_gqf_e.set(-text \"\")");
	refresh(dlg);
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
	struct filedialog *dlg = arg;
	char curfolder[384];
	char folderlist[8192];
	char num[8];
	int nselection;
	char *selection;

	mtk_req(dlg->appid, curfolder, sizeof(curfolder), "fd_g1_l.text");
	memmove(curfolder, curfolder+1, sizeof(curfolder)-1);
	mtk_req(dlg->appid, folderlist, sizeof(folderlist), "fd_g2_folders.text");
	mtk_req(dlg->appid, num, sizeof(num), "fd_g2_folders.selection");
	nselection = atoi(num);
	selection = get_selection(folderlist, nselection);

	if(strcmp(selection, "../") == 0) {
		char *c;
		if(strcmp(curfolder, dlg->unlock > 2 ? "/": ROOT_FOLDER) == 0) return;
		*(curfolder+strlen(curfolder)-1) = 0;
		c = strrchr(curfolder, '/');
		c++;
		*c = 0;
	} else
		strncat(curfolder, selection, sizeof(curfolder));

	display_folder(dlg, curfolder);
}

static void update_filename(struct filedialog *dlg)
{
	char filelist[8192];
	char num[8];
	int nselection;
	char *selection;

	mtk_req(dlg->appid, filelist, sizeof(filelist), "fd_g2_files.text");
	if(strlen(filelist) == 0) return;
	mtk_req(dlg->appid, num, sizeof(num), "fd_g2_files.selection");
	nselection = atoi(num);
	selection = get_selection(filelist, nselection);

	mtk_cmdf(dlg->appid, "fd_filename.set(-text \"%s\")", selection);
}

static void file_selchange_callback(mtk_event *e, void *arg)
{
	struct filedialog *dlg = arg;
	
	update_filename(dlg);
}

static void file_selcommit_callback(mtk_event *e, void *arg)
{
	struct filedialog *dlg = arg;
	
	close_filedialog(dlg);
	update_filename(dlg);
	if(dlg->ok_callback)
		dlg->ok_callback(dlg->ok_callback_arg);
}

static void dlg_ok_callback(mtk_event *e, void *arg)
{
	struct filedialog *dlg = arg;
	char file[384];

	mtk_req(dlg->appid, file, sizeof(file), "fd_filename.text");
	if((strcmp(file, "") == 0) || (strchr(file, '/') != NULL))
		return;
	close_filedialog(dlg);
	if(dlg->ok_callback)
		dlg->ok_callback(dlg->ok_callback_arg);
}

static void dlg_close_callback(mtk_event *e, void *arg)
{
	struct filedialog *dlg = arg;
	
	close_filedialog(dlg);
	if(dlg->cancel_callback)
		dlg->cancel_callback(dlg->ok_callback_arg);
}

struct filedialog *create_filedialog(const char *name, int is_save, const char *extfilter, void (*ok_callback)(void *), void *ok_callback_arg, void (*cancel_callback)(void *), void *cancel_callback_arg)
{
	struct filedialog *dlg;

	dlg = malloc(sizeof(struct filedialog));
	if(dlg == NULL) abort();
	dlg->appid = mtk_init_app(name);
	dlg->is_save = is_save;
	dlg->extfilter = extfilter;
	dlg->extfilter2 = "";
	dlg->extfilter3 = "";
	dlg->unlock = 0;
	dlg->ok_callback = ok_callback;
	dlg->ok_callback_arg = ok_callback_arg;
	dlg->cancel_callback = cancel_callback;
	dlg->cancel_callback_arg = cancel_callback_arg;

	mtk_cmd(dlg->appid, "fd_g = new Grid()");

	mtk_cmd(dlg->appid, "fd_g1 = new Grid()");
	mtk_cmd(dlg->appid, "fd_g1_s1 = new Separator(-vertical no)");
	mtk_cmd(dlg->appid, "fd_g1_l = new Label(-text \"\e"ROOT_FOLDER"\")");
	mtk_cmd(dlg->appid, "fd_g1_s2 = new Separator(-vertical no)");
	mtk_cmd(dlg->appid, "fd_g1.place(fd_g1_s1, -column 1 -row 1)");
	mtk_cmd(dlg->appid, "fd_g1.place(fd_g1_l, -column 2 -row 1)");
	mtk_cmd(dlg->appid, "fd_g1.place(fd_g1_s2, -column 3 -row 1)");
	
	mtk_cmd(dlg->appid, "fd_gqf = new Grid()");
	mtk_cmd(dlg->appid, "fd_gqf_l = new Label(-text \"Quick find: \")");
	mtk_cmd(dlg->appid, "fd_gqf_e = new Entry()");
	mtk_cmd(dlg->appid, "fd_gqf_clear = new Button(-text \"Clear\")");
	mtk_cmd(dlg->appid, "fd_gqf.place(fd_gqf_l, -column 1 -row 1)");
	mtk_cmd(dlg->appid, "fd_gqf.place(fd_gqf_e, -column 2 -row 1)");
	mtk_cmd(dlg->appid, "fd_gqf.place(fd_gqf_clear, -column 3 -row 1)");
	mtk_cmd(dlg->appid, "fd_gqf.columnconfig(3, -size 0)");

	mtk_cmd(dlg->appid, "fd_g2 = new Grid()");
	mtk_cmd(dlg->appid, "fd_g2_folders = new List()");
	mtk_cmd(dlg->appid, "fd_g2_foldersf = new Frame(-content fd_g2_folders -scrollx yes -scrolly yes)");
	mtk_cmd(dlg->appid, "fd_g2_files = new List()");
	mtk_cmd(dlg->appid, "fd_g2_filesf = new Frame(-content fd_g2_files -scrollx yes -scrolly yes)");
	mtk_cmd(dlg->appid, "fd_g2.place(fd_g2_foldersf, -column 1 -row 1)");
	mtk_cmd(dlg->appid, "fd_g2.place(fd_g2_filesf, -column 2 -row 1)");
	mtk_cmd(dlg->appid, "fd_g2.rowconfig(1, -size 200)");

	mtk_cmd(dlg->appid, "fd_g_selection = new Grid()");
	mtk_cmd(dlg->appid, "fd_selection0 = new Label(-text \"Selection:\")"); 
	mtk_cmd(dlg->appid, "fd_selection = new Label(-text \"\e/\")");
	mtk_cmd(dlg->appid, "fd_g_selection.place(fd_selection0, -column 1 -row 1)");
	mtk_cmd(dlg->appid, "fd_g_selection.place(fd_selection, -column 2 -row 1)");

	mtk_cmd(dlg->appid, "fd_filename = new Entry()");

	mtk_cmd(dlg->appid, "fd_g3 = new Grid()");
	mtk_cmd(dlg->appid, "fd_g3_ok = new Button(-text \"OK\")");
	mtk_cmd(dlg->appid, "fd_g3_cancel = new Button(-text \"Cancel\")");
	mtk_cmd(dlg->appid, "fd_g3.columnconfig(1, -size 340)");
	mtk_cmd(dlg->appid, "fd_g3.place(fd_g3_ok, -column 2 -row 1)");
	mtk_cmd(dlg->appid, "fd_g3.place(fd_g3_cancel, -column 3 -row 1)");

	mtk_cmd(dlg->appid, "fd_g.place(fd_g1, -column 1 -row 1)");
	mtk_cmd(dlg->appid, "fd_g.place(fd_gqf, -column 1 -row 2)");
	mtk_cmd(dlg->appid, "fd_g.place(fd_g2, -column 1 -row 3)");
	mtk_cmd(dlg->appid, "fd_g.place(fd_g_selection, -column 1 -row 4 -align \"w\")");
	mtk_cmd(dlg->appid, "fd_g.place(fd_filename, -column 1 -row 5)");
	mtk_cmd(dlg->appid, "fd_g.place(fd_g3, -column 1 -row 6)");
	mtk_cmd(dlg->appid, "fd_g.rowconfig(4, -size 0)");

	mtk_cmdf(dlg->appid, "fd = new Window(-content fd_g -title \"%s\")", name);
	
	mtk_bind(dlg->appid, "fd_g1_l", "press", unlock_callback, (void *)dlg);
	
	mtk_bind(dlg->appid, "fd_gqf_e", "change", gqf_change_callback, (void *)dlg);
	mtk_bind(dlg->appid, "fd_gqf_clear", "commit", gqf_clear_callback, (void *)dlg);

	mtk_bind(dlg->appid, "fd_g2_folders", "selcommit", folder_selcommit_callback, (void *)dlg);
	mtk_bind(dlg->appid, "fd_g2_files", "selchange", file_selchange_callback, (void *)dlg);
	mtk_bind(dlg->appid, "fd_g2_files", "selcommit", file_selcommit_callback, (void *)dlg);

	mtk_bind(dlg->appid, "fd_g3_ok", "commit", dlg_ok_callback, (void *)dlg);
	mtk_bind(dlg->appid, "fd_g3_cancel", "commit", dlg_close_callback, (void *)dlg);
	mtk_bind(dlg->appid, "fd", "close", dlg_close_callback, (void *)dlg);

	return dlg;
}

void filedialog_extfilterex(struct filedialog *dlg, const char *extfilter2, const char *extfilter3)
{
	dlg->extfilter2 = extfilter2;
	dlg->extfilter3 = extfilter3;
}

void open_filedialog(struct filedialog *dlg)
{
	dlg->unlock = 0;
	refresh(dlg);
	mtk_cmd(dlg->appid, "fd.open()");
}

void close_filedialog(struct filedialog *dlg)
{
	mtk_cmd(dlg->appid, "fd.close()");
}

void get_filedialog_selection(struct filedialog *dlg, char *buffer, int buflen)
{
	char file[384];
	char *c;

	mtk_req(dlg->appid, buffer, buflen, "fd_g1_l.text");
	memmove(buffer, buffer+1, buflen-1);
	mtk_req(dlg->appid, file, sizeof(file), "fd_filename.text");
	strncat(buffer, file, buflen);
	if(dlg->is_save) {
		c = strrchr(buffer, '.');
		if((c == NULL) || (strcmp(c+1, dlg->extfilter) != 0)) {
			strncat(buffer, ".", buflen);
			strncat(buffer, dlg->extfilter, buflen);
		}
	}
}
