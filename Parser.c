#include "Parser.h"
#include "Cons.h"
#include "Lex.h"
#include "Node.h"
#include <string.h>

//#define DEBUG_TOKENS

Token* Pop1(Cons** tokens) {
  if (!(*tokens)) return NULL;

  Token* tok = (*tokens)->Value;
  *tokens    = (*tokens)->Tail;

  return tok;
}

TokenType Peek(Cons** tokens) {
  if (!(*tokens)) return TOK_NONE;
  Token* t = (*tokens)->Value;
  return t->TokenType;
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

Node* ParseExpression(Cons** stream, TokenType delimiter1, TokenType delimiter2);
Block* ParseBlock(Cons** stream);
Var* ParseVar(Cons** stream);
Set* ParseSet(Cons** stream);
Return* ParseReturn(Cons** stream);
Node* ParseStatement(Cons** stream);
If* ParseIf(Cons** stream);
While* ParseWhile(Cons** stream);
Fn* ParseFn(Cons** stream);
ExternFn* ParseExternFn(Cons** stream);

BOOL IsInfix(TokenType tt) {
  return tt == '&' || tt == '|' || tt == '+' || tt == '-' || tt == '*' || tt == '/' || tt == '<' || tt == '>'
      || tt == TOK_DOUBLE_EQUAL || tt == TOK_NOT_EQUAL || tt == TOK_GREATER_THAN || tt == TOK_LESS_THAN
      || tt == TOK_GREATER_THAN_EQUAL || tt == TOK_LESS_THAN_EQUAL || tt == TOK_DOUBLE_AND
      || tt == TOK_DOUBLE_OR;
}

Node* ParseExpression(Cons** stream, TokenType delimiter1, TokenType delimiter2) {
  Node* so_far            = NULL;
  Node* infix_lhs         = NULL;
  Token* hanging_operator = NULL;
  Token* t                = NULL;

  while (1) {
    if (hanging_operator && infix_lhs) {
      Reference* pseudo_fn     = malloc(sizeof(Reference));
      pseudo_fn->NodeType      = NODE_REFERENCE;
      pseudo_fn->ReferenceName = hanging_operator->Str;

      Call* call          = malloc(sizeof(Call));
      call->NodeType      = NODE_CALL;
      call->CallFunction  = (Node*)pseudo_fn;
      call->CallArguments = NULL;

      call->CallArguments = Append(&call->CallArguments, infix_lhs);
      call->CallArguments = Append(&call->CallArguments, so_far);

      so_far           = (Node*)call;
      infix_lhs        = NULL;
      hanging_operator = NULL;
    }

    if (Peek(stream) == delimiter1 || Peek(stream) == delimiter2) {
      return so_far;
    }

    t = Pop(stream);

    if (t->TokenType == TOK_ID) {
      infix_lhs          = so_far;
      Reference* ref     = malloc(sizeof(Reference));
      ref->NodeType      = NODE_REFERENCE;
      ref->ReferenceName = t->Str;
      so_far             = (Node*)ref;
      continue;
    }

    if (t->TokenType == TOK_NUMBER) {
      infix_lhs        = so_far;
      Number* num      = malloc(sizeof(Number));
      num->NodeType    = NODE_NUMBER;
      num->NumberValue = t->TokenNumber;
      so_far           = (Node*)num;
      continue;
    }

    if (t->TokenType == '(') {
      if (!hanging_operator && so_far) {
	// Function call
	Call* call         = malloc(sizeof(Call));
	call->NodeType     = NODE_CALL;
	call->CallFunction = so_far;

	// Parse argument list
	call->CallArguments = NULL;
	if (Peek(stream) == ')') {
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

	infix_lhs          = so_far;
	so_far = (Node*)call;
	continue;
      } else {
	infix_lhs          = so_far;
	so_far = ParseExpression(stream, ')', ')');
	if (!so_far) return NULL;

	Pop(stream);
	continue;
      }
    }

    if (IsInfix(t->TokenType)) {
      hanging_operator = t;
      continue;
    }

    fprintf(stderr, "Unexpected token in expression: %ld - '%s'\n", t->TokenType, t->Str);
    return NULL;
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

If* ParseIf(Cons** stream) {
  If* if_statement       = malloc(sizeof(If));
  if_statement->NodeType = NODE_IF;

  // Parse condition
  if_statement->IfCondition = ParseExpression(stream, '{', '{');
  if (!if_statement->IfCondition) return NULL;

  // Parse then block
  if_statement->IfThenBlock = ParseBlock(stream);
  if (!if_statement->IfThenBlock) return NULL;

  // Parse else block
  if (Peek(stream) == TOK_ELSE) {
    Pop(stream);
    if_statement->IfElseBlock = ParseBlock(stream);
    if (!if_statement->IfElseBlock) return NULL;
  } else {
    if_statement->IfElseBlock = NULL;
  }

  return if_statement;
}

While* ParseWhile(Cons** stream) {
  While* while_loop    = malloc(sizeof(While));
  while_loop->NodeType = NODE_WHILE;

  // Parse condition
  while_loop->WhileCondition = ParseExpression(stream, '{', '{');
  if (!while_loop->WhileCondition) return NULL;

  // Parse then block
  while_loop->WhileBody = ParseBlock(stream);
  if (!while_loop->WhileBody) return NULL;

  return while_loop;
}

Node* ParseStatement(Cons** stream) {
  Token* tok = Pop(stream);
  if (!tok) return NULL;

  if (tok->TokenType == TOK_VAR) return (Node*)ParseVar(stream);
  if (tok->TokenType == TOK_SET) return (Node*)ParseSet(stream);
  if (tok->TokenType == TOK_RETURN) return (Node*)ParseReturn(stream);
  if (tok->TokenType == TOK_IF) return (Node*)ParseIf(stream);
  if (tok->TokenType == TOK_WHILE) return (Node*)ParseWhile(stream);

  return NULL;
}

Block* ParseBlock(Cons** stream) {
  if (!Expect(stream, '{')) return NULL;

  Block* block           = malloc(sizeof(Block));
  block->NodeType        = NODE_BLOCK;
  block->BlockStatements = NULL;

  while (Peek(stream) != '}') {
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

  if (Peek(stream) == ')') {
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

ExternFn* ParseExternFn(Cons** stream) {
  ExternFn* efn = malloc(sizeof(ExternFn));
  efn->NodeType = NODE_EXTERNFN;

  Token* name = Expect(stream, TOK_ID);
  if (!name) return NULL;
  efn->ExternFnName = name->Str;

  if (!Expect(stream, ';'))
    return NULL;

  return efn;
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

      case TOK_EXTERNFN: {
	ExternFn* efn = ParseExternFn(&stream);
	if (!efn) return NULL;
        ast = Append(&ast, efn);
      }
    }
  }

  return ast;
}
