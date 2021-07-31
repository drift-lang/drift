/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#ifndef FT_TABLE_H
#define FT_TABLE_H

#include "list.h"
#include "object.h"

/* Structure of key value mapping table */
typedef struct {
    list *name; /* K */
    list *objs; /* V */
} table;

/* New table */
table *new_table();

/* Add key and value */
void add_table(table *t, char *name, object *val);

/* Get value with key */
void *get_table(table *t, char *name);

/* Count of table values */
int count_table(table *t);

/* Free */
void free_table(table *t);

#endif