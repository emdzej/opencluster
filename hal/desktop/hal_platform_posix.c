/**
 * @file hal_platform_posix.c
 * Desktop platform HAL implementation using POSIX APIs.
 */

#include "hal_platform.h"

#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

uint32_t hal_tick_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void hal_delay_ms(uint32_t ms)
{
    usleep(ms * 1000);
}

hal_mutex_t hal_mutex_create(void)
{
    pthread_mutex_t *mtx = malloc(sizeof(pthread_mutex_t));
    if (!mtx) return NULL;
    if (pthread_mutex_init(mtx, NULL) != 0) {
        free(mtx);
        return NULL;
    }
    return (hal_mutex_t)mtx;
}

void hal_mutex_lock(hal_mutex_t mutex)
{
    if (mutex) {
        pthread_mutex_lock((pthread_mutex_t *)mutex);
    }
}

void hal_mutex_unlock(hal_mutex_t mutex)
{
    if (mutex) {
        pthread_mutex_unlock((pthread_mutex_t *)mutex);
    }
}

void hal_mutex_destroy(hal_mutex_t mutex)
{
    if (mutex) {
        pthread_mutex_destroy((pthread_mutex_t *)mutex);
        free(mutex);
    }
}
