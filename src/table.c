/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#include "table.h"

/* New table */
table *new_table() {
    table *t = (table *) malloc(sizeof(table));
    t->name = new_list();
    t->objs = new_list();
    return t;
}

/* Add key and value */
void add_table(table *t, char *name, object *val) {
    t->name = append_list(t->name, name);
    t->objs = append_list(t->objs, val);
}

/* Get value with key */
void *get_table(table *t, char *name) {
    for (int i = 0; i < t->name->len; i ++) {
        if (
            strcmp(name, (char *)t->name->data[i]) == 0
        ) {
            return t->objs->data[i];
        }
    }
    return NULL;
}

/* Count of table values */
int count_table(table *t) {
    return t->name->len;
}

/* Free */
void free_table(table *t) {
    free_list(t->name);
    free_list(t->objs);
}