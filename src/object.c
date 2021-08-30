/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#include "object.h"

const char *obj_string(object *obj) {
  char *str = malloc(sizeof(char) * DEBUG_OBJ_STR_CAP);
  switch (obj->kind) {
  case OBJ_INT:
    sprintf(str, "int %d", obj->value.num);
    return str;
  case OBJ_FLOAT:
    sprintf(str, "float %f", obj->value.f);
    return str;
  case OBJ_STRING:
    sprintf(str, "string \"%s\"", obj->value.str);
    return str;
  case OBJ_CHAR:
    sprintf(str, "char '%c'", obj->value.c);
    return str;
  case OBJ_BOOL:
    sprintf(str, "bool %s", obj->value.b ? "T" : "F");
    return str;
  case OBJ_NIL:
    sprintf(str, "nil");
    return str;
  case OBJ_ENUMERATE:
    sprintf(str, "enumerate \"%s\"", obj->value.en.name);
    return str;
  case OBJ_FUNCTION:
    sprintf(str, "function \"%s\"", obj->value.fn.name);
    return str;
  case OBJ_INTERFACE:
    sprintf(str, "interface \"%s\"", obj->value.in.name);
    return str;
  case OBJ_CLASS:
    sprintf(str, "class \"%s\"", obj->value.cl.name);
    return str;
  case OBJ_ARRAY:
    sprintf(str, "array %d", obj->value.arr.element->item);
    return str;
  case OBJ_TUPLE:
    sprintf(str, "tuple %d", obj->value.tup.element->item);
    return str;
  case OBJ_MAP:
    sprintf(str, "map %d", obj->value.map.v->item);
    return str;
  case OBJ_MODULE:
    sprintf(str, "module \"%s\"", obj->value.mod.name);
    return str;
  case OBJ_BUILTIN:
    sprintf(str, "builtin \"%s\"", obj->value.bu.name);
    return str;
  }
}

const char *obj_raw_string(object *obj, bool multiple) {
  char *str = malloc(sizeof(char) * STRING_CAP_MAX);
  switch (obj->kind) {
  case OBJ_INT:
    sprintf(str, "%d", obj->value.num);
    return str;
  case OBJ_FLOAT:
    sprintf(str, "%f", obj->value.f);
    return str;
  case OBJ_STRING:
    sprintf(str, "%s", obj->value.str);
    return str;
  case OBJ_CHAR:
    sprintf(str, "'%c'", obj->value.c);
    return str;
  case OBJ_BOOL:
    sprintf(str, "%s", obj->value.b ? "T" : "F");
    return str;
  case OBJ_ARRAY: {
    keg *elem = obj->value.arr.element;
    if (elem->item == 0) {
      free(str);
      return multiple ? "\t" : "[]";
    }
    sprintf(str, "%s", multiple ? "" : "[");
    for (int i = 0; i < elem->item; i++) {
      strcat(str, obj_raw_string((object *)elem->data[i], multiple));
      if (i + 1 != elem->item) {
        strcat(str, multiple ? "\t" : ", ");
      }
    }
    if (!multiple) {
      strcat(str, "]");
    }
    return str;
  }
  case OBJ_TUPLE: {
    keg *elem = obj->value.arr.element;
    if (elem->item == 0) {
      free(str);
      return "()";
    }
    sprintf(str, "(");
    for (int i = 0; i < elem->item; i++) {
      strcat(str, obj_raw_string((object *)elem->data[i], multiple));
      if (i + 1 != elem->item) {
        strcat(str, ", ");
      }
    }
    strcat(str, ")");
    return str;
  }
  case OBJ_MAP: {
    keg *k = obj->value.map.k;
    keg *v = obj->value.map.v;
    if (k->item == 0) {
      free(str);
      return "{}";
    }
    sprintf(str, "{");
    for (int i = 0; i < k->item; i++) {
      strcat(str, obj_raw_string((object *)k->data[i], multiple));
      strcat(str, ": ");
      object *obj = v->data[i];
      if (obj->kind == OBJ_STRING) {
        strcat(str, "\"");
        strcat(str, obj->value.str);
        strcat(str, "\"");
      } else {
        strcat(str, obj_raw_string(obj, multiple));
      }
      if (i + 1 != k->item) {
        strcat(str, ", ");
      }
    }
    strcat(str, "}");
    return str;
  }
  default: {
    free(str);
    return obj_string(obj);
  }
  }
}

object *lp = NULL;
object *rp = NULL;

void eval_obj_num(double *lv, double *rv, int m) {
  switch (m) {
  case 1:
    *lv = (double)lp->value.num;
    *rv = (double)rp->value.num;
    break;
  case 2:
    *lv = lp->value.num;
    *rv = rp->value.f;
    break;
  case 3:
    *lv = lp->value.f;
    *rv = rp->value.num;
    break;
  case 4:
    *lv = lp->value.f;
    *rv = rp->value.f;
    break;
  }
}

object *op_basic(uint8_t op, int m) {
  double lv, rv;
  eval_obj_num(&lv, &rv, m);

  object *obj = malloc(sizeof(object));
  if (op == TO_ADD || op == TO_SUB || op == TO_MUL || op == TO_DIV ||
      op == TO_SUR) {
    obj->kind = OBJ_FLOAT;
  }
  if (op == TO_GR || op == TO_GR_EQ || op == TO_LE || op == TO_LE_EQ ||
      op == TO_EQ_EQ || op == TO_NOT_EQ) {
    obj->kind = OBJ_BOOL;
  }
  switch (op) {
  case TO_ADD:
    if (m == 1) {
      obj->kind = OBJ_INT;
      obj->value.num = lv + rv;
    } else if (m == 5) {
      obj->kind = OBJ_STRING;
      char *cp = malloc(sizeof(char) * STRING_CAP_MAX);
      sprintf(cp, "%s%s", lp->value.str, rp->value.str);
      obj->value.str = cp;
    } else {
      obj->value.f = lv + rv;
    }
    break;
  case TO_SUB:
    obj->value.f = lv - rv;
    break;
  case TO_MUL:
    obj->value.f = lv * rv;
    break;
  case TO_DIV:
    obj->value.f = lv / rv;
    break;
  case TO_SUR:
    obj->value.f = (int)lv % (int)rv;
    break;
  case TO_GR:
    obj->value.b = lv > rv;
    break;
  case TO_GR_EQ:
    obj->value.b = lv >= rv;
    break;
  case TO_LE:
    obj->value.b = lv < rv;
    break;
  case TO_LE_EQ:
    obj->value.b = lv <= rv;
    break;
  case TO_EQ_EQ:
    obj->value.b = lv == rv;
    break;
  case TO_NOT_EQ:
    obj->value.b = lv != rv;
    break;
  }
  return obj;
}

object *op_logic(uint8_t op, int m) {
  object *obj = malloc(sizeof(object));
  obj->kind = OBJ_BOOL;
#define SIMPLE_LOGIC(op) \
  if (m == 1) { \
    obj->value.b = strcmp(lp->value.str, rp->value.str) == 0; \
  } \
  if (m == 2) { \
    obj->value.b = lp->value.c == rp->value.c; \
  } \
  if (m == 3) { \
    obj->value.b = true; \
  } \
  if (m == 4) { \
    obj->value.b = lp->value.b || rp->value.b; \
  }
  switch (op) {
  case TO_EQ_EQ:
    SIMPLE_LOGIC(==);
    break;
  case TO_NOT_EQ:
    SIMPLE_LOGIC(!=);
    break;
  case TO_AND:
    if (m == 4)
      obj->value.b = lp->value.b && rp->value.b;
    break;
  case TO_OR:
    if (m == 4)
      obj->value.b = lp->value.b || rp->value.b;
    break;
  }
#undef SIMPLE_LOGIC
  return obj;
}

eval_op_rule op_basic_rules[] = {
    {OBJ_INT,    OBJ_INT,    1},
    {OBJ_INT,    OBJ_FLOAT,  2},
    {OBJ_FLOAT,  OBJ_INT,    3},
    {OBJ_FLOAT,  OBJ_FLOAT,  4},
    {OBJ_STRING, OBJ_STRING, 5}
};

eval_op_rule op_logic_rules[] = {
    {OBJ_STRING, OBJ_STRING, 1},
    {OBJ_CHAR,   OBJ_CHAR,   2},
    {OBJ_NIL,    OBJ_NIL,    3},
    {OBJ_BOOL,   OBJ_BOOL,   4}
};

object *binary_op(uint8_t op, object *a, object *b) {
  int l = 5;
  for (int i = 0, sec = false; i < l; i++) {
    eval_op_rule rule = sec ? op_logic_rules[i] : op_basic_rules[i];
    if (a->kind == rule.l && b->kind == rule.r) {
      lp = a;
      rp = b;
      return sec ? op_logic(op, rule.m) : op_basic(op, rule.m);
    }
    if (i + 1 == l && !sec) {
      l = 4;
      i = 0;
      sec = true;
    }
  }
  return NULL;
}

bool type_checker(type *tp, object *obj) {
  if (obj->kind == OBJ_NIL) {
    return true;
  }
  switch (tp->kind) {
  case T_ARRAY:
  case T_TUPLE:
    if (tp->kind == T_ARRAY && obj->kind != OBJ_ARRAY)
      return false;
    if (tp->kind == T_TUPLE && obj->kind != OBJ_TUPLE)
      return false;
    keg *elem;
    if (tp->kind == T_ARRAY)
      elem = obj->value.arr.element;
    if (tp->kind == T_TUPLE)
      elem = obj->value.tup.element;
    if (elem->item != 0) {
      for (int i = 0; i < elem->item; i++) {
        object *x = (object *)elem->data[i];
        if (!type_checker((type *)tp->inner.single, x)) {
          return false;
        }
      }
    }
    break;
  case T_MAP:
    if (obj->kind != OBJ_MAP)
      return false;
    keg *k = obj->value.map.k;
    keg *v = obj->value.map.v;
    for (int i = 0; i < k->item; i++) {
      if (!type_checker((type *)tp->inner.both.T1, (object *)k->data[i])) {
        return false;
      }
    }
    for (int i = 0; i < v->item; i++) {
      if (!type_checker((type *)tp->inner.both.T2, (object *)v->data[i])) {
        return false;
      }
    }
    break;
  case T_FUNCTION:
    if (obj->kind != OBJ_FUNCTION)
      return false;
    if (tp->inner.fn.arg->item != obj->value.fn.k->item)
      return false;
    if (tp->inner.fn.ret != NULL) {
      if (obj->value.fn.ret == NULL) {
        return false;
      }
      if (((type *)tp->inner.fn.ret)->kind !=
          ((type *)obj->value.fn.ret)->kind) {
        return false;
      }
    }
    for (int i = 0; i < tp->inner.fn.arg->item; i++) {
      type *x = (type *)tp->inner.fn.arg->data[i];
      type *y = (type *)obj->value.fn.v->data[i];
      if (x->kind != y->kind) {
        return false;
      }
      if (!type_checker(x, (object *)obj->value.fn.v->data[i])) {
        return false;
      }
    }
    break;
  default: {
    if ((tp->kind == T_INT && obj->kind != OBJ_INT) ||
        (tp->kind == T_FLOAT && obj->kind != OBJ_FLOAT) ||
        (tp->kind == T_STRING && obj->kind != OBJ_STRING) ||
        (tp->kind == T_CHAR && obj->kind != OBJ_CHAR) ||
        (tp->kind == T_BOOL && obj->kind != OBJ_BOOL)) {
      return false;
    } else {
      if (tp->kind == T_USER) {
        const char *name = tp->inner.name;

        if ((obj->kind == OBJ_FUNCTION &&
                strcmp(name, obj->value.fn.name) != 0) ||
            (obj->kind == OBJ_ENUMERATE &&
                strcmp(name, obj->value.en.name) != 0) ||
            (obj->kind == OBJ_INTERFACE &&
                strcmp(name, obj->value.in.name) != 0)) {
          return false;
        }
      }
    }
  }
  }
  return true;
}

bool obj_eq(object *a, object *b) {
  switch (a->kind) {
  case OBJ_INT:
    if (b->kind == OBJ_INT)
      return a->value.num == b->value.num;
    break;
  case OBJ_FLOAT:
    if (b->kind == OBJ_FLOAT)
      return a->value.f == b->value.f;
    break;
  case OBJ_CHAR:
    if (b->kind == OBJ_CHAR)
      return a->value.c == b->value.c;
    break;
  case OBJ_STRING:
    if (b->kind == OBJ_STRING)
      return strcmp(a->value.str, b->value.str) == 0;
    break;
  case OBJ_BOOL:
    if (b->kind == OBJ_BOOL)
      return a->value.b ? b->value.b == true : b->value.b == false;
    break;
  default:
    return false;
  }
}

bool obj_kind_eq(object *a, object *b) {
  if ((a->kind == OBJ_INT && b->kind != OBJ_INT) ||
      (a->kind == OBJ_FLOAT && b->kind != OBJ_FLOAT) ||
      (a->kind == OBJ_STRING && b->kind != OBJ_STRING) ||
      (a->kind == OBJ_CHAR && b->kind != OBJ_CHAR) ||
      (a->kind == OBJ_BOOL && b->kind != OBJ_BOOL)) {
    return false;
  }
  return true;
}

const char *obj_type_string(object *obj) {
  switch (obj->kind) {
  case OBJ_INT:
    return "int";
  case OBJ_FLOAT:
    return "float";
  case OBJ_CHAR:
    return "char";
  case OBJ_STRING:
    return "string";
  case OBJ_BOOL:
    return "bool";
  case OBJ_ARRAY:
    return "array";
  case OBJ_TUPLE:
    return "tuple";
  case OBJ_MAP:
    return "map";
  case OBJ_FUNCTION:
    return "function";
  case OBJ_ENUMERATE:
    return "enumerate";
  case OBJ_CLASS:
    return "class";
  case OBJ_INTERFACE:
    return "interface";
  case OBJ_MODULE:
    return "module";
  case OBJ_NIL:
    return "nil";
  }
}

int obj_len(object *obj) {
  switch (obj->kind) {
  case OBJ_STRING:
    return strlen(obj->value.str);
  case OBJ_ARRAY:
    return obj->value.arr.element->item;
  case OBJ_TUPLE:
    return obj->value.tup.element->item;
  case OBJ_MAP:
    return obj->value.map.k->item;
  default:
    return -1;
  }
}