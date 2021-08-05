/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#ifndef FT_TOKEN_H
#define FT_TOKEN_H

#include <string.h>
#include <stdlib.h>

/* Types of all tokens */
typedef enum {
    EOH,       LITERAL,   NUMBER,  STRING,    CHAR,      FLOAT,
    ADD,       SUB,       MUL,     DIV,       SUR,       AS_ADD,
    AS_SUB,    AS_MUL,    AS_DIV,  AS_SUR,    R_ARROW,   DOT,
    COMMA,     COLON,     EQ,      SEMICOLON, GREATER,   LESS,
    GR_EQ,     LE_EQ,     ADDR,    OR,        BANG,      BANG_EQ,
    EQ_EQ,     L_BRACE,   R_BRACE, L_PAREN,   R_PAREN,   L_BRACKET,
    R_BRACKET, UNDERLINE, SLASH,   DEF,       RET,       FOR,
    AOP,       IF,        EF,      NF,        NEW,       OUT,
    GO,        USE
} token_kind;

/* Token string */
static const char *token_string[] = {
    "EOF", "LITERAL", "NUMBER", "STRING", "CHAR", "FLOAT",
    "+",   "-",       "*",      "/",      "%",    "+=",
    "-=",  "*=",      "/=",     "%=",     "->",   ".",
    ",",   ":",       "=",      ";",      ">",    "<",
    ">=",  "<=",      "&",      "|",      "!",    "!=",
    "==",  "{",       "}",      "(",      ")",    "[",
    "]",   "_",       "\\",     "def",    "ret",  "for",
    "aop", "if",      "ef",     "nf",     "new",  "out",
    "go",  "use"
};

/* Token properties */
typedef struct {
    token_kind kind;
    char *literal;
    int line;
    int off;
} token;

#endif