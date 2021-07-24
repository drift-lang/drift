/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* My list structure */
typedef struct { void **data; int len; int cap; } list;

/* Create an empty list */
list *new_list() {
    list *l = (list *) malloc(sizeof(list));
    l->data = NULL;
    l->len = 0;
    l->cap = 0;
    return l;
}

/* The tail is added,
 * and the capacity is expanded twice automatically */
list *append_list(list *l, void *ptr) {
    if (l == NULL) l = new_list(); /* New */
    if (l->cap == 0 || l->len + 1 > l->cap) {
        l->cap = 
            l->cap == 0 ? 4 : l->cap * 2;
        l->data = realloc(l->data, sizeof(void *) * l->cap); /* Double capacity */
    }
    l->data[l->len ++] = ptr;
    return l;
}

/* Tail data */
void *list_back(list *l) { return l->data[l->len - 1]; }

/* Pop up tail data */
void *pop_list_back(list *l) { return l->data[-- l->len]; }

/* Release the list elements and themselves */
void free_list(list *l) {
    for (int i = 0; i < l->len; i ++) free(l->data[i]);
    free(l->data);
    free(l);
}

/* Types of all tokens */
typedef enum {
    EOH,	   LITERAL,   NUMBER,    STRING,  CHAR,      FLOAT,
    ADD,       SUB,       MUL,       DIV,     SUR,       AS_ADD,
    AS_SUB,    AS_MUL,    AS_DIV,    AS_SUR,  R_ARROW,   L_ARROW,
    DOT,	   COMMA,     COLON,     EQ,      SEMICOLON, GREATER,
    LESS,      GR_EQ,     LE_EQ,     ADDR,	  OR,        BANG,
    BANG_EQ,   EQ_EQ,     L_BRACE,   R_BRACE, L_PAREN,   R_PAREN,
    L_BRACKET, R_BRACKET, UNDERLINE, DEF,     RET, 	     FOR,
    AOP,       IF,        EF,        NF,      NEW, 	     OUT,
    GO, 	   MOD,       USE
} token_kind;

/* Keywords */
const char *keyword[12] = {
    "def", "ret", "for", "aop", "if",  "ef",
    "nf",  "new", "out", "go",  "mod", "use"
};

/* Token properties */
typedef struct { token_kind kind; const char *literal; int line; int off; } token;

/* Whether the symbol is a space, carriage return, line feed, indent */
bool is_space(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

/* Are the symbols numbers 0 to 9 */
bool is_digit(char c) { return c >= '0' && c <= '9'; }

/* Is the symbol a legal identifier */
bool is_ident(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/* Analyze the types of strings */
token_kind to_keyword(const char *literal) {
    for (int i = 0; i < 12; i ++) {
        if (strcmp(literal, keyword[i]) == 0) {
            if (i ==  0) return DEF; if (i ==  1) return RET;
            if (i ==  2) return FOR; if (i ==  3) return AOP;
            if (i ==  4) return IF;  if (i ==  5) return EF;
            if (i ==  6) return NF;  if (i ==  7) return NEW;
            if (i ==  8) return OUT; if (i ==  9) return GO;
            if (i == 10) return MOD; if (i == 11) return USE;
        }
    }
    return LITERAL;
}

/* Build token */
token *new_token(token_kind k, const char *literal, int line, int off) {
    token *tok = (token *) malloc(sizeof(token));
    tok->kind = k;
    tok->literal = literal;
    tok->line = line;
    tok->off = off;
    return tok;
}

/* Token list */
list *tokens = NULL;

/* Push token into token list */
void push_token_list(token *tok) { tokens = append_list(tokens, tok); }

/* Determine whether the next character is the specified type */
bool next(const char *buf, int *p, char c) {
    if (*p + 1 > strlen(buf)) return false;
    if (buf[*p + 1] == c) {
        *p += 2;
        return true;
    }
    (*p) ++;
    return false;
}

/* Handles the literal amount of multiple characters */
void paste_literal(char *literal, const char *buf, int *p, int i) {
    for (int x = 0, j = *p; x < j; x ++) {
        literal[x] = buf[i - *p];
        (*p) --;
        if (x + 1 == j) {
            literal[x + 1] = '\0';
        }
    }
}

/* Lexical analysis */
void lexer(const char *buf, int fsize) {
    bool new_line = true; /* Indent judgement */
    for (int i = 0, line = 1, off = 0; i < fsize;) {
        char c = buf[i];
        while (is_space(c)) { /* Space */
            if (c == '\n') {
                line ++;
                off = -1;
                new_line = true;
            }
            c = buf[++ i];
            /* If it's a new line,
             * And it's dealing with spaces, increase indent */
            if (new_line) off ++;
        }
        if (new_line) new_line = false; /* To catch indent */
        if (is_digit(c)) { /* Digit */
            int p = 0;
            bool f = false;
            while (is_digit(c) || c == '.') {
                if (c == '.') f = true;
                p ++;
                c = buf[++ i];
            }
            char *literal = (char *) malloc(p * sizeof(char));
            paste_literal(literal, buf, &p, i);
            /* Append */
            push_token_list(new_token(f ? FLOAT : NUMBER, literal, line, off));
            continue;
        }
        if (is_ident(c)) { /* Ident */
            int p = 0;
            while (is_ident(c)) {
                p ++;
                c = buf[++ i];
            }
            char *literal = (char *) malloc((p + 1) * sizeof(char));
            paste_literal(literal, buf, &p, i);
            /* Append */
            push_token_list(new_token(
                to_keyword(literal), literal, line, off));
            continue;
        }
        token t = {.line = line, .off = off}; /* Others */
        switch (c) {
            case '+':
                if (next(buf, &i, '=')) {t.kind = AS_ADD; t.literal = "+=";}
                else {t.kind = ADD; t.literal = "+";}
                break;
            case '-':
                if (next(buf, &i, '=')) {t.kind = AS_SUB; t.literal = "-=";}
                else {
                    if (buf[i] == '>') {
                        t.kind = R_ARROW; t.literal = "->";
                        i ++;
                    } else {t.kind = SUB; t.literal = "-";}
                }
                break;
            case '*':
                if (next(buf, &i, '=')) {t.kind = AS_MUL; t.literal = "*=";}
                else {t.kind = MUL; t.literal = "*";}
                break;
            case '/':
                if (next(buf, &i, '=')) {t.kind = AS_DIV; t.literal = "/=";}
                else {t.kind = DIV; t.literal = "/";}
                break;
            case '%':
                if (next(buf, &i, '=')) {t.kind = AS_SUR; t.literal = "%=";}
                else {t.kind = SUR; t.literal = "%";}
                break;
            case '<':
                if (next(buf, &i, '=')) {t.kind = LE_EQ; t.literal = "<=";}
                else {
                    if (buf[i] == '-') {
                        t.kind = L_ARROW; t.literal = "<-";
                        i ++;
                    } else {t.kind = LESS; t.literal = "<";}
                }
                break;
            case '>':
                if (next(buf, &i, '=')) {t.kind = GR_EQ; t.literal = ">=";}
                else {t.kind = GREATER; t.literal = ">";}
                break;
            case ' ':
            case '\t':
            case '\r':
            case '\n':
            case '\0':
                continue;
            case '#':
                while (buf[++ i] != '\n');
                continue;
            case '\'': { /* Char */
                i ++;
                char *literal = (char *) malloc(sizeof(char));
                literal[0] = buf[i ++];
                c = buf[i];
                if (c != '\'') {
                    fprintf(stderr,
                        "<lexer %d>: missing single quotation mark to the right.\n",
                        line);
                    exit(EXIT_FAILURE);
                } else {
                    i += 2;
                }
                t.kind = CHAR; t.literal = literal;
            }
                break;
            case '"': { /* String */
                c = buf[++ i];
                int p = 0;
                while (c != '"') {
                    c = buf[++ i];
                    p ++;
                    if (i == fsize) {
                        fprintf(stderr,
                            "<lexer %d>: missing closing double quote.\n", line);
                        exit(EXIT_FAILURE);
                    }
                }
                char *literal = (char *) malloc((p + 1) * sizeof(char));
                paste_literal(literal, buf, &p, i);
                i ++;
                t.kind = STRING; t.literal = literal;
            }
                break;
            case '.':
                i ++;
                t.kind = DOT; t.literal = ".";
                break;
            case ',':
                i ++;
                t.kind = COMMA; t.literal = ",";
                break;
            case ':':
                i ++;
                t.kind = COLON; t.literal = ":";
                break;
            case '=':
                if (next(buf, &i, '=')) {t.kind = EQ_EQ; t.literal = "==";}
                else {t.kind = EQ; t.literal = "=";}
                break;
            case ';':
                i ++;
                t.kind = SEMICOLON; t.literal = ";";
                break;
            case '&':
                i ++;
                t.kind = ADDR; t.literal = "&";
                break;
            case '|':
                i ++;
                t.kind = OR; t.literal = "|";
                break;
            case '!':
                if (next(buf, &i, '=')) {t.kind = BANG_EQ; t.literal = "!=";}
                else {t.kind = BANG; t.literal = "!";}
                break;
            case '{':
                i ++;
                t.kind = L_BRACE; t.literal = "{";
                break;
            case '}':
                i ++;
                t.kind = R_BRACE; t.literal = "}";
                break;
            case '[':
                i ++;
                t.kind = L_BRACKET; t.literal = "[";
                break;
            case ']':
                i ++;
                t.kind = R_BRACKET; t.literal = "]";
                break;
            case '(':
                i ++;
                t.kind = L_PAREN; t.literal = "(";
                break;
            case ')':
                i ++;
                t.kind = R_PAREN; t.literal = ")";
                break;
            default:
                fprintf(stderr, "<lexer %d>: unknown character '%c'.\n",
                    line, c);
                exit(EXIT_FAILURE);
        }
        /* Other */
        push_token_list(new_token(t.kind, t.literal, t.line, t.off));
    }
    /* Terminator */
    token *end = list_back(tokens);
    token *eoh = new_token(EOH, "EOH", end->line + 1, 0);
    push_token_list(eoh); /* End of handler */
}

/* 
 * Frame:
 *
 *   1. Bytecodes    byte
 *   2. Constants    object
 *   3. Names        string
 *   4. Offsets      int
 *   5. Types        type
 *
 * All bytecodes
 * 
 *   J: Object   N: Name    S: Const    F: Offset
 *   T: Type     P: Top     *: None
 */
typedef enum {
    LOAD_CONST,    // S
    LOAD_OF,       // N
    LOAD_ENUM,     // S
    LOAD_WHOLE,    // S
    LOAD_FUNC,     // S
    ASSIGN_TO,     // N
    STORE_NAME,    // N T
    INDEX_TO,      // *
    REPLACE_TO,    // *
    GET_OF,        // N
    SET_OF,        // N
    CALL_FUNC,     // F
    SET_NAME,      // N
    NEW_OBJ,       // N F
    SET_MODULE,    // N
    USE_MOD,       // N
    BUILD_ARR,     // F
    BUILD_TUP,     // F
    BUILD_MAP,     // F
    TO_ADD,        // P1 + P2
    TO_SUB,        // P1 - P2
    TO_MUL,        // P1 * P2
    TO_DIV,        // P1 / P2
    TO_SUR,        // P1 % P2
    TO_GR,         // P1 > P2
    TO_LE,         // P1 < P2
    TO_GR_EQ,      // P1 >= P2
    TO_LE_EQ,      // P1 <= P2
    TO_EQ_EQ,      // P1 == P2
    TO_NOT_EQ,     // P1 != P2
    TO_AND,        // P1 & P2
    TO_OR,         // P1 | P2
    TO_BANG,       // !P
    TO_NOT,        // -P
    JUMP_TO,       // F
    TRUE_JUMP_TO,  // F
    FALSE_JUMP_TO, // F
    RET_NONE,      // *
    RET_VAL,       // P
} op_code;

/* Type system */
typedef enum {
    T_INT,    // int
    T_FLOAT,  // float
    T_CHAR,   // char
    T_STRING, // string
    T_BOOL,   // bool
    T_ARRAY,  // []T
    T_TUPLE,  // ()T
    T_MAP,    // {}<T1, T2>
    T_FUNC,   // |[]T| -> T
    T_USER,   // ?
} type_kind;

/*
 * Type:
 *
 *   1. int       2.  float
 *   3. char      4.  string
 *   5. bool      6.  array
 *   7. tuple     8.  map
 *   9. function  10. user
 */
typedef struct {
    u_int8_t kind; /* Type system */
    union {
        struct type *single; /* Contains a single type */
        struct type *T1, *T2; /* It contains two types */
        const char *name; /* Customer type */
        struct {
            struct type *arg; /* Function arguments */
            struct type *ret; /* Function returns */
        } func;
    } inner; 
} type;

/* Constant system */
typedef enum { C_INT, C_FLOAT, C_STRING, C_CHAR, C_BOOL } const_kind;

/*
 * Constant:
 * 
 *   1. int    2. float
 *   3. string 4. char
 *   5. bool
 */
typedef struct {
    u_int8_t kind; /* Constant system */
    union {
        int integer; /* int */
        double floating; /* float */
        const char *str; /* string */
        char ch; /* char */
        bool boolean; /* bool */
    } value;
} constant;

/* Generate object, subject data type. */
typedef struct {
    const char *description; /* Description name */
    list *names; /* Name set */
    list *codes; /* Bytecode set */
    list *offsets; /* Offset set */
    list *types; /* Type set */
    list *constants; /* Constant set */
} code_object;

/* Compilation status*/
typedef struct {
    token pre; /* last token */
    token cur; /* current token */
} compile_state;

/* Global state of compiler */
compile_state state;

/* Compiled object list */
list *objs = NULL;

/* Iterator of lexical list */
void iter() {
    static int p = 0;
    state.pre = state.cur;
    if (p == tokens->len) {
        return;
    }
    state.cur = *(token *) tokens->data[p++];
    // printf("%s %s\n", state.pre.literal, state.cur.literal);
}

/* Operation expression priority
   Note: priority is next level processing */
typedef enum {
    P_LOWEST,  // *
/*     P_ASSIGN,  // =
    P_OR,      // |
    P_AND,     // &
    P_EQ,      // == !=
    P_COMPARE, // > >= < <=
    P_TERM,    // + -
    P_FACTOR,  // * / %
    P_UNARY,   // ! -
    P_CALL,    // . () */
    P_TERM,    // + -
    P_FACTOR,  // * / %
    P_UNARY,   // ! -
    P_CALL,    // . ()
} precedence;

typedef void (* function)(); /* Prefix and infix handing functions */

/* Rule */
typedef struct {
    token_kind kind; /* Token kind */
    function prefix; /* Prefix handing */
    function infix; /* Infix handing */
    int precedence; /* Priority of this token */
} rule;

/* Placeholder */
rule get_rule(token_kind kind);
void set_precedence(int prec);

int get_pre_prec() { return get_rule(state.pre.kind).precedence; } /* Pre */
int get_cur_prec() { return get_rule(state.cur.kind).precedence; } /* Cur */

/* NUMBER */
void number() {
    printf("NUMBER %s\n", state.pre.literal);
}

/* UNARY: ! - */
void unary() {
    iter();
    set_precedence(P_UNARY);
    printf("UNARY\n");
}

/* BINARY: + - * / % */
void binary() {
    token_kind operator = state.pre.kind;

    iter(); /* In order to resolve the next expression */
    /* Start again, parse from prefix.
       1. When the future expression is higher than the current one,
          the next infix is processed.
       2. A higher level starts after me for priority. */
    set_precedence(
        get_pre_prec() + 1);

    switch (operator) {
        case ADD: printf("OP_ADD\n"); break;
        case SUB: printf("OP_SUB\n"); break;
        case MUL: printf("OP_MUL\n"); break;
        case DIV: printf("OP_DIV\n"); break;
    }
}

/* GROUP: (E) */
void group() {
    iter();
    set_precedence(P_LOWEST);
    iter();
    if (state.pre.kind != R_PAREN) {
        fprintf(stderr, 
            "<compiler %d>: lost right paren symbol.\n", state.pre.line);
        exit(EXIT_FAILURE);
    }
}

/* Rules */
rule rules[] = {
    { EOH,     NULL,   NULL,   P_LOWEST },
    { NUMBER,  number, NULL,   P_LOWEST },
    { ADD,     NULL,   binary, P_TERM   },
    { SUB,     unary,  binary, P_TERM   },
    { MUL,     NULL,   binary, P_FACTOR },
    { DIV,     NULL,   binary, P_FACTOR },
    { SUR,     NULL,   binary, P_FACTOR },
    { L_PAREN, group,  NULL,   P_CALL   },
};

/* Search by dictionary type */
rule get_rule(token_kind kind) {
    for (int i = 1;
            i < sizeof(rules) / sizeof(rules[0]); i ++) {
        if (rules[i].kind == kind) {
            return rules[i];
        }
    } /* Others return to the lowest level */
    return rules[0];
}

/* Principal algorithm */
void set_precedence(int precedence) {
    rule prefix = get_rule(state.pre.kind); /* Get prefix */
    if (prefix.prefix == NULL) {
        fprintf(stderr,
            "<compiler %d>: not found prefix function of token '%s'.\n",
            state.pre.line,
            state.pre.literal
        );
        exit(EXIT_FAILURE);
    }
    prefix.prefix(); /* Process prefix */
    while (precedence < get_cur_prec()) { /* Determine future priorities */
        rule infix = get_rule(state.cur.kind); /* Get infix */
        iter();
        infix.infix(); /* Process infix */
    }
}

/* Compiler */
void compile() {
    iter(); // x 1
    iter(); // 1 2

    /* Push global object */
    code_object *obj = (code_object *) malloc(sizeof(code_object));
    obj->description = "main";
    objs = append_list(objs, obj);

    set_precedence(P_LOWEST);
}

/* ? */
int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: ./drift (file)\n");
        exit(EXIT_FAILURE);
    }
    const char *path = argv[1];
    FILE *fp = fopen(path, "r"); /* Open file of path */
    if (fp == NULL) {
        printf("<compiler>: failed to read buffer of file: %s.\n", path);
        exit(EXIT_FAILURE);
    }

    fseek(fp, 0, SEEK_END);
    int fsize = ftell(fp); /* Returns the size of file */
    fseek(fp, 0, SEEK_SET);
    char *buf = (char *) malloc(fsize * sizeof(char));

    fread(buf, fsize, sizeof(char), fp); /* Read file to buffer*/
    buf[fsize] = '\0';

    /* Lexical analysis */
    lexer(buf, fsize);
/*     for (int i = 0; i < tokens->len; i ++) {
        token *t = tokens->data[i];
        printf("[%3d]\t%-5d %-20s %-20d %d\n", 
            i, t->kind, t->literal, t->line, t->off);
    } */

    /* Compiler */
    compile();
    
    fclose(fp); /* Close file */
    free(buf); /* Close buffer */
    free_list(tokens); /* Release tokens */
    return 0;
}
