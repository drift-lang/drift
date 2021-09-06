#include "../src/vm.h"

reg_mod *strconv() {
    reg_mod *m = new_mod("strconv");
    emit_member(m, "to_int", C_METHOD);
    emit_member(m, "to_float", C_METHOD);
    emit_member(m, "to_string", C_METHOD);
    return m;
}

object *get_front(keg *arg) {
    check_front(arg);
    return arg->data[0];
}

void to_int(keg *arg) {
    object *obj = get_front(arg);
    int num;

    switch (obj->kind) {
    case OBJ_STRING: {
        char *str = obj->value.str;
        num = atoi(str);
        break;
    }
    case OBJ_FLOAT:
        num = (int)obj->value.f;
        break;
    case OBJ_CHAR:
        num = obj->value.c;
        break;
    default:
        throw_error("only allowed string and float object");
    }
    push_stack(new_num(num));
}

void to_float(keg *arg) {
    object *obj = get_front(arg);
    double d = 0;

    switch (obj->kind) {
    case OBJ_INT:
        d = (double)obj->value.num;
        break;
    case OBJ_STRING: {
        char *str = obj->value.str;
        d = (double)atoi(str);
        break;
    }
    default:
        throw_error("only allowed int and string object");
    }
    push_stack(new_float(d));
}

void to_string(keg *arg) {
    object *obj = get_front(arg);
    char *str = malloc(sizeof(char) * STRING_CAP_MAX);

    switch (obj->kind) {
    case OBJ_INT:
        sprintf(str, "%d", obj->value.num);
        break;
    case OBJ_FLOAT:
        sprintf(str, "%f", obj->value.f);
        break;
    default:
        throw_error("only allowed int and float object");
    }
    push_stack(new_string(str));
}

static const char *user_mods[] = {"strconv", NULL};

void init() {
    reg_c_mod(user_mods);
}