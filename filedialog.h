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

#ifndef __FILEDIALOG_H
#define __FILEDIALOG_H

void create_filedialog(long appid, const char *name, int is_save, void (*ok_callback)(dope_event *,void *), void (*cancel_callback)(dope_event *,void *));
void open_filedialog(long appid, const char *name);
void close_filedialog(long appid, const char *name);
void get_filedialog_selection(long appid, const char *name, char *buffer, int buflen);

#endif /* __FILEDIALOG_H */
