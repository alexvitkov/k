#include "Token.h"

TokenType Peek(Cons** tokens) {
  if (!(*tokens)) return TOK_NONE;
  Token* t = (*tokens)->Value;
  return t->TokenType;
}


Token* Pop1(Cons** tokens) {
  if (!(*tokens)) return NULL;

  Token* tok = (*tokens)->Value;
  *tokens    = (*tokens)->Tail;

  return tok;
}

Token* Expect1(Cons** tokens, TokenType tt) {
  Token* t = Pop1(tokens);
  if (!t || t->TokenType != tt) return NULL;
  return t;
}

