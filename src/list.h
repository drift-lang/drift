/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#ifndef FT_LIST_H
#define FT_LIST_H

#include <stdlib.h>

/* My list structure */
typedef struct {
    void **data;
    int len;
    int cap;
} list;

/* Create an empty list */
list *new_list();

/* The tail is added,
 * and the capacity is expanded twice automatically */
list *append_list(list *l, void *ptr);

/* Tail data */
void *list_back(list *l);

/* Pop up tail data */
void *pop_list_back(list *l);

/* Insert element at specified location */
void insert_list(list *l, int p, void *ptr);

/* Release the list elements and themselves */
void free_list(list *l);

#endif