/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#include "lexer.h"
#include "compiler.h"
#include "vm.h"

#define COMPILER_VERSION "Drift 0.0.1 (MADE AT Jul 29 15:40:45)"

bool show_tokens; /* Exec arguments */
bool show_bytes;

/* Run source code */
void run(char *source, int fsize) {
    /* Lexical analysis */
    list *tokens = lexer(source, fsize);

    if (show_tokens) {
        for (int i = 0; i < tokens->len; i ++) {
            token *t = tokens->data[i];
            printf("[%3d]\t%-5d %-20s %-20d %d\n", 
                i, t->kind, t->literal, t->line, t->off);
        }
    }

    /* Compiler */
    list *codes = compile(tokens);
    if (show_bytes) dissemble(codes->data[0]);

    /* Virtual machine */
    evaluate(codes->data[0]);

    free(source);
    free_list(tokens);
    free_list(codes);
}

/* ? */
int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: [OPTION] FILE\n \
\n\
        -t  output token list\n\
        -b  output bytecode list\n\n\
FILE: drift program file with .ft suffix\n");
        exit(EXIT_FAILURE);
    }

    if (argc == 2 && strcmp(argv[1], "-v") == 0) {
        printf("%s\n", COMPILER_VERSION); /* Output version */
        exit(EXIT_SUCCESS);
    }
    if (argc == 3) {
        if (
            strcmp(argv[1], "-t") == 0) show_tokens = true;
        if (
            strcmp(argv[1], "-b") == 0) show_bytes = true;
    }
    const char *path = argc == 3 ? argv[2] : argv[1];
    int len = strlen(path) - 1;
    if (path[len] != 't'  || 
        path[len - 1] != 'f' ||
        path[len - 2] != '.') {
            fprintf(stderr, 
                "\033[1;31merror:\033[0m please load the source file with .ft sufix.\n");
            exit(EXIT_FAILURE);
    }
    FILE *fp = fopen(path, "r"); /* Open file of path */
    if (fp == NULL) {
        printf("\033[1;31merror:\033[0m failed to read buffer of file: %s.\n", path);
        exit(EXIT_FAILURE);
    }

    fseek(fp, 0, SEEK_END);
    int fsize = ftell(fp); /* Returns the size of file */
    fseek(fp, 0, SEEK_SET);
    char *buf = (char *) malloc(fsize * sizeof(char));

    fread(buf, fsize, sizeof(char), fp); /* Read file to buffer*/
    buf[fsize] = '\0';

    run(buf, fsize);
    
    fclose(fp); /* Close file */
    return 0;
}