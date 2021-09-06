#include <dirent.h>
#include <stdlib.h>

#include "../src/vm.h"

reg_mod *os() {
    reg_mod *m = new_mod("os");
    emit_member(m, "command", C_METHOD);
    emit_member(m, "listdir", C_METHOD);
    return m;
}

void command(keg *arg) {
    const char *cm = check_str(arg, 0);
    int status = system(cm);
    object *obj = new_num(status);
    push_stack(obj);
}

void listdir(keg *arg) {
    const char *ph = check_str(arg, 0);

    struct dirent *dt;
    DIR *dir = opendir(ph);
    if (dir == NULL) {
        throw_error("cannot open this dir");
    }

    object *list = malloc(sizeof(object));
    list->kind = OBJ_ARRAY;
    list->value.arr.element = new_keg();

    type *T = malloc(sizeof(type));
    T->kind = T_STRING;
    list->value.arr.T = T;

    keg *elem = list->value.arr.element;

    while (true) {
        dt = readdir(dir);
        if (dt != NULL) {
            if (dt->d_type == 8) {
                object *str = malloc(sizeof(object));
                
                str->kind = OBJ_STRING;
                str->value.str = dt->d_name;

                elem = append_keg(elem, str);
            }
        } else {
            break;
        }
    }
    push_stack(list);
}

static const char *mods[] = {"os", NULL};

void init() {
    reg_c_mod(mods);
}