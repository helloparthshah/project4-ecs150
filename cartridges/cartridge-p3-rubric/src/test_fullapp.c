#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include "RVCOS.h"
#include "RVCGraphics.h"
#include "tests.h"


#define FULLAPP_BG 1
#define FULLAPP_PALETTE 1

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

int test_fullapp_p2() {
    fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;

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
    return check_proceed("Test passed?");
}

typedef struct intersection_s intersection_t;
typedef struct lane_s lane_t;

struct lane_s {
    TThreadID               tid;
    int8_t                  is_col;
    int                     ticks_per_move;
    int                     pos;
    int                     vehicle_len;
    intersection_t         *intersection;
};

struct intersection_s {
    int                     x;
    int                     y;
    int                     width;
    int                     height;
    TMutexID                lock;
    lane_t                **lanes;
};

#define MAX_VHCL_LEN        10
#define WRITE_BUF_LEN       32 + MAX_VHCL_LEN * 16

int is_collides_intersection(int pos, int is_col, intersection_t *i) {
    return (is_col && pos >= i->y && pos < i->y + i->height) ||
        (!is_col && pos >= i->x && pos < i->x + i->width);
}

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

TThreadReturn lane_thread(void *param) {
    lane_t *l = (lane_t *)param;

    char write_buf[WRITE_BUF_LEN];
    int write_pos;

    int mutex_held = 0;
    int i;

    int vehicle_pos, vehicle_back_pos;

    vehicle_pos = 0;
    vehicle_back_pos = vehicle_pos - l->vehicle_len;

    while (vehicle_pos < l->vehicle_len + (l->is_col ? TXT_HGHT : TXT_WDTH)) {
        write_pos = 0;

        // draw vehicle
        int draw_start;
        if (l->is_col) {
            draw_start = MIN(TXT_HGHT - 1, vehicle_pos);
            write_pos += snprintf(write_buf, WRITE_BUF_LEN, "\x1B[%d;%dH", draw_start, l->pos);
        } else {
            draw_start = MIN(TXT_WDTH - 1, vehicle_pos);
            write_pos += snprintf(write_buf, WRITE_BUF_LEN, "\x1B[%d;%dH", l->pos, draw_start);
        }
        for (i = draw_start; i >= 0 && i > vehicle_back_pos; i -= 1) {
            write_pos += snprintf(write_buf + write_pos, WRITE_BUF_LEN - write_pos, "#");
            if (i > 0) {
                write_pos += snprintf(write_buf + write_pos, WRITE_BUF_LEN - write_pos,
                    l->is_col ? "\x1B[A\x1B[D" /* up, left */ : "\x1B[D\x1B[D" /* left, left */);
                if (i == vehicle_back_pos + 1)
                    write_pos += snprintf(write_buf + write_pos, WRITE_BUF_LEN - write_pos, " ");
            }
        }

        RVCWriteText(write_buf, write_pos);

        RVCThreadSleep(l->ticks_per_move);

        // move up
        vehicle_pos += 1;
        vehicle_back_pos += 1;

        if (!mutex_held && is_collides_intersection(vehicle_pos, l->is_col, l->intersection)) {
            RVCMutexAcquire(l->intersection->lock, RVCOS_TIMEOUT_INFINITE);
            mutex_held = 1;
        }
        if (mutex_held) {
            for (i = vehicle_pos; i >= vehicle_back_pos; i -= 1) {
                if (is_collides_intersection(i, l->is_col, l->intersection)) break;
                if (i == vehicle_back_pos) {
                    RVCMutexRelease(l->intersection->lock);
                    mutex_held = 0;
                }
            }
        }
    }
    if (l->is_col) {
        write_pos = snprintf(write_buf, WRITE_BUF_LEN, "\x1B[%d;%dH ", TXT_HGHT - 1, l->pos);
    } else {
        write_pos = snprintf(write_buf, WRITE_BUF_LEN, "\x1B[%d;%dH ", l->pos, TXT_WDTH - 1);
    }
    RVCWriteText(write_buf, write_pos);

    return RVCOS_STATUS_SUCCESS;
}

int test_fullapp_p3() {
    int i, x, y;
    int total_vehicles;
    lane_t *LanesMemoryBase;
    TMemoryPoolID LanesPoolID;
    TMemorySize AvailableSpace;
    TThreadState s;
    TThreadReturn trc;

    intersection_t the_crossing = {
        .x = 30,
        .y = 15,
        .width = 4,
        .height = 6
    };

    int n_lanes_max = the_crossing.width + the_crossing.height;

    RVCMutexCreate(&the_crossing.lock);

    if (RVCMemoryAllocate(sizeof(lane_t *) * n_lanes_max, (void **)&the_crossing.lanes)
        != RVCOS_STATUS_SUCCESS)
    {
        check_proceed("Alloc Error");
        RVCWriteText_LITERAL("\x1B[H");
    }
    memset(the_crossing.lanes, 0, sizeof(lane_t *) * n_lanes_max);

    if (RVCMemoryAllocate(MAX(64, sizeof(lane_t)) * n_lanes_max * 2, (void **)&LanesMemoryBase)
        != RVCOS_STATUS_SUCCESS)
    {
        check_proceed("Alloc Error");
        RVCWriteText_LITERAL("\x1B[H");
    }
    memset(LanesMemoryBase, 0, MAX(64, sizeof(lane_t)) * n_lanes_max);

    if (RVCMemoryPoolCreate(LanesMemoryBase, 64 * n_lanes_max, &LanesPoolID)
        != RVCOS_STATUS_SUCCESS)
    {
        check_proceed("Pool Error");
        RVCWriteText_LITERAL("\x1B[H");
    }

    for (i = 0; i < TXT_HGHT - 1; ++i) {
        if (i == 0 || (i >= TXT_HGHT / 4 && i < 3 * TXT_HGHT / 4))
            RVCWriteText_LITERAL("\n");
        else if (i + 1 == TXT_HGHT / 4 || i == 3 * TXT_HGHT / 4)
            RVCWriteText_LITERAL(
                " +++++++++++++++                                +++++++++++++++\n");
        else
            RVCWriteText_LITERAL(
                "               +                                +\n");
    }

    total_vehicles = 100;

    while (total_vehicles > 0) {
        for (i = 0; i < n_lanes_max; ++i) {
            if (the_crossing.lanes[i] == NULL || (
                RVCThreadState(the_crossing.lanes[i]->tid, &s) == RVCOS_STATUS_SUCCESS && s == RVCOS_THREAD_STATE_DEAD))
            {
                if (the_crossing.lanes[i] != NULL) {
                    RVCThreadDelete(the_crossing.lanes[i]->tid);
                    if (RVCMemoryPoolDeallocate(LanesPoolID, the_crossing.lanes[i]) != RVCOS_STATUS_SUCCESS)
                        check_proceed("Dealloc Error");
                }
                if (RVCMemoryPoolAllocate(LanesPoolID, MAX(64, sizeof(lane_t)),
                    (void **)&the_crossing.lanes[i]) != RVCOS_STATUS_SUCCESS)
                    check_proceed("Alloc Error");

                memset(the_crossing.lanes[i], 0, sizeof(lane_t));

                if (i < the_crossing.height) {
                    the_crossing.lanes[i]->is_col = 0;
                    the_crossing.lanes[i]->pos = the_crossing.y + i;
                } else {
                    the_crossing.lanes[i]->is_col = 1;
                    the_crossing.lanes[i]->pos = the_crossing.x + i - the_crossing.height;
                }

                the_crossing.lanes[i]->ticks_per_move = 200;
                the_crossing.lanes[i]->vehicle_len =  2 + (random() % 4);
                the_crossing.lanes[i]->intersection = &the_crossing;

                RVCThreadCreate(lane_thread, (void *)the_crossing.lanes[i], 2048,
                    RVCOS_THREAD_PRIORITY_HIGH, &the_crossing.lanes[i]->tid);
                RVCThreadActivate(the_crossing.lanes[i]->tid);

                --total_vehicles;
            }
        }
        RVCThreadSleep(100);
    }

    for (i = 0; i < n_lanes_max; ++i) {
        if (the_crossing.lanes[i]) {
            RVCThreadWait(the_crossing.lanes[i]->tid, &trc, RVCOS_TIMEOUT_INFINITE);
        }
    }

    RVCMemoryDeallocate(the_crossing.lanes);
    RVCMemoryPoolDelete(LanesPoolID);
    RVCMemoryDeallocate(LanesMemoryBase);

    return check_proceed("Test passed?");
}
