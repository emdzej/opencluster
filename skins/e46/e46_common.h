/**
 * @file e46_common.h
 * BMW E46 cluster -- shared constants, colors, and helpers.
 *
 * All E46 gauge skins share a common visual language:
 * - Black face, white markings/numbers
 * - White needle with a center hub
 * - Red zone marking on tach
 * - Scale widget for tick marks + needle
 */

#ifndef E46_COMMON_H
#define E46_COMMON_H

#include "lvgl.h"
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── E46 color palette ─────────────────────────────────────────────── */
#define E46_BLACK          0x000000
#define E46_WHITE          0xFFFFFF
#define E46_AMBER          0xFF6400   /* Backlight ON color */
#define E46_NEEDLE         0xFFFFFF   /* White needle (backlight off) */
#define E46_TICK_MAJOR     0xFFFFFF
#define E46_TICK_MINOR     0xAAAAAA
#define E46_LABEL          0xFFFFFF
#define E46_RED_ZONE       0xFF3333
#define E46_BLUE_ZONE      0x4488FF   /* Coolant cold zone */
#define E46_DIM_LABEL      0x888888
#define E46_DIM_AMBER      0x995500   /* Dim text when backlight ON */
#define E46_FACE_BG        0x111111   /* Slightly lighter than pure black */
#define E46_NEEDLE_HUB     0x333333

/* ── Unit systems ──────────────────────────────────────────────────── */
typedef enum {
    E46_UNITS_METRIC,    /* km/h, L/100km */
    E46_UNITS_IMPERIAL,  /* MPH, MPG */
} e46_unit_system_t;

/* Convert km/h to mph */
static inline float e46_kmh_to_mph(float kmh) { return kmh * 0.621371f; }

/* Convert L/100km to MPG (US) */
static inline float e46_l100km_to_mpg(float l100km)
{
    if (l100km <= 0.0f) return 0.0f;
    return 235.215f / l100km;
}

/* ── Gauge size classes ─────────────────────────────────────────────── */
/*
 * The E46 cluster has two gauge sizes:
 *   LARGE = speedometer & tachometer (center pair)
 *   SMALL = fuel & coolant (outer pair)
 * Proportions are tuned to match the real cluster at 800x480.
 */

/* Large gauge defaults (speedometer, tachometer) */
#define E46_LG_MINOR_WIDTH    1
#define E46_LG_MINOR_LENGTH   6
#define E46_LG_MAJOR_WIDTH    3
#define E46_LG_MAJOR_LENGTH   18
#define E46_LG_NEEDLE_WIDTH   3
#define E46_LG_NEEDLE_INSET   10   /* px inward from scale radius */
#define E46_LG_HUB_DIA        24
#define E46_LG_HUB_BORDER     2

/* Small gauge defaults (fuel, coolant) */
#define E46_SM_MINOR_WIDTH    1
#define E46_SM_MINOR_LENGTH   5
#define E46_SM_MAJOR_WIDTH    2
#define E46_SM_MAJOR_LENGTH   12
#define E46_SM_NEEDLE_WIDTH   2
#define E46_SM_NEEDLE_INSET   8
#define E46_SM_HUB_DIA        16
#define E46_SM_HUB_BORDER     2

/* ── Shared gauge construction helpers ─────────────────────────────── */

/**
 * Create a round lv_scale configured for E46 look.
 * `is_large` selects large (speedo/tach) or small (fuel/coolant) proportions.
 * Returns the scale object. Caller should set range, tick counts, etc.
 */
static inline lv_obj_t *e46_create_round_scale_ex(lv_obj_t *parent,
                                                   int diameter, bool is_large)
{
    lv_obj_t *scale = lv_scale_create(parent);
    lv_obj_set_size(scale, diameter, diameter);
    lv_obj_center(scale);
    lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_INNER);

    int minor_w = is_large ? E46_LG_MINOR_WIDTH  : E46_SM_MINOR_WIDTH;
    int minor_l = is_large ? E46_LG_MINOR_LENGTH  : E46_SM_MINOR_LENGTH;
    int major_w = is_large ? E46_LG_MAJOR_WIDTH  : E46_SM_MAJOR_WIDTH;
    int major_l = is_large ? E46_LG_MAJOR_LENGTH  : E46_SM_MAJOR_LENGTH;

    /* Minor tick marks */
    lv_obj_set_style_line_color(scale, lv_color_hex(E46_TICK_MINOR), LV_PART_ITEMS);
    lv_obj_set_style_line_width(scale, minor_w, LV_PART_ITEMS);
    lv_obj_set_style_length(scale, minor_l, LV_PART_ITEMS);

    /* Major tick marks */
    lv_obj_set_style_line_color(scale, lv_color_hex(E46_TICK_MAJOR), LV_PART_INDICATOR);
    lv_obj_set_style_line_width(scale, major_w, LV_PART_INDICATOR);
    lv_obj_set_style_length(scale, major_l, LV_PART_INDICATOR);

    /* Labels -- larger font for big gauges */
    const lv_font_t *font = is_large ? &lv_font_montserrat_16 : &lv_font_montserrat_12;
    lv_obj_set_style_text_font(scale, font, LV_PART_INDICATOR);
    lv_obj_set_style_text_color(scale, lv_color_hex(E46_LABEL), LV_PART_INDICATOR);

    /* Main arc line (the arc that connects tick marks) -- hidden */
    lv_obj_set_style_arc_color(scale, lv_color_hex(E46_TICK_MINOR), LV_PART_MAIN);
    lv_obj_set_style_arc_width(scale, 0, LV_PART_MAIN);

    return scale;
}

/** Convenience wrapper -- original API, defaults to small gauge. */
static inline lv_obj_t *e46_create_round_scale(lv_obj_t *parent, int diameter)
{
    return e46_create_round_scale_ex(parent, diameter, false);
}

/**
 * Create a needle line for use with lv_scale_set_line_needle_value().
 */
static inline lv_obj_t *e46_create_needle(lv_obj_t *scale, int width)
{
    lv_obj_t *needle = lv_line_create(scale);
    lv_obj_set_style_line_color(needle, lv_color_hex(E46_NEEDLE), 0);
    lv_obj_set_style_line_width(needle, width, 0);
    lv_obj_set_style_line_rounded(needle, true, 0);
    return needle;
}

/**
 * Create the center hub circle over the needle pivot.
 * `is_large` selects large or small hub proportions.
 */
static inline lv_obj_t *e46_create_hub_ex(lv_obj_t *parent, bool is_large)
{
    int dia    = is_large ? E46_LG_HUB_DIA    : E46_SM_HUB_DIA;
    int border = is_large ? E46_LG_HUB_BORDER : E46_SM_HUB_BORDER;

    lv_obj_t *hub = lv_obj_create(parent);
    lv_obj_remove_style_all(hub);
    lv_obj_set_size(hub, dia, dia);
    lv_obj_center(hub);
    lv_obj_set_style_radius(hub, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(hub, lv_color_hex(E46_NEEDLE_HUB), 0);
    lv_obj_set_style_bg_opa(hub, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(hub, lv_color_hex(E46_WHITE), 0);
    lv_obj_set_style_border_width(hub, border, 0);
    lv_obj_remove_flag(hub, LV_OBJ_FLAG_SCROLLABLE);
    return hub;
}

/** Convenience wrapper -- original API with explicit diameter. */
static inline lv_obj_t *e46_create_hub(lv_obj_t *parent, int diameter)
{
    lv_obj_t *hub = lv_obj_create(parent);
    lv_obj_remove_style_all(hub);
    lv_obj_set_size(hub, diameter, diameter);
    lv_obj_center(hub);
    lv_obj_set_style_radius(hub, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(hub, lv_color_hex(E46_NEEDLE_HUB), 0);
    lv_obj_set_style_bg_opa(hub, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(hub, lv_color_hex(E46_WHITE), 0);
    lv_obj_set_style_border_width(hub, 2, 0);
    lv_obj_remove_flag(hub, LV_OBJ_FLAG_SCROLLABLE);
    return hub;
}

/* ── Backlight / amber theme helpers ───────────────────────────────── */

/** Get the primary marking color based on backlight state. */
static inline uint32_t e46_mark_color(bool backlight_on)
{
    return backlight_on ? E46_AMBER : E46_WHITE;
}

/** Get the minor tick color based on backlight state. */
static inline uint32_t e46_minor_color(bool backlight_on)
{
    return backlight_on ? 0xAA5500 : E46_TICK_MINOR;
}

/** Get the dim label color based on backlight state. */
static inline uint32_t e46_dim_color(bool backlight_on)
{
    return backlight_on ? E46_DIM_AMBER : E46_DIM_LABEL;
}

/**
 * Apply the backlight theme to a standard E46 scale + needle + hub.
 * Call this from each skin's update() when the backlight state changes.
 */
static inline void e46_apply_backlight(lv_obj_t *scale, lv_obj_t *needle,
                                       lv_obj_t *hub, bool backlight_on)
{
    uint32_t mark  = e46_mark_color(backlight_on);
    uint32_t minor = e46_minor_color(backlight_on);

    /* Minor ticks */
    lv_obj_set_style_line_color(scale, lv_color_hex(minor), LV_PART_ITEMS);

    /* Major ticks */
    lv_obj_set_style_line_color(scale, lv_color_hex(mark), LV_PART_INDICATOR);

    /* Labels */
    lv_obj_set_style_text_color(scale, lv_color_hex(mark), LV_PART_INDICATOR);

    /* Main arc */
    lv_obj_set_style_arc_color(scale, lv_color_hex(minor), LV_PART_MAIN);

    /* Needle */
    lv_obj_set_style_line_color(needle, lv_color_hex(mark), 0);

    /* Hub border */
    lv_obj_set_style_border_color(hub, lv_color_hex(mark), 0);
}

#ifdef __cplusplus
}
#endif

#endif /* E46_COMMON_H */
