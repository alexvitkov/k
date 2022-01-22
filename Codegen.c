#include "Node.h"

const NUM NUM_SIZE = 8u;
static NUM CurrentStackOffset;

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
  OP_ADD,
};
typedef NUM Operator;

const char* RegisterNames[] = {
  "rax",
  "rcx",
  "rdx",
  "rbx",
  "rsp",
  "rbp",
  "rsi",
  "rdi",
  "r8",
  "r9",
  "r10",
  "r11",
  "r12",
  "r13",
  "r14",
};

enum LocationSpace {
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

static void ReleaseTemp() {
  CurrentStackOffset += NUM_SIZE;
}

static void CodegenExpression(Fn* fn, Node* expression, Location* expr_location);

static void Emit(Operator op, Location* dst, Location* src) {
  Location* tmp = src;
  if (src->LocationSpace == LOC_RBP_RELATIVE && dst->LocationSpace == LOC_RBP_RELATIVE) {
    NewLine();
    printf("MOV ");
    tmp = &TempRegister;
    PrintLocation(tmp);
    printf(", ");
    PrintLocation(src);
  }

  NewLine();
  switch (op) {
  case OP_MOV: printf("MOV "); break;
  case OP_ADD: printf("ADD "); break;
  }
  PrintLocation(dst);
  printf(", ");
  PrintLocation(tmp);
}

static void EmitRet() {
  if (CurrentStackOffset != 0) {
    NewLine();
    printf("ADD rsp, %ld", -CurrentStackOffset);
  }
  NewLine();
  printf("RET");
}

static void CodegenOperator(Fn* fn, Call* call, Operator op, Location* destination) {
  AcquireTemp(destination);
  Emit(OP_MOV, destination, &ZeroLocation);

  Cons* args = call->CallArguments;
  while (args) {
    Node* arg_expression = args->Value;
    Location arg_location;
    CodegenExpression(fn, arg_expression, &arg_location);
    Emit(op, destination, &arg_location);

    args = args->Tail;
  }

  ReleaseTemp();
}

static void CodegenCall(Fn* fn, Call* call, Location* destination) {
  if (call->CallFunction->NodeType != NODE_REFERENCE) {
    fprintf(stderr, "Invalid function call\n");
    exit(1);
  }

  const char* fn_name = ((Reference*)call->CallFunction)->ReferenceName;

  if(strcmp(fn_name, "add") == 0) {
    CodegenOperator(fn, call, OP_ADD, destination);
    return;
  }

  fprintf(stderr, "Undefined function '%s'\n", fn_name);
}

static void CodegenExpression(Fn* fn, Node* expression, Location* expr_location) {
  switch (expression->NodeType) {
    case NODE_NUMBER: {
      expr_location->LocationSpace  = LOC_CONSTANT;
      expr_location->LocationOffset = ((Number*)expression)->NumberValue;
      return;
    }

    case NODE_REFERENCE: {
      GetVarLocation(fn, ((Reference*)expression)->ReferenceName, expr_location);
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
  Location expr_location;
  GetVarLocation(fn, set->SetName, &var_location);
  CodegenExpression(fn, set->SetValue, &expr_location);
  Emit(OP_MOV, &var_location, &expr_location);
}

static void CodegenReturn(Fn* fn, Return* ret) {
  if (ret->ReturnValue) {
    Location expr_location;
    CodegenExpression(fn, ret->ReturnValue, &expr_location);
    Emit(OP_MOV, &ReturnLocation, &expr_location);
  }
  NewLine();
  printf("POP rbp");
  EmitRet();
}

static void CodegenStatement(Fn* fn, Node* statement) {
  switch (statement->NodeType) {
    case NODE_SET: CodegenSet(fn, (Set*)statement); return;
    case NODE_RETURN: CodegenReturn(fn, (Return*)statement); return;
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
  printf("SUB rsp, %ld", -CurrentStackOffset);

  CodegenBlock(fn, fn->FnBlock);

  NewLine();
  printf("POP rbp");
  EmitRet();

  printf("\n\n");
}

void Codegen(Node* node) {
  switch (node->NodeType) {
    case NODE_FN: CodegenFn((Fn*)node); return;
  }
}
