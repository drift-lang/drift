/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#ifndef FT_CODE_H
#define FT_CODE_H

#include "list.h"

/* Generate object, subject data type. */
typedef struct {
  char *description; /* Description name */
  list *names;       /* Name set */
  list *codes;       /* Bytecode set */
  list *offsets;     /* Offset set */
  list *types;       /* Type set */
  list *objects;     /* Objects set */
  list *lines;       /* Lines set */
} code_object;

#endif