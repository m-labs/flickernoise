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

const char *unique(const char *s);
const char *unique_n(const char *s, int n);
void unique_free(void);
void unique_dump(void);

#endif /* !SYMTAB_H */
