#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_ALLOCATION_COUNT 0x40

typedef struct nodesList freeNode, *freeNodeRef;

struct nodesList {
  struct nodesList *DNext;
};

typedef struct {
  int DCount;
  int DStructureSize;
  freeNodeRef DFirstFree;
} allocStruct, *allocStructRef;

void *MemoryAlloc(int size);

void AllocStructInit(allocStructRef alloc, int size);
void *AllocStructAllocate(allocStructRef alloc);
void AllocStructDeallocate(allocStructRef alloc, void *obj);

typedef struct {
  int DX;
  int DY;
  int DZ;
} SThreeDPos, *SThreeDPosRef;

typedef struct {
  int DSize;
  void *DBase;
} SMemoryPoolFreeChunk, *SMemoryPoolFreeChunkRef;

allocStruct freeChunks;
int SuspendAllocationOfFreeChunks = 0;
SMemoryPoolFreeChunk memory_pool_array[5];

SMemoryPoolFreeChunkRef AllocateFreeChunk(void);
void DeallocateFreeChunk(SMemoryPoolFreeChunkRef chunk);

int main(int argc, char *argv[]) {
  SMemoryPoolFreeChunkRef Array[MIN_ALLOCATION_COUNT * 2];

  AllocStructInit(&freeChunks, sizeof(SMemoryPoolFreeChunk));
  for (int Index = 0; Index < 5; Index++) {
    DeallocateFreeChunk(&memory_pool_array[Index]);
  }
  for (int Index = 0; Index < MIN_ALLOCATION_COUNT * 2; Index++) {
    Array[Index] = AllocateFreeChunk();
    printf("%d => %p\n", Index, Array[Index]);
  }
  return 0;
}

void *MemoryAlloc(int size) {
  printf("@line %d \n", __LINE__);
  AllocateFreeChunk();
  return malloc(
      size); // counter that counts the free memory blocks (number of). use
             // pointers to move between different memory blocks
  // use one pointer for the front and over write the previous stuff in the mem?
  // you can maintain a valid vector to see which one is free and which one is
  // occupied (memory block) use counter and pointer to know the size (pool
  // size) to know which one is used
}
/*
system pool - larger memory blocks inside? avail mem block? if you wanna use a
mem block, you request from the block pool is like a container to store memory;
out of the pool we allocate one memory block. initialize pool - initialize
pointer and size of the pool and then the free counter of blocks in the pool
(which is max size since every size is free) struct size and pool size

we have multiple pools; we use pool reference; maximum size of the pool ? ;
pool size will also increase; you have several pools;
you have lots of pool and you can allocate memory froma pool reference that
points to one of the pools; we have a mem block that we allocate size of a block
- 64 bytes (not sure).

when you call initialize - pool and another is struct size. struct size will
limit your size of entire pool; struct size is the size of the current free one;
or pool size;

companion - allocate pool in one reference (container for mem will be pool?)?
WTF?
*/

void AllocStructInit(allocStructRef alloc, int size) {
  alloc->DCount = 0;
  alloc->DStructureSize = size;
  alloc->DFirstFree = NULL;
}

void *AllocStructAllocate(allocStructRef alloc) {
  if (!alloc->DCount) {
    alloc->DFirstFree =
        MemoryAlloc(alloc->DStructureSize * MIN_ALLOCATION_COUNT);
    freeNodeRef Current = alloc->DFirstFree;
    for (int i = 0; i < MIN_ALLOCATION_COUNT; i++) {
      if (i + 1 < MIN_ALLOCATION_COUNT) {
        Current->DNext =
            (freeNodeRef)((uint8_t *)Current + alloc->DStructureSize);
        Current = Current->DNext;
      } else {
        Current->DNext = NULL;
      }
    }
    alloc->DCount = MIN_ALLOCATION_COUNT;
  }
  freeNodeRef NewStruct = alloc->DFirstFree;
  alloc->DFirstFree = alloc->DFirstFree->DNext;
  alloc->DCount--;
  return NewStruct;
}

void AllocStructDeallocate(allocStructRef alloc, void *obj) {
  freeNodeRef OldStruct = (freeNodeRef)obj;
  alloc->DCount++;
  OldStruct->DNext = alloc->DFirstFree;
  alloc->DFirstFree = OldStruct;
}

SMemoryPoolFreeChunkRef AllocateFreeChunk(void) {
  if (3 > freeChunks.DCount && !SuspendAllocationOfFreeChunks) {
    SuspendAllocationOfFreeChunks = 1;
    printf("@line %d \n", __LINE__);
    uint8_t *Ptr =
        MemoryAlloc(freeChunks.DStructureSize * MIN_ALLOCATION_COUNT);
    for (int Index = 0; Index < MIN_ALLOCATION_COUNT; Index++) {
      AllocStructDeallocate(&freeChunks,
                            Ptr + Index * freeChunks.DStructureSize);
    }
    SuspendAllocationOfFreeChunks = 0;
  }
  return (SMemoryPoolFreeChunkRef)AllocStructAllocate(&freeChunks);
}

void DeallocateFreeChunk(SMemoryPoolFreeChunkRef chunk) {
  AllocStructDeallocate(&freeChunks, (void *)chunk);
}

// initialize the entire pool; another function for allocate; delete the old
// one? use standard library companion