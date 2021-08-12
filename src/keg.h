/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#ifndef FT_LIST_H
#define FT_LIST_H

#include <stdlib.h>

/* My keg structure */
typedef struct {
  void **data;
  int item;
  int cap;
} keg;

/* Create an empty keg */
keg *new_keg();

/* The tail is added,
 * and the capacity is expanded twice automatically */
keg *append_keg(keg *, void *);

/* Tail data */
void *back_keg(keg *);

/* Pop up tail data */
void *pop_back_keg(keg *);

/* Insert element at specified location */
void insert_keg(keg *, int, void *);

/* Replace data in keg subscript */
void replace_keg(keg *, int, void *);

/* Release the keg elements and themselves */
void free_keg(keg *);

#endif