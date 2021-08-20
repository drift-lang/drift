/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#ifndef FT_TOKEN_H
#define FT_TOKEN_H

#include <stdlib.h>
#include <string.h>

typedef enum {
        EOH,
        LITERAL,
        NUMBER,
        STRING,
        CHAR,
        FLOAT,
        ADD,
        SUB,
        MUL,
        DIV,
        SUR,
        AS_ADD,
        AS_SUB,
        AS_MUL,
        AS_DIV,
        AS_SUR,
        R_ARROW,
        DOT,
        COMMA,
        COLON,
        EQ,
        SEMICOLON,
        GREATER,
        LESS,
        GR_EQ,
        LE_EQ,
        ADDR,
        OR,
        BANG,
        BANG_EQ,
        EQ_EQ,
        L_BRACE,
        R_BRACE,
        L_PAREN,
        R_PAREN,
        L_BRACKET,
        R_BRACKET,
        UNDERLINE,
        SLASH,
        DEF,
        RET,
        FOR,
        AOP,
        IF,
        EF,
        NF,
        NEW,
        OUT,
        GO,
        USE,
        NIL
} token_kind;

static const char *token_string[] = {
    "EOF", "LITERAL", "NUMBER", "STRING", "CHAR", "FLOAT", "+",   "-",  "*",
    "/",   "%",       "+=",     "-=",     "*=",   "/=",    "%=",  "->", ".",
    ",",   ":",       "=",      ";",      ">",    "<",     ">=",  "<=", "&",
    "|",   "!",       "!=",     "==",     "{",    "}",     "(",   ")",  "[",
    "]",   "_",       "\\",     "def",    "ret",  "for",   "aop", "if", "ef",
    "nf",  "new",     "out",    "go",     "use",  "nil",
};

typedef struct {
        token_kind kind;
        char *literal;
        int line;
        int off;
} token;

#endif