/**
 * @file skin_tachometer.c
 * Tachometer gauge skin -- arc-based RPM display.
 *
 * Uses LVGL arc + label widgets to render an RPM gauge.
 */

#include "skin.h"
#include "skin_registry.h"
#include "lvgl.h"

typedef struct {
    lv_obj_t   *arc;
    lv_obj_t   *label_rpm;
    lv_obj_t   *label_unit;
    int         max_rpm;
} tacho_ctx_t;

static void *tacho_create(lv_obj_t *parent, int width, int height)
{
    tacho_ctx_t *ctx = lv_malloc(sizeof(tacho_ctx_t));
    if (!ctx) return NULL;

    ctx->max_rpm = 8000;

    int size = (width < height) ? width : height;

    /* Background arc (full range, dark) */
    ctx->arc = lv_arc_create(parent);
    lv_obj_set_size(ctx->arc, size - 20, size - 20);
    lv_obj_center(ctx->arc);
    lv_arc_set_rotation(ctx->arc, 135);
    lv_arc_set_bg_angles(ctx->arc, 0, 270);
    lv_arc_set_range(ctx->arc, 0, (int16_t)ctx->max_rpm);
    lv_arc_set_value(ctx->arc, 0);
    lv_obj_remove_style(ctx->arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(ctx->arc, LV_OBJ_FLAG_CLICKABLE);

    /* Style the indicator (colored part) */
    lv_obj_set_style_arc_width(ctx->arc, 20, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(ctx->arc, lv_color_hex(0xFF4444), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx->arc, 20, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ctx->arc, lv_color_hex(0x333333), LV_PART_MAIN);

    /* RPM value label */
    ctx->label_rpm = lv_label_create(parent);
    lv_label_set_text(ctx->label_rpm, "0");
    lv_obj_set_style_text_font(ctx->label_rpm, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(ctx->label_rpm, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(ctx->label_rpm, LV_ALIGN_CENTER, 0, -10);

    /* Unit label */
    ctx->label_unit = lv_label_create(parent);
    lv_label_set_text(ctx->label_unit, "RPM");
    lv_obj_set_style_text_font(ctx->label_unit, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(ctx->label_unit, lv_color_hex(0x888888), 0);
    lv_obj_align(ctx->label_unit, LV_ALIGN_CENTER, 0, 30);

    return ctx;
}

static void tacho_update(void *opaque, const vehicle_data_t *data)
{
    tacho_ctx_t *ctx = (tacho_ctx_t *)opaque;
    if (!ctx || !data) return;

    int16_t rpm = (int16_t)((data->rpm > (uint16_t)ctx->max_rpm) ? ctx->max_rpm : data->rpm);
    lv_arc_set_value(ctx->arc, rpm);

    /* Update label */
    lv_label_set_text_fmt(ctx->label_rpm, "%d", data->rpm);
    lv_obj_align(ctx->label_rpm, LV_ALIGN_CENTER, 0, -10);

    /* Color shift: green < 4000, yellow 4000-6000, red > 6000 */
    lv_color_t color;
    if (data->rpm < 4000) {
        color = lv_color_hex(0x44FF44);
    } else if (data->rpm < 6000) {
        color = lv_color_hex(0xFFCC00);
    } else {
        color = lv_color_hex(0xFF4444);
    }
    lv_obj_set_style_arc_color(ctx->arc, color, LV_PART_INDICATOR);
}

static void tacho_destroy(void *opaque)
{
    if (opaque) {
        lv_free(opaque);
    }
}

const gauge_skin_t skin_tachometer = {
    .name         = "tachometer",
    .display_name = "Arc Tachometer",
    .create       = tacho_create,
    .update       = tacho_update,
    .destroy      = tacho_destroy,
};
