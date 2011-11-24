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

#ifndef __PATCHPOOL_H
#define __PATCHPOOL_H

struct patchpool {
	int alloc_size;
	char **names;
};

void patchpool_init(struct patchpool *pp);
void patchpool_free(struct patchpool *pp);
void patchpool_add(struct patchpool *pp, char *entry);
void patchpool_del(struct patchpool *pp, const char *entry);
int patchpool_count(struct patchpool *pp);

void patchpool_diff(struct patchpool *pp, struct patchpool *to_delete);

void patchpool_add_multi(struct patchpool *pp, const char *entry);
void patchpool_add_files(struct patchpool *pp, const char *folder, const char **extensions);

#endif /* __PATCHPOOL_H */
