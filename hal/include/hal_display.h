/**
 * @file hal_display.h
 * OpenCluster display HAL interface.
 *
 * Abstracts display initialization so the same application code works
 * on desktop (SDL2) and ESP32 (SPI/RGB TFT).
 */

#ifndef HAL_DISPLAY_H
#define HAL_DISPLAY_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the display hardware and register an LVGL display driver.
 *
 * @param width   Horizontal resolution in pixels.
 * @param height  Vertical resolution in pixels.
 * @return        Pointer to the created LVGL display, or NULL on failure.
 */
lv_display_t *hal_display_init(int width, int height);

/**
 * Set the window title (desktop only, no-op on embedded).
 */
void hal_display_set_title(lv_display_t *disp, const char *title);

#ifdef __cplusplus
}
#endif

#endif /* HAL_DISPLAY_H */
