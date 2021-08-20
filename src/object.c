/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#include "object.h"

const char *obj_string(object *obj) {
        char *str = malloc(sizeof(char) * DEBUG_OBJ_STR_CAP);
        switch (obj->kind) {
        case OBJ_INT:
                sprintf(str, "int %d", obj->value.integer);
                return str;
        case OBJ_FLOAT:
                sprintf(str, "float %f", obj->value.floating);
                return str;
        case OBJ_STRING:
                sprintf(str, "string \"%s\"", obj->value.string);
                return str;
        case OBJ_CHAR:
                sprintf(str, "char '%c'", obj->value.ch);
                return str;
        case OBJ_BOOL:
                sprintf(str, "bool %s", obj->value.boolean ? "T" : "F");
                return str;
        case OBJ_NIL:
                sprintf(str, "nil");
                return str;
        case OBJ_ENUMERATE:
                sprintf(str, "enumerate \"%s\"", obj->value.en.name);
                return str;
        case OBJ_FUNCTION:
                sprintf(str, "function \"%s\"", obj->value.fn.name);
                return str;
        case OBJ_INTERFACE:
                sprintf(str, "interface \"%s\"", obj->value.in.name);
                return str;
        case OBJ_CLASS:
                sprintf(str, "class \"%s\"", obj->value.cl.name);
                return str;
        case OBJ_ARR:
                sprintf(str, "arr %d", obj->value.arr.element->item);
                return str;
        case OBJ_TUP:
                sprintf(str, "tup %d", obj->value.tup.element->item);
                return str;
        case OBJ_MAP:
                sprintf(str, "map %d", obj->value.map.v->item);
                return str;
        }
}

const char *obj_raw_string(object *obj) {
        char *str = malloc(sizeof(char) * STRING_CAP);
        switch (obj->kind) {
        case OBJ_INT:
                sprintf(str, "%d", obj->value.integer);
                return str;
        case OBJ_FLOAT:
                sprintf(str, "%f", obj->value.floating);
                return str;
        case OBJ_STRING:
                sprintf(str, "%s", obj->value.string);
                return str;
        case OBJ_CHAR:
                sprintf(str, "'%c'", obj->value.ch);
                return str;
        case OBJ_BOOL:
                sprintf(str, "%s", obj->value.boolean ? "T" : "F");
                return str;
        case OBJ_ARR: {
                keg *elem = obj->value.arr.element;
                if (elem->item == 0) {
                        free(str);
                        return "[]";
                }
                sprintf(str, "[");
                for (int i = 0; i < elem->item; i++) {
                        strcat(str, obj_raw_string((object *)elem->data[i]));
                        if (i + 1 != elem->item) {
                                strcat(str, ", ");
                        }
                }
                strcat(str, "]");
                return str;
        }
        case OBJ_TUP: {
                keg *elem = obj->value.arr.element;
                if (elem->item == 0) {
                        free(str);
                        return "()";
                }
                sprintf(str, "(");
                for (int i = 0; i < elem->item; i++) {
                        strcat(str, obj_raw_string((object *)elem->data[i]));
                        if (i + 1 != elem->item) {
                                strcat(str, ", ");
                        }
                }
                strcat(str, ")");
                return str;
        }
        case OBJ_MAP: {
                keg *k = obj->value.map.k;
                keg *v = obj->value.map.v;
                if (k->item == 0) {
                        free(str);
                        return "{}";
                }
                sprintf(str, "{");
                for (int i = 0; i < k->item; i++) {
                        strcat(str, obj_raw_string((object *)k->data[i]));
                        strcat(str, ": ");
                        strcat(str, obj_raw_string((object *)v->data[i]));
                        if (i + 1 != k->item) {
                                strcat(str, ", ");
                        }
                }
                strcat(str, "}");
                return str;
        }
        default: {
                free(str);
                return obj_string(obj);
        }
        }
}

bool je_ins = false;

#define EV_INT(obj, val) \
        obj->kind = OBJ_INT; \
        obj->value.integer = val; \
        je_ins = true
#define EV_FLO(obj, val) \
        obj->kind = OBJ_FLOAT; \
        obj->value.floating = val; \
        je_ins = true
#define EV_BOL(obj, val) \
        obj->kind = OBJ_BOOL; \
        obj->value.boolean = val; \
        je_ins = true

#define OP_A(A, B, J, P) \
        if (A->kind == OBJ_INT) { \
                if (B->kind == OBJ_INT) { \
                        EV_INT(J, A->value.integer P B->value.integer); \
                } \
                if (B->kind == OBJ_FLOAT) { \
                        EV_FLO(J, A->value.integer P B->value.floating); \
                } \
        } \
        if (A->kind == OBJ_FLOAT) { \
                if (B->kind == OBJ_INT) { \
                        EV_FLO(J, A->value.floating P B->value.integer); \
                } \
                if (B->kind == OBJ_FLOAT) { \
                        EV_FLO(J, A->value.floating P B->value.floating); \
                } \
        }

#define OP_B(A, B, J, P) \
        if (A->kind == OBJ_INT) { \
                if (B->kind == OBJ_INT) { \
                        EV_BOL(J, A->value.integer P B->value.integer); \
                } \
                if (B->kind == OBJ_FLOAT) { \
                        EV_BOL(J, A->value.integer P B->value.floating); \
                } \
        } \
        if (A->kind == OBJ_FLOAT) { \
                if (B->kind == OBJ_INT) { \
                        EV_BOL(J, A->value.floating P B->value.integer); \
                } \
                if (B->kind == OBJ_FLOAT) { \
                        EV_BOL(J, A->value.floating P B->value.floating); \
                } \
        }

object *binary_op(u_int8_t op, object *a, object *b) {
        object *je = malloc(sizeof(object));
        switch (op) {
        case TO_ADD:
                OP_A(a, b, je, +)
                if (a->kind == OBJ_STRING && b->kind == OBJ_STRING) {
                        je->kind = OBJ_STRING;
                        char *new = malloc(sizeof(char) * STRING_CAP_MAX);
                        sprintf(new, "%s%s", a->value.string, b->value.string);
                        je->value.string = new;
                        je_ins = true;
                }
                break;
        case TO_SUB:
                OP_A(a, b, je, -) break;
        case TO_MUL:
                OP_A(a, b, je, *) break;
        case TO_DIV:
                if (a->kind == OBJ_INT) {
                        if (b->kind == OBJ_INT && b->value.integer > 0) {
                                EV_FLO(je, a->value.integer / b->value.integer);
                        }
                        if (b->kind == OBJ_FLOAT && b->value.floating > 0) {
                                EV_FLO(je,
                                       a->value.integer / b->value.floating);
                        }
                }
                if (a->kind == OBJ_FLOAT) {
                        if (b->kind == OBJ_INT && b->value.integer > 0) {
                                EV_FLO(je,
                                       a->value.floating / b->value.integer);
                        }
                        if (b->kind == OBJ_FLOAT && b->value.floating > 0) {
                                EV_FLO(je,
                                       a->value.floating / b->value.floating);
                        }
                }
                break;
        case TO_SUR:
                if (a->kind == OBJ_INT &&
                    (b->kind == OBJ_INT && b->value.integer > 0)) {
                        EV_FLO(je, a->value.integer % b->value.integer);
                }
                break;
        case TO_GR:
                OP_B(a, b, je, >) break;
        case TO_GR_EQ:
                OP_B(a, b, je, >=) break;
        case TO_LE:
                OP_B(a, b, je, <) break;
        case TO_LE_EQ:
                OP_B(a, b, je, <=) break;
        case TO_EQ_EQ:
                OP_B(a, b, je, ==);
                if (a->kind == OBJ_INT && b->kind == OBJ_BOOL) {
                        if (b->value.boolean) {
                                EV_BOL(je, a->value.integer > 0);
                        } else {
                                EV_BOL(je, a->value.integer <= 0);
                        }
                }
                if (a->kind == OBJ_FLOAT && b->kind == OBJ_BOOL) {
                        if (b->value.boolean) {
                                EV_BOL(je, a->value.floating > 0);
                        } else {
                                EV_BOL(je, a->value.floating <= 0);
                        }
                }
                if (a->kind == OBJ_BOOL) {
                        if (b->kind == OBJ_INT) {
                                if (a->value.boolean) {
                                        EV_BOL(je, b->value.integer > 0);
                                } else {
                                        EV_BOL(je, b->value.integer <= 0);
                                }
                        }
                        if (b->kind == OBJ_FLOAT) {
                                if (a->value.boolean) {
                                        EV_BOL(je, b->value.floating > 0);
                                } else {
                                        EV_BOL(je, b->value.floating <= 0);
                                }
                        }
                        if (b->kind == OBJ_BOOL) {
                                EV_BOL(je,
                                       a->value.boolean == b->value.boolean);
                        }
                }
                if (a->kind == OBJ_STRING && b->kind == OBJ_STRING) {
                        EV_BOL(je,
                               strcmp(a->value.string, b->value.string) == 0);
                }
                if (a->kind == OBJ_CHAR && b->kind == OBJ_CHAR) {
                        EV_BOL(je, a->value.ch == b->value.ch);
                }
                break;
        case TO_NOT_EQ:
                OP_B(a, b, je, !=);
                if (a->kind == OBJ_INT && b->kind == OBJ_BOOL) {
                        if (b->value.boolean) {
                                EV_BOL(je, a->value.integer <= 0);
                        } else {
                                EV_BOL(je, a->value.integer > 0);
                        }
                }
                if (a->kind == OBJ_FLOAT && b->kind == OBJ_BOOL) {
                        if (b->value.boolean) {
                                EV_BOL(je, a->value.floating <= 0);
                        } else {
                                EV_BOL(je, a->value.floating > 0);
                        }
                }
                if (a->kind == OBJ_BOOL) {
                        if (b->kind == OBJ_INT) {
                                if (a->value.boolean) {
                                        EV_BOL(je, b->value.integer <= 0);
                                } else {
                                        EV_BOL(je, b->value.integer > 0);
                                }
                        }
                        if (b->kind == OBJ_FLOAT) {
                                if (a->value.boolean) {
                                        EV_BOL(je, b->value.floating <= 0);
                                } else {
                                        EV_BOL(je, b->value.floating > 0);
                                }
                        }
                        if (b->kind == OBJ_BOOL) {
                                EV_BOL(je,
                                       a->value.boolean == b->value.boolean);
                        }
                }
                if (a->kind == OBJ_STRING && b->kind == OBJ_STRING) {
                        EV_BOL(je,
                               strcmp(a->value.string, b->value.string) != 0);
                }
                if (a->kind == OBJ_CHAR && b->kind == OBJ_CHAR) {
                        EV_BOL(je, a->value.ch != b->value.ch);
                }
                break;
        case TO_AND:
                OP_B(a, b, je, &&);
                if (a->kind == OBJ_INT && b->kind == OBJ_BOOL) {
                        if (b->value.boolean) {
                                EV_BOL(je, a->value.integer <= 0);
                        } else {
                                EV_BOL(je, a->value.integer > 0);
                        }
                }
                if (a->kind == OBJ_FLOAT && b->kind == OBJ_BOOL) {
                        if (b->value.boolean) {
                                EV_BOL(je, a->value.floating <= 0);
                        } else {
                                EV_BOL(je, a->value.floating > 0);
                        }
                }
                if (a->kind == OBJ_BOOL) {
                        if (b->kind == OBJ_INT) {
                                if (a->value.boolean) {
                                        EV_BOL(je, b->value.integer <= 0);
                                } else {
                                        EV_BOL(je, b->value.integer > 0);
                                }
                        }
                        if (b->kind == OBJ_FLOAT) {
                                if (a->value.boolean) {
                                        EV_BOL(je, b->value.floating <= 0);
                                } else {
                                        EV_BOL(je, b->value.floating > 0);
                                }
                        }
                        if (b->kind == OBJ_BOOL) {
                                EV_BOL(je,
                                       a->value.boolean == b->value.boolean);
                        }
                }
                break;
        case TO_OR:
                OP_B(a, b, je, ||);
                if (a->kind == OBJ_INT && b->kind == OBJ_BOOL) {
                        EV_BOL(je, a->value.integer > 0 || b->value.boolean);
                }
                if (a->kind == OBJ_FLOAT && b->kind == OBJ_BOOL) {
                        EV_BOL(je, a->value.floating > 0 || b->value.boolean);
                }
                if (a->kind == OBJ_BOOL) {
                        if (a->value.boolean) {
                                EV_BOL(je, a->value.boolean);
                        } else {
                                if (b->kind == OBJ_INT) {
                                        EV_BOL(je, b->value.integer > 0);
                                }
                                if (b->kind == OBJ_FLOAT) {
                                        EV_BOL(je, b->value.floating > 0);
                                }
                                if (b->kind == OBJ_BOOL) {
                                        EV_BOL(je, b->value.boolean);
                                }
                        }
                }
                break;
        }

        if (!je_ins) {
                free(je);
                return NULL;
        }
        je_ins = false;
        return je;
}

bool type_checker(type *tp, object *obj) {
        if (obj->kind == OBJ_NIL) {
                return true;
        }
        switch (tp->kind) {
        case T_ARRAY:
        case T_TUPLE:
                if (tp->kind == T_ARRAY && obj->kind != OBJ_ARR)
                        return false;
                if (tp->kind == T_TUPLE && obj->kind != OBJ_TUP)
                        return false;
                keg *elem;
                if (tp->kind == T_ARRAY)
                        elem = obj->value.arr.element;
                if (tp->kind == T_TUPLE)
                        elem = obj->value.tup.element;
                if (elem->item != 0) {
                        for (int i = 0; i < elem->item; i++) {
                                object *x = (object *)elem->data[i];
                                if (!type_checker((type *)tp->inner.single,
                                                  x)) {
                                        return false;
                                }
                        }
                }
                break;
        case T_MAP:
                if (obj->kind != OBJ_MAP)
                        return false;
                keg *k = obj->value.map.k;
                keg *v = obj->value.map.v;
                for (int i = 0; i < k->item; i++) {
                        if (!type_checker((type *)tp->inner.both.T1,
                                          (object *)k->data[i])) {
                                return false;
                        }
                }
                for (int i = 0; i < v->item; i++) {
                        if (!type_checker((type *)tp->inner.both.T2,
                                          (object *)v->data[i])) {
                                return false;
                        }
                }
                break;
        case T_FUNCTION:
                if (obj->kind != OBJ_FUNCTION)
                        return false;
                if (tp->inner.fn.arg->item != obj->value.fn.k->item)
                        return false;
                if (tp->inner.fn.ret != NULL) {
                        if (obj->value.fn.ret == NULL) {
                                return false;
                        }
                        if (((type *)tp->inner.fn.ret)->kind !=
                            ((type *)obj->value.fn.ret)->kind) {
                                return false;
                        }
                }
                for (int i = 0; i < tp->inner.fn.arg->item; i++) {
                        type *x = (type *)tp->inner.fn.arg->data[i];
                        type *y = (type *)obj->value.fn.v->data[i];
                        if (x->kind != y->kind) {
                                return false;
                        }
                        if (!type_checker(x,
                                          (object *)obj->value.fn.v->data[i])) {
                                return false;
                        }
                }
                break;
        default: {
                if ((tp->kind == T_INT && obj->kind != OBJ_INT) ||
                    (tp->kind == T_FLOAT && obj->kind != OBJ_FLOAT) ||
                    (tp->kind == T_STRING && obj->kind != OBJ_STRING) ||
                    (tp->kind == T_CHAR && obj->kind != OBJ_CHAR) ||
                    (tp->kind == T_BOOL && obj->kind != OBJ_BOOL)) {
                        return false;
                } else {
                        if (tp->kind == T_USER) {
                                const char *name = tp->inner.name;

                                if ((obj->kind == OBJ_FUNCTION &&
                                     strcmp(name, obj->value.fn.name) != 0) ||
                                    (obj->kind == OBJ_ENUMERATE &&
                                     strcmp(name, obj->value.en.name) != 0) ||
                                    (obj->kind == OBJ_INTERFACE &&
                                     strcmp(name, obj->value.in.name) != 0)) {
                                        return false;
                                }
                        }
                }
        }
        }
        return true;
}

bool obj_eq(object *a, object *b) {
        switch (a->kind) {
        case OBJ_INT:
                if (b->kind == OBJ_INT)
                        return a->value.integer == b->value.integer;
                break;
        case OBJ_FLOAT:
                if (b->kind == OBJ_FLOAT)
                        return a->value.floating == b->value.floating;
                break;
        case OBJ_CHAR:
                if (b->kind == OBJ_CHAR)
                        return a->value.ch == b->value.ch;
                break;
        case OBJ_STRING:
                if (b->kind == OBJ_STRING)
                        return strcmp(a->value.string, b->value.string) == 0;
                break;
        case OBJ_BOOL:
                if (b->kind == OBJ_BOOL)
                        return a->value.boolean ? b->value.boolean == true
                                                : b->value.boolean == false;
                break;
        default:
                return false;
        }
}

bool basic(object *obj) {
        if (obj->kind == OBJ_INT || obj->kind == OBJ_FLOAT ||
            obj->kind == OBJ_STRING || obj->kind == OBJ_CHAR ||
            obj->kind == OBJ_BOOL || obj->kind == OBJ_ARR ||
            obj->kind == OBJ_TUP || obj->kind == OBJ_MAP ||
            obj->kind == OBJ_NIL) {
                return true;
        }
        return false;
}

bool obj_kind_eq(object *a, object *b) {
        if ((a->kind == OBJ_INT && b->kind != OBJ_INT) ||
            (a->kind == OBJ_FLOAT && b->kind != OBJ_FLOAT) ||
            (a->kind == OBJ_STRING && b->kind != OBJ_STRING) ||
            (a->kind == OBJ_CHAR && b->kind != OBJ_CHAR) ||
            (a->kind == OBJ_BOOL && b->kind != OBJ_BOOL)) {
                return false;
        }
        return true;
}

const char *obj_type_string(object *obj) {
        switch (obj->kind) {
        case OBJ_INT:
                return "int";
        case OBJ_FLOAT:
                return "float";
        case OBJ_CHAR:
                return "char";
        case OBJ_STRING:
                return "string";
        case OBJ_BOOL:
                return "bool";
        case OBJ_ARR:
                return "array";
        case OBJ_TUP:
                return "tuple";
        case OBJ_MAP:
                return "map";
        case OBJ_FUNCTION:
                return "function";
        case OBJ_ENUMERATE:
                return "enumerate";
        case OBJ_CLASS:
                return "class";
        case OBJ_INTERFACE:
                return "interface";
        case OBJ_MODULE:
                return "module";
        case OBJ_NIL:
                return "nil";
        }
}

int obj_len(object *obj) {
        switch (obj->kind) {
        case OBJ_STRING:
                return strlen(obj->value.string);
        case OBJ_ARR:
                return obj->value.arr.element->item;
        case OBJ_TUP:
                return obj->value.tup.element->item;
        case OBJ_MAP:
                return obj->value.map.k->item;
        default:
                return -1;
        }
}