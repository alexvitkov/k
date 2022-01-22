#include "Parser.h"
#include "Cons.h"
#include "Lex.h"
#include "Node.h"

//#define DEBUG_TOKENS

Token* Pop1(Cons** tokens) {
  if (!(*tokens)) return NULL;

  Token* tok = (*tokens)->Value;
  *tokens    = (*tokens)->Tail;

  return tok;
}

Token* Peek(Cons** tokens) {
  if (!(*tokens)) return NULL;
  return (*tokens)->Value;
}

Token* Expect1(Cons** tokens, TokenType tt) {
  Token* t = Pop1(tokens);
  if (!t || t->TokenType != tt) return NULL;
  return t;
}

#ifdef DEBUG_TOKENS
#define Pop(tokens)                                                                                          \
  ({                                                                                                         \
    Token* tok = Pop1(tokens);                                                                               \
    printf("Line %4d: Pop %s\n", __LINE__, tok->Str);                                                        \
    tok;                                                                                                     \
  })

#define Expect(tokens, b)                                                                                    \
  ({                                                                                                         \
    Token* tok = Expect1(tokens, b);                                                                         \
    printf("Line %4d: Expect %s\n", __LINE__, tok->Str);                                                     \
    tok;                                                                                                     \
  })
#else
#define Pop Pop1
#define Expect Expect1
#endif

Node* ParseExpression(Cons** stream, TokenType delimiter1, TokenType delimiter2) {
  Node* so_far = NULL;

  while (1) {
    Token* t = Peek(stream);
    if (!t) return NULL;

    if (t->TokenType == delimiter1 || t->TokenType == delimiter2) {
      return so_far;
    } else {
      t = Pop(stream);
    }

    if (t->TokenType == TOK_ID) {
      if (so_far == NULL) {
	Reference* ref     = malloc(sizeof(Reference));
	ref->NodeType      = NODE_REFERENCE;
	ref->ReferenceName = t->Str;
	so_far             = (Node*)ref;
	continue;
      } else
	return NULL;
    }

    if (t->TokenType == TOK_NUMBER) {
      if (so_far == NULL) {
	Number* num      = malloc(sizeof(Number));
	num->NodeType    = NODE_NUMBER;
	num->NumberValue = t->TokenNumber;
	so_far           = (Node*)num;
	continue;
      } else
	return NULL;
    }

    if (t->TokenType == '(') {
      if (so_far) {
	// Function call
	Call* call         = malloc(sizeof(Call));
	call->NodeType     = NODE_CALL;
	call->CallFunction = so_far;

	// Parse argument list
	call->CallArguments = NULL;
	if (Peek(stream) && Peek(stream)->TokenType == ')') {
	  Pop(stream);
	} else {
	  while (1) {
	    Node* argument = ParseExpression(stream, ',', ')');
	    if (!argument) return NULL;
	    call->CallArguments = Append(&call->CallArguments, argument);

	    Token* tok = Pop(stream);
	    if (tok->TokenType == ')') break;
	    if (tok->TokenType != ',') return NULL;
	  }
	}

	so_far = (Node*)call;
      } else {
	// Infix operator
	return NULL;
      }
    }
  }

  return NULL;
}

Var* ParseVar(Cons** stream) {
  Var* var      = malloc(sizeof(Var));
  var->NodeType = NODE_VAR;

  Token* tok = Expect(stream, TOK_ID);
  if (!tok) return NULL;
  var->VarName = tok->Str;

  if (!Expect(stream, ';')) return NULL;
  return var;
}

Set* ParseSet(Cons** stream) {
  Set* set      = malloc(sizeof(Set));
  set->NodeType = NODE_SET;

  Token* tok = Expect(stream, TOK_ID);
  if (!tok) return NULL;
  set->SetName = tok->Str;

  if (!Expect(stream, '=')) return NULL;

  set->SetValue = ParseExpression(stream, ';', ';');
  if (!set->SetValue) return NULL;

  if (!Expect(stream, ';')) return NULL;
  return set;
}

Return* ParseReturn(Cons** stream) {
  Return* ret      = malloc(sizeof(Return));
  ret->NodeType    = NODE_RETURN;
  ret->ReturnValue = ParseExpression(stream, ';', ';');
  if (!ret->ReturnValue) return NULL;
  if (!Expect(stream, ';')) return NULL;
  return ret;
}

Node* ParseStatement(Cons** stream) {
  Token* tok = Pop(stream);
  if (!tok) return NULL;

  if (tok->TokenType == TOK_VAR) return (Node*)ParseVar(stream);
  if (tok->TokenType == TOK_SET) return (Node*)ParseSet(stream);
  if (tok->TokenType == TOK_RETURN) return (Node*)ParseReturn(stream);

  return NULL;
}

Block* ParseBlock(Cons** stream) {
  if (!Expect(stream, '{')) return NULL;

  Block* block           = malloc(sizeof(Block));
  block->NodeType        = NODE_BLOCK;
  block->BlockStatements = NULL;

  while (Peek(stream)->TokenType != '}') { // TODO null deref
    Node* statement = ParseStatement(stream);
    if (!statement) return NULL;
    block->BlockStatements = Append(&block->BlockStatements, statement);
  }

  if (!Expect(stream, '}')) return NULL;
  return block;
}

Fn* ParseFn(Cons** stream) {
  Fn* fn       = malloc(sizeof(Fn));
  fn->NodeType = NODE_FN;

  Token* tok;

  // Parse fn nfame
  tok = Expect(stream, TOK_ID);
  if (!tok) return NULL;
  fn->FnName = tok->Str;

  // Parse fn paramters
  fn->FnParamNames = NULL;

  if (!Expect(stream, '(')) return NULL;

  if (Peek(stream) && Peek(stream)->TokenType == ')') {
    Pop(stream);
  } else {
    while (1) {
      tok = Expect(stream, TOK_ID);
      if (!tok) return NULL;

      fn->FnParamNames = Append(&fn->FnParamNames, tok->Str);

      tok = Pop(stream);
      if (tok->TokenType == ')') break;
      if (tok->TokenType != ',') return NULL;
    }
  }

  fn->FnBlock = ParseBlock(stream);
  if (!fn->FnBlock) return NULL;

  return fn;
}

Cons* ParseTopLevel(Cons* tokens) {
  Cons* ast    = NULL;
  Cons* stream = tokens;

  while (1) {
    if (!stream) return ast;
    Token* tok = Pop(&stream);
    if (!tok) break;

    switch (tok->TokenType) {
      case TOK_FN: {
        Fn* fn = ParseFn(&stream);
        if (!fn) return NULL;
        ast = Append(&ast, fn);
        break;
      }
    }
  }

  return ast;
}
