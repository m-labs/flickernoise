/*
 * symtab.h - Symbol table
 *
 * Copyright 2011-2012 by Werner Almesberger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 */

#ifndef SYMTAB_H
#define	SYMTAB_H

#include <fpvm/symbol.h>


#define	SF_SYSTEM	(1 << 0)
#define	SF_ASSIGNED	(1 << 1)
#define	SF_FIXED	SF_SYSTEM

struct sym {
	struct fpvm_sym fpvm_sym;
	int pfv_idx, pvv_idx;	/* index; -1 if not a variable known to FN */
	int flags;
};


extern struct sym well_known[];
extern int num_well_known;
extern struct sym *user_syms;
extern int num_user_syms;

#define	FPVM2SYM(fpvm_sym)	((struct sym *) (fpvm_sym))

#define	foreach_sym(p) \
	for ((p) = well_known; (p) != user_syms+num_user_syms; \
	    (p) = (p) == well_known+num_well_known-1 ? user_syms : (p)+1)

void symtab_init(void);
struct sym *unique(const char *s);
struct sym *unique_n(const char *s, int n);
void symtab_free(void);

#endif /* !SYMTAB_H */
