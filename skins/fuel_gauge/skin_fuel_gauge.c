/**
 * @file skin_fuel_gauge.c
 * Fuel level gauge skin -- vertical bar style.
 *
 * Renders a vertical fuel bar with percentage and low-fuel warning.
 */

#include "skin.h"
#include "skin_registry.h"
#include "lvgl.h"

typedef struct {
    lv_obj_t *bar;
    lv_obj_t *label_pct;
    lv_obj_t *label_title;
    lv_obj_t *label_warn;
} fuel_ctx_t;

static void *fuel_create(lv_obj_t *parent, int width, int height)
{
    fuel_ctx_t *ctx = lv_malloc(sizeof(fuel_ctx_t));
    if (!ctx) return NULL;

    int bar_w = width / 4;
    int bar_h = (height * 60) / 100;

    /* Fuel bar */
    ctx->bar = lv_bar_create(parent);
    lv_obj_set_size(ctx->bar, bar_w, bar_h);
    lv_obj_align(ctx->bar, LV_ALIGN_CENTER, 0, -10);
    lv_bar_set_range(ctx->bar, 0, 100);
    lv_bar_set_value(ctx->bar, 0, LV_ANIM_OFF);

    /* Bar style -- dark background */
    lv_obj_set_style_bg_color(ctx->bar, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ctx->bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(ctx->bar, 4, LV_PART_MAIN);

    /* Bar indicator style -- green by default */
    lv_obj_set_style_bg_color(ctx->bar, lv_color_hex(0x44FF44), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(ctx->bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(ctx->bar, 4, LV_PART_INDICATOR);

    /* Title label */
    ctx->label_title = lv_label_create(parent);
    lv_label_set_text(ctx->label_title, "FUEL");
    lv_obj_set_style_text_font(ctx->label_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ctx->label_title, lv_color_hex(0x888888), 0);
    lv_obj_align_to(ctx->label_title, ctx->bar, LV_ALIGN_OUT_TOP_MID, 0, -10);

    /* Percentage label */
    ctx->label_pct = lv_label_create(parent);
    lv_label_set_text(ctx->label_pct, "0%");
    lv_obj_set_style_text_font(ctx->label_pct, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(ctx->label_pct, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align_to(ctx->label_pct, ctx->bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

    /* Low fuel warning (hidden by default) */
    ctx->label_warn = lv_label_create(parent);
    lv_label_set_text(ctx->label_warn, "LOW");
    lv_obj_set_style_text_font(ctx->label_warn, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ctx->label_warn, lv_color_hex(0xFF4444), 0);
    lv_obj_align_to(ctx->label_warn, ctx->label_pct, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
    lv_obj_add_flag(ctx->label_warn, LV_OBJ_FLAG_HIDDEN);

    return ctx;
}

static void fuel_update(void *opaque, const vehicle_data_t *data)
{
    fuel_ctx_t *ctx = (fuel_ctx_t *)opaque;
    if (!ctx || !data) return;

    uint8_t pct = data->fuel_level_pct;
    if (pct > 100) pct = 100;

    lv_bar_set_value(ctx->bar, pct, LV_ANIM_ON);
    lv_label_set_text_fmt(ctx->label_pct, "%d%%", pct);
    lv_obj_align_to(ctx->label_pct, ctx->bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

    /* Color: green > 25%, yellow 10-25%, red < 10% */
    lv_color_t color;
    if (pct > 25) {
        color = lv_color_hex(0x44FF44);
    } else if (pct > 10) {
        color = lv_color_hex(0xFFCC00);
    } else {
        color = lv_color_hex(0xFF4444);
    }
    lv_obj_set_style_bg_color(ctx->bar, color, LV_PART_INDICATOR);

    /* Low fuel warning */
    if (data->warning_flags & WARN_LOW_FUEL) {
        lv_obj_remove_flag(ctx->label_warn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ctx->label_warn, LV_OBJ_FLAG_HIDDEN);
    }
}

static void fuel_destroy(void *opaque)
{
    if (opaque) {
        lv_free(opaque);
    }
}

const gauge_skin_t skin_fuel_gauge = {
    .name         = "fuel_gauge",
    .display_name = "Fuel Level Gauge",
    .create       = fuel_create,
    .update       = fuel_update,
    .destroy      = fuel_destroy,
};
