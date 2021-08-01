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

/* Undefined error */
void undefined_error(char *name) {
    fprintf(stderr, "\033[1;31mvm %d:\033[0m undefined name '%s'.\n",
        GET_LINE(), name);
    exit(EXIT_FAILURE);
}

/* Unsupport type to OP_? */
void unsupport_type_error(const char *code) {
    fprintf(stderr,
        "\033[1;31mvm %d:\033[0m unsupport type to '%s'.\n",
        GET_LINE(), code);
    exit(EXIT_FAILURE);
}

/* Inspection type */
void type_error() {
    fprintf(stderr,
        "\033[1;31mvm %d:\033[0m inspection type.\n", GET_LINE());
    exit(EXIT_FAILURE);
}

/* Data error */
void data_error() {
    fprintf(stderr,
        "\033[1;31mvm %d:\033[0m data error\n", GET_LINE());
    exit(EXIT_FAILURE);
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
                        type_error();
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
                /* Add to current table */
                add_table(top_frame()->tb, name, obj);
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
                /* Update the content of the table */
                set_table(top_frame()->tb, name, obj);
                break;
            }
            case TO_ADD:   case TO_SUB:    case TO_MUL: case TO_DIV:   case TO_SUR:
            case TO_GR:    case TO_GR_EQ:  case TO_LE:  case TO_LE_EQ: /* x */
            case TO_EQ_EQ: case TO_NOT_EQ: case TO_AND: case TO_OR: {
                object *b = POP(); /* P2 */
                object *a = POP(); /* P1 */
                object *r = binary_op(code, a, b);
                if (r == NULL) { /* Operand error */
                    unsupport_type_error(code_string[code]);
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
                    unsupport_type_error(code_string[code]);
                }
                /* Update */
                set_table(top_frame()->tb, name, r);
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
                while (count > 0) {
                    insert_list( /* Build data type */
                        obj->value.arr.element, 0, POP());
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
                    insert_list(
                        obj->value.tup.element, 0, POP());
                    count --;
                }
                PUSH(obj);
                break;
            }
            case BUILD_MAP: { /* N */
                int16_t count = GET_OFF();
                object *obj = (object *)malloc(sizeof(object));
                obj->kind = OBJ_MAP;
                obj->value.map.k = new_list();
                obj->value.map.v = new_list();
                if (count == 0) {
                    PUSH(obj);
                    break;
                }
                while (count > 0) {
                    object *b = POP();
                    object *a = POP();
                    insert_list(
                        obj->value.map.k, 0, a); /* Key */
                    insert_list(
                        obj->value.map.v, 0, b); /* Value */
                    count -= 2;
                }
                PUSH(obj);
                break;
            }
            case TO_INDEX: { /* x */
                object *p = POP();
                object *obj = POP();
                if (obj->kind != OBJ_ARR && obj->kind != OBJ_MAP) { /* Array and Map */
                    type_error();
                }
                if (obj->kind == OBJ_ARR) { /* Array */
                    if (p->kind != OBJ_INT) {
                        type_error(); /* Index only integer */
                    }
                    if (obj->value.arr.element->len == 0) { /* None elements */
                        data_error();
                    }
                    PUSH(
                        (object *)obj->value.arr.element->data[
                            p->value.integer /* Get indexes add to table */
                        ]);
                }
                if (obj->kind == OBJ_MAP) { /* Map */
                    if (obj->value.map.k->len == 0) {
                        data_error();
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
                        data_error();
                    }
                }
                break;
            }
            case TO_REPLACE: { /* x */
                object *obj = POP();
                object *idx = POP();
                object *j = POP();
                if (j->kind != OBJ_ARR && j->kind != OBJ_MAP) { /* Array and Map */
                    type_error();
                }
                if (j->kind == OBJ_ARR) { /* Array */
                    if (idx->kind != OBJ_INT) {
                        type_error();
                    }
                    if (!type_checker(j->value.arr.T, obj)) { /* Inconsistent type */
                        type_error();
                    }
                    int p = idx->value.integer; /* Indexes */
                    if (j->value.arr.element->len == 0) {
                        /* Insert empty list directly */
                        insert_list(
                            j->value.arr.element, 0, obj);
                    } else {
                        /* Index out of bounds */
                        if (p > j->value.arr.element->len - 1) {
                            data_error();
                        }
                        /* Replace data */
                        replace_list(j->value.arr.element, p, obj);
                    }
                }
                if (j->kind == OBJ_MAP) { /* Map */
                    if (
                        !type_checker(j->value.map.T1, idx) ||
                        !type_checker(j->value.map.T2, obj) /* Type check of K and V */
                    ) {
                        type_error();
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
                        insert_list(j->value.map.k, 0, idx);
                        insert_list(j->value.map.v, 0, obj);
                    } else {
                        replace_list(j->value.map.v, p, obj); /* Replace data */
                    }
                }
                break;
            }
            case TO_RET: { /* x */
                if (top_data()->len != 0) {
                    printf("%s\n", obj_raw_string(POP()));
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
void evaluate(code_object *code) {
    /* Set virtual machine state */
    state.frame = new_list();
    state.ip = 0;
    state.op = 0;

    /* main frame */
    frame *main = new_frame(code);
    state.frame = append_list(state.frame, main);

    eval(); /* GO */
}