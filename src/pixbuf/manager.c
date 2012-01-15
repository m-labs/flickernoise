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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "pixbuf.h"
#include "loaders.h"

static struct pixbuf *head;

struct pixbuf *pixbuf_new(int width, int height)
{
	struct pixbuf *p;
	
	p = malloc(sizeof(struct pixbuf)+width*height*2);
	if(p == NULL) return NULL;
	p->refcnt = 1;
	p->filename = NULL;
	p->next = head;
	p->width = width;
	p->height = height;
	head = p;
	return p;
}

struct pixbuf *pixbuf_search(char *filename)
{
	struct pixbuf *p;
	struct stat st;

	if(lstat(filename, &st) < 0)
		return NULL;
	for(p = head; p; p = p->next)
		if(strcmp(p->filename, filename) == 0 &&
		    st.st_mtime == p->st.st_mtime)
			return p;
	return NULL;
}

void pixbuf_inc_ref(struct pixbuf *p)
{
	if(p != NULL)
		p->refcnt++;
}

void pixbuf_dec_ref(struct pixbuf *p)
{
	struct pixbuf **anchor;
	
	if(!p)
		return;
	if(--p->refcnt)
		return;
	for(anchor = &head; *anchor != p; anchor = &(*anchor)->next);
	*anchor = p->next;
	free(p->filename);
	free(p);
}

struct pixbuf *pixbuf_get(char *filename)
{
	struct pixbuf *p;
	FILE *file;

	p = pixbuf_search(filename);
	if(p != NULL) {
		pixbuf_inc_ref(p);
		return p;
	}

	file = fopen(filename, "rb");
	if(!file)
		return NULL;

	/* try all loaders */
	p = pixbuf_load_png(file);
	if(!p) {
		rewind(file);
		p = pixbuf_load_jpeg(file);
	}
	if(p) {
		p->filename = strdup(filename);
		fstat(fileno(file), &p->st);
		if(!p->filename) {
			free(p);
			p = NULL;
		}
	}
	fclose(file);
	return p;
}

struct pixbuf *pixbuf_update(struct pixbuf *p)
{
	struct stat st;

	if(lstat(p->filename, &st) < 0)
		return NULL;
	if(st.st_mtime == p->st.st_mtime) {
		p->refcnt++;
		return p;
	}
	return pixbuf_get(p->filename);
}
