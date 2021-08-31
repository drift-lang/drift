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
#define STRING_PATH_MAX 64
#define BUILTIN_COUNT   13

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

char *get_filename(const char *p);

void free_frame(frame *f);

void free_tokens(keg *);

#endif