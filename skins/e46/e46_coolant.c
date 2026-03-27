/**
 * @file e46_coolant.c
 * BMW E46 coolant temperature skin.
 *
 * Small round gauge: cold (blue) on the left to hot (red) on the right.
 * ~240° sweep. Uses a thermometer icon symbol.
 * Matches the far-right gauge in the E46 cluster.
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
    int       scale_dia;
    uint8_t   last_backlight;  /* Track backlight state for change detection */
    /* Section styles must persist */
    lv_style_t style_cold_indicator;
    lv_style_t style_cold_items;
    lv_style_t style_hot_indicator;
    lv_style_t style_hot_items;
} e46_coolant_ctx_t;

/*
 * E46 coolant gauge: range 50-130°C (what's shown on the dial).
 * Internally we map coolant_temp_c to this range.
 * Labels: C (cold), midpoint, H (hot)
 */
static const char *coolant_labels[] = {"C", "", "", "", "H", NULL};

static void *e46_coolant_create(lv_obj_t *parent, int width, int height)
{
    e46_coolant_ctx_t *ctx = lv_malloc(sizeof(e46_coolant_ctx_t));
    if (!ctx) return NULL;

    int size = (width < height) ? width : height;
    ctx->scale_dia = size - 10;

    /* Black circular background */
    lv_obj_set_style_bg_color(parent, lv_color_hex(E46_BLACK), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(parent, LV_RADIUS_CIRCLE, 0);

    /* Scale: map 50..130°C to gauge range -- small gauge proportions */
    ctx->scale = e46_create_round_scale_ex(parent, ctx->scale_dia, false);
    lv_scale_set_range(ctx->scale, 50, 130);
    lv_scale_set_angle_range(ctx->scale, 240);
    lv_scale_set_rotation(ctx->scale, 150);

    lv_scale_set_total_tick_count(ctx->scale, 17);
    lv_scale_set_major_tick_every(ctx->scale, 4);
    lv_scale_set_label_show(ctx->scale, true);
    lv_scale_set_text_src(ctx->scale, coolant_labels);

    /* ── Cold zone section (blue): 50-70°C ──────────────────────── */
    lv_style_init(&ctx->style_cold_indicator);
    lv_style_set_line_color(&ctx->style_cold_indicator, lv_color_hex(E46_BLUE_ZONE));
    lv_style_set_line_width(&ctx->style_cold_indicator, E46_SM_MAJOR_WIDTH);
    lv_style_set_text_color(&ctx->style_cold_indicator, lv_color_hex(E46_BLUE_ZONE));

    lv_style_init(&ctx->style_cold_items);
    lv_style_set_line_color(&ctx->style_cold_items, lv_color_hex(E46_BLUE_ZONE));
    lv_style_set_line_width(&ctx->style_cold_items, E46_SM_MINOR_WIDTH);

    lv_scale_section_t *cold_sec = lv_scale_add_section(ctx->scale);
    lv_scale_section_set_range(cold_sec, 50, 70);
    lv_scale_set_section_style_indicator(ctx->scale, cold_sec, &ctx->style_cold_indicator);
    lv_scale_set_section_style_items(ctx->scale, cold_sec, &ctx->style_cold_items);

    /* ── Hot zone section (red): 110-130°C ──────────────────────── */
    lv_style_init(&ctx->style_hot_indicator);
    lv_style_set_line_color(&ctx->style_hot_indicator, lv_color_hex(E46_RED_ZONE));
    lv_style_set_line_width(&ctx->style_hot_indicator, E46_SM_MAJOR_WIDTH);
    lv_style_set_text_color(&ctx->style_hot_indicator, lv_color_hex(E46_RED_ZONE));

    lv_style_init(&ctx->style_hot_items);
    lv_style_set_line_color(&ctx->style_hot_items, lv_color_hex(E46_RED_ZONE));
    lv_style_set_line_width(&ctx->style_hot_items, E46_SM_MINOR_WIDTH);

    lv_scale_section_t *hot_sec = lv_scale_add_section(ctx->scale);
    lv_scale_section_set_range(hot_sec, 110, 130);
    lv_scale_set_section_style_indicator(ctx->scale, hot_sec, &ctx->style_hot_indicator);
    lv_scale_set_section_style_items(ctx->scale, hot_sec, &ctx->style_hot_items);

    /* Needle -- small gauge proportions */
    ctx->needle = e46_create_needle(ctx->scale, E46_SM_NEEDLE_WIDTH);
    lv_scale_set_line_needle_value(ctx->scale, ctx->needle,
                                   ctx->scale_dia / 2 - E46_SM_NEEDLE_INSET, 50);

    /* Hub -- small gauge proportions */
    ctx->hub = e46_create_hub_ex(parent, false);

    /* Thermometer icon (using built-in symbol) */
    ctx->label_icon = lv_label_create(parent);
    lv_label_set_text(ctx->label_icon, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(ctx->label_icon, lv_color_hex(E46_DIM_LABEL), 0);
    lv_obj_set_style_text_font(ctx->label_icon, &lv_font_montserrat_16, 0);
    lv_obj_align(ctx->label_icon, LV_ALIGN_CENTER, 0, size / 5);

    ctx->last_backlight = 0;

    return ctx;
}

static void e46_coolant_update(void *opaque, const vehicle_data_t *data)
{
    e46_coolant_ctx_t *ctx = (e46_coolant_ctx_t *)opaque;
    if (!ctx || !data) return;

    /* Backlight theme change detection */
    bool bl_on = data->backlight > 0;
    bool bl_changed = (data->backlight > 0) != (ctx->last_backlight > 0);
    if (bl_changed) {
        e46_apply_backlight(ctx->scale, ctx->needle, ctx->hub, bl_on);
        /* Blue/red zone section colors stay the same -- only normal
         * ticks/labels/needle change. The section styles override the
         * base styles for their ranges, so no action needed for zones. */
        ctx->last_backlight = data->backlight;
    }

    /* Clamp to gauge range */
    int temp = data->coolant_temp_c;
    if (temp < 50) temp = 50;
    if (temp > 130) temp = 130;

    lv_scale_set_line_needle_value(ctx->scale, ctx->needle,
                                   ctx->scale_dia / 2 - E46_SM_NEEDLE_INSET, temp);

    /* Warning icon turns red when overheating */
    if (data->engine_flags & ENG_OVERHEAT) {
        lv_obj_set_style_text_color(ctx->label_icon,
                                    lv_color_hex(E46_RED_ZONE), 0);
    } else if (data->coolant_temp_c < 60) {
        /* Cold -- show blue */
        lv_obj_set_style_text_color(ctx->label_icon,
                                    lv_color_hex(E46_BLUE_ZONE), 0);
    } else {
        /* Normal range -- dim (respects backlight) */
        lv_obj_set_style_text_color(ctx->label_icon,
                                    lv_color_hex(e46_dim_color(bl_on)), 0);
    }
}

static void e46_coolant_destroy(void *opaque)
{
    if (opaque) {
        e46_coolant_ctx_t *ctx = (e46_coolant_ctx_t *)opaque;
        lv_style_reset(&ctx->style_cold_indicator);
        lv_style_reset(&ctx->style_cold_items);
        lv_style_reset(&ctx->style_hot_indicator);
        lv_style_reset(&ctx->style_hot_items);
        lv_free(ctx);
    }
}

const gauge_skin_t skin_e46_coolant = {
    .name         = "e46_coolant",
    .display_name = "BMW E46 Coolant",
    .create       = e46_coolant_create,
    .update       = e46_coolant_update,
    .destroy      = e46_coolant_destroy,
};
