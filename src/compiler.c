/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#include <stdbool.h>
#include <stdio.h>

#include "keg.h"
#include "object.h"
#include "opcode.h"
#include "token.h"
#include "type.h"

typedef struct {
    keg *tokens;
    token pre;
    token cur;
    int16_t iof;
    int16_t inf;
    int16_t itf;
    int p;
    bool loop;
    keg *codes;
} compile_state;

code_object *new_code(char *des) {
    code_object *code = malloc(sizeof(code_object));
    code->description = des;
    code->codes = NULL;
    code->lines = NULL;
    code->names = NULL;
    code->objects = NULL;
    code->offsets = NULL;
    code->types = NULL;
    code->jumps = NULL;
    return code;
}

compile_state cst;

compile_state backup_state() {
    compile_state up;
    up.iof = cst.iof;
    up.inf = cst.inf;
    up.itf = cst.itf;
    up.p = cst.p;
    return up;
}

void reset_state(compile_state *state, compile_state up) {
    state->iof = up.iof;
    state->inf = up.inf;
    state->itf = up.itf;
    state->p = up.p;
}

void clear_state() {
    cst.iof = 0;
    cst.inf = 0;
    cst.itf = 0;
}

void push_code_keg(code_object *code) {
    cst.codes = append_keg(cst.codes, code);
}

code_object *back_code() {
    return back_keg(cst.codes);
}

void replace_holder(int16_t place, int16_t off) {
    code_object *code = back_code();
    if (code->offsets == NULL) {
        return;
    }
    for (int i = 0; i < code->offsets->item; i++) {
        if (*(int16_t *)(code->offsets->data[i]) == place) {
            int16_t *f = malloc(sizeof(int16_t));
            *f = off;
            replace_keg(code->offsets, i, f);
        }
    }
}

int16_t *get_offset_p() {
    code_object *code = back_code();
    int16_t *f = malloc(sizeof(int16_t));
    *f = code->offsets->item;
    return f;
}

int get_code_len() {
    code_object *code = back_code();
    if (code->codes == NULL) {
        return 0;
    }
    return code->codes->item;
}

void insert_offset(int p, int16_t off) {
    code_object *code = back_code();
    int16_t *f = malloc(sizeof(int16_t));
    *f = off;
    if (p == -1) {
        code->offsets = append_keg(code->offsets, f);
    } else {
        insert_keg(code->offsets, p, f);
    }
    code->jumps = append_keg(code->jumps, f);
}

void emit_offset(int16_t off) {
    code_object *code = back_code();
    int16_t *f = malloc(sizeof(int16_t));
    *f = off;
    code->offsets = append_keg(code->offsets, f);
}

void emit_name(char *name) {
    code_object *code = back_code();
    if (code->names != NULL) {
        for (int i = 0; i < code->names->item; i++) {
            if (strcmp((char *)code->names->data[i], name) == 0) {
                emit_offset(i);
                return;
            }
        }
    }
    code->names = append_keg(code->names, name);
    emit_offset(cst.inf++);
}

void emit_type(type *t) {
    code_object *code = back_code();
    for (int i = 0; code->types != NULL && i < code->types->item; i++) {
        type *x = code->types->data[i];
        if (x->kind <= 4 && t->kind <= 4 && x->kind == t->kind) {
            free(t);
            emit_offset(i);
            return;
        }
    }
    code->types = append_keg(code->types, t);
    emit_offset(cst.itf++);
}

void emit_obj(object *obj) {
    code_object *code = back_code();
    code->objects = append_keg(code->objects, obj);
    emit_offset(cst.iof++);
}

int l = 0;
int t = -1;

void emit_code(u_int8_t op) {
    op_code *c = malloc(sizeof(u_int8_t));
    *c = op;
    int *n = malloc(sizeof(int));
    if (t != -1) {
        *n = t;
        t = -1;
    } else {
        *n = l;
    }
    code_object *code = back_code();
    code->codes = append_keg(code->codes, c);
    code->lines = append_keg(code->lines, n);
}

int p = 0;

void iter() {
    cst.pre = cst.cur;
    if (p == cst.tokens->item) {
        return;
    }
    cst.cur = *(token *)cst.tokens->data[p++];
    cst.p = p - 2;
}

typedef enum {
    P_LOWEST,
    P_OR,
    P_AND,
    P_EQ,
    P_COMPARE,
    P_TERM,
    P_FACTOR,
    P_NEW,
    P_UNARY,
    P_CALL,
} precedence;

typedef void (*function)();

typedef struct {
    token_kind kind;
    function prefix;
    function infix;
    int precedence;
} rule;

rule get_rule(token_kind kind);
void set_precedence(int prec);

int get_pre_prec() {
    return get_rule(cst.pre.kind).precedence;
}

int get_cur_prec() {
    return get_rule(cst.cur.kind).precedence;
}

bool ass_operator() {
    return cst.cur.kind == AS_ADD || cst.cur.kind == AS_SUB ||
           cst.cur.kind == AS_MUL || cst.cur.kind == AS_DIV ||
           cst.cur.kind == AS_SUR;
}

u_int8_t get_ass_code(token_kind kind) {
    if (kind == AS_ADD)
        return ASS_ADD;
    if (kind == AS_SUB)
        return ASS_SUB;
    if (kind == AS_MUL)
        return ASS_MUL;
    if (kind == AS_DIV)
        return ASS_DIV;
    return ASS_SUR;
}

u_int8_t get_rep_ass_code(token_kind kind) {
    if (kind == AS_ADD)
        return TO_REP_ADD;
    if (kind == AS_SUB)
        return TO_REP_SUB;
    if (kind == AS_MUL)
        return TO_REP_MUL;
    if (kind == AS_DIV)
        return TO_REP_DIV;
    return TO_REP_SUR;
}

u_int8_t get_set_ass_code(token_kind kind) {
    if (kind == AS_ADD)
        return SE_ASS_ADD;
    if (kind == AS_SUB)
        return SE_ASS_SUB;
    if (kind == AS_MUL)
        return SE_ASS_MUL;
    if (kind == AS_DIV)
        return SE_ASS_DIV;
    return SE_ASS_SUR;
}

void both_iter() {
    iter();
    iter();
}

enum expect_kind { PRE, CUR };

void expect_error(token_kind kind) {
    fprintf(stderr,
            "\033[1;31mcompiler %d:\033[0m unexpected '%s' but found '%s'.\n",
            cst.pre.line, token_string[kind], cst.pre.literal);
    exit(EXIT_SUCCESS);
}

void expect(enum expect_kind exp, token_kind kind) {
    if (exp == PRE && cst.pre.kind != kind) {
        expect_error(kind);
    }
    if (exp == CUR && cst.cur.kind != kind) {
        expect_error(kind);
    }
    iter();
}

void debug() {
    printf("%s %s\n", cst.pre.literal, cst.cur.literal);
}

void syntax_error() {
    fprintf(stderr, "\033[1;31mcompiler %d:\033[0m syntax error.\n",
            cst.pre.line);
    exit(EXIT_SUCCESS);
}

void no_block_error() {
    fprintf(stderr, "\033[1;31mcompiler %d:\033[0m no block statement.\n",
            cst.pre.line);
    exit(EXIT_SUCCESS);
}

void literal() {
    token tok = cst.pre;
    object *obj = malloc(sizeof(object));

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
    case NIL:
        obj->kind = OBJ_NIL;
        break;
    }
    emit_obj(obj);
    emit_code(CONST_OF);
}

void name() {
    token name = cst.pre;
    if (cst.cur.kind == EQ) {
        both_iter();
        set_precedence(P_LOWEST);
        emit_code(ASSIGN_TO);
    } else if (ass_operator()) {
        token_kind op = cst.cur.kind;
        both_iter();
        set_precedence(P_LOWEST);
        emit_code(get_ass_code(op));
    } else {
        emit_code(LOAD_OF);
    }
    emit_name(name.literal);
}

void unary() {
    token_kind op = cst.pre.kind;
    iter();
    set_precedence(P_UNARY);

    if (op == SUB) {
        emit_code(TO_NOT);
    }
    if (op == BANG) {
        emit_code(TO_BANG);
    }
}

void binary() {
    token_kind op = cst.pre.kind;

    int prec = get_pre_prec();
    iter();
    set_precedence(prec);

    switch (op) {
    case ADD:
        emit_code(TO_ADD);
        break;
    case SUB:
        emit_code(TO_SUB);
        break;
    case MUL:
        emit_code(TO_MUL);
        break;
    case DIV:
        emit_code(TO_DIV);
        break;
    case SUR:
        emit_code(TO_SUR);
        break;
    case OR:
        emit_code(TO_OR);
        break;
    case ADDR:
        emit_code(TO_AND);
        break;
    case EQ_EQ:
        emit_code(TO_EQ_EQ);
        break;
    case BANG_EQ:
        emit_code(TO_NOT_EQ);
        break;
    case GREATER:
        emit_code(TO_GR);
        break;
    case GR_EQ:
        emit_code(TO_GR_EQ);
        break;
    case LESS:
        emit_code(TO_LE);
        break;
    case LE_EQ:
        emit_code(TO_LE_EQ);
        break;
    }
}

void group() {
    iter();
    if (cst.cur.kind == R_PAREN || cst.cur.kind == COMMA) {
        int item = 0;
        while (cst.pre.kind != R_PAREN) {
            set_precedence(P_LOWEST);
            item++;
            iter();
            if (cst.pre.kind == R_PAREN) {
                break;
            }
            expect(PRE, COMMA);
        }
        emit_code(BUILD_TUP);
        emit_offset(item);
        return;
    }
    if (cst.pre.kind == R_PAREN) {
        emit_code(BUILD_TUP);
        emit_offset(0);
        return;
    }
    set_precedence(P_LOWEST);
    expect(CUR, R_PAREN);
}

void get() {
    iter();
    token name = cst.pre;
    if (cst.cur.kind == EQ) {
        both_iter();
        set_precedence(P_LOWEST);
        emit_code(SET_OF);
    } else if (ass_operator()) {
        token_kind op = cst.cur.kind;
        both_iter();
        set_precedence(P_LOWEST);
        emit_code(get_set_ass_code(op));
    } else {
        emit_code(GET_OF);
    }
    emit_name(name.literal);
}

void call() {
    iter();
    int item = 0;
    while (cst.pre.kind != R_PAREN) {
        set_precedence(P_LOWEST);
        item++;
        iter();
        if (cst.pre.kind == R_PAREN) {
            break;
        }
        expect(PRE, COMMA);
    }
    emit_code(CALL_FUNC);
    emit_offset(item);
}

void indexes() {
    iter();
    set_precedence(P_LOWEST);
    expect(CUR, R_BRACKET);
    if (cst.cur.kind == EQ) {
        both_iter();
        set_precedence(P_LOWEST);
        emit_code(TO_REPLACE);
    } else if (ass_operator()) {
        token_kind op = cst.cur.kind;
        both_iter();
        set_precedence(P_LOWEST);
        emit_code(get_rep_ass_code(op));
    } else {
        emit_code(TO_INDEX);
    }
}

void array() {
    iter();
    int item = 0;
    while (cst.pre.kind != R_BRACKET) {
        set_precedence(P_LOWEST);
        item++;
        iter();
        if (cst.pre.kind == R_BRACKET) {
            break;
        }
        expect(PRE, COMMA);
    }
    emit_code(BUILD_ARR);
    emit_offset(item);
}

void map() {
    iter();
    int item = 0;
    while (cst.pre.kind != R_BRACE) {
        set_precedence(P_LOWEST);
        iter();
        expect(PRE, COLON);
        set_precedence(P_LOWEST);
        iter();
        item += 2;
        if (cst.pre.kind == R_BRACE) {
            break;
        }
        expect(PRE, COMMA);
    }
    emit_code(BUILD_MAP);
    emit_offset(item);
}

void tnew() {
    iter();
    set_precedence(P_LOWEST);
    iter();
    expect(PRE, L_BRACE);
    int item = 0;
    while (cst.pre.kind != R_BRACE) {
        if (cst.pre.kind != LITERAL) {
            syntax_error();
        }
        emit_code(SET_NAME);
        emit_name(cst.pre.literal);
        iter();
        expect(PRE, COLON);
        set_precedence(P_LOWEST);
        iter();
        item += 2;
        if (cst.pre.kind == R_BRACE) {
            break;
        }
        expect(PRE, COMMA);
    }
    emit_code(NEW_OBJ);
    emit_offset(item);
}

void get_in() {
    iter();
    if (cst.pre.kind != LITERAL) {
        syntax_error();
    }
    emit_name(cst.pre.literal);
    emit_code(GET_IN_OF);
}

rule rules[] = {
    {EOH,       NULL,    NULL,    P_LOWEST },
    {LITERAL,   name,    NULL,    P_LOWEST },
    {NUMBER,    literal, NULL,    P_LOWEST },
    {FLOAT,     literal, NULL,    P_LOWEST },
    {STRING,    literal, NULL,    P_LOWEST },
    {CHAR,      literal, NULL,    P_LOWEST },
    {NIL,       literal, NULL,    P_LOWEST },
    {BANG,      unary,   NULL,    P_LOWEST },
    {ADD,       NULL,    binary,  P_TERM   },
    {SUB,       unary,   binary,  P_TERM   },
    {MUL,       NULL,    binary,  P_FACTOR },
    {DIV,       NULL,    binary,  P_FACTOR },
    {SUR,       NULL,    binary,  P_FACTOR },
    {NEW,       tnew,    NULL,    P_NEW    },
    {OR,        NULL,    binary,  P_OR     },
    {ADDR,      NULL,    binary,  P_AND    },
    {EQ_EQ,     NULL,    binary,  P_EQ     },
    {BANG_EQ,   NULL,    binary,  P_EQ     },
    {GREATER,   NULL,    binary,  P_COMPARE},
    {GR_EQ,     NULL,    binary,  P_COMPARE},
    {LESS,      NULL,    binary,  P_COMPARE},
    {LE_EQ,     NULL,    binary,  P_COMPARE},
    {L_PAREN,   group,   call,    P_CALL   },
    {DOT,       get_in,  get,     P_CALL   },
    {L_BRACKET, array,   indexes, P_CALL   },
    {L_BRACE,   map,     NULL,    P_CALL   },
};

rule get_rule(token_kind kind) {
    for (int i = 1; i < sizeof(rules) / sizeof(rules[0]); i++) {
        if (rules[i].kind == kind) {
            return rules[i];
        }
    }
    return rules[0];
}

void set_precedence(int precedence) {
    rule prefix = get_rule(cst.pre.kind);
    if (prefix.prefix == NULL) {
        fprintf(stderr,
                "\033[1;31mcompiler %d:\033[0m not found prefix "
                "function of token "
                "'%s'.\n",
                cst.pre.line, cst.pre.literal);
        exit(EXIT_SUCCESS);
    }
    prefix.prefix();
    while (precedence <= get_cur_prec()) {
        rule infix = get_rule(cst.cur.kind);
        if (infix.infix != NULL) {
            iter();
            infix.infix();
        } else {
            break;
        }
    }
}

type *set_type() {
    token now = cst.pre;
    type *T = malloc(sizeof(type));
    switch (now.kind) {
    case LITERAL:
        if (strcmp(now.literal, "int") == 0)
            T->kind = T_INT;
        else if (strcmp(now.literal, "float") == 0)
            T->kind = T_FLOAT;
        else if (strcmp(now.literal, "bool") == 0)
            T->kind = T_BOOL;
        else if (strcmp(now.literal, "char") == 0)
            T->kind = T_CHAR;
        else if (strcmp(now.literal, "string") == 0)
            T->kind = T_STRING;
        else if (strcmp(now.literal, "any") == 0)
            T->kind = T_ANY;
        else {
            T->kind = T_USER;
            T->inner.name = now.literal;
        }
        break;
    case L_BRACKET: {
        both_iter();
        T->kind = T_ARRAY;
        T->inner.single = (struct type *)set_type();
        break;
    }
    case L_PAREN: {
        both_iter();
        T->kind = T_TUPLE;
        T->inner.single = (struct type *)set_type();
        break;
    }
    case L_BRACE: {
        both_iter();
        T->kind = T_MAP;
        expect(PRE, LESS);
        T->inner.both.T1 = (struct type *)set_type();
        iter();
        expect(PRE, COMMA);
        T->inner.both.T2 = (struct type *)set_type();
        expect(CUR, GREATER);
        break;
    }
    case OR:
        iter();
        keg *arg = new_keg();
        type *ret = NULL;
        while (cst.pre.kind != OR) {
            arg = append_keg(arg, set_type());
            iter();
            if (cst.pre.kind == OR) {
                break;
            }
            expect(PRE, COMMA);
        }
        if (cst.cur.kind == R_ARROW) {
            both_iter();
            ret = set_type();
        }
        T->kind = T_FUNCTION;
        T->inner.fn.arg = arg;
        T->inner.fn.ret = (struct type *)ret;
        break;
    default:
        fprintf(stderr, "\033[1;31mcompiler %d:\033[0m unknown '%s' type.\n",
                cst.pre.line, cst.pre.literal);
        exit(EXIT_SUCCESS);
    }
    return T;
}

void block();

keg *parse_generic() {
    keg *gt = new_keg();
    iter();
    if (cst.pre.kind == GREATER) {
        syntax_error();
    }
    while (true) {
        token name = cst.pre;
        if (name.kind != LITERAL) {
            syntax_error();
        }
        iter();

        type *t = malloc(sizeof(type));
        t->kind = T_GENERIC;

        generic *ge = malloc(sizeof(generic));
        ge->name = name.literal;

        if (cst.pre.kind == GREATER || cst.pre.kind == COMMA) {
            ge->count = 0;
        } else {
            type *T = set_type();
            iter();
            if (cst.pre.kind == OR) {
                ge->mtype.multiple = new_keg();
                ge->mtype.multiple = append_keg(ge->mtype.multiple, T);
                iter();

                while (cst.pre.kind != COMMA && cst.pre.kind != GREATER) {
                    ge->mtype.multiple =
                        append_keg(ge->mtype.multiple, set_type());
                    iter();
                }
                ge->count = ge->mtype.multiple->item;
            } else {
                ge->count = 1;
                ge->mtype.T = T;
            }
        }
        t->inner.ge = (struct generic *)ge;
        gt = append_keg(gt, t);

        if (cst.pre.kind == COMMA) {
            iter();
            continue;
        }
        if (cst.pre.kind == GREATER) {
            iter();
            break;
        }
    }
    return gt;
}

enum generic_type { NONE_TYPE, OTHER };

void check_generic_type(keg *gt, enum generic_type t) {
    for (int i = 0; i < gt->item; i++) {
        generic *g = (generic *)((type *)gt->data[i])->inner.ge;
        if (t == NONE_TYPE && g->count != 0) {
            syntax_error();
        }
        if (t == OTHER && g->count < 1) {
            syntax_error();
        }
    }
}

void def_interface(token name, keg *gt, int off, int poff) {
    if (off < poff) {
        no_block_error();
    }
    check_generic_type(gt, NONE_TYPE);

    object *obj = malloc(sizeof(object));
    obj->kind = OBJ_INTERFACE;
    obj->value.in.name = name.literal;
    obj->value.in.element = NULL;
    obj->value.in.gt = gt;

    while (true) {
        method *m = malloc(sizeof(method));
        m->ret = NULL;
        iter();

        if (cst.pre.kind != SLASH) {
            keg *arg = new_keg();

            while (true) {
                arg = append_keg(arg, set_type());
                iter();
                if (cst.pre.kind == SLASH) {
                    break;
                }
                expect(PRE, COMMA);
            }
            iter();

            m->name = cst.pre.literal;
            m->arg = arg;
        } else {
            iter();
            m->name = cst.pre.literal;
            m->arg = new_keg();
        }
        if (cst.cur.kind == R_ARROW) {
            both_iter();
            m->ret = set_type();
        }

        obj->value.in.element = append_keg(obj->value.in.element, m);

        if (cst.cur.off == off) {
            iter();
        } else {
            break;
        }
    }
    t = name.line;
    emit_code(INTERFACE);
    emit_obj(obj);
}

void def_function(keg *gt) {
    check_generic_type(gt, OTHER);

    keg *K = new_keg();
    keg *V = new_keg();

    object *obj = malloc(sizeof(object));
    obj->kind = OBJ_FUNCTION;
    obj->value.fn.k = K;
    obj->value.fn.v = V;
    obj->value.fn.mutiple = NULL;
    obj->value.fn.self = NULL;
    obj->value.fn.gt = gt;

    if (cst.pre.kind != R_PAREN) {
        while (true) {
            if (cst.cur.kind == R_PAREN) {
                break;
            }

            K = append_keg(K, cst.pre.literal);
            if (cst.cur.kind != COMMA) {
                iter();

                if (cst.pre.kind == L_ARROW) {
                    iter();
                    obj->value.fn.mutiple = set_type();

                    iter();
                    if (cst.pre.kind != R_PAREN) {
                        fprintf(stderr,
                                "\033[1;31mcompiler %d:\033[0m multiple "
                                "parameters can only be at the end.\n",
                                cst.pre.line);
                        exit(EXIT_SUCCESS);
                    }
                    break;
                }
                type *T = set_type();
                iter();

                while (K->item != V->item) {
                    V = append_keg(V, T);
                }
                if (cst.pre.kind == R_PAREN) {
                    break;
                }
                expect(PRE, COMMA);
            } else {
                both_iter();
            }
        }
    }
    iter();
    token name = cst.pre;
    iter();

    if (cst.pre.kind == R_ARROW) {
        iter();
        obj->value.fn.ret = set_type();
        iter();
    } else {
        obj->value.fn.ret = NULL;
    }
    if (cst.pre.kind == SEMICOLON) {
        obj->value.fn.std = true;
        obj->value.fn.name = name.literal;
        obj->value.fn.code = NULL;
    } else {
        compile_state up_state = backup_state();
        clear_state();

        code_object *code = new_code(name.literal);
        push_code_keg(code);
        block();

        reset_state(&cst, up_state);

        code_object *ptr = pop_back_keg(cst.codes);
        obj->value.fn.std = false;
        obj->value.fn.name = ptr->description;
        obj->value.fn.code = ptr;
    }
    t = name.line;
    emit_code(FUNCTION);
    emit_obj(obj);
}

void def_class(token name, keg *gt) {
    check_generic_type(gt, OTHER);
    
    compile_state up_state = backup_state();
    clear_state();

    code_object *code = new_code(name.literal);
    push_code_keg(code);
    block();

    reset_state(&cst, up_state);

    code_object *ptr = pop_back_keg(cst.codes);

    object *obj = malloc(sizeof(object));
    obj->kind = OBJ_CLASS;
    obj->value.cl.name = ptr->description;
    obj->value.cl.code = ptr;
    obj->value.cl.fr = NULL;
    obj->value.cl.init = false;
    obj->value.cl.gt = gt;

    t = name.line;
    emit_code(CLASS);
    emit_obj(obj);
}

void stmt() {
    l = cst.pre.line;
    switch (cst.pre.kind) {
    case DEF: {
        iter();
        if (cst.pre.kind == EOH) {
            return;
        }
        
        keg *gt = new_keg();
        if (cst.pre.kind == LESS) {
            gt = parse_generic();
        }
        token name = cst.pre;
        iter();

        int poff = (*(token *)(cst.tokens->data)[cst.p - 1]).off;

        if (cst.pre.kind == LESS) {
            if (gt->data != NULL) {
                syntax_error();
            }
            gt = parse_generic();
        }
        int off = cst.pre.off;

        if (cst.pre.kind == SLASH) {
            def_interface(name, gt, off, poff);
        } else if (name.kind == L_PAREN) {
            def_function(gt);
        } else if (cst.pre.kind == DEF) {
            def_class(name, gt);
        } else {
            type *T = set_type();
            iter();

            if (cst.pre.kind == EQ) {
                iter();
                set_precedence(P_LOWEST);

                emit_type(T);
                emit_code(STORE_NAME);
                emit_name(name.literal);
            } else {
                if (T->kind != T_USER) {
                    syntax_error();
                }
                if (off < poff) {
                    no_block_error();
                }
                keg *elem = new_keg();
                elem = append_keg(elem, T->inner.name);
                free(T);

                while (true) {
                    elem = append_keg(elem, cst.pre.literal);
                    if (cst.cur.off == off) {
                        iter();
                    } else {
                        break;
                    }
                }
                object *obj = malloc(sizeof(object));
                obj->kind = OBJ_ENUMERATE;
                obj->value.en.name = name.literal;
                obj->value.en.element = elem;

                emit_code(ENUMERATE);
                emit_obj(obj);
            }
        }
        break;
    }
    case IF: {
        iter();
        set_precedence(P_LOWEST);
        iter();

        emit_code(F_JUMP_TO);
        int16_t if_p = *get_offset_p();
        block();

        keg *p = new_keg();

        if (cst.cur.kind == EF || cst.cur.kind == NF) {
            p = append_keg(p, get_offset_p());
            emit_code(JUMP_TO);
        }

        insert_offset(if_p, get_code_len());

        while (cst.cur.kind == EF) {
            both_iter();
            set_precedence(P_LOWEST);
            iter();

            emit_code(F_JUMP_TO);
            int16_t ef_p = *get_offset_p();
            block();

            if (cst.cur.kind == EF || cst.cur.kind == NF) {
                int16_t *f = get_offset_p();
                *f += 1;
                p = append_keg(p, f);
                emit_code(JUMP_TO);
            }

            insert_offset(ef_p, get_code_len());
        }
        if (cst.cur.kind == NF) {
            both_iter();
            block();
        }
        for (int i = 0; i < p->item; i++) {
            int f = *(int *)p->data[i];
            insert_offset(f + 1, get_code_len());
        }
        free_keg(p);
        break;
    }
    case AOP: {
        cst.loop = true;
        iter();

        int begin_p = get_code_len();
        if (cst.pre.kind != R_ARROW) {
            set_precedence(P_LOWEST);
            iter();

            emit_code(F_JUMP_TO);
            int16_t expr_p = *get_offset_p();

            block();

            emit_code(JUMP_TO);
            insert_offset(-1, begin_p);

            insert_offset(expr_p, get_code_len());
        } else {
            iter();
            block();

            emit_code(JUMP_TO);
            insert_offset(-1, begin_p);
        }

        replace_holder(-1, get_code_len());
        replace_holder(-2, begin_p);
        break;
    }
    case FOR: {
        cst.loop = true;
        iter();

        stmt();
        iter();
        expect(PRE, SEMICOLON);

        int begin_p = get_code_len();

        set_precedence(P_LOWEST);
        iter();
        expect(PRE, SEMICOLON);
        emit_code(F_JUMP_TO);
        int16_t expr_p = *get_offset_p();

        emit_code(JUMP_TO);
        int16_t body_p = *get_offset_p();

        int update_p = get_code_len();

        set_precedence(P_LOWEST);
        iter();
        emit_code(JUMP_TO);
        insert_offset(-1, begin_p);

        insert_offset(body_p, get_code_len());
        block();
        emit_code(JUMP_TO);
        insert_offset(-1, update_p);

        insert_offset(expr_p, get_code_len());

        replace_holder(-1, get_code_len());
        replace_holder(-2, update_p);
        break;
    }
    case OUT:
    case GO:
        if (!cst.loop) {
            fprintf(stderr, "\033[1;31mcompiler %d:\033[0m Loop control \
statement cannot be used outside loop.\n",
                    cst.pre.line);
            exit(EXIT_SUCCESS);
        }
        token_kind kind = cst.pre.kind;
        iter();
        if (cst.pre.kind == R_ARROW) {
            emit_code(JUMP_TO);
        } else {
            set_precedence(P_LOWEST);
            emit_code(T_JUMP_TO);
        }
        emit_offset(kind == OUT ? -1 : -2);
        break;
    case RET:
        iter();
        if (cst.pre.kind == R_ARROW) {
            emit_code(TO_RET);
        } else {
            stmt();
            emit_code(RET_OF);
        }
        break;
    case USE: {
        token_kind kind = cst.pre.kind;
        iter();
        bool internal = cst.pre.kind == L_ARROW;
        if (internal) {
            iter();
        }
        if (cst.pre.kind != LITERAL) {
            syntax_error();
        }
        emit_name(cst.pre.literal);
        emit_code(internal ? USE_IN_MOD : USE_MOD);
        break;
    }
    default:
        set_precedence(P_LOWEST);
    }
}

void block() {
    token *tok = cst.tokens->data[cst.p - 1];
    int off = cst.pre.off;
    if (off < tok->off) {
        no_block_error();
    }
    while (true) {
        stmt();
        if (cst.cur.off == off) {
            iter();
        } else {
            break;
        }
    }
}

extern keg *compile(keg *t) {
    p = 0;

    cst.tokens = t;
    cst.codes = NULL;
    cst.p = 0;
    clear_state();

    both_iter();

    code_object *code = new_code("main");
    push_code_keg(code);

    while (cst.pre.kind != EOH) {
        stmt();
        iter();
        cst.loop = false;
    }
    emit_code(TO_RET);
    return cst.codes;
}

extern void disassemble_code(code_object *code) {
    printf("%s: %d code, %d name, %d type, %d object, %d offset, %d jump\n",
           code->description, code->codes == NULL ? 0 : code->codes->item,
           code->names == NULL ? 0 : code->names->item,
           code->types == NULL ? 0 : code->types->item,
           code->objects == NULL ? 0 : code->objects->item,
           code->offsets == NULL ? 0 : code->offsets->item,
           code->jumps == NULL ? 0 : code->jumps->item);

    for (int b = 0, p = 0, pl = -1; b < code->codes->item; b++) {
        int line = *(int *)code->lines->data[b];
        if (line != pl) {
            if (pl != -1) {
                printf("\n");
            }
            printf("L%-4d", *(int *)code->lines->data[b]);
            pl = line;
        } else {
            printf("%-5s", " ");
        }

        op_code *inner = code->codes->data[b];
        for (int i = 0; code->jumps != NULL && i < code->jumps->item; i++) {
            int16_t off = *(int16_t *)code->jumps->data[i];
            if (b == off) {
                printf("%3s %5d %10s", ">>", b, code_string[*inner]);
                goto next;
            }
        }
        printf("%9d %10s", b, code_string[*inner]);
    next:
        printf("%-2c", ' ');
        switch (*inner) {
        case CONST_OF:
        case ENUMERATE:
        case FUNCTION:
        case INTERFACE:
        case CLASS: {
            int16_t *off = code->offsets->data[p++];
            object *obj = code->objects->data[*off];
            printf("%d %s\n", *off, obj_string(obj));
            break;
        }
        case LOAD_OF:
        case GET_OF:
        case GET_IN_OF:
        case SET_OF:
        case ASSIGN_TO:
        case ASS_ADD:
        case ASS_SUB:
        case ASS_MUL:
        case ASS_DIV:
        case ASS_SUR:
        case SE_ASS_ADD:
        case SE_ASS_SUB:
        case SE_ASS_MUL:
        case SE_ASS_DIV:
        case SE_ASS_SUR:
        case SET_NAME:
        case USE_MOD:
        case USE_IN_MOD: {
            int16_t *off = code->offsets->data[p++];
            char *name = code->names->data[*off];
            if (*inner == SET_NAME || *inner == USE_MOD)
                printf("%d '%s'\n", *off, name);
            else
                printf("%d #%s\n", *off, name);
            break;
        }
        case CALL_FUNC:
        case NEW_OBJ:
        case JUMP_TO:
        case T_JUMP_TO:
        case F_JUMP_TO: {
            int16_t *off = code->offsets->data[p++];
            printf("%d\n", *off);
            break;
        }
        case STORE_NAME: {
            int16_t *x = code->offsets->data[p];
            int16_t *y = code->offsets->data[p + 1];
            printf("%d %s %d '%s'\n", *x, type_string(code->types->data[*x]),
                   *y, code->names->data[*y]);
            p += 2;
            break;
        }
        case BUILD_ARR:
        case BUILD_TUP:
        case BUILD_MAP: {
            int16_t *item = code->offsets->data[p++];
            printf("%d\n", *item);
            break;
        }
        default:
            printf("\n");
        }
    }

    if (code->objects != NULL) {
        for (int i = 0; i < code->objects->item; i++) {
            object *obj = code->objects->data[i];
            if (obj->kind == OBJ_FUNCTION) {
                disassemble_code(obj->value.fn.code);
            } else if (obj->kind == OBJ_CLASS) {
                disassemble_code(obj->value.cl.code);
            }
        }
    }
}
