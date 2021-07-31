/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#ifndef FT_COMPILER_H
#define FT_COMPILER_H

#include <stdbool.h>
#include <stdio.h>

#include "list.h"
#include "token.h"
#include "opcode.h"
#include "type.h"
#include "object.h"
#include "method.h"

/* Compilation status*/
typedef struct {
    list *tokens;
    token pre; /* Last token */
    token cur; /* Current token */
    int16_t iof; /* Offset of object */
    int16_t inf; /* Offset of name */
    int16_t itf; /* Offset of type */
    int p; /* Read position */
    bool loop_handler; /* Is current handing loop statement? */
    list *codes; /* Compiled code object */
} compile_state;

/* Compiler */
list *compile(list *tokens);

/* Detailed information */
void dissemble(code_object *code);

#endif