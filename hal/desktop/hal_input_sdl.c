/**
 * @file hal_input_sdl.c
 * Desktop input HAL implementation.
 *
 * LVGL's SDL driver automatically creates mouse and keyboard input devices
 * when the SDL window is created. This file is a no-op placeholder that
 * satisfies the HAL interface.
 */

#include "hal_input.h"

void hal_input_init(lv_display_t *disp)
{
    /* LVGL's SDL window driver automatically registers mouse + keyboard.
     * Nothing additional to do on desktop. */
    (void)disp;
}
