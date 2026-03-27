/**
 * @file main.c
 * OpenCluster display node -- desktop entry point.
 *
 * Initializes LVGL with an SDL2 window, connects to the cluster bus
 * (UDP multicast on desktop), and renders gauges using the layout
 * system which supports multiple skins per screen.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>

#include "lvgl.h"
#include "hal_display.h"
#include "hal_input.h"
#include "hal_can.h"
#include "hal_platform.h"
#include "vehicle_data.h"
#include "can_protocol.h"
#include "skin.h"
#include "skin_registry.h"
#include "layout.h"

/* External skin declarations */
extern const gauge_skin_t skin_tachometer;
extern const gauge_skin_t skin_speedometer;
extern const gauge_skin_t skin_fuel_gauge;
extern const gauge_skin_t skin_coolant_temp;

static volatile bool s_running = true;

static void signal_handler(int sig)
{
    (void)sig;
    s_running = false;
}

/* Parse simple command-line arguments */
typedef struct {
    int         width;
    int         height;
    const char *skin_name;      /* Legacy single-skin mode */
    const char *layout_name;    /* Layout mode (overrides skin_name) */
    const char *slot_skins[LAYOUT_MAX_SLOTS];  /* Skin per slot */
    uint8_t     node_id;
} app_config_t;

static void print_usage(void)
{
    printf("Usage: display_node [options]\n"
           "\n"
           "Display options:\n"
           "  --width N        Display width (default: 480)\n"
           "  --height N       Display height (default: 320)\n"
           "  --node-id N      Node ID on cluster bus (default: 1)\n"
           "\n"
           "Single-gauge mode (legacy):\n"
           "  --skin NAME      Gauge skin name (default: tachometer)\n"
           "\n"
           "Multi-gauge mode:\n"
           "  --layout NAME    Layout template (single, dual_horizontal,\n"
           "                   dual_vertical, quad)\n"
           "  --slot0 NAME     Skin for slot 0\n"
           "  --slot1 NAME     Skin for slot 1\n"
           "  --slot2 NAME     Skin for slot 2\n"
           "  --slot3 NAME     Skin for slot 3\n"
           "\n"
           "Examples:\n"
           "  display_node --skin tachometer --width 240 --height 240\n"
           "  display_node --layout dual_horizontal --slot0 speedometer --slot1 tachometer\n"
           "  display_node --layout quad --slot0 speedometer --slot1 tachometer "
           "--slot2 fuel_gauge --slot3 coolant_temp\n");
}

static void parse_args(int argc, char **argv, app_config_t *cfg)
{
    cfg->width       = 480;
    cfg->height      = 320;
    cfg->skin_name   = "tachometer";
    cfg->layout_name = NULL;
    cfg->node_id     = 1;
    for (int i = 0; i < LAYOUT_MAX_SLOTS; i++) {
        cfg->slot_skins[i] = NULL;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            cfg->width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            cfg->height = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--skin") == 0 && i + 1 < argc) {
            cfg->skin_name = argv[++i];
        } else if (strcmp(argv[i], "--layout") == 0 && i + 1 < argc) {
            cfg->layout_name = argv[++i];
        } else if (strcmp(argv[i], "--slot0") == 0 && i + 1 < argc) {
            cfg->slot_skins[0] = argv[++i];
        } else if (strcmp(argv[i], "--slot1") == 0 && i + 1 < argc) {
            cfg->slot_skins[1] = argv[++i];
        } else if (strcmp(argv[i], "--slot2") == 0 && i + 1 < argc) {
            cfg->slot_skins[2] = argv[++i];
        } else if (strcmp(argv[i], "--slot3") == 0 && i + 1 < argc) {
            cfg->slot_skins[3] = argv[++i];
        } else if (strcmp(argv[i], "--node-id") == 0 && i + 1 < argc) {
            cfg->node_id = (uint8_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage();
            exit(0);
        }
    }
}

static void print_available_skins(void)
{
    fprintf(stderr, "Available skins:\n");
    for (int i = 0; i < skin_registry_count(); i++) {
        const gauge_skin_t *s = skin_registry_get(i);
        fprintf(stderr, "  - %s (%s)\n", s->name, s->display_name);
    }
}

static void print_available_layouts(void)
{
    fprintf(stderr, "Available layouts:\n");
    for (int i = 0; i < layout_registry_count(); i++) {
        const layout_template_t *l = layout_registry_get(i);
        fprintf(stderr, "  - %s (%d slots)\n", l->name, l->slot_count);
    }
}

int main(int argc, char **argv)
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    app_config_t cfg;
    parse_args(argc, argv, &cfg);

    /* Register skins */
    skin_registry_register(&skin_tachometer);
    skin_registry_register(&skin_speedometer);
    skin_registry_register(&skin_fuel_gauge);
    skin_registry_register(&skin_coolant_temp);

    /* Register built-in layouts */
    layout_register_builtins();

    /* Initialize LVGL */
    lv_init();

    /* Initialize display via HAL */
    lv_display_t *disp = hal_display_init(cfg.width, cfg.height);
    if (!disp) {
        fprintf(stderr, "Failed to initialize display\n");
        return 1;
    }

    /* Initialize input */
    hal_input_init(disp);

    /* Initialize vehicle data store */
    vehicle_data_init();

    /* Initialize CAN */
    if (hal_can_init() != 0) {
        fprintf(stderr, "Warning: CAN init failed, running without CAN data\n");
    }

    /* Set screen background to black */
    lv_obj_t *scr = lv_display_get_screen_active(disp);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    layout_state_t layout_state;
    memset(&layout_state, 0, sizeof(layout_state));

    if (cfg.layout_name) {
        /* ── Multi-gauge layout mode ────────────────────────────── */
        const layout_template_t *tmpl = layout_registry_find(cfg.layout_name);
        if (!tmpl) {
            fprintf(stderr, "Unknown layout '%s'.\n", cfg.layout_name);
            print_available_layouts();
            return 1;
        }

        if (layout_init(&layout_state, tmpl, scr, cfg.width, cfg.height) != 0) {
            fprintf(stderr, "Failed to initialize layout\n");
            return 1;
        }

        /* Assign skins to slots */
        for (int i = 0; i < tmpl->slot_count; i++) {
            const char *skin_name = cfg.slot_skins[i];
            if (!skin_name) {
                /* Default skin assignments if not specified */
                static const char *defaults[] = {
                    "speedometer", "tachometer", "fuel_gauge", "coolant_temp"
                };
                skin_name = defaults[i];
            }

            const gauge_skin_t *skin = skin_registry_find(skin_name);
            if (!skin) {
                fprintf(stderr, "Unknown skin '%s' for slot %d.\n",
                        skin_name, i);
                print_available_skins();
                return 1;
            }

            if (layout_set_skin(&layout_state, i, skin) != 0) {
                fprintf(stderr, "Failed to create skin '%s' in slot %d\n",
                        skin_name, i);
                return 1;
            }
        }

        /* Set window title */
        char title[128];
        snprintf(title, sizeof(title), "OpenCluster - %s [Node %d]",
                 tmpl->name, cfg.node_id);
        hal_display_set_title(disp, title);

        printf("OpenCluster display node started (layout mode)\n");
        printf("  Display: %dx%d\n", cfg.width, cfg.height);
        printf("  Layout:  %s (%d slots)\n", tmpl->name, tmpl->slot_count);
        for (int i = 0; i < tmpl->slot_count; i++) {
            const gauge_skin_t *s = layout_state.slots[i].skin;
            printf("  Slot %d:  %s (%s)\n", i,
                   s ? s->name : "(empty)",
                   s ? s->display_name : "");
        }
        printf("  Node ID: %d\n", cfg.node_id);
        printf("  CAN:     UDP multicast %s:%d\n", "224.0.0.100", 4200);
    } else {
        /* ── Legacy single-skin mode ────────────────────────────── */
        const gauge_skin_t *skin = skin_registry_find(cfg.skin_name);
        if (!skin) {
            fprintf(stderr, "Unknown skin '%s'.\n", cfg.skin_name);
            print_available_skins();
            return 1;
        }

        /* Use single layout with the requested skin */
        if (layout_init(&layout_state, &layout_single, scr,
                        cfg.width, cfg.height) != 0) {
            fprintf(stderr, "Failed to initialize layout\n");
            return 1;
        }
        if (layout_set_skin(&layout_state, 0, skin) != 0) {
            fprintf(stderr, "Failed to create skin '%s'\n", skin->name);
            return 1;
        }

        /* Set window title */
        char title[128];
        snprintf(title, sizeof(title), "OpenCluster - %s [Node %d]",
                 skin->display_name, cfg.node_id);
        hal_display_set_title(disp, title);

        printf("OpenCluster display node started\n");
        printf("  Display: %dx%d\n", cfg.width, cfg.height);
        printf("  Skin:    %s (%s)\n", skin->name, skin->display_name);
        printf("  Node ID: %d\n", cfg.node_id);
        printf("  CAN:     UDP multicast %s:%d\n", "224.0.0.100", 4200);
    }

    /* Main loop */
    vehicle_data_t local_data;
    can_frame_t can_frame;

    while (s_running) {
        /* Poll for CAN frames (non-blocking) */
        while (hal_can_receive(&can_frame, 0) == 0) {
            vehicle_data_lock();
            vehicle_data_t *vd = (vehicle_data_t *)vehicle_data_get_ptr();
            switch (can_frame.id) {
                case CAN_ID_ENGINE:
                    can_decode_engine(&can_frame, vd);
                    break;
                case CAN_ID_DRIVETRAIN:
                    can_decode_drivetrain(&can_frame, vd);
                    break;
                case CAN_ID_FUEL_ELEC:
                    can_decode_fuel_elec(&can_frame, vd);
                    break;
                default:
                    break;
            }
            vd->last_update_ms = hal_tick_ms();
            vehicle_data_unlock();
        }

        /* Get vehicle data snapshot and update all gauges */
        vehicle_data_get(&local_data);
        layout_update(&layout_state, &local_data);

        /* Run LVGL timer handler */
        lv_timer_handler();

        /* Sleep ~5ms to yield CPU */
        hal_delay_ms(5);
    }

    /* Cleanup */
    layout_destroy(&layout_state);
    hal_can_deinit();
    printf("Display node shut down.\n");

    return 0;
}
