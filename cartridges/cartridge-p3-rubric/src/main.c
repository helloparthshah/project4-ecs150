#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include "RVCOS.h"
#include "RVCGraphics.h"
#include "tests.h"

int WriteString(const char *format, ...) {
    // call RVCWriteText with at most one line's worth of data
    //      with support for format strings
    char buf[TXT_WDTH];
    int len;

    va_list args;
    va_start(args, format);

    len = vsnprintf(buf, TXT_WDTH, format, args);

    va_end(args);

    if (len > 0)
        RVCWriteText(buf, len);
}

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

    int qry_wdth = 16;
    int col = TXT_WDTH - qry_wdth;
    int row = TXT_HGHT - 4;

    memset  (&graphics->text_data[row  ][col], '_', 16);
    snprintf(&graphics->text_data[row+1][col], 16, "| %s", query);
    snprintf(&graphics->text_data[row+2][col], 16, "|J ->  accept");
    snprintf(&graphics->text_data[row+3][col], 16, "|K ->  decline");

    int b3_released = 0;
    int b4_released = 0;

    while(1){
        busy_wait(10000);

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
    while (row < TXT_HGHT)
        memset(&graphics->text_data[row++][col], ' ', qry_wdth);
    return rc;
}


int main() {
    test *current_test;
    uint8_t screen_backup[TXT_HGHT][TXT_WDTH];

    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;
    // a cartridge should not directly control the hardware in this way,
    // it should always use system calls instead.

    // only done here because the cartridge is trying to evaluate whether the firmware
    // functions correctly, and can't assume that for example RVCWriteText works
    // when testing features other than RVCWriteText.

    test all_tests[] = {
        { "Can run apps from project 2 - controller",
            test_controller },
        { "Can run apps from project 2 - writetext",
            test_writetext },
        { "Can run apps from project 2 - timer interrupts",
            test_timer },
        { "Can run apps from project 2 - switch contexts",
            test_contextswitch },
        { "Can run apps from project 2 - query threads",
            test_status },
        { "Can run apps from project 2 - threads can sleep",
            test_sleep },
        { "Can run apps from project 2 - Threads can wait",
            test_wait },
        { "Can run apps from project 2 - Full application",
            test_fullapp_p2 },
        { "Support VT100 codes in WriteText",
            test_writetext_vt100 },
        { "WriteText blocks until refresh",
            test_writetext_blocking },
        { "Mutex - errors",
            test_mutex_error },
        { "Mutex - functionality",
            test_mutex_functionality },
        { "Memory - errors",
            test_memory_error },
        { "Memory - functionality",
            test_memory_functionality },
        { "Full applications execute correctly",
            test_fullapp_p3 },
        { {"\0"} }
    };

    int line = 2;
    int name_col = 8;
    int result_col = 2;

    for (current_test = all_tests; current_test->name[0]; ++current_test, line += 2) {
        graphics->mode_controls.mode = MODE_TEXT;
        strcpy(&graphics->text_data[line][name_col], current_test->name);

        if (!current_test->tfn || check_proceed("Run or skip?")) {
            strcpy(&graphics->text_data[line][result_col], "----");
            continue;
        }

        // backup and clear screen
        memcpy(screen_backup, graphics->text_data, TXT_SIZE);
        memset(graphics->text_data, ' ', TXT_SIZE);

        current_test->rc = current_test->tfn();

        // reset cursor position
        RVCWriteText_LITERAL("\x1B[H");

        // restore screen
        memcpy(graphics->text_data, screen_backup, TXT_SIZE);
        strcpy(&graphics->text_data[line][result_col],
            current_test->rc ? "----" : "PASS");
    }

    while (1) {
        /* wait */
    }

    return 0;
}
