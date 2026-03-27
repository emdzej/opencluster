/**
 * @file hal_platform.h
 * OpenCluster platform HAL interface.
 *
 * Abstracts OS primitives: tick source, delays, mutexes.
 * Desktop: POSIX.  ESP32: FreeRTOS.
 */

#ifndef HAL_PLATFORM_H
#define HAL_PLATFORM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Get current tick count in milliseconds (monotonic). */
uint32_t hal_tick_ms(void);

/** Sleep for the given number of milliseconds. */
void hal_delay_ms(uint32_t ms);

/** Opaque mutex handle. */
typedef void *hal_mutex_t;

/** Create a new mutex. Returns NULL on failure. */
hal_mutex_t hal_mutex_create(void);

/** Lock the mutex (blocking). */
void hal_mutex_lock(hal_mutex_t mutex);

/** Unlock the mutex. */
void hal_mutex_unlock(hal_mutex_t mutex);

/** Destroy the mutex and free resources. */
void hal_mutex_destroy(hal_mutex_t mutex);

#ifdef __cplusplus
}
#endif

#endif /* HAL_PLATFORM_H */
