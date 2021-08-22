/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#include "table.h"

table *new_table() {
    table *t = malloc(sizeof(table));
    t->name = new_keg();
    t->objs = new_keg();
    return t;
}

int count_table(table *t) {
    return t->name->item;
}

void add_table(table *t, char *name, object *val) {
    for (int i = 0; i < count_table(t); i++) {
        if (strcmp(name, (char *)t->name->data[i]) == 0) {
            replace_keg(t->objs, i, val);
            return;
        }
    }
    t->name = append_keg(t->name, name);
    t->objs = append_keg(t->objs, val);
}

void *get_table(table *t, char *name) {
    for (int i = 0; i < count_table(t); i++) {
        if (strcmp(name, (char *)t->name->data[i]) == 0) {
            return t->objs->data[i];
        }
    }
    return NULL;
}

void disassemble_table(table *t, const char *name) {
    printf("%s: %d item\n", name, count_table(t));
    for (int i = 0; i < count_table(t); i++) {
        printf("%20s -> (%x):%10c%s\n", (char *)t->name->data[i],
               t->objs->data[i], ' ', obj_string((object *)t->objs->data[i]));
    }
}

void free_table(table *t) {
    free_keg(t->name);
    free_keg(t->objs);
    free(t);
}