#include "Node.h"

static void NewLine(NUM indent) {
  printf("\n");
  for (int i = 0; i < indent; i++)
    printf("  ");
}

static void PrintVar(Var* var, NUM indent) {
  printf("var %s;", var->VarName);
}

static void PrintNumber(Number* number, NUM indent) {
  printf("%ld", number->NumberValue);
}

static void PrintCall(Call* call, NUM indent) {
  printf("%s", ((Fn*)call->CallFunction)->FnName);
  
  printf("(");
  Cons* arg = call->CallArguments;
  while (arg) {
    PrintNode(arg->Value, indent);
    arg = arg->Tail;
    if (arg) printf(", ");
  }

  printf(")");
}

static void PrintIf(If* if_statement, NUM indent) {
  printf("if ");
  PrintNode(if_statement->IfCondition, indent);
  printf(" ");
  PrintNode((Node*)if_statement->IfThenBlock, indent);

  if (if_statement->IfElseBlock) {
    printf(" else ");
    PrintNode((Node*)if_statement->IfElseBlock, indent);
  }
}

static void PrintWhile(While* while_loop, NUM indent) {
  printf("while ");
  PrintNode(while_loop->WhileCondition, indent);
  printf(" ");
  PrintNode((Node*)while_loop->WhileBody, indent);
}

static void PrintReference(Reference* ref, NUM indent) {
  printf("%s", ref->ReferenceName);
}

static void PrintSet(Set* set, NUM indent) {
  printf("set %s = ", set->SetName);
  PrintNode(set->SetValue, indent);
  printf(";");
}

static void PrintReturn(Return* ret, NUM indent) {
  printf("return ");
  PrintNode(ret->ReturnValue, indent);
  printf(";");
}

static void PrintBlock(Block* block, NUM indent) {
  printf("{");

  Cons* statement = block->BlockStatements;
  while (statement) {
    NewLine(indent + 1);
    PrintNode(statement->Value, indent + 1);
    statement = statement->Tail;
  }

  NewLine(indent);
  printf("}");
}

static void PrintFn(Fn* fn, NUM indent) {
  printf("fn %s(", fn->FnName);

  Cons* param = fn->FnParamNames;
  while (param) {
    printf("%s", (char*)param->Value);
    param = param->Tail;
    if (param) printf(", ");
  }
  printf(") ");
  PrintBlock(fn->FnBlock, indent);
}

void PrintNode(Node* node, NUM indent) {
  switch (node->NodeType) {
    case NODE_FN: return PrintFn((Fn*)node, indent);
    case NODE_BLOCK: return PrintBlock((Block*)node, indent);
    case NODE_VAR: return PrintVar((Var*)node, indent);
    case NODE_SET: return PrintSet((Set*)node, indent);
    case NODE_RETURN: return PrintReturn((Return*)node, indent);
    case NODE_REFERENCE: return PrintReference((Reference*)node, indent);
    case NODE_CALL: return PrintCall((Call*)node, indent);
    case NODE_NUMBER: return PrintNumber((Number*)node, indent);
    case NODE_IF: return PrintIf((If*)node, indent);
    case NODE_WHILE: return PrintWhile((While*)node, indent);
    default: printf("node"); return;
  }
}
