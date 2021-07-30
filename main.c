/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#include "lexer.h"
#include "compiler.h"

#define COMPILER_VERSION "Drift 0.0.1 (MADE AT Jul 29 15:40:45)"

/* ? */
int main(int argc, char **argv) {
    if (argc < 2) {
        printf("\033[1;31musage:\033[0m ./drift [-t | -b] -v | (file)\n");
        exit(EXIT_FAILURE);
    }

    bool show_token = false; /* Exec arguments */
    bool show_byte = false;

    if (argc == 2 && strcmp(argv[1], "-v") == 0) {
        printf("%s\n", COMPILER_VERSION); /* Output version */
        exit(EXIT_SUCCESS);
    }
    if (argc == 3) {
        if (
            strcmp(argv[1], "-t") == 0) show_token = true;
        if (
            strcmp(argv[1], "-b") == 0) show_byte = true;
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

    /* Lexical analysis */
    list *tokens = lexer(buf, fsize);
    if (show_token) {
        for (int i = 0; i < tokens->len; i ++) {
            token *t = tokens->data[i];
            printf("[%3d]\t%-5d %-20s %-20d %d\n", 
                i, t->kind, t->literal, t->line, t->off);
        }
    }

    /* Compiler */
    list *objs = compile(tokens);
    if (show_byte) dissemble(objs->data[0]);
    
    fclose(fp); /* Close file */
    free(buf); /* Close buffer */
    free_list(tokens); /* Release tokens */
    return 0;
}