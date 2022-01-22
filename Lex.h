#pragma once
#include "Common.h"
#include "Cons.h"

enum TokenTypeEnum {
  TOK_NONE,

  TOK_INFER_KEYWORD_OR_IDENTIFIER = 128,
  TOK_NUMBER,
  TOK_ID,
  TOK_IF,
  TOK_WHILE,

  TOK_DOUBLE_EQ,
  TOK_AND,
};

typedef NUM TokenType;

typedef struct Token {
  TokenType Type;
  char* Str;
  NUM Number;
} Token;


Cons* LexFile(char* file);
