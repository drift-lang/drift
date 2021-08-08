/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#include "compiler.h"

/* Allocate new code object */
code_object *new_code(char *des) {
  code_object *obj = (code_object *)malloc(sizeof(code_object));
  obj->description = des;
  obj->codes = NULL;
  obj->lines = NULL;
  obj->names = NULL;
  obj->objects = NULL;
  obj->offsets = NULL;
  obj->types = NULL;
  return obj;
}

/* Global state of compiler */
compile_state state;

/* Backup global state */
compile_state backup_state() {
  compile_state up;
  up.iof = state.iof;
  up.inf = state.inf;
  up.itf = state.itf;
  up.p = state.p;
  return up;
}

/* Reset global state */
void reset_state(compile_state *state, compile_state up) {
  state->iof = up.iof;
  state->inf = up.inf;
  state->itf = up.itf;
  state->p = up.p;
}

/* New state */
void clear_state() {
  state.iof = 0;
  state.inf = 0;
  state.itf = 0;
}

/* Push new code object to list */
void push_code_list(code_object *obj) {
  state.codes = append_list(state.codes, obj);
}

/* Get tail data */
code_object *back() {
  return (code_object *)back_list(state.codes);
}

/* Replace placeholder in offset list */
void replace_holder(int16_t place, int16_t off) {
  code_object *obj = back();
  if (obj->offsets == NULL) {
    return;
  }
  for (int i = 0; i < obj->offsets->len; i++) {
    if (*(int16_t *)(obj->offsets->data[i]) == place) {
      int16_t *f = (int16_t *)malloc(sizeof(int16_t));
      *f = off;
      obj->offsets->data[i] = f;
    }
  }
}

/* Gets the current offset list length */
int *get_top_offset_p() {
  code_object *obj = back();
  int *p = (int *)malloc(sizeof(int));
  *p = obj->offsets->len;
  return p;
}

/* Gets the top bytecode list length */
int get_top_code_len() {
  code_object *obj = back();
  if (obj->codes == NULL) {
    return 0;
  }
  return obj->codes->len; /* The offset used to insert the specified position */
}

/* Insert offset position new element */
void insert_top_offset(int p, int16_t off) {
  code_object *obj = back();
  int16_t *f = (int16_t *)malloc(sizeof(int16_t));
  *f = off;
  insert_list(obj->offsets, p, f);
}

/* Offset */
void emit_top_offset(int16_t off) {
  code_object *obj = back();
  int16_t *f = (int16_t *)malloc(sizeof(int16_t));
  *f = off;
  obj->offsets = append_list(obj->offsets, f);
}

/* Name */
void emit_top_name(char *name) {
  code_object *obj = back();
  if (obj->names != NULL) {
    for (int i = 0; i < obj->names->len; i++) {
      if (/* Duplicate reference to existing name */
          strcmp((char *)obj->names->data[i], name) == 0) {
        emit_top_offset(i);
        return;
      }
    }
  } else {
    obj->names = append_list(obj->names, name);
    emit_top_offset(state.inf++);
  }
}

/* Type */
void emit_top_type(type *t) {
  code_object *obj = back();
  if (obj->types != NULL) {
    for (int i = 0; i < obj->types->len; i++) {
      type *x = (type *)obj->types->data[i];
      if (/* Basic types do not need to be recreated */
          (x->kind <= 4 && t->kind <= 4) && x->kind == t->kind) {
        free(t);
        emit_top_offset(i);
        return;
      }
    }
  } else {
    obj->types = append_list(obj->types, t);
    emit_top_offset(state.itf++);
  }
}

/* Object */
void emit_top_obj(object *obj) {
  code_object *code = back();
  code->objects = append_list(code->objects, obj);
  emit_top_offset(state.iof++);
}

/* Line */
void emit_top_line(int *line) {
  code_object *code = back();
  code->lines = append_list(code->lines, line);
}

/* Code */
void emit_top_code(u_int8_t code) {
  code_object *obj = back();
  op_code *c = (op_code *)malloc(sizeof(u_int8_t));
  *c = code;
  obj->codes = append_list(obj->codes, c);
  int *l = (int *)malloc(sizeof(int));
  *l = state.pre.line;
  emit_top_line(l);
}

int p = 0; /* Traversal lexical list */

/* Iterator of lexical list */
void iter() {
  state.pre = state.cur;
  if (p == state.tokens->len) {
    return;
  }
  state.cur = *(token *)state.tokens->data[p++];
  state.p = p - 2; /* Always point to the current lexical subscript */
}

/* Operation expression priority
   Note: priority is next level processing */
typedef enum {
  P_LOWEST, // *
  // P_ASSIGN,  // =
  P_OR,      // |
  P_AND,     // &
  P_EQ,      // == !=
  P_COMPARE, // > >= < <=
  P_TERM,    // + -
  P_FACTOR,  // * / %
  P_NEW,     // new
  P_UNARY,   // ! -
  P_CALL,    // . () []
} precedence;

typedef void (*function)(); /* Prefix and infix handing functions */

/* Rule */
typedef struct {
  token_kind kind; /* Token kind */
  function prefix; /* Prefix handing */
  function infix;  /* Infix handing */
  int precedence;  /* Priority of this token */
} rule;

/* Placeholder */
rule get_rule(token_kind kind);
void set_precedence(int prec);

int get_pre_prec() {
  return get_rule(state.pre.kind).precedence;
} /* Pre */

int get_cur_prec() {
  return get_rule(state.cur.kind).precedence;
} /* Cur */

/* Find whether the future is an assignment operation */
bool ass_operator() {
  return state.cur.kind == AS_ADD || state.cur.kind == AS_SUB ||
         state.cur.kind == AS_MUL || state.cur.kind == AS_DIV ||
         state.cur.kind == AS_SUR;
}

/* Bytecode returned by assignment operation */
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

/* Array assignment bytecode */
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

/* Assign value to members of the object */
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
} /* Skip two */

/* Expected current lexical type */
void expect_pre(token_kind kind) {
  if (state.pre.kind != kind) {
    fprintf(stderr,
            "\033[1;31mcompiler %d:\033[0m unexpected '%s' but found '%s'.\n",
            state.pre.line, token_string[kind], state.pre.literal);
    exit(EXIT_FAILURE);
  } else {
    iter();
  }
}

/* Expected next lexical type */
void expect_cur(token_kind kind) {
  if (state.cur.kind != kind) {
    fprintf(stderr,
            "\033[1;31mcompiler %d:\033[0m unexpected '%s' but found '%s'.\n",
            state.cur.line, token_string[kind], state.cur.literal);
    exit(EXIT_FAILURE);
  } else {
    iter();
  }
}

/* Output debug information. */
void debug() {
  printf("%s %s\n", state.pre.literal, state.cur.literal);
}

/* Make simple error message in compiler */
void syntax_error() {
  fprintf(stderr, "\033[1;31mcompiler %d:\033[0m syntax error.\n",
          state.pre.line);
  exit(EXIT_FAILURE);
}

/* No block error message */
void no_block_error() {
  fprintf(stderr, "\033[1;31mcompiler %d:\033[0m no block statement.\n",
          state.pre.line);
  exit(EXIT_FAILURE);
}

/* LITERAL */
void literal() {
  token tok = state.pre;
  object *obj = (object *)malloc(sizeof(object));

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

/* NAME: <IDENT> */
void name() {
  token name = state.pre;
  if (state.cur.kind == EQ) { /* = */
    both_iter();
    set_precedence(P_LOWEST);
    emit_top_code(ASSIGN_TO);
  } else if (ass_operator()) { /* += -= *= /= %= */
    token_kind operator= state.cur.kind;
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
  token_kind operator= state.pre.kind;
  iter();
  set_precedence(P_UNARY);

  if (operator== SUB)
    emit_top_code(TO_NOT); /* - */
  if (operator== BANG)
    emit_top_code(TO_BANG); /* ! */
}

/* BINARY: + - * / % */
void binary() {
  token_kind operator= state.pre.kind;

  int prec = get_pre_prec();
  iter(); /* In order to resolve the next expression */
  /* Start again, parse from prefix.
     1. When the future expression is higher than the current one,
        the next infix is processed.
     2. A higher level starts after me for priority. */
  set_precedence(prec);

  switch (operator) {
  case ADD:
    emit_top_code(TO_ADD);
    break; /* + */
  case SUB:
    emit_top_code(TO_SUB);
    break; /* - */
  case MUL:
    emit_top_code(TO_MUL);
    break; /* * */
  case DIV:
    emit_top_code(TO_DIV);
    break; /* / */
  case SUR:
    emit_top_code(TO_SUR);
    break; /* % */
  case OR:
    emit_top_code(TO_OR);
    break; /* | */
  case ADDR:
    emit_top_code(TO_AND);
    break; /* & */
  case EQ_EQ:
    emit_top_code(TO_EQ_EQ);
    break; /* == */
  case BANG_EQ:
    emit_top_code(TO_NOT_EQ);
    break; /* != */
  case GREATER:
    emit_top_code(TO_GR);
    break; /* > */
  case GR_EQ:
    emit_top_code(TO_GR_EQ);
    break; /* >= */
  case LESS:
    emit_top_code(TO_LE);
    break; /* < */
  case LE_EQ:
    emit_top_code(TO_LE_EQ);
    break; /* <= */
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
      count++;
      iter();
      if (state.pre.kind == R_PAREN) {
        break;
      }
      expect_pre(COMMA);
    }
    emit_top_code(BUILD_TUP);
    emit_top_offset(count);
    return;
  }
  if (state.pre.kind == R_PAREN) { /* Empty tuple */
    emit_top_code(BUILD_TUP);
    emit_top_offset(0);
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
    token_kind operator= state.cur.kind;
    both_iter();
    set_precedence(P_LOWEST);
    emit_top_code(get_set_ass_code(operator));
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
    count++;
    iter();
    if (state.pre.kind == R_PAREN) {
      break;
    }
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
    token_kind operator= state.cur.kind;
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
    count++;
    iter();
    if (state.pre.kind == R_BRACKET) {
      break;
    }
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
    if (state.pre.kind == R_BRACE) {
      break;
    }
    expect_pre(COMMA);
  }
  emit_top_code(BUILD_MAP);
  emit_top_offset(count);
}

/* NEW: T{K: V..} */
void new () {
  iter();
  set_precedence(P_LOWEST);
  iter();
  expect_pre(L_BRACE);
  int count = 0;
  while (state.pre.kind != R_BRACE) {
    if (state.pre.kind != LITERAL) {
      syntax_error();
    }
    emit_top_code(SET_NAME);
    emit_top_name(state.pre.literal);
    iter();
    expect_pre(COLON);
    set_precedence(P_LOWEST);
    iter();
    count += 2;
    if (state.pre.kind == R_BRACE) {
      break;
    }
    expect_pre(COMMA);
  }
  emit_top_code(NEW_OBJ);
  emit_top_offset(count);
}

/* Rules */
rule rules[] = {
    {EOH, NULL, NULL, P_LOWEST},
    {LITERAL, name, NULL, P_LOWEST},
    {NUMBER, literal, NULL, P_LOWEST},
    {FLOAT, literal, NULL, P_LOWEST},
    {STRING, literal, NULL, P_LOWEST},
    {CHAR, literal, NULL, P_LOWEST},
    {NIL, literal, NULL, P_LOWEST},      // nil
    {BANG, unary, NULL, P_LOWEST},       // !
    {ADD, NULL, binary, P_TERM},         // +
    {SUB, unary, binary, P_TERM},        // -
    {MUL, NULL, binary, P_FACTOR},       // *
    {DIV, NULL, binary, P_FACTOR},       // /
    {SUR, NULL, binary, P_FACTOR},       // %
    {NEW, new, NULL, P_NEW},             // new
    {OR, NULL, binary, P_OR},            // |
    {ADDR, NULL, binary, P_AND},         // &
    {EQ_EQ, NULL, binary, P_EQ},         // ==
    {BANG_EQ, NULL, binary, P_EQ},       // !=
    {GREATER, NULL, binary, P_COMPARE},  // >
    {GR_EQ, NULL, binary, P_COMPARE},    // >=
    {LESS, NULL, binary, P_COMPARE},     // <
    {LE_EQ, NULL, binary, P_COMPARE},    // <=
    {L_PAREN, group, call, P_CALL},      // (
    {DOT, NULL, get, P_CALL},            // .
    {L_BRACKET, array, indexes, P_CALL}, // [
    {L_BRACE, map, NULL, P_CALL},        // {
};

/* Search by dictionary type */
rule get_rule(token_kind kind) {
  for (int i = 1; i < sizeof(rules) / sizeof(rules[0]); i++) {
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
            "\033[1;31mcompiler %d:\033[0m not found prefix function of token "
            "'%s'.\n",
            state.pre.line, state.pre.literal);
    exit(EXIT_FAILURE);
  }
  prefix.prefix();                         /* Process prefix */
  while (precedence < get_cur_prec()) {    /* Determine future priorities */
    rule infix = get_rule(state.cur.kind); /* Get infix */
    if (infix.infix != NULL) {
      iter();
      infix.infix(); /* Process infix */
    } else {
      break;
    }
  }
}

/* Type system */
type *set_type() {
  token now = state.pre;
  type *T = (type *)malloc(sizeof(type));
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
  case L_BRACKET: /* Array */
    both_iter();
    T->kind = T_ARRAY;
    T->inner.single = (struct type *)set_type();
    break;
  case L_PAREN: /* Tuple */
    both_iter();
    T->kind = T_TUPLE;
    T->inner.single = (struct type *)set_type();
    break;
  case L_BRACE: /* Map */
    both_iter();
    T->kind = T_MAP;
    expect_pre(LESS);
    T->inner.both.T1 = (struct type *)set_type();
    iter();
    expect_pre(COMMA);
    T->inner.both.T2 = (struct type *)set_type();
    expect_cur(GREATER);
    break;
  case OR: /* Function */
    iter();
    list *arg = new_list();
    type *ret;
    while (state.pre.kind != OR) {
      arg = append_list(arg, set_type());
      iter();
      if (state.pre.kind == OR) {
        break;
      }
      expect_pre(COMMA);
    }
    if (state.cur.kind == R_ARROW) {
      both_iter();
      ret = set_type();
    }
    T->kind = T_FUNC;
    T->inner.func.arg = arg;
    T->inner.func.ret = (struct type *)ret;
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

    /* Judge the original offset */
    token *tok = state.tokens->data[state.p - 1];
    int off = state.pre.off;

    if (state.pre.kind == SLASH) { /* Interface */
      if (off <= tok->off) {
        no_block_error();
      }

      object *fa = (object *)malloc(sizeof(object)); /* Object */
      fa->kind = OBJ_FACE;
      fa->value.face.name = name.literal;

      /* Face block parsing */
      while (true) {
        method *m = (method *)malloc(sizeof(method));

        iter(); /* Skip left slash */

        /* Inner elements */
        if (state.pre.kind != SLASH) { /* Have argument */
          list *arg = new_list();

          while (true) { /* Parsing arguments */
            arg = append_list(arg, set_type());
            iter();
            if (state.pre.kind == SLASH) {
              break;
            }
            expect_pre(COMMA);
          }
          iter(); /* Skip right slash */

          m->name = state.pre.literal;
          m->T = arg;
        } else { /* No argument */
          iter();
          m->name = state.pre.literal;
          m->T = new_list();
        }

        if (state.cur.kind == R_ARROW) { /* Method return */
          both_iter();
          m->ret = set_type();
        }

        fa->value.face.element = /* Method elements in faces */
            append_list(fa->value.face.element, m);

        if (state.cur.off == off) { /* To next statemt in block */
          iter();
        } else {
          break;
        }
      }
      /* Bytecode */
      emit_top_code(LOAD_FACE);
      emit_top_obj(fa);
    } else if (name.kind == L_PAREN) { /* Function */
      list *K = new_list();            /* Argument name */
      list *V = new_list();            /* Argument type */

      if (state.pre.kind != R_PAREN) { /* Arguments */
        while (true) {
          if (state.cur.kind == R_PAREN) {
            break;
          }

          K = append_list(K, state.pre.literal); /* Name */
          if (state.cur.kind != COMMA) {         /* And type */
            iter();
            type *T = set_type(); /* Type */
            iter();

            /* Type alignment for multiple argument types */
            while (K->len != V->len) {
              V = append_list(V, T); /* One-on-one */
            }
            if (state.pre.kind == R_PAREN) {
              break;
            }
            expect_pre(COMMA);
          } else {
            both_iter();
          }
        }
      }

      object *func = (object *)malloc(sizeof(object)); /* Object */
      func->kind = OBJ_FUNC;
      func->value.func.k = K; /* Argument name */
      func->value.func.v = V; /* Argument type */

      iter();
      token name = state.pre; /* Function name */
      iter();

      if (state.pre.kind == R_ARROW) { /* Function return */
        iter();
        func->value.func.ret = set_type();
        iter();
      }

      /* Back up the current compilation state,
         It will create a new object to parse the function body */
      compile_state up_state = backup_state();
      clear_state(); /* New state */

      /* New code object */
      code_object *code = new_code(name.literal);
      push_code_list(code); /* To top */
      block();              /* Function body */

      /* Reset status */
      reset_state(&state, up_state);

      code_object *ptr =
          (code_object *)pop_back_list(state.codes); /* Pop back */
      func->value.func.code = ptr;
      func->value.func.name = ptr->description;

      /* Bytecode */
      emit_top_code(LOAD_FUNC);
      emit_top_obj(func);
    } else if (state.pre.kind == DEF) {        /* Whole */
      compile_state up_state = backup_state(); /* Current state */
      clear_state();                           /* New state */

      /* Code */
      code_object *code = new_code(name.literal);
      push_code_list(code); /* Code */
      block();              /* Body */

      reset_state(&state, up_state); /* State */

      code_object *ptr =
          (code_object *)pop_back_list(state.codes); /* Pop back */
      /* Object */
      object *wh = (object *)malloc(sizeof(object));
      wh->kind = OBJ_WHOLE;
      wh->value.whole.name = ptr->description;
      wh->value.whole.code = ptr;

      /* Bytecode */
      emit_top_code(LOAD_WHOLE);
      emit_top_obj(wh);
    } else {
      type *T = set_type();
      iter();

      /* Variable: def <name> <type> = <stmt> */
      if (state.pre.kind == EQ) {
        iter();
        set_precedence(P_LOWEST); /* expression */

        emit_top_type(T);
        emit_top_code(STORE_NAME);
        emit_top_name(name.literal);
      } else { /* Enumeration */
        if (T->kind != T_USER) {
          syntax_error();
        }
        if (off <= tok->off) {
          no_block_error();
        }

        list *elem = new_list();
        elem = append_list(elem, (char *)T->inner.name); /* Previous */
        free(T);
        /* Enums */
        while (true) {
          elem = append_list(elem, (char *)state.pre.literal);
          if (state.cur.off == off) {
            iter();
          } else {
            break;
          }
        }
        /* Enum object */
        object *en = (object *)malloc(sizeof(object));
        en->kind = OBJ_ENUM;
        en->value.enumeration.name = name.literal;
        en->value.enumeration.element = elem;
        /* Bytecode */
        emit_top_code(LOAD_ENUM);
        emit_top_obj(en);
      }
    }
    break;
  }
  case IF: {
    iter();
    set_precedence(P_LOWEST); /* if condition */
    iter();

    emit_top_code(F_JUMP_TO);
    int if_p = *get_top_offset_p(); /* TO: F_JUMP_TO */
    block();                        /* if body */

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
      block();                        /* ef body */

      /* Jump out the statements */
      if (state.cur.kind == EF || state.cur.kind == NF) {
        int *f = get_top_offset_p();
        *f += 1; /* The ef node has not yet inserted an offset,
        so a placeholder is added here. */
        p = append_list(p, f);
        emit_top_code(JUMP_TO);
      }
      /* TO: ef condition(F_JUMP_TO) */
      insert_top_offset(ef_p, get_top_code_len());
    }

    /* nf node, direct resolution */
    if (state.cur.kind == NF) {
      both_iter();
      block(); /* nf body */
    }

    /* The position of the jump out conditional statements is here */
    for (int i = 0; i < p->len; i++)
      insert_top_offset(*(int *)(p->data[i]) + 1,
                        /* Skip to the current bytecode position */
                        get_top_code_len());
    free_list(p); /* Release cache list */
    break;
  }
  case AOP: {
    state.loop_handler = true;
    iter();

    /* Expression position for loop return */
    int begin_p = get_top_code_len();
    if (state.pre.kind != R_ARROW) {
      set_precedence(P_LOWEST); /* Condition */
      iter();

      emit_top_code(F_JUMP_TO);
      int expr_p = *get_top_offset_p();

      block(); /* Loop block */

      emit_top_code(JUMP_TO); /* Jump to the beginning */
      emit_top_offset(begin_p);

      insert_top_offset(expr_p, get_top_code_len()); /* TO: F_JUMP_TO */
    } else {
      iter();
      block(); /* Loop block */

      emit_top_code(JUMP_TO);
      emit_top_offset(begin_p);
    }
    /* Replace placeholder in offset list */
    replace_holder(-1, get_top_code_len());
    replace_holder(-2, begin_p);
    break;
  }
  case FOR: {
    state.loop_handler = true;
    iter();

    stmt(); /* Initial */
    iter();
    expect_pre(SEMICOLON);

    int begin_p = get_top_code_len(); /* Bytecode pos */
    /* Expression condition */
    set_precedence(P_LOWEST);
    iter();
    expect_pre(SEMICOLON);
    emit_top_code(F_JUMP_TO);
    int expr_p = *get_top_offset_p();

    emit_top_code(JUMP_TO);
    int body_p = *get_top_offset_p();

    int update_p = get_top_code_len(); /* Bytecode pos */
    /* Expression update */
    set_precedence(P_LOWEST);
    iter();
    emit_top_code(JUMP_TO);
    emit_top_offset(begin_p);

    /* Loop body */
    insert_top_offset(body_p, get_top_code_len());
    block();
    emit_top_code(JUMP_TO);
    emit_top_offset(update_p);

    insert_top_offset(expr_p, get_top_code_len());

    /* Replace placeholder in offset list */
    replace_holder(-1, get_top_code_len());
    replace_holder(-2, update_p);
    break;
  }
  case OUT:
  case GO:
    if (!state.loop_handler) {
      fprintf(stderr, "\033[1;31mcompiler %d:\033[0m Loop control \
statement cannot be used outside loop.\n",
              state.pre.line);
      exit(EXIT_FAILURE);
    }
    token_kind kind = state.pre.kind;
    iter();
    if (state.pre.kind == R_ARROW) {
      emit_top_code(JUMP_TO); /* No condition */
    } else {
      set_precedence(P_LOWEST);
      emit_top_code(T_JUMP_TO); /* Condition */
    }
    /* Placeholder position */
    emit_top_offset(kind == OUT ? -1 : -2);
    break;
  case RET:
    iter();
    if (state.pre.kind == R_ARROW) {
      emit_top_code(TO_RET);
    } else {
      stmt();
      emit_top_code(RET_OF);
    }
    break;
  case USE: {
    token_kind kind = state.pre.kind;
    iter();
    if (state.pre.kind != LITERAL) {
      syntax_error();
    }
    emit_top_name(state.pre.literal);
    emit_top_code(USE_MOD);
    break;
  }
  default:
    set_precedence(P_LOWEST); /* Expression */
  }
}

/* Compile block statement of the same level */
void block() {
  token *tok = (token *)state.tokens->data[state.p - 1];
  int off = state.pre.off;
  /* Block */
  if (off <= tok->off) {
    no_block_error();
  }
  while (true) {
    stmt();                     /* Single statement */
    if (state.cur.off == off) { /* Loop compilation multiple statements */
      iter();
    } else {
      break;
    }
  }
}

/* Compiler */
list *compile(list *t) {
  p = 0; /* Set state */

  state.tokens = t;
  state.codes = NULL;
  state.p = 0;
  clear_state();

  both_iter();

  /* Push global object */
  code_object *code = new_code("main");
  push_code_list(code);

  while (state.pre.kind != EOH) {
    stmt();
    iter();
    state.loop_handler = false;
  }

  emit_top_code(TO_RET); /* End compile */
  return state.codes;
}

/* Detailed information */
void dissemble(code_object *code) {
  printf("<%s>: %d code, %d name, %d type, %d object, %d offset\n",
         code->description, code->codes == NULL ? 0 : code->codes->len,
         code->names == NULL ? 0 : code->names->len,
         code->types == NULL ? 0 : code->types->len,
         code->objects == NULL ? 0 : code->objects->len,
         code->offsets == NULL ? 0 : code->offsets->len);

  // for (int i = 0; i < code->offsets->len; i ++) printf("%d\n", *(int16_t
  // *)code->offsets->data[i]);

  for (int b = 0, p = 0, pl = -1; b < code->codes->len; b++) {
    int line = *(int *)code->lines->data[b];
    if (line != pl) {
      // if (pl != -1)
      //     printf("\n");
      printf("L%-4d", *(int *)code->lines->data[b]);
      pl = line;
    } else {
      printf("%-5s", " ");
    }

    op_code *inner = (op_code *)code->codes->data[b];
    printf("%2d %10s", b, code_string[*inner]);
    switch (*inner) {
    case CONST_OF:
    case LOAD_ENUM:
    case LOAD_FUNC:
    case LOAD_FACE:
    case LOAD_WHOLE: {
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
      printf("%4d %s %d '%s'\n", *x, type_string((type *)code->types->data[*x]),
             *y, (char *)code->names->data[*y]);
      p += 2;
      break;
    }
    case BUILD_ARR:
    case BUILD_TUP:
    case BUILD_MAP: {
      int16_t *count = (int16_t *)code->offsets->data[p++];
      printf("%4d\n", *count);
      break;
    }
    default:
      printf("\n");
    }
  }

  /* Output the details of the objects in it */
  if (code->objects != NULL) {
    for (int i = 0; i < code->objects->len; i++) {
      object *obj = (object *)code->objects->data[i];
      /* Dissemble code object */
      if (obj->kind == OBJ_FUNC) {
        /* printf("[Func %s]: %d arg, %s ret\n",
            obj->value.func.name,
            obj->value.func.k->len,
            type_string(obj->value.func.ret)); */
        dissemble(obj->value.func.code);
      } else if (obj->kind == OBJ_WHOLE) {
        dissemble(obj->value.whole.code);
      }
    }
  }
}