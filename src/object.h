/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#ifndef FT_OBJECT_H
#define FT_OBJECT_H

#include <stdio.h>
#include <stdbool.h>

#include "list.h"
#include "type.h"
#include "code.h"
#include "opcode.h"

/* Object type */
typedef enum {
    OBJ_INT,  OBJ_FLOAT, OBJ_STRING, OBJ_CHAR, OBJ_BOOL,
    OBJ_ENUM, OBJ_FUNC,  OBJ_WHOLE,  OBJ_FACE,
} obj_kind;

/* Object system */
typedef struct {
    u_int8_t kind; /* Kind */
    union {
        int integer; /* int */
        double floating; /* float */
        char *string; /* string */
        char ch; /* char */
        bool boolean; /* bool */
        struct {
            char *name;
            list *element;
        } enumeration; /* enum */
        struct {
            char *name;
            list *k;
            list *v;
            type *ret;
            code_object *code;
        } func; /* function */
        struct {
            char *name;
            list *element;
        } face; /* interface */
        struct {
            char *name;
            code_object *code;
        } whole; /* whole */
    } value; /* Inner value */
} object;

/* Output object */
const char *obj_string(object *obj);

/* Binary operation */
object *binary_op(u_int8_t op, object *a, object *b);

#endif