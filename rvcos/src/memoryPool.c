#include "Deque.h"
#include "RVCOS.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern volatile MemoryPoolArray memory_pool_array;

typedef struct nodesList node, *nodeRef;
// typedef struct Chunk chunk, *chunkRef;

/* struct Chunk {
  void *ptr;
  int size;
}; */

struct nodesList {
  // chunk *head, *tail;
  chunk nodes[256];
  int size;
  int used;
};

void pushNode(volatile node *d, void *ptr, int size) {
  /* if (d->used == d->size) {
    d->size *= 2;
    d->nodes = realloc(d->nodes, d->size * sizeof(chunk));
  } */
  d->nodes[d->used++] = (chunk){.ptr = ptr, .size = size};
  /* chunk *n;
  n = (chunk *)malloc(sizeof(chunk));
  if (n == NULL)
    return;
  // Setting the value of the node
  n->ptr = ptr;
  n->size = size;
  n->prev = d->tail;
  n->next = NULL;
  // If the only node then set to first
  if (d->head == NULL) {
    d->head = d->tail = n;
  } else {
    // Set next of tail to node and set tail to n
    d->tail->next = n;
    d->tail = n;
  } */
}

// chunk popNode(volatile node *d) {
// Get value
// struct Chunk v = {d->head->prev, d->head->next, d->head->ptr,
// d->head->size};
/* chunk *n = d->head;
if (d->head == d->tail) {
  d->head = d->tail = NULL;
} else
  // Set head to next
  d->head = n->next;
// free(n);
RVCMemoryPoolDeallocate(0, n);
return *n; */
// }

node freeNodesList;
// node allocNodesList;

extern uint8_t _pool_size;
extern uint8_t _heap_base;

void writei(int, int);

void initSystemPool() {
  pushNode(&freeNodesList, &_heap_base, &_pool_size);
  // void *p = malloc(&_pool_size);
  mp_init(&memory_pool_array);
  memory_pool_array.pools[0] = (SMemoryPoolFreeChunk){
      .DBase = &_heap_base,
      .DSize = &_pool_size,
      .Used = 0,
      .id = memory_pool_array.used,
  };
  /* mp_push_back(&memory_pool_array, (SMemoryPoolFreeChunk){
                                       .DBase = &_heap_base,
                                       .DSize = &_pool_size,
                                       .Used = 0,
                                       .id = memory_pool_array.used,
                                   }); */

  // RVCMemoryAllocate(sizeof(node), (void **)&freeNodesList);
  // freeNodesList->head = freeNodesList->tail = NULL;
}

/* void *Alloc() {
  nodeRef pCurUnit = freeNodesList;
  freeNodesList = pCurUnit->next; // Get a unit from free linkedlist.
  if (NULL != freeNodesList) {
    freeNodesList->prev = NULL;
  }

  pCurUnit->next = allocNodesList;

  if (NULL != allocNodesList) {
    allocNodesList->prev = pCurUnit;
  }
  allocNodesList = pCurUnit;

  return (void *)((char *)pCurUnit + sizeof(node));
} */

void write(char *, uint32_t);

TStatus RVCMemoryPoolCreate(void *base, TMemorySize size,
                            TMemoryPoolIDRef memoryref) {
  if (base == NULL || size == 0 || memoryref == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  chunk c = freeNodesList.nodes[0];
  if (c.size != size) {
    freeNodesList.nodes[0] = (chunk){.ptr = NULL, .size = 0};
    freeNodesList.used--;
    pushNode(&freeNodesList, c.ptr, base - c.ptr);
    pushNode(&freeNodesList, base + size, c.size - size - (base - c.ptr));
  }

  // writei(c.size, 20);

  // Creating the memory pool and pushing into array
  *memoryref = memory_pool_array.used;
  mp_push_back(&memory_pool_array, (SMemoryPoolFreeChunk){
                                       .DBase = base,
                                       .DSize = size,
                                       .Used = 0,
                                       .id = memory_pool_array.used,
                                   });

  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCMemoryPoolDelete(TMemoryPoolID memory) {
  if (memory >= memory_pool_array.used)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  memory_pool_array.pools[memory].DBase = NULL;
  memory_pool_array.pools[memory].DSize = 0;
  memory_pool_array.pools[memory].Used = 0;
  memory_pool_array.pools[memory].id = -1;
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCMemoryPoolQuery(TMemoryPoolID memory, TMemorySizeRef bytesleft) {
  if (memory >= memory_pool_array.used || bytesleft == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  *bytesleft = memory_pool_array.pools[memory].DSize -
               memory_pool_array.pools[memory].Used;
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCMemoryPoolAllocate(TMemoryPoolID memory, TMemorySize size,
                              void **pointer) {
  if (memory >= memory_pool_array.used || size == 0 || pointer == NULL) {
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  }
  if (size + memory_pool_array.pools[memory].Used >
      memory_pool_array.pools[memory].DSize)
    return RVCOS_STATUS_ERROR_INSUFFICIENT_RESOURCES;

  memory_pool_array.pools[memory].Used += size;

  if (memory == RVCOS_MEMORY_POOL_ID_SYSTEM) {
    // *pointer = memory_pool_array.pools[memory].DBase;
    memory_pool_array.pools[memory].DBase =
        (void *)((uint8_t *)memory_pool_array.pools[memory].DBase + size);
    *pointer = (void *)((uint8_t *)malloc(size));
  } else {
    *pointer = memory_pool_array.pools[memory].DBase;
    memory_pool_array.pools[memory].DBase =
        (void *)((uint8_t *)memory_pool_array.pools[memory].DBase + size);

    /* for (int i = 0; i < allocNodesList.size; i++) {
      if (allocNodesList.nodes[i].ptr >=
              memory_pool_array.pools[memory].DBase &&
          allocNodesList.nodes[i].ptr <=
              (void *)((uint8_t *)memory_pool_array.pools[memory].DBase +
                       memory_pool_array.pools[memory].DSize)) {
        c = allocNodesList.nodes[i];
        break;
      }
    }
    *pointer = c.ptr;
    c.ptr += size; */
    // *pointer = (void *)((uint8_t *)malloc(size));
  }
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCMemoryPoolDeallocate(TMemoryPoolID memory, void *pointer) {
  // frees the memory
  memory_pool_array.pools[memory].Used = 0;
  // free(pointer);
  memory_pool_array.pools[memory].DBase--;
  memory_pool_array.pools[memory].DSize++;
  pointer = memory_pool_array.pools[memory].DBase;
  // AllocStructDeallocate((allocStructRef)&freeChunks, pointer);
  return RVCOS_STATUS_SUCCESS;
}