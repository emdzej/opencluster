/**
 * @file layout.c
 * OpenCluster layout template system -- implementation.
 *
 * Creates LVGL containers from permille-based slot definitions,
 * manages skin lifecycle per slot, and dispatches updates.
 */

#include "layout.h"

#include <string.h>

/* ── Built-in layout templates ──────────────────────────────────────── */

const layout_template_t layout_single = {
    .name       = "single",
    .slot_count = 1,
    .slots      = {
        { .x = 0, .y = 0, .w = 1000, .h = 1000 },
    },
};

const layout_template_t layout_dual_horizontal = {
    .name       = "dual_horizontal",
    .slot_count = 2,
    .slots      = {
        { .x = 0,   .y = 0, .w = 500, .h = 1000 },
        { .x = 500, .y = 0, .w = 500, .h = 1000 },
    },
};

const layout_template_t layout_dual_vertical = {
    .name       = "dual_vertical",
    .slot_count = 2,
    .slots      = {
        { .x = 0, .y = 0,   .w = 1000, .h = 500 },
        { .x = 0, .y = 500, .w = 1000, .h = 500 },
    },
};

const layout_template_t layout_quad = {
    .name       = "quad",
    .slot_count = 4,
    .slots      = {
        { .x = 0,   .y = 0,   .w = 500, .h = 500 },
        { .x = 500, .y = 0,   .w = 500, .h = 500 },
        { .x = 0,   .y = 500, .w = 500, .h = 500 },
        { .x = 500, .y = 500, .w = 500, .h = 500 },
    },
};

/* ── Layout registry ────────────────────────────────────────────────── */

static const layout_template_t *s_layouts[LAYOUT_REGISTRY_MAX];
static int s_layout_count = 0;
static bool s_builtins_registered = false;

void layout_register_builtins(void)
{
    if (s_builtins_registered) return;
    s_builtins_registered = true;

    layout_registry_register(&layout_single);
    layout_registry_register(&layout_dual_horizontal);
    layout_registry_register(&layout_dual_vertical);
    layout_registry_register(&layout_quad);
}

int layout_registry_register(const layout_template_t *tmpl)
{
    if (s_layout_count >= LAYOUT_REGISTRY_MAX) return -1;
    s_layouts[s_layout_count++] = tmpl;
    return 0;
}

const layout_template_t *layout_registry_find(const char *name)
{
    layout_register_builtins();
    for (int i = 0; i < s_layout_count; i++) {
        if (strcmp(s_layouts[i]->name, name) == 0) {
            return s_layouts[i];
        }
    }
    return NULL;
}

const layout_template_t *layout_registry_get(int index)
{
    layout_register_builtins();
    if (index < 0 || index >= s_layout_count) return NULL;
    return s_layouts[index];
}

int layout_registry_count(void)
{
    layout_register_builtins();
    return s_layout_count;
}

/* ── Layout lifecycle ───────────────────────────────────────────────── */

/**
 * Convert permille value to pixels.
 */
static inline int permille_to_px(int permille, int total_px)
{
    return (permille * total_px) / 1000;
}

int layout_init(layout_state_t *state,
                const layout_template_t *tmpl,
                lv_obj_t *screen,
                int screen_w, int screen_h)
{
    if (!state || !tmpl || !screen) return -1;

    memset(state, 0, sizeof(*state));
    state->tmpl     = tmpl;
    state->screen   = screen;
    state->screen_w = screen_w;
    state->screen_h = screen_h;

    /* Create a container for each slot */
    for (int i = 0; i < tmpl->slot_count; i++) {
        const layout_slot_t *slot = &tmpl->slots[i];

        int px_x = permille_to_px(slot->x, screen_w);
        int px_y = permille_to_px(slot->y, screen_h);
        int px_w = permille_to_px(slot->w, screen_w);
        int px_h = permille_to_px(slot->h, screen_h);

        lv_obj_t *cont = lv_obj_create(screen);
        lv_obj_remove_style_all(cont);
        lv_obj_set_pos(cont, px_x, px_y);
        lv_obj_set_size(cont, px_w, px_h);

        /* Black background, no border, no padding */
        lv_obj_set_style_bg_color(cont, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(cont, 0, 0);
        lv_obj_set_style_pad_all(cont, 0, 0);
        lv_obj_set_style_radius(cont, 0, 0);

        /* Clip children to container bounds */
        lv_obj_add_flag(cont, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

        state->slots[i].cont = cont;
        state->slots[i].skin = NULL;
        state->slots[i].ctx  = NULL;
    }

    return 0;
}

int layout_set_skin(layout_state_t *state, int slot,
                    const gauge_skin_t *skin)
{
    if (!state || !state->tmpl) return -1;
    if (slot < 0 || slot >= state->tmpl->slot_count) return -1;

    layout_slot_state_t *ss = &state->slots[slot];

    /* Destroy existing skin in this slot, if any */
    if (ss->skin && ss->ctx) {
        ss->skin->destroy(ss->ctx);
        ss->ctx = NULL;
    }

    if (!skin) {
        ss->skin = NULL;
        return 0;
    }

    /* Compute the slot's pixel size from the template (don't use
     * lv_obj_get_width/height -- LVGL may not have run a layout
     * pass yet, so those would return 0). */
    const layout_slot_t *slot_def = &state->tmpl->slots[slot];
    int slot_w = permille_to_px(slot_def->w, state->screen_w);
    int slot_h = permille_to_px(slot_def->h, state->screen_h);

    /* Create the skin inside this slot's container */
    void *ctx = skin->create(ss->cont, slot_w, slot_h);
    if (!ctx) return -1;

    ss->skin = skin;
    ss->ctx  = ctx;
    return 0;
}

void layout_update(layout_state_t *state, const vehicle_data_t *data)
{
    if (!state || !state->tmpl || !data) return;

    for (int i = 0; i < state->tmpl->slot_count; i++) {
        layout_slot_state_t *ss = &state->slots[i];
        if (ss->skin && ss->ctx) {
            ss->skin->update(ss->ctx, data);
        }
    }
}

void layout_destroy(layout_state_t *state)
{
    if (!state || !state->tmpl) return;

    for (int i = 0; i < state->tmpl->slot_count; i++) {
        layout_slot_state_t *ss = &state->slots[i];

        /* Destroy the skin */
        if (ss->skin && ss->ctx) {
            ss->skin->destroy(ss->ctx);
            ss->ctx  = NULL;
            ss->skin = NULL;
        }

        /* Delete the LVGL container (and all children) */
        if (ss->cont) {
            lv_obj_delete(ss->cont);
            ss->cont = NULL;
        }
    }

    state->tmpl = NULL;
}
