#ifndef __TESTS_H__
#define __TESTS_H__

#include <stdarg.h>

int WriteString(const char *format, ...);

#define CONTROLLER  (*((volatile SControllerStatus *)0x40000018))

#define RVCWriteText_LITERAL(s) \
            (RVCWriteText(s, sizeof(s) - 1))

int check_proceed(char *query);
void busy_wait(int cycles);

typedef int (*test_fn)();
typedef struct {
    char name[128];
    test_fn tfn;
    int rc;
} test;

// IO
int test_controller();
int test_writetext();
int test_writetext_vt100();
int test_writetext_blocking();
int test_timer();

// Threads
int test_rvcinitialize();
int test_contextswitch();
int test_status();
int test_sleep();
int test_wait();
int test_writetext_blocking();

// Memory
int test_memory_error();
int test_memory_functionality();

// Mutex
int test_mutex_error();
int test_mutex_functionality();

// Applications
int test_fullapp_p2();
int test_fullapp_p3();

#endif
