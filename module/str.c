#include <string.h>

#include "../src/vm.h"

reg_mod *str() {
  reg_mod *m = new_mod("str");
  emit_member(m, "end_with", C_METHOD);
  emit_member(m, "to_str", C_METHOD);
  return m;
}

void end_with(keg *arg) {
  const char *b = check_str(arg, 0);
  const char *a = check_str(arg, 1);

  int x = strlen(a);
  int y = strlen(b);

  bool equal = true;

  if (y > x) {
    throw_error("Judge that the length of the string is greater than itself");
  }
  for (int i = x - y, j = 0; i < x; i++) {
    if (a[i] != b[j++]) {
      equal = false;
      break;
    }
  }
  push_stack(new_bool(equal));
}

static const char *mods[] = {"str", NULL};

void init() { reg_c_mod(mods); }