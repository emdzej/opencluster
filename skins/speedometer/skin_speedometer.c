/**
 * @file skin_speedometer.c
 * Speedometer gauge skin -- scale + needle style speed display.
 *
 * Uses LVGL arc + labels to show speed in km/h.
 */

#include "skin.h"
#include "skin_registry.h"
#include "lvgl.h"

typedef struct {
    lv_obj_t   *arc;
    lv_obj_t   *label_speed;
    lv_obj_t   *label_unit;
    lv_obj_t   *label_gear;
    int         max_speed;   /* km/h */
} speedo_ctx_t;

static void *speedo_create(lv_obj_t *parent, int width, int height)
{
    speedo_ctx_t *ctx = lv_malloc(sizeof(speedo_ctx_t));
    if (!ctx) return NULL;

    ctx->max_speed = 260;

    int size = (width < height) ? width : height;

    /* Background arc */
    ctx->arc = lv_arc_create(parent);
    lv_obj_set_size(ctx->arc, size - 20, size - 20);
    lv_obj_center(ctx->arc);
    lv_arc_set_rotation(ctx->arc, 135);
    lv_arc_set_bg_angles(ctx->arc, 0, 270);
    lv_arc_set_range(ctx->arc, 0, (int16_t)ctx->max_speed);
    lv_arc_set_value(ctx->arc, 0);
    lv_obj_remove_style(ctx->arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(ctx->arc, LV_OBJ_FLAG_CLICKABLE);

    /* Style */
    lv_obj_set_style_arc_width(ctx->arc, 16, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(ctx->arc, lv_color_hex(0x00AAFF), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx->arc, 16, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ctx->arc, lv_color_hex(0x333333), LV_PART_MAIN);

    /* Speed value */
    ctx->label_speed = lv_label_create(parent);
    lv_label_set_text(ctx->label_speed, "0");
    lv_obj_set_style_text_font(ctx->label_speed, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(ctx->label_speed, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(ctx->label_speed, LV_ALIGN_CENTER, 0, -15);

    /* Unit label */
    ctx->label_unit = lv_label_create(parent);
    lv_label_set_text(ctx->label_unit, "km/h");
    lv_obj_set_style_text_font(ctx->label_unit, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ctx->label_unit, lv_color_hex(0x888888), 0);
    lv_obj_align(ctx->label_unit, LV_ALIGN_CENTER, 0, 25);

    /* Gear indicator */
    ctx->label_gear = lv_label_create(parent);
    lv_label_set_text(ctx->label_gear, "N");
    lv_obj_set_style_text_font(ctx->label_gear, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(ctx->label_gear, lv_color_hex(0x44FF44), 0);
    lv_obj_align(ctx->label_gear, LV_ALIGN_CENTER, 0, 55);

    return ctx;
}

static void speedo_update(void *opaque, const vehicle_data_t *data)
{
    speedo_ctx_t *ctx = (speedo_ctx_t *)opaque;
    if (!ctx || !data) return;

    int speed_kmh = data->speed_kmh_x10 / 10;
    int16_t clamped = (int16_t)((speed_kmh > ctx->max_speed) ? ctx->max_speed : speed_kmh);
    lv_arc_set_value(ctx->arc, clamped);

    lv_label_set_text_fmt(ctx->label_speed, "%d", speed_kmh);
    lv_obj_align(ctx->label_speed, LV_ALIGN_CENTER, 0, -15);

    /* Gear display */
    if (data->gear == 0) {
        lv_label_set_text(ctx->label_gear, "N");
        lv_obj_set_style_text_color(ctx->label_gear, lv_color_hex(0x44FF44), 0);
    } else if (data->gear == 255) {
        lv_label_set_text(ctx->label_gear, "R");
        lv_obj_set_style_text_color(ctx->label_gear, lv_color_hex(0xFF4444), 0);
    } else {
        lv_label_set_text_fmt(ctx->label_gear, "%d", data->gear);
        lv_obj_set_style_text_color(ctx->label_gear, lv_color_hex(0xFFFFFF), 0);
    }

    /* Arc color: blue up to 120, yellow 120-180, red 180+ */
    lv_color_t color;
    if (speed_kmh < 120) {
        color = lv_color_hex(0x00AAFF);
    } else if (speed_kmh < 180) {
        color = lv_color_hex(0xFFCC00);
    } else {
        color = lv_color_hex(0xFF4444);
    }
    lv_obj_set_style_arc_color(ctx->arc, color, LV_PART_INDICATOR);
}

static void speedo_destroy(void *opaque)
{
    if (opaque) {
        lv_free(opaque);
    }
}

const gauge_skin_t skin_speedometer = {
    .name         = "speedometer",
    .display_name = "Arc Speedometer",
    .create       = speedo_create,
    .update       = speedo_update,
    .destroy      = speedo_destroy,
};
