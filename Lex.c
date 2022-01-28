#include "Common.h"
#include "Token.h"
#include "Cons.h"
#include <stdlib.h>
#include <string.h>

BOOL IsDigit(char ch);
BOOL IsSpace(char ch);
BOOL IsLetter(char ch);
NUM StrToNum(char* str);
TokenType InferTokenType(char* str);
TokenType GetTwoCharOperator(char c1, char c2);
TokenType GetSingleCharOperator(char c);
Token* MakeToken(char* file, NUM offset, NUM length, TokenType type);
Token* LexCharacterLiteral(char* file, NUM* offset_ptr);

Token* LexString(char* file, NUM* offset_ptr) {
  Token* tok = NULL;
  NUM offset = *offset_ptr;

  if (file[offset] != '"')
    return NULL;

  offset++;

  while (file[offset] != '"')
    offset++;

  tok = MakeToken(file, *offset_ptr + 1, offset - *offset_ptr - 1, TOK_STRING);

  offset++;
  *offset_ptr = offset;
  return tok;
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

    // String
    tok = LexString(file, &i);
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

