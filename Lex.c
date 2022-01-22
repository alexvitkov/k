#include "Lex.h"
#include "Common.h"
#include "Cons.h"
#include <stdlib.h>
#include <string.h>

TokenType InferTokenType(char* str) {
  if (strcmp(str, "if") == 0) return TOK_IF;
  if (strcmp(str, "while") == 0) return TOK_WHILE;
  if (strcmp(str, "fn") == 0) return TOK_FN;
  if (strcmp(str, "return") == 0) return TOK_RETURN;
  if (strcmp(str, "set") == 0) return TOK_SET;
  if (strcmp(str, "var") == 0) return TOK_VAR;
  return TOK_ID;
}

NUM StrToNum(char* str) {
  NUM num         = 0;
  NUM digitsCount = strlen(str);

  NUM digit = 0;

  while (digit < digitsCount) {
    num *= 10;
    num += str[digit] - '0';
    digit++;
  };

  return num;
}

Token* MakeToken(char* file, NUM offset, NUM length, TokenType type) {
  Token* tok = malloc(sizeof(Token));

  tok->Str = malloc(length + 1);
  memcpy(tok->Str, file + offset, length);
  tok->Str[length] = 0;

  if (type == TOK_INFER_KEYWORD_OR_IDENTIFIER) {
    tok->TokenType = InferTokenType(tok->Str);
  } else if (type == TOK_NUMBER) {
    tok->TokenNumber = StrToNum(tok->Str);
    tok->TokenType   = TOK_NUMBER;
  } else {
    tok->TokenType = type;
  }

  return tok;
}

BOOL IsLetter(char ch) {
  if (ch >= 'a' && ch <= 'z') return TRUE;
  if (ch >= 'A' && ch <= 'Z') return TRUE;
  if (ch == '_') return TRUE;
  return FALSE;
}

BOOL IsDigit(char ch) { return ch >= '0' && ch <= '9'; }

BOOL IsSpace(char ch) { return ch == ' ' || ch == '\n' || ch == '\t'; }

TokenType GetTwoCharOperator(char c1, char c2) {
  if ((c1 == '=') && (c2 == '=')) return TOK_DOUBLE_EQ;
  if ((c1 == '&') && (c2 == '&')) return TOK_AND;
  return TOK_NONE;
}

TokenType GetSingleCharOperator(char c) {
  if (c == '=' || c == ';' || c == '(' || c == ')' || c == '{' || c == '}' || c == ',' || c == '<' || c == '>') {
    return c;
  }
  return TOK_NONE;
}

Cons* LexFile(char* file) {
  Cons* list = 0;

  BOOL is_in_word   = FALSE;
  BOOL is_in_number = FALSE;

  NUM i          = 0;
  NUM word_start = -1;

  while (1) {
    Token* current = malloc(sizeof(Token));

    // Lex identifier
    if (IsLetter(file[i])) {
      if (!is_in_word) {
	is_in_word = TRUE;
	word_start = i;
      }
      i++;
      continue;
    } else if (is_in_word) {
      is_in_word = FALSE;
      list       = Append(&list, MakeToken(file, word_start, i - word_start, TOK_INFER_KEYWORD_OR_IDENTIFIER));
    }

    // Lex number
    if (IsDigit(file[i])) {
      if (!is_in_number) {
	is_in_number = TRUE;
	word_start   = i;
      }
      i++;
      continue;
    } else if (is_in_number) {
      is_in_number = FALSE;
      list         = Append(&list, MakeToken(file, word_start, i - word_start, TOK_NUMBER));
    }

    // Lex 2-char op
    TokenType tt = GetTwoCharOperator(file[i], file[i + 1]);
    if (tt) {
      list = Append(&list, MakeToken(file, i, 2, tt));
      i += 2;
      continue;
    }

    // Lex 1-char op
    tt = GetSingleCharOperator(file[i]);
    if (tt) {
      list = Append(&list, MakeToken(file, i, 1, tt));
      i++;
      continue;
    }

    if (IsSpace(file[i])) {
      i++;
      continue;
    }

    if (!file[i]) {
      break;
    }

    return NULL;
  }

  return list;
}
