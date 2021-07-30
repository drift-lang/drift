/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#include "token.h"

/* Analyze the types of strings */
token_kind to_keyword(const char *literal) {
    for (int i = 39; i < 51; i ++) {
        if (strcmp(literal, token_string[i]) == 0) {
            return i;
        }
    }
    return LITERAL;
}

/* Build token */
token *new_token(token_kind k, char *literal, int line, int off) {
    token *tok = (token *) malloc(sizeof(token));
    tok->kind = k;
    tok->literal = literal;
    tok->line = line;
    tok->off = off;
    return tok;
}