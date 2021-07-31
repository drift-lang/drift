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
    return list_back(state.frame);
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
#define POP() (object *)pop_list_back(top_frame()->data)

/* Data acquisition of code object */
#define GET_NAME() (char *)    top_code()->names->data[state.op ++]
#define GET_TYPE() (type *)    top_code()->types->data[state.op ++]
#define GET_OBJ()  (object *)  top_code()->objects->data[state.op ++]
#define GET_OFF()  (int16_t *) top_code()->offsets->data[state.op ++]

#define GET_LINE() *(int *)      top_code()->lines->data[state.ip]
#define GET_CODE() *(u_int8_t *) top_code()->codes->data[state.ip]

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
            case TO_ADD:   case TO_SUB:    case TO_MUL: case TO_DIV:   case TO_SUR:
            case TO_GR:    case TO_GR_EQ:  case TO_LE:  case TO_LE_EQ: /* x */
            case TO_EQ_EQ: case TO_NOT_EQ: case TO_AND: case TO_OR: {
                object *b = POP(); /* P2 */
                object *a = POP(); /* P1 */
                object *r = binary_op(code, a, b);
                if (r == NULL) { /* Operand error */
                    fprintf(stderr,
                        "\033[1;31mvm %d:\033[0m unsupport type to '%s'.\n",
                        GET_LINE(), code_string[code]);
                    exit(EXIT_FAILURE);
                }
                PUSH(r);
                break;
            }
            case TO_RET: { /* x */
                if (top_data()->len != 0) {
                    printf("%s\n", obj_string(POP()));
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
