/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#ifndef FT_CODE_H
#define FT_CODE_H

#include "list.h"

/* Generate object, subject data type. */
typedef struct {
    char *description; /* Description name */
    list *names; /* Name set */
    list *codes; /* Bytecode set */
    list *offsets; /* Offset set */
    list *types; /* Type set */
    list *objects; /* Objects set */
    list *lines; /* Lines set */
} code_object;

/* Allocate new code object */
code_object *new_code(char *);

#endif