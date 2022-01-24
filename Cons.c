#include "Cons.h"
#include <stdlib.h>



Cons* Append(Cons** list, void* value) {
  if (*list) {
    Append(&((*list)->Tail), value);
  }
  else {
    Cons* newNode = malloc(sizeof(Cons));
    newNode->Value = value;
    newNode->Tail = 0;

    *list = newNode;
  }

  return *list;
}

NUM Length(Cons* list) {
  if (!list)
    return 0;
  else
    return 1 + Length(list->Tail);
}
