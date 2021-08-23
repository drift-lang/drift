/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#include "token.h"
#include "vm.h"

#define COMPILER_VERSION "Drift 0.0.1 (MADE AT Jul 2021 29, 15:40:45)"
#define DRIFT_LICENSE    "GNU General Public License GPL v3.0"

bool show_tokens;
bool show_bytes;
bool show_tb;

extern keg *lexer(const char *, int);
extern keg *compile(keg *);
extern void disassemble_code(code_object *);

void run(char *source, int fsize, char *filename) {
    keg *tokens = lexer(source, fsize);
    free(source);

    if (show_tokens) {
        for (int i = 0; i < tokens->item; i++) {
            token *t = tokens->data[i];
            printf("[%3d]\t%-5d %-5d %-5d %-30s\n", i, t->kind, t->line, t->off,
                   t->literal);
        }
        return;
    }

    keg *codes = compile(tokens);
    if (show_bytes) {
        disassemble_code(codes->data[0]);
        return;
    }

    vm_state state = evaluate(codes->data[0], filename);
    if (show_tb) {
        frame *main = state.frame->data[0];
        disassemble_table(main->tb, main->code->description);
    }

    free_keg(codes);
    free(state.filename);

    free_tokens(tokens);
}

void usage() {
    printf("usage: drift <option> FILE(.ft)\n \
\n\
command: \n\
  token       show lexical token list\n\
  op          show bytecode\n\
  tb          after exec, show environment mapping\n\n\
version:  %s\n\
license:  %s\n\
           @ bingxio - bingxio@qq.com\n",
           COMPILER_VERSION, DRIFT_LICENSE);
    exit(EXIT_FAILURE);
}

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
        fprintf(stderr, "\033[1;31merror:\033[0m no input file.\n");
        exit(EXIT_FAILURE);
    }
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        printf("\033[1;31merror:\033[0m failed to read buffer of file: "
               "'%s'\n",
               path);
        exit(EXIT_FAILURE);
    }

    fseek(fp, 0, SEEK_END);
    int fsize = ftell(fp);
    rewind(fp);
    char *buf = malloc(fsize + 1);

    fread(buf, sizeof(char), fsize, fp);
    buf[fsize] = '\0';

    run(buf, fsize, get_filename(path));

    fclose(fp);
    return 0;
}