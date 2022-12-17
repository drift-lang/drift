#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "../src/vm.h"

reg_mod *os() {
  reg_mod *m = new_mod("os");
  emit_member(m, "command", C_METHOD);
  emit_member(m, "listdir", C_METHOD);
  emit_member(m, "filename", C_METHOD);
  emit_member(m, "rm", C_METHOD);
  emit_member(m, "filesize", C_METHOD);
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

  object *list = new_array(T_STRING);
  keg *elem = list->value.arr.element;

  while (true) {
    dt = readdir(dir);
    if (dt != NULL) {
      if (dt->d_type == 8) {
        elem = append_keg(elem, new_string(dt->d_name));
      }
    } else {
      break;
    }
  }
  push_stack(list);
}

void filename(keg *arg) {
  const char *str = check_str(arg, 0);
  char *d = malloc(sizeof(char) * 32);

  memset(d, 0, sizeof(char) * 32);

  for (int i = 0; i < strlen(str); i++) {
    if (str[i] != '.') {
      d[i] = str[i];
    }
  }
  push_stack(new_string(d));
}

void rm(keg *arg) {
  const char *path = check_str(arg, 0);
  int status = remove(path);
  push_stack(new_num(status));
}

void filesize(keg *arg) {
  const char *path = check_str(arg, 0);

  struct stat sb;
  int ret = stat(path, &sb);
  int n = 0;
  if (ret == 0) {
    n = sb.st_size;
  }
  push_stack(new_num(n));
}

static const char *mods[] = {"os", NULL};

void init() { reg_c_mod(mods); }