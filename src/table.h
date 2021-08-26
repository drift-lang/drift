/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#ifndef FT_TABLE_H
#define FT_TABLE_H

#include <stdbool.h>

#include "keg.h"
#include "object.h"

typedef struct {
    keg *name;
    keg *value;
} table;

table *new_table();

int count_table(table *);

void add_table(table *, char *, void*);

void *get_table(table *, char *);

void disassemble_table(table *, const char *);

void free_table(table *);

#endif