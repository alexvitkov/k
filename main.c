#include "Common.h"
#include "Cons.h"
#include "Lex.h"
#include "Util.h"




int main(int argc, const char** argv) {
  char* file = ReadFile("test");
  if (!file) {
    printf("Failed to open file\n");
    return 1;
  }

  Cons* c = LexFile(file);

  while (c) {
    Token* t = c->Value;
    printf("%.5ld %.5ld %s\n", t->Type, t->Number, t->Str);
    c = c->Tail;
  }

  return 0;
}
