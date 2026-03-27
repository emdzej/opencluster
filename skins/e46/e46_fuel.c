/**
 * @file e46_fuel.c
 * BMW E46 fuel gauge skin.
 *
 * Small round gauge: 0 (empty) to 1/1 (full), with 1/2 midpoint.
 * Needle sweeps ~270 degrees. Fuel pump icon simulated with label.
 * Matches the far-left gauge in the E46 cluster.
 */

#include "skin.h"
#include "skin_registry.h"
#include "e46_common.h"
#include "lvgl.h"

typedef struct {
    lv_obj_t *scale;
    lv_obj_t *needle;
    lv_obj_t *hub;
    lv_obj_t *label_icon;
    uint8_t   last_backlight;  /* Track backlight state for change detection */
} e46_fuel_ctx_t;

static const char *fuel_labels[] = {"0", "", "1/2", "", "1/1", NULL};

static void *e46_fuel_create(lv_obj_t *parent, int width, int height)
{
    e46_fuel_ctx_t *ctx = lv_malloc(sizeof(e46_fuel_ctx_t));
    if (!ctx) return NULL;

    int size = (width < height) ? width : height;
    int scale_dia = size - 10;

    /* Black circular background */
    lv_obj_set_style_bg_color(parent, lv_color_hex(E46_BLACK), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(parent, LV_RADIUS_CIRCLE, 0);

    /* Scale -- small gauge (default) */
    ctx->scale = e46_create_round_scale_ex(parent, scale_dia, false);
    lv_scale_set_range(ctx->scale, 0, 100);
    lv_scale_set_angle_range(ctx->scale, 240);
    lv_scale_set_rotation(ctx->scale, 150);

    lv_scale_set_total_tick_count(ctx->scale, 21);
    lv_scale_set_major_tick_every(ctx->scale, 5);
    lv_scale_set_label_show(ctx->scale, true);
    lv_scale_set_text_src(ctx->scale, fuel_labels);

    /* Needle -- small gauge proportions */
    ctx->needle = e46_create_needle(ctx->scale, E46_SM_NEEDLE_WIDTH);
    lv_scale_set_line_needle_value(ctx->scale, ctx->needle,
                                   scale_dia / 2 - E46_SM_NEEDLE_INSET, 0);

    /* Hub -- small gauge proportions */
    ctx->hub = e46_create_hub_ex(parent, false);

    /* Fuel pump icon */
    ctx->label_icon = lv_label_create(parent);
    lv_label_set_text(ctx->label_icon, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(ctx->label_icon, lv_color_hex(E46_DIM_LABEL), 0);
    lv_obj_set_style_text_font(ctx->label_icon, &lv_font_montserrat_16, 0);
    lv_obj_align(ctx->label_icon, LV_ALIGN_CENTER, 0, size / 5);

    ctx->last_backlight = 0;

    return ctx;
}

static void e46_fuel_update(void *opaque, const vehicle_data_t *data)
{
    e46_fuel_ctx_t *ctx = (e46_fuel_ctx_t *)opaque;
    if (!ctx || !data) return;

    /* Backlight theme change detection */
    bool bl_on = data->backlight > 0;
    bool bl_changed = (data->backlight > 0) != (ctx->last_backlight > 0);
    if (bl_changed) {
        e46_apply_backlight(ctx->scale, ctx->needle, ctx->hub, bl_on);
        ctx->last_backlight = data->backlight;
    }

    int val = data->fuel_level_pct;
    if (val > 100) val = 100;
    if (val < 0) val = 0;

    lv_scale_set_line_needle_value(ctx->scale, ctx->needle,
                                   lv_obj_get_width(ctx->scale) / 2 - E46_SM_NEEDLE_INSET,
                                   val);

    /* Warn color on icon when low */
    if (data->warning_flags & WARN_LOW_FUEL) {
        lv_obj_set_style_text_color(ctx->label_icon,
                                    lv_color_hex(E46_RED_ZONE), 0);
    } else {
        lv_obj_set_style_text_color(ctx->label_icon,
                                    lv_color_hex(e46_dim_color(bl_on)), 0);
    }
}

static void e46_fuel_destroy(void *opaque)
{
    if (opaque) lv_free(opaque);
}

const gauge_skin_t skin_e46_fuel = {
    .name         = "e46_fuel",
    .display_name = "BMW E46 Fuel",
    .create       = e46_fuel_create,
    .update       = e46_fuel_update,
    .destroy      = e46_fuel_destroy,
};
