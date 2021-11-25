#include "RVCOS.h"
#include "Deque.h"
#include <stdint.h>
#include <string.h>

#define CONTROLLER (*((volatile uint32_t *)0x40000018))
#define VIP (*((volatile uint32_t *)0x40000004))

volatile TCBArray tcb;
volatile MCBArray mcb;
volatile PrioDeque *ready_queue;
volatile Deque *threads_waiting;
volatile Deque *threads_blocked_on_mutexes;
volatile Deque *threads_sleeping;
volatile TBDeque *text_buffer_queue;
volatile uint32_t ticks = 0;
volatile uint32_t cart_gp;
volatile int curr_running = 0;

volatile MemoryPoolArray memory_pool_array;

volatile char *VIDEO_MEMORY = (volatile char *)(0x50000000 + 0xFE800);

void write(const TTextCharacter *c, uint32_t line) {
  for (uint32_t i = 0; i < strlen(c); i++) {
    VIDEO_MEMORY[line * 0x40 + i] = c[i];
  }
}

void writei(uint32_t c, int line) {
  char hex[32];
  sprintf(hex, "%x", c);
  write(hex, line);
}

void switch_context(uint32_t **oldctx, uint32_t *newctx);

extern void csr_write_mie(uint32_t);

extern void csr_enable_interrupts(void);

extern void csr_disable_interrupts(void);

uint32_t call_on_other_gp(volatile void *param, volatile TThreadEntry entry,
                          uint32_t gp);

__attribute__((always_inline)) inline void set_tp(uint32_t tp) {
  asm volatile("lw tp, 0(%0)" ::"r"(&tp));
}

__attribute__((always_inline)) inline uint32_t get_tp(void) {
  uint32_t result;
  asm volatile("mv %0,tp" : "=r"(result));
  return result;
}

__attribute__((always_inline)) inline void set_ra(uint32_t tp) {
  asm volatile("lw ra, 0(%0)" ::"r"(tp));
}

uint32_t *initialize_stack(uint32_t *sp, TThreadEntry fun, void *param,
                           uint32_t tp) {
  sp--;
  *sp = (uint32_t)fun; // sw      ra,48(sp)
  sp--;
  *sp = tp; // sw      tp,44(sp)
  sp--;
  *sp = 0; // sw      t0,40(sp)
  sp--;
  *sp = 0; // sw      t1,36(sp)
  sp--;
  *sp = 0; // sw      t2,32(sp)
  sp--;
  *sp = 0; // sw      s0,28(sp)
  sp--;
  *sp = 0; // sw      s1,24(sp)
  sp--;
  *sp = (uint32_t)param; // sw      a0,20(sp)
  sp--;
  *sp = 0; // sw      a1,16(sp)
  sp--;
  *sp = 0; // sw      a2,12(sp)
  sp--;
  *sp = 0; // sw      a3,8(sp)
  sp--;
  *sp = 0; // sw      a4,4(sp)
  sp--;
  *sp = 0; // sw      a5,0(sp)
  return sp;
}

TThreadReturn idleThread(void *param) {
  csr_enable_interrupts();
  csr_write_mie(0x888);
  while (1)
    ;
  return 0;
}

void dec_tick() {
  // Looping through the threads to check which are sleeping
  // Loop through threads_sleeping and decrement sleep_for
  int s = 0;
  s = size(threads_sleeping);
  for (int i = 0; i < s; i++) {
    TThreadID t = pop_front(threads_sleeping);
    if (tcb.threads[t].sleep_for == 0) {
      tcb.threads[t].state = RVCOS_THREAD_STATE_READY;
      push_back_prio(ready_queue, t);
    } else {
      tcb.threads[t].sleep_for--;
      push_back(threads_sleeping, t);
    }
  }

  s = size(threads_waiting);
  for (int i = 0; i < s; i++) {
    TThreadID t = pop_front(threads_waiting);
    if (tcb.threads[t].wait_timeout == 0) {
      tcb.threads[t].state = RVCOS_THREAD_STATE_READY;
      push_back_prio(ready_queue, t);
    } else {
      tcb.threads[t].wait_timeout--;
      push_back(threads_waiting, t);
    }
  }

  s = size(threads_blocked_on_mutexes);
  for (int i = 0; i < s; i++) {
    TThreadID t = pop_front(threads_blocked_on_mutexes);

    if (tcb.threads[t].mutex_timeout == 0) {
      tcb.threads[t].mutex_timeout = -1;
      remove_prio(mcb.mutexes[tcb.threads[t].waiting_for_mutex].waiting, t);
      tcb.threads[t].state = RVCOS_THREAD_STATE_READY;
      push_back_prio(ready_queue, t);
    } else if (tcb.threads[t].mutex_timeout > 0) {
      tcb.threads[t].mutex_timeout--;
      push_back(threads_blocked_on_mutexes, t);
    }
  }
}

TThreadReturn skeleton(void *param) {
  // Setting the thread pointer to the current running
  set_tp(curr_running);
  // Enabling interrupts
  csr_enable_interrupts();
  csr_write_mie(0x888);
  // Calling the entry function and storing the return value
  uint32_t ret_value =
      call_on_other_gp(tcb.threads[curr_running].param,
                       tcb.threads[curr_running].entry, cart_gp);
  // Disabling the interrupts
  csr_write_mie(0x000);
  csr_disable_interrupts();
  // Calling terminate
  RVCThreadTerminate(curr_running, ret_value);
  return 0;
}

void scheduler() {
  int old_running = curr_running;

  // Add back only if it was running previously
  if (tcb.threads[old_running].state == RVCOS_THREAD_STATE_RUNNING) {
    tcb.threads[old_running].state = RVCOS_THREAD_STATE_READY;
    push_back_prio(ready_queue, old_running);
  }

  // Getting the highest priority thread
  curr_running = pop_front_prio(ready_queue);

  // Switching the context
  if (tcb.threads[curr_running].state == RVCOS_THREAD_STATE_READY) {
    tcb.threads[curr_running].state = RVCOS_THREAD_STATE_RUNNING;
    if (old_running != curr_running) {
      switch_context((uint32_t **)&tcb.threads[old_running].ctx,
                     tcb.threads[curr_running].ctx);
    }
  }
}

extern volatile int line;

extern uint8_t _heap_base;
extern uint8_t _pool_size;

#define MTIME_LOW (*((volatile uint32_t *)0x40000008))
#define MTIME_HIGH (*((volatile uint32_t *)0x4000000C))
#define MTIMECMP_LOW (*((volatile uint32_t *)0x40000010))
#define MTIMECMP_HIGH (*((volatile uint32_t *)0x40000014))

TStatus RVCInitialize(uint32_t *gp) {
  if (gp == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;

  // Storing the cartridge gp
  cart_gp = (uint32_t)gp;
  // Resetting the ticks
  ticks = 0;
  // Initializing the system memory pool
  initSystemPool();
  uint32_t p;
  // Initializing the ready queue
  ready_queue = pdmalloc();
  threads_sleeping = dmalloc();
  threads_waiting = dmalloc();
  threads_blocked_on_mutexes = dmalloc();
  text_buffer_queue = tbmalloc();
  // Initializing the mutexes
  tcb_init(&tcb, 256);
  mutex_init(&mcb, 256);

  if (ready_queue == NULL || threads_sleeping == NULL ||
      threads_waiting == NULL || tcb.threads == NULL || mcb.mutexes == NULL)
    return RVCOS_STATUS_FAILURE;

  // Create idle thread
  void *ptr;
  RVCMemoryAllocate(1024, &ptr);
  tcb_push_back(&tcb, (Thread){
                          .ctx = initialize_stack(ptr + 1024, idleThread, 0, 0),
                          .param = 0,
                          .entry = idleThread,
                          .id = tcb.used,
                          .priority = RVCOS_THREAD_PRIORITY_LOWEST,
                          .memsize = 1024,
                          .sleep_for = 0,
                          .wait_timeout = 0,
                          .state = RVCOS_THREAD_STATE_READY,
                      });
  curr_running = tcb.used;
  set_tp(curr_running);
  // Create main thread
  tcb_push_back(&tcb, (Thread){
                          .id = tcb.used,
                          .priority = RVCOS_THREAD_PRIORITY_NORMAL,
                          .state = RVCOS_THREAD_STATE_RUNNING,
                          .sleep_for = 0,
                          .wait_timeout = 0,
                          .mutex_timeout = -1,
                          .mutex_id = dmalloc(),
                      });
  uint64_t NewCompare = (((uint64_t)MTIME_HIGH) << 32) | MTIME_LOW;
  NewCompare += RVCOS_TICKS_MS;
  MTIMECMP_HIGH = NewCompare >> 32;
  MTIMECMP_LOW = NewCompare;
  csr_enable_interrupts();
  csr_write_mie(0x888);
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCTickMS(uint32_t *tickmsref) {
  // Returnign the current ticksms
  if (tickmsref == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  *tickmsref = RVCOS_TICKS_MS;
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCTickCount(TTickRef tickref) {
  if (tickref == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  // Returning the current tick count
  *tickref = ticks;
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadCreate(TThreadEntry entry, void *param, TMemorySize memsize,
                        TThreadPriority prio, TThreadIDRef tid) {
  if (entry == NULL || tid == NULL || memsize == 0 || prio > 3)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  // Creating the thread and adding to the tcb
  *tid = tcb.used;
  tcb_push_back(&tcb, (Thread){
                          .entry = entry,
                          .id = tcb.used,
                          .priority = prio,
                          .param = param,
                          .memsize = memsize,
                          .state = RVCOS_THREAD_STATE_CREATED,
                          .sleep_for = 0,
                          .wait_timeout = 0,
                          .mutex_timeout = -1,
                          .mutex_id = dmalloc(),
                      });
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadDelete(TThreadID thread) {
  if (tcb.threads[thread].id != thread)
    return RVCOS_STATUS_ERROR_INVALID_ID;
  Thread t;
  free(tcb.threads[thread].ctx);
  tcb.threads[thread] = t;
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadActivate(TThreadID thread) {
  if (tcb.threads[thread].id != thread)
    return RVCOS_STATUS_ERROR_INVALID_ID;
  // Initializing context
  void *ptr;
  RVCMemoryAllocate(tcb.threads[thread].memsize, &ptr);
  tcb.threads[thread].ctx =
      initialize_stack(ptr + tcb.threads[thread].memsize, skeleton,
                       tcb.threads[thread].param, thread);
  if (tcb.threads[thread].ctx == NULL)
    return RVCOS_STATUS_FAILURE;
  //  Setting the state to ready
  tcb.threads[thread].state = RVCOS_THREAD_STATE_READY;
  push_back_prio(ready_queue, thread);
  scheduler();
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadTerminate(TThreadID thread, TThreadReturn returnval) {
  if (tcb.threads[thread].id != thread)
    return RVCOS_STATUS_ERROR_INVALID_ID;
  // Setting state to dead
  tcb.threads[thread].state = RVCOS_THREAD_STATE_DEAD;
  // Setting the return value
  tcb.threads[thread].return_val = returnval;
  // If current is waited by then set all to ready
  if (tcb.threads[thread].waited_by != NULL) {
    int s = size(tcb.threads[thread].waited_by);
    for (int i = 0; i < s; i++) {
      uint32_t wid = pop_front(tcb.threads[thread].waited_by);
      if (tcb.threads[wid].state == RVCOS_THREAD_STATE_WAITING) {
        tcb.threads[wid].state = RVCOS_THREAD_STATE_READY;
        tcb.threads[wid].wait_timeout = 0;
        removeT(threads_waiting, wid);
        push_back_prio(ready_queue, wid);
        scheduler();
      }
    }
  }

  // Releasing the mutex
  if (tcb.threads[thread].mutex_id != NULL) {
    int s = size(tcb.threads[thread].mutex_id);
    for (int i = 0; i < s; i++) {
      uint32_t mid = pop_front(tcb.threads[thread].mutex_id);
      RVCMutexRelease(mid);
    }
  }

  scheduler();
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadWait(TThreadID thread, TThreadReturnRef returnref,
                      TTick timeout) {
  if (tcb.threads[thread].id != thread)
    return RVCOS_STATUS_ERROR_INVALID_ID;
  if (returnref == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;

  TThreadID wid = curr_running;

  if (timeout == RVCOS_TIMEOUT_IMMEDIATE) {
    scheduler();
    return RVCOS_STATUS_SUCCESS;
  }

  // Setting state to waiting
  tcb.threads[wid].state = RVCOS_THREAD_STATE_WAITING;

  // Adding to the waiting queue
  if (tcb.threads[thread].waited_by == NULL)
    tcb.threads[thread].waited_by = dmalloc();
  if (tcb.threads[thread].waited_by == NULL)
    return RVCOS_STATUS_FAILURE;

  push_back(tcb.threads[thread].waited_by, wid);

  // Setting the timeout
  if (timeout != RVCOS_TIMEOUT_INFINITE) {
    tcb.threads[wid].wait_timeout = timeout;
    push_back(threads_waiting, wid);
  }

  // remove_prio(ready_queue, wid);

  // Scheduling till it's dead
  while (tcb.threads[thread].state != RVCOS_THREAD_STATE_DEAD) {
    scheduler();
  }

  // Setting the return value
  *returnref = tcb.threads[thread].return_val;
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadSleep(TTick tick) {
  if (tick == RVCOS_TIMEOUT_INFINITE)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;

  if (tick != RVCOS_TIMEOUT_IMMEDIATE) {
    TThreadID sid = curr_running;
    // Setting state to waiting
    tcb.threads[sid].state = RVCOS_THREAD_STATE_WAITING;
    tcb.threads[sid].sleep_for = tick;
    // Adding to the sleeping queue
    push_back(threads_sleeping, sid);
    // Remove from ready queue
    remove_prio(ready_queue, sid);
  }
  // Calling scheduler
  scheduler();

  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadID(TThreadIDRef threadref) {
  // Getting the tp
  *threadref = get_tp();
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadState(TThreadID thread, TThreadStateRef stateref) {
  if (tcb.threads[thread].id != thread)
    return RVCOS_STATUS_ERROR_INVALID_ID;
  if (stateref == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  // returning the state
  *stateref = tcb.threads[thread].state;
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCMutexCreate(TMutexIDRef mutexref) {
  if (mutexref == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  // Creating the mutex and adding it to the mutex_array
  *mutexref = mcb.used;
  mutex_push_back(&mcb, (Mutex){
                            .id = mcb.used,
                            .owner = RVCOS_THREAD_ID_INVALID,
                            .waiting = pdmalloc(),
                            .state = RVCOS_MUTEX_STATE_UNLOCKED,
                        });
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCMutexDelete(TMutexID mutex) {
  // Deleting the mutex
  // mutex_array.mutexes[mutex].state = RVCOS_MUTEX_STATE_DELETED;
  return RVCOS_STATUS_SUCCESS;
}
TStatus RVCMutexQuery(TMutexID mutex, TThreadIDRef ownerref) {
  if (mcb.mutexes[mutex].id != mutex)
    return RVCOS_STATUS_ERROR_INVALID_ID;
  if (ownerref == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  // Returning the owner
  *ownerref = mcb.mutexes[mutex].owner;
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCMutexAcquire(TMutexID mutex, TTick timeout) {
  if (mcb.mutexes[mutex].id != mutex)
    return RVCOS_STATUS_ERROR_INVALID_ID;

  if (timeout == RVCOS_TIMEOUT_IMMEDIATE) {
    if (mcb.mutexes[mutex].state == RVCOS_MUTEX_STATE_LOCKED) {
      return RVCOS_STATUS_FAILURE;
    }
    return RVCOS_STATUS_SUCCESS;
  }

  if (mcb.mutexes[mutex].state == RVCOS_MUTEX_STATE_UNLOCKED) {
    push_back(tcb.threads[curr_running].mutex_id, mutex);
    // Setting the owner
    mcb.mutexes[mutex].owner = curr_running;
    // Setting the state
    mcb.mutexes[mutex].state = RVCOS_MUTEX_STATE_LOCKED;
    return RVCOS_STATUS_SUCCESS;
  } else {
    if (timeout == RVCOS_TIMEOUT_INFINITE) {
      // Setting the timeout
      tcb.threads[curr_running].mutex_timeout = -1;
    } else {
      // Setting the timeout
      tcb.threads[curr_running].mutex_timeout = timeout;
    }
    // Adding to the waiting queue
    push_back_prio(mcb.mutexes[mutex].waiting, curr_running);
    //  Setting the mutex current thread is waiting on
    tcb.threads[curr_running].waiting_for_mutex = mutex;
    tcb.threads[curr_running].state = RVCOS_THREAD_STATE_WAITING;
    push_back(threads_blocked_on_mutexes, curr_running);
  }
  while (tcb.threads[curr_running].state == RVCOS_THREAD_STATE_WAITING) {
    scheduler();
  }
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCMutexRelease(TMutexID mutex) {
  // Releaseing the mutex
  if (mcb.mutexes[mutex].id != mutex)
    return RVCOS_STATUS_ERROR_INVALID_ID;
  if (mcb.mutexes[mutex].state != RVCOS_MUTEX_STATE_LOCKED)
    return RVCOS_STATUS_FAILURE;
  if (mcb.mutexes[mutex].owner != curr_running)
    return RVCOS_STATUS_FAILURE;

  // Setting the state
  mcb.mutexes[mutex].state = RVCOS_MUTEX_STATE_UNLOCKED;
  // Removing the mutex
  removeT(tcb.threads[curr_running].mutex_id, mutex);

  // Checking if there are any threads waiting for the mutex
  if (pd_size(mcb.mutexes[mutex].waiting) > 0) {
    // Setting the owner
    mcb.mutexes[mutex].owner = pop_front_prio(mcb.mutexes[mutex].waiting);
    // Removing from the blocked timeout queue
    removeT(threads_blocked_on_mutexes, mcb.mutexes[mutex].owner);
    // Setting the state
    mcb.mutexes[mutex].state = RVCOS_MUTEX_STATE_LOCKED;

    push_back(tcb.threads[mcb.mutexes[mutex].owner].mutex_id, mutex);

    push_back(tcb.threads[mcb.mutexes[mutex].owner].mutex_id, mutex);
    // Adding to the ready queue
    tcb.threads[mcb.mutexes[mutex].owner].state = RVCOS_THREAD_STATE_READY;
    push_back_prio(ready_queue, mcb.mutexes[mutex].owner);
  } else {
    // Resetting the owner
    mcb.mutexes[mutex].owner = RVCOS_THREAD_ID_INVALID;
  }
  scheduler();
  return RVCOS_STATUS_SUCCESS;
}

volatile int cursor = 0;
/*
Normal 0
Esc 1
[ 2
digit1 3
; 4
digit2 5
digit3 6
 */
volatile int char_mode = 0;

volatile char current_char_list[256];
volatile int current_char_list_index = 0;

void write_to_videomem(char c) {
  char_mode = 0;
  current_char_list_index = 0;
  // if backspace move cursor back
  if (c == '\b') {
    if (cursor > 0) {
      // VIDEO_MEMORY[--cursor] = ' ';
      cursor--;
    }
  } else if (c == '\r') {
    // Carriage return
    cursor -= cursor % 0x40;
  } else if (c == '\n') {
    // Jump to nextline
    cursor += 0x40;
    // Move back to start of the line
    cursor -= cursor % 0x40;
  } else if (c != '\0') {
    // else printing the charactor
    VIDEO_MEMORY[cursor++] = c;
  }
}

// Outputs charactor based on char_mode
void output_char(const char *buffer, int writesize) {
  for (int i = 0; i < writesize; i++) {
    char c = buffer[i];
    if (c == '\x1B') {
      // Setting char_mode to esc
      char_mode = 1;
    } else if (c == '[') {
      // Setting char_mode to [
      if (char_mode == 1)
        char_mode = 2;
      else {
        write_to_videomem(c);
      }
    } else if ((c >= 'A' && c <= 'D') || c == 'H') {
      // Up, down, left, right
      if (char_mode == 2) {
        if (c == 'A') {
          // if (cursor > 0x40)
          cursor -= 0x40;
        } else if (c == 'B') {
          // if (cursor <= 0x40 * 35)
          cursor += 0x40;
        } else if (c == 'C') {
          // if ((cursor + 1) % 0x40 != 0)
          cursor += 1;
        } else if (c == 'D') {
          // if (cursor % 0x40 != 0)
          cursor -= 1;
        } else if (c == 'H') {
          cursor = 0;
        }
        char_mode = 0;
        current_char_list_index = 0;
      } else if (char_mode == 5 && c == 'H') {
        int line = 0;
        int column = 0;
        int i = 0;
        for (i = 0; i < current_char_list_index; i++) {
          if (current_char_list[i] == ';') {
            break;
          } else {
            line = line * 10 + current_char_list[i] - '0';
          }
        }
        // parse column from current_char_list
        for (i++; i < current_char_list_index; i++) {
          column = column * 10 + current_char_list[i] - '0';
        }
        // move cursor to line, column
        cursor = line * 0x40 + column;
        char_mode = 0;
        current_char_list_index = 0;
      } else {
        write_to_videomem(c);
      }
    } else if (char_mode == 3 && current_char_list_index == 1 &&
               current_char_list[0] == '2' && c == 'J') {
      // Clear the screen
      memset((void *)VIDEO_MEMORY, 0, 0x40 * 36);
      char_mode = 0;
      current_char_list_index = 0;
    } else if (c >= '0' && c <= '9') {
      if (char_mode == 2 || char_mode == 3) {
        current_char_list[current_char_list_index++] = c;
        char_mode = 3;
      } else if (char_mode == 4 || char_mode == 5) {
        current_char_list[current_char_list_index++] = c;
        char_mode = 5;
      } else {
        write_to_videomem(c);
      }
    } else if (c == ';') {
      if (char_mode == 3) {
        current_char_list[current_char_list_index++] = ';';
        char_mode = 4;
      } else {
        write_to_videomem(c);
      }
    } else {
      write_to_videomem(c);
    }
    if ((cursor) / 0x40 >= 36) {
      // Shifting everything up when reach end of screen
      memmove((void *)VIDEO_MEMORY, (void *)VIDEO_MEMORY + 0x40, 0x40 * 36);
      cursor -= 0x40;
    }
  }
}

void video_interrupt_handler(void) {
  VIP = 0x2;
  if (text_buffer_queue != NULL) {
    while (isEmptyTB(text_buffer_queue) == 0) {
      TextBuffer tb = tb_pop_front(text_buffer_queue);
      output_char(tb.buffer, tb.writesize);
      tcb.threads[tb.tid].state = RVCOS_THREAD_STATE_READY;
      push_back_prio(ready_queue, tb.tid);
      if (tcb.threads[tb.tid].priority > tcb.threads[curr_running].priority) {
        // push_back_prio(ready_queue, tb.tid);
        scheduler();
      }
    }
    // scheduler();
  }
}

TStatus RVCWriteText(const TTextCharacter *buffer, TMemorySize writesize) {
  if (buffer == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  // Pushing to text_buffer_queue
  tb_push_back(text_buffer_queue, (TextBuffer){
                                      .buffer = buffer,
                                      .writesize = writesize,
                                      .tid = curr_running,
                                  });
  tcb.threads[curr_running].state = RVCOS_THREAD_STATE_WAITING;
  scheduler();
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCReadController(SControllerStatusRef statusref) {
  if (!statusref)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  // Setting the read controller
  *(uint32_t *)statusref = CONTROLLER;
  return RVCOS_STATUS_SUCCESS;
}