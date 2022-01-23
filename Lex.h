#pragma once
#include "Common.h"
#include "Cons.h"

enum TokenTypeEnum {
  TOK_NONE,

  TOK_ID = 128,
  TOK_NUMBER = 129,

  // operators
  TOK_DOUBLE_EQUAL,
  TOK_NOT_EQUAL,
  TOK_GREATER_THAN,
  TOK_LESS_THAN,
  TOK_GREATER_THAN_EQUAL,
  TOK_LESS_THAN_EQUAL,
  TOK_DOUBLE_AND,
  TOK_DOUBLE_OR,

  // keywords
  TOK_CONST,
  TOK_IF,
  TOK_ELSE,
  TOK_WHILE,
  TOK_FN,
  TOK_RETURN,
  TOK_VAR,
  TOK_SET,

  // pseudo tokens
  TOK_INFER_KEYWORD_OR_IDENTIFIER = 128,
};

typedef NUM TokenType;

typedef struct Token {
  TokenType TokenType;
  char* Str;
  NUM TokenNumber;
} Token;


Cons* LexFile(char* file);
