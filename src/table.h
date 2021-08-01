/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#ifndef FT_TABLE_H
#define FT_TABLE_H

#include <stdbool.h>

#include "list.h"
#include "object.h"

/* Structure of key value mapping table */
typedef struct {
    list *name; /* K */
    list *objs; /* V */
} table;

/* New table */
table *new_table();

/* Count of table values */
int count_table(table *);

/* Add key and value */
void add_table(table *, char *, object *);

/* Set the value corresponding to the key */
void set_table(table *, char *, object *);

/* Get value with key */
void *get_table(table *, char *);

/* Check if the key exists */
bool exist(table *, char *);

/* Free */
void free_table(table *);

#endif