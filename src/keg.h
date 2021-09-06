/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#ifndef FT_LIST_H
#define FT_LIST_H

#include <stdlib.h>

typedef struct {
    void **data;
    int item;
    int cap;
} keg;

keg *new_keg();

keg *append_keg(keg *, void *);

void *back_keg(keg *);

void *pop_back_keg(keg *);

void insert_keg(keg *, int, void *);

void replace_keg(keg *, int, void *);

void remove_keg(keg *, int);

void free_keg(keg *);

#endif