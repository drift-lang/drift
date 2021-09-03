/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#ifndef FT_OBJECT_H
#define FT_OBJECT_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "code.h"
#include "keg.h"
#include "opcode.h"
#include "type.h"

#define DEBUG_OBJ_STR_CAP 64
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
  OBJ_ARRAY,
  OBJ_TUPLE,
  OBJ_MAP,
  OBJ_MODULE,
  OBJ_NIL,
  OBJ_BUILTIN,
  OBJ_CFUNC,
  OBJ_CMODS,
  OBJ_CUSER
} obj_kind;

typedef enum {
  BU_FUNCTION,
  BU_NAME,
} builtin_kind;

typedef struct {
  uint8_t kind;
  union {
    int num;
    double f;
    char *str;
    char c;
    bool b;
    struct {
      char *name;
      keg *element;
    } en;
    struct {
      char *name;
      keg *k;
      keg *v;
      type *mutiple;
      type *ret;
      code_object *code;
      void *self;
      keg *gt;
    } fn;
    struct {
      char *name;
      keg *element;
      struct object *class;
      keg *gt;
    } in;
    struct {
      char *name;
      code_object *code;
      struct frame *fr;
      bool init;
      keg *gt;
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
      char *name;
      struct table *tb;
    } mod;
    struct {
      builtin_kind kind;
      char *name;
      void *func;
    } bu;
    struct {
      const char *name;
      void (*func)(keg *);
    } cf;
    struct {
      const char *name;
      keg *var;
      keg *met;
    } cm;
    struct {
      void *ptr;
    } cu;
  } value;
} object;

typedef struct {
  char *name;
  keg *arg;
  type *ret;
} method;

typedef struct {
  obj_kind l;
  obj_kind r;
  int m;
} eval_op_rule;

typedef struct {
  char *name;
  void *ptr;
} addr_kv;

const char *obj_string(object *);

const char *obj_raw_string(object *, bool);

object *binary_op(uint8_t, object *, object *);

bool type_checker(type *, object *);

bool obj_eq(object *, object *);

bool obj_kind_eq(object *, object *);

const char *obj_type_string(object *);

int obj_len(object *);

object *new_num(int);
object *new_float(double);
object *new_string(char *);
object *new_char(char);
object *new_bool(bool);
object *new_userdata(void *);

#endif