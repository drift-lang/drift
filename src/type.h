/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#ifndef FT_TYPE_H
#define FT_TYPE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "keg.h"

#define DEBUG_TYPE_STR_CAP 128

typedef enum {
  T_INT,
  T_FLOAT,
  T_CHAR,
  T_STRING,
  T_BOOL,
  T_ARRAY,
  T_TUPLE,
  T_MAP,
  T_FUNCTION,
  T_USER,
  T_ANY,
  T_GENERIC
} type_kind;

typedef struct {
  uint8_t kind;
  union {
    struct type *single;
    char *name;
    struct {
      keg *arg;
      struct type *ret;
    } fn;
    struct {
      struct type *T1;
      struct type *T2;
    } both;
    struct generic *ge;
  } inner;
} type;

typedef struct {
  char *name;
  int count;
  union {
    type *T;
    keg *multiple;
  } mtype;
} generic;

const char *type_string(type *);

bool type_eq(type *, type *);

bool copy_type(type *);

type *new_type(type_kind);

#endif