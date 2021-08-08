/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#include "list.h"

/* Create an empty list */
list *new_list() {
  list *l = (list *)malloc(sizeof(list));
  l->data = NULL;
  l->len = 0;
  l->cap = 0;
  return l;
}

/* The tail is added,
 * and the capacity is expanded twice automatically */
list *append_list(list *l, void *ptr) {
  if (l == NULL) {
    l = new_list(); /* New */
  }
  if (l->cap == 0 || l->len + 1 > l->cap) {
    l->cap = l->cap == 0 ? 4 : l->cap * 2;
    l->data = realloc(l->data, sizeof(void *) * l->cap); /* Double capacity */
  }
  l->data[l->len++] = ptr;
  return l;
}

/* Tail data */
void *back_list(list *l) {
  if (l->len == 0) {
    return NULL;
  }
  return l->data[l->len - 1];
}

/* Pop up tail data */
void *pop_back_list(list *l) {
  if (l->len == 0) {
    return NULL;
  }
  return l->data[--l->len];
}

/* Insert element at specified location */
void insert_list(list *l, int p, void *ptr) {
  if (p < 0) {
    return;
  }
  if (p != 0 && p > l->len - 1) {
    p = l->len - 1;
  }
  l = append_list(l, ptr);
  for (int i = p, j = 1, m = 2; i < l->len - 1; i++) {
    l->data[l->len - j] = l->data[l->len - m];
    j++;
    m++;
  }
  l->data[p] = ptr;
}

/* Replace data in list subscript */
void replace_list(list *l, int p, void *ptr) {
  if (p < 0) {
    return;
  }
  free(l->data[p]);
  l->data[p] = ptr;
}

/* Release the list elements and themselves */
void free_list(list *l) {
  free(l->data);
  free(l);
}