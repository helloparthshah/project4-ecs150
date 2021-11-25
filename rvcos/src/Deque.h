#ifndef DEQUE
#define DEQUE
#include "RVCOS.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct Node {
  struct Node *next;
  struct Node *prev;
  TThreadID val;
};

typedef struct {
  struct Node *head;
  struct Node *tail;
} Deque;

typedef struct {
  Deque *high;
  Deque *norm;
  Deque *low;
} PrioDeque;

typedef struct {
  uint32_t *ctx;
  TThreadEntry entry;
  void *param;
  TThreadID id;
  TMemorySize memsize;
  TThreadPriority priority;
  TThreadState state;
  Deque *waited_by;
  uint32_t return_val;
  int sleep_for;
  int wait_timeout;
  int mutex_timeout;
  TMutexID waiting_for_mutex;
  Deque *mutex_id;
} Thread;

typedef struct {
  TMutexID id;
  TThreadID owner;
  TMutexState state;
  PrioDeque *waiting;
} Mutex;

typedef struct {
  Thread *threads;
  size_t used;
  size_t size;
} TCBArray;

typedef struct {
  Mutex *mutexes;
  size_t used;
  size_t size;
} MCBArray;

typedef struct {
  SMemoryPoolFreeChunk *pools;
  size_t used;
  size_t size;
} MemoryPoolArray;

// Struct for video controller
typedef struct {
  const TTextCharacter *buffer;
  TMemorySize writesize;
  TThreadID tid;
} TextBuffer;

struct TextNode {
  struct TextNode *next;
  struct TextNode *prev;
  TextBuffer val;
};

typedef struct {
  struct TextNode *head;
  struct TextNode *tail;
} TBDeque;

void tb_push_back(volatile TBDeque *d, TextBuffer v);
TextBuffer tb_pop_front(volatile TBDeque *d);
int isEmptyTB(volatile TBDeque *d);

void tcb_init(volatile TCBArray *a, size_t initialSize);
void tcb_push_back(volatile TCBArray *a, Thread element);

void mutex_init(volatile MCBArray *a, size_t initialSize);
void mutex_push_back(volatile MCBArray *a, Mutex element);

void mp_init(volatile MemoryPoolArray *a);
void mp_push_back(volatile MemoryPoolArray *a, SMemoryPoolFreeChunk element);

Deque *dmalloc();
PrioDeque *pdmalloc();
TBDeque *tbmalloc();

void print(volatile Deque *d, uint32_t line);
uint32_t size(volatile Deque *d);
void push_front(volatile Deque *d, TThreadID v);
void push_back(volatile Deque *d, TThreadID v);

void removeT(volatile Deque *d, TThreadID v);

int isEmpty(volatile Deque *d);

TThreadID pop_front(volatile Deque *d);
TThreadID pop_back(volatile Deque *d);

TThreadID front(volatile Deque *d);
TThreadID end(volatile Deque *d);

void push_back_prio(volatile PrioDeque *d, TThreadID tid);
TThreadID pop_front_prio(volatile PrioDeque *d);
void remove_prio(volatile PrioDeque *d, TThreadID tid);
uint32_t pd_size(volatile PrioDeque *d);

#endif