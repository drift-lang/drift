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
void *list_back(list *l) {
    if (l->len == 0) return NULL;
    return l->data[l->len - 1];
}

/* Pop up tail data */
void *pop_list_back(list *l) { return l->data[-- l->len]; }

/* Insert element at specified location */
void insert_list(list *l, int p, void *ptr) {
    if (p < 0) return;
    if (p > l->len - 1) p = l->len - 1;
    l = append_list(l, ptr);
    for (int i = p, j = 1, m = 2; i < l->len - 1; i ++) {
        l->data[l->len - j] = l->data[l->len - m];
        j ++;
        m ++;
    }
    l->data[p] = ptr;
}

/* Release the list elements and themselves */
void free_list(list *l) {
    for (int i = 0; i < l->len; i ++) free(l->data[i]);
    free(l->data);
    free(l);
}

/* Types of all tokens */
typedef enum {
    EOH,       LITERAL,   NUMBER,    STRING,  CHAR,      FLOAT,
    ADD,       SUB,       MUL,       DIV,     SUR,       AS_ADD,
    AS_SUB,    AS_MUL,    AS_DIV,    AS_SUR,  R_ARROW,   L_ARROW,
    DOT,       COMMA,     COLON,     EQ,      SEMICOLON, GREATER,
    LESS,      GR_EQ,     LE_EQ,     ADDR,    OR,        BANG,
    BANG_EQ,   EQ_EQ,     L_BRACE,   R_BRACE, L_PAREN,   R_PAREN,
    L_BRACKET, R_BRACKET, UNDERLINE, DEF,     RET,       FOR,
    AOP,       IF,        EF,        NF,      NEW,       OUT,
    GO,        MOD,       USE
} token_kind;

/* Token string */
const char *token_string[] = {
    "EOH", "LITERAL", "NUMBER", "STRING", "CHAR", "FLOAT",
    "+",   "-",       "*",      "/",      "%",    "+=",
    "-=",  "*=",      "/=",     "%=",     "->",   "<-",
    ".",   ",",       ":",      "=",      ";",    ">",
    "<",   ">=",      "<=",     "&",      "|",    "!",
    "!=",  "==",      "{",      "}",      "(",    ")",
    "[",   "]",       "_",      "def",    "ret",  "for",
    "aop", "if",      "ef",     "nf",     "new",  "out",
    "go",  "mod",     "use"
};

/* Token properties */
typedef struct { token_kind kind; char *literal; int line; int off; } token;

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
            if (i + 1 >= fsize) {
                ++ i;
                break;
            }
            c = buf[++ i];
            /* If it's a new line,
             * And it's dealing with spaces, increase indent */
            if (new_line) off ++;
        }
        if (i >= fsize) break;
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
    if (tokens != NULL) {
        token *end = list_back(tokens);
        token *eoh = new_token(EOH, "EOH", end->line + 1, 0);
        push_token_list(eoh); /* End of handler */
    } else {
        push_token_list(new_token(EOH, "EOH", 0, 0)); /* No token */
    }
}

/* 
 * Frame:
 *
 *   1. Bytecodes    byte
 *   2. Objects      object
 *   3. Names        string
 *   4. Offsets      int
 *   5. Types        type
 *
 * All bytecodes
 * 
 *   J: Object   N: Name    F: Offset
 *   T: Type     P: Top     *: None
 */
typedef enum {
    CONST_OF,      // J
    LOAD_OF,       // N
    LOAD_ENUM,     // J
    LOAD_WHOLE,    // J
    LOAD_FUNC,     // J
    ASSIGN_TO,     // N
    STORE_NAME,    // N T
    TO_INDEX,      // *
    TO_REPLACE,    // *
    GET_OF,        // N
    SET_OF,        // N
    CALL_FUNC,     // F
    ASS_ADD,       // N
    ASS_SUB,       // N
    ASS_MUL,       // N
    ASS_DIV,       // N
    ASS_SUR,       // N
    TO_REP_ADD,    // *
    TO_REP_SUB,    // *
    TO_REP_MUL,    // *
    TO_REP_DIV,    // *
    TO_REP_SUR,    // *
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
    T_JUMP_TO,     // F
    F_JUMP_TO,     // F
    TO_RET,        // *
    RET_OF,        // P
} op_code;

/* Output bytecode */
const char *code_string[] = {
    "CONST_OF",     "LOAD_OF",       "LOAD_ENUM",  "LOAD_WHOLE", "LOAD_FUNC",
    "ASSIGN_TO",    "STORE_NAME",    "TO_INDEX",   "TO_REPLACE", "GET_OF",
    "SET_OF",       "CALL_FUNC",     "ASS_ADD",    "ASS_SUB",    "ASS_MUL",
    "ASS_DIV",      "ASS_SUR",       "TO_REP_ADD", "TO_REP_SUB", "TO_REP_MUL", 
    "TO_REP_DIV",   "TO_REP_SUR",    "SET_NAME",   "NEW_OBJ",    "SET_MODULE",
    "USE_MOD",      "BUILD_ARR",     "BUILD_TUP",  "BUILD_MAP",  "TO_ADD",
    "TO_SUB",       "TO_MUL",        "TO_DIV",     "TO_SUR",     "TO_GR",
    "TO_LE",        "TO_GR_EQ",      "TO_LE_EQ",   "TO_EQ_EQ",   "TO_NOT_EQ",
    "TO_AND",       "TO_OR",         "TO_BANG",    "TO_NOT",     "JUMP_TO",
    "T_JUMP_TO",    "F_JUMP_TO",     "TO_RET",     "RET_OF",
};

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
        const char *name; /* Customer type */
        struct {
            list *arg; /* Function arguments */
            struct type *ret; /* Function returns */
        } func;
        struct {
            struct type *T1;
            struct type *T2;
        } both; /* It contains two types */
    } inner; 
} type;

/* Output type */
const char *type_string(type *t) {
    char *str = (char *) malloc(sizeof(char) * 128);
    switch (t->kind) {
        case T_INT:    free(str); return "<int>";
        case T_FLOAT:  free(str); return "<float>";
        case T_CHAR:   free(str); return "<char>";
        case T_STRING: free(str); return "<string>";
        case T_BOOL:   free(str); return "<bool>";
        case T_ARRAY:
            sprintf(str, "[%s]", type_string((type *)t->inner.single));
            return str;
        case T_TUPLE:
            sprintf(str, "(%s)", type_string((type *)t->inner.single));
            return str;
        case T_MAP:
            sprintf(str, "{%s : %s}",
                type_string((type *)t->inner.both.T1), 
                type_string((type *)t->inner.both.T2));
            return str;
        case T_FUNC:
            if (t->inner.func.arg == NULL &&
                t->inner.func.ret == NULL) {
                    free(str);
                    return "<|| -> None>";
                }
            if (t->inner.func.arg != NULL) {
                sprintf(str, "<|");
                for (int i = 0; i < t->inner.func.arg->len; i ++) {
                    strcat(str, 
                        type_string((type *) t->inner.func.arg->data[i]));
                    if (i + 1 != t->inner.func.arg->len) {
                        strcat(str, ", ");
                    }
                }
                strcat(str, "|>");
                if (t->inner.func.ret != NULL) {
                    strcat(str, type_string((type *)t->inner.func.ret));
                }
                return str;
            }
            if (t->inner.func.arg == NULL &&
                t->inner.func.ret != NULL) {
                    sprintf(str, "<|| -> %s>", 
                        type_string((type *)t->inner.func.ret));
                    return str;
                }
        case T_USER:
            sprintf(str, "<%s>", t->inner.name);
            return str;
    }
}

/* Generate object, subject data type. */
typedef struct {
    const char *description; /* Description name */
    list *names; /* Name set */
    list *codes; /* Bytecode set */
    list *offsets; /* Offset set */
    list *types; /* Type set */
    list *objects; /* Objects set */
} code_object;

/* Object type */
typedef enum {
    OBJ_INT,  OBJ_FLOAT, OBJ_STRING, OBJ_CHAR,
    OBJ_ENUM, OBJ_FUNC,  OBJ_WHOLE
} obj_kind;

/* Object system */
typedef struct {
    u_int8_t kind; /* Kind */
    union {
        int integer; /* int */
        double floating; /* float */
        char *string; /* string */
        char ch; /* char */
        struct {
            char *name;
            list *element;
        } enumeration; /* enum */
        struct {
            char *name;
            list *k;
            list *v;
            type *ret;
            code_object *code;
        } func; /* function */
        struct {
            char *name;
            list *inherit;
            code_object *code;
        } whole; /* whole */
    } value; /* Inner value */
} object;

/* Output object */
const char *obj_string(object *obj) {
    char *str = (char *) malloc(sizeof(char) * 128);
    switch (obj->kind) {
        case OBJ_INT:    sprintf(str, "int %d", obj->value.integer);       return str;
        case OBJ_FLOAT:  sprintf(str, "float %f", obj->value.floating);    return str;
        case OBJ_STRING: sprintf(str, "string \"%s\"", obj->value.string); return str;
        case OBJ_CHAR:   sprintf(str, "char '%c'", obj->value.ch);         return str;
        case OBJ_ENUM:
            sprintf(str, "enum \"%s\"", obj->value.enumeration.name);
            return str;
        case OBJ_FUNC:
            sprintf(str, "func \"%s\"", obj->value.func.name);
            return str;
        case OBJ_WHOLE:
            sprintf(str, "whole \"%s\"", obj->value.whole.name);
            return str;
    }
}

/* Compilation status*/
typedef struct {
    token pre; /* Last token */
    token cur; /* Current token */
    u_int8_t iof; /* Offset of object */
    u_int8_t inf; /* Offset of name */
    u_int8_t itf; /* Offset of type */
    int p; /* Read position */
} compile_state;

/* Global state of compiler */
compile_state state;

/* Compiled object list */
list *objs = NULL;

/* Push new object to list */
void push_obj_list(code_object *obj) { objs = append_list(objs, obj); }

/* Gets the current offset list length */
int *get_top_offset_p() {
    code_object *obj = (code_object *) list_back(objs);
    int *p = (int *) malloc(sizeof(int));
    *p = obj->offsets->len;
    return p;
}

/* Gets the top bytecode list length */
int get_top_code_len() {
    code_object *obj = (code_object *) list_back(objs);
    return obj->codes->len + 1; /* The offset used to insert the specified position */
}

/* Insert offset position new element */
void insert_top_offset(int p, u_int8_t off) {
    code_object *obj = (code_object *) list_back(objs);
    u_int8_t *f = (u_int8_t *) malloc(sizeof(u_int8_t));
    *f = off;
    insert_list(obj->offsets, p, f);
}

/* Offset */
void emit_top_offset(u_int8_t off) {
    code_object *obj = (code_object *) list_back(objs);
    u_int8_t *f = (u_int8_t *) malloc(sizeof(u_int8_t));
    *f = off;
    obj->offsets = append_list(obj->offsets, f);
}

/* Name */
void emit_top_name(char *name) {
    code_object *obj = (code_object *) list_back(objs);
    if (obj->names != NULL) {
        for (int i = 0; i < obj->names->len; i ++) {
            if ( /* Duplicate reference to existing name */
                strcmp((char *)obj->names->data[i], name) == 0) {
                    emit_top_offset(i);
                    return;
                }
        }
    }
    obj->names = append_list(obj->names, name);
    emit_top_offset(state.inf ++);
}

/* Type */
void emit_top_type(type *t) {
    code_object *obj = (code_object *) list_back(objs);
    obj->types = append_list(obj->types, t);
    emit_top_offset(state.itf ++);
}

/* Object */
void emit_top_obj(object *obj) {
    code_object *code = (code_object *) list_back(objs);
    code->objects = append_list(code->objects, obj);
    emit_top_offset(state.iof ++);
}

/* Code */
void emit_top_code(u_int8_t code) {
    code_object *obj = (code_object *) list_back(objs);
    op_code *c = (op_code *) malloc(sizeof(u_int8_t));
    *c = code;
    obj->codes = append_list(obj->codes, c);
}

/* Iterator of lexical list */
void iter() {
    static int p = 0;
    state.pre = state.cur;
    if (p == tokens->len)
        return;
    state.cur = *(token *) tokens->data[p++];
    state.p = p - 2; /* Always point to the current lexical subscript */
}

/* Operation expression priority
   Note: priority is next level processing */
typedef enum {
    P_LOWEST,  // *
    // P_ASSIGN,  // =
    P_OR,      // |
    P_AND,     // &
    P_EQ,      // == !=
    P_COMPARE, // > >= < <=
    P_TERM,    // + -
    P_FACTOR,  // * / %
    P_UNARY,   // ! -
    P_CALL,    // . () []
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

/* Find whether the future is an assignment operation */
bool ass_operator() {
    return state.cur.kind == AS_ADD || state.cur.kind == AS_SUB ||
        state.cur.kind == AS_MUL || state.cur.kind == AS_DIV ||
        state.cur.kind == AS_SUR;
}

/* Bytecode returned by assignment operation */
u_int8_t get_ass_code(token_kind kind) {
    if (kind == AS_ADD) return ASS_ADD; if (kind == AS_SUB) return ASS_SUB;
    if (kind == AS_MUL) return ASS_MUL; if (kind == AS_DIV) return ASS_DIV;
    return ASS_SUR;
}

/* Array assignment bytecode */
u_int8_t get_rep_ass_code(token_kind kind) {
    if (kind == AS_ADD) return TO_REP_ADD; if (kind == AS_SUB) return TO_REP_SUB;
    if (kind == AS_MUL) return TO_REP_MUL; if (kind == AS_DIV) return TO_REP_DIV;
    return TO_REP_SUR;
}

void both_iter() { iter(); iter(); } /* Skip two */

/* Expected current lexical type */
void expect_pre(token_kind kind) {
    if (state.pre.kind != kind) {
        fprintf(stderr, "\033[1;31m<compiler %d>:\033[0m unexpected '%s' but found '%s'.\n",
            state.pre.line, token_string[kind], state.pre.literal);
        exit(EXIT_FAILURE);
    } else {
        iter();
    }
}

/* Expected next lexical type */
void expect_cur(token_kind kind) {
    if (state.cur.kind != kind) {
        fprintf(stderr, "\033[1;31m<compiler %d>:\033[0m unexpected '%s' but found '%s'.\n",
            state.cur.line, token_string[kind], state.cur.literal);
        exit(EXIT_FAILURE);
    } else {
        iter();
    }
}

/* Output debug information. */
void debug() { printf("%s %s\n", state.pre.literal, state.cur.literal); }

/* LITERAL */
void literal() {
    token tok = state.pre;
    object *obj = (object *) malloc(sizeof(object));

    switch (tok.kind) {
        case NUMBER:
            obj->kind = OBJ_INT;
            obj->value.integer = atoi(tok.literal);
            break;
        case FLOAT:
            obj->kind = OBJ_FLOAT;
            obj->value.floating = atof(tok.literal);
            break;
        case CHAR:
            obj->kind = OBJ_CHAR;
            obj->value.ch = tok.literal[0];
            break;
        case STRING:
            obj->kind = OBJ_STRING;
            obj->value.string = tok.literal;
            break;
    }
    emit_top_obj(obj);
    emit_top_code(CONST_OF);
}

/* NAME: <IDENT> */
void name() {
    token name = state.pre;
    if (state.cur.kind == EQ) { /* = */
        both_iter();
        set_precedence(P_LOWEST);
        emit_top_code(ASSIGN_TO);
    } else if (ass_operator()) { /* += -= *= /= %= */
        token_kind operator = state.cur.kind;
        both_iter();
        set_precedence(P_LOWEST);
        emit_top_code(get_ass_code(operator));
    } else {
        emit_top_code(LOAD_OF);
    }
    emit_top_name(name.literal);
}

/* UNARY: ! - */
void unary() {
    token_kind operator = state.pre.kind;
    iter();
    set_precedence(P_UNARY);

    if (operator == SUB)  emit_top_code(TO_NOT);  /* - */
    if (operator == BANG) emit_top_code(TO_BANG); /* ! */
    if (operator == SUR)  emit_top_code(TO_SUR);  /* % */
}

/* BINARY: + - * / % */
void binary() {
    token_kind operator = state.pre.kind;
    
    int prec = get_pre_prec();
    iter(); /* In order to resolve the next expression */
    /* Start again, parse from prefix.
       1. When the future expression is higher than the current one,
          the next infix is processed.
       2. A higher level starts after me for priority. */
    set_precedence(prec);

    switch (operator) {
        case ADD:     emit_top_code(TO_ADD);    break; /* + */
        case SUB:     emit_top_code(TO_SUB);    break; /* - */
        case MUL:     emit_top_code(TO_MUL);    break; /* * */
        case DIV:     emit_top_code(TO_DIV);    break; /* / */
        case SUR:     emit_top_code(TO_SUR);    break; /* % */
        case OR:      emit_top_code(TO_OR);     break; /* | */
        case ADDR:    emit_top_code(TO_AND);    break; /* & */
        case EQ_EQ:   emit_top_code(TO_EQ_EQ);  break; /* == */
        case BANG_EQ: emit_top_code(TO_NOT_EQ); break; /* != */
        case GREATER: emit_top_code(TO_GR);     break; /* > */
        case GR_EQ:   emit_top_code(TO_GR_EQ);  break; /* >= */
        case LESS:    emit_top_code(TO_LE);     break; /* < */
        case LE_EQ:   emit_top_code(TO_LE_EQ);  break; /* <= */
    }
}

/* GROUP OR TUP: (E) */
void group() {
    iter();
    if (state.cur.kind == R_PAREN || state.cur.kind == COMMA) {
        /* Tuple */
        int count = 0;
        while (state.pre.kind != R_PAREN) {
            set_precedence(P_LOWEST);
            count ++;
            iter();
            if (state.pre.kind == R_PAREN) break;
            expect_pre(COMMA);
        }
        emit_top_code(BUILD_TUP);
        emit_top_offset(count);
        return;
    }
    set_precedence(P_LOWEST); /* Group */
    iter();
    expect_pre(R_PAREN);
}

/* GET: X.Y */
void get() {
    iter();
    token name = state.pre;
    if (state.cur.kind == EQ) { /* = */
        both_iter();
        set_precedence(P_LOWEST);
        emit_top_code(SET_OF);
    } else if (ass_operator()) { /* += -= *= /= %= */
        token_kind operator = state.cur.kind;
        both_iter();
        set_precedence(P_LOWEST);
        emit_top_code(get_ass_code(operator));
    } else {
        emit_top_code(GET_OF);
    }
    emit_top_name(name.literal);
}

/* CALL: X(Y..) */
void call() {
    iter();
    int count = 0;
    while (state.pre.kind != R_PAREN) {
        set_precedence(P_LOWEST);
        count ++;
        iter();
        if (state.pre.kind == R_PAREN) break;
        expect_pre(COMMA);
    }
    emit_top_code(CALL_FUNC);
    emit_top_offset(count);
}

/* INDEX: E[E] */
void indexes() {
    iter();
    set_precedence(P_LOWEST);
    expect_cur(R_BRACKET);
    if (state.cur.kind == EQ) { /* = */
        both_iter();
        set_precedence(P_LOWEST);
        emit_top_code(TO_REPLACE);
    } else if (ass_operator()) { /* += -= *= /= %= */
        token_kind operator = state.cur.kind;
        both_iter();
        set_precedence(P_LOWEST);
        emit_top_code(get_rep_ass_code(operator));
    } else {
        emit_top_code(TO_INDEX);
    }
}

/* ARRAY: [E..] */
void array() {
    iter();
    int count = 0;
    while (state.pre.kind != R_BRACKET) {
        set_precedence(P_LOWEST);
        count ++;
        iter();
        if (state.pre.kind == R_BRACKET) break;
        expect_pre(COMMA);
    }
    emit_top_code(BUILD_ARR);
    emit_top_offset(count);
}

/* MAP: {K1: V2..} */
void map() {
    iter();
    int count = 0;
    while (state.pre.kind != R_BRACE) {
        set_precedence(P_LOWEST);
        iter();
        expect_pre(COLON);
        set_precedence(P_LOWEST);
        iter();
        count += 2;
        if (state.pre.kind == R_BRACE) break;
        expect_pre(COMMA);
    }
    emit_top_code(BUILD_MAP);
    emit_top_offset(count);
}

/* Rules */
rule rules[] = {
    { EOH,       NULL,    NULL,    P_LOWEST  },
    { LITERAL,   name,    NULL,    P_LOWEST  },
    { NUMBER,    literal, NULL,    P_LOWEST  },
    { FLOAT,     literal, NULL,    P_LOWEST  },
    { STRING,    literal, NULL,    P_LOWEST  },
    { CHAR,      literal, NULL,    P_LOWEST  },
    { ADD,       NULL,    binary,  P_TERM    }, // +
    { SUB,       unary,   binary,  P_TERM    }, // -
    { MUL,       NULL,    binary,  P_FACTOR  }, // *
    { DIV,       NULL,    binary,  P_FACTOR  }, // /
    { SUR,       NULL,    binary,  P_FACTOR  }, // %
    { OR,        NULL,    binary,  P_OR      }, // |
    { ADDR,      NULL,    binary,  P_AND     }, // &
    { EQ_EQ,     NULL,    binary,  P_EQ      }, // ==
    { BANG_EQ,   NULL,    binary,  P_EQ      }, // !=
    { GREATER,   NULL,    binary,  P_COMPARE }, // >
    { GR_EQ,     NULL,    binary,  P_COMPARE }, // >=
    { LESS,      NULL,    binary,  P_COMPARE }, // <
    { LE_EQ,     NULL,    binary,  P_COMPARE }, // <=
    { L_PAREN,   group,   call,    P_CALL    }, // (
    { DOT,       NULL,    get,     P_CALL    }, // .
    { L_BRACKET, array,   indexes, P_CALL    }, // [
    { L_BRACE,   map,     NULL,    P_CALL    }, // {
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
            "\033[1;31m<compiler %d>:\033[0m not found prefix function of token '%s'.\n",
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

/* Type system */
type *set_type() {
    token now = state.pre;
    type *T = (type *) malloc(sizeof(type));
    switch (now.kind) {
        case LITERAL:
            if (strcmp(now.literal, "int") == 0)         T->kind = T_INT;
            else if (strcmp(now.literal, "float") == 0)  T->kind = T_FLOAT;
            else if (strcmp(now.literal, "bool") == 0)   T->kind = T_BOOL;
            else if (strcmp(now.literal, "string") == 0) T->kind = T_STRING;
            else {
                T->kind = T_USER;
                T->inner.name = now.literal;
            }
            break;
        case L_BRACKET: /* Array */
            both_iter();
            T->kind = T_ARRAY;
            T->inner.single = (struct type *) set_type();
            break;
        case L_PAREN: /* Tuple */
            both_iter();
            T->kind = T_TUPLE;
            T->inner.single = (struct type *) set_type();
            break;
        case L_BRACE: /* Map */
            both_iter();
            T->kind = T_MAP;
            expect_pre(LESS);
            T->inner.both.T1 = (struct type *) set_type();
            iter();
            expect_pre(COMMA);
            T->inner.both.T2 = (struct type *) set_type();
            expect_cur(GREATER);
            break;
    }
    return T;
}

void block(); /* Placeholder */

/* Statements */
void stmt() {
    switch (state.pre.kind) {
        case DEF: {
            iter();
            token name = state.pre;
            iter();
            if (name.kind == L_PAREN) { /* Function */
                printf("FUNCTION\n");
            } else if (state.pre.kind == DEF ||
                       state.pre.kind == L_ARROW) { /* Whole */
                printf("WHOLE\n");
            } else { /* Variable: def <name> <type> = <stmt> */
                type *T = set_type();
                iter();
                expect_pre(EQ);

                set_precedence(P_LOWEST); /* expression */

                emit_top_type(T);
                emit_top_code(STORE_NAME);
                emit_top_name(name.literal);
            }
            break;
        }
        case IF: {
            iter();
            set_precedence(P_LOWEST); /* if condition */
            iter();

            emit_top_code(F_JUMP_TO);
            int if_p = *get_top_offset_p(); /* TO: F_JUMP_TO */
            block(); /* if body */

            /* Used to cache JUMP_TO where the bytecode is inserted */
            list *p = new_list();
            
            /* Conditional fetching is used to jump out of conditional statements */
            if (state.cur.kind == EF || state.cur.kind == NF) {
                p = append_list(p, get_top_offset_p());
                emit_top_code(JUMP_TO);
            }
            /* TO: if condition(F_JUMP_TO) */
            insert_top_offset(if_p, get_top_code_len());

            /* ef nodes */
            while (state.cur.kind == EF) {
                both_iter();
                set_precedence(P_LOWEST); /* ef condition */
                iter();

                emit_top_code(F_JUMP_TO);
                int ef_p = *get_top_offset_p(); /* TO: F_JUMP_TO */
                block();

                /* Jump out the statements */
                if (state.cur.kind == EF || state.cur.kind == NF) {
                    p = append_list(p, get_top_offset_p());
                    emit_top_code(JUMP_TO);
                }
                /* TO: ef condition(F_JUMP_TO) */
                insert_top_offset(ef_p, get_top_code_len());
            }

            /* nf node, direct resolution */
            if (state.cur.kind == NF) {
                both_iter();
                block();
            }

            /* The position of the jump out conditional statements is here */
            for (int i = 0; i < p->len; i ++)
                insert_top_offset(
                    *(int *)(p->data[i]) + 1,
                    /* Skip to the current bytecode position */
                    get_top_code_len()
                );
            free_list(p); /* Release cache list */
            break;
        }
        case AOP:
        case FOR:
        case OUT:
        case GO:
        case RET:
        case NEW:
        case MOD:
        case USE:
            break;
        default: set_precedence(P_LOWEST); /* Expression */
    }
}

/* Compile module code of the same level */
void block() {
    token *tok = (token *)tokens->data[state.p - 1];
    int off = state.pre.off;

    if (off <= tok->off) {
        fprintf(stderr, "\033[1;31m<compiler %d>:\033[0m no block statement.\n",
            state.pre.line);
        exit(EXIT_FAILURE);
    }
    while (true) {
        stmt(); /* Single statement */
        if (state.cur.off == off) { /* Loop compilation multiple statements */
            iter();
        } else {
            break;
        }
    }
}

/* Compiler */
void compile() {
    both_iter();

    /* Push global object */
    code_object *obj = (code_object *) malloc(sizeof(code_object));
    obj->description = "main";
    push_obj_list(obj);

    while (state.pre.kind != EOH) {
        stmt();
        iter();
    }
    emit_top_code(TO_RET);
}

/* detailed information */
void dissemble() {
    for (int i = 0; i < objs->len; i ++) {
        code_object *code 
            = (code_object *) objs->data[i];
        printf("<%s>: %d code, %d name, %d type, %d object, %d offset\n",
            code->description,
            code->codes == NULL ? 0 : code->codes->len,
            code->names == NULL ? 0 : code->names->len,
            code->types == NULL ? 0 : code->types->len,
            code->objects == NULL ? 0 : code->objects->len,
            code->offsets == NULL ? 0 : code->offsets->len);

        // for (int i = 0; i < code->offsets->len; i ++)
        //     printf("%d\n", *(u_int8_t *)code->offsets->data[i]);

        for (int b = 0, p = 0; b < code->codes->len; b ++) {
            op_code *inner
                = (op_code *) code->codes->data[b];
            printf("[%2d] %10s ", b + 1, code_string[*inner]);
            switch (*inner) {
                case CONST_OF: {
                    u_int8_t *off = (u_int8_t *) code->offsets->data[p ++];
                    object *obj = (object *) code->objects->data[*off];
                    printf("%3d %3s\n", *off, obj_string(obj));
                    break;
                }
                case LOAD_OF: case GET_OF:  case SET_OF:  case ASSIGN_TO:
                case ASS_ADD: case ASS_SUB: case ASS_MUL: case ASS_DIV:
                case ASS_SUR: {
                    u_int8_t *off = (u_int8_t *) code->offsets->data[p ++];
                    char *name = (char *) code->names->data[*off];
                    printf("%3d #%s\n", *off, name);
                    break;
                }
                case CALL_FUNC:
                case JUMP_TO:
                case T_JUMP_TO:
                case F_JUMP_TO: {
                    u_int8_t *off = (u_int8_t *) code->offsets->data[p ++];
                    printf("%3d\n", *off);
                    break;
                }
                case STORE_NAME: {
                    u_int8_t *x = (u_int8_t *) code->offsets->data[p];
                    u_int8_t *y = (u_int8_t *) code->offsets->data[p + 1];
                    printf("%3d %s %d '%s'\n", 
                        *x, type_string((type *) code->types->data[*x]), *y,
                        (char *) code->names->data[*y]);
                    p += 2;
                    break;
                }
                case BUILD_ARR: case BUILD_TUP: case BUILD_MAP: {
                    u_int8_t *count = (u_int8_t *) code->offsets->data[p ++];
                    printf("%3d\n", *count);
                    break;
                }
                default: printf("\n");
            }
        }
    }
}

/* ? */
int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: ./drift (file)\n");
        exit(EXIT_FAILURE);
    }
    const char *path = argv[1];
    int len = strlen(path) - 1;
    if (path[len] != 't'  || 
        path[len - 1] != 'f' ||
        path[len - 2] != '.') {
            fprintf(stderr, 
                "\033[0;31merror:\033[0m please load the source file with .ft sufix.\n");
            exit(EXIT_FAILURE);
    }
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
    /* for (int i = 0; i < tokens->len; i ++) {
        token *t = tokens->data[i];
        printf("[%3d]\t%-5d %-20s %-20d %d\n", 
            i, t->kind, t->literal, t->line, t->off);
    } */

    /* Compiler */
    compile();

    dissemble();
    
    fclose(fp); /* Close file */
    free(buf); /* Close buffer */
    free_list(tokens); /* Release tokens */
    return 0;
}