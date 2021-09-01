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

#define STRING_EVAL_MAX 128
#define STRING_PATH_MAX 64
#define BUILTIN_COUNT   13

#define C_MOD_MEMCOUNT  64

static void *pp = NULL;

typedef struct {
  code_object *code;
  table *tb;
  keg *data;
  object *ret;
  table *tp;
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

typedef struct {
  char *name;
  void (*ptr)(keg *);
} reg_fn;

enum mem_kind { C_VAR, C_METHOD, C_CLASS };

typedef struct {
  char *name;
  enum mem_kind kind;
  void *ptr;
} reg_mem;

typedef struct {
  char *name;
  reg_mem member[C_MOD_MEMCOUNT];
} reg_multiple;

void reg_c_func(const char *[]);
void reg_c_class(const void *[]);
void reg_c_mod(const void *[]);

void push_num();
void push_float();
void push_string();
void push_char();
void push_bool();

void set_var();
void set_mod();

object *new_num(int);
object *new_float(double);
object *new_string(char *);
object *new_char(char);
object *new_bool(bool);

char *get_filename(const char *p);

void free_frame(frame *f);

void free_tokens(keg *);

#endif