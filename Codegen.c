#include "Node.h"

const NUM NUM_SIZE = 8u;
static NUM CurrentStackOffset;
static NUM NextLabel = 0;

enum RegisterEnum {
  REG_RAX = 0,
  REG_RCX = 1,
  REG_RDX = 2,
  REG_RBX = 3,
  REG_RSP = 4,
  REG_RBP = 5,
  REG_RSI = 6,
  REG_RDI = 7,
  REG_R8 = 8,
  REG_R9 = 9,
  REG_R10 = 10,
  REG_R11 = 11,
  REG_R12 = 12,
  REG_R13 = 13,
  REG_R14 = 14,
};
typedef NUM Register;

enum OperatorEnum {
  OP_NONE,
  OP_MOV,
  OP_TEST,
  OP_CMP,
  OP_ADD,
  OP_SUB,
  OP_BAND,
  OP_MUL,
  OP_BOR,
  OP_JMP,
  OP_JNZ,
  OP_JZ,

  OP_LT,
  OP_LE,
  OP_GT,
  OP_GE,
  OP_EQ,
  OP_NE,
};
typedef NUM Operator;


/* clang-format off */
const char* RegisterNames[] = {
  "rax", "rcx", "rdx", "rbx",
  "rsp", "rbp", "rsi", "rdi",
  "r8", "r9", "r10", "r11",
  "r12", "r13", "r14",
};

const char* RegisterNames8[] = {
  "al", "cl", "dl", "bl",
  "spl", "bpl", "sil", "dil",
  "r8l", "r9l", "r10l", "r11l",
  "r12l", "r13l", "r14l",
};
/* clang-format on */

enum LocationSpace {
  LOC_NONE,
  LOC_REGISTER,
  LOC_RBP_RELATIVE,
  LOC_CONSTANT,
};
typedef NUM LocationSpace;

typedef struct Location {
  LocationSpace LocationSpace;
  NUM LocationOffset;
} Location;

static Location ReturnLocation = { LOC_REGISTER, REG_RAX };
static Location ZeroLocation = { LOC_CONSTANT, 0 };
static Location TempRegister = { LOC_REGISTER, REG_R11 };

static Location ArgumentLocations[] = {
  { LOC_REGISTER, REG_RDI },
  { LOC_REGISTER, REG_RSI },
  { LOC_REGISTER, REG_RDX },
  { LOC_REGISTER, REG_RCX },
  { LOC_REGISTER, REG_R8 },
  { LOC_REGISTER, REG_R10 },
};

static void NewLine();
static void CodegenBlock(Fn* fn, Block* block);
static void PrintLocation(Location* loc);
static void PrintLocationByte(Location* loc);
static NUM GetStackFrameSize(Fn* fn);
static void Push(Location* loc);
static void Pop(Location* loc);

static void NewLine() {
  printf("\n    ");
}

static void PrintLocation(Location* loc) {
  switch (loc->LocationSpace) {
    case LOC_CONSTANT: {
      printf("%ld", loc->LocationOffset);
      return;
    }
    case LOC_REGISTER: {
      printf("%s", RegisterNames[loc->LocationOffset]);
      return;
    }
    case LOC_RBP_RELATIVE: {
      if (loc->LocationOffset > 0) {
	printf("QWORD +%ld[rbp]", loc->LocationOffset);
      } else if (loc->LocationOffset < 0) {
	printf("QWORD -%ld[rbp]", -loc->LocationOffset);
      } else {
	printf("QWORD [rbp]");
      }
      return;
    }
    case LOC_NONE: {
      printf("<<<<<NONE>>>>>");
      return;
    }
  }
}

static void PrintLocationByte(Location* loc) {
  switch (loc->LocationSpace) {
    case LOC_REGISTER: {
      printf("%s", RegisterNames8[loc->LocationOffset]);
      return;
    }
    case LOC_RBP_RELATIVE: {
      if (loc->LocationOffset > 0) {
	printf("BYTE +%ld[rbp]", loc->LocationOffset);
      } else if (loc->LocationOffset < 0) {
	printf("BYTE -%ld[rbp]", -loc->LocationOffset);
      } else {
	printf("BYTE [rbp]");
      }
      return;
    }
  }
}

static NUM GetStackFrameSize(Fn* fn) {
  NUM size = 0;

  Cons* statement_list = fn->FnBlock->BlockStatements;
  while (statement_list) {
    Node* statement = statement_list->Value;
    statement_list = statement_list->Tail;

    if (statement->NodeType == NODE_VAR)
      size += NUM_SIZE;
  }

  return size;
}

static void GetVarLocation(Fn* fn, const char* var_name, Location* out) {
  // Check if it's a parameter
  NUM argument_index = 0;

  Cons* argument_list = fn->FnParamNames;
  while (argument_list) {
    char* arg_name = argument_list->Value;
    if (strcmp(arg_name, var_name) == 0) {
      out->LocationSpace = ArgumentLocations[argument_index].LocationSpace;
      out->LocationOffset = ArgumentLocations[argument_index].LocationOffset;
      return;
    }
    argument_index++;
    argument_list = argument_list->Tail;
  }

  // Check if it's a local variable
  NUM local_index = 0;
  Cons* statement_list = fn->FnBlock->BlockStatements;
  while (statement_list) {
    Node* statement = statement_list->Value;
    statement_list = statement_list->Tail;

    if (statement->NodeType == NODE_VAR) {
      const char* other_name = ((Var*)statement)->VarName;
      if (strcmp(other_name, var_name) == 0) {
	out->LocationSpace  = LOC_RBP_RELATIVE;
	out->LocationOffset = - (local_index + 1) * NUM_SIZE;
        return;
      }
      local_index ++;
    }
  }

  // error
  fprintf(stderr, "Invalid variable %s\n", var_name);
  exit(1);
}

static void AcquireTemp(Location* out) {
  CurrentStackOffset -= NUM_SIZE;
  out->LocationSpace = LOC_RBP_RELATIVE;
  out->LocationOffset = CurrentStackOffset;
}

static void CodegenExpression(Fn* fn, Node* expression, Location* expr_location);

static void Emit(Operator op, Location* dst, Location* src) {
  Location* src1 = src;
  Location* dst1 = dst;

  if (src->LocationSpace == LOC_RBP_RELATIVE && dst->LocationSpace == LOC_RBP_RELATIVE) {
    NewLine();
    printf("MOV ");
    src1 = &TempRegister;
    PrintLocation(src1);
    printf(", ");
    PrintLocation(src);
  }

  else if (dst->LocationSpace == LOC_CONSTANT) {
    NewLine();
    printf("MOV ");
    dst1 = &TempRegister;
    PrintLocation(dst1);
    printf(", ");
    PrintLocation(dst);
  }

  NewLine();
  switch (op) {
  case OP_MOV: printf("MOV "); break;
  case OP_ADD: printf("ADD "); break;
  case OP_SUB: printf("SUB "); break;
  case OP_BAND: printf("AND "); break;
  case OP_BOR: printf("OR "); break;
  case OP_TEST: printf("TEST "); break;
  case OP_CMP: printf("CMP "); break;
  }
  PrintLocation(dst1);
  printf(", ");
  PrintLocation(src1);
}

static void EmitSet(Operator op, Location* dst, Location* lhs, Location* rhs) {
  Emit(OP_CMP, lhs, rhs);

  Emit(OP_MOV, dst, &ZeroLocation);

  NewLine();
  switch (op) {
    case OP_LT: printf("SETL "); break;
    case OP_LE: printf("SETLE "); break;
    case OP_GT: printf("SETG "); break;
    case OP_GE: printf("SETGE "); break;
    case OP_EQ: printf("SETE "); break;
    case OP_NE: printf("SETNE "); break;
  }

  PrintLocationByte(dst);
}

static void Push(Location* loc) {
  NewLine();
  printf("PUSH ");
  PrintLocation(loc);
}

static void Pop(Location* loc) {
  NewLine();
  printf("POP ");
  PrintLocation(loc);
}

static void PushArgs() {
  Push(&ReturnLocation);
  for (NUM i = 0; i < 6; i++) {
    Push(&ArgumentLocations[i]);
  }
}

static void PopArgs() {
  for (NUM i = 5; i >= 0; i--) {
    Pop(&ArgumentLocations[i]);
  }
  Pop(&ReturnLocation);
}

static void EmitMul(Location* dst, Location* lhs, Location* rhs) {
  Location RDX = { LOC_REGISTER, REG_RAX };
  Location RAX = { LOC_REGISTER, REG_RAX };

  Push(&RDX);
  Push(&RAX);
  Emit(OP_MOV, &RAX, lhs);


  Location* rhs2 = rhs;
  if (rhs2->LocationSpace == LOC_CONSTANT) {
    Emit(OP_MOV, &TempRegister, rhs2);
    rhs2 = &TempRegister;
  }

  NewLine();
  printf("MUL ");
  PrintLocation(rhs2);

  Emit(OP_MOV, dst, &RAX);

  Pop(&RAX);
  Pop(&RDX);
}

static NUM GetLabel() {
  return NextLabel++;
}

static void PlaceLabel(NUM label) {
  printf("\n_label%ld:", label);
}

static void EmitJump(Operator jumpType, NUM label) {
  NewLine();
  switch (jumpType) {
  case OP_JMP: printf("JMP "); break;
  case OP_JNZ: printf("JNZ "); break;
  case OP_JZ: printf("JZ "); break;
  }
  printf("_label%ld", label);
}

static void EmitRet() {
  NewLine();
  printf("ADD rsp, 1000");
  NewLine();
  printf("POP rbp");
  NewLine();
  printf("RET");
}

static void CodegenOperator(Fn* fn, Call* call, Operator op, Location* destination) {
  BOOL allocated_temp = destination->LocationSpace == LOC_NONE;
  if (allocated_temp) AcquireTemp(destination);

  Cons* args = call->CallArguments;

  CodegenExpression(fn, args->Value, destination);
  args = args->Tail;

  while (args) {
    Node* arg_expression = args->Value;
    Location arg_location = { LOC_NONE };
    CodegenExpression(fn, arg_expression, &arg_location);
    Emit(op, destination, &arg_location);

    args = args->Tail;
  }
}


static void CodegenComparisonOperator(Fn* fn, Call* call, Operator op, Location* destination) {
  BOOL allocated_temp = destination->LocationSpace == LOC_NONE;
  if (allocated_temp) AcquireTemp(destination);

  Location lhs_location = { LOC_NONE };
  Location rhs_location = { LOC_NONE };

  Cons* args = call->CallArguments;
  CodegenExpression(fn, args->Value, &lhs_location);

  args = args->Tail;
  CodegenExpression(fn, args->Value, &rhs_location);

  if (op == OP_MUL) {
    EmitMul(destination, &lhs_location, &rhs_location);
  }
  else {
    EmitSet(op, destination, &lhs_location, &rhs_location);
  }
}

static void CodegenGet8(Fn* fn, Call* call, Location* destination) {
  BOOL allocated_temp = destination->LocationSpace == LOC_NONE;
  if (allocated_temp) AcquireTemp(destination);

  CodegenExpression(fn, call->CallArguments->Value, &TempRegister);
  Emit(OP_MOV, destination, &ZeroLocation);

  NewLine();
  printf("MOV r11b, [R11]");
  
  NewLine();
  printf("MOV ");
  PrintLocationByte(destination);
  printf(", r11b");
}

static void CodegenCall(Fn* fn, Call* call, Location* destination) {
  if (call->CallFunction->NodeType != NODE_REFERENCE) {
    fprintf(stderr, "Invalid function call\n");
    exit(1);
  }

  const char* fn_name = ((Reference*)call->CallFunction)->ReferenceName;

  // Builtins:
  /* clang-format off */
  if (strcmp(fn_name, "+") == 0) { CodegenOperator(fn, call, OP_ADD, destination); return; }
  if (strcmp(fn_name, "-") == 0) { CodegenOperator(fn, call, OP_SUB, destination); return; }
  if (strcmp(fn_name, "&") == 0) { CodegenOperator(fn, call, OP_BAND, destination); return; }
  if (strcmp(fn_name, "|") == 0) { CodegenOperator(fn, call, OP_BOR, destination); return; }
  if (strcmp(fn_name, ">")  == 0) { CodegenComparisonOperator(fn, call, OP_GT, destination); return; }
  if (strcmp(fn_name, "<")  == 0) { CodegenComparisonOperator(fn, call, OP_LT, destination); return; }
  if (strcmp(fn_name, ">=") == 0) { CodegenComparisonOperator(fn, call, OP_GE, destination); return; }
  if (strcmp(fn_name, "<=") == 0) { CodegenComparisonOperator(fn, call, OP_LE, destination); return; }
  if (strcmp(fn_name, "==")  == 0) { CodegenComparisonOperator(fn, call, OP_EQ, destination); return; }
  if (strcmp(fn_name, "!=") == 0) { CodegenComparisonOperator(fn, call, OP_NE, destination); return; }
  if (strcmp(fn_name, "*") == 0)  { CodegenComparisonOperator(fn, call, OP_MUL, destination); return; }
  /* clang-format on */

  if (strcmp(fn_name, "get8") == 0) {
    CodegenGet8(fn, call, destination);
    return;
  }

  BOOL allocated_temp = destination->LocationSpace == LOC_NONE;
  if (allocated_temp) AcquireTemp(destination);

  PushArgs();

  NUM argument_index = 0;
  Cons* arg = call->CallArguments;
  while (arg) {
    CodegenExpression(fn, arg->Value, &ArgumentLocations[argument_index]);
    arg = arg->Tail;
    argument_index++;
  }

  NewLine();
  printf("CALL %s", fn_name);
  Emit(OP_MOV, &TempRegister, &ReturnLocation);
  PopArgs();
  Emit(OP_MOV, destination, &TempRegister);

  return;
}

static void CodegenExpression(Fn* fn, Node* expression, Location* expr_location) {
  switch (expression->NodeType) {
    case NODE_NUMBER: {
      if (expr_location->LocationSpace == LOC_NONE) {
	expr_location->LocationSpace  = LOC_CONSTANT;
	expr_location->LocationOffset = ((Number*)expression)->NumberValue;
      } else {
	Location loc = { LOC_CONSTANT, ((Number*)expression)->NumberValue };
	Emit(OP_MOV, expr_location, &loc);
      }
      return;
    }

    case NODE_REFERENCE: {
      if (expr_location->LocationSpace == LOC_NONE) {
	GetVarLocation(fn, ((Reference*)expression)->ReferenceName, expr_location);
      } else {
	Location loc;
	GetVarLocation(fn, ((Reference*)expression)->ReferenceName, &loc);
	Emit(OP_MOV, expr_location, &loc);
      }
      return;
    }

    case NODE_CALL: {
      CodegenCall(fn, (Call*)expression, expr_location);
      return;
    }
  }

  fprintf(stderr, "Not implemented\n");
  exit(1);
}



static void CodegenSet(Fn* fn, Set* set) {
  Location var_location;
  GetVarLocation(fn, set->SetName, &var_location);
  CodegenExpression(fn, set->SetValue, &var_location);
}

static void CodegenReturn(Fn* fn, Return* ret) {
  if (ret->ReturnValue) {
    CodegenExpression(fn, ret->ReturnValue, &ReturnLocation);
  }
  NewLine();
  EmitRet();
}

static void CodegenIf(Fn* fn, If* if_statement) {
  NUM else_label = GetLabel();
  NUM end_label = GetLabel();

  Location condition = { LOC_NONE };
  CodegenExpression(fn, if_statement->IfCondition, &condition);

  Emit(OP_TEST, &condition, &condition);
  EmitJump(OP_JZ, else_label);

  CodegenBlock(fn, if_statement->IfThenBlock);
  EmitJump(OP_JMP, end_label);
  PlaceLabel(else_label);

  if (if_statement->IfElseBlock) {
    CodegenBlock(fn, if_statement->IfElseBlock);
  }
  PlaceLabel(end_label);
}

static void CodegenWhile(Fn* fn, While* while_loop) {
  Location condition = { LOC_NONE };
  NUM start_label = GetLabel();
  NUM done_label = GetLabel();

  PlaceLabel(start_label);
  CodegenExpression(fn, while_loop->WhileCondition, &condition);

  Emit(OP_TEST, &condition, &condition);
  EmitJump(OP_JZ, done_label);

  CodegenBlock(fn, while_loop->WhileBody);
  EmitJump(OP_JMP, start_label);

  PlaceLabel(done_label);
}

static void CodegenStatement(Fn* fn, Node* statement) {
  switch (statement->NodeType) {
    case NODE_SET: CodegenSet(fn, (Set*)statement); return;
    case NODE_RETURN: CodegenReturn(fn, (Return*)statement); return;
    case NODE_IF: CodegenIf(fn, (If*)statement); return;
    case NODE_WHILE: CodegenWhile(fn, (While*)statement); return;
  }
}

static void CodegenBlock(Fn* fn, Block* block) {
  Cons* statements = block->BlockStatements;

  while (statements) {
    CodegenStatement(fn, statements->Value);
    statements = statements->Tail;
  }
}

static void CodegenFn(Fn* fn) {
  printf("global %s\n", fn->FnName);
  printf("%s:", fn->FnName);

  NewLine();
  printf("PUSH rbp");
  NewLine();
  printf("MOV rbp, rsp");
  NewLine();
  CurrentStackOffset = -GetStackFrameSize(fn);
  printf("SUB rsp, 1000");

  CodegenBlock(fn, fn->FnBlock);

  EmitRet();

  printf("\n\n");
}

void Codegen(Node* node) {
  switch (node->NodeType) {
    case NODE_FN: CodegenFn((Fn*)node); return;
  }
}

void GlobalCodegen(Cons* nodes) {
  while (nodes) {
    Node* node = nodes->Value;
    Codegen(node);
    nodes = nodes->Tail;
  }

}
