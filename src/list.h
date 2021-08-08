/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
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
list *append_list(list *, void *);

/* Tail data */
void *back_list(list *);

/* Pop up tail data */
void *pop_back_list(list *);

/* Insert element at specified location */
void insert_list(list *, int, void *);

/* Replace data in list subscript */
void replace_list(list *, int, void *);

/* Release the list elements and themselves */
void free_list(list *);

#endif