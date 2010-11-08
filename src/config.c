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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

struct element {
	struct element *next;
	char key[MAX_KEY_LEN];
	int is_string;
	union {
		int i;
		char string[4];
	} payload;
};

static struct element *new_int_element(const char *key, int i, struct element *next)
{
	struct element *new;

	new = malloc(sizeof(struct element));
	if(new == NULL) return NULL;

	new->next = next;
	memcpy(new->key, key, MAX_KEY_LEN);
	new->key[MAX_KEY_LEN-1] = 0;
	new->is_string = 0;
	new->payload.i = i;

	return new;
}

static struct element *new_string_element(const char *key, const char *string, struct element *next)
{
	int len;
	struct element *new;

	len = strlen(string)+1;
	new = malloc(sizeof(struct element)-4+len);
	if(new == NULL) return NULL;

	new->next = next;
	memcpy(new->key, key, MAX_KEY_LEN);
	new->key[MAX_KEY_LEN-1] = 0;
	new->is_string = 1;
	memcpy(new->payload.string, string, len);

	return new;
}

static struct element *head;

static struct element *config_find(const char *key)
{
	struct element *e;

	e = head;
	while(e != NULL) {
		if(strcmp(e->key, key) == 0) return e;
		e = e->next;
	}
	return NULL;
}

int config_read_int(const char *key, int default_value)
{
	struct element *e;

	e = config_find(key);
	if(e == NULL) return default_value;
	if(e->is_string) return default_value;

	return e->payload.i;
}

void config_write_int(const char *key, int value)
{
	struct element *e;

	e = config_find(key);
	if(e != NULL) {
		e->is_string = 0;
		e->payload.i = value;
		return;
	}
	e = new_int_element(key, value, head);
	if(e == NULL) return;
	head = e;
}

const char *config_read_string(const char *key)
{
	struct element *e;

	e = config_find(key);
	if(e == NULL) return NULL;
	if(!e->is_string) return NULL;

	return e->payload.string;
}

void config_write_string(const char *key, const char *value)
{
	struct element *e;

	config_delete(key);
	e = new_string_element(key, value, head);
	if(e == NULL) return;
	head = e;
}

void config_delete(const char *key)
{
	struct element *e, *e2;

	e = config_find(key);
	if(e == NULL) return;

	if(e == head) {
		head = head->next;
		free(e);
		return;
	}

	e2 = head;
	while(e2->next != e)
		e2 = e2->next;
	e2->next = e->next;
	free(e);
}

void config_free()
{
	struct element *e, *next;

	e = head;
	while(e != NULL) {
		next = e->next;
		free(e);
		e = next;
	}
	head = NULL;
}

int config_load(const char *filename)
{
	FILE *fd;
	char *line;
	size_t n;
	size_t readbytes;
	char *c;

	config_free();

	fd = fopen(filename, "r");
	if(fd == NULL) return 0;

	line = NULL;
	while(!feof(fd)) {
		readbytes = __getline(&line, &n, fd);
		if(readbytes <= 0)
			break;
		c = strchr(line, '\r');
		if(c) *c = 0;
		c = strchr(line, '\n');
		if(c) *c = 0;

		c = strchr(line, '=');
		if(c == NULL) {
			printf("malformed line: '%s'\n", line);
			continue;
		}
		*c = 0;
		c++;
		if(*c == 's') {
			c++;
			config_write_string(line, c);
		} else if(*c == 'i') {
			int val;
			char *err;
			c++;
			val = strtol(c, &err, 0);
			config_write_int(line, val);
		} else {
			printf("unknown data type %c\n", *c);
			continue;
		}
	}

	free(line);
	fclose(fd);
	return 1;
}

int config_save(const char *filename)
{
	FILE *fd;
	struct element *e;

	fd = fopen(filename, "w");
	if(fd == NULL) return 0;

	e = head;
	while(e != NULL) {
		if(e->is_string)
			fprintf(fd, "%s=s%s\n", e->key, e->payload.string);
		else
			fprintf(fd, "%s=i%d\n", e->key, e->payload.i);
		e = e->next;
	}

	if(fclose(fd) != 0) return 0;
	return 1;
}
