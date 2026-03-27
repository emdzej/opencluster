/**
 * @file hal_input.h
 * OpenCluster input HAL interface.
 *
 * Abstracts local input device initialization (touch, mouse, buttons).
 * The HAL registers LVGL input devices appropriate for the platform.
 */

#ifndef HAL_INPUT_H
#define HAL_INPUT_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize local input devices and register them with LVGL.
 *
 * @param disp  The LVGL display to attach input devices to.
 */
void hal_input_init(lv_display_t *disp);

#ifdef __cplusplus
}
#endif

#endif /* HAL_INPUT_H */
