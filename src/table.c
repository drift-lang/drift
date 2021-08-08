/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#include "table.h"

/* New table */
table *new_table() {
  table *t = (table *)malloc(sizeof(table));
  t->name = new_list();
  t->objs = new_list();
  return t;
}

/* Count of table values */
int count_table(table *t) {
  return t->name->len;
}

/* Add key and value */
void add_table(table *t, char *name, object *val) {
  if (get_table(t, name) != NULL) {
    set_table(t, name, val);
    return;
  }
  t->name = append_list(t->name, name);
  t->objs = append_list(t->objs, val);
}

/* Set the value corresponding to the key */
void set_table(table *t, char *name, object *val) {
  for (int i = 0; i < count_table(t); i++) {
    if (strcmp(name, (char *)t->name->data[i]) == 0) {
      replace_list(t->objs, i, val);
      break;
    }
  }
}

/* Get value with key */
void *get_table(table *t, char *name) {
  for (int i = 0; i < count_table(t); i++) {
    if (strcmp(name, (char *)t->name->data[i]) == 0) {
      return t->objs->data[i];
    }
  }
  return NULL;
}

/* Dissemble */
void dissemble_table(table *t, const char *name) {
  printf("<%s>: %d item\n", name, count_table(t));
  for (int i = 0; i < count_table(t); i++) {
    printf("%20s -> (%x):%20s\n", (char *)t->name->data[i], t->objs->data[i],
           obj_string((object *)t->objs->data[i]));
  }
}

/* Free */
void free_table(table *t) {
  free_list(t->name);
  free_list(t->objs);
  free(t);
}