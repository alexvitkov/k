#include "Util.h"
#include <stdio.h>
#include <stdlib.h>

char* ReadFile(const char* filename) {
  FILE* f = fopen(filename, "r");
  if (!f) {
    return 0;
  }

  if (fseek(f, 0, SEEK_END) < 0) {
    fclose(f);
    return 0;
  }
    
  long length = ftell(f);

  char* ptr = (char*)malloc(length + 1);
  if (!ptr) {
    fclose(f);
    return 0;
  }

  ptr[length] = 0;
      
  rewind(f);
  fread(ptr, 1, length, f);
  fclose(f);
  return ptr;
}
