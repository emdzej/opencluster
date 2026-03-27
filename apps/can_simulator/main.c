/**
 * @file main.c
 * OpenCluster CAN simulator -- broadcasts fake vehicle data.
 *
 * Generates realistic-looking vehicle data (RPM ramps, speed oscillation,
 * temperature changes) and broadcasts it on the cluster bus via UDP multicast.
 * This simulates the master node for desktop development.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <math.h>

#include "hal_can.h"
#include "hal_platform.h"
#include "vehicle_data.h"
#include "can_protocol.h"

static volatile bool s_running = true;

static void signal_handler(int sig)
{
    (void)sig;
    s_running = false;
}

/**
 * Simulated vehicle state -- evolves over time to produce
 * realistic-looking gauge data.
 */
typedef struct {
    float rpm;
    float speed;
    float coolant_temp;
    float fuel_level;
    float throttle;
    float fuel_consumption; /* L/100km */
    int   gear;
    bool  backlight;        /* Instrument backlight on/off */
    float time_s;
} sim_state_t;

static void sim_init(sim_state_t *sim)
{
    sim->rpm              = 800.0f;    /* idle */
    sim->speed            = 0.0f;
    sim->coolant_temp     = 20.0f;    /* cold start */
    sim->fuel_level       = 75.0f;
    sim->throttle         = 0.0f;
    sim->fuel_consumption = 0.0f;
    sim->gear             = 0;        /* neutral */
    sim->backlight        = false;    /* headlights off at start */
    sim->time_s           = 0.0f;
}

/**
 * Advance the simulation by dt seconds.
 * Produces a repeating drive cycle: idle -> accelerate -> cruise -> decelerate.
 */
static void sim_step(sim_state_t *sim, float dt)
{
    sim->time_s += dt;

    /* 30-second drive cycle */
    float cycle = fmodf(sim->time_s, 30.0f);

    if (cycle < 2.0f) {
        /* Idle */
        sim->throttle = 0.0f;
        sim->gear     = 0;
    } else if (cycle < 10.0f) {
        /* Accelerate through gears */
        float t = (cycle - 2.0f) / 8.0f;  /* 0..1 */
        sim->throttle = 60.0f + 30.0f * sinf(t * 3.14159f);
        sim->gear     = 1 + (int)(t * 5.0f);
        if (sim->gear > 5) sim->gear = 5;
    } else if (cycle < 20.0f) {
        /* Cruise */
        sim->throttle = 25.0f + 5.0f * sinf(sim->time_s * 0.5f);
        sim->gear     = 5;
    } else if (cycle < 28.0f) {
        /* Decelerate */
        float t = (cycle - 20.0f) / 8.0f;  /* 0..1 */
        sim->throttle = 25.0f * (1.0f - t);
        sim->gear     = 5 - (int)(t * 4.0f);
        if (sim->gear < 1) sim->gear = 1;
    } else {
        /* Coast to idle */
        sim->throttle = 0.0f;
        sim->gear     = 0;
    }

    /* RPM model: base idle + throttle contribution */
    float target_rpm = 800.0f + sim->throttle * 80.0f;
    if (sim->gear > 0) {
        target_rpm = 1500.0f + sim->throttle * (100.0f - (float)sim->gear * 8.0f);
    }
    sim->rpm += (target_rpm - sim->rpm) * dt * 3.0f;
    if (sim->rpm < 700.0f) sim->rpm = 700.0f;
    if (sim->rpm > 8000.0f) sim->rpm = 8000.0f;

    /* Speed model: loosely coupled to RPM and gear */
    float target_speed = 0.0f;
    if (sim->gear > 0) {
        target_speed = (sim->rpm / 8000.0f) * (float)sim->gear * 55.0f;
    }
    sim->speed += (target_speed - sim->speed) * dt * 1.5f;
    if (sim->speed < 0.0f) sim->speed = 0.0f;
    if (sim->speed > 260.0f) sim->speed = 260.0f;

    /* Coolant temperature: slowly warms up, stabilizes around 90C */
    float target_temp = 90.0f + sim->throttle * 0.1f;
    sim->coolant_temp += (target_temp - sim->coolant_temp) * dt * 0.05f;

    /* Fuel: very slowly decreases */
    sim->fuel_level -= dt * 0.02f;
    if (sim->fuel_level < 0.0f) sim->fuel_level = 100.0f;  /* refill */

    /* Fuel consumption model (L/100km):
     * Idle: ~1.5 L/100km equivalent, cruising: ~7-8, hard accel: ~15-25 */
    if (sim->speed > 5.0f) {
        /* Consumption proportional to throttle and inversely to speed */
        float base = 5.0f + sim->throttle * 0.2f;
        float speed_factor = 80.0f / fmaxf(sim->speed, 20.0f);
        sim->fuel_consumption += (base * speed_factor - sim->fuel_consumption) * dt * 2.0f;
    } else {
        /* At standstill, show ~0 or very high if engine running */
        sim->fuel_consumption += (0.0f - sim->fuel_consumption) * dt * 2.0f;
    }
    if (sim->fuel_consumption < 0.0f) sim->fuel_consumption = 0.0f;
    if (sim->fuel_consumption > 30.0f) sim->fuel_consumption = 30.0f;

    /* Backlight: off for first 5 seconds, then toggles every 15s for demo */
    if (sim->time_s < 5.0f) {
        sim->backlight = false;
    } else {
        /* Toggle every 15 seconds after initial 5s delay */
        int period = (int)((sim->time_s - 5.0f) / 15.0f);
        sim->backlight = (period % 2 == 0);
    }
}

static void sim_to_vehicle_data(const sim_state_t *sim, vehicle_data_t *vd)
{
    vd->rpm             = (uint16_t)sim->rpm;
    vd->throttle_pct    = (uint8_t)sim->throttle;
    vd->coolant_temp_c  = (int8_t)sim->coolant_temp;
    vd->oil_pressure_psi = 40 + (uint8_t)(sim->throttle * 0.3f);
    vd->engine_flags    = ENG_RUNNING;
    vd->speed_kmh_x10   = (uint16_t)(sim->speed * 10.0f);
    vd->gear            = (uint8_t)sim->gear;
    vd->odometer_km     = 42000;
    vd->fuel_level_pct  = (uint8_t)sim->fuel_level;
    vd->battery_mv      = 13800;
    vd->warning_flags   = 0;
    vd->fuel_consumption_x10 = (uint16_t)(sim->fuel_consumption * 10.0f);
    vd->ambient_temp_c  = 22;
    vd->backlight       = sim->backlight ? 255 : 0;

    /* Trigger low fuel warning below 15% */
    if (sim->fuel_level < 15.0f) {
        vd->warning_flags |= WARN_LOW_FUEL;
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (hal_can_init() != 0) {
        fprintf(stderr, "Failed to initialize CAN transport\n");
        return 1;
    }

    printf("OpenCluster CAN simulator started\n");
    printf("  Broadcasting on UDP multicast 224.0.0.100:4200\n");
    printf("  Press Ctrl+C to stop\n\n");

    sim_state_t sim;
    sim_init(&sim);

    vehicle_data_t vd;
    can_frame_t frame;

    uint32_t last_engine_ms = 0;
    uint32_t last_drive_ms  = 0;
    uint32_t last_fuel_ms   = 0;
    uint32_t last_print_ms  = 0;

    uint32_t start_ms = hal_tick_ms();

    while (s_running) {
        uint32_t now = hal_tick_ms();
        float dt = 0.020f;  /* 20ms step */

        sim_step(&sim, dt);
        sim_to_vehicle_data(&sim, &vd);

        /* Broadcast engine data at 20 Hz (50ms) */
        if (now - last_engine_ms >= 50) {
            can_encode_engine(&vd, &frame);
            hal_can_send(&frame);
            last_engine_ms = now;
        }

        /* Broadcast drivetrain data at 20 Hz (50ms) */
        if (now - last_drive_ms >= 50) {
            can_encode_drivetrain(&vd, &frame);
            hal_can_send(&frame);
            last_drive_ms = now;
        }

        /* Broadcast fuel/electrical at 5 Hz (200ms) */
        if (now - last_fuel_ms >= 200) {
            can_encode_fuel_elec(&vd, &frame);
            hal_can_send(&frame);
            can_encode_extended(&vd, &frame);
            hal_can_send(&frame);
            can_encode_command(&vd, &frame);
            hal_can_send(&frame);
            last_fuel_ms = now;
        }

        /* Print status every 2 seconds */
        if (now - last_print_ms >= 2000) {
            float elapsed = (float)(now - start_ms) / 1000.0f;
            printf("[%6.1fs] RPM: %5d | Speed: %5.1f km/h | Gear: %d | "
                   "Temp: %4.1f C | Fuel: %4.1f%% | BL: %s\n",
                   elapsed,
                   vd.rpm,
                   (float)vd.speed_kmh_x10 / 10.0f,
                   vd.gear,
                   (float)vd.coolant_temp_c,
                   (float)vd.fuel_level_pct,
                   vd.backlight ? "ON" : "OFF");
            last_print_ms = now;
        }

        hal_delay_ms(20);
    }

    hal_can_deinit();
    printf("\nCAN simulator stopped.\n");

    return 0;
}
