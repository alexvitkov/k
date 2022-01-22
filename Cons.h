#pragma once

typedef struct Cons {
  void* Value;
  struct Cons* Tail;
} Cons;


Cons* Tail(Cons* list);
Cons* Append(Cons** list, void* value);
