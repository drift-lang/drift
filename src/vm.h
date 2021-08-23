/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#ifndef FT_VM_H
#define FT_VM_H

#include <dirent.h>
#include <stdio.h>

#include "code.h"
#include "keg.h"
#include "opcode.h"
#include "table.h"
#include "token.h"

#if defined(__linux__) || defined(__APPLE__)
    #include <unistd.h>
#elif defined(_WIN32)
    #include <dirent.h>
    #include <windows.h>
#endif

#define STRING_EVAL_MAX 128

typedef struct {
    code_object *code;
    table *tb;
    keg *data;
    object *ret;
} frame;

typedef struct {
    keg *frame;
    int16_t op;
    int16_t ip;
    bool loop_ret;
    object *class;
    char *filename;
} vm_state;

vm_state evaluate(code_object *, char *);

typedef void (*built)(frame *, frame *);

typedef struct {
    char *name;
    built func;
} builtin;

char *get_filename(const char *p);

void free_frame(frame *f);

void free_tokens(keg *);

typedef void (*fn_impl)(vm_state *);

typedef struct {
    const char *name;
    fn_impl fn;
} reg;

void reg_module(reg *);

#endif