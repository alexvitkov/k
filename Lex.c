#include "Lex.h"
#include "Common.h"
#include "Cons.h"
#include <stdlib.h>
#include <string.h>

BOOL IsDigit(char ch);
BOOL IsSpace(char ch);
BOOL IsLetter(char ch);
NUM StrToNum(char* str);

TokenType InferTokenType(char* str) {
  if (strcmp(str, "if") == 0) return TOK_IF;
  if (strcmp(str, "else") == 0) return TOK_ELSE;
  if (strcmp(str, "while") == 0) return TOK_WHILE;
  if (strcmp(str, "fn") == 0) return TOK_FN;
  if (strcmp(str, "return") == 0) return TOK_RETURN;
  if (strcmp(str, "set") == 0) return TOK_SET;
  if (strcmp(str, "var") == 0) return TOK_VAR;
  if (strcmp(str, "externfn") == 0) return TOK_EXTERNFN;
  return TOK_ID;
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


TokenType GetTwoCharOperator(char c1, char c2) {
  if ((c1 == '=') && (c2 == '=')) return TOK_DOUBLE_EQUAL;
  if ((c1 == '!') && (c2 == '=')) return TOK_NOT_EQUAL;
  if ((c1 == '&') && (c2 == '&')) return TOK_DOUBLE_AND;
  if ((c1 == '|') && (c2 == '|')) return TOK_DOUBLE_OR;
  if ((c1 == '>') && (c2 == '=')) return TOK_GREATER_THAN_EQUAL;
  if ((c1 == '<') && (c2 == '=')) return TOK_LESS_THAN_EQUAL;
  return TOK_NONE;
}

Token* LexCharacterLiteral(char* file, NUM* offset_ptr) {
  Token* tok = NULL;
  NUM offset = *offset_ptr;

  if (file[offset] != '\'')
    return NULL;

  offset++;
  if (file[offset] == '\\') {
    tok = MakeToken(file, *offset_ptr, 3, TOK_NONE);
    offset++;
    switch (file[offset]) {
    case 'n': tok->TokenNumber = '\n'; break;
    case 't': tok->TokenNumber = '\t'; break;
    case '\'': tok->TokenNumber = '\''; break;
    case '\\': tok->TokenNumber = '\\'; break;
    default: fprintf(stderr, "Invalid escape sequence"); exit(1);
    }
  }
  else {
    tok = MakeToken(file, *offset_ptr, 2, TOK_NONE);
    tok->TokenNumber = file[offset];
  }

  offset += 2;

  tok->TokenType = TOK_NUMBER;
  *offset_ptr = offset;
  return tok;
}

TokenType GetSingleCharOperator(char c) {
  if (c == '&' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '|' || c == '=' || c == ';' || c == '(' || c == ')' || c == '{' || c == '}' || c == ',' || c == '<' || c == '>') {
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

  Token* tok;

  while (1) {
    // Lex comments
    if (file[i] == '/' && file[i+1] == '/') {
      while (file[i] != '\n')
	i++;
      continue;
    }

    // Character literal
    tok = LexCharacterLiteral(file, &i);
    if (tok) {
      list = Append(&list, tok);
      continue;
    }

    // Lex identifier
    if (is_in_word) {
      if (!IsLetter(file[i]) && !IsDigit(file[i])) {
	is_in_word = FALSE;
	list = Append(&list, MakeToken(file, word_start, i - word_start, TOK_INFER_KEYWORD_OR_IDENTIFIER));
      }
      else {
	i++;
	continue;
      }
    }
    else if (IsLetter(file[i])) {
      is_in_word = TRUE;
      word_start = i;
      i++;
      continue;
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

    fprintf(stderr, "Unexpected character '%c'\n", file[i]);
    return NULL;
  }

  return list;
}
