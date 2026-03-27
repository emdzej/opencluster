/**
 * @file hal_display_sdl.c
 * Desktop display HAL implementation using LVGL's built-in SDL2 driver.
 */

#include "hal_display.h"
#include "lvgl.h"
#include "src/drivers/sdl/lv_sdl_window.h"

lv_display_t *hal_display_init(int width, int height)
{
    lv_display_t *disp = lv_sdl_window_create(width, height);
    if (!disp) {
        LV_LOG_ERROR("Failed to create SDL window");
        return NULL;
    }
    return disp;
}

void hal_display_set_title(lv_display_t *disp, const char *title)
{
    if (disp && title) {
        lv_sdl_window_set_title(disp, title);
    }
}
