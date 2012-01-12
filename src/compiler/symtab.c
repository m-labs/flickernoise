/*
 * symtab.c - Symbol table
 *
 * Copyright 2011-2012 by Werner Almesberger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "compiler.h"
#include "symtab.h"


#define	INITIAL_ALLOC	64


struct key_n {
	const char *s;
	int n;
};

struct sym well_known[] = {
#include "fnp.inc"
};
int num_well_known;

struct sym *user_syms = NULL;
int num_user_syms = 0;

static int allocated = 0;


/*
 * "a" is not NUL-terminated and its length "a" is determined by "n".
 * "b" is NUL-terminated.
 */

static int strcmp_n(const char *a, const char *b, int n)
{
	int diff;

	diff = strncmp(a, b, n);
	if (diff)
		return diff;
	/* handle implicit NUL in string "a" */
	return -b[n];
}


static char *strdup_n(const char *s, int n)
{
	char *new;

	new = malloc(n+1);
	memcpy(new, s, n);
	new[n] = 0;
	return new;
}


static void grow_table(void)
{
	if(num_user_syms != allocated)
		return;

	allocated = allocated ? allocated*2 : INITIAL_ALLOC;
	user_syms = realloc(user_syms, allocated*sizeof(*user_syms));
}


static int cmp(const void *a, const void *b)
{
	const struct sym *sym = b;

	return strcmp(a, sym->fpvm_sym.name);
}


static int cmp_n(const void *a, const void *b)
{
	const struct key_n *key = a;
	const struct sym *sym = b;

	return strcmp_n(key->s, sym->fpvm_sym.name, key->n);
}


struct sym *unique(const char *s)
{
	struct sym *res, *walk, *new;

	res = bsearch(s, well_known, num_well_known, sizeof(*well_known), cmp);
	if(res)
		return res;
	for(walk = user_syms; walk != user_syms+num_user_syms; walk++)
		if(!strcmp(walk->fpvm_sym.name, s))
			return walk;
	grow_table();
	new = user_syms+num_user_syms++;
	new->fpvm_sym.name = strdup(s);
	new->pfv_idx = new->pvv_idx = -1;
	new->flags = 0;
	return new;
}


struct sym *unique_n(const char *s, int n)
{
	struct key_n key = {
		.s = s,
		.n = n,
	};
	struct sym *res, *walk, *new;

	assert(n);
	res = bsearch(&key, well_known, num_well_known, sizeof(*well_known),
	    cmp_n);
	if(res)
		return res;
	for(walk = user_syms; walk != user_syms+num_user_syms; walk++)
		if(!strcmp_n(s, walk->fpvm_sym.name, n))
			return walk;
	grow_table();
	new = user_syms+num_user_syms++;
	new->fpvm_sym.name = strdup_n(s, n);
	new->pfv_idx = new->pvv_idx = -1;
	new->flags = 0;
	return new;
}


void symtab_init(void)
{
	int i;

	num_well_known = sizeof(well_known)/sizeof(*well_known);
	for(i = 0; i != num_well_known; i++)
		well_known[i].flags &= SF_FIXED;
}


void symtab_free(void)
{
	int i;

	for(i = 0; i != num_user_syms; i++)
		free((void *) user_syms[i].fpvm_sym.name);
	free(user_syms);
	user_syms = NULL;
	num_user_syms = allocated = 0;
}
