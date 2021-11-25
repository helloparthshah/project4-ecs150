#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include "RVCOS.h"
#include "RVCGraphics.h"
#include "tests.h"


#define RVCWriteText_LITERAL(s) \
            (RVCWriteText(s, sizeof(s)))


// avoided using global variables in most tests so that partial credit can be assigned
// when a feature works, but setting the global pointer doesn't.

int test_jump_cartridge() {
    // already passed if it made it here ..

    return 0;
}

int test_writetext() {
    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;
    uint8_t screen_backup[TXT_HGHT][TXT_WDTH];
    memcpy(screen_backup, graphics->text_data, TXT_SIZE);

    int i;
    RVCWriteText_LITERAL("\x1B[2J"); // clear screen

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


    char buf[32];
    int len = snprintf(buf, 32, "\x1B[%d;%dH", TXT_HGHT / 2, TXT_WDTH / 2);
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


    int rc = check_proceed("Test passed?");
    memcpy(graphics->text_data, screen_backup, TXT_SIZE);
    return rc;
}


int test_controller() {
    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;
    char padding[4]; // leave some extra room in case the wrong SControllerStatus size is used
    SControllerStatus ControllerStatus;
    SControllerStatus RealControllerStatus;
    uint8_t screen_backup[TXT_HGHT][TXT_WDTH];
    memcpy(screen_backup, graphics->text_data, TXT_SIZE);
    memset(graphics->text_data, ' ', TXT_SIZE);

    int left_seen = 0;
    int right_seen = 0;
    int down_seen = 0;

    strcpy(&graphics->text_data[TXT_HGHT / 2][TXT_WDTH / 4],
        "|   | left");
    strcpy(&graphics->text_data[TXT_HGHT / 2][3 * TXT_WDTH / 4],
        "|   | right");
    strcpy(&graphics->text_data[3 * TXT_HGHT / 4][TXT_WDTH / 2],
        "|   | down");

    int rc = 0;
    while(!left_seen || !right_seen || !down_seen){
        uint32_t Delay = 100000;
        while(Delay){
            Delay--;
        }
        RVCReadController(&ControllerStatus);
        RealControllerStatus = CONTROLLER;
        if(ControllerStatus.DRight){
            strcpy(&graphics->text_data[TXT_HGHT / 2][3 * TXT_WDTH / 4],
                "| X |");
            right_seen = 1;
        }
        if(ControllerStatus.DLeft){
            strcpy(&graphics->text_data[TXT_HGHT / 2][TXT_WDTH / 4],
                "| X |");
            left_seen = 1;
        }
        if(ControllerStatus.DDown){
            strcpy(&graphics->text_data[3 * TXT_HGHT / 4][TXT_WDTH / 2],
                "| X |");
            down_seen = 1;
        }

        if (RealControllerStatus.DButton4) {
            rc = 1;
            break;
        }
    }

    memcpy(graphics->text_data, screen_backup, TXT_SIZE);
    return rc;
}

#define MTIME_LOW       (*((volatile uint32_t *)0x40000008))
#define MTIME_HIGH      (*((volatile uint32_t *)0x4000000C))
#define MTIMECMP_LOW    (*((volatile uint32_t *)0x40000010))
#define MTIMECMP_HIGH   (*((volatile uint32_t *)0x40000014))
int test_timer() {
    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;
    uint8_t screen_backup[TXT_HGHT][TXT_WDTH];
    memcpy(screen_backup, graphics->text_data, TXT_SIZE);
    memset(graphics->text_data, ' ', TXT_SIZE);

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
    memcpy(graphics->text_data, screen_backup, TXT_SIZE);
    return rc;
}
