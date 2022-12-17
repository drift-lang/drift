/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#ifndef DRIFT_TRACE_H
#define DRIFT_TRACE_H

extern bool repl_mode;
extern bool trace;

#define TRACE(fmt, ...)                                                        \
  fprintf(stderr, fmt, __VA_ARGS__);                                           \
  trace = true;                                                                \
  if (!repl_mode) {                                                            \
    exit(EXIT_SUCCESS);                                                        \
  }

#endif