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

void WriteString(int row, int col, char *str) {
    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;
    strcpy(&graphics->text_data[row][col], str);
}

TThreadReturn contextswitch_thread(void *param) {
    int row = 10;
    int col = 25;

    WriteString(row++, col, "High Thread");

    return 0;
}

int test_contextswitch() {
    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;
    uint8_t screen_backup[TXT_HGHT][TXT_WDTH];
    memcpy(screen_backup, graphics->text_data, TXT_SIZE);
    memset(graphics->text_data, ' ', TXT_SIZE);
    TThreadID HighThreadID;

    int row = 10;
    int col = 4;

    WriteString(row++, col, "Main Thread");
    RVCThreadCreate(contextswitch_thread,NULL,2048,RVCOS_THREAD_PRIORITY_HIGH,&HighThreadID);
    WriteString(row++, col, "High Created");
    RVCThreadActivate(HighThreadID);
    WriteString(row++, col, "Main Exiting");

    int rc = check_proceed("Test passed?");
    memcpy(graphics->text_data, screen_backup, TXT_SIZE);
    return rc;
}

TThreadID MainThreadID, query_HighThreadID;

TThreadReturn query_HighPriorityThread(void *param){
    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;

    TThreadID CurrentThreadID;
    TThreadState ThreadState;

    int row = 12;
    int col = 26;

    WriteString(row++, col, "High Thread:");
    RVCThreadID(&CurrentThreadID);
    if(CurrentThreadID != query_HighThreadID){
        WriteString(row++, col, "Invalid Thread ID");
        return 1;
    }
    WriteString(row++, col, "Valid Thread ID");
    WriteString(row++, col, "Checking Main: ");
    RVCThreadState(MainThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_READY != ThreadState){
        WriteString(row++, col, "Invalid State");
        return 1;
    }
    WriteString(row++, col, "Valid State");
    WriteString(row++, col, "Checking This: ");
    RVCThreadState(query_HighThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_RUNNING != ThreadState){
        WriteString(row++, col, "Invalid State");
        return 1;
    }
    WriteString(row++, col, "Valid State");
    return 0;
}


int do_test_status() {
    TThreadID CurrentThreadID;
    TThreadState ThreadState;

    int row = 12;
    int col = 6;

    WriteString(row++, col, "Main Thread:");
    RVCThreadID(&MainThreadID);
    RVCThreadState(MainThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_RUNNING != ThreadState){
        WriteString(row++, col, "Invalid State");
        return 1;
    }
    WriteString(row++, col, "Valid State");
    RVCThreadCreate(query_HighPriorityThread,NULL,2048,RVCOS_THREAD_PRIORITY_HIGH,&query_HighThreadID);
    WriteString(row++, col, "High Created:");
    RVCThreadState(query_HighThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_CREATED != ThreadState){
        WriteString(row++, col, "Invalid State");
        return 1;
    }
    WriteString(row++, col, "Valid State");
    RVCThreadActivate(query_HighThreadID);
    WriteString(row++, col, "Checking High:");
    RVCThreadState(query_HighThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_DEAD != ThreadState){
        WriteString(row++, col, "Invalid State");
        return 1;
    }
    WriteString(row++, col, "Valid State");
    RVCThreadID(&CurrentThreadID);
    if(CurrentThreadID != MainThreadID){
        WriteString(row++, col, "Invalid Main Thread ID");
        return 1;
    }
    WriteString(row++, col, "Main Exiting");

    return 0;
}


int test_status() {
    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;
    uint8_t screen_backup[TXT_HGHT][TXT_WDTH];
    memcpy(screen_backup, graphics->text_data, TXT_SIZE);
    memset(graphics->text_data, ' ', TXT_SIZE);

    if (do_test_status() != 0) {
        WriteString(1,1, "Error");
    }

    int rc = check_proceed("Test passed?");
    memcpy(graphics->text_data, screen_backup, TXT_SIZE);
    return rc;
}

int test_sleep() {
    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;
    uint8_t screen_backup[TXT_HGHT][TXT_WDTH];
    memcpy(screen_backup, graphics->text_data, TXT_SIZE);
    memset(graphics->text_data, ' ', TXT_SIZE);

    uint32_t MSTickPerTick;
    TTick CurrentTicks, StartTicks, SleepTicks;

    int row = 10;
    int col = 10;

    WriteString(row++, col, "Main Thread: Starting");
    RVCTickMS(&MSTickPerTick);
    SleepTicks = 5000 / MSTickPerTick;
    RVCTickCount(&StartTicks);
    RVCThreadSleep(SleepTicks);
    RVCTickCount(&CurrentTicks);
    WriteString(row++, col, "Main Thread: Awake");

    if (CurrentTicks == 0) {
        WriteString(row++, col, "Problem with RVCTickCount!");
    } else if (CurrentTicks < StartTicks + SleepTicks){
        WriteString(row++, col, "Main Thread: Woke too soon!");
    } else {
        WriteString(row++, col, "Main Thread: Slept for correct duration");
    }
    WriteString(row++, col, "Main Thread: Exiting");

    int rc = check_proceed("Test passed?");
    memcpy(graphics->text_data, screen_backup, TXT_SIZE);
    return rc;
}

#define EXPECTED_RETURN             5

TThreadID wait_LowThreadID, wait_HighThreadID;

TThreadReturn wait_HighPriorityThread(void *param){
    TThreadID CurrentThreadID;
    TThreadState ThreadState;

    int row = 10;
    int col = 38;

    WriteString(row++, col, "High Thread:   ");
    RVCThreadID(&CurrentThreadID);
    if(CurrentThreadID != wait_HighThreadID){
        WriteString(row++, col, "Invalid Thread ID");
        return 1;
    }
    WriteString(row++, col, "Valid Thread ID");
    WriteString(row++, col, "Checking Main: ");
    RVCThreadState(MainThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_WAITING != ThreadState){
        WriteString(row++, col, "Invalid State");
        return 1;
    }
    WriteString(row++, col, "Valid State");
    WriteString(row++, col, "Checking This: ");
    RVCThreadState(wait_HighThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_RUNNING != ThreadState){
        WriteString(row++, col, "Invalid State");
        return 1;
    }
    WriteString(row++, col, "Valid State");
    return EXPECTED_RETURN;
}

TThreadReturn wait_LowPriorityThread(void *param){
    TThreadID CurrentThreadID;
    TThreadState ThreadState;
    TThreadReturn ReturnValue;

    int row = 10;
    int col = 20;

    WriteString(row++, col, "Low Thread:");
    RVCThreadID(&CurrentThreadID);
    if(CurrentThreadID != wait_LowThreadID){
        WriteString(row++, col, "Invalid Thread ID");
        return 1;
    }
    WriteString(row++, col, "Valid Thread ID");
    WriteString(row++, col, "Checking Main:");
    RVCThreadState(MainThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_WAITING != ThreadState){
        WriteString(row++, col, "Invalid State");
        return 1;
    }
    WriteString(row++, col, "Valid State");
    WriteString(row++, col, "Checking This:");
    RVCThreadState(wait_LowThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_RUNNING != ThreadState){
        WriteString(row++, col, "Invalid State");
        return 1;
    }
    WriteString(row++, col, "Valid State");
    RVCThreadActivate(wait_HighThreadID);
    RVCThreadWait(wait_HighThreadID,&ReturnValue,RVCOS_TIMEOUT_INFINITE);
    WriteString(row++, col, "Low Awake:");
    if(EXPECTED_RETURN != ReturnValue){
        WriteString(row++, col, "Invalid Return");
        return 0;
    }
    WriteString(row++, col, "Valid Return");
    RVCThreadTerminate(wait_LowThreadID,ReturnValue);

    return 0;
}

int do_test_wait() {
    TThreadID CurrentThreadID;
    TThreadState ThreadState;
    TThreadReturn ReturnValue;

    int row = 10;
    int col = 2;

    WriteString(row++, col, "Main Thread:");
    RVCThreadID(&MainThreadID);
    RVCThreadState(MainThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_RUNNING != ThreadState){
        WriteString(row++, col, "Invalid State");
        return 1;
    }
    WriteString(row++, col, "Valid State");
    RVCThreadCreate(wait_HighPriorityThread,NULL,2048,RVCOS_THREAD_PRIORITY_HIGH,&wait_HighThreadID);
    WriteString(row++, col, "High Created:");
    RVCThreadState(wait_HighThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_CREATED != ThreadState){
        WriteString(row++, col, "Invalid State");
        return 1;
    }
    WriteString(row++, col, "Valid State");
    RVCThreadCreate(wait_LowPriorityThread,NULL,2048,RVCOS_THREAD_PRIORITY_LOW,&wait_LowThreadID);
    WriteString(row++, col, "Low Created:");
    RVCThreadState(wait_LowThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_CREATED != ThreadState){
        WriteString(row++, col, "Invalid State");
        return 1;
    }
    WriteString(row++, col, "Valid State");
    RVCThreadActivate(wait_LowThreadID);
    WriteString(row++, col, "Low Activated: ");
    RVCThreadState(wait_LowThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_READY != ThreadState){
        WriteString(row++, col, "Invalid State");
        return 1;
    }
    WriteString(row++, col, "Valid State");
    RVCThreadWait(wait_LowThreadID,&ReturnValue,RVCOS_TIMEOUT_INFINITE);

    WriteString(row++, col, "Checking Low:");
    if(EXPECTED_RETURN != ReturnValue){
        WriteString(row++, col, "Invalid Return");
        return 1;
    }
    RVCThreadState(wait_LowThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_DEAD != ThreadState){
        WriteString(row++, col, "Invalid State");
        return 1;
    }
    WriteString(row++, col, "Valid Return and State");

    WriteString(row++, col, "Checking High:");
    RVCThreadState(wait_HighThreadID,&ThreadState);
    if(RVCOS_THREAD_STATE_DEAD != ThreadState){
        WriteString(row++, col, "Invalid State");
        return 1;
    }
    WriteString(row++, col, "Valid State");
    RVCThreadID(&CurrentThreadID);
    if(CurrentThreadID != MainThreadID){
        WriteString(row++, col, "Invalid Main Thread ID");
        return 1;
    }
    WriteString(row++, col, "Main Exiting");

    return 0;
}

int test_wait() {
    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;
    uint8_t screen_backup[TXT_HGHT][TXT_WDTH];
    memcpy(screen_backup, graphics->text_data, TXT_SIZE);
    memset(graphics->text_data, ' ', TXT_SIZE);

    if (do_test_wait() != 0) {
        WriteString(1,1, "Error");
    }

    int rc = check_proceed("Test passed?");
    memcpy(graphics->text_data, screen_backup, TXT_SIZE);
    return rc;
}

typedef struct {
    uint32_t        color;
    int             draw_delay;
    TThreadPriority prio;

    int             start_row;
    int             end_row;

    TThreadID       id;
    TThreadReturn   rc;
    TThreadState    s;
    TTick           terminated;
} draw_bar_params;

#define FULLAPP_BG 1
#define FULLAPP_PALETTE 1

TThreadReturn drawbar_thread(void *param) {
    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;
    draw_bar_params *params = (draw_bar_params *)param;

    for (int i = 0; i < BG_WDTH; ++i) {
        for (int j = params->start_row; j < params->end_row; ++j) {
            graphics->background_data[FULLAPP_BG][j * BG_WDTH + i] = params->color;
        }
        busy_wait(params->draw_delay);
    }

    RVCTickCount(&(params->terminated));

    return 0;
}

int test_fullapp() {
    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;
    uint8_t screen_backup[TXT_HGHT][TXT_WDTH];
    memcpy(screen_backup, graphics->text_data, TXT_SIZE);
    memset(graphics->text_data, ' ', TXT_SIZE);

    draw_bar_params thread_params[] = {
        {.color = 0xFF0000FF, .draw_delay = 20000, .prio = RVCOS_THREAD_PRIORITY_HIGH},
        {.color = 0xFF00FF00, .draw_delay = 25000, .prio = RVCOS_THREAD_PRIORITY_NORMAL},
        {.color = 0xFF00FFFF, .draw_delay = 10000, .prio = RVCOS_THREAD_PRIORITY_NORMAL},
        {.color = 0xFFFF00FF, .draw_delay = 10000, .prio = RVCOS_THREAD_PRIORITY_LOW},
        {.color = 0xFFFF0000, .draw_delay =  5000, .prio = RVCOS_THREAD_PRIORITY_LOW},
        {.color = 0xFFFFFF00, .draw_delay = 10000, .prio = RVCOS_THREAD_PRIORITY_LOW}
    };

    int n_thread = sizeof(thread_params) / sizeof(draw_bar_params);

    for (int i = 0; i < n_thread; ++i) {
        thread_params[i].start_row = i * BG_HGHT / n_thread;
        thread_params[i].end_row = (i + 1) * BG_HGHT / n_thread;
        graphics->background_palettes[FULLAPP_PALETTE][i+1] = thread_params[i].color;
        thread_params[i].color = i+1;
        RVCThreadCreate(drawbar_thread, &thread_params[i], 1024 * 8,
            thread_params[i].prio,
            &thread_params[i].id);
    }

    int row = 10;
    int col1 = 2;
    int col2 = 30;

    for (int i = 0; i < n_thread; ++i) {
        sprintf(&graphics->text_data[row + i][col1], "Thread %d: id %2d, priority %d",
            i, thread_params[i].id, thread_params[i].prio);
    }

    check_proceed("Threads created:");

    TTick T0;
    RVCTickCount(&T0);

    graphics->mode_controls.mode = MODE_GRAPHICS;
    graphics->background_controls[FULLAPP_BG].X_512 = BG_WDTH;
    graphics->background_controls[FULLAPP_BG].Y_288 = BG_HGHT;
    graphics->background_controls[FULLAPP_BG].palette = FULLAPP_PALETTE;

    for (int i = 0; i < n_thread; ++i) {
        RVCThreadActivate(thread_params[i].id);
    }

    for (int i = 0; i < n_thread; ++i) {
        RVCThreadWait(thread_params[i].id, &thread_params[i].rc,
            RVCOS_TIMEOUT_INFINITE);
    }

    for (int i = 0; i < n_thread; ++i) {
        RVCThreadState(thread_params[i].id, &thread_params[i].s);
        sprintf(&graphics->text_data[row + i][col2], "final state %d",
            thread_params[i].s);
        sprintf(&graphics->text_data[row + i][col2 + 15], "at %7d",
            (int)(thread_params[i].terminated - T0));
    }

    graphics->mode_controls.mode = MODE_TEXT;
    int rc = check_proceed("Test passed?");
    memcpy(graphics->text_data, screen_backup, TXT_SIZE);
    return rc;
}
