/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#ifndef FT_LEXER_H
#define FT_LEXER_H

#include <stdbool.h>
#include <stdio.h>

#include "list.h"
#include "token.h"

/* Lexical analysis */
list *lexer(const char *, int);

#endif