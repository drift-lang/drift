/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#ifndef FT_CODE_H
#define FT_CODE_H

#include "keg.h"

typedef struct {
  char *description;
  keg *names;
  keg *codes;
  keg *offsets;
  keg *types;
  keg *objects;
  keg *lines;
} code_object;

#endif