/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#include "token.h"
#include "vm.h"

#define COMPILER_VERSION "Drift 0.0.1 (MADE AT Sep 2021 17, 09:38:45)"
#define DRIFT_LICENSE    "GNU General Public License GPL v3.0"

bool show_tokens;
bool show_bytes;
bool show_tb;
bool repl_mode;

extern keg *lexer(const char *, int);
extern keg *compile(keg *);

extern void disassemble_code(code_object *);
extern void disassemble_token(keg *);

bool trace;

void run(char *source, int fsize, char *filename) {
    keg *tokens = lexer(source, fsize);
    free(source);

    if (show_tokens) {
        disassemble_token(tokens);
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

static vm_state state;

void run_repl(char *line, int size) {
    keg *tokens = lexer(line, size);
    if (trace) {
        return;
    }

    keg *codes = compile(tokens);
    if (trace) {
        return;
    }

    state = evaluate(codes->data[0], "REPL");

    free(line);
    free_tokens(tokens);
    free_keg(codes);
}

void usage() {
    printf("usage: drift [FILE(.ft)] <option>\n \
\n\
command: \n\
  repl        enter read-eval-print-loop mode\n\
  token       show lexical token list\n\
  op          show bytecode\n\
  tb          after exec, show environment mapping\n\n\
version:  %s\n\
license:  %s\n\
           @ bingxio - bingxio@qq.com\n",
        COMPILER_VERSION, DRIFT_LICENSE);
    exit(EXIT_SUCCESS);
}

char *rp_line = NULL;

void repl() {
    repl_mode = true;
    printf("%s\n", COMPILER_VERSION);

    while (true) {
        printf("> ");
        rp_line = malloc(sizeof(char) * 128);

        if (fgets(rp_line, 128, stdin) == NULL) {
            printf("get input error\n");
            goto out;
        }

        int len = strlen(rp_line);

        if (len <= 0) {
            goto out;
        } else {
            bool allspace = true;

            for (int i = 0; i < len; i++) {
                char c = rp_line[i];

                if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
                    allspace = false;
                }
            }
            if (allspace) {
                goto out;
            }
        }

        rp_line[len - 1] = '\0';
        run_repl(rp_line, len);
        continue;
    out:
        free(rp_line);
    }
}

int code_argc = 0;
char **code_argv = NULL;

int main(int argc, char **argv) {
    code_argc = argc;
    code_argv = argv;
    if (argc < 2) {
        usage();
    }
    if (argc == 2 && strcmp(argv[1], "repl") == 0) {
        repl();
        return 0;
    }
    if (argc == 3) {
        if (strcmp(argv[2], "token") == 0)
            show_tokens = true;
        if (strcmp(argv[2], "op") == 0)
            show_bytes = true;
        if (strcmp(argv[2], "tb") == 0)
            show_tb = true;
    }
    const char *path = argv[1];
    int len = strlen(path) - 1;
    if (path[len] != 't' || path[len - 1] != 'f' || path[len - 2] != '.') {
        fprintf(stderr, "\033[1;31merror:\033[0m no input file.\n");
        exit(EXIT_SUCCESS);
    }
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        printf("\033[1;31merror:\033[0m failed to read buffer of file: '%s'\n",
            path);
        exit(EXIT_SUCCESS);
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
