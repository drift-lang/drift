/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#include "vm.h"

/* Virtual machine state */
vm_state state;

/* Create a frame object */
frame *new_frame(code_object *code) {
    frame *f = (frame *) malloc(sizeof(frame));
    f->code = code;
    f->data = new_list();
    f->ret = NULL;
    f->tb = new_table();
    return f;
}

/* Release frame struct */
void free_frame(frame *f) {
    // code_object *code = f->code;
    // free(code->description);
    // free_list(code->names);
    // free_list(code->codes);
    // free_list(code->offsets);
    // free_list(code->types);
    // free_list(code->objects);
    // free_list(code->lines);
    // free_table(f->tb);
    // free_list(f->data);
    // free(f->ret);
    // free(f->module);
    // free(f);
}

/* Return font frame object */
frame *top_frame() {
    return back_list(state.frame);
}

/* Return main frame object */
frame *main_frame() {
    return (frame *)state.frame->data[0];
}

/* Return code object of front frame */
code_object *top_code() {
    frame *f = top_frame();
    return f->code;
}

/* Return data stack of front frame */
list *top_data() {
    frame *f = top_frame();
    return f->data;
}

/* Push data stack */
#define PUSH(obj) top_frame()->data \
    = append_list(top_frame()->data, obj)

/* Pop data stack */
#define POP() (object *) pop_back_list(top_frame()->data)

#define GET_OFF()  *(int16_t *) top_code()->offsets->data[state.op ++]

/* Data acquisition of code object */
#define GET_NAME() (char *)    top_code()->names->data[GET_OFF()]
#define GET_TYPE() (type *)    top_code()->types->data[GET_OFF()]
#define GET_OBJ()  (object *)  top_code()->objects->data[GET_OFF()]

#define GET_LINE() *(int *)      top_code()->lines->data[state.ip]
#define GET_CODE() *(u_int8_t *) top_code()->codes->data[state.ip]

#define GET_PR_CODE() *(u_int8_t *) top_code()->codes->data[state.ip - 2]
#define GET_PR_OBJ()  (object *)    top_code()->objects->data[ \
    (* (int16_t *)top_code()->offsets->data[state.op - 1]) \
] \

/* Undefined error */
void undefined_error(char *name) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m undefined name '%s'.\n",
        GET_LINE(), name);
    exit(EXIT_FAILURE);
}

/* Type error */
void type_error(type *T, object *obj) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m expect type %s, but found %s.\n",
        GET_LINE(), type_string(T), obj_string(obj));
    exit(EXIT_FAILURE);
}

/* Operand error */
void unsupport_operand_error(const char *code) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m unsupport operand to '%s'.\n",
        GET_LINE(), code);
    exit(EXIT_FAILURE);
}

/* Make simple error message */
void simple_error(const char *msg) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m %s.\n", GET_LINE(), msg);
    exit(EXIT_FAILURE);
}

/* Builtin function: print(object...) */
void print(frame *f, list *arg) {
    if (arg->len == 0) {
        printf("\n");
        return;
    }
    for (int i = arg->len - 1; i >= 0; i --) {
        printf("%s\t", obj_raw_string((object *)arg->data[i]));
    }
    printf("\n");
}

/* Builtin function: len(object) -> int */
void len(frame *f, list *arg) {
    if (arg->len != 1) {
        simple_error("'len' function require an argument");
    }
    object *obj = (object *)pop_back_list(arg);
    if (obj->kind != OBJ_STRING) {
        simple_error("only string length can be obtained");
    }
    object *len = (object *)malloc(sizeof(object));
    len->kind = OBJ_INT;
    len->value.integer = strlen(obj->value.string);
    PUSH(len);
}

/* Builtin function: typeof(object) -> string */
void ob_type(frame *f, list *arg) {
    if (arg->len != 1) {
        simple_error("'typeof' function require an argument");
    }
    object *str = (object *)malloc(sizeof(object));
    str->kind = OBJ_STRING;
    str->value.string =
        (char *)obj_type_string((object *)pop_back_list(arg));
    PUSH(str);
}

/* Builtin function: sleep(int) */
void s_sleep(frame *f, list *arg) {
    if (arg->len != 1) {
        simple_error("'typeof' function require an argument");
    }
    object *obj = (object *)pop_back_list(arg);
    if (obj->kind != OBJ_INT) {
        simple_error("'typeof' function receive an integer object");
    }
#if defined(__linux__) || defined(__APPLE__)
    sleep(obj->value.integer / 1000);
#elif defined(_WIN32)
    Sleep(obj->value.integer);
#endif
}

/* Builtin function list */
builtin bts[] = {
    { "print",  print   },
    { "len",    len     },
    { "typeof", ob_type },
    { "sleep",  s_sleep },
};

/* Get builtin function with name */
object *get_builtin(char *name) {
    int p = -1;
    for (int i = 0;
        i < sizeof(bts) / sizeof(bts[0]); i ++) {
            if (strcmp(bts[i].name, name) == 0) {
                p = i;
                break;
            }
        }
    if (p != -1) { /* Placeholder object */
        object *f = (object *)malloc(sizeof(object));
        f->kind = OBJ_FUNC;
        f->value.func.name = name;
        return f;
    } else {
        return NULL;
    }
}

/* Jumps to the specified bytecode position and
   moves the offset */
void jump(int16_t to) {
    state.op --; /* Step back the offset of the current jump */
    bool reverse = state.ip > to;
    if (reverse)
        /* Inversion skip the jump bytecode */
        state.ip --;
    while (
        reverse ?
            state.ip >= to : /* Condition */
            state.ip < to
    ) {
        switch (GET_CODE()) {
            case CONST_OF:   case LOAD_OF:    case LOAD_ENUM:  case LOAD_FUNC:
            case LOAD_FACE:  case ASSIGN_TO:  case GET_OF:     case SET_OF:
            case CALL_FUNC:  case ASS_ADD:    case ASS_SUB:    case ASS_MUL:
            case ASS_DIV:    case ASS_SUR:    case SE_ASS_ADD: case SE_ASS_SUB:
            case SE_ASS_MUL: case SE_ASS_DIV: case SE_ASS_SUR: case SET_NAME:
            case USE_MOD:    case BUILD_ARR:  case BUILD_TUP:  case BUILD_MAP:
            case JUMP_TO:    case T_JUMP_TO:  case F_JUMP_TO:
                /* These bytecodes contain a parameter */
                reverse ?
                    state.op -- : state.op ++;
                break;
            case STORE_NAME: /* Two parameter */
            case NEW_OBJ:
                if (reverse)
                    state.op -= 2;
                else
                    state.op += 2;
                break;
        }
        reverse ? /* Set bytecode position */
            state.ip -- : state.ip ++;
    }
    if (!reverse) /* Loop update */
        state.ip --;
}

/* Find object in overlay frames */
void *lookup(char *name) {
    for (int i = state.frame->len - 1; i >= 0; i --) {
        frame *f = (frame *)state.frame->data[i];
        void *ptr = get_table(f->tb, name);
        if (ptr != NULL) {
            return ptr;
        }
    }
    return NULL;
}

/* Evaluate */
void eval() {
    while (state.ip < top_code()->codes->len) { /* Traversal bytecode list */
        u_int8_t code = GET_CODE(); /* Bytecode */
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
                if ( /* Store and object type check */
                    !type_checker(T, obj)) {
                        type_error(T, obj);
                    }
                if (obj->kind == OBJ_ARR) { /* Sets the type specified */
                    obj->value.arr.T = (type *)T->inner.single;
                }
                else if (obj->kind == OBJ_TUP) { /* Tuple */
                    obj->value.tup.T = (type *)T->inner.single;
                }
                else if (obj->kind == OBJ_MAP) { /* Map */
                    obj->value.map.T1 = (type *)T->inner.both.T1;
                    obj->value.map.T2 = (type *)T->inner.both.T2;
                }
                /* Interface */
                object *new = (object *)malloc(sizeof(object));
                memcpy(new, obj, sizeof(object));
                /* Add to current table */
                add_table(top_frame()->tb, name, new);
                break;
            }
            case LOAD_OF: { /* N */
                char *name = GET_NAME();
                void *resu = lookup(name); /* Lookup frames */
                if (resu == NULL) {
                    if (state.whole != NULL && state.frame->len > 1) {
                        frame *f = (frame *)state.whole->value.whole.fr;
                        resu = get_table(f->tb, name); /* Set to whole frame */
                        if (resu == NULL)
                            resu = get_builtin(name);
                        else
                            state.whole = NULL;
                    }
                    /* Builtin function */
                    else {
                        resu = get_builtin(name);
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
                if (!exist(top_frame()->tb, name)) { /* Get */
                    undefined_error(name);
                }
                object *ori = (object *)get_table(top_frame()->tb, name);
                if (!obj_kind_eq(ori, obj)) {
                    simple_error("inconsistent type");
                }
                if (ori->kind == OBJ_FACE) {
                    if (obj->kind != OBJ_WHOLE) {
                        simple_error("interface needs to be assigned by whole");
                    }
                    if (obj->value.whole.init == false) {
                        simple_error("whole is not initialized");
                    }
                    list *elem =
                        ori->value.face.element; /* Face method */
                    frame *fr = (frame *) obj->value.whole.fr;
                    for (int i = 0; i < elem->len; i ++) {
                        method *m =
                            (method *)elem->data[i]; /* Traverse the node and check the type */
                        void *p = get_table(fr->tb, m->name);
                        if (p == NULL) {
                            simple_error("whole does not contain some member");
                        }
                        object *fn = (object *)p; /* Whole object inner */
                        if (fn->kind != OBJ_FUNC) {
                            simple_error("an interface can only be a method");
                        }
                        if ( /* Check return type */
                            !type_eq(m->ret, fn->value.func.ret)) {
                                simple_error("return type in the method are inconsistent");
                        } /* Check function arguments */
                        if (m->T->len != fn->value.func.v->len) {
                            simple_error("inconsistent method arguments");
                        } /* Check function arguments type */
                        for (int j = 0; j < m->T->len; j ++) {
                            type *a = (type *)m->T->data[j];
                            type *b = (type *)fn->value.func.v->data[j];
                            if (!type_eq(a, b)) {
                                simple_error("Inconsistent types of method arguments");
                            }
                        }
                    }
                    /* Store whole field into face object */
                    ori->value.face.whole = (struct object *)obj;
                    break;
                }
                /* Update the content of the table */
                add_table(top_frame()->tb, name, obj);
                break;
            }
            case TO_ADD:   case TO_SUB:    case TO_MUL: case TO_DIV:   case TO_SUR:
            case TO_GR:    case TO_GR_EQ:  case TO_LE:  case TO_LE_EQ: /* x */
            case TO_EQ_EQ: case TO_NOT_EQ: case TO_AND: case TO_OR: {
                object *b = POP(); /* P2 */
                object *a = POP(); /* P1 */
                void *r = binary_op(code, a, b);
                if (r == NULL) { /* Operand error */
                    unsupport_operand_error(code_string[code]);
                }
                PUSH((object *)r);
                break;
            }
            case ASS_ADD: case ASS_SUB: case ASS_MUL: case ASS_DIV: case ASS_SUR: { /* N */
                char *name = GET_NAME();
                object *obj = POP();
                if (!exist(top_frame()->tb, name)) {
                    undefined_error(name); /* Get name */
                }
                object *resu = (object *)
                    get_table(top_frame()->tb, name); /* Get name */
                u_int8_t op;
                if (code == ASS_ADD) op = TO_ADD; if (code == ASS_SUB) op = TO_SUB;
                if (code == ASS_MUL) op = TO_MUL; if (code == ASS_DIV) op = TO_DIV;
                if (code == ASS_SUR) op = TO_SUR;
                void *r =  binary_op(op, (object *)resu, obj); /* To operation */
                if (r == NULL) {
                    unsupport_operand_error(code_string[code]);
                }
                /* Update */
                add_table(top_frame()->tb, name, (object *)r);
                break;
            }
            case BUILD_ARR: { /* F */
                int16_t count = GET_OFF();
                object *obj = (object *)malloc(sizeof(object));
                obj->kind = OBJ_ARR;
                obj->value.arr.element = new_list();
                if (count == 0) {
                    PUSH(obj);
                    break;
                }
                while (count > 0) { /* Build data */
                    append_list(obj->value.arr.element, POP());
                    count --;
                }
                PUSH(obj);
                break;
            }
            case BUILD_TUP: { /* F */
                int16_t count = GET_OFF();
                object *obj = (object *)malloc(sizeof(object));
                obj->kind = OBJ_TUP;
                obj->value.tup.element = new_list();
                if (count == 0) {
                    PUSH(obj);
                    break;
                }
                while (count > 0) {
                    append_list(obj->value.tup.element, POP());
                    count --;
                }
                PUSH(obj);
                break;
            }
            case BUILD_MAP: { /* N */
                int16_t count = GET_OFF();
                object *obj = (object *)malloc(sizeof(object));
                obj->kind = OBJ_MAP;
                obj->value.map.k = new_list(); /* K */
                obj->value.map.v = new_list(); /* V */
                if (count == 0) {
                    PUSH(obj);
                    break;
                }
                while (count > 0) {
                    object *b = POP();
                    object *a = POP();
                    append_list(obj->value.map.k, a); /* Key */
                    append_list(obj->value.map.v, b); /* Value */
                    count -= 2;
                }
                PUSH(obj);
                break;
            }
            case TO_INDEX: { /* x */
                object *p = POP();
                object *obj = POP();
                if (obj->kind != OBJ_ARR && obj->kind != OBJ_MAP) { /* Array and Map */
                    simple_error("only array and map can be called with subscipt");
                }
                if (obj->kind == OBJ_ARR) { /* Array */
                    if (p->kind != OBJ_INT) {
                        simple_error("get value using integer subscript"); /* Index only integer */
                    }
                    if (obj->value.arr.element->len == 0) { /* None elements */
                        simple_error("array entry is empty");
                    }
                    PUSH(
                        (object *)obj->value.arr.element->data[
                            p->value.integer /* Get indexes add to table */
                        ]);
                }
                if (obj->kind == OBJ_MAP) { /* Map */
                    if (obj->value.map.k->len == 0) {
                        simple_error("map entry is empty");
                    }
                    int i = 0; /* Traverse the key, find the corresponding,
                        and change the variable */
                    for (; i < obj->value.map.k->len; i ++) {
                        if (obj_eq( /* Object equal compare */
                            p, (object *)obj->value.map.k->data[i])) {
                                PUSH(
                                    (object *)obj->value.map.v->data[i]);
                                break;
                            }
                    }
                    /* Not found */
                    if (i == obj->value.map.k->len) {
                        simple_error("the map does not have this key");
                    }
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
                    if (!type_checker(j->value.arr.T, obj)) { /* Inconsistent type */
                        type_error(j->value.arr.T, obj);
                    }
                    int p = idx->value.integer; /* Indexes */
                    if (j->value.arr.element->len == 0) {
                        /* Insert empty list directly */
                        insert_list(
                            j->value.arr.element, 0, obj);
                    } else {
                        /* Index out of bounds */
                        if (p > j->value.arr.element->len - 1) {
                            simple_error("index out of bounds");
                        }
                        /* Replace data */
                        replace_list(j->value.arr.element, p, obj);
                    }
                }
                if (j->kind == OBJ_MAP) { /* Map */
                    /* Type check of K and V */
                    if (!type_checker(j->value.map.T1, idx)) type_error(j->value.map.T1, idx);
                    if (!type_checker(j->value.map.T2, obj)) type_error(j->value.map.T2, obj);
                    int p = -1; /* Find the location of the key */
                    for (int i = 0; i < j->value.map.k->len; i ++) {
                        if (obj_eq(
                            idx, (object *)j->value.map.k->data[i])) {
                                p = i;
                                break;
                        }
                    }
                    if (p == -1) { /* Insert empty list directly */
                        insert_list(j->value.map.k, 0, idx);
                        insert_list(j->value.map.v, 0, obj);
                    } else {
                        replace_list(j->value.map.v, p, obj); /* Replace data */
                    }
                }
                break;
            }
            case TO_REP_ADD: case TO_REP_SUB: case TO_REP_MUL: case TO_REP_DIV:
            case TO_REP_SUR: { /* x */
                object *obj = POP();
                object *idx = POP();
                object *j = POP();
                u_int8_t op;
                if (code == TO_REP_ADD) op = TO_ADD; if (code == TO_REP_SUB) op = TO_SUB;
                if (code == TO_REP_MUL) op = TO_MUL; if (code == TO_REP_DIV) op = TO_DIV;
                if (code == TO_REP_SUR) op = TO_SUR;
                if (j->kind == OBJ_ARR) { /* Array */
                    if (idx->kind != OBJ_INT) {
                        simple_error("get value using integer subscript");
                    }
                    int p = idx->value.integer; /* Indexes */
                    if (j->value.arr.element->len == 0 || p < 0 || p > j->value.arr.element->len - 1) {
                        simple_error("index out of bounds");
                    }
                    void *r = binary_op( /* To operand */
                        op,
                        (object *)j->value.arr.element->data[p], obj);
                    if (r == NULL) {
                        /* Operand error */
                        unsupport_operand_error(code_string[code]);
                    }
                    replace_list(j->value.arr.element, p, (object *)r); /* Replace data */
                }
                if (j->kind == OBJ_MAP) { /* Map */
                    /* Type check of K and V */
                    if (!type_checker(j->value.map.T1, idx)) type_error(j->value.map.T1, idx);
                    if (!type_checker(j->value.map.T2, obj)) type_error(j->value.map.T2, obj);
                    if (j->value.map.k->len == 0) {
                        simple_error("map entry is empty");
                    }
                    int p = -1; /* Find the location of the key */
                    for (int i = 0; i < j->value.map.k->len; i ++) {
                        if (obj_eq(
                            idx, (object *)j->value.map.k->data[i])) {
                                p = i;
                                break;
                        }
                    }
                    if (p == -1) { /* Insert empty list directly */
                        simple_error("no replace key exist");
                    }
                    object *r = binary_op(op,
                        (object *)j->value.map.v->data[p], obj);
                    if (r == NULL) {
                        unsupport_operand_error(code_string[code]);
                    }
                    replace_list(j->value.map.v, p, r); /* Replace date */
                }
                break;
            }
            case TO_BANG: { /* !P */
                object *obj = POP();
                object *new =
                    (object *)malloc(sizeof(object)); /* Alloc new */
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
                object *new =
                    (object *)malloc(sizeof(object)); /* Alloc new */
                if (obj->kind == OBJ_INT) {
                    new->value.integer = -obj->value.integer;
                    PUSH(new);
                    break;
                }
                if (obj->kind == OBJ_FLOAT) {
                    new->value.floating = -obj->value.floating;
                    PUSH(new);
                    break;
                }
                unsupport_operand_error(code_string[code]);
                break;
            }
            case JUMP_TO:
            case F_JUMP_TO:
            case T_JUMP_TO: { /* F */
                int16_t off = GET_OFF(); /* Offset */
                if (code == JUMP_TO) {
                    jump(off); /* Just jump */
                    break;
                }
                bool ok = /* Condition */
                    ((object *)POP())->value.boolean;
                if (code == T_JUMP_TO && ok)
                    jump(off);
                if (code == F_JUMP_TO && ok == false)
                    jump(off);
                break;
            }
            case LOAD_FUNC: { /* J */
                object *obj = GET_OBJ();
                add_table(top_frame()->tb,
                    obj->value.func.name, obj); /* Func */
                break;
            }
            case CALL_FUNC: { /* F */
                int16_t off = GET_OFF();
                list *arg = new_list();
                while (off > 0) { /* Skip function object */
                    if (top_frame()->data->len - 1 <= 0) {
                        simple_error("stack data error");
                    }
                    append_list(arg, POP());
                    off --;
                }

                object *func = POP(); /* Function */
                for (int i = 0;
                    i < sizeof(bts) / sizeof(bts[0]); i ++) {
                        if (strcmp(bts[i].name, func->value.func.name) == 0) {
                            bts[i].func(top_frame(), arg); /* Call builtin function */
                            goto next;
                        }
                }

                list *k = func->value.func.k;
                list *v = func->value.func.v;

                if (k->len != arg->len) {
                    simple_error("inconsistent funtion arguments");
                }
                /* Call frame */
                frame *f = new_frame(func->value.func.code); /* Func code object */

                /* Check arguments */
                for (int i = 0; i < k->len; i ++) {
                    char *N = (char *)k->data[i]; /* Name */
                    type *T = (type *)v->data[i]; /* Type */
                    object *p = arg->data[-- arg->len]; /* Reverse */
                    if (
                        !type_checker(T, p) /* Check type consistent */
                    ) type_error(T, p);
                    /* Add to table of frame */
                    add_table(f->tb, N, p);
                }

                /* Backup pointer */
                int16_t op_up = state.op;
                int16_t ip_up = state.ip;

                state.op = 0; /* Clear */
                state.ip = 0;
                state.frame = append_list(state.frame, f);
                eval();

                frame *p = (frame *)pop_back_list(state.frame); /* Evaluated */

                /* Function return */
                if (func->value.func.ret != NULL) {
                    if (p->ret == NULL ||
                            !type_checker(func->value.func.ret, p->ret)) {
                                if (p->ret == NULL) {
                                    simple_error("function missing return value");
                                }
                        type_error(func->value.func.ret, p->ret); /* Ret type */
                    }
                    PUSH(p->ret);
                }

                free_list(p->data);
                free_table(p->tb);

                state.op = op_up; /* Reset pointer to main */
                state.ip = ip_up;
                break;
            }
            case LOAD_FACE: { /* J */
                object *obj = GET_OBJ();
                add_table(top_frame()->tb,
                    obj->value.face.name, obj); /* Face */
                break;
            }
            case LOAD_ENUM: { /* J */
                object *obj = GET_OBJ();
                add_table(top_frame()->tb,
                    obj->value.enumeration.name, obj); /* Enum */
                break;
            }
            case GET_OF: { /* N */
                char *name = GET_NAME();
                object *obj = POP();
                if (obj->kind != OBJ_ENUM &&
                    obj->kind != OBJ_WHOLE && obj->kind != OBJ_TUP && obj->kind != OBJ_FACE) {
                        simple_error("only enum, face, tuple and whole type are supported");
                }
                if (obj->kind == OBJ_ENUM) { /* Enum */
                    list *elem = obj->value.enumeration.element;
                    /* Enumeration get integer */
                    object *p = (object *)malloc(sizeof(object));
                    p->kind = OBJ_INT;
                    p->value.integer = -1; /* Origin value */
                    for (int i = 0; i < elem->len; i ++) {
                        if (
                            strcmp(name, (char *)elem->data[i]) == 0) {
                                p->value.integer = i;
                                break;
                        }
                    }
                    PUSH(p);
                }
                if (obj->kind == OBJ_WHOLE) { /* Whole */
                    if (obj->value.whole.init == false) {
                        simple_error("whole did not load initialization members");
                    }
                    frame *fr = (frame *)obj->value.whole.fr;
                    void *ptr = get_table(fr->tb, name);
                    if (ptr == NULL) {
                        simple_error("nonexistent member");
                    }
                    state.whole = obj; /* Record the whole that is currently invoke */
                    PUSH((object *)ptr);
                }
                if (obj->kind == OBJ_TUP) { /* Tuple */
                    if (obj->value.tup.element->len == 0) {
                        simple_error("tuple entry is empty");
                    }
                    for (int i = 0; i < strlen(name); i ++) {
                        if ( /* */
                            !(name[i] >= '0' && name[i] <= '9')) {
                                simple_error("subscript can only be obtained with integer");
                        }
                    }
                    /* Index */
                    int idx = atoi(name); /* Index is reversed */
                    int p =
                        obj->value.tup.element->len - 1 - idx; /* Previous index */
                    if (idx >= obj->value.tup.element->len) {
                        simple_error("index out of bounds");
                    }
                    PUSH((object *)obj->value.tup.element->data[p]);
                }
                if (obj->kind == OBJ_FACE) { /* Face */
                    if (obj->value.face.whole == NULL) {
                        simple_error("interface is not initialized");
                    }
                    list *elem =
                        obj->value.face.element; /* Face method */
                    for (int i = 0; i < elem->len; i ++) {
                        method *m =
                            (method *)elem->data[i];
                        if (strcmp(m->name, name) == 0) {
                            object *wh =
                                (object *)obj->value.face.whole; /* Get whole object */
                            frame *fr =
                                (frame *)wh->value.whole.fr; /* Get frame in whole */
                            state.whole = wh;
                            PUSH(
                                (object *)get_table(fr->tb, name));
                            goto next;
                        }
                    }
                    simple_error("nonexistent member");
                }
                break;
            }
            case SET_OF: { /* N */
                char *name = GET_NAME();
                object *val = POP(); /* Will set value */
                object *obj = POP();
                if (obj->kind != OBJ_WHOLE) {
                    simple_error("only members of whole can be set");
                }
                frame *fr = (frame *)obj->value.whole.fr; /* Whole frame */
                void *ptr = get_table(fr->tb, name);
                if (ptr == NULL) {
                    simple_error("nonexistent member");
                }
                object *j = (object *)ptr; /* Origin value */
                if (val->kind != j->kind) {
                    simple_error("wrong type set");
                }
                if (!basic(j)) /* Check basic object */
                    simple_error("only members of basic objects can be assign");

                if (!obj_kind_eq(obj, val))
                    simple_error("inconsistent type");

                set_table(fr->tb, name, val); /* Restore */
                break;
            }
            case LOAD_WHOLE: { /* J */
                object *obj = GET_OBJ();
                add_table(top_frame()->tb,
                    obj->value.whole.name, obj); /* Whole */
                break;
            }
            case NEW_OBJ: { /* N F*/
                char *name = GET_NAME();
                int16_t count = GET_OFF();
                void *wh = get_table(top_frame()->tb, name); /* Get whole */
                if (wh == NULL) {
                    undefined_error(name);
                }

                object *obj = (object *)wh; /* To whole */
                object *new = (object *)malloc(sizeof(object)); /* Copy */
                memcpy(new, obj, sizeof(object));

                /* Evaluate frame of whole */
                new->value.whole.fr =
                    (struct frame *)new_frame(obj->value.whole.code);
                /* Backup pointer */
                int16_t op_up = state.op;
                int16_t ip_up = state.ip;

                state.op = 0;
                state.ip = 0;
                state.frame = append_list(state.frame, new->value.whole.fr);
                eval(); /* Evaluate */

                pop_back_list(state.frame); /* Pop */

                state.op = op_up; /* Reset pointer */
                state.ip = ip_up;

                frame *f = (frame *)new->value.whole.fr;

                /* Initialize member */
                while (count > 0) {
                    object *y = POP(); /* Get value */
                    object *x = POP();
                    void *ptr = get_table(f->tb, x->value.string);
                    if (ptr == NULL) {
                        undefined_error(name); /* Get key */
                    }
                    add_table(f->tb, x->value.string, y);
                    count -= 2;
                }

                new->value.whole.init = true; /* Whole object */
                PUSH(new);
                break;
            }
            case SET_NAME: { /* N */
                char *name = GET_NAME();
                object *n = (object *)malloc(sizeof(object));
                n->kind = OBJ_STRING;
                n->value.string = name;
                PUSH(n); /* Push name of string object */
                break;
            }
            case SE_ASS_ADD: case SE_ASS_SUB: case SE_ASS_MUL: case SE_ASS_DIV:
            case SE_ASS_SUR: { /* N */
                char *name = GET_NAME();
                object *val = POP();
                object *obj = POP();

                /* Frame of whole */
                frame *f = (frame *)obj->value.whole.fr;

                void *ptr = get_table(f->tb, name);
                if (ptr == NULL) {
                    undefined_error(name);
                }

                u_int8_t op; /* Translate operator */
                if (code == SE_ASS_ADD) op = TO_ADD; if (code == SE_ASS_SUB) op = TO_SUB;
                if (code == SE_ASS_MUL) op = TO_MUL; if (code == SE_ASS_DIV) op = TO_DIV;
                if (code == SE_ASS_SUR) op = TO_SUR;

                void *r = binary_op(op, (object *)ptr, val); /* To binary */
                if (r == NULL) {
                    unsupport_operand_error(code_string[code]);
                }
                set_table(f->tb, name, (object *)r); /* Restore */
                break;
            }
            case TO_RET:
            case RET_OF: { /* x */
                state.ip = top_frame()->code->codes->len; /* Catch */
                state.loop_ret = true;
                if (code == RET_OF) { /* Return value */
                    if (GET_PR_CODE() == LOAD_FUNC) {
                        top_frame()->ret = GET_PR_OBJ(); /* Grammar sugar */
                    } else {
                        top_frame()->ret = POP();
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
        state.ip ++; /* IP */
    }
}

/* Eval */
vm_state evaluate(code_object *code, char *filename) {
    /* Set virtual machine state */
    state.frame = new_list();
    state.ip = 0;
    state.op = 0;
    state.filename = filename;

    /* main frame */
    frame *main = new_frame(code);
    state.frame = append_list(state.frame, main);

    eval(); /* GO */
    return state;
}

char *get_filename(const char *p) {
    char *name = (char *)malloc(sizeof(char) * 64);
    int j = 0;
    for (int i = 0; p[i]; i ++)
        if (p[i] == '/') j = i;
    strcpy(name, &p[j]);
    return name;
}

bool filename_eq(char *a, char *b) {
    int i = 0;
    for (; a[i] != '.'; i ++) {
        if (a[i] == '\0' || b[i] != a[i]) {
            return false;
        }
    }
    return i == strlen(b);
}

list *read_path(const char *path) {
    DIR *dir;
    struct dirent *p;
    list *pl = new_list();
    if ((dir = opendir(path)) == NULL) {
        simple_error("failed to open the std library directory  ");
    }
    while ((p = readdir(dir)) != NULL) {
        /* if (p->d_type == 4 &&
            strcmp(p->d_name, ".") != 0 && strcmp(p->d_name, "..") != 0) {
        } */
        if (p->d_type == 8) { /* File */
            char *name = p->d_name;
            int len = strlen(name) - 1;
            if (name[len] == 't' &&
                name[len - 1] == 'f' &&
                name[len - 2] == '.') {
                    pl = append_list(pl, name);
                }
        }
    }
    return pl;
}

void load_eval(const char *path) {
    FILE *fp = fopen(path, "r"); /* Open file of path */
    if (fp == NULL) {
        printf("\033[1;31merror:\033[0m failed to read buffer of file: %s.\n", path);
        exit(EXIT_FAILURE);
    }
    fseek(fp, 0, SEEK_END);
    int fsize = ftell(fp); /* Returns the size of file */
    fseek(fp, 0, SEEK_SET);
    char *buf = (char *)malloc(fsize * sizeof(char));

    fread(buf, fsize, sizeof(char), fp); /* Read file to buffer*/
    buf[fsize] = '\0';

    list *tokens = lexer(buf, fsize);
    list *codes = compile(tokens);
    vm_state state = evaluate(codes->data[0], get_filename(path));

    printf("END\n");

    free(buf);
    free_list(tokens);
    free_list(codes);

    free(state.filename);
}

/* Load module by name */
void load_module(char *name) {
    void *path = getenv("FT_PATH");
    if (path != NULL) {
        /* Standard library */
        list *pl = read_path((char *)path);
    } else {
        /* Current path */
        list *pl = read_path(".");
        for (int i = 0; i < pl->len; i ++) {
            char *pname = (char *)pl->data[i];
            if (filename_eq(state.filename, name)) {
                continue; /* Current file */
            }
            // if (filename_eq(pname, name) == 0) {
            //     return;
            // }
            printf("LOAD: %s\n", pname);
        }
    }
}