#pragma once
#include "Common.h"
#include "Cons.h"
#include "Token.h"

enum NodeTypeEnum {
  NODE_NONE = 0,

  NODE_NUMBER,
  NODE_BLOCK,
  NODE_FN,
  NODE_RETURN,
  NODE_VAR,
  NODE_SET,
  NODE_REFERENCE,
  NODE_CALL,
  NODE_INFIX,
  NODE_IF,
  NODE_WHILE,
  NODE_STRING,
  NODE_BREAK,
  NODE_CONTINUE,
};
typedef NUM NodeType;

typedef struct Node {
  NodeType NodeType;
} Node;

typedef struct Block {
  NodeType NodeType;
  Cons* BlockStatements;
} Block;

typedef struct Number {
  NodeType NodeType; // NODE_NUMBER
  NUM NumberValue;
} Number;

typedef struct Fn {
  NodeType NodeType; // NODE_FN
  const char* FnName;
  Cons* FnParamNames;
  Block* FnBlock;
} Fn;

typedef struct Return {
  NodeType NodeType; // NODE_FN
  Node* ReturnValue;
} Return;

typedef struct Var {
  NodeType NodeType; // NODE_VAR
  const char* VarName;
} Var;

typedef struct Set {
  NodeType NodeType; // NODE_SET
  Node* SetDestination;
  Node* SetValue;
  BOOL SetIsEightBit;
} Set;

typedef struct Reference {
  NodeType NodeType; // NODE_REFERENCE
  const char* ReferenceName;
} Reference;

typedef struct Call {
  NodeType NodeType; // NODE_CALL
  Node* CallFunction;
  Cons* CallArguments;
} Call;

typedef struct If {
  NodeType NodeType; // NODE_IF
  Node* IfCondition;
  Block* IfThenBlock;
  Block* IfElseBlock;
} If;

typedef struct While {
  NodeType NodeType; // NODE_WHILE
  Node* WhileCondition;
  Block* WhileBody;
} While;

typedef struct String {
  NodeType NodeType; // NODE_STRING
  const char* StringStr;
  NUM StringLabel;
} String;

typedef struct Break {
  NodeType NodeType; // NODE_BREAK
} Break;

typedef struct Continue {
  NodeType NodeType; // NODE_CONTINUE
} Continue;

void PrintNode(Node* node, NUM indent);
