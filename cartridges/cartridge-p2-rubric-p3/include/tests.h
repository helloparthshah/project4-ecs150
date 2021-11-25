#ifndef __TESTS_H__
#define __TESTS_H__

#define CONTROLLER  (*((volatile SControllerStatus *)0x40000018))

int check_proceed(char *query);
void busy_wait(int cycles);

typedef int (*test_fn)();
typedef struct {
    char name[128];
    test_fn tfn;
    int rc;
} test;

// hardware
int test_jump_cartridge();
int test_controller();
int test_writetext();
int test_timer();

// threads
int test_rvcinitialize();
int test_contextswitch();
int test_status();
int test_sleep();
int test_wait();
int test_fullapp();

#endif
