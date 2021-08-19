/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#ifndef FT_CODE_H
#define FT_CODE_H

#include "keg.h"

/* Generate object, subject data type. */
typedef struct {
    char *description; /* Description name */
    keg *names;        /* Name set */
    keg *codes;        /* Bytecode set */
    keg *offsets;      /* Offset set */
    keg *types;        /* Type set */
    keg *objects;      /* Object set */
    keg *lines;        /* Line set */
} code_object;

#endif