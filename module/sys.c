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
    object *list = new_array(T_STRING);
    keg *elem = list->value.arr.element;

    for (int i = 1; i < code_argc; i++) {
        char *str = code_argv[i];
        elem = append_keg(elem, new_string(str));
    }
    push_stack(list);
}

static const char *mods[] = {"sys", NULL};

void init() {
    reg_c_mod(mods);
}