#include "Codegen.h"
#include "Common.h"
#include "Cons.h"
#include "Lex.h"
#include "Node.h"
#include "Parser.h"
#include "Util.h"

int main(int argc, const char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s filename > out.asm\n", argv[0]);
    return 1;
  }

  char* file = ReadFile(argv[1]);
  if (!file) {
    fprintf(stderr, "Failed to open file\n");
    return 1;
  }

  Cons* tokens = LexFile(file);
  if (!tokens) {
    fprintf(stderr, "Lex error\n");
    return 1;
  }

  Cons* nodes = ParseTopLevel(tokens);
  if (!nodes) {
    fprintf(stderr, "Parse error\n");
    return 1;
  }

  // while (c) {
  // Token* t = c->Value;
  // printf("%.5ld %.5ld %s\n", t->Type, t->Number, t->Str);
  // c = c->Tail;
  // }

  /* while (nodes) { */
  /*   Node* node = nodes->Value; */
  /*   PrintNode(node, 0); */
  /*   printf("\n\n"); */
  /*   nodes = nodes->Tail; */
  /* } */

  GlobalCodegen(nodes);

  return 0;
}
