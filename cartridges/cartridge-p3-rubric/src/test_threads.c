#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include "RVCOS.h"
#include "RVCGraphics.h"
#include "tests.h"


__attribute__((always_inline)) inline uint32_t *gp_read(void){
    uint32_t result;
    asm volatile ("addi %0, gp, 0" : "=r"(result));
    return (uint32_t *)result;
}
int test_rvcinitialize() {
    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;
    uint32_t *gp = gp_read();

    sprintf(&(graphics->text_data[3][3]), "GP: %p", gp);

    int rc = RVCInitialize(gp);
    sprintf(&(graphics->text_data[4][3]), "Initialize returned %d", rc);

    return check_proceed("Test passed?");
}

void WriteString_p2(int row, int col, char *str) {
    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;
    strcpy(&graphics->text_data[row][col], str);
}

TThreadReturn contextswitch_thread(void *param) {
    int row = 10;
    int col = 25;

    WriteString_p2(row++, col, "High Thread");

    return 0;
}

int test_contextswitch() {
    TThreadID HighThreadID;

    int row = 10;
    int col = 4;

    WriteString_p2(row++, col, "Main Thread");
    RVCThreadCreate(contextswitch_thread,NULL,2048,RVCOS_THREAD_PRIORITY_HIGH,&HighThreadID);
    WriteString_p2(row++, col, "High Created");
    RVCThreadActivate(HighThreadID);
    WriteString_p2(row++, col, "Main Exiting");

    int rc = check_proceed("Test passed?");
    return rc;
}

TThreadID MainThreadID, query_HighThreadID;

TThreadReturn query_HighPriorityThread(void *param){
    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;

    TThreadID CurrentThreadID;
    TThreadState ThreadState;

    int row = 12;
    int col = 26;

    WriteString_p2(row++, col, "High Thread:");
    RVCThreadID(&CurrentThreadID);
    if(CurrentThreadID != query_HighThreadID){
        WriteString_p2(row++, col, "Invalid Thread ID");
        return 1;
    }
    WriteString_p2(row++, col, "Valid Thread ID");
    WriteString_p2(row++, col, "Checking Main: ");
    RVCThreadState(MainThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_READY != ThreadState){
        WriteString_p2(row++, col, "Invalid State");
        return 1;
    }
    WriteString_p2(row++, col, "Valid State");
    WriteString_p2(row++, col, "Checking This: ");
    RVCThreadState(query_HighThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_RUNNING != ThreadState){
        WriteString_p2(row++, col, "Invalid State");
        return 1;
    }
    WriteString_p2(row++, col, "Valid State");
    return 0;
}


int do_test_status() {
    TThreadID CurrentThreadID;
    TThreadState ThreadState;

    int row = 12;
    int col = 6;

    WriteString_p2(row++, col, "Main Thread:");
    RVCThreadID(&MainThreadID);
    RVCThreadState(MainThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_RUNNING != ThreadState){
        WriteString_p2(row++, col, "Invalid State");
        return 1;
    }
    WriteString_p2(row++, col, "Valid State");
    RVCThreadCreate(query_HighPriorityThread,NULL,2048,RVCOS_THREAD_PRIORITY_HIGH,&query_HighThreadID);
    WriteString_p2(row++, col, "High Created:");
    RVCThreadState(query_HighThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_CREATED != ThreadState){
        WriteString_p2(row++, col, "Invalid State");
        return 1;
    }
    WriteString_p2(row++, col, "Valid State");
    RVCThreadActivate(query_HighThreadID);
    WriteString_p2(row++, col, "Checking High:");
    RVCThreadState(query_HighThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_DEAD != ThreadState){
        WriteString_p2(row++, col, "Invalid State");
        return 1;
    }
    WriteString_p2(row++, col, "Valid State");
    RVCThreadID(&CurrentThreadID);
    if(CurrentThreadID != MainThreadID){
        WriteString_p2(row++, col, "Invalid Main Thread ID");
        return 1;
    }
    WriteString_p2(row++, col, "Main Exiting");

    return 0;
}


int test_status() {

    if (do_test_status() != 0) {
        WriteString_p2(1,1, "Error");
    }

    return check_proceed("Test passed?");
}

int test_sleep() {
    uint32_t MSTickPerTick;
    TTick CurrentTicks, StartTicks, SleepTicks;

    int row = 10;
    int col = 10;

    WriteString_p2(row++, col, "Main Thread: Starting");
    RVCTickMS(&MSTickPerTick);
    SleepTicks = 5000 / MSTickPerTick;
    RVCTickCount(&StartTicks);
    RVCThreadSleep(SleepTicks);
    RVCTickCount(&CurrentTicks);
    WriteString_p2(row++, col, "Main Thread: Awake");

    if (CurrentTicks == 0) {
        WriteString_p2(row++, col, "Problem with RVCTickCount!");
    } else if (CurrentTicks < StartTicks + SleepTicks){
        WriteString_p2(row++, col, "Main Thread: Woke too soon!");
    } else {
        WriteString_p2(row++, col, "Main Thread: Slept for correct duration");
    }
    WriteString_p2(row++, col, "Main Thread: Exiting");

    return check_proceed("Test passed?");
}

#define EXPECTED_RETURN             5

TThreadID wait_LowThreadID, wait_HighThreadID;

TThreadReturn wait_HighPriorityThread(void *param){
    TThreadID CurrentThreadID;
    TThreadState ThreadState;

    int row = 10;
    int col = 38;

    WriteString_p2(row++, col, "High Thread:   ");
    RVCThreadID(&CurrentThreadID);
    if(CurrentThreadID != wait_HighThreadID){
        WriteString_p2(row++, col, "Invalid Thread ID");
        return 1;
    }
    WriteString_p2(row++, col, "Valid Thread ID");
    WriteString_p2(row++, col, "Checking Main: ");
    RVCThreadState(MainThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_WAITING != ThreadState){
        WriteString_p2(row++, col, "Invalid State");
        return 1;
    }
    WriteString_p2(row++, col, "Valid State");
    WriteString_p2(row++, col, "Checking This: ");
    RVCThreadState(wait_HighThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_RUNNING != ThreadState){
        WriteString_p2(row++, col, "Invalid State");
        return 1;
    }
    WriteString_p2(row++, col, "Valid State");
    return EXPECTED_RETURN;
}

TThreadReturn wait_LowPriorityThread(void *param){
    TThreadID CurrentThreadID;
    TThreadState ThreadState;
    TThreadReturn ReturnValue;

    int row = 10;
    int col = 20;

    WriteString_p2(row++, col, "Low Thread:");
    RVCThreadID(&CurrentThreadID);
    if(CurrentThreadID != wait_LowThreadID){
        WriteString_p2(row++, col, "Invalid Thread ID");
        return 1;
    }
    WriteString_p2(row++, col, "Valid Thread ID");
    WriteString_p2(row++, col, "Checking Main:");
    RVCThreadState(MainThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_WAITING != ThreadState){
        WriteString_p2(row++, col, "Invalid State");
        return 1;
    }
    WriteString_p2(row++, col, "Valid State");
    WriteString_p2(row++, col, "Checking This:");
    RVCThreadState(wait_LowThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_RUNNING != ThreadState){
        WriteString_p2(row++, col, "Invalid State");
        return 1;
    }
    WriteString_p2(row++, col, "Valid State");
    RVCThreadActivate(wait_HighThreadID);
    RVCThreadWait(wait_HighThreadID,&ReturnValue,RVCOS_TIMEOUT_INFINITE);
    WriteString_p2(row++, col, "Low Awake:");
    if(EXPECTED_RETURN != ReturnValue){
        WriteString_p2(row++, col, "Invalid Return");
        return 0;
    }
    WriteString_p2(row++, col, "Valid Return");
    RVCThreadTerminate(wait_LowThreadID,ReturnValue);

    return 0;
}

int do_test_wait() {
    TThreadID CurrentThreadID;
    TThreadState ThreadState;
    TThreadReturn ReturnValue;

    int row = 10;
    int col = 2;

    WriteString_p2(row++, col, "Main Thread:");
    RVCThreadID(&MainThreadID);
    RVCThreadState(MainThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_RUNNING != ThreadState){
        WriteString_p2(row++, col, "Invalid State");
        return 1;
    }
    WriteString_p2(row++, col, "Valid State");
    RVCThreadCreate(wait_HighPriorityThread,NULL,2048,RVCOS_THREAD_PRIORITY_HIGH,&wait_HighThreadID);
    WriteString_p2(row++, col, "High Created:");
    RVCThreadState(wait_HighThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_CREATED != ThreadState){
        WriteString_p2(row++, col, "Invalid State");
        return 1;
    }
    WriteString_p2(row++, col, "Valid State");
    RVCThreadCreate(wait_LowPriorityThread,NULL,2048,RVCOS_THREAD_PRIORITY_LOW,&wait_LowThreadID);
    WriteString_p2(row++, col, "Low Created:");
    RVCThreadState(wait_LowThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_CREATED != ThreadState){
        WriteString_p2(row++, col, "Invalid State");
        return 1;
    }
    WriteString_p2(row++, col, "Valid State");
    RVCThreadActivate(wait_LowThreadID);
    WriteString_p2(row++, col, "Low Activated: ");
    RVCThreadState(wait_LowThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_READY != ThreadState){
        WriteString_p2(row++, col, "Invalid State");
        return 1;
    }
    WriteString_p2(row++, col, "Valid State");
    RVCThreadWait(wait_LowThreadID,&ReturnValue,RVCOS_TIMEOUT_INFINITE);

    WriteString_p2(row++, col, "Checking Low:");
    if(EXPECTED_RETURN != ReturnValue){
        WriteString_p2(row++, col, "Invalid Return");
        return 1;
    }
    RVCThreadState(wait_LowThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_DEAD != ThreadState){
        WriteString_p2(row++, col, "Invalid State");
        return 1;
    }
    WriteString_p2(row++, col, "Valid Return and State");

    WriteString_p2(row++, col, "Checking High:");
    RVCThreadState(wait_HighThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_DEAD != ThreadState){
        WriteString_p2(row++, col, "Invalid State");
        return 1;
    }
    WriteString_p2(row++, col, "Valid State");
    RVCThreadID(&CurrentThreadID);
    if(CurrentThreadID != MainThreadID){
        WriteString_p2(row++, col, "Invalid Main Thread ID");
        return 1;
    }
    WriteString_p2(row++, col, "Main Exiting");

    return 0;
}

int test_wait() {

    if (do_test_wait() != 0) {
        WriteString_p2(1,1, "Error");
    }

    return check_proceed("Test passed?");
}
