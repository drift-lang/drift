#include "../src/vm.h"

extern int code_argc;
extern char **code_argv;

reg_mod *sys() {
    reg_mod *m = new_mod("sys");
    emit_member(m, "argc", C_VAR);
    emit_member(m, "argv", C_VAR);
    return m;
}

void argc() {
    push_stack(new_num(code_argc - 1));
}

void argv() {
    object *list = malloc(sizeof(object));
    list->kind = OBJ_ARRAY;
    list->value.arr.element = new_keg();

    type *T = malloc(sizeof(type));
    T->kind = T_STRING;
    list->value.arr.T = T;

    keg *elem = list->value.arr.element;
    *code_argv++;

    while (*code_argv) {
        object *str = malloc(sizeof(object));

        str->kind = OBJ_STRING;
        str->value.str = *code_argv++;

        elem = append_keg(elem, str);
    }
    printf("%s\n", obj_string(list));
    push_stack(list);
}

static const char *mods[] = {"sys", NULL};

void init() {
    reg_c_mod(mods);
}