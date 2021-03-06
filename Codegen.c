#include "Node.h"
#include "ProgramData.h"

const NUM NUM_SIZE = 8u;
static NUM CurrentStackOffset;
static NUM NextLabel = 0;
static NUM CurrentBreakLabel = 0;
static NUM CurrentContinueLabel = 0;

static NUM STACK_FRAME = 2000;

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
  OP_LEA,
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
  LOC_NONE = 0,
  LOC_REGISTER = 1,
  LOC_RBP_RELATIVE = 2,
  LOC_CONSTANT = 3,
  LOC_STRING = 4,
  LOC_STATIC = 5,
  LOC_EXTERN = 6,
};
typedef NUM LocationSpace;

typedef struct Location {
  LocationSpace LocationSpace;
  NUM LocationOffset;
} Location;

static Location ReturnLocation = { LOC_REGISTER, REG_RAX };
static Location ZeroLocation = { LOC_CONSTANT, 0 };
static Location TempRegister = { LOC_REGISTER, REG_R11 };

static Location ArgumentLocationsReg[] = {
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
static void EmitPush(Location* loc);
static void EmitPop(Location* loc);
static void AcquireTemp(Location* out);
static void Emit(Operator op, Location* dst, Location* src);

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
    case LOC_STRING: {
      printf("_string%ld", loc->LocationOffset);
      return;
    }
    case LOC_STATIC: {
      Var* var = Nth(StaticVariables, loc->LocationOffset);
      printf("QWORD [%s]", var->VarName);
      return;
    }
    case LOC_EXTERN: {
      const char* var = Nth(Externs, loc->LocationOffset);
      printf("QWORD [%s]", var);
      return;
    }
    default: {
      printf("<<<<<%ld>>>>>", loc->LocationSpace);
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
  NUM size = Length(fn->FnParamNames) * NUM_SIZE;

  Cons* statement_list = fn->FnBlock->BlockStatements;
  while (statement_list) {
    Node* statement = statement_list->Value;
    statement_list = statement_list->Tail;

    if (statement->NodeType == NODE_VAR)
      size += NUM_SIZE;
  }

  return size;
}

static void AddressOfRBPRelative(Location* rbp_rel, Location* out) {
  if (rbp_rel->LocationSpace != LOC_RBP_RELATIVE) {
    fprintf(stderr, "Invalid AddressOfRBPRelative\n");
    exit(1);
  }
  BOOL allocated_temp = out->LocationSpace == LOC_NONE;
  if (allocated_temp) AcquireTemp(out);

  Emit(OP_LEA, &TempRegister, rbp_rel);
  Emit(OP_MOV, out, &TempRegister);
}

static NUM GetLocalsCount(Fn* fn) {
  NUM count = 0;
  Cons* statement_list = fn->FnBlock->BlockStatements;
  while (statement_list) {
    Node* statement = statement_list->Value;
    if (statement->NodeType == NODE_VAR) {
      count++;
    }
    statement_list = statement_list->Tail;
  }

  return count;
}

static void GetVarLocation(Fn* fn, const char* var_name, Location* out, BOOL is_lvalue) {
  // Check if it's a parameter
  NUM argument_index = 0;

  Cons* argument_list = fn->FnParamNames;
  while (argument_list) {
    char* arg_name = argument_list->Value;
    if (strcmp(arg_name, var_name) == 0) {
      Location loc;
      loc.LocationSpace = LOC_RBP_RELATIVE;
      loc.LocationOffset = -(argument_index + 1) * NUM_SIZE;

      if (is_lvalue) {
	AddressOfRBPRelative(&loc, out);
      } else {
	out->LocationSpace  = loc.LocationSpace;
	out->LocationOffset = loc.LocationOffset;
      }
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
	Location loc;

	loc.LocationSpace  = LOC_RBP_RELATIVE;
	loc.LocationOffset = -( GetLocalsCount(fn) + Length(fn->FnParamNames) - local_index) * NUM_SIZE;

	if (is_lvalue) {
	  AddressOfRBPRelative(&loc, out);
	} else {
	  out->LocationSpace = loc.LocationSpace;
	  out->LocationOffset = loc.LocationOffset;
	}
        return;
      }
      local_index ++;
    }
  }

  // Check if it's a static
  Cons* statics = StaticVariables;
  NUM static_index = 0;
  while (statics) {
    Var* var = statics->Value;
    if (strcmp(var->VarName, var_name) == 0) {
      out->LocationSpace = LOC_STATIC;
      out->LocationOffset = static_index;
      return;
    }
    statics = statics->Tail;
    static_index++;
  }

  // Check if it's a extern
  Cons* extern_var = Externs;
  NUM extern_index = 0;
  while (extern_var) {
    const char* var = extern_var->Value;
    if (strcmp(var, var_name) == 0) {
      out->LocationSpace = LOC_EXTERN;
      out->LocationOffset = extern_index;
      return;
    }
    extern_var = extern_var->Tail;
    extern_index++;
  }

  // error
  fprintf(stderr, "Invalid variable %s\n", var_name);
  exit(1);
}

static void AcquireTemp(Location* out) {
  CurrentStackOffset -= NUM_SIZE;
  out->LocationSpace = LOC_RBP_RELATIVE;
  out->LocationOffset = CurrentStackOffset;

  if (CurrentStackOffset < -(STACK_FRAME - 10)) {
    fprintf(stderr, "Function blew up the stack frame\n");
    exit(1);
  }
}

static void CodegenExpression(Fn* fn, Node* expression, Location* expr_location, BOOL is_lvalue);

static BOOL IsMemoryLocation(NUM loc) {
  return loc == LOC_RBP_RELATIVE || loc == LOC_STATIC || loc == LOC_STRING || loc == LOC_EXTERN;
}

static void Emit(Operator op, Location* dst, Location* src) {
  Location* src1 = src;
  Location* dst1 = dst;

  if (IsMemoryLocation(src->LocationSpace) && IsMemoryLocation(dst->LocationSpace)) {
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
  case OP_LEA: printf("LEA "); break;
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

static void EmitPush(Location* loc) {
  NewLine();
  printf("PUSH ");
  PrintLocation(loc);
}

static void EmitPop(Location* loc) {
  NewLine();
  printf("POP ");
  PrintLocation(loc);
}

static void EmitMul(Location* dst, Location* lhs, Location* rhs) {
  Location RDX = { LOC_REGISTER, REG_RAX };
  Location RAX = { LOC_REGISTER, REG_RAX };

  EmitPush(&RDX);
  EmitPush(&RAX);
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

  EmitPop(&RAX);
  EmitPop(&RDX);
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

static void EmitRet(Fn* fn) {
  NewLine();
  printf("ADD rsp, %ld\n", STACK_FRAME);
  NewLine();
  printf("POP rbp");
  NewLine();
  printf("RET");
}

static void CodegenOperator(Fn* fn, Call* call, Operator op, Location* destination) {
  BOOL allocated_temp = destination->LocationSpace == LOC_NONE;
  if (allocated_temp) AcquireTemp(destination);

  Cons* args = call->CallArguments;

  CodegenExpression(fn, args->Value, destination, FALSE);
  args = args->Tail;

  while (args) {
    Node* arg_expression = args->Value;
    Location arg_location = { LOC_NONE };
    CodegenExpression(fn, arg_expression, &arg_location, FALSE);
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
  CodegenExpression(fn, args->Value, &lhs_location, FALSE);

  args = args->Tail;
  CodegenExpression(fn, args->Value, &rhs_location, FALSE);

  if (op == OP_MUL) {
    EmitMul(destination, &lhs_location, &rhs_location);
  }
  else {
    EmitSet(op, destination, &lhs_location, &rhs_location);
  }
}

static void DereferenceR11(Location* destination, BOOL byte) {
  Emit(OP_MOV, destination, &ZeroLocation);

  if (byte) {
    NewLine();
    printf("MOV r11b, [r11]");
    NewLine();
    printf("AND r11, 0x00000000000000FF");

    NewLine();
    printf("MOV ");
    PrintLocationByte(destination);
    printf(", r11b");
  } else {
    NewLine();
    printf("MOV r11, [r11]");

    NewLine();
    printf("MOV ");
    PrintLocation(destination);
    printf(", r11");
  }
}

static void CodegenGet(Fn* fn, Call* call, Location* destination, BOOL byte) {
  BOOL allocated_temp = destination->LocationSpace == LOC_NONE;
  if (allocated_temp) AcquireTemp(destination);

  CodegenExpression(fn, call->CallArguments->Value, &TempRegister, FALSE);
  DereferenceR11(destination, byte);
}

static void CodegenAddr(Fn* fn, Call* call, Location* destination, BOOL byte) {
  CodegenExpression(fn, call->CallArguments->Value, destination, TRUE);
}

static BOOL IsPLT(const char* name) {
  Cons* nodes = Externs;

  while (nodes) {
    if (strcmp(name, nodes->Value) == 0) return TRUE;
    nodes = nodes->Tail;
  }
  return FALSE;
}

static void CodegenArrow(Fn* fn, Call* call, Location* destination, BOOL is_lvalue) {

  CodegenOperator(fn, call, OP_ADD, destination);

  /* BOOL allocated_temp = destination->LocationSpace == LOC_NONE; */
  /* if (allocated_temp) AcquireTemp(destination); */

  /* Location lhs_loc = { LOC_NONE }; */
  /* Location rhs_loc = { LOC_NONE }; */
  /* Node* lhs = call->CallArguments->Value; */
  /* Node* rhs = call->CallArguments->Tail->Value; */

  /* CodegenExpression(fn, lhs, &lhs_loc, FALSE); */
  /* CodegenExpression(fn, rhs, &rhs_loc, FALSE); */
  /* Emit(OP_ADD, &lhs_loc, &rhs_loc); */

  if (!is_lvalue) {
    Emit(OP_MOV, &TempRegister, destination);
    DereferenceR11(destination, FALSE);
  }
}

  static void CodegenCall(Fn* fn, Call* call, Location* destination, BOOL is_lvalue) {
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
  if (strcmp(fn_name, "->") == 0)  { CodegenArrow(fn, call, destination, is_lvalue); return; }
  if (strcmp(fn_name, "get") == 0) { CodegenGet(fn, call, destination, FALSE); return; }
  if (strcmp(fn_name, "get8") == 0) { CodegenGet(fn, call, destination, TRUE); return; }
  if (strcmp(fn_name, "addr") == 0) { CodegenAddr(fn, call, destination, TRUE); return; }
  /* clang-format on */

  BOOL allocated_temp = destination->LocationSpace == LOC_NONE;
  if (allocated_temp) AcquireTemp(destination);

  NUM argument_index = 0;
  Cons* arg = call->CallArguments;

  Location loc[Length(call->CallArguments)];

  while (arg) {
    AcquireTemp(&loc[argument_index]);

    CodegenExpression(fn, arg->Value, &loc[argument_index], FALSE);
    arg = arg->Tail;
    argument_index++;
  }

  for (NUM i = 0; i < argument_index; i++) {
    Emit(OP_MOV, &ArgumentLocationsReg[i], &loc[i]);
  }

  NewLine();
  printf("XOR rax, rax");

  NewLine();
  printf("CALL %s", fn_name);

  if (IsPLT(fn_name))
    printf(" WRT ..plt");

  Emit(OP_MOV, &TempRegister, &ReturnLocation);
  Emit(OP_MOV, destination, &TempRegister);

  return;
}

static void CodegenNumber(Fn* fn, NUM number, Location* expr_location) {
  if (expr_location->LocationSpace == LOC_NONE) {
    expr_location->LocationSpace  = LOC_CONSTANT;
    expr_location->LocationOffset = number;
  } else {
    Location loc = {LOC_CONSTANT, number};
    Emit(OP_MOV, expr_location, &loc);
  }
}

static void CodegenExpression(Fn* fn, Node* expression, Location* expr_location, BOOL is_lvalue) {
  printf("\n; ");
  PrintNode(expression, 0);
  switch (expression->NodeType) {
    case NODE_NUMBER: {
      CodegenNumber(fn, ((Number*)expression)->NumberValue, expr_location);
      return;
    }

    case NODE_STRING: {
      String* str = (String*)expression;
      if (expr_location->LocationSpace == LOC_NONE) {
	expr_location->LocationSpace  = LOC_STRING;
	expr_location->LocationOffset = str->StringLabel;
      } else {
	Location loc = {LOC_STRING, str->StringLabel};
	Emit(OP_MOV, expr_location, &loc);
      }
      return;
    }

    case NODE_REFERENCE: {
      const char* name = ((Reference*)expression)->ReferenceName;

      // Check if it's a constant
      Cons* nodes = Consts;
      while (nodes) {
	Const* constant = nodes->Value;
	if (strcmp(constant->ConstName, name) == 0)  {
	  CodegenNumber(fn, constant->ConstValue, expr_location);
	  return;
	}
	nodes = nodes->Tail;
      }

      // Check if it's a varaible
      if (expr_location->LocationSpace == LOC_NONE) {
	GetVarLocation(fn, name, expr_location, is_lvalue);
      } else {
	Location loc = { LOC_NONE };
	GetVarLocation(fn, name, &loc, is_lvalue);
	Emit(OP_MOV, expr_location, &loc);
      }
      return;
    }

    case NODE_CALL: {
      CodegenCall(fn, (Call*)expression, expr_location, is_lvalue);
      return;
    }
  }

  fprintf(stderr, "Expression Type Not implemented\n");
  exit(1);
}

static void CodegenSet(Fn* fn, Set* set) {

  Location dst_location = { LOC_NONE };
  CodegenExpression(fn, set->SetDestination, &dst_location, TRUE);

  Location src_location = { LOC_NONE };
  CodegenExpression(fn, set->SetValue, &src_location, FALSE);


  NewLine();
  printf("MOV QWORD r12, ");
  PrintLocation(&src_location);

  Emit(OP_MOV, &TempRegister, &dst_location);
  NewLine();

  if (set->SetIsEightBit) {
    printf("MOV BYTE [r11], r12b");
  } else {
    printf("MOV QWORD [r11], r12");
  }
}

static void CodegenReturn(Fn* fn, Return* ret) {
  Location loc;
  AcquireTemp(&loc);

  if (ret->ReturnValue) {
    CodegenExpression(fn, ret->ReturnValue, &loc, FALSE);
    Emit(OP_MOV, &ReturnLocation, &loc);
  }
  EmitRet(fn);
}

static void CodegenIf(Fn* fn, If* if_statement) {
  NUM else_label = GetLabel();
  NUM end_label = GetLabel();

  Location condition = { LOC_NONE };
  CodegenExpression(fn, if_statement->IfCondition, &condition, FALSE);

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

  NUM OldContinueLabel = CurrentContinueLabel;
  NUM OldBreakLabel = CurrentBreakLabel;
  CurrentContinueLabel = start_label;
  CurrentBreakLabel = done_label;

  PlaceLabel(start_label);
  CodegenExpression(fn, while_loop->WhileCondition, &condition, FALSE);

  Emit(OP_TEST, &condition, &condition);
  EmitJump(OP_JZ, done_label);

  CodegenBlock(fn, while_loop->WhileBody);
  EmitJump(OP_JMP, start_label);

  PlaceLabel(done_label);

  CurrentContinueLabel = OldContinueLabel;
  CurrentBreakLabel = OldBreakLabel;
}

static void CodegenBreak(Fn* fn) {
  NewLine();
  printf("JMP _label%ld", CurrentBreakLabel);
}

static void CodegenContinue(Fn* fn) {
  NewLine();
  printf("JMP _label%ld", CurrentContinueLabel);
}

static void CodegenStatement(Fn* fn, Node* statement) {
  switch (statement->NodeType) {
    case NODE_SET: CodegenSet(fn, (Set*)statement); return;
    case NODE_RETURN: CodegenReturn(fn, (Return*)statement); return;
    case NODE_IF: CodegenIf(fn, (If*)statement); return;
    case NODE_WHILE: CodegenWhile(fn, (While*)statement); return;
    case NODE_BREAK: CodegenBreak(fn); return;
    case NODE_CONTINUE: CodegenContinue(fn); return;
    case NODE_VAR: return;
  }

  Location loc = { LOC_NONE };
  CodegenExpression(fn, statement, &loc, FALSE);
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

  NUM argc = Length(fn->FnParamNames);

  for (int i = 0; i < argc; i++)
    EmitPush(&ArgumentLocationsReg[i]);

  CurrentStackOffset = -GetStackFrameSize(fn);
  NewLine();
  printf("SUB rsp, %ld", STACK_FRAME - argc * NUM_SIZE);

  CodegenBlock(fn, fn->FnBlock);

  EmitRet(fn);

  printf("\n\n");
}

void GlobalCodegen() {
  // Externs
  Cons* efn = Externs;
  while (efn) {
    printf("extern %s\n", (const char*)efn->Value);
    efn = efn->Tail;
  }

  // Uninitialized static variables
  printf("segment .bss\n");
  Cons* statics = StaticVariables;
  while (statics) {
    Var* stat = statics->Value;
    printf("%s: resq 1\n", stat->VarName);
    statics = statics->Tail;
  }

  // Strings
  printf("segment .rodata\n");
  Cons* strings = Strings;
  NUM str_index = 0;
  while (strings) {
    String* str = strings->Value;
    str->StringLabel = str_index;

    strings = strings->Tail;
    str_index = str_index + 1;

    printf("_string%ld: db \"%s\", 0\n", str->StringLabel, str->StringStr);
  }

  printf("segment .text\n");
  Cons* fn = Functions;
  while (fn) {
    CodegenFn((Fn*)fn->Value);
    fn = fn->Tail;
  }
}
