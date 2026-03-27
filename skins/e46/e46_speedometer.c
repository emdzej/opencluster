/**
 * @file e46_speedometer.c
 * BMW E46 speedometer skin.
 *
 * Large round needle gauge: 0-260 km/h, ~270° sweep.
 * White markings on black face, white needle with center hub.
 * Matches the center-left gauge in the E46 cluster.
 */

#include "skin.h"
#include "skin_registry.h"
#include "e46_common.h"
#include "lvgl.h"

#include <stdio.h>

typedef struct {
    lv_obj_t *scale;
    lv_obj_t *needle;
    lv_obj_t *hub;
    lv_obj_t *label_speed;   /* Digital readout at bottom center */
    lv_obj_t *label_unit;    /* "km/h" */
    int       max_kmh;
    int       scale_dia;
    uint8_t   last_backlight; /* Track backlight state for change detection */
} e46_speedo_ctx_t;

/* Major tick labels: every 20 km/h from 0 to 260 */
static const char *speed_labels[] = {
    "0", "20", "40", "60", "80", "100", "120", "140",
    "160", "180", "200", "220", "240", "260", NULL
};

static void *e46_speedo_create(lv_obj_t *parent, int width, int height)
{
    e46_speedo_ctx_t *ctx = lv_malloc(sizeof(e46_speedo_ctx_t));
    if (!ctx) return NULL;

    ctx->max_kmh = 260;

    int size = (width < height) ? width : height;
    ctx->scale_dia = size - 10;

    /* Black circular background */
    lv_obj_set_style_bg_color(parent, lv_color_hex(E46_BLACK), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(parent, LV_RADIUS_CIRCLE, 0);

    /* Scale -- large gauge proportions */
    ctx->scale = e46_create_round_scale_ex(parent, ctx->scale_dia, true);
    lv_scale_set_range(ctx->scale, 0, ctx->max_kmh);
    lv_scale_set_angle_range(ctx->scale, 270);
    lv_scale_set_rotation(ctx->scale, 135);  /* Start at ~7 o'clock */

    /* 14 major divisions (0,20,...,260) => 13 intervals, minor ticks between */
    lv_scale_set_total_tick_count(ctx->scale, 131);  /* 13*10 + 1 */
    lv_scale_set_major_tick_every(ctx->scale, 10);
    lv_scale_set_label_show(ctx->scale, true);
    lv_scale_set_text_src(ctx->scale, speed_labels);

    /* Needle -- large gauge proportions */
    ctx->needle = e46_create_needle(ctx->scale, E46_LG_NEEDLE_WIDTH);
    lv_scale_set_line_needle_value(ctx->scale, ctx->needle,
                                   ctx->scale_dia / 2 - E46_LG_NEEDLE_INSET, 0);

    /* Hub -- large gauge proportions */
    ctx->hub = e46_create_hub_ex(parent, true);

    /* Digital speed readout */
    ctx->label_speed = lv_label_create(parent);
    lv_label_set_text(ctx->label_speed, "0");
    lv_obj_set_style_text_font(ctx->label_speed, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(ctx->label_speed, lv_color_hex(E46_WHITE), 0);
    lv_obj_align(ctx->label_speed, LV_ALIGN_CENTER, 0, size / 5);

    /* Unit label */
    ctx->label_unit = lv_label_create(parent);
    lv_label_set_text(ctx->label_unit, "km/h");
    lv_obj_set_style_text_font(ctx->label_unit, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(ctx->label_unit, lv_color_hex(E46_DIM_LABEL), 0);
    lv_obj_align(ctx->label_unit, LV_ALIGN_CENTER, 0, size / 5 + 20);

    ctx->last_backlight = 0;

    return ctx;
}

static void e46_speedo_update(void *opaque, const vehicle_data_t *data)
{
    e46_speedo_ctx_t *ctx = (e46_speedo_ctx_t *)opaque;
    if (!ctx || !data) return;

    /* Backlight theme change detection */
    bool bl_on = data->backlight > 0;
    bool bl_changed = (data->backlight > 0) != (ctx->last_backlight > 0);
    if (bl_changed) {
        e46_apply_backlight(ctx->scale, ctx->needle, ctx->hub, bl_on);
        /* Digital readout and unit label colors */
        lv_obj_set_style_text_color(ctx->label_speed,
                                    lv_color_hex(e46_mark_color(bl_on)), 0);
        lv_obj_set_style_text_color(ctx->label_unit,
                                    lv_color_hex(e46_dim_color(bl_on)), 0);
        ctx->last_backlight = data->backlight;
    }

    int speed_kmh = data->speed_kmh_x10 / 10;
    if (speed_kmh > ctx->max_kmh) speed_kmh = ctx->max_kmh;
    if (speed_kmh < 0) speed_kmh = 0;

    lv_scale_set_line_needle_value(ctx->scale, ctx->needle,
                                   ctx->scale_dia / 2 - E46_LG_NEEDLE_INSET,
                                   speed_kmh);

    /* Digital readout */
    lv_label_set_text_fmt(ctx->label_speed, "%d", data->speed_kmh_x10 / 10);
    lv_obj_align(ctx->label_speed, LV_ALIGN_CENTER, 0,
                 lv_obj_get_height(lv_obj_get_parent(ctx->scale)) / 5);
}

static void e46_speedo_destroy(void *opaque)
{
    if (opaque) lv_free(opaque);
}

const gauge_skin_t skin_e46_speedometer = {
    .name         = "e46_speedometer",
    .display_name = "BMW E46 Speedometer",
    .create       = e46_speedo_create,
    .update       = e46_speedo_update,
    .destroy      = e46_speedo_destroy,
};
