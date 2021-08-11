/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#ifndef FT_TYPE_H
#define FT_TYPE_H

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "list.h"

/* Type system */
typedef enum {
  T_INT,      // int
  T_FLOAT,    // float
  T_CHAR,     // char
  T_STRING,   // string
  T_BOOL,     // bool
  T_ARRAY,    // []T
  T_TUPLE,    // ()T
  T_MAP,      // {}<T1, T2>
  T_FUNCTION, // |[]T| -> T
  T_USER,     // ?
} type_kind;

/*
 * Type:
 *
 *   1. int       2.  float
 *   3. char      4.  string
 *   5. bool      6.  array
 *   7. tuple     8.  map
 *   9. function  10. user
 */
typedef struct {
  u_int8_t kind; /* Type system */
  union {
    struct type *single; /* Contains a single type */
    const char *name;    /* Customer type */
    struct {
      list *arg;        /* Function arguments */
      struct type *ret; /* Function returns */
    } fn;
    struct {
      struct type *T1;
      struct type *T2;
    } both; /* It contains two types */
  } inner;
} type;

/* Output type */
const char *type_string(type *);

/* Return two types is equal */
bool type_eq(type *, type *);

#endif