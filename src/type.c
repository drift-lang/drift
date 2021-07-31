/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#include "type.h"

/* Output type */
const char *type_string(type *t) {
    char *str = (char *) malloc(sizeof(char) * 128);
    switch (t->kind) {
        case T_INT:    free(str); return "<int>";
        case T_FLOAT:  free(str); return "<float>";
        case T_CHAR:   free(str); return "<char>";
        case T_STRING: free(str); return "<string>";
        case T_BOOL:   free(str); return "<bool>";
        case T_ARRAY:
            sprintf(str, "[%s]", type_string((type *)t->inner.single));
            return str;
        case T_TUPLE:
            sprintf(str, "(%s)", type_string((type *)t->inner.single));
            return str;
        case T_MAP:
            sprintf(str, "{%s : %s}",
                type_string((type *)t->inner.both.T1), 
                type_string((type *)t->inner.both.T2));
            return str;
        case T_FUNC:
            if (t->inner.func.arg == NULL &&
                t->inner.func.ret == NULL) {
                    free(str);
                    return "<|| -> None>";
                }
            if (t->inner.func.arg != NULL) {
                sprintf(str, "<|");
                for (int i = 0; i < t->inner.func.arg->len; i ++) {
                    strcat(str, 
                        type_string((type *) t->inner.func.arg->data[i]));
                    if (i + 1 != t->inner.func.arg->len) {
                        strcat(str, ", ");
                    }
                }
                strcat(str, "|>");
                if (t->inner.func.ret != NULL) {
                    strcat(str, " -> ");
                    strcat(str, type_string((type *)t->inner.func.ret));
                }
                return str;
            }
            if (t->inner.func.arg == NULL &&
                t->inner.func.ret != NULL) {
                    sprintf(str, "<|| -> %s>", 
                        type_string((type *)t->inner.func.ret));
                    return str;
                }
        case T_USER:
            sprintf(str, "<%s>", t->inner.name);
            return str;
    }
}