/**
 * @file vehicle_data.c
 * Vehicle data store implementation with mutex protection.
 */

#include "vehicle_data.h"
#include "hal_platform.h"
#include <string.h>

static vehicle_data_t s_data;
static hal_mutex_t    s_mutex;

void vehicle_data_init(void)
{
    memset(&s_data, 0, sizeof(s_data));
    s_mutex = hal_mutex_create();
}

void vehicle_data_get(vehicle_data_t *out)
{
    if (!out) return;
    hal_mutex_lock(s_mutex);
    memcpy(out, &s_data, sizeof(vehicle_data_t));
    hal_mutex_unlock(s_mutex);
}

const vehicle_data_t *vehicle_data_get_ptr(void)
{
    return &s_data;
}

void vehicle_data_lock(void)
{
    hal_mutex_lock(s_mutex);
}

void vehicle_data_unlock(void)
{
    hal_mutex_unlock(s_mutex);
}

void vehicle_data_update(const vehicle_data_t *new_data)
{
    if (!new_data) return;
    hal_mutex_lock(s_mutex);
    memcpy(&s_data, new_data, sizeof(vehicle_data_t));
    s_data.last_update_ms = hal_tick_ms();
    hal_mutex_unlock(s_mutex);
}
