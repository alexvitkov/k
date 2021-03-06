#pragma once
#include "Common.h"

typedef struct Cons {
  void* Value;
  struct Cons* Tail;
} Cons;


Cons* Append(Cons** list, void* value);
NUM Length(Cons* list);
void* Nth(Cons* list, NUM n);

Cons* LexFile(char* file);
