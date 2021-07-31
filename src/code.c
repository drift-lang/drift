/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#include "code.h"

/* Allocate new code object */
code_object *new_code(char *des) {
    code_object *obj = (code_object *) malloc(sizeof(code_object));
    obj->description = des;
    obj->codes = NULL;
    obj->lines = NULL;
    obj->names = NULL;
    obj->objects = NULL;
    obj->offsets = NULL;
    obj->types = NULL;
    return obj;
}