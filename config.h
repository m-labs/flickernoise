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

#ifndef __CONFIG_H
#define __CONFIG_H

#define MAX_KEY_LEN 32

int config_read_int(const char *key);
void config_write_int(const char *key, int value);

const char *config_read_string(const char *key);
void config_write_string(const char *key, const char *value);

void config_delete(const char *key);
void config_free();

int config_load(const char *filename);
int config_save(const char *filename);

#endif /* __CONFIG_H */
