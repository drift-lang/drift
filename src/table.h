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

/* Structure of key value mapping table */
typedef struct {
    keg *name; /* K */
    keg *objs; /* V */
} table;

/* New table */
table *new_table();

/* Count of table values */
int count_table(table *);

/* Add key and value */
void add_table(table *, char *, object *);

/* Get value with key */
void *get_table(table *, char *);

/* Dis */
void disassemble_table(table *, const char *);

/* Free */
void free_table(table *);

#endif