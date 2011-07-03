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

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "patchpool.h"

void patchpool_init(struct patchpool *pp)
{
	pp->alloc_size = 0;
	pp->names = NULL;
}

void patchpool_free(struct patchpool *pp)
{
	int i;
	
	for(i=0;i<pp->alloc_size;i++)
		free(pp->names[i]);
	free(pp->names);
}

#define REALLOC_COUNT 16

void patchpool_add(struct patchpool *pp, char *entry)
{
	int i;
	int newsize;
	
	for(i=0;i<pp->alloc_size;i++)
		if(pp->names[i] == NULL) {
			pp->names[i] = entry;
			return;
		}
	
	newsize = pp->alloc_size + REALLOC_COUNT;
	pp->names = realloc(pp->names, newsize*sizeof(char *));
	if(pp->names == NULL) {
		printf("realloc failed in patchpool\n");
		pp->alloc_size = 0;
		pp->names = 0;
	}
	memset(&pp->names[pp->alloc_size], 0, REALLOC_COUNT*sizeof(char *));
	pp->names[pp->alloc_size] = entry;
	pp->alloc_size = newsize;
}

void patchpool_del(struct patchpool *pp, const char *entry)
{
	int i;
	
	for(i=0;i<pp->alloc_size;i++)
		if((pp->names[i] != NULL) && (strcmp(entry, pp->names[i]) == 0)) {
			free(pp->names[i]);
			pp->names[i] = NULL;
			return;
		}
}

int patchpool_count(struct patchpool *pp)
{
	int i;
	int count;
	
	count = 0;
	for(i=0;i<pp->alloc_size;i++) {
		if(pp->names[i] != NULL)
			count++;
	}
	return count;
}

void patchpool_diff(struct patchpool *pp, struct patchpool *to_delete)
{
	int i;
	
	for(i=0;i<to_delete->alloc_size;i++)
		if(to_delete->names[i] != NULL)
			patchpool_del(pp, to_delete->names[i]);
}

void patchpool_add_multi(struct patchpool *pp, const char *entry)
{
	int offset;
	char *e;
	
	while(1) {
		offset = 0;
		while((entry[offset] != '\n') && (entry[offset] != 0))
			offset++;
		if(offset != 0) {
			e = strndup(entry, offset);
			patchpool_add(pp, e);
		}
		if(entry[offset] == 0)
			break;
		else
			entry += offset + 1;
	}
}

void patchpool_add_files(struct patchpool *pp, const char *folder, const char *extension)
{
	DIR *d;
	struct dirent *entry;
	struct stat s;
	char fullname[384];
	char *c;
	
	d = opendir(folder);
	if(!d) return;
	while((entry = readdir(d))) {
		if(entry->d_name[0] == '.') continue;
		strncpy(fullname, folder, sizeof(fullname));
		strncat(fullname, entry->d_name, sizeof(fullname));
		lstat(fullname, &s);
		if(!S_ISDIR(s.st_mode)) {
			c = strrchr(entry->d_name, '.');
			if((c != NULL) && (strcmp(extension, c+1) == 0))
				patchpool_add(pp, strdup(entry->d_name));
		}
	}
	closedir(d);
}
