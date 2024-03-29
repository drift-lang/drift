/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#include <stdbool.h>
#include <stdio.h>

#include "keg.h"
#include "token.h"
#include "trace.h"

static inline bool is_space(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\0';
}

static inline bool is_digit(char c) {
  return c >= '0' && c <= '9';
}

static inline bool is_ident(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

token_kind to_keyword(const char* literal) {
  for (int i = 36; i < 48; i++) {
    if (strcmp(literal, token_string[i]) == 0) {
      return i;
    }
  }
  return LITERAL;
}

token* new_token(token_kind k, char* literal, int line, int off) {
  token* tok = malloc(sizeof(token));
  tok->kind = k;
  tok->literal = literal;
  tok->line = line;
  tok->off = off;
  return tok;
}

bool next(const char* buf, int* p, char c) {
  if (*p + 1 > strlen(buf)) {
    return false;
  }
  if (buf[*p + 1] == c) {
    *p += 2;
    return true;
  }
  (*p)++;
  return false;
}

void paste_literal(char* literal, const char* buf, int* p, int i) {
  for (int x = 0, j = *p; x < j; x++) {
    literal[x] = buf[i - *p];
    (*p)--;
    if (x + 1 == j) {
      literal[x + 1] = '\0';
    }
  }
}

extern keg* lexer(const char* buf, int fsize) {
  keg* tokens = NULL;
  bool new_line = true;
  for (int i = 0, line = 1, off = 0; i < fsize;) {
    char c = buf[i];
    while (is_space(c)) {
      if (c == '\n') {
        line++;
        off = -1;
        new_line = true;
      }
      if (i + 1 >= fsize) {
        ++i;
        break;
      }
      c = buf[++i];
      if (new_line) {
        off++;
      }
    }
    if (i >= fsize) {
      break;
    }
    if (new_line) {
      new_line = false;
    }
    if (is_digit(c)) {
      int p = 0;
      bool f = false;
      while (is_digit(c) || c == '.') {
        if (c == '.') {
          f = true;
        }
        p++;
        c = buf[++i];
      }
      char* literal = malloc((p + 1) * sizeof(char));
      paste_literal(literal, buf, &p, i);

      token* tok = new_token(f ? FLOAT : NUMBER, literal, line, off);
      tokens = append_keg(tokens, tok);
      continue;
    }
    if (is_ident(c)) {
      int p = 0;
      while (is_ident(c)) {
        p++;
        c = buf[++i];
      }
      char* literal = malloc((p + 1) * sizeof(char));
      paste_literal(literal, buf, &p, i);

      token* tok = new_token(to_keyword(literal), literal, line, off);
      tokens = append_keg(tokens, tok);
      continue;
    }
    token t = {.line = line, .off = off};
    switch (c) {
      case '+':
        i++;
        t.kind = ADD;
        t.literal = "+";
        break;
      case '-':
        if (next(buf, &i, '>')) {
          t.kind = R_ARROW;
          t.literal = "->";
        } else {
          t.kind = SUB;
          t.literal = "-";
        }
        break;
      case '*':
        i++;
        t.kind = MUL;
        t.literal = "*";
        break;
      case '/':
        i++;
        t.kind = DIV;
        t.literal = "/";
        break;
      case '%':
        i++;
        t.kind = SUR;
        t.literal = "%";
        break;
      case '<':
        if (next(buf, &i, '=')) {
          t.kind = LE_EQ;
          t.literal = "<=";
        } else {
          if (buf[i] == '-') {
            t.kind = L_ARROW;
            t.literal = "<-";
            i++;
          } else {
            t.kind = LESS;
            t.literal = "<";
          }
        }
        break;
      case '>':
        if (next(buf, &i, '=')) {
          t.kind = GR_EQ;
          t.literal = ">=";
        } else {
          t.kind = GREATER;
          t.literal = ">";
        }
        break;
      case ' ':
      case '\t':
      case '\r':
      case '\n':
      case '\0':
        continue;
      case '#':
        while (buf[++i] != '\n')
          ;
        continue;
      case '\'': {
        i++;
        char* literal = malloc(sizeof(char));
        literal[0] = buf[i++];
        c = buf[i];
        if (c != '\'') {
          TRACE(
              "\033[1;31mlexer %d:\033[0m missing single quotation "
              "mark to the right.\n",
              line)
          goto out;
        } else {
          i += 1;
        }
        t.kind = CHAR;
        t.literal = literal;
      } break;
      case '"': {
        c = buf[++i];
        int p = 0;
        while (c != '"') {
          c = buf[++i];
          p++;
          if (i == fsize) {
            TRACE(
                "\033[1;31mlexer %d:\033[0m missing closing double "
                "quote.\n",
                line)
            goto out;
          }
        }
        char* literal;
        if (p == 0) {
          literal = "";
        } else {
          literal = malloc((p + 1) * sizeof(char));
          paste_literal(literal, buf, &p, i);
        }
        for (int i = 0; i < strlen(literal); i++) {
          if (literal[i] == '\n') {
            line++;
          }
        }
        i++;
        t.kind = STRING;
        t.literal = literal;
      } break;
      case '.':
        i++;
        t.kind = DOT;
        t.literal = ".";
        break;
      case ',':
        i++;
        t.kind = COMMA;
        t.literal = ",";
        break;
      case ':':
        if (next(buf, &i, ':')) {
          t.kind = REF;
          t.literal = "::";
        } else {
          t.kind = COLON;
          t.literal = ":";
        }
        break;
      case '=':
        if (next(buf, &i, '=')) {
          t.kind = EQ_EQ;
          t.literal = "==";
        } else {
          t.kind = EQ;
          t.literal = "=";
        }
        break;
      case ';':
        i++;
        t.kind = SEMICOLON;
        t.literal = ";";
        break;
      case '&':
        i++;
        t.kind = ADDR;
        t.literal = "&";
        break;
      case '|':
        i++;
        t.kind = OR;
        t.literal = "|";
        break;
      case '!':
        if (next(buf, &i, '=')) {
          t.kind = BANG_EQ;
          t.literal = "!=";
        } else {
          t.kind = BANG;
          t.literal = "!";
        }
        break;
      case '{':
        i++;
        t.kind = L_BRACE;
        t.literal = "{";
        break;
      case '}':
        i++;
        t.kind = R_BRACE;
        t.literal = "}";
        break;
      case '[':
        i++;
        t.kind = L_BRACKET;
        t.literal = "[";
        break;
      case ']':
        i++;
        t.kind = R_BRACKET;
        t.literal = "]";
        break;
      case '(':
        i++;
        t.kind = L_PAREN;
        t.literal = "(";
        break;
      case ')':
        i++;
        t.kind = R_PAREN;
        t.literal = ")";
        break;
      case '\\':
        i++;
        t.kind = SLASH;
        t.literal = "\\";
        break;
      default:
        TRACE("\033[1;31mlexer %d:\033[0m unknown character '%c' ASCII %d.\n",
              line, c, c)
        goto out;
    }
    tokens = append_keg(tokens, new_token(t.kind, t.literal, t.line, t.off));
  }
  if (tokens != NULL) {
    token* end = back_keg(tokens);
    token* eoh = new_token(EOH, "EOF", end->line + 1, 0);
    tokens = append_keg(tokens, eoh);
  } else {
    tokens = append_keg(tokens, new_token(EOH, "EOF", 0, 0));
  }
out:
  return tokens;
}

extern void disassemble_token(keg* tokens) {
  for (int i = 0; i < tokens->item; i++) {
    token* t = tokens->data[i];
    printf("[%3d]\t%-5d %-5d %-5d %-30s\n", i, t->kind, t->line, t->off,
           t->literal);
  }
}