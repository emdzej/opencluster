/**
 * @file skin_coolant_temp.c
 * Coolant temperature gauge skin -- arc-based temperature display.
 *
 * Renders an arc gauge showing coolant temperature in Celsius
 * with color shifts for cold/normal/overheat zones.
 */

#include "skin.h"
#include "skin_registry.h"
#include "lvgl.h"

typedef struct {
    lv_obj_t *arc;
    lv_obj_t *label_temp;
    lv_obj_t *label_unit;
    lv_obj_t *label_warn;
    int       min_temp;
    int       max_temp;
} coolant_ctx_t;

static void *coolant_create(lv_obj_t *parent, int width, int height)
{
    coolant_ctx_t *ctx = lv_malloc(sizeof(coolant_ctx_t));
    if (!ctx) return NULL;

    ctx->min_temp = -40;
    ctx->max_temp = 130;   /* Display range up to 130C */

    int size = (width < height) ? width : height;

    /* Background arc */
    ctx->arc = lv_arc_create(parent);
    lv_obj_set_size(ctx->arc, size - 20, size - 20);
    lv_obj_center(ctx->arc);
    lv_arc_set_rotation(ctx->arc, 135);
    lv_arc_set_bg_angles(ctx->arc, 0, 270);
    lv_arc_set_range(ctx->arc, (int16_t)ctx->min_temp, (int16_t)ctx->max_temp);
    lv_arc_set_value(ctx->arc, ctx->min_temp);
    lv_obj_remove_style(ctx->arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(ctx->arc, LV_OBJ_FLAG_CLICKABLE);

    /* Style */
    lv_obj_set_style_arc_width(ctx->arc, 16, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(ctx->arc, lv_color_hex(0x4488FF), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx->arc, 16, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ctx->arc, lv_color_hex(0x333333), LV_PART_MAIN);

    /* Temperature value */
    ctx->label_temp = lv_label_create(parent);
    lv_label_set_text(ctx->label_temp, "--");
    lv_obj_set_style_text_font(ctx->label_temp, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(ctx->label_temp, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(ctx->label_temp, LV_ALIGN_CENTER, 0, -10);

    /* Unit label */
    ctx->label_unit = lv_label_create(parent);
    lv_label_set_text(ctx->label_unit, "COOLANT");
    lv_obj_set_style_text_font(ctx->label_unit, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ctx->label_unit, lv_color_hex(0x888888), 0);
    lv_obj_align(ctx->label_unit, LV_ALIGN_CENTER, 0, 25);

    /* Overheat warning (hidden by default) */
    ctx->label_warn = lv_label_create(parent);
    lv_label_set_text(ctx->label_warn, "OVERHEAT");
    lv_obj_set_style_text_font(ctx->label_warn, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ctx->label_warn, lv_color_hex(0xFF4444), 0);
    lv_obj_align(ctx->label_warn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_add_flag(ctx->label_warn, LV_OBJ_FLAG_HIDDEN);

    return ctx;
}

static void coolant_update(void *opaque, const vehicle_data_t *data)
{
    coolant_ctx_t *ctx = (coolant_ctx_t *)opaque;
    if (!ctx || !data) return;

    int temp = data->coolant_temp_c;
    int clamped = temp;
    if (clamped < ctx->min_temp) clamped = ctx->min_temp;
    if (clamped > ctx->max_temp) clamped = ctx->max_temp;

    lv_arc_set_value(ctx->arc, (int16_t)clamped);

    /* Temperature label (show actual, not clamped) */
    lv_label_set_text_fmt(ctx->label_temp, "%d°", temp);
    lv_obj_align(ctx->label_temp, LV_ALIGN_CENTER, 0, -10);

    /* Color zones:
     * Cold  (< 60C):  blue
     * Normal (60-100C): green
     * Warm  (100-110C): yellow
     * Hot   (> 110C):  red
     */
    lv_color_t color;
    if (temp < 60) {
        color = lv_color_hex(0x4488FF);
    } else if (temp <= 100) {
        color = lv_color_hex(0x44FF44);
    } else if (temp <= 110) {
        color = lv_color_hex(0xFFCC00);
    } else {
        color = lv_color_hex(0xFF4444);
    }
    lv_obj_set_style_arc_color(ctx->arc, color, LV_PART_INDICATOR);

    /* Overheat warning */
    if (data->engine_flags & ENG_OVERHEAT) {
        lv_obj_remove_flag(ctx->label_warn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ctx->label_warn, LV_OBJ_FLAG_HIDDEN);
    }
}

static void coolant_destroy(void *opaque)
{
    if (opaque) {
        lv_free(opaque);
    }
}

const gauge_skin_t skin_coolant_temp = {
    .name         = "coolant_temp",
    .display_name = "Coolant Temperature",
    .create       = coolant_create,
    .update       = coolant_update,
    .destroy      = coolant_destroy,
};
