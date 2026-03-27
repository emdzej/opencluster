/**
 * @file main.c
 * OpenCluster display node -- desktop entry point.
 *
 * Initializes LVGL with an SDL2 window, connects to the cluster bus
 * (UDP multicast on desktop), and renders gauges using the selected skin.
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

/* External skin declarations */
extern const gauge_skin_t skin_tachometer;
extern const gauge_skin_t skin_speedometer;

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
    const char *skin_name;
    uint8_t     node_id;
} app_config_t;

static void parse_args(int argc, char **argv, app_config_t *cfg)
{
    cfg->width     = 480;
    cfg->height    = 320;
    cfg->skin_name = "tachometer";
    cfg->node_id   = 1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            cfg->width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            cfg->height = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--skin") == 0 && i + 1 < argc) {
            cfg->skin_name = argv[++i];
        } else if (strcmp(argv[i], "--node-id") == 0 && i + 1 < argc) {
            cfg->node_id = (uint8_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: display_node [options]\n"
                   "  --width N      Display width (default: 480)\n"
                   "  --height N     Display height (default: 320)\n"
                   "  --skin NAME    Gauge skin (default: tachometer)\n"
                   "  --node-id N    Node ID on cluster bus (default: 1)\n");
            exit(0);
        }
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

    /* Find the requested skin */
    const gauge_skin_t *skin = skin_registry_find(cfg.skin_name);
    if (!skin) {
        fprintf(stderr, "Unknown skin '%s'. Available skins:\n", cfg.skin_name);
        for (int i = 0; i < skin_registry_count(); i++) {
            const gauge_skin_t *s = skin_registry_get(i);
            fprintf(stderr, "  - %s (%s)\n", s->name, s->display_name);
        }
        return 1;
    }

    /* Initialize LVGL */
    lv_init();

    /* Initialize display via HAL */
    lv_display_t *disp = hal_display_init(cfg.width, cfg.height);
    if (!disp) {
        fprintf(stderr, "Failed to initialize display\n");
        return 1;
    }

    /* Set window title */
    char title[128];
    snprintf(title, sizeof(title), "OpenCluster - %s [Node %d]",
             skin->display_name, cfg.node_id);
    hal_display_set_title(disp, title);

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

    /* Create the gauge skin */
    void *skin_ctx = skin->create(scr, cfg.width, cfg.height);
    if (!skin_ctx) {
        fprintf(stderr, "Failed to create skin '%s'\n", skin->name);
        return 1;
    }

    printf("OpenCluster display node started\n");
    printf("  Display: %dx%d\n", cfg.width, cfg.height);
    printf("  Skin:    %s (%s)\n", skin->name, skin->display_name);
    printf("  Node ID: %d\n", cfg.node_id);
    printf("  CAN:     UDP multicast %s:%d\n",
           "224.0.0.100", 4200);

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

        /* Get vehicle data snapshot and update skin */
        vehicle_data_get(&local_data);
        skin->update(skin_ctx, &local_data);

        /* Run LVGL timer handler */
        lv_timer_handler();

        /* Sleep ~5ms to yield CPU */
        hal_delay_ms(5);
    }

    /* Cleanup */
    skin->destroy(skin_ctx);
    hal_can_deinit();
    printf("Display node shut down.\n");

    return 0;
}
