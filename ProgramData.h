#pragma once

#include "Common.h"
#include "Cons.h"

typedef struct Const {
  const char* ConstName;
  NUM ConstValue;
} Const;

extern Cons* Strings;
extern Cons* Externs;
extern Cons* Functions;
extern Cons* Consts;
extern Cons* StaticVariables;
