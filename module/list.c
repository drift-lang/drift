#include "../src/vm.h"

reg_mod *list() {
    reg_mod *m = new_mod("list");
    emit_member(m, "to_string", C_METHOD);
    return m;
}

void to_string(keg *arg) {
    object *obj = check_front(arg);

    if (obj->kind != OBJ_ARRAY) {
        throw_error("object is not an array type");
    }

    keg *elem = obj->value.arr.element;

    char *str = malloc(sizeof(char) * 256);
    memset(str, 0, 256);

    for (int i = 0; i < elem->item; i++) {
        strcat(str, obj_raw_string(elem->data[i], false));
        strcat(str, " ");
    }
    push_stack(new_string(str));
}

static const char *mods[] = {"list", NULL};

void init() {
    reg_c_mod(mods);
}