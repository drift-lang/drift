/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
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

/* Dissemble */
void dissemble_table(table *, const char *);

/* Free */
void free_table(table *);

#endif