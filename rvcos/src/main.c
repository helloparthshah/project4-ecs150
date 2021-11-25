#include "Deque.h"
#include "RVCOS.h"

#define CARTRIDGE (*((volatile uint32_t *)0x4000001C))

void enter_cartridge();

// Checking if cartridge is inserted
volatile int isInit = 0;

volatile uint32_t *saved_sp;

int main() {
  /* char *c = "\x1B[30;40HHello World!";
  RVCWriteText(c, strlen(c)); */
  while (1) {
    if (CARTRIDGE & 0x1 && isInit == 0) {
      isInit = 1;
      enter_cartridge();
    }
    if (!(CARTRIDGE & 0x1) && isInit == 1) {
      isInit = 0;
    }
  }
  return 0;
}

uint32_t c_syscall_handler(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3,
                           uint32_t a4, uint32_t code) {
  void *p0 = (void *)a0;
  void *p1 = (void *)a1;
  void *p2 = (void *)a2;
  void *p4 = (void *)a4;

  TStatus status = RVCOS_STATUS_FAILURE;
  // Calling syscalls based on ecall code
  if (code == 0) {
    status = RVCInitialize(p0);
  } else if (code == 1) {
    status = RVCThreadCreate((TThreadEntry)p0, p1, (TMemorySize)a2, a3, p4);
  } else if (code == 2) {
    status = RVCThreadDelete(a0);
  } else if (code == 3) {
    status = RVCThreadActivate(a0);
  } else if (code == 4) {
    status = RVCThreadTerminate(a0, a1);
  } else if (code == 5) {
    status = RVCThreadWait(a0, (TThreadReturnRef)p1, a2);
  } else if (code == 6) {
    status = RVCThreadID(p0);
  } else if (code == 7) {
    status = RVCThreadState(a0, p1);
  } else if (code == 8) {
    status = RVCThreadSleep(a0);
  } else if (code == 9) {
    status = RVCTickMS(p0);
  } else if (code == 10) {
    status = RVCTickCount(p0);
  } else if (code == 11) {
    status = RVCWriteText(p0, a1);
  } else if (code == 12) {
    status = RVCReadController((SControllerStatusRef)p0);
  } else if (code == 13) {
    status = RVCMemoryPoolCreate(p0, a1, p2);
  } else if (code == 14) {
    status = RVCMemoryPoolDelete(a0);
  } else if (code == 15) {
    status = RVCMemoryPoolQuery(a0, p1);
  } else if (code == 16) {
    status = RVCMemoryPoolAllocate(a0, a1, p2);
  } else if (code == 17) {
    status = RVCMemoryPoolDeallocate(a0, p1);
  } else if (code == 18) {
    status = RVCMutexCreate(p0);
  } else if (code == 19) {
    status = RVCMutexDelete(a0);
  } else if (code == 20) {
    status = RVCMutexQuery(a0, p1);
  } else if (code == 21) {
    status = RVCMutexAcquire(a0, a1);
  } else if (code == 22) {
    status = RVCMutexRelease(a0);
  }
  return status;
}


/* 
Pallette collection of colors (256 values) which is 32 bits ARGB
And each pixel is a value from the pallette

Alpha is opacity 0 to 255
RGB need to pre multiplied by a alpha value for transparency

3 types:
4 Bgs 512x288
64 large sprites 33x33
small sprites 1x1 to 16x16

8 Layers could be draw
bgs and small on any layer
large only on layer 4

Could be put anywhere on the screen

Each sprites and bgs have data sections and control blocks

4 palettes for bg and 4 for sprites

For update activated->pending->changes->activated

We set one bit to change mode
Last address in the video controller section
BLock it until the mode changes

Upcall- Get a call back once the create and activate 
Once per interrupt once stuff is completed
Above high priority
Upcall has thread id invalid

Create- Making space for offscreen buffer

Delete- deletes the offscreen buffer

Activate-  Upper left corner can be negative to be offscreen

Draw- Draws the pixels

Pallette create- allocates the 245B 
Data structure to track which graphics are currently using

Update- updates the colors
Default pallette provided

Extra credit
support for 8 pallettes
track dirty buffers- between activation and draw did you update the buffer
Use dma to move data around.
2 dma channels we could use 
32 bits per cycle
ld and store take 2 cycles
 */