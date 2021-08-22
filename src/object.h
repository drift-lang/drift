/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#ifndef FT_OBJECT_H
#define FT_OBJECT_H

#include <stdbool.h>
#include <stdio.h>

#include "code.h"
#include "keg.h"
#include "opcode.h"
#include "type.h"

#define DEBUG_OBJ_STR_CAP 128
#define STRING_CAP        128
#define STRING_CAP_MAX    1024

typedef enum {
    OBJ_INT,
    OBJ_FLOAT,
    OBJ_STRING,
    OBJ_CHAR,
    OBJ_BOOL,
    OBJ_ENUMERATE,
    OBJ_FUNCTION,
    OBJ_CLASS,
    OBJ_INTERFACE,
    OBJ_ARR,
    OBJ_TUP,
    OBJ_MAP,
    OBJ_MODULE,
    OBJ_NIL
} obj_kind;

typedef struct {
    u_int8_t kind;
    union {
        int integer;
        double floating;
        char *string;
        char ch;
        bool boolean;
        struct {
            char *name;
            keg *element;
        } en;
        struct {
            char *name;
            keg *k;
            keg *v;
            type *ret;
            code_object *code;
            bool std;
        } fn;
        struct {
            char *name;
            keg *element;
            struct object *class;
        } in;
        struct {
            char *name;
            code_object *code;
            struct frame *fr;
            bool init;
        } cl;
        struct {
            keg *element;
            type *T;
        } arr;
        struct {
            keg *element;
            type *T;
        } tup;
        struct {
            keg *k;
            keg *v;
            type *T1;
            type *T2;
        } map;
        struct {
            struct table *tb;
        } mod;
    } value;
} object;

typedef struct {
    char *name;
    keg *T;
    type *ret;
} method;

const char *obj_string(object *);

const char *obj_raw_string(object *);

object *binary_op(u_int8_t, object *, object *);

bool type_checker(type *, object *);

bool obj_eq(object *, object *);

bool obj_kind_eq(object *, object *);

const char *obj_type_string(object *);

int obj_len(object *);

#endif