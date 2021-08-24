/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#include "vm.h"

extern keg *lexer(const char *, int);
extern keg *compile(keg *);

vm_state vst;

frame *new_frame(code_object *code) {
    frame *f = malloc(sizeof(frame));
    f->code = code;
    f->data = new_keg();
    f->ret = NULL;
    f->tb = new_table();
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

frame *back_frame() {
    return back_keg(vst.frame);
}

frame *main_frame() {
    return (frame *)vst.frame->data[0];
}

code_object *top_code() {
    frame *f = back_frame();
    return f->code;
}

keg *top_data() {
    frame *f = back_frame();
    return f->data;
}

#define PUSH(obj)     back_frame()->data = append_keg(back_frame()->data, obj)

#define POP()         (object *)pop_back_keg(back_frame()->data)

#define GET_OFF()     *(int16_t *)top_code()->offsets->data[vst.op++]

#define GET_NAME()    (char *)top_code()->names->data[GET_OFF()]
#define GET_TYPE()    (type *)top_code()->types->data[GET_OFF()]
#define GET_OBJ()     (object *)top_code()->objects->data[GET_OFF()]

#define GET_LINE()    *(int *)top_code()->lines->data[vst.ip]
#define GET_CODE()    *(u_int8_t *)top_code()->codes->data[vst.ip]

#define GET_PR_CODE() *(u_int8_t *)top_code()->codes->data[vst.ip - 2]
#define GET_PR_OBJ() \
    (object *)top_code() \
        ->objects->data[(*(int16_t *)top_code()->offsets->data[vst.op - 1])]

void undefined_error(char *name) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m undefined name '%s'.\n",
            GET_LINE(), name);
    exit(0);
}

void type_error(type *T, object *obj) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m expect type %s, but found %s.\n",
            GET_LINE(), type_string(T), obj_string(obj));
    exit(EXIT_SUCCESS);
}

void unsupport_operand_error(const char *op) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m unsupport operand to '%s'.\n",
            GET_LINE(), op);
    exit(EXIT_SUCCESS);
}

void simple_error(const char *msg) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m %s.\n", GET_LINE(), msg);
    exit(EXIT_SUCCESS);
}

void bt_println(frame *f, frame *b) {
    object *obj = get_table(b->tb, "arg");
    for (int i = 0; i < obj->value.arr.element->item; i++) {
        object *p = obj->value.arr.element->data[i];
        printf("%s\t", obj_std_string(p));
    }
    printf("\n");
}

void bt_print(frame *f, frame *b) {
    object *obj = get_table(b->tb, "arg");
    for (int i = 0; i < obj->value.arr.element->item; i++) {
        object *p = obj->value.arr.element->data[i];
        printf("%s\t", obj_std_string(p));
    }
}

void bt_put(frame *f, frame *m) {
    printf("%s", obj_raw_string(get_table(m->tb, "obj")));
}

void bt_len(frame *f, frame *b) {
    object *len = malloc(sizeof(object));
    len->kind = OBJ_INT;
    len->value.integer = obj_len(get_table(b->tb, "obj"));
    PUSH(len);
}

void bt_type(frame *f, frame *b) {
    object *str = malloc(sizeof(object));
    str->kind = OBJ_STRING;
    str->value.string = (char *)obj_type_string(get_table(b->tb, "obj"));
    PUSH(str);
}

void bt_sleep(frame *f, frame *m) {
    object *obj = get_table(m->tb, "milliseconds");
#if defined(__linux__) || defined(__APPLE__)
    sleep(obj->value.integer / 1000);
#elif defined(_WIN32)
    Sleep(obj->value.integer);
#endif
}

void bt_rand_int(frame *f, frame *m) {
    object *b = get_table(m->tb, "from");
    object *a = get_table(m->tb, "to");
    int x = b->value.integer;
    int y = a->value.integer;
#include <time.h>
    static int r = 0;
    srand(r++);
    object *obj = malloc(sizeof(object));
    obj->kind = OBJ_INT;
    obj->value.integer = rand() % y + x;
    PUSH(obj);
}

void bt_append_entry(frame *f, frame *m) {
    object *arr = get_table(m->tb, "a");
    object *val = get_table(m->tb, "b");
    type *T = arr->value.arr.T;
    if (!type_checker(T, val)) {
        type_error(T, val);
    }
    append_keg(arr->value.arr.element, val);
}

void bt_remove_entry(frame *f, frame *m) {
    object *arr = get_table(m->tb, "a");
    int idx = ((object *)get_table(m->tb, "b"))->value.integer;
    keg *elem = arr->value.arr.element;
    if (elem->item == 0) {
        simple_error("empty array can not to remove entry");
    }
    if (idx < 0 || idx > elem->item - 1) {
        simple_error("index out of bounds");
    }
    remove_keg(elem, idx);
}

void bt_input(frame *f, frame *m) {
    char *literal = malloc(sizeof(char) * 32);
    fgets(literal, 32, stdin);
    literal[strlen(literal) - 1] = '\0';
    object *obj = malloc(sizeof(object));
    obj->kind = OBJ_STRING;
    obj->value.string = literal;
    PUSH(obj);
}

builtin bts[] = {
    {"println", bt_println     },
    {"print",   bt_print       },
    {"put",     bt_put         },
    {"len",     bt_len         },
    {"typeof",  bt_type        },
    {"sleep",   bt_sleep       },
    {"rand",    bt_rand_int    },
    {"append",  bt_append_entry},
    {"remove",  bt_remove_entry},
    {"input",   bt_input       },
};

builtin *get_std(char *name) {
    for (int i = 0; i < sizeof(bts) / sizeof(bts[0]); i++) {
        if (strcmp(bts[i].name, name) == 0) {
            return &bts[i];
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
        switch (GET_CODE()) {
        case CONST_OF:
        case LOAD_OF:
        case ENUMERATE:
        case FUNCTION:
        case INTERFACE:
        case ASSIGN_TO:
        case GET_OF:
        case SET_OF:
        case CALL_FUNC:
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
        case BUILD_ARR:
        case BUILD_TUP:
        case BUILD_MAP:
        case JUMP_TO:
        case T_JUMP_TO:
        case F_JUMP_TO:
        case NEW_OBJ:
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
    void *p = get_table(back_frame()->tb, name);
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
    return NULL;
}

object *make_nil() {
    object *obj = malloc(sizeof(object));
    obj->kind = OBJ_NIL;
    return obj;
}

void load_module(char *, bool);

void check_interface(object *in, object *cl) {
    if (cl->value.cl.init == false) {
        simple_error("class is not initialized");
    }

    keg *elem = in->value.in.element;
    frame *fr = (frame *)cl->value.cl.fr;

    for (int i = 0; i < elem->item; i++) {
        method *m = elem->data[i];

        void *p = get_table(fr->tb, m->name);
        if (p == NULL) {
            simple_error("class does not contain some member");
        }

        object *fn = p;
        if (fn->kind != OBJ_FUNCTION) {
            simple_error("an interface can only be a method");
        }
        if (!type_eq(m->ret, fn->value.fn.ret)) {
            simple_error("return type in the method are inconsistent");
        }
        if (m->T->item != fn->value.fn.v->item) {
            simple_error("inconsistent method arguments");
        }
        for (int j = 0; j < m->T->item; j++) {
            type *a = m->T->data[j];
            type *b = fn->value.fn.v->data[j];
            if (!type_eq(a, b)) {
                simple_error("inconsistent types of method arguments");
            }
        }
    }
}

void eval() {
    while (vst.ip < top_code()->codes->item) {
        u_int8_t code = GET_CODE();
        switch (code) {
        case CONST_OF: {
            object *obj = GET_OBJ();
            PUSH(obj);
            break;
        }
        case STORE_NAME: {
            type *T = GET_TYPE();
            char *name = GET_NAME();
            object *obj = POP();
            if (T->kind == T_BOOL && obj->kind == OBJ_INT) {
                obj->value.boolean = obj->value.integer > 0;
                obj->kind = OBJ_BOOL;
            }
            if (T->kind == T_USER && obj->kind == OBJ_CLASS) {
                void *ptr = lookup(T->inner.name);
                if (ptr == NULL) {
                    simple_error("undefined interface or class");
                }
                object *in = ptr;
                if (in->kind == OBJ_INTERFACE) {
                    check_interface(in, obj);
                    in->value.in.class = (struct object *)obj;
                    obj = in;
                }
            } else {
                if (!type_checker(T, obj)) {
                    type_error(T, obj);
                }
            }
            if (obj->kind == OBJ_ARRAY) {
                obj->value.arr.T = (type *)T->inner.single;
            } else if (obj->kind == OBJ_TUPLE) {
                obj->value.tup.T = (type *)T->inner.single;
            } else if (obj->kind == OBJ_MAP) {
                obj->value.map.T1 = (type *)T->inner.both.T1;
                obj->value.map.T2 = (type *)T->inner.both.T2;
            }

            object *new = obj;
            if (copy_type(T)) {
                new = malloc(sizeof(object));
                memcpy(new, obj, sizeof(object));
            }

            add_table(back_frame()->tb, name, new);
            break;
        }
        case LOAD_OF: {
            char *name = GET_NAME();
            void *ptr = lookup(name);
            if (ptr == NULL) {
                undefined_error(name);
            }
            PUSH(ptr);
            break;
        }
        case ASSIGN_TO: {
            char *name = GET_NAME();
            object *obj = POP();
            void *p = lookup(name);
            if (p == NULL) {
                undefined_error(name);
            }
            object *origin = p;
            if (!obj_kind_eq(origin, obj)) {
                simple_error("inconsistent type");
            }
            if (origin->kind == OBJ_INTERFACE) {
                if (obj->kind != OBJ_CLASS) {
                    simple_error("interface needs to be assigned by whole");
                }
                check_interface(origin, obj);
                origin->value.in.class = (struct object *)obj;
                break;
            }
            add_table(back_frame()->tb, name, obj);
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
            object *b = POP();
            object *a = POP();
            if (a->kind == OBJ_STRING && b->kind == OBJ_STRING) {
                int la = strlen(a->value.string);
                int lb = strlen(b->value.string);
                if (la + lb > STRING_EVAL_MAX) {
                    simple_error("number of characters is greater "
                                 "than 128-bit bytes");
                }
            }
            void *r = binary_op(code, a, b);
            if (r == NULL) {
                unsupport_operand_error(code_string[code]);
            }
            PUSH(r);
            break;
        }
        case ASS_ADD:
        case ASS_SUB:
        case ASS_MUL:
        case ASS_DIV:
        case ASS_SUR: {
            char *name = GET_NAME();
            object *obj = POP();
            void *p = lookup(name);
            if (p == NULL) {
                undefined_error(name);
            }
            u_int8_t op;
            if (code == ASS_ADD)
                op = TO_ADD;
            if (code == ASS_SUB)
                op = TO_SUB;
            if (code == ASS_MUL)
                op = TO_MUL;
            if (code == ASS_DIV)
                op = TO_DIV;
            if (code == ASS_SUR)
                op = TO_SUR;
            void *r = binary_op(op, p, obj);
            if (r == NULL) {
                unsupport_operand_error(code_string[code]);
            }
            add_table(back_frame()->tb, name, r);
            break;
        }
        case BUILD_ARR: {
            int16_t item = GET_OFF();
            object *obj = malloc(sizeof(object));
            obj->kind = OBJ_ARRAY;
            obj->value.arr.element = new_keg();
            if (item == 0) {
                PUSH(obj);
                break;
            }
            while (item > 0) {
                insert_keg(obj->value.arr.element, 0, POP());
                item--;
            }
            PUSH(obj);
            break;
        }
        case BUILD_TUP: {
            int16_t item = GET_OFF();
            object *obj = malloc(sizeof(object));
            obj->kind = OBJ_TUPLE;
            obj->value.tup.element = new_keg();
            if (item == 0) {
                PUSH(obj);
                break;
            }
            while (item > 0) {
                insert_keg(obj->value.tup.element, 0, POP());
                item--;
            }
            PUSH(obj);
            break;
        }
        case BUILD_MAP: {
            int16_t item = GET_OFF();
            object *obj = malloc(sizeof(object));
            obj->kind = OBJ_MAP;
            obj->value.map.k = new_keg();
            obj->value.map.v = new_keg();
            if (item == 0) {
                PUSH(obj);
                break;
            }
            while (item > 0) {
                object *b = POP();
                object *a = POP();
                append_keg(obj->value.map.k, a);
                append_keg(obj->value.map.v, b);
                item -= 2;
            }
            PUSH(obj);
            break;
        }
        case TO_INDEX: {
            object *p = POP();
            object *obj = POP();
            if (obj->kind != OBJ_ARRAY && obj->kind != OBJ_STRING &&
                obj->kind != OBJ_MAP) {
                simple_error("only array, string and map can "
                             "be called with subscipt");
            }
            if (obj->kind == OBJ_ARRAY) {
                if (p->kind != OBJ_INT) {
                    simple_error("get value using integer "
                                 "subscript");
                }
                if (obj->value.arr.element->item == 0) {
                    simple_error("array entry is empty");
                }
                if (p->value.integer >= obj->value.arr.element->item) {
                    simple_error("index out of bounds");
                }
                PUSH(obj->value.arr.element->data[p->value.integer]);
            }
            if (obj->kind == OBJ_MAP) {
                if (obj->value.map.k->item == 0) {
                    simple_error("map entry is empty");
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
                    simple_error("get value using integer "
                                 "subscript");
                }
                int len = strlen(obj->value.string);
                if (len == 0) {
                    simple_error("empty string");
                }
                if (p->value.integer >= len) {
                    simple_error("index out of bounds");
                }
                object *ch = malloc(sizeof(object));
                ch->kind = OBJ_CHAR;
                ch->value.ch = obj->value.string[p->value.integer];
                PUSH(ch);
            }
            break;
        }
        case TO_REPLACE: {
            object *obj = POP();
            object *idx = POP();
            object *j = POP();
            if (j->kind == OBJ_ARRAY) {
                if (idx->kind != OBJ_INT) {
                    simple_error("get value using integer subscript");
                }
                if (!type_checker(j->value.arr.T, obj)) {
                    type_error(j->value.arr.T, obj);
                }
                int p = idx->value.integer;
                if (j->value.arr.element->item == 0) {

                    insert_keg(j->value.arr.element, 0, obj);
                } else {
                    if (p > j->value.arr.element->item - 1) {
                        simple_error("index out of bounds");
                    }
                    replace_keg(j->value.arr.element, p, obj);
                }
            }
            if (j->kind == OBJ_MAP) {
                if (!type_checker(j->value.map.T1, idx)) {
                    type_error(j->value.map.T1, idx);
                }
                if (!type_checker(j->value.map.T2, obj)) {
                    type_error(j->value.map.T2, obj);
                }
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
        case TO_REP_ADD:
        case TO_REP_SUB:
        case TO_REP_MUL:
        case TO_REP_DIV:
        case TO_REP_SUR: {
            object *obj = POP();
            object *idx = POP();
            object *j = POP();
            u_int8_t op;
            if (code == TO_REP_ADD)
                op = TO_ADD;
            if (code == TO_REP_SUB)
                op = TO_SUB;
            if (code == TO_REP_MUL)
                op = TO_MUL;
            if (code == TO_REP_DIV)
                op = TO_DIV;
            if (code == TO_REP_SUR)
                op = TO_SUR;
            if (j->kind == OBJ_ARRAY) {
                if (idx->kind != OBJ_INT) {
                    simple_error("get value using integer subscript");
                }
                int p = idx->value.integer;
                if (j->value.arr.element->item == 0 || p < 0 ||
                    p > j->value.arr.element->item - 1) {
                    simple_error("index out of bounds");
                }
                void *r = binary_op(op, j->value.arr.element->data[p], obj);
                if (r == NULL) {
                    unsupport_operand_error(code_string[code]);
                }
                replace_keg(j->value.arr.element, p, r);
            }
            if (j->kind == OBJ_MAP) {
                if (!type_checker(j->value.map.T1, idx)) {
                    type_error(j->value.map.T1, idx);
                }
                if (!type_checker(j->value.map.T2, obj)) {
                    type_error(j->value.map.T2, obj);
                }
                if (j->value.map.k->item == 0) {
                    simple_error("map entry is empty");
                }
                int p = -1;
                for (int i = 0; i < j->value.map.k->item; i++) {
                    if (obj_eq(idx, j->value.map.k->data[i])) {
                        p = i;
                        break;
                    }
                }
                if (p == -1) {
                    simple_error("no replace key exist");
                }
                object *r = binary_op(op, j->value.map.v->data[p], obj);
                if (r == NULL) {
                    unsupport_operand_error(code_string[code]);
                }
                replace_keg(j->value.map.v, p, r);
            }
            break;
        }
        case TO_BANG: {
            object *obj = POP();
            object *new = malloc(sizeof(object));
            switch (obj->kind) {
            case OBJ_INT:
                new->value.boolean = !obj->value.integer;
                break;
            case OBJ_FLOAT:
                new->value.boolean = !obj->value.floating;
                break;
            case OBJ_CHAR:
                new->value.boolean = !obj->value.ch;
                break;
            case OBJ_STRING:
                new->value.boolean = !strlen(obj->value.string);
                break;
            case OBJ_BOOL:
                new->value.boolean = !obj->value.boolean;
                break;
            default:
                new->value.boolean = false;
            }
            new->kind = OBJ_BOOL;
            PUSH(new);
            break;
        }
        case TO_NOT: {
            object *obj = POP();
            object *new = malloc(sizeof(object));
            if (obj->kind == OBJ_INT) {
                new->value.integer = -obj->value.integer;
                new->kind = OBJ_INT;
                PUSH(new);
                break;
            }
            if (obj->kind == OBJ_FLOAT) {
                new->value.floating = -obj->value.floating;
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
            int16_t off = GET_OFF();
            if (code == JUMP_TO) {
                jump(off);
                break;
            }
            bool ok = ((object *)POP())->value.boolean;
            if (code == T_JUMP_TO && ok) {
                jump(off);
            }
            if (code == F_JUMP_TO && ok == false) {
                jump(off);
            }
            break;
        }
        case FUNCTION: {
            object *obj = GET_OBJ();
            add_table(back_frame()->tb, obj->value.fn.name, obj);
            break;
        }
        case CALL_FUNC: {
            int16_t off = GET_OFF();
            keg *arg = new_keg();
            while (off > 0) {
                if (back_frame()->data->item - 1 <= 0) {
                    simple_error("stack data error");
                }
                append_keg(arg, POP());
                off--;
            }

            object *fn = POP();

            keg *k = fn->value.fn.k;
            keg *v = fn->value.fn.v;

            if ((fn->value.fn.mutiple != NULL && k->item != 1 &&
                 arg->item < k->item) ||
                (fn->value.fn.mutiple == NULL && k->item != arg->item)) {
                simple_error("inconsistent funtion arguments");
            }

            frame *f = new_frame(fn->value.fn.code);

            for (int i = 0; i < k->item; i++) {
                char *N = k->data[i];

                if (v->item == i && fn->value.fn.mutiple != NULL) {
                    type *T = fn->value.fn.mutiple;
                    object *a = malloc(sizeof(object));
                    a->kind = OBJ_ARRAY;
                    a->value.arr.element = NULL;

                    if (arg->item == 0) {
                        a->value.arr.element = new_keg();
                    } else {
                        while (arg->item > 0) {
                            object *p = arg->data[--arg->item];
                            if (!type_checker(T, p)) {
                                type_error(T, p);
                            }
                            a->value.arr.element =
                                append_keg(a->value.arr.element, p);
                        }
                    }
                    add_table(f->tb, N, a);
                } else {
                    type *T = v->data[i];
                    object *p = arg->data[--arg->item];
                    if (!type_checker(T, p)) {
                        type_error(T, p);
                    }
                    add_table(f->tb, N, p);
                }
            }

            if (fn->value.fn.std) {
                builtin *bt = get_std(fn->value.fn.name);
                if (bt == NULL) {
                    simple_error("standard function not found");
                }
                bt->func(back_frame(), f);
                goto next;
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
                        simple_error("function missing return value");
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
            object *obj = GET_OBJ();
            add_table(back_frame()->tb, obj->value.in.name, obj);
            break;
        }
        case ENUMERATE: {
            object *obj = GET_OBJ();
            add_table(back_frame()->tb, obj->value.en.name, obj);
            break;
        }
        case GET_OF: {
            char *name = GET_NAME();
            object *obj = POP();
            if (obj->kind != OBJ_ENUMERATE && obj->kind != OBJ_CLASS &&
                obj->kind != OBJ_TUPLE && obj->kind != OBJ_INTERFACE &&
                obj->kind != OBJ_MODULE) {
                simple_error(
                    "only enum, interface, module, tuple and class type "
                    "are supported");
            }
            if (obj->kind == OBJ_ENUMERATE) {
                keg *elem = obj->value.en.element;

                object *p = malloc(sizeof(object));
                p->kind = OBJ_INT;
                p->value.integer = -1;
                for (int i = 0; i < elem->item; i++) {
                    if (strcmp(name, (char *)elem->data[i]) == 0) {
                        p->value.integer = i;
                        break;
                    }
                }
                PUSH(p);
            }
            if (obj->kind == OBJ_CLASS) {
                if (obj->value.cl.init == false) {
                    simple_error("class did not load initialization members");
                }
                frame *fr = (frame *)obj->value.cl.fr;
                void *ptr = get_table(fr->tb, name);
                if (ptr == NULL) {
                    simple_error("nonexistent member");
                }
                object *val = ptr;
                if (val->kind == OBJ_FUNCTION) {
                    val->value.fn.self = obj->value.cl.fr;
                }
                PUSH(ptr);
            }
            if (obj->kind == OBJ_TUPLE) {
                if (obj->value.tup.element->item == 0) {
                    simple_error("tuple entry is empty");
                }
                for (int i = 0; i < strlen(name); i++) {
                    if (!(name[i] >= '0' && name[i] <= '9')) {
                        simple_error(
                            "subscript can only be obtained with integer");
                    }
                }
                int idx = atoi(name);
                int p = obj->value.tup.element->item - 1 - idx;
                if (idx >= obj->value.tup.element->item) {
                    simple_error("index out of bounds");
                }
                PUSH(obj->value.tup.element->data[p]);
            }
            if (obj->kind == OBJ_INTERFACE) {
                if (obj->value.in.class == NULL) {
                    simple_error("interface is not initialized");
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
                simple_error("nonexistent member");
            }
            if (obj->kind == OBJ_MODULE) {
                void *p = get_table((table *)obj->value.mod.tb, name);
                if (p == NULL) {
                    undefined_error(name);
                }
                PUSH(p);
            }
            break;
        }
        case GET_IN_OF: {
            char *name = GET_NAME();
            if (vst.call->item < 2) {
                simple_error("need to use this statement in the class");
            }
            void *ptr = lookup(name);
            if (ptr == NULL) {
                simple_error("nonexistent member");
            }
            PUSH(ptr);
            break;
        }
        case SET_OF: {
            char *name = GET_NAME();
            object *val = POP();
            object *obj = POP();
            if (obj->kind != OBJ_CLASS && obj->kind != OBJ_MODULE) {
                simple_error("only members of class and module can be set");
            }
            table *tb = NULL;
            if (obj->kind == OBJ_CLASS) {
                frame *fr = (frame *)obj->value.cl.fr;
                tb = fr->tb;
            }
            if (obj->kind == OBJ_MODULE) {
                tb = (table *)obj->value.mod.tb;
            }
            void *ptr = get_table(tb, name);
            if (ptr == NULL) {
                simple_error("nonexistent member");
            }
            object *j = ptr;
            if (val->kind != j->kind && j->kind != OBJ_NIL) {
                simple_error("wrong type set");
            }
            if (!obj_kind_eq(obj, val)) {
                simple_error("inconsistent type");
            }
            add_table(tb, name, val);
            break;
        }
        case CLASS: {
            object *obj = GET_OBJ();
            add_table(back_frame()->tb, obj->value.cl.name, obj);
            break;
        }
        case NEW_OBJ: {
            int16_t arg = GET_OFF();

            keg *k = NULL;
            keg *v = NULL;

            while (arg > 0) {
                v = append_keg(v, POP());
                k = append_keg(k, POP());
                arg -= 2;
            }

            object *obj = POP();
            if (obj->kind != OBJ_CLASS) {
                simple_error("only class object can be created");
            }

            object *new = malloc(sizeof(object));
            memcpy(new, obj, sizeof(object));

            new->value.cl.fr = (struct frame *)new_frame(obj->value.cl.code);

            int16_t op_up = vst.op;
            int16_t ip_up = vst.ip;

            vst.op = 0;
            vst.ip = 0;
            vst.frame = append_keg(vst.frame, new->value.cl.fr);
            eval();

            pop_back_keg(vst.frame);

            vst.op = op_up;
            vst.ip = ip_up;

            frame *f = (frame *)new->value.cl.fr;

            if (k != NULL) {
                for (int i = 0; i < k->item; i++) {
                    char *key = ((object *)k->data[i])->value.string;
                    object *val = v->data[i];
                    void *p = get_table(f->tb, key);
                    if (p == NULL) {
                        undefined_error(key);
                    }
                    add_table(f->tb, key, val);
                }
            }

            new->value.cl.init = true;
            PUSH(new);
            break;
        }
        case SET_NAME: {
            char *name = GET_NAME();
            object *n = malloc(sizeof(object));
            n->kind = OBJ_STRING;
            n->value.string = name;
            PUSH(n);
            break;
        }
        case SE_ASS_ADD:
        case SE_ASS_SUB:
        case SE_ASS_MUL:
        case SE_ASS_DIV:
        case SE_ASS_SUR: {
            char *name = GET_NAME();
            object *val = POP();
            object *obj = POP();

            frame *f = (frame *)obj->value.cl.fr;

            void *ptr = get_table(f->tb, name);
            if (ptr == NULL) {
                undefined_error(name);
            }

            u_int8_t op;
            if (code == SE_ASS_ADD)
                op = TO_ADD;
            if (code == SE_ASS_SUB)
                op = TO_SUB;
            if (code == SE_ASS_MUL)
                op = TO_MUL;
            if (code == SE_ASS_DIV)
                op = TO_DIV;
            if (code == SE_ASS_SUR)
                op = TO_SUR;

            void *r = binary_op(op, ptr, val);
            if (r == NULL) {
                unsupport_operand_error(code_string[code]);
            }
            add_table(f->tb, name, r);
            break;
        }
        case TO_RET:
        case RET_OF: {
            vst.ip = back_frame()->code->codes->item;
            vst.loop_ret = true;
            if (code == RET_OF) {
                if (GET_PR_CODE() == FUNCTION) {
                    back_frame()->ret = GET_PR_OBJ();
                } else {
                    back_frame()->ret = POP();
                }
            }
            break;
        }
        case USE_MOD:
        case USE_IN_MOD: {
            load_module(GET_NAME(), code == USE_IN_MOD);
            break;
        }
        default: {
            fprintf(stderr, "\033[1;31mvm %d:\033[0m unreachable '%s'.\n",
                    GET_LINE(), code_string[code]);
            exit(EXIT_SUCCESS);
        }
        }
    next:
        vst.ip++;
    }
}

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

keg *read_path(keg *pl, char *path) {
    DIR *dir;
    struct dirent *p;
    if ((dir = opendir(path)) == NULL) {
        simple_error("failed to open the std library or current directory");
    }
    while ((p = readdir(dir)) != NULL) {
        if (p->d_type == 4 && strcmp(p->d_name, ".") != 0 &&
            strcmp(p->d_name, "..") != 0) {}
        if (p->d_type == 8) {
            char *name = p->d_name;
            int len = strlen(name) - 1;
            if (name[len] == 't' && name[len - 1] == 'f' &&
                name[len - 2] == '.') {
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
        printf("\033[1;31mvm %d:\033[0m failed to read buffer of file "
               "'%s'\n",
               GET_LINE(), path);
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
            add_table(back_frame()->tb, tb->name->data[i], tb->objs->data[i]);
        }
    } else {
        object *obj = malloc(sizeof(object));
        obj->kind = OBJ_MODULE;
        obj->value.mod.tb = (struct table *)tb;
        obj->value.mod.name = name;
        add_table(back_frame()->tb, name, obj);
    }
    free_keg(codes);

    fclose(fp);
    free_tokens(tokens);
}

void load_module(char *name, bool internal) {
    bool ok = false;
    if (filename_eq(vst.filename, name)) {
        simple_error("cannot reference itself");
    }
    keg *pl = new_keg();
    void *path = getenv("FTPATH");
    if (path != NULL) {
        pl = read_path(pl, path);
    }
    pl = read_path(pl, ".");
    for (int i = 0; i < pl->item; i++) {
        char *addr = pl->data[i];
        if (filename_eq(get_filename(addr), name)) {
            load_eval(addr, name, internal);
            ok = true;
            break;
        }
    }
    if (!ok) {
        fprintf(stderr, "\033[1;31mvm %d:\033[0m undefined module '%s'.\n",
                GET_LINE(), name);
        exit(EXIT_SUCCESS);
    } else {
        free_keg(pl);
    }
}
