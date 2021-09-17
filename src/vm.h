/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#ifndef FT_VM_H
#define FT_VM_H

#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>

#include "code.h"
#include "keg.h"
#include "opcode.h"
#include "table.h"
#include "token.h"

#if defined(__linux__) || defined(__APPLE__)
    #include <unistd.h>
#elif defined(_WIN32)
    #include <windows.h>
#endif

#define STRING_EVAL_MAX 1024
#define STRING_PATH_MAX 64
#define BUILTIN_COUNT   13

#define C_MOD_MEMCOUNT  32

extern bool repl_mode;

typedef struct {
    code_object *code;
    table *tb;
    keg *data;
    object *ret;
    table *tp;
    keg *range;
} frame;

typedef struct {
    keg *frame;
    int16_t op;
    int16_t ip;
    bool loop_ret;
    char *filename;
    keg *call;
} vm_state;

vm_state evaluate(code_object *, char *);

typedef struct {
    char *name;
    builtin_kind kind;
    void *func;
} builtin;

enum mem_kind { C_VAR, C_METHOD };

typedef struct {
    char *name;
    enum mem_kind kind;
} reg_mem;

typedef struct {
    char *name;
    reg_mem member[C_MOD_MEMCOUNT];
    int i;
} reg_mod;

typedef struct {
    char *name;
    int p;
    keg *arr;
} range_iter;

reg_mod *new_mod(char *);
void emit_member(reg_mod *, char *, enum mem_kind);

void reg_c_func(const char *[]);
void reg_c_mod(const char *[]);
void reg_name(char *, object *);

void push_stack(object *);

int check_num(keg *, int);
double check_float(keg *, int);
char *check_str(keg *, int);
char check_char(keg *, int);
bool check_bool(keg *, int);
void *check_userdata(keg *, int);
object *check_front(keg *);
void check_empty(keg *);

void throw_error(const char *);

char *get_filename(const char *p);

void free_frame(frame *f);

void free_tokens(keg *);

#endif