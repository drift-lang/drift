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

/* Evaluate frame structure */
typedef struct {
  code_object *code; /* Code object keg */
  table *tb;         /* Object mapping table */
  keg *data;         /* Eval data stack */
  object *ret;       /* Return value of frame */
} frame;

/* Virtual machine state */
typedef struct {
  keg *frame;     /* Frame keg */
  int16_t op;     /* Position of offset */
  int16_t ip;     /* IP */
  bool loop_ret;  /* Break loop */
  object *whole;  /* Method of load whole */
  char *filename; /* Current evaluate filename */
} vm_state;

/* Evaluate code object */
vm_state evaluate(code_object *, char *);

/* Built in function prototype */
typedef void (*built)(frame *, keg *);

/* Builtin function */
typedef struct {
  char *name; /* Function name */
  built func; /* Function handler */
} builtin;

/* Returns the file name of path string */
char *get_filename(const char *p);

/* Free frame struct */
void free_frame(frame *f);

/* Free lexical token list */
void free_tokens(keg *);

/* External api */
typedef void (*fn_impl)(vm_state *);

/* API Structure */
typedef struct {
  const char *name;
  fn_impl fn; /* vm_state <-> module */
} reg;

/* Regist module */
void reg_module(reg *);

#endif