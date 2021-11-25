#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MIN_ALLOCATION_COUNT 0x40

typedef struct {
  uint8_t id;
  uint8_t *base_ptr;
  uint32_t size;

} SMemoryPool, *SMemoryPoolRef;

typedef struct nodesList freeNode, *freeNodeRef;

struct nodesList {
  struct nodesList *next, *prev;
};

freeNodeRef freeNodesList;
freeNodeRef allocNodesList;

void initSystemPool() {
  void *m_pMemBlock =
      malloc(10 * MIN_ALLOCATION_COUNT); // Allocate a memory block.
  printf("Malloced\n");
  if (m_pMemBlock != NULL) {
    for (unsigned long i = 0; i < 10; i++) {
      freeNodeRef pCurUnit =
          (freeNodeRef)((char *)m_pMemBlock + i * sizeof(freeNode));

      pCurUnit->prev = NULL;
      pCurUnit->next = freeNodesList; // Insert the new unit at head.

      if (freeNodesList != NULL) {
        freeNodesList->prev = pCurUnit;
      }
      freeNodesList = pCurUnit;
    }
  }
}

void *Alloc() {
  freeNodeRef pCurUnit = freeNodesList;
  freeNodesList = pCurUnit->next; // Get a unit from free linkedlist.
  if (NULL != freeNodesList) {
    freeNodesList->prev = NULL;
  }

  pCurUnit->next = allocNodesList;

  if (NULL != allocNodesList) {
    allocNodesList->prev = pCurUnit;
  }
  allocNodesList = pCurUnit;

  return (void *)((char *)pCurUnit + sizeof(freeNode));
}

int main() {
  initSystemPool();
  int *test[10];
  for (size_t i = 0; i < 10; i++) {
    test[i] = Alloc();
    printf("%p\n", test[i]);
  }
  return 0;
}