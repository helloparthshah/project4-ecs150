#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include "RVCOS.h"
#include "RVCGraphics.h"
#include "tests.h"

void busy_wait(int cycles) {
    int i = 0;
    while (i < cycles) {
        ++i;
    }
}

int check_proceed(char *query) {
    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;
    SControllerStatus RealControllerStatus;
    int rc = 0;

    int row = 6;
    int col = 5;

    sprintf(&graphics->text_data[row  ][col], "  %s   ", query);
    sprintf(&graphics->text_data[row+1][col], "  J ->    accept   ");
    sprintf(&graphics->text_data[row+2][col], "  K ->    decline  ");

    int b3_released = 0;
    int b4_released = 0;

    while(1){
        busy_wait(100000);

        RealControllerStatus = CONTROLLER;

        if (RealControllerStatus.DButton3) {
            if (b3_released)
                break;
        } else {
            // wait for button to be released first before counting another press
            b3_released = 1;
        }
        if (RealControllerStatus.DButton4) {
            if (b4_released) {
                rc = 1;
                break;
            }
        } else {
            b4_released = 1;
        }
    }
    memset(&graphics->text_data[row  ][col], ' ', sizeof("  J ->    accept   "));
    memset(&graphics->text_data[row+1][col], ' ', sizeof("  J ->    accept   "));
    memset(&graphics->text_data[row+2][col], ' ', sizeof("  J ->    accept   "));
    return rc;
}


int main() {
    test *current_test;

    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;
    // a cartridge should not directly control the hardware in this way,
    // it should always use system calls instead.

    // only done here because the cartridge is trying to evaluate whether the firmware
    // functions correctly, and can't assume that for example RVCWriteText works
    // when testing features other than RVCWriteText.

    test all_tests[] = {
        { "Initialize main thread, return control to console",
            test_rvcinitialize },
        { "Able to read input from console controller",
            test_controller },
        { "Support text output to console",
            test_writetext },
        { "Can setup and respond to timer interrupts",
            test_timer },
        { "Able to switch contexts",
            test_contextswitch },
        { "Supports querying of status/thread ID of threads",
            test_status },
        { "Threads can sleep",
            test_sleep },
        { "Threads can wait on other threads",
            test_wait },
        { "Full applications execute correctly",
            test_fullapp },
        { {"\0"} }
    };

    int line = 10;
    int name_col = 8;
    int result_col = 2;

    for (current_test = all_tests; current_test->name[0]; ++current_test, line += 2) {
        graphics->mode_controls.mode = MODE_TEXT;
        strcpy(&graphics->text_data[line][name_col], current_test->name);

        if (!current_test->tfn || check_proceed("Run or skip?")) {
            strcpy(&graphics->text_data[line][result_col], "----");
            continue;
        }

        current_test->rc = current_test->tfn();
        strcpy(&graphics->text_data[line][result_col],
            current_test->rc ? "----" : "PASS");
    }

    while (1) {
        /* wait */
    }

    return 0;
}
