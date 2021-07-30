/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#include "object.h"

/* Output object */
const char *obj_string(object *obj) {
    char *str = (char *) malloc(sizeof(char) * 128);
    switch (obj->kind) {
        case OBJ_INT:    sprintf(str, "int %d", obj->value.integer);       return str;
        case OBJ_FLOAT:  sprintf(str, "float %f", obj->value.floating);    return str;
        case OBJ_STRING: sprintf(str, "string \"%s\"", obj->value.string); return str;
        case OBJ_CHAR:   sprintf(str, "char '%c'", obj->value.ch);         return str;
        case OBJ_ENUM:
            sprintf(str, "enum \"%s\"", obj->value.enumeration.name);
            return str;
        case OBJ_FUNC:
            sprintf(str, "func \"%s\"", obj->value.func.name);
            return str;
        case OBJ_FACE:
            sprintf(str, "face \"%s\"", obj->value.face.name);
            return str;
        case OBJ_WHOLE:
            sprintf(str, "whole \"%s\"", obj->value.whole.name);
            return str;
    }
}