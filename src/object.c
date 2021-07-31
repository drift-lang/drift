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
    }
}

/* Indicates whether an object is executed */
bool je_ins = false;

/* Easy to set object properties */
#define EV_INT(obj, val) obj->kind = OBJ_INT; \
    obj->value.integer = val; \
    je_ins = true
#define EV_FLO(obj, val) obj->kind = OBJ_FLOAT; \
    obj->value.floating = val; \
    je_ins = true
#define EV_BOL(obj, val) obj->kind = OBJ_BOOL; \
    obj->value.boolean = val; \
    je_ins = true

/* Digital operation: + - **/
#define OP_A(A, B, J, P) \
if (A->kind == OBJ_INT) { \
    if (B->kind == OBJ_INT) \
        EV_INT(J, \
            A->value.integer P B->value.integer); \
    if (B->kind == OBJ_FLOAT) { \
        EV_FLO(J, \
            A->value.integer P B->value.floating); \
        } \
    } \
if (A->kind == OBJ_FLOAT) { \
    if (B->kind == OBJ_INT) \
        EV_FLO(J, \
            A->value.floating P B->value.integer); \
    if (B->kind == OBJ_FLOAT) { \
        EV_FLO(J, \
            A->value.floating P B->value.floating); \
    } \
}

/* Logical operation: > >= < <= == != | & */
#define OP_B(A, B, J, P) \
if (A->kind == OBJ_INT) { \
    if (B->kind == OBJ_INT) \
        EV_BOL(J, \
            A->value.integer P B->value.integer); \
    if (B->kind == OBJ_FLOAT) { \
        EV_BOL(J, \
            A->value.integer P B->value.floating); \
        } \
    } \
if (A->kind == OBJ_FLOAT) { \
    if (B->kind == OBJ_INT) \
        EV_BOL(J, \
            A->value.floating P B->value.integer); \
    if (B->kind == OBJ_FLOAT) { \
        EV_BOL(J, \
            A->value.floating P B->value.floating); \
    } \
}

/* Binary operation */
object *binary_op(u_int8_t op, object *a, object *b) {
    object *je = (object *) malloc(sizeof(object));
    switch (op) {
        case TO_ADD:
            OP_A(a, b, je, +)
            if (a->kind == OBJ_STRING && b->kind == OBJ_STRING) {
                je->kind = OBJ_STRING;
                strcat(a->value.string, b->value.string);
                je->value.string = a->value.string;
            }
            break;
        case TO_SUB: OP_A(a, b, je, -) break;
        case TO_MUL: OP_A(a, b, je, *) break;
        case TO_DIV:
            if (a->kind == OBJ_INT) {
                if (b->kind == OBJ_INT && b->value.integer > 0) {
                    EV_FLO(je,
                        a->value.integer / b->value.integer);
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
            if (a->kind == OBJ_INT && b->kind == OBJ_BOOL) {
                if (b->value.boolean) {
                    EV_BOL(je,
                        a->value.integer > 0);
                } else
                    EV_BOL(je, a->value.integer <= 0);
            }
            if (a->kind == OBJ_FLOAT && b->kind == OBJ_BOOL) {
                if (b->value.boolean) {
                    EV_BOL(je,
                        a->value.floating > 0);
                } else
                    EV_BOL(je, a->value.floating <= 0);
            }
            if (a->kind == OBJ_BOOL) {
                if (b->kind == OBJ_INT)
                    if (a->value.boolean) {
                        EV_BOL(je, b->value.integer > 0);
                    } else
                        EV_BOL(je, b->value.integer <= 0);
                if (b->kind == OBJ_FLOAT)
                    if (a->value.boolean) {
                        EV_BOL(je, b->value.floating > 0);
                    } else
                        EV_BOL(je, b->value.floating <= 0);
                if (b->kind == OBJ_BOOL) {
                    EV_BOL(je,
                        a->value.boolean == b->value.boolean);
                }
            }
            if (a->kind == OBJ_STRING && b->kind == OBJ_STRING) {
                EV_BOL(je,
                    strcmp(a->value.string, b->value.string) == 0);
            }
            break;
        case TO_NOT_EQ:
            OP_B(a, b, je, !=);
            if (a->kind == OBJ_INT && b->kind == OBJ_BOOL) {
                if (b->value.boolean) {
                    EV_BOL(je,
                        a->value.integer <= 0);
                } else
                    EV_BOL(je, a->value.integer > 0);
            }
            if (a->kind == OBJ_FLOAT && b->kind == OBJ_BOOL) {
                if (b->value.boolean) {
                    EV_BOL(je,
                        a->value.floating <= 0);
                } else
                    EV_BOL(je, a->value.floating > 0);
            }
            if (a->kind == OBJ_BOOL) {
                if (b->kind == OBJ_INT)
                    if (a->value.boolean) {
                        EV_BOL(je, b->value.integer <= 0);
                    } else
                        EV_BOL(je, b->value.integer > 0);
                if (b->kind == OBJ_FLOAT)
                    if (a->value.boolean) {
                        EV_BOL(je, b->value.floating <= 0);
                    } else
                        EV_BOL(je, b->value.floating > 0);
                if (b->kind == OBJ_BOOL) {
                    EV_BOL(je,
                        a->value.boolean == b->value.boolean);
                }
            }
            if (a->kind == OBJ_STRING && b->kind == OBJ_STRING) {
                EV_BOL(je,
                    strcmp(a->value.string, b->value.string) != 0);
            }
            break;
        case TO_AND:
            OP_B(a, b, je, &&);
            if (a->kind == OBJ_INT && b->kind == OBJ_BOOL) {
                if (b->value.boolean) {
                    EV_BOL(je,
                        a->value.integer <= 0);
                } else
                    EV_BOL(je, a->value.integer > 0);
            }
            if (a->kind == OBJ_FLOAT && b->kind == OBJ_BOOL) {
                if (b->value.boolean) {
                    EV_BOL(je,
                        a->value.floating <= 0);
                } else
                    EV_BOL(je, a->value.floating > 0);
            }
            if (a->kind == OBJ_BOOL) {
                if (b->kind == OBJ_INT)
                    if (a->value.boolean) {
                        EV_BOL(je, b->value.integer <= 0);
                    } else
                        EV_BOL(je, b->value.integer > 0);
                if (b->kind == OBJ_FLOAT)
                    if (a->value.boolean) {
                        EV_BOL(je, b->value.floating <= 0);
                    } else
                        EV_BOL(je, b->value.floating > 0);
                if (b->kind == OBJ_BOOL) {
                    EV_BOL(je,
                        a->value.boolean == b->value.boolean);
                }
            }
            break;
        case TO_OR:
            OP_B(a, b, je, ||);
            if (a->kind == OBJ_INT && b->kind == OBJ_BOOL) {
                EV_BOL(je,
                    a->value.integer > 0 || b->value.boolean);
            }
            if (a->kind == OBJ_FLOAT && b->kind == OBJ_BOOL) {
                EV_BOL(je,
                    a->value.floating > 0 || b->value.boolean);
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
    /* Returns NULL if not executed */
    if (!je_ins) return NULL;
    je_ins = false;
    return je;
}
