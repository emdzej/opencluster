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

/* SDL2 for screenshot BMP saving */
#include <SDL.h>
#include "src/drivers/sdl/lv_sdl_window.h"
#include "src/draw/snapshot/lv_snapshot.h"

/* External skin declarations */
extern const gauge_skin_t skin_tachometer;
extern const gauge_skin_t skin_speedometer;
extern const gauge_skin_t skin_fuel_gauge;
extern const gauge_skin_t skin_coolant_temp;

/* BMW E46 skins */
extern const gauge_skin_t skin_e46_fuel;
extern const gauge_skin_t skin_e46_speedometer;
extern const gauge_skin_t skin_e46_tach_7k;
extern const gauge_skin_t skin_e46_tach_6k;
extern const gauge_skin_t skin_e46_coolant;

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
    const char *screenshot_path; /* If set, render one frame and save to this path */
    int         screenshot_rpm;
    int         screenshot_speed_x10;
    int         screenshot_fuel;
    int         screenshot_coolant;
    int         screenshot_gear;
    int         screenshot_backlight;
    int         screenshot_consumption_x10;
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
           "                   dual_vertical, quad, e46_cluster)\n"
           "  --slot0 NAME     Skin for slot 0\n"
           "  --slot1 NAME     Skin for slot 1\n"
           "  --slot2 NAME     Skin for slot 2\n"
           "  --slot3 NAME     Skin for slot 3\n"
           "\n"
           "Screenshot mode (render one frame and exit):\n"
           "  --screenshot FILE   Save screenshot as BMP to FILE\n"
           "  --rpm N             RPM value for screenshot (default: 3500)\n"
           "  --speed N           Speed in km/h for screenshot (default: 120)\n"
           "  --fuel N            Fuel level 0-100 (default: 65)\n"
           "  --coolant N         Coolant temp in C (default: 90)\n"
           "  --gear N            Gear 0=N,1-8,255=R (default: 4)\n"
           "  --backlight N       Backlight 0=off,1-255=on (default: 0)\n"
           "  --consumption N     Fuel consumption L/100km*10 (default: 85)\n"
           "\n"
           "Examples:\n"
           "  display_node --skin tachometer --width 240 --height 240\n"
           "  display_node --layout dual_horizontal --slot0 speedometer --slot1 tachometer\n"
           "  display_node --layout quad --slot0 speedometer --slot1 tachometer "
           "--slot2 fuel_gauge --slot3 coolant_temp\n"
           "  display_node --layout e46_cluster --width 800 --height 480\n"
           "  display_node --layout e46_cluster --slot2 e46_tach_6k "
           "--width 800 --height 480\n"
           "  display_node --screenshot e46.bmp --layout e46_cluster "
           "--width 800 --height 480 --rpm 4500 --speed 140\n");
}

static void parse_args(int argc, char **argv, app_config_t *cfg)
{
    cfg->width       = 480;
    cfg->height      = 320;
    cfg->skin_name   = "tachometer";
    cfg->layout_name = NULL;
    cfg->node_id     = 1;
    cfg->screenshot_path = NULL;
    cfg->screenshot_rpm  = 3500;
    cfg->screenshot_speed_x10 = 1200;  /* 120 km/h */
    cfg->screenshot_fuel = 65;
    cfg->screenshot_coolant = 90;
    cfg->screenshot_gear = 4;
    cfg->screenshot_backlight = 0;
    cfg->screenshot_consumption_x10 = 85;  /* 8.5 L/100km */
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
        } else if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) {
            cfg->screenshot_path = argv[++i];
        } else if (strcmp(argv[i], "--rpm") == 0 && i + 1 < argc) {
            cfg->screenshot_rpm = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--speed") == 0 && i + 1 < argc) {
            cfg->screenshot_speed_x10 = atoi(argv[++i]) * 10;
        } else if (strcmp(argv[i], "--fuel") == 0 && i + 1 < argc) {
            cfg->screenshot_fuel = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--coolant") == 0 && i + 1 < argc) {
            cfg->screenshot_coolant = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--gear") == 0 && i + 1 < argc) {
            cfg->screenshot_gear = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--backlight") == 0 && i + 1 < argc) {
            cfg->screenshot_backlight = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--consumption") == 0 && i + 1 < argc) {
            cfg->screenshot_consumption_x10 = atoi(argv[++i]);
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

/**
 * Capture a screenshot using LVGL's snapshot API and save as BMP.
 *
 * lv_snapshot_take() renders the widget tree to an off-screen buffer,
 * bypassing the SDL renderer entirely.  We then wrap the pixel data
 * in an SDL_Surface and save it as BMP.
 */
static int screenshot_save(lv_obj_t *screen, const char *path, int width, int height)
{
    lv_draw_buf_t *snap = lv_snapshot_take(screen, LV_COLOR_FORMAT_ARGB8888);
    if (!snap) {
        fprintf(stderr, "lv_snapshot_take failed\n");
        return -1;
    }

    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(
        snap->data, width, height, 32, snap->header.stride,
        SDL_PIXELFORMAT_ARGB8888);
    if (!surface) {
        fprintf(stderr, "SDL_CreateRGBSurfaceWithFormatFrom failed: %s\n",
                SDL_GetError());
        lv_draw_buf_destroy(snap);
        return -1;
    }

    int rc = SDL_SaveBMP(surface, path);
    SDL_FreeSurface(surface);
    lv_draw_buf_destroy(snap);

    if (rc != 0) {
        fprintf(stderr, "SDL_SaveBMP failed: %s\n", SDL_GetError());
        return -1;
    }

    printf("Screenshot saved to %s\n", path);
    return 0;
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

    /* BMW E46 skins */
    skin_registry_register(&skin_e46_fuel);
    skin_registry_register(&skin_e46_speedometer);
    skin_registry_register(&skin_e46_tach_7k);
    skin_registry_register(&skin_e46_tach_6k);
    skin_registry_register(&skin_e46_coolant);

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
                /* Default skin assignments depend on layout */
                static const char *defaults_generic[] = {
                    "speedometer", "tachometer", "fuel_gauge", "coolant_temp"
                };
                static const char *defaults_e46[] = {
                    "e46_fuel", "e46_speedometer", "e46_tach_7k", "e46_coolant"
                };
                if (strcmp(tmpl->name, "e46_cluster") == 0) {
                    skin_name = defaults_e46[i];
                } else {
                    skin_name = defaults_generic[i];
                }
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

    /* ── Screenshot mode: inject data, render, save, exit ──────── */
    if (cfg.screenshot_path) {
        vehicle_data_t snap = {0};
        snap.rpm                  = (uint16_t)cfg.screenshot_rpm;
        snap.speed_kmh_x10       = (uint16_t)cfg.screenshot_speed_x10;
        snap.fuel_level_pct       = (uint8_t)cfg.screenshot_fuel;
        snap.coolant_temp_c       = (int8_t)cfg.screenshot_coolant;
        snap.gear                 = (uint8_t)cfg.screenshot_gear;
        snap.backlight            = (uint8_t)cfg.screenshot_backlight;
        snap.fuel_consumption_x10 = (uint16_t)cfg.screenshot_consumption_x10;
        snap.engine_flags         = ENG_RUNNING;
        snap.battery_mv           = 14200;
        snap.oil_pressure_psi     = 45;
        snap.throttle_pct         = 35;
        snap.last_update_ms       = hal_tick_ms();

        /* Update the vehicle data store so skins can read it */
        vehicle_data_update(&snap);

        /* Feed data to the layout and pump LVGL several times to
         * ensure all widgets are fully rendered and flushed. */
        for (int frame = 0; frame < 10; frame++) {
            layout_update(&layout_state, &snap);
            lv_timer_handler();
            hal_delay_ms(20);
        }

        int rc = screenshot_save(scr, cfg.screenshot_path,
                                 cfg.width, cfg.height);
        layout_destroy(&layout_state);
        return rc;
    }

    /* ── Normal interactive mode ─────────────────────────────────── */

    /* Initialize CAN */
    if (hal_can_init() != 0) {
        fprintf(stderr, "Warning: CAN init failed, running without CAN data\n");
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
                case CAN_ID_EXTENDED:
                    can_decode_extended(&can_frame, vd);
                    break;
                case CAN_ID_COMMAND:
                    can_decode_command(&can_frame, vd);
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
