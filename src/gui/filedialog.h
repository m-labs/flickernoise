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

#ifndef __FILEDIALOG_H
#define __FILEDIALOG_H

struct filedialog {
	int appid;
	int is_save;
	const char *extfilter;
	const char *extfilter2;
	const char *extfilter3;
	void (*ok_callback)(void *);
	void *ok_callback_arg;
	void (*cancel_callback)(void *);
	void *cancel_callback_arg;
};

struct filedialog *create_filedialog(const char *name, int is_save, const char *extfilter, void (*ok_callback)(void *), void *ok_callback_arg, void (*cancel_callback)(void *), void *cancel_callback_arg);
void filedialog_extfilterex(struct filedialog *dlg, const char *extfilter2, const char *extfilter3);
void open_filedialog(struct filedialog *dlg);
void close_filedialog(struct filedialog *dlg);
void get_filedialog_selection(struct filedialog *dlg, char *buffer, int buflen);

#endif /* __FILEDIALOG_H */
