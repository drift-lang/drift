/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#include "keg.h"

keg *new_keg() {
        keg *g = malloc(sizeof(keg));
        g->data = NULL;
        g->item = 0;
        g->cap = 0;
        return g;
}

keg *append_keg(keg *g, void *ptr) {
        if (g == NULL) {
                g = new_keg();
        }
        if (g->cap == 0 || g->item + 1 > g->cap) {
                g->cap = g->cap == 0 ? 4 : g->cap * 2;
                g->data = realloc(g->data, sizeof(void *) * g->cap);
        }
        g->data[g->item++] = ptr;
        return g;
}

void *back_keg(keg *g) {
        if (g->item == 0) {
                return NULL;
        }
        return g->data[g->item - 1];
}

void *pop_back_keg(keg *g) {
        if (g->item == 0) {
                return NULL;
        }
        return g->data[--g->item];
}

void insert_keg(keg *g, int p, void *ptr) {
        if (p < 0) {
                return;
        }
        if (p != 0 && p > g->item - 1) {
                p = g->item - 1;
        }
        g = append_keg(g, ptr);
        for (int i = p, j = 1, m = 2; i < g->item - 1; i++) {
                g->data[g->item - j] = g->data[g->item - m];
                j++;
                m++;
        }
        g->data[p] = ptr;
}

void replace_keg(keg *g, int p, void *ptr) {
        if (p < 0) {
                return;
        }
        g->data[p] = ptr;
}

void remove_keg(keg *g, int i) {
        if (i < 0 || i > g->item - 1) {
                return;
        }
        for (int j = i; j <= g->item; j++) {
                g->data[j] = g->data[j + 1];
        }
        g->item--;
}

void free_keg(keg *g) {
        if (g->data != NULL) {
                free(g->data);
        }
        free(g);
}