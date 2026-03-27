/**
 * @file skin.h
 * OpenCluster gauge skin interface.
 *
 * A skin is a self-contained gauge renderer.  It knows how to create
 * LVGL widgets, update them from vehicle data, and tear them down.
 */

#ifndef SKIN_H
#define SKIN_H

#include "lvgl.h"
#include "vehicle_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Gauge skin definition.
 *
 * Each skin provides three lifecycle functions:
 * - create:  build the LVGL widget tree inside a parent, return opaque context
 * - update:  called each frame with fresh vehicle data
 * - destroy: tear down widgets, free context
 */
typedef struct {
    const char *name;           /**< Unique ID, e.g. "tachometer_arc" */
    const char *display_name;   /**< Human-readable, e.g. "Arc Tachometer" */

    /** Create the gauge UI inside `parent`. Returns opaque context pointer. */
    void *(*create)(lv_obj_t *parent, int width, int height);

    /** Update the gauge with current vehicle data. */
    void  (*update)(void *ctx, const vehicle_data_t *data);

    /** Destroy the gauge and free context. */
    void  (*destroy)(void *ctx);
} gauge_skin_t;

#ifdef __cplusplus
}
#endif

#endif /* SKIN_H */
