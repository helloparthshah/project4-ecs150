#include "Deque.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  Array tcb;
  initArray(&tcb, 256);
  insertArray(&tcb, 5);
  printf("%zu\n", tcb.size);
  return 0;
}