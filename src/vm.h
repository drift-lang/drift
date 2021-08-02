/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#ifndef FT_VM_H
#define FT_VM_H

#include <stdio.h>

#include "list.h"
#include "code.h"
#include "frame.h"
#include "opcode.h"

/* Virtual machine state */
typedef struct {
    list *frame; /* Frame list */
    int16_t op; /* Position of offset */
    int16_t ip; /* IP */
    bool loop_ret; /* Break loop */
} vm_state;

/* Evaluate code object */
vm_state evaluate(code_object *);

#endif