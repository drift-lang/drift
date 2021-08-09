/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#include "token.h"
#include "vm.h"

#define COMPILER_VERSION "Drift 0.0.1 (MADE AT Jul 2021 29, 15:40:45)"
#define DRIFT_LICENSE    "GNU Public License v3.0"

bool show_tokens; /* Exec arguments */
bool show_bytes;
bool show_tb;

extern list *lexer(const char *, int);
extern list *compile(list *);
extern void dissemble(code_object *);

/* Run source code */
void run(char *source, int fsize, char *filename) {
  /* Lexical analysis */
  list *tokens = lexer(source, fsize);

  if (show_tokens) {
    for (int i = 0; i < tokens->len; i++) {
      token *t = tokens->data[i];
      printf("[%3d]\t%-5d %-5d %-5d %-30s\n", i, t->kind, t->line, t->off,
             t->literal);
    }
  }

  /* Compiler */
  list *codes = compile(tokens);
  if (show_bytes) {
    dissemble(codes->data[0]);
  }

  /* Virtual machine */
  vm_state state = evaluate(codes->data[0], filename);
  if (show_tb) {
    frame *main = (frame *)state.frame->data[0];
    dissemble_table(main->tb, main->code->description);
  }

  free(source); /* Release memory */
  free_list(tokens);
  free_list(codes);

  free(state.filename);
}

/* Print usage information */
void usage() {
  printf("usage: drift <option> FILE(.ft)\n \
\n\
command: \n\
  token       show lexical list\n\
  op          show bytecode\n\
  tb          after exec, show environment mapping\n\n\
version:  %s\n\
license:  %s\n\
           @ bingxio, 丙杺, 黄菁\n",
         COMPILER_VERSION, DRIFT_LICENSE);
  exit(EXIT_FAILURE);
}

/* ? */
int main(int argc, char **argv) {
  if (argc < 2) {
    usage();
  }
  if (argc == 3) {
    if (strcmp(argv[1], "token") == 0)
      show_tokens = true;
    if (strcmp(argv[1], "op") == 0)
      show_bytes = true;
    if (strcmp(argv[1], "tb") == 0)
      show_tb = true;
  }
  const char *path = argc == 3 ? argv[2] : argv[1];
  int len = strlen(path) - 1;
  if (path[len] != 't' || path[len - 1] != 'f' || path[len - 2] != '.') {
    fprintf(stderr, "\033[1;31merror:\033[0m please load the source file with "
                    ".ft sufix.\n");
    exit(EXIT_FAILURE);
  }
  FILE *fp = fopen(path, "r"); /* Open file of path */
  if (fp == NULL) {
    printf("\033[1;31merror:\033[0m failed to read buffer of file: %s.\n",
           path);
    exit(EXIT_FAILURE);
  }

  fseek(fp, 0, SEEK_END);
  int fsize = ftell(fp); /* Returns the size of file */
  rewind(fp);
  char *buf = (char *)malloc(fsize + 1);

  fread(buf, sizeof(char), fsize, fp); /* Read file to buffer*/
  buf[fsize] = '\0';

  run(buf, fsize, get_filename(path));

  fclose(fp); /* Close file */
  return 0;
}