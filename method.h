/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#ifndef FT_METHOD_H
#define FT_METHOD_H

#include "list.h"
#include "type.h"

/* Method structure of interface */
typedef struct {
    char *name; /* Face name */
    list *T; /* Types of arguments */
    type *ret; /* Return type */
} method;

#endif