/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#include "type.h"

const char *type_string(type *t) {
        char *str = malloc(sizeof(char) * DEBUG_TYPE_STR_CAP);
        switch (t->kind) {
        case T_INT:
                free(str);
                return "<int>";
        case T_FLOAT:
                free(str);
                return "<float>";
        case T_CHAR:
                free(str);
                return "<char>";
        case T_STRING:
                free(str);
                return "<string>";
        case T_BOOL:
                free(str);
                return "<bool>";
        case T_ARRAY:
                sprintf(str, "[]%s", type_string((type *)t->inner.single));
                return str;
        case T_TUPLE:
                sprintf(str, "()%s", type_string((type *)t->inner.single));
                return str;
        case T_MAP:
                sprintf(str, "{}<%s : %s>",
                        type_string((type *)t->inner.both.T1),
                        type_string((type *)t->inner.both.T2));
                return str;
        case T_FUNCTION:
                if (t->inner.fn.arg == NULL && t->inner.fn.ret == NULL) {
                        free(str);
                        return "<|| -> none>";
                }
                if (t->inner.fn.arg != NULL) {
                        sprintf(str, "<|");
                        for (int i = 0; i < t->inner.fn.arg->item; i++) {
                                strcat(str,
                                       type_string(
                                           (type *)t->inner.fn.arg->data[i]));
                                if (i + 1 != t->inner.fn.arg->item) {
                                        strcat(str, ", ");
                                }
                        }
                        strcat(str, "|>");
                        if (t->inner.fn.ret != NULL) {
                                strcat(str, " -> ");
                                strcat(str,
                                       type_string((type *)t->inner.fn.ret));
                        }
                        return str;
                }
                if (t->inner.fn.arg == NULL && t->inner.fn.ret != NULL) {
                        sprintf(str, "<|| -> %s>",
                                type_string((type *)t->inner.fn.ret));
                        return str;
                }
        case T_USER:
                sprintf(str, "<%s>", t->inner.name);
                return str;
        }
}

bool type_eq(type *a, type *b) {
        if (a == NULL && b == NULL)
                return true;
        if (a == NULL && b != NULL)
                return false;
        if (a != NULL && b == NULL)
                return false;
        if ((a->kind == T_INT && b->kind != T_INT) ||
            (a->kind == T_FLOAT && b->kind != T_FLOAT) ||
            (a->kind == T_CHAR && b->kind != T_CHAR) ||
            (a->kind == T_STRING && b->kind != T_STRING) ||
            (a->kind == T_BOOL && b->kind != T_BOOL)) {
                return false;
        }
        if (a->kind == T_ARRAY) {
                if (b->kind != T_ARRAY)
                        return false;
                return type_eq((type *)a->inner.single,
                               (type *)b->inner.single);
        }
        if (a->kind == T_TUPLE) {
                if (b->kind != T_TUPLE)
                        return false;
                return type_eq((type *)a->inner.single,
                               (type *)b->inner.single);
        }
        if (a->kind == T_MAP) {
                if (b->kind != T_MAP)
                        return false;
                if (!type_eq((type *)a->inner.both.T1,
                             (type *)b->inner.both.T1))
                        return false;
                if (!type_eq((type *)a->inner.both.T2,
                             (type *)b->inner.both.T2))
                        return false;
        }
        if (a->kind == T_FUNCTION) {
                if (b->kind != T_FUNCTION)
                        return false;
                if (a->inner.fn.ret != NULL) {
                        if (b->inner.fn.ret == NULL)
                                return false;
                        if (!type_eq((type *)a->inner.fn.ret,
                                     (type *)b->inner.fn.ret))
                                return false;
                }
                if (a->inner.fn.ret == NULL) {
                        if (b->inner.fn.ret != NULL)
                                return false;
                }
                if (a->inner.fn.arg->item != b->inner.fn.arg->item)
                        return false;
                for (int i = 0; i < a->inner.fn.arg->item; i++) {
                        type *A = (type *)a->inner.fn.arg->data[i];
                        type *B = (type *)b->inner.fn.arg->data[i];
                        if (!type_eq(A, B)) {
                                return false;
                        }
                }
        }
        return true;
}