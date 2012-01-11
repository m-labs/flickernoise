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

#include "symtab.h"


#define	INITIAL_ALLOC	64


struct key_n {
	const char *s;
	int n;
};

struct sym well_known[] = {
#include "fnp.inc"
};

static struct sym *syms = NULL;
static int num_syms = 0, allocated = 0;


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
	if(num_syms != allocated)
		return;

	allocated = allocated ? allocated*2 : INITIAL_ALLOC;
	syms = realloc(syms, allocated*sizeof(*syms));
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
	struct sym *res;
	struct sym *walk;

	res = bsearch(s, well_known, sizeof(well_known)/sizeof(*well_known),
	    sizeof(s), cmp);
	if(res)
		return res;
	for(walk = syms; walk != syms+num_syms; walk++)
		if(!strcmp(walk->fpvm_sym.name, s))
			return walk;
	grow_table();
	syms[num_syms].fpvm_sym.name = strdup(s);
	return syms+num_syms++;
}


struct sym *unique_n(const char *s, int n)
{
	struct key_n key = {
		.s = s,
		.n = n,
	};
	struct sym *res;
	struct sym *walk;

	assert(n);
	res = bsearch(&key, well_known, sizeof(well_known)/sizeof(*well_known),
	    sizeof(s), cmp_n);
	if(res)
		return res;
	for(walk = syms; walk != syms+num_syms; walk++)
		if(!strcmp_n(s, walk->fpvm_sym.name, n))
			return walk;
	grow_table();
	syms[num_syms].fpvm_sym.name = strdup_n(s, n);
	return syms+num_syms++;
}


void unique_free(void)
{
	int i;

	for(i = 0; i != num_syms; i++)
		free((void *) syms[i].fpvm_sym.name);
	free(syms);
	syms = NULL;
	num_syms = allocated = 0;
}


#ifdef STANDALONE

void unique_dump(void)
{
	int i;

	for(i = 0; i != sizeof(well_known)/sizeof(*well_known); i++)
		printf("%s\n", well_known[i].fpvm_sym.name);
	printf("\n");
	for(i = 0; i != num_syms; i++)
		printf("%s\n", syms[i].fpvm_sym.name);
}

#endif /* STANDALONE */
