/**
 * @file layout.h
 * OpenCluster layout template system.
 *
 * A layout defines how multiple gauge slots are arranged on a screen.
 * Positions use permille (0-1000) coordinates so they scale to any
 * resolution.  The layout engine translates these to pixel positions
 * at render time.
 */

#ifndef LAYOUT_H
#define LAYOUT_H

#include "lvgl.h"
#include "skin.h"
#include "vehicle_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of gauge slots per layout. */
#define LAYOUT_MAX_SLOTS 4

/** Maximum number of built-in + registered layouts. */
#define LAYOUT_REGISTRY_MAX 16

/**
 * A slot defines one gauge's position and size within the layout.
 * All values are in permille (0-1000) relative to the screen.
 */
typedef struct {
    int x;   /**< Left edge, 0-1000 permille */
    int y;   /**< Top edge, 0-1000 permille */
    int w;   /**< Width, 0-1000 permille */
    int h;   /**< Height, 0-1000 permille */
} layout_slot_t;

/**
 * A layout template defines the arrangement of gauge slots on screen.
 */
typedef struct {
    const char   *name;                      /**< Unique ID, e.g. "dual_horizontal" */
    int           slot_count;                /**< Number of active slots (1-4) */
    layout_slot_t slots[LAYOUT_MAX_SLOTS];   /**< Slot definitions */
} layout_template_t;

/**
 * Runtime state for one active slot -- binds a skin to a position.
 */
typedef struct {
    const gauge_skin_t *skin;    /**< The skin rendering this slot (NULL = empty) */
    void               *ctx;     /**< Opaque skin context returned by skin->create */
    lv_obj_t           *cont;    /**< LVGL container object for this slot */
} layout_slot_state_t;

/**
 * Runtime state for the entire layout.
 */
typedef struct {
    const layout_template_t *tmpl;                     /**< Active layout template */
    layout_slot_state_t      slots[LAYOUT_MAX_SLOTS];  /**< Per-slot state */
    lv_obj_t                *screen;                   /**< LVGL screen object */
    int                      screen_w;                 /**< Screen width in pixels */
    int                      screen_h;                 /**< Screen height in pixels */
} layout_state_t;

/* ── Built-in layout templates ──────────────────────────────────────── */

/** Full-screen single gauge. */
extern const layout_template_t layout_single;

/** Two gauges side-by-side (for rectangular displays). */
extern const layout_template_t layout_dual_horizontal;

/** Two gauges stacked top-bottom. */
extern const layout_template_t layout_dual_vertical;

/** 2x2 grid of four gauges. */
extern const layout_template_t layout_quad;

/** BMW E46 asymmetric: small-large-large-small (fuel, speedo, tacho, coolant). */
extern const layout_template_t layout_e46_cluster;

/* ── Layout registry ────────────────────────────────────────────────── */

/**
 * Register a layout template.
 * Built-in layouts are auto-registered on first use.
 * @return 0 on success, -1 if registry is full.
 */
int layout_registry_register(const layout_template_t *tmpl);

/**
 * Look up a layout by name.
 * @return Pointer to the template, or NULL if not found.
 */
const layout_template_t *layout_registry_find(const char *name);

/** Get layout by index. */
const layout_template_t *layout_registry_get(int index);

/** Get total number of registered layouts. */
int layout_registry_count(void);

/** Register all built-in layouts.  Safe to call multiple times. */
void layout_register_builtins(void);

/* ── Layout lifecycle ───────────────────────────────────────────────── */

/**
 * Initialize a layout state.  Creates LVGL containers for each slot
 * based on the template's permille coordinates, scaled to the actual
 * screen size.
 *
 * @param state     Layout state to initialize.
 * @param tmpl      Layout template to use.
 * @param screen    LVGL screen to create widgets on.
 * @param screen_w  Screen width in pixels.
 * @param screen_h  Screen height in pixels.
 * @return 0 on success.
 */
int layout_init(layout_state_t *state,
                const layout_template_t *tmpl,
                lv_obj_t *screen,
                int screen_w, int screen_h);

/**
 * Assign a skin to a slot.  Creates the skin's widgets inside the
 * slot's container.
 *
 * @param state  Layout state.
 * @param slot   Slot index (0-based).
 * @param skin   Gauge skin to assign.
 * @return 0 on success, -1 on error.
 */
int layout_set_skin(layout_state_t *state, int slot,
                    const gauge_skin_t *skin);

/**
 * Update all slots with fresh vehicle data.
 * Call this once per frame from the main loop.
 */
void layout_update(layout_state_t *state, const vehicle_data_t *data);

/**
 * Destroy all slot skins and containers.  The layout state can be
 * re-initialized with layout_init() afterwards.
 */
void layout_destroy(layout_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* LAYOUT_H */
