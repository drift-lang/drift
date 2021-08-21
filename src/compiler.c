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
    code_object *code = (code_object *)malloc(sizeof(code_object));
    code->description = des;
    code->codes = NULL;
    code->lines = NULL;
    code->names = NULL;
    code->objects = NULL;
    code->offsets = NULL;
    code->types = NULL;
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
    return (code_object *)back_keg(cst.codes);
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

int16_t *get_top_offset_p() {
    code_object *code = back_code();
    int16_t *f = malloc(sizeof(int16_t));
    *f = code->offsets->item;
    return f;
}

int get_top_code_len() {
    code_object *code = back_code();
    if (code->codes == NULL) {
        return 0;
    }
    return code->codes->item;
}

void insert_top_offset(int p, int16_t off) {
    code_object *code = back_code();
    int16_t *f = malloc(sizeof(int16_t));
    *f = off;
    insert_keg(code->offsets, p, f);
}

void emit_top_offset(int16_t off) {
    code_object *code = back_code();
    int16_t *f = malloc(sizeof(int16_t));
    *f = off;
    code->offsets = append_keg(code->offsets, f);
}

void emit_top_name(char *name) {
    code_object *code = back_code();
    if (code->names != NULL) {
        for (int i = 0; i < code->names->item; i++) {
            if (strcmp((char *)code->names->data[i], name) == 0) {
                emit_top_offset(i);
                return;
            }
        }
    }
    code->names = append_keg(code->names, name);
    emit_top_offset(cst.inf++);
}

void emit_top_type(type *t) {
    code_object *code = back_code();
    for (int i = 0; code->types != NULL && i < code->types->item; i++) {
        type *x = (type *)code->types->data[i];
        if (x->kind <= 4 && t->kind <= 4 && x->kind == t->kind) {
            free(t);
            emit_top_offset(i);
            return;
        }
    }
    code->types = append_keg(code->types, t);
    emit_top_offset(cst.itf++);
}

void emit_top_obj(object *obj) {
    code_object *code = back_code();
    code->objects = append_keg(code->objects, obj);
    emit_top_offset(cst.iof++);
}

void emit_top_code(u_int8_t op) {
    op_code *c = (op_code *)malloc(sizeof(u_int8_t));
    *c = op;
    int *l = malloc(sizeof(int));
    *l = cst.pre.line;
    code_object *code = back_code();
    code->codes = append_keg(code->codes, c);
    code->lines = append_keg(code->lines, l);
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
    exit(EXIT_FAILURE);
}

void no_block_error() {
    fprintf(stderr, "\033[1;31mcompiler %d:\033[0m no block statement.\n",
            cst.pre.line);
    exit(EXIT_FAILURE);
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
    emit_top_obj(obj);
    emit_top_code(CONST_OF);
}

void name() {
    token name = cst.pre;
    if (cst.cur.kind == EQ) {
        both_iter();
        set_precedence(P_LOWEST);
        emit_top_code(ASSIGN_TO);
    } else if (ass_operator()) {
        token_kind operator= cst.cur.kind;
        both_iter();
        set_precedence(P_LOWEST);
        emit_top_code(get_ass_code(operator));
    } else {
        emit_top_code(LOAD_OF);
    }
    emit_top_name(name.literal);
}

void unary() {
    token_kind operator= cst.pre.kind;
    iter();
    set_precedence(P_UNARY);

    if (operator== SUB) {
        emit_top_code(TO_NOT);
    }
    if (operator== BANG) {
        emit_top_code(TO_BANG);
    }
}

void binary() {
    token_kind operator= cst.pre.kind;

    int prec = get_pre_prec();
    iter();
    set_precedence(prec);

    switch (operator) {
    case ADD:
        emit_top_code(TO_ADD);
        break;
    case SUB:
        emit_top_code(TO_SUB);
        break;
    case MUL:
        emit_top_code(TO_MUL);
        break;
    case DIV:
        emit_top_code(TO_DIV);
        break;
    case SUR:
        emit_top_code(TO_SUR);
        break;
    case OR:
        emit_top_code(TO_OR);
        break;
    case ADDR:
        emit_top_code(TO_AND);
        break;
    case EQ_EQ:
        emit_top_code(TO_EQ_EQ);
        break;
    case BANG_EQ:
        emit_top_code(TO_NOT_EQ);
        break;
    case GREATER:
        emit_top_code(TO_GR);
        break;
    case GR_EQ:
        emit_top_code(TO_GR_EQ);
        break;
    case LESS:
        emit_top_code(TO_LE);
        break;
    case LE_EQ:
        emit_top_code(TO_LE_EQ);
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
        emit_top_code(BUILD_TUP);
        emit_top_offset(item);
        return;
    }
    if (cst.pre.kind == R_PAREN) {
        emit_top_code(BUILD_TUP);
        emit_top_offset(0);
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
        emit_top_code(SET_OF);
    } else if (ass_operator()) {
        token_kind operator= cst.cur.kind;
        both_iter();
        set_precedence(P_LOWEST);
        emit_top_code(get_set_ass_code(operator));
    } else {
        emit_top_code(GET_OF);
    }
    emit_top_name(name.literal);
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
    emit_top_code(CALL_FUNC);
    emit_top_offset(item);
}

void indexes() {
    iter();
    set_precedence(P_LOWEST);
    expect(CUR, R_BRACKET);
    if (cst.cur.kind == EQ) {
        both_iter();
        set_precedence(P_LOWEST);
        emit_top_code(TO_REPLACE);
    } else if (ass_operator()) {
        token_kind operator= cst.cur.kind;
        both_iter();
        set_precedence(P_LOWEST);
        emit_top_code(get_rep_ass_code(operator));
    } else {
        emit_top_code(TO_INDEX);
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
    emit_top_code(BUILD_ARR);
    emit_top_offset(item);
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
    emit_top_code(BUILD_MAP);
    emit_top_offset(item);
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
        emit_top_code(SET_NAME);
        emit_top_name(cst.pre.literal);
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
    emit_top_code(NEW_OBJ);
    emit_top_offset(item);
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
    {DOT,       NULL,    get,     P_CALL   },
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
        exit(EXIT_FAILURE);
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
        else {
            T->kind = T_USER;
            T->inner.name = now.literal;
        }
        break;
    case L_BRACKET:
        both_iter();
        T->kind = T_ARRAY;
        T->inner.single = (struct type *)set_type();
        break;
    case L_PAREN:
        both_iter();
        T->kind = T_TUPLE;
        T->inner.single = (struct type *)set_type();
        break;
    case L_BRACE:
        both_iter();
        T->kind = T_MAP;
        expect(PRE, LESS);
        T->inner.both.T1 = (struct type *)set_type();
        iter();
        expect(PRE, COMMA);
        T->inner.both.T2 = (struct type *)set_type();
        expect(CUR, GREATER);
        break;
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
    }
    return T;
}

void block();

void stmt() {
    switch (cst.pre.kind) {
    case DEF: {
        iter();
        token name = cst.pre;
        iter();

        token *tok = cst.tokens->data[cst.p - 1];
        int off = cst.pre.off;

        if (cst.pre.kind == SLASH) {
            if (off <= tok->off) {
                no_block_error();
            }

            object *obj = malloc(sizeof(object));
            obj->kind = OBJ_INTERFACE;
            obj->value.in.name = name.literal;
            obj->value.in.element = NULL;

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
                    m->T = arg;
                } else {
                    iter();
                    m->name = cst.pre.literal;
                    m->T = new_keg();
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

            emit_top_code(LOAD_FACE);
            emit_top_obj(obj);
        } else if (name.kind == L_PAREN) {
            keg *K = new_keg();
            keg *V = new_keg();

            if (cst.pre.kind != R_PAREN) {
                while (true) {
                    if (cst.cur.kind == R_PAREN) {
                        break;
                    }

                    K = append_keg(K, cst.pre.literal);
                    if (cst.cur.kind != COMMA) {
                        iter();
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

            object *obj = malloc(sizeof(object));
            obj->kind = OBJ_FUNCTION;
            obj->value.fn.k = K;
            obj->value.fn.v = V;

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

                code_object *ptr = (code_object *)pop_back_keg(cst.codes);
                obj->value.fn.code = ptr;
                obj->value.fn.name = ptr->description;
            }

            emit_top_code(FUNCTION);
            emit_top_obj(obj);
        } else if (cst.pre.kind == DEF) {
            compile_state up_state = backup_state();
            clear_state();

            code_object *code = new_code(name.literal);
            push_code_keg(code);
            block();

            reset_state(&cst, up_state);

            code_object *ptr = (code_object *)pop_back_keg(cst.codes);

            object *obj = malloc(sizeof(object));
            obj->kind = OBJ_CLASS;
            obj->value.cl.name = ptr->description;
            obj->value.cl.code = ptr;
            obj->value.cl.fr = NULL;
            obj->value.cl.init = false;

            emit_top_code(CLASS);
            emit_top_obj(obj);
        } else {
            type *T = set_type();
            iter();

            if (cst.pre.kind == EQ) {
                iter();
                set_precedence(P_LOWEST);

                emit_top_type(T);
                emit_top_code(STORE_NAME);
                emit_top_name(name.literal);
            } else {
                if (T->kind != T_USER) {
                    syntax_error();
                }
                if (off <= tok->off) {
                    no_block_error();
                }

                keg *elem = new_keg();
                elem = append_keg(elem, (char *)T->inner.name);
                free(T);

                while (true) {
                    elem = append_keg(elem, (char *)cst.pre.literal);
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

                emit_top_code(ENUMERATE);
                emit_top_obj(obj);
            }
        }
        break;
    }
    case IF: {
        iter();
        set_precedence(P_LOWEST);
        iter();

        emit_top_code(F_JUMP_TO);
        int16_t if_p = *get_top_offset_p();
        block();

        keg *p = new_keg();

        if (cst.cur.kind == EF || cst.cur.kind == NF) {
            p = append_keg(p, get_top_offset_p());
            emit_top_code(JUMP_TO);
        }

        insert_top_offset(if_p, get_top_code_len());

        while (cst.cur.kind == EF) {
            both_iter();
            set_precedence(P_LOWEST);
            iter();

            emit_top_code(F_JUMP_TO);
            int16_t ef_p = *get_top_offset_p();
            block();

            if (cst.cur.kind == EF || cst.cur.kind == NF) {
                int16_t *f = get_top_offset_p();
                *f += 1;
                p = append_keg(p, f);
                emit_top_code(JUMP_TO);
            }

            insert_top_offset(ef_p, get_top_code_len());
        }

        if (cst.cur.kind == NF) {
            both_iter();
            block();
        }

        for (int i = 0; i < p->item; i++) {
            int f = *(int *)p->data[i];
            insert_top_offset(f + 1, get_top_code_len());
        }
        free_keg(p);
        break;
    }
    case AOP: {
        cst.loop = true;
        iter();

        int begin_p = get_top_code_len();
        if (cst.pre.kind != R_ARROW) {
            set_precedence(P_LOWEST);
            iter();

            emit_top_code(F_JUMP_TO);
            int16_t expr_p = *get_top_offset_p();

            block();

            emit_top_code(JUMP_TO);
            emit_top_offset(begin_p);

            insert_top_offset(expr_p, get_top_code_len());
        } else {
            iter();
            block();

            emit_top_code(JUMP_TO);
            emit_top_offset(begin_p);
        }

        replace_holder(-1, get_top_code_len());
        replace_holder(-2, begin_p);
        break;
    }
    case FOR: {
        cst.loop = true;
        iter();

        stmt();
        iter();
        expect(PRE, SEMICOLON);

        int begin_p = get_top_code_len();

        set_precedence(P_LOWEST);
        iter();
        expect(PRE, SEMICOLON);
        emit_top_code(F_JUMP_TO);
        int16_t expr_p = *get_top_offset_p();

        emit_top_code(JUMP_TO);
        int16_t body_p = *get_top_offset_p();

        int update_p = get_top_code_len();

        set_precedence(P_LOWEST);
        iter();
        emit_top_code(JUMP_TO);
        emit_top_offset(begin_p);

        insert_top_offset(body_p, get_top_code_len());
        block();
        emit_top_code(JUMP_TO);
        emit_top_offset(update_p);

        insert_top_offset(expr_p, get_top_code_len());

        replace_holder(-1, get_top_code_len());
        replace_holder(-2, update_p);
        break;
    }
    case OUT:
    case GO:
        if (!cst.loop) {
            fprintf(stderr, "\033[1;31mcompiler %d:\033[0m Loop control \
statement cannot be used outside loop.\n",
                    cst.pre.line);
            exit(EXIT_FAILURE);
        }
        token_kind kind = cst.pre.kind;
        iter();
        if (cst.pre.kind == R_ARROW) {
            emit_top_code(JUMP_TO);
        } else {
            set_precedence(P_LOWEST);
            emit_top_code(T_JUMP_TO);
        }
        emit_top_offset(kind == OUT ? -1 : -2);
        break;
    case RET:
        iter();
        if (cst.pre.kind == R_ARROW) {
            emit_top_code(TO_RET);
        } else {
            stmt();
            emit_top_code(RET_OF);
        }
        break;
    case USE: {
        token_kind kind = cst.pre.kind;
        iter();
        if (cst.pre.kind != LITERAL) {
            syntax_error();
        }
        emit_top_name(cst.pre.literal);
        emit_top_code(USE_MOD);
        break;
    }
    default:
        set_precedence(P_LOWEST);
    }
}

void block() {
    token *tok = (token *)cst.tokens->data[cst.p - 1];
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

    emit_top_code(TO_RET);
    return cst.codes;
}

extern void disassemble_code(code_object *code) {
    printf("<%s>: %d code, %d name, %d type, %d object, %d offset\n",
           code->description, code->codes == NULL ? 0 : code->codes->item,
           code->names == NULL ? 0 : code->names->item,
           code->types == NULL ? 0 : code->types->item,
           code->objects == NULL ? 0 : code->objects->item,
           code->offsets == NULL ? 0 : code->offsets->item);

    for (int b = 0, p = 0, pl = -1; b < code->codes->item; b++) {
        int line = *(int *)code->lines->data[b];
        if (line != pl) {
            printf("L%-4d", *(int *)code->lines->data[b]);
            pl = line;
        } else {
            printf("%-5s", " ");
        }

        op_code *inner = (op_code *)code->codes->data[b];
        printf("%2d %10s", b, code_string[*inner]);
        switch (*inner) {
        case CONST_OF:
        case ENUMERATE:
        case FUNCTION:
        case LOAD_FACE:
        case CLASS: {
            int16_t *off = (int16_t *)code->offsets->data[p++];
            object *obj = (object *)code->objects->data[*off];
            printf("%4d %3s\n", *off, obj_string(obj));
            break;
        }
        case LOAD_OF:
        case GET_OF:
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
        case USE_MOD: {
            int16_t *off = (int16_t *)code->offsets->data[p++];
            char *name = (char *)code->names->data[*off];
            if (*inner == SET_NAME || *inner == USE_MOD)
                printf("%4d '%s'\n", *off, name);
            else
                printf("%4d #%s\n", *off, name);
            break;
        }
        case CALL_FUNC:
        case NEW_OBJ:
        case JUMP_TO:
        case T_JUMP_TO:
        case F_JUMP_TO: {
            int16_t *off = (int16_t *)code->offsets->data[p++];
            printf("%4d\n", *off);
            break;
        }
        case STORE_NAME: {
            int16_t *x = (int16_t *)code->offsets->data[p];
            int16_t *y = (int16_t *)code->offsets->data[p + 1];
            printf("%4d %s %d '%s'\n", *x,
                   type_string((type *)code->types->data[*x]), *y,
                   (char *)code->names->data[*y]);
            p += 2;
            break;
        }
        case BUILD_ARR:
        case BUILD_TUP:
        case BUILD_MAP: {
            int16_t *item = (int16_t *)code->offsets->data[p++];
            printf("%4d\n", *item);
            break;
        }
        default:
            printf("\n");
        }
    }

    if (code->objects != NULL) {
        for (int i = 0; i < code->objects->item; i++) {
            object *obj = (object *)code->objects->data[i];
            if (obj->kind == OBJ_FUNCTION) {
                disassemble_code(obj->value.fn.code);
            } else if (obj->kind == OBJ_CLASS) {
                disassemble_code(obj->value.cl.code);
            }
        }
    }
}
