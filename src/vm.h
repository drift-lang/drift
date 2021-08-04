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

#if defined(__linux__) || defined(__APPLE__)
    #include <unistd.h>
#elif defined(_WIN32)
    #include <windows.h>
    #include <dirent.h>
#endif

/* Virtual machine state */
typedef struct {
    list *frame; /* Frame list */
    int16_t op; /* Position of offset */
    int16_t ip; /* IP */
    bool loop_ret; /* Break loop */
    object *whole; /* Method of load whole */
} vm_state;

/* Evaluate code object */
vm_state evaluate(code_object *);

/* Built in function prototype */
typedef void (* built)(frame *, list *);

/* Builtin function */
typedef struct {
    char *name; /* Function name */
    built func; /* Function handler */
} builtin;

void load_module();

#endif