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

/* Make simple error message */
void simple_error(const char *msg) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m %s.\n", GET_LINE(), msg);
    exit(EXIT_FAILURE);
}

/* Operand error */
void unsupport_operand_error(const char *code) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m unsupport operand to '%s'.\n",
        GET_LINE(), code);
    exit(EXIT_FAILURE);
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
            case SET_MOD:    case USE_MOD:    case BUILD_ARR:  case BUILD_TUP:
            case BUILD_MAP:  case JUMP_TO:    case T_JUMP_TO:  case F_JUMP_TO:
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
                object *new = (object *)malloc(sizeof(object));
                memcpy(new, obj, sizeof(object));
                /* Add to current table */
                add_table(top_frame()->tb, name, new);
                break;
            }
            case LOAD_OF: { /* N */
                char *name = GET_NAME();
                void *resu = get_table(top_frame()->tb, name); /* Get table */
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
                if (!obj_kind_eq(
                        (object *)get_table(top_frame()->tb, name), obj)) {
                    simple_error("inconsistent type");
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
                object *r = binary_op(code, a, b);
                if (r == NULL) { /* Operand error */
                    unsupport_operand_error(code_string[code]);
                }
                PUSH(r);
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
                object *r =  binary_op(op, (object *)resu, obj); /* To operation */
                if (r == NULL) {
                    unsupport_operand_error(code_string[code]);
                }
                /* Update */
                add_table(top_frame()->tb, name, r);
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
                    object *r = binary_op( /* To operand */
                        op,
                        (object *)j->value.arr.element->data[p], obj);
                    if (r == NULL) {
                        /* Operand error */
                        unsupport_operand_error(code_string[code]);
                    }
                    replace_list(j->value.arr.element, p, r); /* Replace data */
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
                while (off > 0) {
                    append_list(arg, POP());
                    off --;
                }
                object *func = POP(); /* Function */
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
                if (obj->kind != OBJ_ENUM && obj->kind != OBJ_WHOLE && obj->kind != OBJ_TUP) {
                    simple_error("only enum, tuple and whole type are supported");
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
                    frame *fr = (frame *)obj->value.whole.fr;
                    void *ptr = get_table(fr->tb, name);
                    if (ptr == NULL) {
                        simple_error("nonexistent member");
                    }
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

                object *r = binary_op(op, (object *)ptr, val); /* To binary */
                if (r == NULL) {
                    unsupport_operand_error(code_string[code]);
                }
                set_table(f->tb, name, r); /* Restore */
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
            default: { /* Unreachable */
                fprintf(stderr, "\033[1;31mvm %d:\033[0m unreachable '%s'.\n",
                    GET_LINE(), code_string[code]);
            }
        }
        state.ip ++; /* IP */
    }
}

/* Eval */
vm_state evaluate(code_object *code) {
    /* Set virtual machine state */
    state.frame = new_list();
    state.ip = 0;
    state.op = 0;

    /* main frame */
    frame *main = new_frame(code);
    state.frame = append_list(state.frame, main);

    eval(); /* GO */
    return state;
}