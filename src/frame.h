/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#ifndef FT_FRAME_H
#define FT_FRAME_H

#include "code.h"
#include "table.h"
#include "list.h"
#include "object.h"

/* Evaluate frame structure */
typedef struct {
    code_object *code; /* Code object list */
    table *tb; /* Object mapping table */
    list *data; /* Eval data stack */
    object *ret; /* Return value of frame */
    char *module; /* Module name */
} frame;

#endif
