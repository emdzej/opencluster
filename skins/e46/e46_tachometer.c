/**
 * @file e46_tachometer.c
 * BMW E46 tachometer skins -- petrol (7k) and diesel (6k).
 *
 * Large round needle gauge with red zone and fuel consumption sub-gauge
 * displayed as a text label near the bottom of the dial face.
 *
 * Two exported skins:
 *   skin_e46_tach_7k  -- Petrol: 0-7000 RPM, red zone 6500+
 *   skin_e46_tach_6k  -- Diesel: 0-6000 RPM, red zone 5000+
 */

#include "skin.h"
#include "skin_registry.h"
#include "e46_common.h"
#include "lvgl.h"

#include <stdio.h>

/* ── Configuration for the two variants ─────────────────────────────── */

typedef struct {
    int  max_rpm;          /* Full-scale RPM value */
    int  red_zone_start;   /* RPM where red zone begins */
    int  major_count;      /* Number of major labels (including 0) */
    int  ticks_per_major;  /* Minor ticks per major interval */
    const char **labels;   /* Major tick label array (NULL-terminated) */
} e46_tach_config_t;

/* Petrol: 0-7, major every 1000, red zone 6500+ */
static const char *labels_7k[] = {
    "0", "1", "2", "3", "4", "5", "6", "7", NULL
};

static const e46_tach_config_t config_7k = {
    .max_rpm        = 7000,
    .red_zone_start = 6500,
    .major_count    = 8,     /* 0..7 */
    .ticks_per_major = 5,
    .labels          = labels_7k,
};

/* Diesel: 0-6, major every 1000, red zone 5000+ */
static const char *labels_6k[] = {
    "0", "1", "2", "3", "4", "5", "6", NULL
};

static const e46_tach_config_t config_6k = {
    .max_rpm        = 6000,
    .red_zone_start = 5000,
    .major_count    = 7,     /* 0..6 */
    .ticks_per_major = 5,
    .labels          = labels_6k,
};

/* ── Runtime context ────────────────────────────────────────────────── */

typedef struct {
    lv_obj_t *scale;
    lv_obj_t *needle;
    lv_obj_t *hub;
    lv_obj_t *label_consumption;   /* Fuel consumption sub-gauge */
    lv_obj_t *label_consumption_unit;
    lv_obj_t *label_x1000;        /* "x1000 r/min" */
    int       scale_dia;
    int       max_rpm;
    uint8_t   last_backlight;      /* Track backlight state for change detection */
    /* Red zone section styles -- must persist for the lifetime of the widget */
    lv_style_t style_red_indicator;
    lv_style_t style_red_items;
} e46_tach_ctx_t;

/* ── Shared create logic ────────────────────────────────────────────── */

static void *e46_tach_create_impl(lv_obj_t *parent, int width, int height,
                                  const e46_tach_config_t *cfg)
{
    e46_tach_ctx_t *ctx = lv_malloc(sizeof(e46_tach_ctx_t));
    if (!ctx) return NULL;

    ctx->max_rpm = cfg->max_rpm;

    int size = (width < height) ? width : height;
    ctx->scale_dia = size - 10;

    /* Black circular background */
    lv_obj_set_style_bg_color(parent, lv_color_hex(E46_BLACK), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(parent, LV_RADIUS_CIRCLE, 0);

    /* Scale -- large gauge proportions */
    ctx->scale = e46_create_round_scale_ex(parent, ctx->scale_dia, true);
    lv_scale_set_range(ctx->scale, 0, cfg->max_rpm);
    lv_scale_set_angle_range(ctx->scale, 270);
    lv_scale_set_rotation(ctx->scale, 135);

    /* Ticks: (major_count - 1) intervals * ticks_per_major + 1 */
    int total_ticks = (cfg->major_count - 1) * cfg->ticks_per_major + 1;
    lv_scale_set_total_tick_count(ctx->scale, (uint32_t)total_ticks);
    lv_scale_set_major_tick_every(ctx->scale, (uint32_t)cfg->ticks_per_major);
    lv_scale_set_label_show(ctx->scale, true);
    lv_scale_set_text_src(ctx->scale, cfg->labels);

    /* ── Red zone section ───────────────────────────────────────── */
    lv_style_init(&ctx->style_red_indicator);
    lv_style_set_line_color(&ctx->style_red_indicator, lv_color_hex(E46_RED_ZONE));
    lv_style_set_line_width(&ctx->style_red_indicator, 4);  /* Heavy red zone */
    lv_style_set_text_color(&ctx->style_red_indicator, lv_color_hex(E46_RED_ZONE));

    lv_style_init(&ctx->style_red_items);
    lv_style_set_line_color(&ctx->style_red_items, lv_color_hex(E46_RED_ZONE));
    lv_style_set_line_width(&ctx->style_red_items, 3);  /* Minor ticks in red zone */

    lv_scale_section_t *red_sec = lv_scale_add_section(ctx->scale);
    lv_scale_section_set_range(red_sec, cfg->red_zone_start, cfg->max_rpm);
    lv_scale_set_section_style_indicator(ctx->scale, red_sec, &ctx->style_red_indicator);
    lv_scale_set_section_style_items(ctx->scale, red_sec, &ctx->style_red_items);

    /* Needle -- large gauge proportions */
    ctx->needle = e46_create_needle(ctx->scale, E46_LG_NEEDLE_WIDTH);
    lv_scale_set_line_needle_value(ctx->scale, ctx->needle,
                                   ctx->scale_dia / 2 - E46_LG_NEEDLE_INSET, 0);

    /* Hub -- large gauge proportions */
    ctx->hub = e46_create_hub_ex(parent, true);

    /* "x1000 r/min" text -- positioned higher (near top of dial) */
    ctx->label_x1000 = lv_label_create(parent);
    lv_label_set_text(ctx->label_x1000, "x1000 r/min");
    lv_obj_set_style_text_font(ctx->label_x1000, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(ctx->label_x1000, lv_color_hex(E46_DIM_LABEL), 0);
    lv_obj_align(ctx->label_x1000, LV_ALIGN_CENTER, 0, -(size / 4));

    /* Fuel consumption readout */
    ctx->label_consumption = lv_label_create(parent);
    lv_label_set_text(ctx->label_consumption, "--.-");
    lv_obj_set_style_text_font(ctx->label_consumption, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(ctx->label_consumption, lv_color_hex(E46_WHITE), 0);
    lv_obj_align(ctx->label_consumption, LV_ALIGN_CENTER, 0, size / 5);

    ctx->label_consumption_unit = lv_label_create(parent);
    lv_label_set_text(ctx->label_consumption_unit, "L/100km");
    lv_obj_set_style_text_font(ctx->label_consumption_unit, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(ctx->label_consumption_unit,
                                lv_color_hex(E46_DIM_LABEL), 0);
    lv_obj_align(ctx->label_consumption_unit, LV_ALIGN_CENTER, 0, size / 5 + 16);

    ctx->last_backlight = 0;

    return ctx;
}

/* ── Shared update ──────────────────────────────────────────────────── */

static void e46_tach_update(void *opaque, const vehicle_data_t *data)
{
    e46_tach_ctx_t *ctx = (e46_tach_ctx_t *)opaque;
    if (!ctx || !data) return;

    /* Backlight theme change detection */
    bool bl_on = data->backlight > 0;
    bool bl_changed = (data->backlight > 0) != (ctx->last_backlight > 0);
    if (bl_changed) {
        e46_apply_backlight(ctx->scale, ctx->needle, ctx->hub, bl_on);
        /* Consumption text + unit + x1000 label */
        lv_obj_set_style_text_color(ctx->label_consumption,
                                    lv_color_hex(e46_mark_color(bl_on)), 0);
        lv_obj_set_style_text_color(ctx->label_consumption_unit,
                                    lv_color_hex(e46_dim_color(bl_on)), 0);
        lv_obj_set_style_text_color(ctx->label_x1000,
                                    lv_color_hex(e46_dim_color(bl_on)), 0);
        ctx->last_backlight = data->backlight;
    }

    int rpm = (int)data->rpm;
    if (rpm > ctx->max_rpm) rpm = ctx->max_rpm;
    if (rpm < 0) rpm = 0;

    lv_scale_set_line_needle_value(ctx->scale, ctx->needle,
                                   ctx->scale_dia / 2 - E46_LG_NEEDLE_INSET, rpm);

    /* Fuel consumption display */
    if (data->fuel_consumption_x10 > 0) {
        int whole = data->fuel_consumption_x10 / 10;
        int frac  = data->fuel_consumption_x10 % 10;
        lv_label_set_text_fmt(ctx->label_consumption, "%d.%d", whole, frac);
    } else {
        lv_label_set_text(ctx->label_consumption, "--.-");
    }

    /* Re-center after text change */
    int parent_h = lv_obj_get_height(lv_obj_get_parent(ctx->scale));
    lv_obj_align(ctx->label_consumption, LV_ALIGN_CENTER, 0, parent_h / 5);
}

/* ── Shared destroy ─────────────────────────────────────────────────── */

static void e46_tach_destroy(void *opaque)
{
    if (opaque) {
        e46_tach_ctx_t *ctx = (e46_tach_ctx_t *)opaque;
        lv_style_reset(&ctx->style_red_indicator);
        lv_style_reset(&ctx->style_red_items);
        lv_free(ctx);
    }
}

/* ── Petrol 7k variant ──────────────────────────────────────────────── */

static void *e46_tach_7k_create(lv_obj_t *parent, int width, int height)
{
    return e46_tach_create_impl(parent, width, height, &config_7k);
}

const gauge_skin_t skin_e46_tach_7k = {
    .name         = "e46_tach_7k",
    .display_name = "BMW E46 Tach (Petrol 7k)",
    .create       = e46_tach_7k_create,
    .update       = e46_tach_update,
    .destroy      = e46_tach_destroy,
};

/* ── Diesel 6k variant ──────────────────────────────────────────────── */

static void *e46_tach_6k_create(lv_obj_t *parent, int width, int height)
{
    return e46_tach_create_impl(parent, width, height, &config_6k);
}

const gauge_skin_t skin_e46_tach_6k = {
    .name         = "e46_tach_6k",
    .display_name = "BMW E46 Tach (Diesel 6k)",
    .create       = e46_tach_6k_create,
    .update       = e46_tach_update,
    .destroy      = e46_tach_destroy,
};
