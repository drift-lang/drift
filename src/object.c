/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#include "object.h"

/* Output object */
const char *obj_string(object *obj) {
    char *str = (char *) malloc(sizeof(char) * 128);
    switch (obj->kind) {
        case OBJ_INT:    sprintf(str, "int %d", obj->value.integer);              return str;
        case OBJ_FLOAT:  sprintf(str, "float %f", obj->value.floating);           return str;
        case OBJ_STRING: sprintf(str, "string \"%s\"", obj->value.string);        return str;
        case OBJ_CHAR:   sprintf(str, "char '%c'", obj->value.ch);                return str;
        case OBJ_BOOL:   sprintf(str, "bool %s", obj->value.boolean ? "T" : "F"); return str;
        case OBJ_ENUM:
            sprintf(str, "enum \"%s\"", obj->value.enumeration.name);
            return str;
        case OBJ_FUNC:
            sprintf(str, "func \"%s\"", obj->value.func.name);
            return str;
        case OBJ_FACE:
            sprintf(str, "face \"%s\"", obj->value.face.name);
            return str;
        case OBJ_WHOLE:
            sprintf(str, "whole \"%s\"", obj->value.whole.name);
            return str;
        case OBJ_ARR:
            sprintf(str, "arr (%d)", obj->value.arr.element->len);
            return str;
        case OBJ_TUP:
            sprintf(str, "tup (%d)", obj->value.tup.element->len);
            return str;
        case OBJ_MAP:
            sprintf(str, "map (%d)", obj->value.map.v->len);
            return str;
    }
}

/* Object's raw data */
const char *obj_raw_string(object *obj) {
    char *str = (char *) malloc(sizeof(char) * 128);
    switch (obj->kind) {
        case OBJ_INT:    sprintf(str, "%d", obj->value.integer);             return str;
        case OBJ_FLOAT:  sprintf(str, "%f", obj->value.floating);            return str;
        case OBJ_STRING: sprintf(str, "%s", obj->value.string);              return str;
        case OBJ_CHAR:   sprintf(str, "%c", obj->value.ch);                  return str;
        case OBJ_BOOL:   sprintf(str, "%s", obj->value.boolean ? "T" : "F"); return str;
        case OBJ_ARR: { /* Array */
            list *elem = obj->value.arr.element;
            if (elem->len == 0) {
                free(str);
                return "[]";
            }
            sprintf(str, "[");
            for (int i = 0; i < elem->len; i ++) {
                strcat(str,
                    obj_raw_string((object *)elem->data[i]));
                if (i + 1 != elem->len) {
                    strcat(str, ", ");
                }
            }
            strcat(str, "]");
            return str;
        }
        case OBJ_TUP: { /* Tuple */
            list *elem = obj->value.arr.element;
            if (elem->len == 0) {
                free(str);
                return "()";
            }
            sprintf(str, "(");
            for (int i = 0; i < elem->len; i ++) {
                strcat(str,
                    obj_raw_string((object *)elem->data[i]));
                if (i + 1 != elem->len) {
                    strcat(str, ", ");
                }
            }
            strcat(str, ")");
            return str;
        }
        case OBJ_MAP: { /* Map */
            list *k = obj->value.map.k;
            list *v = obj->value.map.v;
            if (k->len == 0) {
                free(str);
                return "{}";
            }
            sprintf(str, "{");
            for (int i = 0; i < k->len; i ++) {
                strcat(str,
                    obj_raw_string((object *)k->data[i]));
                strcat(str, ": ");
                strcat(str,
                    obj_raw_string((object *)v->data[i]));
                if (i + 1 != k->len) {
                    strcat(str, ", ");
                }
            }
            strcat(str, "}");
            return str;
        }
        default: { /* Other */
            free(str);
            return obj_string(obj);
        }
    }
}

/* Indicates whether an object is executed */
bool je_ins = false;

/* Easy to set object properties */
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

/* Digital operation: + - **/
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

/* Logical operation: > >= < <= == != | & */
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

/* Binary operation */
object *binary_op(u_int8_t op, object *a, object *b) {
    object *je = (object *) malloc(sizeof(object));
    switch (op) {
        case TO_ADD:
            OP_A(a, b, je, +)
            if (a->kind == OBJ_STRING && b->kind == OBJ_STRING) { /* string + string */
                je->kind = OBJ_STRING;
                
                char *new = malloc(sizeof(char) * strlen(a->value.string));
                strcpy(new, a->value.string); /* Copy string */
                strcat(new, b->value.string); /* To new string*/
                
                je->value.string = new;
                je_ins = true;
            }
            break;
        case TO_SUB: OP_A(a, b, je, -) break;
        case TO_MUL: OP_A(a, b, je, *) break;
        case TO_DIV:
            if (a->kind == OBJ_INT) {
                if (b->kind == OBJ_INT && b->value.integer > 0) { /* int / int */
                    EV_FLO(je,
                        a->value.integer / b->value.integer);
                }
                if (b->kind == OBJ_FLOAT && b->value.floating > 0) { /* int / float */
                    EV_FLO(je,
                        a->value.integer / b->value.floating);
                }
            }
            if (a->kind == OBJ_FLOAT) {
                if (b->kind == OBJ_INT && b->value.integer > 0) { /* float / int */
                    EV_FLO(je,
                        a->value.floating / b->value.integer);
                }
                if (b->kind == OBJ_FLOAT && b->value.floating > 0) { /* float / float */
                    EV_FLO(je,
                        a->value.floating / b->value.floating);
                }
            }
            break;
        case TO_SUR:
            if (a->kind == OBJ_INT &&
                (b->kind == OBJ_INT && b->value.integer > 0)) { /* int % int */
                    EV_FLO(je,
                        a->value.integer % b->value.integer);
                }
            break;
        case TO_GR:     OP_B(a, b, je, >)  break;
        case TO_GR_EQ:  OP_B(a, b, je, >=) break;
        case TO_LE:     OP_B(a, b, je, <)  break;
        case TO_LE_EQ:  OP_B(a, b, je, <=) break;
        case TO_EQ_EQ:
            OP_B(a, b, je, ==);
            if (a->kind == OBJ_INT && b->kind == OBJ_BOOL) { /* int == bool */
                if (b->value.boolean) {
                    EV_BOL(je, a->value.integer > 0);
                } else {
                    EV_BOL(je, a->value.integer <= 0);
                }
            }
            if (a->kind == OBJ_FLOAT && b->kind == OBJ_BOOL) { /* float == bool */
                if (b->value.boolean) {
                    EV_BOL(je, a->value.floating > 0);
                } else {
                    EV_BOL(je, a->value.floating <= 0);
                }
            }
            if (a->kind == OBJ_BOOL) {
                if (b->kind == OBJ_INT) { /* bool == int */
                    if (a->value.boolean) {
                        EV_BOL(je, b->value.integer > 0);
                    } else {
                        EV_BOL(je, b->value.integer <= 0);
                    }
                }
                if (b->kind == OBJ_FLOAT) { /* float == bool */
                    if (a->value.boolean) {
                        EV_BOL(je, b->value.floating > 0);
                    } else {
                        EV_BOL(je, b->value.floating <= 0);
                    }
                }
                if (b->kind == OBJ_BOOL) { /* bool == bool */
                    EV_BOL(je,
                        a->value.boolean == b->value.boolean);
                }
            }
            if (a->kind == OBJ_STRING && b->kind == OBJ_STRING) { /* string == string */
                EV_BOL(je,
                    strcmp(a->value.string, b->value.string) == 0);
            }
            if (a->kind == OBJ_CHAR && b->kind == OBJ_CHAR) { /* char == char */
                EV_BOL(je,
                    a->value.ch == b->value.ch);
            }
            break;
        case TO_NOT_EQ:
            OP_B(a, b, je, !=);
            if (a->kind == OBJ_INT && b->kind == OBJ_BOOL) { /* int != bool */
                if (b->value.boolean) {
                    EV_BOL(je,
                        a->value.integer <= 0);
                } else {
                    EV_BOL(je, a->value.integer > 0);
                }
            }
            if (a->kind == OBJ_FLOAT && b->kind == OBJ_BOOL) { /* float != bool */
                if (b->value.boolean) {
                    EV_BOL(je, a->value.floating <= 0);
                } else {
                    EV_BOL(je, a->value.floating > 0);
                }
            }
            if (a->kind == OBJ_BOOL) {
                if (b->kind == OBJ_INT) {
                    if (a->value.boolean) { /* bool != int */
                        EV_BOL(je, b->value.integer <= 0);
                    } else {
                        EV_BOL(je, b->value.integer > 0);
                    }
                }
                if (b->kind == OBJ_FLOAT) { /* bool != float */
                    if (a->value.boolean) {
                        EV_BOL(je, b->value.floating <= 0);
                    } else {
                        EV_BOL(je, b->value.floating > 0);
                    }
                }
                if (b->kind == OBJ_BOOL) { /* bool != bool */
                    EV_BOL(je,
                        a->value.boolean == b->value.boolean);
                }
            }
            if (a->kind == OBJ_STRING && b->kind == OBJ_STRING) { /* string != string */
                EV_BOL(je,
                    strcmp(a->value.string, b->value.string) != 0);
            }
            if (a->kind == OBJ_CHAR && b->kind == OBJ_CHAR) { /* char != char */
                EV_BOL(je,
                    a->value.ch != b->value.ch);
            }
            break;
        case TO_AND:
            OP_B(a, b, je, &&);
            if (a->kind == OBJ_INT && b->kind == OBJ_BOOL) { /* int & bool */
                if (b->value.boolean) {
                    EV_BOL(je, a->value.integer <= 0);
                } else {
                    EV_BOL(je, a->value.integer > 0);
                }
            }
            if (a->kind == OBJ_FLOAT && b->kind == OBJ_BOOL) { /* float & bool */
                if (b->value.boolean) {
                    EV_BOL(je, a->value.floating <= 0);
                } else {
                    EV_BOL(je, a->value.floating > 0);
                }
            }
            if (a->kind == OBJ_BOOL) {
                if (b->kind == OBJ_INT) { /* bool & int */
                    if (a->value.boolean) {
                        EV_BOL(je, b->value.integer <= 0);
                    } else {
                        EV_BOL(je, b->value.integer > 0);
                    }
                }
                if (b->kind == OBJ_FLOAT) { /* bool & float */
                    if (a->value.boolean) {
                        EV_BOL(je, b->value.floating <= 0);
                    } else {
                        EV_BOL(je, b->value.floating > 0);
                    }
                }
                if (b->kind == OBJ_BOOL) { /* bool & bool */
                    EV_BOL(je,
                        a->value.boolean == b->value.boolean);
                }
            }
            break;
        case TO_OR:
            OP_B(a, b, je, ||);
            if (a->kind == OBJ_INT && b->kind == OBJ_BOOL) { /* int | bool */
                EV_BOL(je,
                    a->value.integer > 0 || b->value.boolean);
            }
            if (a->kind == OBJ_FLOAT && b->kind == OBJ_BOOL) { /* float | bool */
                EV_BOL(je,
                    a->value.floating > 0 || b->value.boolean);
            }
            if (a->kind == OBJ_BOOL) {
                if (a->value.boolean) { /* bool | bool */
                    EV_BOL(je, a->value.boolean);
                } else {
                    if (b->kind == OBJ_INT) { /* bool | int */
                        EV_BOL(je, b->value.integer > 0);
                    }
                    if (b->kind == OBJ_FLOAT) { /* bool | float */
                        EV_BOL(je, b->value.floating > 0);
                    }
                    if (b->kind == OBJ_BOOL) { /* bool | bool */
                        EV_BOL(je, b->value.boolean);
                    }
                }
            }
            break;
    }
    /* Returns NULL if not executed */
    if (!je_ins) {
        free(je);
        return NULL;
    }
    je_ins = false;
    return je;
}

/* The judgement type is the same */
bool type_checker(type *tp, object *obj) {
    switch (tp->kind) {
        case T_ARRAY:
        case T_TUPLE:
            if (tp->kind == T_ARRAY && obj->kind != OBJ_ARR)
                return false;
            if (tp->kind == T_TUPLE && obj->kind != OBJ_TUP)
                return false;
            list *elem;
            if (tp->kind == T_ARRAY) elem = obj->value.arr.element;
            if (tp->kind == T_TUPLE) elem = obj->value.tup.element;
            if (elem->len != 0) {
                for (int i = 0; i < elem->len; i ++) {
                    object *x = (object *)elem->data[i];
                    if (!type_checker(
                            (type *)tp->inner.single, x)) {
                        return false;
                    }
                }
            }
            break;
        case T_MAP:
            if (obj->kind != OBJ_MAP)
                return false;
            list *k = obj->value.map.k;
            list *v = obj->value.map.v;
            for (int i = 0; i < k->len; i ++) {
                if (!type_checker(
                        (type *)tp->inner.both.T1, (object *)k->data[i])) {
                    return false;
                }
            }
            for (int i = 0; i < v->len; i ++) {
                if (!type_checker(
                        (type *)tp->inner.both.T2, (object *)v->data[i])) {
                    return false;
                }
            }
            break;
        case T_FUNC:
            if (obj->kind != OBJ_FUNC)
                return false;
            if (tp->inner.func.arg->len != obj->value.func.k->len)
                return false;
            if (tp->inner.func.ret != NULL) {
                if (obj->value.func.ret == NULL) {
                    return false;
                }
                if (
                    ((type *)tp->inner.func.ret)->kind !=
                    ((type *)obj->value.func.ret)->kind) {
                        return false;
                    }
            }
            for (int i = 0; i < tp->inner.func.arg->len; i ++) {
                type *x =
                    (type *)tp->inner.func.arg->data[i];
                type *y =
                    (type *)obj->value.func.v->data[i];
                if (x->kind != y->kind) {
                    return false;
                }
                if (!type_checker(
                        x, (object *)obj->value.func.v->data[i])) {
                            return false;
                        }
            }
            break;
        default: {
            if (
                (tp->kind == T_INT    && obj->kind != OBJ_INT) ||
                (tp->kind == T_FLOAT  && obj->kind != OBJ_FLOAT) ||
                (tp->kind == T_STRING && obj->kind != OBJ_STRING) ||
                (tp->kind == T_CHAR   && obj->kind != OBJ_CHAR) ||
                (tp->kind == T_BOOL   && obj->kind != OBJ_BOOL)
            ) {
                return false;
            } else {
                if (tp->kind == T_USER) {
                    const char *name = tp->inner.name;

                    if (
                        (obj->kind == OBJ_FUNC &&
                            strcmp(name, obj->value.func.name) != 0) ||
                        (obj->kind == OBJ_ENUM &&
                            strcmp(name, obj->value.enumeration.name) != 0) ||
                        (obj->kind == OBJ_FACE &&
                            strcmp(name, obj->value.face.name) != 0) ||
                        (obj->kind == OBJ_WHOLE &&
                            strcmp(name, obj->value.whole.name) != 0)
                    ) {
                        return false;   
                    }
                }
            }
        }
    }
    return true;
}

/* Are the two objects equal */
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
                return a->value.boolean ?
                    b->value.boolean == true : b->value.boolean == false;
            break;
        default:
            return false;
    }
}

/* Is the basic type */
bool basic(object *obj) {
    if (
        obj->kind == OBJ_INT    || obj->kind == OBJ_FLOAT ||
        obj->kind == OBJ_STRING || obj->kind == OBJ_CHAR  ||
        obj->kind == OBJ_BOOL   || obj->kind == OBJ_ARR   ||
        obj->kind == OBJ_TUP    || obj->kind == OBJ_MAP) {
            return true;
        }
    return false;
}

/* Are the types of the two objects consistent */
bool obj_kind_eq(object *a, object *b) {
    if (
        (a->kind == OBJ_INT    && b->kind != OBJ_INT)    ||
        (a->kind == OBJ_FLOAT  && b->kind != OBJ_FLOAT)  ||
        (a->kind == OBJ_STRING && b->kind != OBJ_STRING) ||
        (a->kind == OBJ_CHAR   && b->kind != OBJ_CHAR)   ||
        (a->kind == OBJ_BOOL   && b->kind != OBJ_BOOL)) {
        return false;
    }
    return true;
}

/* Type string of object */
const char *obj_type_string(object *obj) {
    switch (obj->kind) {
        case OBJ_INT:    return "int";
        case OBJ_FLOAT:  return "float";
        case OBJ_CHAR:   return "char";
        case OBJ_STRING: return "string";
        case OBJ_BOOL:   return "bool";
        case OBJ_ARR:    return "array";
        case OBJ_TUP:    return "tuple";
        case OBJ_MAP:    return "map";
        case OBJ_FUNC:   return "func";
        case OBJ_ENUM:   return "enum";
        case OBJ_WHOLE:  return "whole";
        case OBJ_FACE:   return "face";
    }
}

/* Return the length of object */
int obj_len(object *obj) {
    switch (obj->kind) {
        case OBJ_STRING:
            return strlen(obj->value.string);
        case OBJ_ARR:
            return obj->value.arr.element->len;
        case OBJ_TUP:
            return obj->value.tup.element->len;
        case OBJ_MAP:
            return obj->value.map.k->len;
        default:
            return -1;
    }
}