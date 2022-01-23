#pragma once
#include "Common.h"
#include "Cons.h"

enum TokenTypeEnum {
  TOK_NONE,

  TOK_ID = 128,
  TOK_NUMBER = 129,
  TOK_STRING = 130,

  // operators
  TOK_DOUBLE_EQUAL = 200,
  TOK_NOT_EQUAL = 201,
  TOK_GREATER_THAN = 202,
  TOK_LESS_THAN = 203,
  TOK_GREATER_THAN_EQUAL = 204,
  TOK_LESS_THAN_EQUAL = 205,
  TOK_DOUBLE_AND = 206,
  TOK_DOUBLE_OR = 207,

  // keywords
  TOK_CONST = 1000,
  TOK_IF = 1001,
  TOK_ELSE = 1002,
  TOK_WHILE = 1003,
  TOK_FN = 1004,
  TOK_RETURN = 1005,
  TOK_VAR = 1006,
  TOK_SET = 1007,
  TOK_EXTERNFN = 1008,

  // pseudo tokens
  TOK_INFER_KEYWORD_OR_IDENTIFIER = 2000,
};

typedef NUM TokenType;

typedef struct Token {
  TokenType TokenType;
  char* Str;
  NUM TokenNumber;
} Token;


Cons* LexFile(char* file);
