/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#include "vm.h"

extern keg *lexer(const char *, int);
extern keg *compile(keg *);

vm_state vst;

static void *dl_handle = NULL;
static char *dl_error = NULL;

static keg *c_func = NULL;
static keg *c_mods = NULL;

bool find_cmod_var = false;

object *get_cfunc(char *name) {
    if (c_func == NULL) {
        return NULL;
    }
    for (int i = 0; i < c_func->item; i++) {
        object *obj = c_func->data[i];
        if (strcmp(obj->value.cf.name, name) == 0) {
            return obj;
        }
    }
    return NULL;
}

object *get_cmods(char *name) {
    if (c_mods == NULL) {
        return NULL;
    }
    for (int i = 0; i < c_mods->item; i++) {
        object *obj = c_mods->data[i];
        if (strcmp(obj->value.cm.name, name) == 0) {
            return obj;
        }
    }
    return NULL;
}

object *get_cmods_member(object *mod, char *name) {
    keg *a = mod->value.cm.var;
    keg *b = mod->value.cm.met;

    for (int i = 0; i < a->item; i++) {
        addr_kv *kv = a->data[i];

        if (strcmp(kv->name, name) == 0) {
            void (*fn)() = kv->ptr;
            fn();
            find_cmod_var = true;
            return NULL;
        }
    }
    for (int i = 0; i < b->item; i++) {
        object *obj = b->data[i];
        if (strcmp(obj->value.cf.name, name) == 0) {
            return obj;
        }
    }
    return NULL;
}

frame *new_frame(code_object *code) {
    frame *f = malloc(sizeof(frame));
    f->code = code;
    f->data = new_keg();
    f->ret = NULL;
    f->tb = new_table();
    f->tp = new_table();
    return f;
}

void free_frame(frame *f) {
    printf("free GC\n");
}

void free_tokens(keg *g) {
    for (int i = 0; i < g->item; i++) {
        token *tok = (token *)g->data[i];
        if (tok->kind == FLOAT || tok->kind == NUMBER || tok->kind == CHAR ||
            tok->kind >= 40) {
            free(tok->literal);
        }
        free(tok);
    }
    free_keg(g);
}

#define BACK_FRAME  (frame *)back_keg(vst.frame)
#define MAIN_FRAME  (frame *)vst.frame->data[0]

#define TOP_TB      (BACK_FRAME)->tb
#define TOP_TP      (BACK_FRAME)->tp
#define TOP_CODE    (BACK_FRAME)->code
#define TOP_DATA    (BACK_FRAME)->data

#define PUSH(obj)   TOP_DATA = append_keg(TOP_DATA, obj)
#define POP         (object *)pop_back_keg(TOP_DATA)

#define GET_OFF     *(int16_t *)TOP_CODE->offsets->data[vst.op++]
#define GET_NAME    (char *)TOP_CODE->names->data[GET_OFF]
#define GET_TYPE    (type *)TOP_CODE->types->data[GET_OFF]
#define GET_OBJ     (object *)TOP_CODE->objects->data[GET_OFF]
#define GET_LINE    *(int *)TOP_CODE->lines->data[vst.ip]
#define GET_CODE    *(uint8_t *)TOP_CODE->codes->data[vst.ip]

#define GET_PR_CODE *(uint8_t *)TOP_CODE->codes->data[vst.ip - 2]
#define GET_PR_OBJ \
    (object *)TOP_CODE->objects \
        ->data[(*(int16_t *)TOP_CODE->offsets->data[vst.op - 1])]

void type_error(type *T, object *obj) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m expect type %s, but it's %s.\n",
        GET_LINE, type_string(T), obj_string(obj));
    exit(EXIT_SUCCESS);
}

void check_type(type *T, object *obj) {
    if (!type_checker(T, obj)) {
        type_error(T, obj);
    }
}

void undefined_error(char *name) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m undefined name '%s'.\n", GET_LINE,
        name);
    exit(EXIT_SUCCESS);
}

void unsupport_operand_error(const char *op) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m unsupport operand to '%s'.\n",
        GET_LINE, op);
    exit(EXIT_SUCCESS);
}

void error(const char *msg) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m %s.\n", GET_LINE, msg);
    exit(EXIT_SUCCESS);
}

void bt_simple_error(const char *param) {
    fprintf(stderr,
        "\033[1;31mvm %d:\033[0m wrong parameter of built-in function '%s'.\n",
        GET_LINE, param);
    exit(EXIT_SUCCESS);
}

void bt_println(keg *arg) {
    for (int i = arg->item - 1; i >= 0; i--) {
        object *p = arg->data[i];
        printf("%s\t", obj_raw_string(p, arg->item > 1));
    }
    printf("\n");
}

void bt_print(keg *arg) {
    for (int i = arg->item - 1; i >= 0; i--) {
        object *p = arg->data[i];
        printf("%s\t", obj_raw_string(p, arg->item > 1));
    }
}

void bt_putline(keg *arg) {
    for (int i = arg->item - 1; i >= 0; i--) {
        object *p = arg->data[i];
        printf("%s\n", obj_raw_string(p, arg->item > 1));
    }
}

void bt_put(keg *arg) {
    object *obj = pop_back_keg(arg);
    if (obj == NULL || arg->item != 0) {
        bt_simple_error("put(obj any)");
    }
    printf("%s", obj_raw_string(obj, false));
}

void bt_len(keg *arg) {
    object *obj = pop_back_keg(arg);
    if (obj == NULL || arg->item != 0) {
        bt_simple_error("len(obj any)");
    }
    object *len = malloc(sizeof(object));
    len->kind = OBJ_INT;
    len->value.num = obj_len(obj);
    PUSH(len);
}

void bt_type(keg *arg) {
    object *obj = pop_back_keg(arg);
    if (obj == NULL || arg->item != 0) {
        bt_simple_error("type(obj any)");
    }
    object *str = malloc(sizeof(object));
    str->kind = OBJ_STRING;
    str->value.str = (char *)obj_type_string(obj);
    PUSH(str);
}

void bt_sleep(keg *arg) {
    object *obj = pop_back_keg(arg);
    if (obj == NULL || obj->kind != OBJ_INT || arg->item != 0) {
        bt_simple_error("sleep(milliseconds int)");
    }
#if defined(__linux__) || defined(__APPLE__)
    sleep(obj->value.num / 1000);
#elif defined(_WIN32)
    Sleep(obj->value.num);
#endif
}

void bt_rand_int(keg *arg) {
    object *b = pop_back_keg(arg);
    object *a = pop_back_keg(arg);
    if (a == NULL || b == NULL || a->kind != OBJ_INT || b->kind != OBJ_INT ||
        arg->item != 0) {
        bt_simple_error("rand(start, end int)");
    }
    int x = b->value.num;
    int y = a->value.num;

    object *obj = malloc(sizeof(object));
    obj->kind = OBJ_INT;

    if (x == 0 && y == 0) {
        obj->value.num = -1;
    } else {
#include <sys/time.h>
#include <time.h>
        struct timeval stamp;
        gettimeofday(&stamp, NULL);
        srand(stamp.tv_usec);
        obj->value.num = rand() % y + x;
    }
    PUSH(obj);
}

void bt_append_entry(keg *arg) {
    object *arr = pop_back_keg(arg);
    object *val = pop_back_keg(arg);
    if (arr == NULL || arr->kind != OBJ_ARRAY || val == NULL ||
        arg->item != 0) {
        bt_simple_error("append(arr []any, new any)");
    }
    type *T = arr->value.arr.T;
    check_type(T, val);
    append_keg(arr->value.arr.element, val);
}

void bt_remove_entry(keg *arg) {
    object *arr = pop_back_keg(arg);
    object *idx = pop_back_keg(arg);
    if (arr == NULL || arr->kind != OBJ_ARRAY || idx == NULL ||
        idx->kind != OBJ_INT || arg->item != 0) {
        bt_simple_error("remove(arr []any, index int)");
    }
    int p = idx->value.num;
    keg *elem = arr->value.arr.element;
    if (elem->item == 0) {
        error("empty array can not to remove entry");
    }
    if (p < 0 || p > elem->item - 1) {
        error("index out of bounds");
    }
    remove_keg(elem, p);
}

void bt_input(keg *arg) {
    if (arg->item != 0) {
        bt_simple_error("input()");
    }
    char *literal = malloc(sizeof(char) * 32);
    fgets(literal, 32, stdin);
    literal[strlen(literal) - 1] = '\0';
    object *obj = malloc(sizeof(object));
    obj->kind = OBJ_STRING;
    obj->value.str = literal;
    PUSH(obj);
}

object *bt_true() {
    object *obj = malloc(sizeof(object));
    obj->kind = OBJ_BOOL;
    obj->value.b = true;
    return obj;
}

object *bt_false() {
    object *obj = malloc(sizeof(object));
    obj->kind = OBJ_BOOL;
    obj->value.b = false;
    return obj;
}

builtin bts[BUILTIN_COUNT] = {
    {"println", BU_FUNCTION, bt_println     },
    {"print",   BU_FUNCTION, bt_print       },
    {"putline", BU_FUNCTION, bt_putline     },
    {"put",     BU_FUNCTION, bt_put         },
    {"len",     BU_FUNCTION, bt_len         },
    {"type",    BU_FUNCTION, bt_type        },
    {"sleep",   BU_FUNCTION, bt_sleep       },
    {"rand",    BU_FUNCTION, bt_rand_int    },
    {"append",  BU_FUNCTION, bt_append_entry},
    {"remove",  BU_FUNCTION, bt_remove_entry},
    {"input",   BU_FUNCTION, bt_input       },
    {"true",    BU_NAME,     bt_true        },
    {"false",   BU_NAME,     bt_false       }
};

object *new_builtin(char *name, builtin_kind kind, void *p) {
    object *obj = malloc(sizeof(object));
    obj->kind = OBJ_BUILTIN;
    obj->value.bu.kind = kind;
    obj->value.bu.name = name;
    obj->value.bu.func = p;
    return obj;
}

object *make_nil() {
    object *obj = malloc(sizeof(object));
    obj->kind = OBJ_NIL;
    return obj;
}

object *get_builtin(char *name) {
    int i;
    for (i = 0; i < BUILTIN_COUNT; i++) {
        if (strcmp(bts[i].name, name) == 0) {
            break;
        }
    }
    if (i != BUILTIN_COUNT) {
        builtin b = bts[i];
        if (b.kind == BU_FUNCTION) {
            return new_builtin(name, b.kind, b.func);
        }
        if (b.kind == BU_NAME) {
            object *(*call)() = b.func;
            return call();
        }
    }
    return NULL;
}

void jump(int16_t to) {
    vst.op--;
    bool reverse = vst.ip > to;
    if (reverse) {
        vst.ip--;
    }
    while (reverse ? vst.ip >= to : vst.ip < to) {
        switch (GET_CODE) {
        case CONST_OF:
        case LOAD_OF:
        case ENUMERATE:
        case FUNCTION:
        case INTERFACE:
        case ASSIGN_TO:
        case GET_OF:
        case SET_OF:
        case CALL_FUNC:
        case SET_NAME:
        case USE_MOD:
        case USE_IN_MOD:
        case BUILD_ARR:
        case BUILD_TUP:
        case BUILD_MAP:
        case JUMP_TO:
        case T_JUMP_TO:
        case F_JUMP_TO:
        case NEW_OBJ:
        case REF_MODULE:
            reverse ? vst.op-- : vst.op++;
            break;
        case STORE_NAME:
            if (reverse)
                vst.op -= 2;
            else
                vst.op += 2;
            break;
        }
        reverse ? vst.ip-- : vst.ip++;
    }
    if (!reverse) {
        vst.ip--;
    }
}

void *lookup(char *name) {
    void *p = get_table(TOP_TB, name);
    if (p != NULL) {
        return p;
    }
    frame *f = (frame *)back_keg(vst.call);
    p = get_table(f->tb, name);
    if (p != NULL) {
        return p;
    }
    f = (frame *)vst.call->data[0];
    p = get_table(f->tb, name);
    if (p != NULL) {
        return p;
    }
    p = get_builtin(name);
    if (p != NULL) {
        return p;
    }
    p = get_cfunc(name);
    if (p != NULL) {
        return p;
    }
    p = get_cmods(name);
    if (p != NULL) {
        return p;
    }
    return NULL;
}

void load_module(char *, char *, bool);

void check_interface(object *in, object *cl) {
    if (cl->value.cl.init == false) {
        error("class is not initialized");
    }

    keg *elem = in->value.in.element;
    frame *f = (frame *)cl->value.cl.fr;

    for (int i = 0; i < elem->item; i++) {
        method *m = elem->data[i];

        void *p = get_table(f->tb, m->name);
        if (p == NULL) {
            error("class does not contain some member");
        }
        object *fn = p;
        if (fn->kind != OBJ_FUNCTION) {
            error("an interface can only be a method");
        }
        if (!type_eq(m->ret, fn->value.fn.ret)) {
            error("return type in the method are inconsistent");
        }
        if (m->arg->item != fn->value.fn.v->item) {
            error("inconsistent method arguments");
        }
        for (int j = 0; j < m->arg->item; j++) {
            type *a = m->arg->data[j];
            type *b = fn->value.fn.v->data[j];

            if (!type_eq(a, b)) {
                error("inconsistent types of method arguments");
            }
        }
    }
}

void check_generic(generic *ge, object *obj) {
    if (ge->count == 1) {
        if (type_checker(ge->mtype.T, obj)) {
            return;
        }
    } else {
        keg *g = ge->mtype.multiple;
        for (int i = 0; i < g->item; i++) {
            if (type_checker(g->data[i], obj)) {
                return;
            }
        }
    }
    error("generic type error");
}

generic *exist_generic(keg *gt, char *tpname) {
    for (int i = 0; i < gt->item; i++) {
        generic *g = (generic *)((type *)gt->data[i])->inner.ge;
        if (strcmp(g->name, tpname) == 0) {
            return g;
        }
    }
    return NULL;
}

void check_set(object *origin, object *new) {
    if (new->kind != origin->kind && origin->kind != OBJ_NIL) {
        error("wrong type set");
    }
    if (!obj_kind_eq(new, origin)) {
        error("inconsistent type");
    }
}

void eval() {
    while (vst.ip < TOP_CODE->codes->item) {
        uint8_t code = GET_CODE;
        switch (code) {
        case CONST_OF: {
            PUSH(GET_OBJ);
            break;
        }
        case STORE_NAME: {
            type *T = GET_TYPE;
            char *name = GET_NAME;
            object *obj = POP;
            if (T->kind == T_BOOL && obj->kind == OBJ_INT) {
                obj->value.b = obj->value.num > 0;
                obj->kind = OBJ_BOOL;
            }
            if (T->kind == T_USER) {
                type *tp = get_table(TOP_TP, T->inner.name);

                if (tp != NULL && tp->kind == T_GENERIC) {
                    check_generic((generic *)tp->inner.ge, obj);
                    T = tp;
                } else {
                    if (obj->kind == OBJ_CLASS) {
                        void *p = lookup(T->inner.name);
                        if (p == NULL) {
                            error("undefined interface or class");
                        }
                        object *in = p;
                        if (in->kind == OBJ_INTERFACE) {
                            check_interface(in, obj);
                            in->value.in.class = (struct object *)obj;
                            obj = in;
                        }
                    }
                }
            } else {
                check_type(T, obj);
            }

            switch (obj->kind) {
            case OBJ_ARRAY:
                obj->value.arr.T = (type *)T->inner.single;
                break;
            case OBJ_TUPLE:
                obj->value.tup.T = (type *)T->inner.single;
                break;
            case OBJ_MAP:
                obj->value.map.T1 = (type *)T->inner.both.T1;
                obj->value.map.T2 = (type *)T->inner.both.T2;
                break;
            }

            object *new = obj;
            if (copy_type(T)) {
                new = malloc(sizeof(object));
                memcpy(new, obj, sizeof(object));
            }

            add_table(TOP_TB, name, new);
            add_table(TOP_TP, name, T);
            break;
        }
        case LOAD_OF: {
            char *name = GET_NAME;
            void *ptr = lookup(name);
            if (ptr == NULL) {
                undefined_error(name);
            }
            PUSH(ptr);
            break;
        }
        case ASSIGN_TO: {
            char *name = GET_NAME;
            object *obj = POP;
            void *p = lookup(name);
            if (p == NULL) {
                undefined_error(name);
            }
            object *origin = p;
            if (!obj_kind_eq(origin, obj)) {
                error("inconsistent type");
            }
            if (origin->kind == OBJ_INTERFACE) {
                if (obj->kind != OBJ_CLASS) {
                    error("interface needs to be assigned by class");
                }
                check_interface(origin, obj);
                origin->value.in.class = (struct object *)obj;
                break;
            }
            add_table(TOP_TB, name, obj);
            break;
        }
        case TO_ADD:
        case TO_SUB:
        case TO_MUL:
        case TO_DIV:
        case TO_SUR:
        case TO_GR:
        case TO_GR_EQ:
        case TO_LE:
        case TO_LE_EQ:
        case TO_EQ_EQ:
        case TO_NOT_EQ:
        case TO_AND:
        case TO_OR: {
            object *b = POP;
            object *a = POP;
            if (a->kind == OBJ_STRING && b->kind == OBJ_STRING) {
                int la = strlen(a->value.str);
                int lb = strlen(b->value.str);
                if (la + lb > STRING_EVAL_MAX) {
                    error("number of characters is greater "
                          "than 128-bit bytes");
                }
            }
            PUSH(binary_op(code, a, b));
            break;
        }
        case BUILD_ARR: {
            int16_t item = GET_OFF;
            object *obj = malloc(sizeof(object));
            obj->kind = OBJ_ARRAY;
            obj->value.arr.element = new_keg();
            if (item == 0) {
                PUSH(obj);
                break;
            }
            while (item > 0) {
                insert_keg(obj->value.arr.element, 0, POP);
                item--;
            }
            PUSH(obj);
            break;
        }
        case BUILD_TUP: {
            int16_t item = GET_OFF;
            object *obj = malloc(sizeof(object));
            obj->kind = OBJ_TUPLE;
            obj->value.tup.element = new_keg();
            if (item == 0) {
                PUSH(obj);
                break;
            }
            while (item > 0) {
                insert_keg(obj->value.tup.element, 0, POP);
                item--;
            }
            PUSH(obj);
            break;
        }
        case BUILD_MAP: {
            int16_t item = GET_OFF;
            object *obj = malloc(sizeof(object));
            obj->kind = OBJ_MAP;
            obj->value.map.k = new_keg();
            obj->value.map.v = new_keg();
            if (item == 0) {
                PUSH(obj);
                break;
            }
            while (item > 0) {
                object *b = POP;
                object *a = POP;
                append_keg(obj->value.map.k, a);
                append_keg(obj->value.map.v, b);
                item -= 2;
            }
            PUSH(obj);
            break;
        }
        case TO_INDEX: {
            object *p = POP;
            object *obj = POP;
            if (obj->kind != OBJ_ARRAY && obj->kind != OBJ_TUPLE &&
                obj->kind != OBJ_STRING && obj->kind != OBJ_MAP) {
                error("only array, tuple, string and map can be called "
                      "with subscipt");
            }
            if (obj->kind == OBJ_ARRAY) {
                if (p->kind != OBJ_INT) {
                    error("get value using integer subscript");
                }
                keg *elem = obj->value.arr.element;

                if (elem->item == 0 || p->value.num >= elem->item) {
                    PUSH(make_nil());
                    break;
                }
                PUSH(elem->data[p->value.num]);
            }
            if (obj->kind == OBJ_TUPLE) {
                if (p->kind != OBJ_INT) {
                    error("get value using integer subscript");
                }
                keg *elem = obj->value.tup.element;

                if (elem->item == 0 || p->value.num >= elem->item) {
                    PUSH(make_nil());
                    break;
                }
                PUSH(obj->value.tup.element->data[p->value.num]);
            }
            if (obj->kind == OBJ_MAP) {
                if (obj->value.map.k->item == 0) {
                    error("map entry is empty");
                }
                int i = 0;
                for (; i < obj->value.map.k->item; i++) {
                    if (obj_eq(p, obj->value.map.k->data[i])) {
                        PUSH(obj->value.map.v->data[i]);
                        break;
                    }
                }
                if (i == obj->value.map.k->item) {
                    PUSH(make_nil());
                }
            }
            if (obj->kind == OBJ_STRING) {
                if (p->kind != OBJ_INT) {
                    error("get value using integer subscript");
                }
                int len = strlen(obj->value.str);
                if (len == 0 || p->value.num >= len) {
                    PUSH(make_nil());
                    break;
                }
                object *ch = malloc(sizeof(object));
                ch->kind = OBJ_CHAR;
                ch->value.c = obj->value.str[p->value.num];
                PUSH(ch);
            }
            break;
        }
        case TO_REPLACE: {
            object *obj = POP;
            object *idx = POP;
            object *j = POP;
            if (j->kind != OBJ_ARRAY && j->kind != OBJ_MAP) {
                error("only array and map types can be set");
            }
            if (j->kind == OBJ_ARRAY) {
                if (idx->kind != OBJ_INT) {
                    error("get value using integer subscript");
                }
                check_type(j->value.arr.T, obj);
                int p = idx->value.num;
                if (j->value.arr.element->item == 0) {
                    insert_keg(j->value.arr.element, 0, obj);
                } else {
                    if (p > j->value.arr.element->item - 1) {
                        error("index out of bounds");
                    }
                    replace_keg(j->value.arr.element, p, obj);
                }
            }
            if (j->kind == OBJ_MAP) {
                check_type(j->value.map.T1, idx);
                check_type(j->value.map.T2, obj);
                int p = -1;
                for (int i = 0; i < j->value.map.k->item; i++) {
                    if (obj_eq(idx, j->value.map.k->data[i])) {
                        p = i;
                        break;
                    }
                }
                if (p == -1) {
                    insert_keg(j->value.map.k, 0, idx);
                    insert_keg(j->value.map.v, 0, obj);
                } else {
                    replace_keg(j->value.map.v, p, obj);
                }
            }
            break;
        }
        case TO_BANG: {
            object *obj = POP;
            object *new = malloc(sizeof(object));
            switch (obj->kind) {
            case OBJ_INT:
                new->value.b = !obj->value.num;
                break;
            case OBJ_FLOAT:
                new->value.b = !obj->value.f;
                break;
            case OBJ_CHAR:
                new->value.b = !obj->value.c;
                break;
            case OBJ_STRING:
                new->value.b = !strlen(obj->value.str);
                break;
            case OBJ_BOOL:
                new->value.b = !obj->value.b;
                break;
            default:
                new->value.b = false;
            }
            new->kind = OBJ_BOOL;
            PUSH(new);
            break;
        }
        case TO_NOT: {
            object *obj = POP;
            object *new = malloc(sizeof(object));
            if (obj->kind == OBJ_INT) {
                new->value.num = -obj->value.num;
                new->kind = OBJ_INT;
                PUSH(new);
                break;
            }
            if (obj->kind == OBJ_FLOAT) {
                new->value.f = -obj->value.f;
                new->kind = OBJ_FLOAT;
                PUSH(new);
                break;
            }
            unsupport_operand_error(code_string[code]);
            break;
        }
        case JUMP_TO:
        case F_JUMP_TO:
        case T_JUMP_TO: {
            int16_t off = GET_OFF;
            if (code == JUMP_TO) {
                jump(off);
                break;
            }
            bool ok = (POP)->value.b;
            if (code == T_JUMP_TO && ok) {
                jump(off);
            }
            if (code == F_JUMP_TO && ok == false) {
                jump(off);
            }
            break;
        }
        case FUNCTION: {
            object *obj = GET_OBJ;
            add_table(TOP_TB, obj->value.fn.name, obj);
            break;
        }
        case CALL_FUNC: {
            int16_t off = GET_OFF;
            keg *arg = new_keg();
            while (off > 0) {
                if (TOP_DATA->item - 1 <= 0) {
                    error("stack overflow!");
                }
                append_keg(arg, POP);
                off--;
            }

            object *fn = POP;

            if (fn->kind == OBJ_CFUNC) {
                fn->value.cf.func(arg);
                goto next;
            }
            if (fn->kind == OBJ_BUILTIN) {
                void (*call)(keg *) = fn->value.bu.func;
                call(arg);
                goto next;
            }
            if (fn->kind != OBJ_FUNCTION) {
                error("i don't known what was called");
            }

            keg *k = fn->value.fn.k;
            keg *v = fn->value.fn.v;

            if ((fn->value.fn.mutiple != NULL && k->item != 1 &&
                    arg->item < k->item) ||
                (fn->value.fn.mutiple == NULL && k->item != arg->item)) {
                error("inconsistent funtion arguments");
            }

            frame *f = new_frame(fn->value.fn.code);
            keg *gt = fn->value.fn.gt;

            for (int i = 0; i < k->item; i++) {
                char *name = k->data[i];
                object *obj = NULL;
                generic *ge = NULL;

                if (v->item == i && fn->value.fn.mutiple != NULL) {
                    type *T = fn->value.fn.mutiple;
                    object *a = malloc(sizeof(object));
                    a->kind = OBJ_ARRAY;
                    a->value.arr.element = NULL;

                    if (T->kind == T_USER) {
                        ge = exist_generic(gt, T->inner.name);
                    }
                    if (arg->item == 0) {
                        a->value.arr.element = new_keg();
                    } else {
                        while (arg->item > 0) {
                            object *p = arg->data[--arg->item];

                            if (ge != NULL) {
                                check_generic(ge, p);
                                ge = NULL;
                            } else {
                                check_type(T, p);
                            }
                            a->value.arr.element =
                                append_keg(a->value.arr.element, p);
                        }
                    }
                    obj = a;
                } else {
                    type *T = v->data[i];

                    if (T->kind == T_USER) {
                        ge = exist_generic(gt, T->inner.name);
                    }
                    object *p = arg->data[--arg->item];
                    if (ge != NULL) {
                        check_generic(ge, p);
                        ge = NULL;
                    } else {
                        check_type(T, p);
                    }
                    obj = p;
                }
                add_table(f->tb, name, obj);
            }

            if (fn->value.fn.self != NULL) {
                vst.call = append_keg(vst.call, fn->value.fn.self);
            }

            int16_t op_up = vst.op;
            int16_t ip_up = vst.ip;

            vst.op = 0;
            vst.ip = 0;
            vst.frame = append_keg(vst.frame, f);
            eval();

            frame *p = pop_back_keg(vst.frame);

            if (fn->value.fn.ret != NULL) {
                if (p->ret == NULL || !type_checker(fn->value.fn.ret, p->ret)) {
                    if (p->ret == NULL) {
                        error("function missing return value");
                    }
                    type_error(fn->value.fn.ret, p->ret);
                }
                PUSH(p->ret);
            }

            vst.op = op_up;
            vst.ip = ip_up;

            if (fn->value.fn.self != NULL) {
                pop_back_keg(vst.call);
            }
            break;
        }
        case INTERFACE: {
            object *obj = GET_OBJ;
            add_table(TOP_TB, obj->value.in.name, obj);
            break;
        }
        case ENUMERATE: {
            object *obj = GET_OBJ;
            add_table(TOP_TB, obj->value.en.name, obj);
            break;
        }
        case GET_OF: {
            char *name = GET_NAME;
            object *obj = POP;
            if (obj->kind != OBJ_ENUMERATE && obj->kind != OBJ_CLASS &&
                obj->kind != OBJ_INTERFACE) {
                error("only enum, interface and class type are supported");
            }
            if (obj->kind == OBJ_ENUMERATE) {
                keg *elem = obj->value.en.element;

                object *p = malloc(sizeof(object));
                p->kind = OBJ_INT;
                p->value.num = -1;
                for (int i = 0; i < elem->item; i++) {
                    if (strcmp(name, (char *)elem->data[i]) == 0) {
                        p->value.num = i;
                        break;
                    }
                }
                PUSH(p);
            }
            if (obj->kind == OBJ_CLASS) {
                if (obj->value.cl.init == false) {
                    error("class did not load initialization members");
                }
                frame *fr = (frame *)obj->value.cl.fr;
                void *ptr = get_table(fr->tb, name);
                if (ptr == NULL) {
                    error("nonexistent member");
                }
                object *val = ptr;
                if (val->kind == OBJ_FUNCTION) {
                    val->value.fn.self = obj->value.cl.fr;
                }
                PUSH(ptr);
            }
            if (obj->kind == OBJ_INTERFACE) {
                if (obj->value.in.class == NULL) {
                    error("interface is not initialized");
                }
                keg *elem = obj->value.in.element;
                for (int i = 0; i < elem->item; i++) {
                    method *m = elem->data[i];
                    if (strcmp(m->name, name) == 0) {
                        object *cl = (object *)obj->value.in.class;
                        frame *fr = (frame *)cl->value.cl.fr;
                        object *val = get_table(fr->tb, name);

                        if (val->kind == OBJ_FUNCTION) {
                            val->value.fn.self = fr;
                        }
                        PUSH(val);
                        goto next;
                    }
                }
                error("nonexistent member");
            }
            break;
        }
        case GET_IN_OF: {
            char *name = GET_NAME;
            if (vst.call->item < 2) {
                error("need to use this statement in the class");
            }
            void *ptr = lookup(name);
            if (ptr == NULL) {
                error("nonexistent member");
            }
            PUSH(ptr);
            break;
        }
        case SET_OF: {
            char *name = GET_NAME;
            object *val = POP;
            object *obj = POP;
            if (obj->kind != OBJ_CLASS) {
                error("only members of class can be set");
            }
            frame *fr = (frame *)obj->value.cl.fr;
            object *ptr = get_table(fr->tb, name);
            if (ptr == NULL) {
                error("nonexistent member");
            }
            check_set(ptr, val);
            add_table(fr->tb, name, val);
            break;
        }
        case REF_MODULE: {
            char *name = GET_NAME;
            object *obj = POP;
            if (obj->kind != OBJ_MODULE && obj->kind != OBJ_CMODS) {
                error("can only be used as a member reference of a module");
            }
            void *ptr = NULL;
            if (obj->kind == OBJ_MODULE) {
                ptr = get_table((table *)obj->value.mod.tb, name);
            }
            if (obj->kind == OBJ_CMODS) {
                ptr = get_cmods_member(obj, name);
            }
            if (find_cmod_var) {
                find_cmod_var = false;
                break;
            }
            if (ptr == NULL) {
                undefined_error(name);
            }
            PUSH(ptr);
            break;
        }
        case REF_SET: {
            char *name = GET_NAME;
            object *val = POP;
            object *obj = POP;
            if (obj->kind != OBJ_MODULE) {
                error("module members can only be set");
            }
            table *tb = (table *)obj->value.mod.tb;
            object *ptr = get_table(tb, name);
            if (ptr == NULL) {
                error("nonexistent member");
            }
            check_set(ptr, val);
            add_table(tb, name, val);
            break;
        }
        case CLASS: {
            object *obj = GET_OBJ;
            add_table(TOP_TB, obj->value.cl.name, obj);
            break;
        }
        case NEW_OBJ: {
            int16_t arg = GET_OFF;

            keg *k = NULL;
            keg *v = NULL;

            while (arg > 0) {
                v = append_keg(v, POP);
                k = append_keg(k, POP);
                arg -= 2;
            }

            object *obj = POP;
            if (obj->kind != OBJ_CLASS) {
                error("only class object can be created");
            }

            object *new = malloc(sizeof(object));
            memcpy(new, obj, sizeof(object));

            frame *f = new_frame(obj->value.cl.code);

            new->value.cl.fr = (struct frame *)f;
            keg *gt = new->value.cl.gt;
            int i = 0;

            for (; i < gt->item; i++) {
                type *T = (type *)gt->data[i];
                add_table(f->tp, ((generic *)T->inner.ge)->name, T);
            }

            int16_t op_up = vst.op;
            int16_t ip_up = vst.ip;

            vst.op = 0;
            vst.ip = 0;
            vst.frame = append_keg(vst.frame, new->value.cl.fr);
            eval();

            pop_back_keg(vst.frame);
            while (i > 0) {
                remove_keg(f->tp->name, 0);
                remove_keg(f->tp->value, 0);
                i--;
            }

            vst.op = op_up;
            vst.ip = ip_up;

            if (k != NULL) {
                for (int i = 0; i < k->item; i++) {
                    char *key = ((object *)k->data[i])->value.str;
                    object *obj = v->data[i];
                    type *T = get_table(f->tp, key);
                    if (T == NULL) {
                        undefined_error(key);
                    }
                    if (T->kind == T_GENERIC) {
                        check_generic((generic *)T->inner.ge, obj);
                    } else {
                        check_type(T, obj);
                    }
                    add_table(f->tb, key, obj);
                }
            }

            new->value.cl.init = true;
            PUSH(new);
            break;
        }
        case SET_NAME: {
            char *name = GET_NAME;
            object *n = malloc(sizeof(object));
            n->kind = OBJ_STRING;
            n->value.str = name;
            PUSH(n);
            break;
        }
        case TO_RET:
        case RET_OF: {
            vst.ip = TOP_CODE->codes->item;
            vst.loop_ret = true;
            if (code == RET_OF) {
                if (GET_PR_CODE == FUNCTION) {
                    (BACK_FRAME)->ret = GET_PR_OBJ;
                } else {
                    (BACK_FRAME)->ret = POP;
                }
            }
            break;
        }
        case USE_MOD:
        case USE_IN_MOD: {
            int16_t count = GET_OFF;
            keg *cap = new_keg();
            bool internal = code == USE_IN_MOD;
            while (count > 0) {
                insert_keg(cap, 0, (POP)->value.str);
                count--;
            }
            if (cap->item == 1) {
                load_module(cap->data[0], NULL, internal);
            } else {
                char *path = malloc(sizeof(char) * STRING_PATH_MAX);
                memset(path, 0, STRING_PATH_MAX);
                for (int i = 0; i < cap->item - 1; i++) {
                    strcat(path, cap->data[i]);
                    strcat(path, "/");
                }
                load_module(cap->data[cap->item - 1], path, internal);
            }
            break;
        }
        default: {
            fprintf(stderr, "\033[1;31mvm %d:\033[0m unreachable '%s'.\n",
                GET_LINE, code_string[code]);
            exit(EXIT_SUCCESS);
        }
        }
    next:
        vst.ip++;
    }
}

void load_dl(const char *path) {
    dl_handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (!dl_handle || (dl_error = dlerror()) != NULL) {
        printf("%s\n", dl_error);
    }
    void (*init)() = dlsym(dl_handle, "init");
    if ((dl_error = dlerror()) != NULL) {
        printf("%s\n", dl_error);
    }
    init();
}

keg *read_path(char *path, char b, char a);

vm_state evaluate(code_object *code, char *filename) {
    vst.frame = new_keg();
    vst.ip = 0;
    vst.op = 0;
    vst.filename = filename;
    vst.call = new_keg();

    frame *main = new_frame(code);
    vst.frame = append_keg(vst.frame, main);
    vst.call = append_keg(vst.call, main);

    eval();

    return vst;
}

char *get_filename(const char *p) {
    char *name = malloc(sizeof(char) * 64);
    int j = 0;
    for (int i = 0; p[i]; i++) {
        if (p[i] == '/') {
            j = i + 1;
        }
    }
    strcpy(name, &p[j]);
    return name;
}

bool filename_eq(char *a, char *b) {
    int i = 0;
    for (; a[i] != '.'; i++) {
        if (a[i] == '\0' || b[i] != a[i]) {
            return false;
        }
    }
    return i == strlen(b);
}

keg *read_path(char *path, char a, char b) {
    DIR *dir;
    struct dirent *p;
    keg *pl = new_keg();
    if ((dir = opendir(path)) == NULL) {
        error("failed to open the directory in module path");
    }
    while ((p = readdir(dir)) != NULL) {
        if (p->d_type == 4 && strcmp(p->d_name, ".") != 0 &&
            strcmp(p->d_name, "..") != 0) {}
        if (p->d_type == 8) {
            char *name = p->d_name;
            int len = strlen(name) - 1;
            if (name[len] == b && name[len - 1] == a && name[len - 2] == '.') {
                char *addr = malloc(sizeof(char) * 64);
                sprintf(addr, "%s/%s", path, name);
                pl = append_keg(pl, addr);
            }
        }
    }
    return pl;
}

void load_eval(const char *path, char *name, bool internal) {
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        printf("\033[1;31mvm %d:\033[0m failed to read buffer of file '%s'\n",
            GET_LINE, path);
        exit(EXIT_SUCCESS);
    }
    fseek(fp, 0, SEEK_END);
    int fsize = ftell(fp);
    rewind(fp);
    char *buf = malloc(fsize + 1);

    fread(buf, sizeof(char), fsize, fp);
    buf[fsize] = '\0';

    int16_t ip_up = vst.ip;
    int16_t op_up = vst.op;

    keg *fr_up = vst.frame;
    keg *cl_up = vst.call;

    keg *tokens = lexer(buf, fsize);
    free(buf);

    keg *codes = compile(tokens);
    vm_state vs = evaluate(codes->data[0], get_filename(path));

    frame *fr = (frame *)vs.frame->data[0];
    table *tb = fr->tb;

    vst.ip = ip_up;
    vst.op = op_up;
    vst.frame = fr_up;
    vst.call = cl_up;

    if (internal) {
        for (int i = 0; i < tb->name->item; i++) {
            add_table(TOP_TB, tb->name->data[i], tb->value->data[i]);
        }
    } else {
        object *obj = malloc(sizeof(object));
        obj->kind = OBJ_MODULE;
        obj->value.mod.tb = (struct table *)tb;
        obj->value.mod.name = name;
        add_table(TOP_TB, name, obj);
    }
    free_keg(codes);

    fclose(fp);
    free_tokens(tokens);
}

void load_module(char *name, char *path, bool internal) {
    bool ok = false;
    if (filename_eq(vst.filename, name)) {
        error("cannot reference itself as a module");
    }
    keg *pl = read_path(path == NULL ? "." : path, 'f', 't');
    for (int i = 0; i < pl->item; i++) {
        char *addr = pl->data[i];
        if (filename_eq(get_filename(addr), name)) {
            load_eval(addr, name, internal);
            ok = true;
            break;
        }
    }
    if (!ok) {
        char *fname = malloc(sizeof(char) * STRING_CAP_MAX);
        sprintf(fname, "%s.so", name);

        keg *pl = NULL;

        char *env = getenv("FTPATH");
        if (env != NULL) {
            pl = read_path(env, 's', 'o');
        } else {
            error("cannot open FTPATH of environment");
        }
        for (int i = 0; i < pl->item; i++) {
            if (strcmp(get_filename(pl->data[i]), fname) == 0) {
                load_dl(pl->data[i]);
                ok = true;
                break;
            }
        }
        free(fname);
    }
    if (!ok) {
        fprintf(stderr, "\033[1;31mvm %d:\033[0m undefined module '%s'.\n",
            GET_LINE, name);
        exit(EXIT_SUCCESS);
    } else {
        free_keg(pl);
    }
}

void reg_c_func(const char *fns[]) {
    for (int i = 0; fns[i] != NULL && i < C_MOD_MEMCOUNT; i++) {
        const char *name = fns[i];
        void (*fn)(keg *) = dlsym(dl_handle, name);

        object *obj = malloc(sizeof(object));
        obj->kind = OBJ_CFUNC;
        obj->value.cf.name = name;
        obj->value.cf.func = fn;

        c_func = append_keg(c_func, obj);
    }
}

void reg_c_mod(const char *mods[]) {
    for (int i = 0; mods[i] != NULL && i < C_MOD_MEMCOUNT; i++) {
        const char *name = mods[i];
        reg_mod *(*fn)() = dlsym(dl_handle, name);
        reg_mod *mod = fn();

        object *obj = malloc(sizeof(object));
        obj->kind = OBJ_CMODS;
        obj->value.cm.name = mod->name;

        obj->value.cm.var = new_keg();
        obj->value.cm.met = new_keg();

        keg *var = obj->value.cm.var;
        keg *met = obj->value.cm.met;

        for (int j = 0; j < mod->i; j++) {
            reg_mem m = mod->member[j];

            if (m.kind == C_VAR) {
                void (*fn)() = dlsym(dl_handle, m.name);

                addr_kv *kv = malloc(sizeof(addr_kv));
                kv->name = m.name;
                kv->ptr = fn;

                var = append_keg(var, kv);
            }
            if (m.kind == C_METHOD) {
                void (*fn)(keg *) = dlsym(dl_handle, m.name);

                object *cf = malloc(sizeof(object));
                cf->kind = OBJ_CFUNC;
                cf->value.cf.name = m.name;
                cf->value.cf.func = fn;

                met = append_keg(met, cf);
            }
        }
        c_mods = append_keg(c_mods, obj);
    }
}

void reg_name(char *name, object *obj) {
    add_table(TOP_TB, name, obj);
}

void push_stack(object *obj) {
    PUSH(obj);
}

void check_c_func_empty(keg *arg, int i) {
    if (arg->item == 0) {
        error("c extension parameter empty");
    }
    if (i >= arg->item) {
        error("index out of bounds arguments");
    }
}

void check_c_func_error(char *require, object *obj) {
    fprintf(stderr,
        "\033[1;31mvm %d:\033[0m c extension parameter require %s but it's "
        "%s.\n",
        GET_LINE, require, obj_string(obj));
    exit(EXIT_SUCCESS);
}

enum check_c_type { CC_INT, CC_FLOAT, CC_STR, CC_CHAR, CC_BOOL, CC_USER };

object *check_c_func(keg *arg, int i, enum check_c_type t) {
    check_c_func_empty(arg, i);
    object *obj = arg->data[i];
    switch (t) {
    case CC_INT:
        if (obj->kind != OBJ_INT)
            check_c_func_error("int", obj);
        break;
    case CC_FLOAT:
        if (obj->kind != OBJ_FLOAT)
            check_c_func_error("float", obj);
        break;
    case CC_STR:
        if (obj->kind != OBJ_STRING)
            check_c_func_error("string", obj);
        break;
    case CC_CHAR:
        if (obj->kind != OBJ_CHAR)
            check_c_func_error("char", obj);
        break;
    case CC_BOOL:
        if (obj->kind != OBJ_BOOL)
            check_c_func_error("bool", obj);
        break;
    case CC_USER:
        if (obj->kind != OBJ_CUSER)
            check_c_func_error("userdata", obj);
        break;
    }
    return obj;
}

int check_num(keg *arg, int i) {
    return check_c_func(arg, i, CC_INT)->value.num;
}

double check_float(keg *arg, int i) {
    return check_c_func(arg, i, CC_FLOAT)->value.f;
}

char *check_str(keg *arg, int i) {
    return check_c_func(arg, i, CC_STR)->value.str;
}

char check_char(keg *arg, int i) {
    return check_c_func(arg, i, CC_CHAR)->value.c;
}

bool check_bool(keg *arg, int i) {
    return check_c_func(arg, i, CC_BOOL)->value.b;
}

void *check_userdata(keg *arg, int i) {
    return check_c_func(arg, i, CC_USER)->value.cu.ptr;
}

object *check_front(keg *arg) {
    if (arg->item == 0) {
        error("arguments is empty");
    }
    if (arg->item > 1) {
        error("arguments count just recevie one");
    }
    return arg->data[0];
}

void check_empty(keg *arg) {
    if (arg->item > 1) {
        error("only empty parameters are received");
    }
}

reg_mod *new_mod(char *name) {
    reg_mod *m = malloc(sizeof(reg_mod));
    m->name = name;
    m->i = 0;
    return m;
}

void emit_member(reg_mod *m, char *name, enum mem_kind k) {
    if (m == NULL) {
        return;
    }
    reg_mem mem = {.name = name, .kind = k};
    m->member[m->i++] = mem;
}

void throw_error(const char *message) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m %s.\n", GET_LINE, message);
    exit(EXIT_SUCCESS);
}