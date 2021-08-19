/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#include "vm.h"

extern keg *lexer(const char *, int);
extern keg *compile(keg *);

/* Virtual machine state */
vm_state vst;

/* Create a frame object */
frame *new_frame(code_object *code) {
    frame *f = malloc(sizeof(frame));
    f->code = code;
    f->data = new_keg();
    f->ret = NULL;
    f->tb = new_table();
    return f;
}

/* Free frame struct */
void free_frame(frame *f) {
    printf("free GC\n");
}

/* Free lexical token list */
void free_tokens(keg *g) {
    for (int i = 0; i < g->item; i++) {
        token *tok = (token *)g->data[i];
        if (tok->kind == FLOAT || tok->kind == NUMBER || tok->kind == CHAR ||
            tok->kind >= 39) {
            free(tok->literal);
        }
        free(tok);
    }
    free_keg(g);
}

/* Return font frame object */
frame *back_frame() {
    return back_keg(vst.frame);
}

/* Return main frame object */
frame *main_frame() {
    return (frame *)vst.frame->data[0];
}

/* Return code object of front frame */
code_object *top_code() {
    frame *f = back_frame();
    return f->code;
}

/* Return data stack of front frame */
keg *top_data() {
    frame *f = back_frame();
    return f->data;
}

/* Push data stack */
#define PUSH(obj)     back_frame()->data = append_keg(back_frame()->data, obj)

/* Pop data stack */
#define POP()         (object *)pop_back_keg(back_frame()->data)

#define GET_OFF()     *(int16_t *)top_code()->offsets->data[vst.op++]

/* Data acquisition of code object */
#define GET_NAME()    (char *)top_code()->names->data[GET_OFF()]
#define GET_TYPE()    (type *)top_code()->types->data[GET_OFF()]
#define GET_OBJ()     (object *)top_code()->objects->data[GET_OFF()]

#define GET_LINE()    *(int *)top_code()->lines->data[vst.ip]
#define GET_CODE()    *(u_int8_t *)top_code()->codes->data[vst.ip]

#define GET_PR_CODE() *(u_int8_t *)top_code()->codes->data[vst.ip - 2]
#define GET_PR_OBJ() \
    (object *)top_code() \
        ->objects->data[(*(int16_t *)top_code()->offsets->data[vst.op - 1])]

/* Undefined error */
void undefined_error(char *name) {
    fprintf(stderr, "\033[1;31mvm %s %d:\033[0m undefined name '%s'.\n",
            vst.filename, GET_LINE(), name);
    exit(EXIT_FAILURE);
}

/* Type error */
void type_error(type *T, object *obj) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m expect type %s, but found %s.\n",
            GET_LINE(), type_string(T), obj_string(obj));
    exit(EXIT_FAILURE);
}

/* Operand error */
void unsupport_operand_error(const char *op) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m unsupport operand to '%s'.\n",
            GET_LINE(), op);
    exit(EXIT_FAILURE);
}

/* Make simple error message */
void simple_error(const char *msg) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m %s.\n", GET_LINE(), msg);
    exit(EXIT_FAILURE);
}

/* Builtin function: (any) println */
void println(frame *f, keg *arg) {
    printf("%s", obj_raw_string((object *)arg->data[0]));
    printf("\n");
}

/* Builtin function: (any) print */
void print(frame *f, keg *arg) {
    printf("%s", obj_raw_string((object *)arg->data[0]));
}

/* Builtin function: (any) len -> int */
void len(frame *f, keg *arg) {
    object *len = malloc(sizeof(object));
    len->kind = OBJ_INT;
    len->value.integer = obj_len((object *)pop_back_keg(arg));
    PUSH(len);
}

/* Builtin function: (any) typeof -> string */
void ob_type(frame *f, keg *arg) {
    object *str = malloc(sizeof(object));
    str->kind = OBJ_STRING;
    str->value.string = (char *)obj_type_string((object *)pop_back_keg(arg));
    PUSH(str);
}

/* Builtin function: (int) sleep */
void s_sleep(frame *f, keg *arg) {
    object *obj = (object *)pop_back_keg(arg);
#if defined(__linux__) || defined(__APPLE__)
    sleep(obj->value.integer / 1000);
#elif defined(_WIN32)
    Sleep(obj->value.integer);
#endif
}

/* Builtin function: (int, int) rand -> int */
void rand_int(frame *f, keg *arg) {
    object *b = (object *)pop_back_keg(arg);
    object *a = (object *)pop_back_keg(arg);
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

/* Builtin function: ([]any, any) append_entry */
void append_entry(frame *f, keg *arg) {
    object *arr = (object *)pop_back_keg(arg);
    object *val = (object *)pop_back_keg(arg);
    type *T = arr->value.arr.T;
    if (!type_checker(T, val)) {
        type_error(T, val);
    }
    append_keg(arr->value.arr.element, val);
}

/* Builtin function: ([]any, int) remove_entry */
void remove_entry(frame *f, keg *arg) {
    object *arr = (object *)pop_back_keg(arg);
    int idx = ((object *)pop_back_keg(arg))->value.integer;
    keg *elem = arr->value.arr.element;
    if (elem->item == 0) {
        simple_error("empty array can not to remove entry");
    }
    if (idx < 0 || idx > elem->item - 1) {
        simple_error("index out of bounds");
    }
    remove_keg(elem, idx);
}

/* Builtin function: () input -> string */
void input(frame *f, keg *arg) {
    char *literal = malloc(sizeof(char) * 32);
    fgets(literal, 32, stdin);
    literal[strlen(literal) - 1] = '\0';
    object *obj = malloc(sizeof(object));
    obj->kind = OBJ_STRING;
    obj->value.string = literal;
    PUSH(obj);
}

/* Builtin function keg */
builtin bts[] = {
    {"println", println     },
    {"print",   print       },
    {"len",     len         },
    {"typeof",  ob_type     },
    {"sleep",   s_sleep     },
    {"rand",    rand_int    },
    {"append",  append_entry},
    {"remove",  remove_entry},
    {"input",   input       },
};

/* Jumps to the specified bytecode position and
   moves the offset */
void jump(int16_t to) {
    vst.op--; /* Step back the offset of the current jump */
    bool reverse = vst.ip > to;
    if (reverse) {
        /* Inversion skip the jump bytecode */
        vst.ip--;
    }
    while (reverse ? vst.ip >= to : /* Condition */
               vst.ip < to) {
        switch (GET_CODE()) {
        case CONST_OF:
        case LOAD_OF:
        case ENUMERATE:
        case FUNCTION:
        case LOAD_FACE:
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
            /* These bytecodes contain a parameter */
            reverse ? vst.op-- : vst.op++;
            break;
        case STORE_NAME: /* Two parameter */
            if (reverse)
                vst.op -= 2;
            else
                vst.op += 2;
            break;
        }
        reverse ? /* Set bytecode position */
            vst.ip--
                : vst.ip++;
    }
    if (!reverse) { /* Loop update */
        vst.ip--;
    }
}

/* Find object in overlay frames */
void *lookup(char *name) {
    for (int i = vst.frame->item - 1; i >= 0; i--) {
        frame *f = (frame *)vst.frame->data[i];
        void *ptr = get_table(f->tb, name);
        if (ptr != NULL) {
            return ptr;
        }
    }
    return NULL;
}

/* nil */
object *make_nil() {
    object *obj = malloc(sizeof(object));
    obj->kind = OBJ_NIL;
    return obj;
}

void load_module(char *); /* Load module by name */

/* The interface implementation of the test class */
void check_interface(object *in, object *cl) {
    if (cl->value.cl.init == false) {
        simple_error("class is not initialized");
    }
    keg *elem = in->value.in.element; /* Interface method */
    frame *fr = (frame *)cl->value.cl.fr;
    for (int i = 0; i < elem->item; i++) {
        method *m =
            (method *)elem->data[i]; /* Traverse the node and check the type */
        void *p = get_table(fr->tb, m->name);
        if (p == NULL) {
            simple_error("class does not contain some member");
        }
        object *fn = (object *)p; /* object inner */
        if (fn->kind != OBJ_FUNCTION) {
            simple_error("an interface can only be a method");
        }
        /* Check return type */
        if (!type_eq(m->ret, fn->value.fn.ret)) {
            simple_error("return type in the method are inconsistent");
        }
        /* Check function arguments */
        if (m->T->item != fn->value.fn.v->item) {
            simple_error("inconsistent method arguments");
        }
        /* Check function arguments type */
        for (int j = 0; j < m->T->item; j++) {
            type *a = (type *)m->T->data[j];
            type *b = (type *)fn->value.fn.v->data[j];
            if (!type_eq(a, b)) {
                simple_error("Inconsistent types of method arguments");
            }
        }
    }
}

/* Evaluate */
void eval() {
    while (vst.ip < top_code()->codes->item) { /* Traversal bytecode keg */
        u_int8_t code = GET_CODE();            /* Bytecode */
        switch (code) {
        case CONST_OF: { /* J */
            object *obj = GET_OBJ();
            PUSH(obj);
            break;
        }
        case STORE_NAME: { /* N T */
            type *T = GET_TYPE();
            char *name = GET_NAME();
            object *obj = POP();
            if (T->kind == T_BOOL && obj->kind == OBJ_INT) { /* int to bool */
                obj->value.boolean = obj->value.integer > 0;
                obj->kind = OBJ_BOOL;
            }
            if (T->kind == T_USER && obj->kind == OBJ_CLASS) {
                void *ptr = get_table(back_frame()->tb, T->inner.name);
                if (ptr == NULL) {
                    simple_error("cannot store to undefined interface");
                }
                object *in = (object *)ptr;
                if (in->kind != OBJ_INTERFACE) {
                    simple_error("type is not interface or class");
                }
                check_interface(in, obj);
            } else {
                if (/* Store and object type check */
                    !type_checker(T, obj)) {
                    type_error(T, obj);
                }
            }
            if (obj->kind == OBJ_ARR) { /* Sets the type specified */
                obj->value.arr.T = (type *)T->inner.single;
            } else if (obj->kind == OBJ_TUP) { /* Tuple */
                obj->value.tup.T = (type *)T->inner.single;
            } else if (obj->kind == OBJ_MAP) { /* Map */
                obj->value.map.T1 = (type *)T->inner.both.T1;
                obj->value.map.T2 = (type *)T->inner.both.T2;
            }
            /* Interface */
            object *new = malloc(sizeof(object));
            memcpy(new, obj, sizeof(object));
            /* Add to current table */
            add_table(back_frame()->tb, name, new);
            break;
        }
        case LOAD_OF: { /* N */
            char *name = GET_NAME();
            void *resu = lookup(name); /* Lookup frames */
            if (resu == NULL) {
                if (vst.whole != NULL && vst.frame->item > 1) {
                    frame *f = (frame *)vst.whole->value.cl.fr;
                    resu = get_table(f->tb, name); /* Set to whole frame */
                }
            }
            if (resu == NULL) {
                undefined_error(name); /* Undefined name */
            }
            PUSH((object *)resu);
            break;
        }
        case ASSIGN_TO: { /* N */
            char *name = GET_NAME();
            object *obj = POP();
            void *p = get_table(back_frame()->tb, name);
            if (p == NULL) { /* Get */
                undefined_error(name);
            }
            object *origin = (object *)p;
            if (!obj_kind_eq(origin, obj)) {
                simple_error("inconsistent type");
            }
            if (origin->kind == OBJ_INTERFACE) {
                if (obj->kind != OBJ_CLASS) {
                    simple_error("interface needs to be assigned by whole");
                }
                check_interface(origin, obj);
                /* Store whole field into face object */
                origin->value.in.class = (struct object *)obj;
                break;
            }
            /* Update the content of the table */
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
        case TO_LE_EQ: /* x */
        case TO_EQ_EQ:
        case TO_NOT_EQ:
        case TO_AND:
        case TO_OR: {
            object *b = POP(); /* P2 */
            object *a = POP(); /* P1 */
            if (a->kind == OBJ_STRING && b->kind == OBJ_STRING) {
                int la = strlen(a->value.string);
                int lb = strlen(b->value.string);
                if (la + lb > STRING_CAP) {
                    simple_error(
                        "number of characters is greater than 128-bit bytes");
                }
            }
            void *r = binary_op(code, a, b);
            if (r == NULL) { /* Operand error */
                unsupport_operand_error(code_string[code]);
            }
            PUSH((object *)r);
            break;
        }
        case ASS_ADD:
        case ASS_SUB:
        case ASS_MUL:
        case ASS_DIV:
        case ASS_SUR: { /* N */
            char *name = GET_NAME();
            object *obj = POP();
            void *p = get_table(back_frame()->tb, name);
            if (p == NULL) {
                undefined_error(name); /* Get name */
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
            void *r = binary_op(op, (object *)p, obj); /* To operation */
            if (r == NULL) {
                unsupport_operand_error(code_string[code]);
            }
            /* Update */
            add_table(back_frame()->tb, name, (object *)r);
            break;
        }
        case BUILD_ARR: { /* F */
            int16_t item = GET_OFF();
            object *obj = malloc(sizeof(object));
            obj->kind = OBJ_ARR;
            obj->value.arr.element = new_keg();
            if (item == 0) {
                PUSH(obj);
                break;
            }
            while (item > 0) { /* Build data */
                insert_keg(obj->value.arr.element, 0, POP());
                item--;
            }
            PUSH(obj);
            break;
        }
        case BUILD_TUP: { /* F */
            int16_t item = GET_OFF();
            object *obj = malloc(sizeof(object));
            obj->kind = OBJ_TUP;
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
        case BUILD_MAP: { /* N */
            int16_t item = GET_OFF();
            object *obj = malloc(sizeof(object));
            obj->kind = OBJ_MAP;
            obj->value.map.k = new_keg(); /* K */
            obj->value.map.v = new_keg(); /* V */
            if (item == 0) {
                PUSH(obj);
                break;
            }
            while (item > 0) {
                object *b = POP();
                object *a = POP();
                append_keg(obj->value.map.k, a); /* Key */
                append_keg(obj->value.map.v, b); /* Value */
                item -= 2;
            }
            PUSH(obj);
            break;
        }
        case TO_INDEX: { /* x */
            object *p = POP();
            object *obj = POP();
            if (obj->kind != OBJ_ARR &&
                obj->kind != OBJ_STRING && /* Array, String and Map */
                obj->kind != OBJ_MAP) {
                simple_error(
                    "only array, string and map can be called with subscipt");
            }
            if (obj->kind == OBJ_ARR) { /* Array */
                if (p->kind != OBJ_INT) {
                    simple_error(
                        "get value using integer subscript"); /* Index only
                                                                 integer */
                }
                if (obj->value.arr.element->item == 0) { /* None elements */
                    simple_error("array entry is empty");
                }
                if (p->value.integer >=
                    obj->value.arr.element->item) { /* Index out */
                    simple_error("index out of bounds");
                }
                PUSH((object *)obj->value.arr.element
                         ->data[p->value.integer /* Get indexes add to table */
                ]);
            }
            if (obj->kind == OBJ_MAP) { /* Map */
                if (obj->value.map.k->item == 0) {
                    simple_error("map entry is empty");
                }
                int i = 0; /* Traverse the key, find the corresponding,
                    and change the variable */
                for (; i < obj->value.map.k->item; i++) {
                    if (obj_eq(/* Object equal compare */
                               p, (object *)obj->value.map.k->data[i])) {
                        PUSH((object *)obj->value.map.v->data[i]);
                        break;
                    }
                }
                /* Not found */
                if (i == obj->value.map.k->item) {
                    PUSH(make_nil());
                }
            }
            if (obj->kind == OBJ_STRING) { /* String*/
                if (p->kind != OBJ_INT) {
                    simple_error(
                        "get value using integer subscript"); /* Integer index
                                                               */
                }
                int len = strlen(obj->value.string);
                if (len == 0) {
                    simple_error("empty string");
                }
                if (p->value.integer >= len) {
                    simple_error("index out of bounds"); /* Index out */
                }
                object *ch = malloc(sizeof(object));
                ch->kind = OBJ_CHAR;
                ch->value.ch = obj->value.string[p->value.integer];
                PUSH(ch);
            }
            break;
        }
        case TO_REPLACE: { /* x */
            object *obj = POP();
            object *idx = POP();
            object *j = POP();
            if (j->kind == OBJ_ARR) { /* Array */
                if (idx->kind != OBJ_INT) {
                    simple_error("get value using integer subscript");
                }
                if (!type_checker(j->value.arr.T,
                                  obj)) { /* Inconsistent type */
                    type_error(j->value.arr.T, obj);
                }
                int p = idx->value.integer; /* Indexes */
                if (j->value.arr.element->item == 0) {
                    /* Insert empty keg directly */
                    insert_keg(j->value.arr.element, 0, obj);
                } else {
                    /* Index out of bounds */
                    if (p > j->value.arr.element->item - 1) {
                        simple_error("index out of bounds");
                    }
                    /* Replace data */
                    replace_keg(j->value.arr.element, p, obj);
                }
            }
            if (j->kind == OBJ_MAP) { /* Map */
                /* Type check of K and V */
                if (!type_checker(j->value.map.T1, idx)) {
                    type_error(j->value.map.T1, idx);
                }
                if (!type_checker(j->value.map.T2, obj)) {
                    type_error(j->value.map.T2, obj);
                }
                int p = -1; /* Find the location of the key */
                for (int i = 0; i < j->value.map.k->item; i++) {
                    if (obj_eq(idx, (object *)j->value.map.k->data[i])) {
                        p = i;
                        break;
                    }
                }
                if (p == -1) { /* Insert empty keg directly */
                    insert_keg(j->value.map.k, 0, idx);
                    insert_keg(j->value.map.v, 0, obj);
                } else {
                    replace_keg(j->value.map.v, p, obj); /* Replace data */
                }
            }
            break;
        }
        case TO_REP_ADD:
        case TO_REP_SUB:
        case TO_REP_MUL:
        case TO_REP_DIV:
        case TO_REP_SUR: { /* x */
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
            if (j->kind == OBJ_ARR) { /* Array */
                if (idx->kind != OBJ_INT) {
                    simple_error("get value using integer subscript");
                }
                int p = idx->value.integer; /* Indexes */
                if (j->value.arr.element->item == 0 || p < 0 ||
                    p > j->value.arr.element->item - 1) {
                    simple_error("index out of bounds");
                }
                void *r =
                    binary_op(/* To operand */
                              op, (object *)j->value.arr.element->data[p], obj);
                if (r == NULL) {
                    /* Operand error */
                    unsupport_operand_error(code_string[code]);
                }
                replace_keg(j->value.arr.element, p,
                            (object *)r); /* Replace data */
            }
            if (j->kind == OBJ_MAP) { /* Map */
                /* Type check of K and V */
                if (!type_checker(j->value.map.T1, idx)) {
                    type_error(j->value.map.T1, idx);
                }
                if (!type_checker(j->value.map.T2, obj)) {
                    type_error(j->value.map.T2, obj);
                }
                if (j->value.map.k->item == 0) {
                    simple_error("map entry is empty");
                }
                int p = -1; /* Find the location of the key */
                for (int i = 0; i < j->value.map.k->item; i++) {
                    if (obj_eq(idx, (object *)j->value.map.k->data[i])) {
                        p = i;
                        break;
                    }
                }
                if (p == -1) { /* Insert empty keg directly */
                    simple_error("no replace key exist");
                }
                object *r =
                    binary_op(op, (object *)j->value.map.v->data[p], obj);
                if (r == NULL) {
                    unsupport_operand_error(code_string[code]);
                }
                replace_keg(j->value.map.v, p, r); /* Replace date */
            }
            break;
        }
        case TO_BANG: { /* !P */
            object *obj = POP();
            object *new = malloc(sizeof(object)); /* Alloc new */
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
        case TO_NOT: { /* -P */
            object *obj = POP();
            object *new = malloc(sizeof(object)); /* Alloc new */
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
        case T_JUMP_TO: {            /* F */
            int16_t off = GET_OFF(); /* Offset */
            if (code == JUMP_TO) {
                jump(off); /* Just jump */
                break;
            }
            bool ok = /* Condition */
                ((object *)POP())->value.boolean;
            if (code == T_JUMP_TO && ok) {
                jump(off);
            }
            if (code == F_JUMP_TO && ok == false) {
                jump(off);
            }
            break;
        }
        case FUNCTION: { /* J */
            object *obj = GET_OBJ();
            add_table(back_frame()->tb, obj->value.fn.name, obj); /* Func */
            break;
        }
        case CALL_FUNC: { /* F */
            int16_t off = GET_OFF();
            keg *arg = new_keg();
            while (off > 0) { /* Skip function object */
                if (back_frame()->data->item - 1 <= 0) {
                    simple_error("stack data error");
                }
                append_keg(arg, POP());
                off--;
            }

            object *fn = POP(); /* Function */

            keg *k = fn->value.fn.k;
            keg *v = fn->value.fn.v;

            /* Arguments item */
            if (k->item != arg->item) {
                simple_error("inconsistent funtion arguments");
            }

            if (fn->value.fn.std) {
                for (int i = 0; i < sizeof(bts) / sizeof(bts[0]); i++) {
                    if (strcmp(bts[i].name, fn->value.fn.name) == 0) {
                        for (int i = 0, j = arg->item; i < k->item; i++) {
                            type *T = (type *)v->data[i]; /* Arg type */
                            object *obj = arg->data[--j]; /* Arg object */
                            if (!type_checker(T, obj)) {
                                type_error(T, obj);
                            }
                        }
                        /* Call std function */
                        bts[i].func(back_frame(), arg);
                        goto next;
                    }
                }
            }

            /* Call frame */
            frame *f = new_frame(fn->value.fn.code); /* Func code object */

            /* Check arguments */
            for (int i = 0; i < k->item; i++) {
                char *N = (char *)k->data[i];       /* Name */
                type *T = (type *)v->data[i];       /* Type */
                object *p = arg->data[--arg->item]; /* Reverse */
                /* Check type consistent */
                if (!type_checker(T, p)) {
                    type_error(T, p);
                }
                /* Add to table of frame */
                add_table(f->tb, N, p);
            }

            /* Backup pointer */
            int16_t op_up = vst.op;
            int16_t ip_up = vst.ip;

            vst.op = 0; /* Clear */
            vst.ip = 0;
            vst.frame = append_keg(vst.frame, f);
            eval();

            frame *p =
                (frame *)pop_back_keg(vst.frame); /* Evaluated and pop frame */

            /* Function return */
            if (fn->value.fn.ret != NULL) {
                if (p->ret == NULL || !type_checker(fn->value.fn.ret, p->ret)) {
                    if (p->ret == NULL) {
                        simple_error("function missing return value");
                    }
                    type_error(fn->value.fn.ret, p->ret); /* Ret type */
                }
                PUSH(p->ret); /* Push to back frame */
            }

            vst.op = op_up; /* Reset pointer to main */
            vst.ip = ip_up;
            break;
        }
        case LOAD_FACE: { /* J */
            object *obj = GET_OBJ();
            add_table(back_frame()->tb, obj->value.in.name, obj); /* Face */
            break;
        }
        case ENUMERATE: { /* J */
            object *obj = GET_OBJ();
            add_table(back_frame()->tb, obj->value.en.name, obj); /* Enum */
            break;
        }
        case GET_OF: { /* N */
            char *name = GET_NAME();
            object *obj = POP();
            if (obj->kind != OBJ_ENUMERATE && obj->kind != OBJ_CLASS &&
                obj->kind != OBJ_TUP && obj->kind != OBJ_INTERFACE &&
                obj->kind != OBJ_MODULE) {
                simple_error("only enum, face, module, tuple and whole type "
                             "are supported");
            }
            if (obj->kind == OBJ_ENUMERATE) { /* Enum */
                keg *elem = obj->value.en.element;
                /* Enumeration get integer */
                object *p = malloc(sizeof(object));
                p->kind = OBJ_INT;
                p->value.integer = -1; /* Origin value */
                for (int i = 0; i < elem->item; i++) {
                    if (strcmp(name, (char *)elem->data[i]) == 0) {
                        p->value.integer = i;
                        break;
                    }
                }
                PUSH(p);
            }
            if (obj->kind == OBJ_CLASS) { /* Whole */
                if (obj->value.cl.init == false) {
                    simple_error("whole did not load initialization members");
                }
                frame *fr = (frame *)obj->value.cl.fr;
                void *ptr = get_table(fr->tb, name);
                if (ptr == NULL) {
                    simple_error("nonexistent member");
                }
                vst.whole = obj; /* Record the whole that is currently invoke */
                PUSH((object *)ptr);
            }
            if (obj->kind == OBJ_TUP) { /* Tuple */
                if (obj->value.tup.element->item == 0) {
                    simple_error("tuple entry is empty");
                }
                for (int i = 0; i < strlen(name); i++) {
                    if (/* */
                        !(name[i] >= '0' && name[i] <= '9')) {
                        simple_error(
                            "subscript can only be obtained with integer");
                    }
                }
                /* Index */
                int idx = atoi(name); /* Index is reversed */
                int p =
                    obj->value.tup.element->item - 1 - idx; /* Previous index */
                if (idx >= obj->value.tup.element->item) {
                    simple_error("index out of bounds");
                }
                PUSH((object *)obj->value.tup.element->data[p]);
            }
            if (obj->kind == OBJ_INTERFACE) { /* Face */
                if (obj->value.in.class == NULL) {
                    simple_error("interface is not initialized");
                }
                keg *elem = obj->value.in.element; /* Face method */
                for (int i = 0; i < elem->item; i++) {
                    method *m = (method *)elem->data[i];
                    if (strcmp(m->name, name) == 0) {
                        object *wh =
                            (object *)
                                obj->value.in.class; /* Get class object */
                        frame *fr =
                            (frame *)wh->value.cl.fr; /* Get frame in class */
                        vst.whole = wh;
                        PUSH((object *)get_table(fr->tb, name));
                        goto next;
                    }
                }
                simple_error("nonexistent member");
            }
            if (obj->kind == OBJ_MODULE) { /* Module */
                void *p = get_table((table *)obj->value.mod.tb, name);
                if (p == NULL) {
                    undefined_error(name);
                }
                PUSH((object *)p);
            }
            break;
        }
        case SET_OF: { /* N */
            char *name = GET_NAME();
            object *val = POP(); /* Will set value */
            object *obj = POP();
            if (obj->kind != OBJ_CLASS && obj->kind != OBJ_MODULE) {
                simple_error("only members of class and module can be set");
            }
            table *tb = NULL;
            if (obj->kind == OBJ_CLASS) {
                frame *fr = (frame *)obj->value.cl.fr; /* frame */
                tb = fr->tb;
            }
            if (obj->kind == OBJ_MODULE) {
                tb = (table *)obj->value.mod.tb;
            }
            void *ptr = get_table(tb, name);
            if (ptr == NULL) {
                simple_error("nonexistent member");
            }
            object *j = (object *)ptr; /* Origin value */
            if (val->kind != j->kind && j->kind != OBJ_NIL) {
                simple_error("wrong type set");
            }
            /* Check basic object */
            if (!basic(j)) {
                simple_error("only members of basic objects can be assign");
            }
            if (!obj_kind_eq(obj, val)) {
                simple_error("inconsistent type");
            }
            add_table(tb, name, val); /* Restore */
            break;
        }
        case CLASS: { /* J */
            object *obj = GET_OBJ();
            add_table(back_frame()->tb, obj->value.cl.name, obj); /* Class */
            break;
        }
        case NEW_OBJ: { /* N F*/
            int16_t item = GET_OFF();

            keg *k = NULL; /* Key */
            keg *v = NULL;

            while (item > 0) { /* Arguments */
                v = append_keg(v, POP());
                k = append_keg(k, POP());
                item -= 2;
            }

            object *obj = POP();
            if (obj->kind != OBJ_CLASS) {
                simple_error("only whole object can be created");
            }

            object *new = malloc(sizeof(object)); /* Copy */
            memcpy(new, obj, sizeof(object));

            /* Evaluate frame of class */
            new->value.cl.fr = (struct frame *)new_frame(obj->value.cl.code);
            /* Backup pointer */
            int16_t op_up = vst.op;
            int16_t ip_up = vst.ip;

            vst.op = 0;
            vst.ip = 0;
            vst.frame = append_keg(vst.frame, new->value.cl.fr);
            eval(); /* Evaluate */

            pop_back_keg(vst.frame); /* Pop */

            vst.op = op_up; /* Reset pointer */
            vst.ip = ip_up;

            frame *f = (frame *)new->value.cl.fr;

            /* Initialize member */
            if (k != NULL) {
                for (int i = 0; i < k->item; i++) {
                    char *key = ((object *)k->data[i])->value.string;
                    object *val = (object *)v->data[i]; /* Set into frame */
                    void *p = get_table(f->tb, key);
                    if (p == NULL) {
                        undefined_error(key);
                    }
                    add_table(f->tb, key, val);
                }
            }

            new->value.cl.init = true; /* Class object */
            PUSH(new);
            break;
        }
        case SET_NAME: { /* N */
            char *name = GET_NAME();
            object *n = malloc(sizeof(object));
            n->kind = OBJ_STRING;
            n->value.string = name;
            PUSH(n); /* Push name of string object */
            break;
        }
        case SE_ASS_ADD:
        case SE_ASS_SUB:
        case SE_ASS_MUL:
        case SE_ASS_DIV:
        case SE_ASS_SUR: { /* N */
            char *name = GET_NAME();
            object *val = POP();
            object *obj = POP();

            /* Frame of whole */
            frame *f = (frame *)obj->value.cl.fr;

            void *ptr = get_table(f->tb, name);
            if (ptr == NULL) {
                undefined_error(name);
            }

            u_int8_t op; /* Translate operator */
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

            void *r = binary_op(op, (object *)ptr, val); /* To binary */
            if (r == NULL) {
                unsupport_operand_error(code_string[code]);
            }
            add_table(f->tb, name, (object *)r); /* Restore */
            break;
        }
        case TO_RET:
        case RET_OF: {                                /* x */
            vst.ip = back_frame()->code->codes->item; /* Catch */
            vst.loop_ret = true;
            if (code == RET_OF) { /* Return value */
                if (GET_PR_CODE() == FUNCTION) {
                    back_frame()->ret = GET_PR_OBJ(); /* Grammar sugar */
                } else {
                    back_frame()->ret = POP();
                }
            }
            break;
        }
        case USE_MOD: { /* N */
            load_module(GET_NAME());
            break;
        }
        default: { /* Unreachable */
            fprintf(stderr, "\033[1;31mvm %d:\033[0m unreachable '%s'.\n",
                    GET_LINE(), code_string[code]);
        }
        }
    next:
        vst.ip++; /* IP */
    }
}

/* Eval */
vm_state evaluate(code_object *code, char *filename) {
    /* Set virtual machine state */
    vst.frame = new_keg();
    vst.ip = 0;
    vst.op = 0;
    vst.filename = filename;
    vst.whole = NULL;

    /* main frame */
    frame *main = new_frame(code);
    vst.frame = append_keg(vst.frame, main);

    eval(); /* GO */
    return vst;
}

/* Returns the file name of path string */
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

/* Is file name equal others */
bool filename_eq(char *a, char *b) {
    int i = 0;
    for (; a[i] != '.'; i++) {
        if (a[i] == '\0' || b[i] != a[i]) {
            return false;
        }
    }
    return i == strlen(b);
}

/* Read files in path and directory */
keg *read_path(keg *pl, char *path) {
    DIR *dir;
    struct dirent *p;
    if ((dir = opendir(path)) == NULL) {
        printf("%s\n", path);
        simple_error("failed to open the std library or current directory");
    }
    while ((p = readdir(dir)) != NULL) {
        if (p->d_type == 4 && strcmp(p->d_name, ".") != 0 &&
            strcmp(p->d_name, "..") != 0) {
            /* Directory */
            // printf("load directory\n");
        }
        if (p->d_type == 8) { /* File */
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

/* Evaluate loaded module and set into the table items */
void load_eval(const char *path, char *name) {
    FILE *fp = fopen(path, "r"); /* Open file of path */
    if (fp == NULL) {
        printf("\033[1;31mvm %d:\033[0m failed to read buffer of file '%s'\n",
               GET_LINE(), path);
        exit(EXIT_FAILURE);
    }
    fseek(fp, 0, SEEK_END);
    int fsize = ftell(fp); /* Returns the size of file */
    fseek(fp, 0, SEEK_SET);
    char *buf = malloc(fsize + 1);

    fread(buf, fsize, sizeof(char), fp); /* Read file to buffer*/
    buf[fsize] = '\0';

    int16_t ip_up = vst.ip; /* Backup state */
    int16_t op_up = vst.op;
    keg *fr_up = vst.frame;

    keg *tokens = lexer(buf, fsize); /* To evaluate */
    free(buf);

    keg *codes = compile(tokens);

    vm_state ps = evaluate(codes->data[0], get_filename(path));

    frame *fr = (frame *)ps.frame->data[0]; /* Global frame */
    table *tb = fr->tb;                     /* Global table environment */

    object *obj = malloc(sizeof(object)); /* Module object */
    obj->kind = OBJ_MODULE;
    obj->value.mod.tb = (struct table *)tb;

    vst.ip = ip_up; /* Reset state */
    vst.op = op_up;
    vst.frame = fr_up;

    add_table(back_frame()->tb, name, obj); /* Module object*/
    free_keg(codes);

    fclose(fp);
    free_tokens(tokens);
}

/* Load module by name
   Load the FTPATH directory first, and then load the project directory */
void load_module(char *name) {
    bool ok = false;
    if (filename_eq(vst.filename, name)) {
        simple_error("cannot reference itself"); /* Current file */
    }
    keg *pl = new_keg();
    void *path = getenv("FTPATH");
    if (path != NULL) {
        pl = read_path(pl, (char *)path); /* Standard library */
    }
    pl = read_path(pl, "."); /* Current path */
    for (int i = 0; i < pl->item; i++) {
        char *addr = (char *)pl->data[i];
        if (filename_eq(get_filename(addr), name)) {
            load_eval(addr, name); /* To eval */
            ok = true;
            break;
        }
    }
    if (!ok) { /* Not found module */
        fprintf(stderr, "\033[1;31mvm %d:\033[0m undefined module '%s'.\n",
                GET_LINE(), name);
        exit(EXIT_FAILURE);
    } else {
        free_keg(pl);
    }
}