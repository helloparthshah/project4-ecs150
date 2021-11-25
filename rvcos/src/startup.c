#include "RVCOS.h"
#include <stdint.h>
#include <stdlib.h>

extern uint8_t _erodata[];
extern uint8_t _data[];
extern uint8_t _edata[];
extern uint8_t _sdata[];
extern uint8_t _esdata[];
extern uint8_t _bss[];
extern uint8_t _ebss[];
extern uint8_t _heap_base;

void *_sbrk(int incr) {
  static char *heap_end;
  char *prev_heap_end;

  if (heap_end == 0) {
    heap_end = (char *)&_heap_base;
  }
  prev_heap_end = heap_end;
  heap_end += incr;
  return (void *)prev_heap_end;
}

// Adapted from
// https://stackoverflow.com/questions/58947716/how-to-interact-with-risc-v-csrs-by-using-gcc-c-code
__attribute__((always_inline)) inline uint32_t csr_mstatus_read(void) {
  uint32_t result;
  asm volatile("csrr %0, mstatus" : "=r"(result));
  return result;
}

__attribute__((always_inline)) inline uint32_t csr_mcause_read(void) {
  uint32_t result;
  asm volatile("csrr %0, mcause" : "=r"(result));
  return result;
}

__attribute__((always_inline)) inline void csr_mstatus_write(uint32_t val) {
  asm volatile("csrw mstatus, %0" : : "r"(val));
}

__attribute__((always_inline)) extern inline void csr_write_mie(uint32_t val) {
  asm volatile("csrw mie, %0" : : "r"(val));
}

__attribute__((always_inline)) extern inline void csr_enable_interrupts(void) {
  asm volatile("csrsi mstatus, 0x8");
}

__attribute__((always_inline)) extern inline void csr_disable_interrupts(void) {
  asm volatile("csrci mstatus, 0x8");
}

#define MTIME_LOW (*((volatile uint32_t *)0x40000008))
#define MTIME_HIGH (*((volatile uint32_t *)0x4000000C))
#define MTIMECMP_LOW (*((volatile uint32_t *)0x40000010))
#define MTIMECMP_HIGH (*((volatile uint32_t *)0x40000014))
#define CONTROLLER (*((volatile uint32_t *)0x40000018))
#define IER (*((volatile uint32_t *)0x40000000))
#define CARTRIDGE (*((volatile uint32_t *)0x4000001C))

void init(void) {
  uint8_t *Source = _erodata;
  uint8_t *Base = _data < _sdata ? _data : _sdata;
  uint8_t *End = _edata > _esdata ? _edata : _esdata;
  while (Base < End) {
    *Base++ = *Source++;
  }
  Base = _bss;
  End = _ebss;
  while (Base < End) {
    *Base++ = 0;
  }

  // csr_write_mie(0x888);    // Enable all interrupt sources
  // csr_enable_interrupts(); // Global interrupt enable
  IER = 0x2;
  MTIMECMP_LOW = 1;
  MTIMECMP_HIGH = 0;
}

extern volatile uint32_t ticks;
extern void scheduler();
extern void dec_tick();
extern void video_interrupt_handler();
#define VIP (*((volatile uint32_t *)0x40000004))

void c_interrupt_handler(void) {
  uint32_t mcause = csr_mcause_read();
  if (mcause == 0x80000007) {
    uint64_t NewCompare = (((uint64_t)MTIMECMP_HIGH) << 32) | MTIMECMP_LOW;
    NewCompare += RVCOS_TICKS_MS;
    MTIMECMP_HIGH = NewCompare >> 32;
    MTIMECMP_LOW = NewCompare;
    if (CARTRIDGE & 0x1) {
      ticks++;
      dec_tick();
      scheduler();
    }
  } else if ((VIP & 0x2) && mcause == 0x8000000B) {
    video_interrupt_handler();
  }
}
