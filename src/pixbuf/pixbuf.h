/*
 * Flickernoise
 * Copyright (C) 2011 Sebastien Bourdeauducq
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

#ifndef __PIXBUF_PIXBUF_H
#define __PIXBUF_PIXBUF_H

#include <sys/stat.h>

struct pixbuf {
	int refcnt;
	char *filename;
	struct stat st;
	struct pixbuf *next;
	int width, height;
	unsigned short pixels[];
};

struct pixbuf *pixbuf_new(int width, int height);
struct pixbuf *pixbuf_search(char *filename);
void pixbuf_inc_ref(struct pixbuf *p);
void pixbuf_dec_ref(struct pixbuf *p);

struct pixbuf *pixbuf_get(char *filename);

#endif /* __PIXBUF_PIXBUF_H */
