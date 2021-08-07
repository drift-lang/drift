/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#ifndef FT_VM_H
#define FT_VM_H

#include <stdio.h>
#include <dirent.h>

#include "list.h"
#include "code.h"
#include "opcode.h"
#include "table.h"

#include "lexer.h"
#include "compiler.h"

#if defined(__linux__) || defined(__APPLE__)
    #include <unistd.h>
#elif defined(_WIN32)
    #include <windows.h>
    #include <dirent.h>
#endif

/* Evaluate frame structure */
typedef struct {
    code_object *code; /* Code object list */
    table *tb; /* Object mapping table */
    list *data; /* Eval data stack */
    object *ret; /* Return value of frame */
} frame;

/* Virtual machine state */
typedef struct {
    list *frame; /* Frame list */
    int16_t op; /* Position of offset */
    int16_t ip; /* IP */
    bool loop_ret; /* Break loop */
    object *whole; /* Method of load whole */
    char *filename; /* Current evaluate filename */
} vm_state;

/* Evaluate code object */
vm_state evaluate(code_object *, char *);

/* Built in function prototype */
typedef void (* built)(frame *, list *);

/* Builtin function */
typedef struct {
    char *name; /* Function name */
    built func; /* Function handler */
} builtin;

/* Returns the file name of path string */
char *get_filename(const char *p);

/* Release frame struct */
void free_frame(frame *f);

/*
 * External API
 */
typedef void (* fn_impl)(vm_state *);

typedef struct {
    const char *name;
    fn_impl fn;
} reg;

void reg_fn(reg *);

#endif