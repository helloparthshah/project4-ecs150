#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include "RVCOS.h"
#include "RVCGraphics.h"
#include "tests.h"

int test_writetext() {
    for (int i = 0; i < TXT_HGHT; ++i) {
        RVCWriteText_LITERAL(
            "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
    }
    RVCWriteText_LITERAL("\n\n\n");
    RVCWriteText_LITERAL("      World!");
    RVCWriteText_LITERAL("\rHello ");
    RVCWriteText_LITERAL("\n");
    RVCWriteText_LITERAL("Hello World!\n");

    return check_proceed("Test passed?");
}

int test_writetext_vt100() {
    int i, rc, len;
    RVCWriteText_LITERAL("\x1B[2J"); // clear screen

    char buf[32];

    for (i = 0; i < TXT_HGHT; ++i) {
        RVCWriteText_LITERAL(
            "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
    }
    for (i = 0; i < 3; ++i)
        RVCWriteText_LITERAL("\n"); // down

    RVCWriteText_LITERAL("      World!");
    for (i = 0; i < 12; ++i)
        RVCWriteText_LITERAL("\x1B[D"); // left

    RVCWriteText_LITERAL("Hello");
    RVCWriteText_LITERAL("\n"); // down
    RVCWriteText_LITERAL("Hello World!\n");

    len = snprintf(buf, 32, "\x1B[%d;%dH", TXT_HGHT / 2, TXT_WDTH / 3);
    RVCWriteText(buf, len);

    RVCWriteText_LITERAL("  Arbitrary cursor position test  ");

    int rel_pos_y = 4;
    int rel_pos_x = 10;
    RVCWriteText_LITERAL("\x1B[H"); // reset cursor to top left
    for (i = 0; i < rel_pos_y; ++i)
        RVCWriteText_LITERAL("\x1B[B"); // down
    for (i = 0; i < rel_pos_x; ++i)
        RVCWriteText_LITERAL("\x1B[C"); // right
    RVCWriteText_LITERAL("    Home-relative cursor test   ");

    rc = check_proceed("Test passed?");
    return rc;
}

#define MTIME_LOW       (*((volatile uint32_t *)0x40000008))
#define MTIME_HIGH      (*((volatile uint32_t *)0x4000000C))
#define MTIME_LOW       (*((volatile uint32_t *)0x40000008))
#define MTIME_HIGH      (*((volatile uint32_t *)0x4000000C))
#define MTIMECMP_LOW    (*((volatile uint32_t *)0x40000010))
#define MTIMECMP_HIGH   (*((volatile uint32_t *)0x40000014))

int test_writetext_blocking() {
    int n_writes = 10;
    uint64_t times[n_writes + 1];
    int i;

    times[0] = (((uint64_t)MTIME_HIGH)<<32) | MTIME_LOW;

    for (i = 1; i <= n_writes; ++i) {
        times[i] = (((uint64_t)MTIME_HIGH)<<32) | MTIME_LOW;
        WriteString("\n  write %2d after %5lu ticks\n\n", i, times[i] - times[i - 1]);
    }

    return check_proceed("Test passed?");
}


int test_controller() {
    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;
    char padding[4]; // leave some extra room in case the wrong SControllerStatus size is used
    SControllerStatus ControllerStatus;
    SControllerStatus RealControllerStatus;

    int left_seen = 0;
    int right_seen = 0;
    int down_seen = 0;

    int offset = (sizeof("|   | left ") - 1) / 2;

    strcpy(&graphics->text_data[TXT_HGHT / 2][TXT_WDTH / 4 - offset],
        "|   | left ");
    strcpy(&graphics->text_data[TXT_HGHT / 2][3 * TXT_WDTH / 4 - offset],
        "|   | right");
    strcpy(&graphics->text_data[3 * TXT_HGHT / 4][TXT_WDTH / 2 - offset],
        "|   | down ");

    int rc = 0;
    while(!left_seen || !right_seen || !down_seen){
        uint32_t Delay = 100000;
        while(Delay){
            Delay--;
        }
        RVCReadController(&ControllerStatus);
        RealControllerStatus = CONTROLLER;
        if(ControllerStatus.DRight){
            strcpy(&graphics->text_data[TXT_HGHT / 2][3 * TXT_WDTH / 4 - offset],
                "| X |");
            right_seen = 1;
        }
        if(ControllerStatus.DLeft){
            strcpy(&graphics->text_data[TXT_HGHT / 2][TXT_WDTH / 4 - offset],
                "| X |");
            left_seen = 1;
        }
        if(ControllerStatus.DDown){
            strcpy(&graphics->text_data[3 * TXT_HGHT / 4][TXT_WDTH / 2 - offset],
                "| X |");
            down_seen = 1;
        }

        if (RealControllerStatus.DButton4) {
            rc = 1;
            break;
        }
    }

    return rc;
}

int test_timer() {
    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;

    uint64_t time, time_cmp;

    int row = 12;
    int col1 = 2;
    int col2 = 17;
    int col3 = 32;

    row += 2;
    strcpy(&graphics->text_data[row][col1],
        "mtime_cmp should always be a bit larger than mtime");
    strcpy(&graphics->text_data[row+1][col1],
        "  if interrupts are being handled -");

    row += 3;
    strcpy(&graphics->text_data[row][col1], "MTIME");
    strcpy(&graphics->text_data[row][col2], "MTIME_CMP");
    strcpy(&graphics->text_data[row][col3], "diff:");

    int n_samples = 4;
    for (int i = 0; i < n_samples; ++i) {
        time = (((uint64_t)MTIME_HIGH)<<32) | MTIME_LOW;
        time_cmp = (((uint64_t)MTIMECMP_HIGH)<<32) | MTIMECMP_LOW;

        row += 2;

        sprintf(&graphics->text_data[row][col1], "%12lu", time);
        sprintf(&graphics->text_data[row][col2], "%12lu", time_cmp);
        sprintf(&graphics->text_data[row][col3], "%12lu", time_cmp - time);

        busy_wait(1000000);
    }

    int rc = check_proceed("Test passed?");
    return rc;
}
