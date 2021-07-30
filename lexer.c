/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL 3.0 License - bingxio <3106740988@qq.com> */
#include "lexer.h"

/* Whether the symbol is a space, carriage return, line feed, indent */
bool is_space(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

/* Are the symbols numbers 0 to 9 */
bool is_digit(char c) { return c >= '0' && c <= '9'; }

/* Is the symbol a legal identifier */
bool is_ident(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/* Token list */
list *tokens = NULL;

/* Push token into token list */
void push_token_list(token *tok) { tokens = append_list(tokens, tok); }

/* Determine whether the next character is the specified type */
bool next(const char *buf, int *p, char c) {
    if (*p + 1 > strlen(buf)) return false;
    if (buf[*p + 1] == c) {
        *p += 2;
        return true;
    }
    (*p) ++;
    return false;
}

/* Handles the literal amount of multiple characters */
void paste_literal(char *literal, const char *buf, int *p, int i) {
    for (int x = 0, j = *p; x < j; x ++) {
        literal[x] = buf[i - *p];
        (*p) --;
        if (x + 1 == j) {
            literal[x + 1] = '\0';
        }
    }
}

/* Lexical analysis */
list *lexer(const char *buf, int fsize) {
    bool new_line = true; /* Indent judgement */
    for (int i = 0, line = 1, off = 0; i < fsize;) {
        char c = buf[i];
        while (is_space(c)) { /* Space */
            if (c == '\n') {
                line ++;
                off = -1;
                new_line = true;
            }
            if (i + 1 >= fsize) {
                ++ i;
                break;
            }
            c = buf[++ i];
            /* If it's a new line,
             * And it's dealing with spaces, increase indent */
            if (new_line) off ++;
        }
        if (i >= fsize) break;
        if (new_line) new_line = false; /* To catch indent */
        if (is_digit(c)) { /* Digit */
            int p = 0;
            bool f = false;
            while (is_digit(c) || c == '.') {
                if (c == '.') f = true;
                p ++;
                c = buf[++ i];
            }
            char *literal = (char *) malloc(p * sizeof(char));
            paste_literal(literal, buf, &p, i);
            /* Append */
            push_token_list(new_token(f ? FLOAT : NUMBER, literal, line, off));
            continue;
        }
        if (is_ident(c)) { /* Ident */
            int p = 0;
            while (is_ident(c)) {
                p ++;
                c = buf[++ i];
            }
            char *literal = (char *) malloc((p + 1) * sizeof(char));
            paste_literal(literal, buf, &p, i);
            /* Append */
            push_token_list(new_token(
                to_keyword(literal), literal, line, off));
            continue;
        }
        token t = {.line = line, .off = off}; /* Others */
        switch (c) {
            case '+':
                if (next(buf, &i, '=')) {t.kind = AS_ADD; t.literal = "+=";}
                else {t.kind = ADD; t.literal = "+";}
                break;
            case '-':
                if (next(buf, &i, '=')) {t.kind = AS_SUB; t.literal = "-=";}
                else {
                    if (buf[i] == '>') {
                        t.kind = R_ARROW; t.literal = "->";
                        i ++;
                    } else {t.kind = SUB; t.literal = "-";}
                }
                break;
            case '*':
                if (next(buf, &i, '=')) {t.kind = AS_MUL; t.literal = "*=";}
                else {t.kind = MUL; t.literal = "*";}
                break;
            case '/':
                if (next(buf, &i, '=')) {t.kind = AS_DIV; t.literal = "/=";}
                else {t.kind = DIV; t.literal = "/";}
                break;
            case '%':
                if (next(buf, &i, '=')) {t.kind = AS_SUR; t.literal = "%=";}
                else {t.kind = SUR; t.literal = "%";}
                break;
            case '<':
                if (next(buf, &i, '=')) {t.kind = LE_EQ; t.literal = "<=";}
                else {t.kind = LESS; t.literal = "<";}
                break;
            case '>':
                if (next(buf, &i, '=')) {t.kind = GR_EQ; t.literal = ">=";}
                else {t.kind = GREATER; t.literal = ">";}
                break;
            case ' ':
            case '\t':
            case '\r':
            case '\n':
            case '\0':
                continue;
            case '#':
                while (buf[++ i] != '\n');
                continue;
            case '\'': { /* Char */
                i ++;
                char *literal = (char *) malloc(sizeof(char));
                literal[0] = buf[i ++];
                c = buf[i];
                if (c != '\'') {
                    fprintf(stderr,
                        "\033[1;31mlexer %d:\033[0m missing single quotation mark to the right.\n",
                        line);
                    exit(EXIT_FAILURE);
                } else {
                    i += 1;
                }
                t.kind = CHAR; t.literal = literal;
            }
                break;
            case '"': { /* String */
                c = buf[++ i];
                int p = 0;
                while (c != '"') {
                    c = buf[++ i];
                    p ++;
                    if (i == fsize) {
                        fprintf(stderr,
                            "\033[1;31mlexer %d:\033[0m missing closing double quote.\n", line);
                        exit(EXIT_FAILURE);
                    }
                }
                char *literal = (char *) malloc((p + 1) * sizeof(char));
                paste_literal(literal, buf, &p, i);
                i ++;
                t.kind = STRING; t.literal = literal;
            }
                break;
            case '.':
                i ++;
                t.kind = DOT; t.literal = ".";
                break;
            case ',':
                i ++;
                t.kind = COMMA; t.literal = ",";
                break;
            case ':':
                i ++;
                t.kind = COLON; t.literal = ":";
                break;
            case '=':
                if (next(buf, &i, '=')) {t.kind = EQ_EQ; t.literal = "==";}
                else {t.kind = EQ; t.literal = "=";}
                break;
            case ';':
                i ++;
                t.kind = SEMICOLON; t.literal = ";";
                break;
            case '&':
                i ++;
                t.kind = ADDR; t.literal = "&";
                break;
            case '|':
                i ++;
                t.kind = OR; t.literal = "|";
                break;
            case '!':
                if (next(buf, &i, '=')) {t.kind = BANG_EQ; t.literal = "!=";}
                else {t.kind = BANG; t.literal = "!";}
                break;
            case '{':
                i ++;
                t.kind = L_BRACE; t.literal = "{";
                break;
            case '}':
                i ++;
                t.kind = R_BRACE; t.literal = "}";
                break;
            case '[':
                i ++;
                t.kind = L_BRACKET; t.literal = "[";
                break;
            case ']':
                i ++;
                t.kind = R_BRACKET; t.literal = "]";
                break;
            case '(':
                i ++;
                t.kind = L_PAREN; t.literal = "(";
                break;
            case ')':
                i ++;
                t.kind = R_PAREN; t.literal = ")";
                break;
            case '\\':
                i ++;
                t.kind = SLASH; t.literal = "\\";
                break;
            default:
                fprintf(stderr, "\033[1;31mlexer %d:\033[0m unknown character '%c'.\n",
                    line, c);
                exit(EXIT_FAILURE);
        }
        /* Other */
        push_token_list(new_token(t.kind, t.literal, t.line, t.off));
    }
    /* Terminator */
    if (tokens != NULL) {
        token *end = list_back(tokens);
        token *eoh = new_token(EOH, "EOF", end->line + 1, 0);
        push_token_list(eoh); /* End of handler */
    } else {
        push_token_list(new_token(EOH, "EOF", 0, 0)); /* No token */
    }
    return tokens;
}