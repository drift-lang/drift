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

#define DEBUG_OBJ_STR_CAP 32
#define STRING_CAP        128
#define STRING_CAP_MAX    1024

/* Object type */
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

/* Object system */
typedef struct {
    u_int8_t kind; /* Kind */
    union {
        int integer;     /* int */
        double floating; /* float */
        char *string;    /* string */
        char ch;         /* char */
        bool boolean;    /* bool */
        struct {
            char *name;
            keg *element;
        } en; /* enum */
        struct {
            char *name;
            keg *k;
            keg *v;
            type *ret;
            code_object *code;
            bool std;
        } fn; /* function */
        struct {
            char *name;
            keg *element;
            struct object *class;
        } in; /* interface */
        struct {
            char *name;
            code_object *code;
            struct frame *fr;
            bool init;
        } cl; /* class */
        struct {
            keg *element;
            type *T;
        } arr; /* array */
        struct {
            keg *element;
            type *T;
        } tup; /* tuple */
        struct {
            keg *k;
            keg *v;
            type *T1;
            type *T2;
        } map; /* map */
        struct {
            struct table *tb;
        } mod; /* module */
    } value;   /* Inner value */
} object;

/* Method structure of interface */
typedef struct {
    char *name; /* Face name */
    keg *T;     /* Types of arguments */
    type *ret;  /* Return type */
} method;

/* Output object */
const char *obj_string(object *);

/* Object's raw data */
const char *obj_raw_string(object *);

/* Binary operation */
object *binary_op(u_int8_t, object *, object *);

/* The judgement type is the same */
bool type_checker(type *, object *);

/* Are the two objects equal */
bool obj_eq(object *, object *);

/* Is the basic type */
bool basic(object *);

/* Are the types of the two objects consistent */
bool obj_kind_eq(object *, object *);

/* Type string of object */
const char *obj_type_string(object *);

/* Return the length of object */
int obj_len(object *);

#endif