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
#include <string.h>

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
	
	p = head;
	while(p != NULL) {
		if((p->filename != NULL) && (strcmp(p->filename, filename) == 0))
			return p;
		p = p->next;
	}
	return NULL;
}

void pixbuf_inc_ref(struct pixbuf *p)
{
	p->refcnt++;
}

void pixbuf_dec_ref(struct pixbuf *p)
{
	struct pixbuf *prev;
	
	if(p != NULL) {
		p->refcnt--;
		if(p->refcnt == 0) {
			if(p == head) {
				head = head->next;
				free(p->filename);
				free(p);
			} else {
				prev = head;
				while(prev->next != p)
					prev = prev->next;
				prev->next = p->next;
				free(p->filename);
				free(p);
			}
		}
	}
}

struct pixbuf *pixbuf_get(char *filename)
{
	struct pixbuf *p;
	
	p = pixbuf_search(filename);
	if(p != NULL) {
		pixbuf_inc_ref(p);
		return p;
	}
	
	/* try all loaders */
	p = pixbuf_load_png(filename);
	if(p != NULL) return p;
	p = pixbuf_load_jpeg(filename);
	if(p != NULL) return p;
	
	/* no loader was successful */
	return NULL;
}
