#include "Common.h"
#include "Cons.h"
#include "Token.h"
#include "Node.h"
#include "Util.h"
#include "ProgramData.h"

int main(int argc, const char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s INPUT_FILES > k.asm\n", argv[0]);
    return 1;
  }

  BOOL print_ast = FALSE;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-ast") == 0) {
      print_ast = TRUE;
      continue;

      }

    char* file = ReadFile(argv[i]);
    if (!file) {
      fprintf(stderr, "%s: Failed to open file\n", argv[i]);
      return 1;
    }

    Cons* tokens = LexFile(file);
    if (!tokens) {
      fprintf(stderr, "%s: Lex error\n", argv[i]);
      return 1;
    }
    
    if (!ParseFile(tokens)) {
      fprintf(stderr, "%s: Parse error\n", argv[i]);
      return 1;
    }
  }

  if (print_ast) {
    Cons* fn = Functions;
    while (fn) {
      PrintNode(fn->Value, 0);
      printf("\n\n");
      fn = fn->Tail;
    }
  }
  else {
    GlobalCodegen();
  }

  return 0;
}
