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
#include "e46_can_protocol.h"

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

/* ---- E46 simulation helpers ---- */

static void sim_to_e46_dme1(const sim_state_t *sim, e46_dme1_t *d)
{
    memset(d, 0, sizeof(*d));
    d->ignition_on          = true;
    d->crankshaft_error     = false;
    d->traction_ack         = true;
    d->gear_change_ok       = true;
    d->charge_state         = E46_CHARGE_OK;
    d->maf_error            = false;
    d->torque_request_pct   = sim->throttle * 0.8f;
    d->engine_rpm           = sim->rpm;
    d->torque_indicated_pct = sim->throttle * 0.75f;
    d->torque_loss_pct      = 15.0f + sim->rpm * 0.001f;
    d->torque_after_charge_pct = sim->throttle * 0.7f;
}

static void sim_to_e46_dme2(const sim_state_t *sim, e46_dme2_t *d)
{
    memset(d, 0, sizeof(*d));
    d->mux_code             = E46_MUX_CAN_LEVEL;
    d->mux_info             = 0x11;
    d->coolant_temp_c       = sim->coolant_temp;
    d->ambient_pressure_hpa = 1013;
    d->engine_running       = (sim->rpm > 400.0f);
    d->accelerator_pct      = sim->throttle;
    d->tps_virtual_pct      = sim->throttle;
    d->cruise_switch        = E46_CRU_SW_NONE;
    d->cruise_state         = E46_CRU_INACTIVE;
}

static void sim_to_e46_dme4(const sim_state_t *sim, e46_dme4_t *d)
{
    memset(d, 0, sizeof(*d));
    d->check_engine         = false;
    d->eml_light            = false;
    d->fuel_cap_light       = false;
    d->fuel_consumption_raw = (uint16_t)(sim->fuel_consumption * 10.0f);
    d->oil_temp_c           = (int16_t)(sim->coolant_temp - 5.0f);
    d->coolant_overheat     = (sim->coolant_temp > 120.0f);
    d->warmup_leds          = (sim->coolant_temp < 60.0f) ? 7 :
                              (sim->coolant_temp < 80.0f) ? 3 : 0;
    d->oil_level_l          = 0.0f;
    d->oil_pressure_low     = false;
}

static void sim_to_e46_asc1(const sim_state_t *sim, e46_asc1_t *d, uint8_t *alive)
{
    memset(d, 0, sizeof(*d));
    d->vehicle_speed_kmh            = sim->speed;
    d->torque_intervention_asc_pct  = 99.6f;  /* no reduction */
    d->torque_intervention_msr_pct  = 0.0f;   /* no increase */
    d->torque_intervention_asc_lm_pct = 99.6f;
    d->brake_light_switch           = false;
    d->alive_counter                = (*alive) & 0x0F;
    (*alive)++;
}

static void sim_to_e46_icl2(const sim_state_t *sim, e46_icl2_t *d, uint16_t *clock_min)
{
    memset(d, 0, sizeof(*d));
    d->odometer_km       = 42000;
    d->fuel_tank_level   = (uint8_t)sim->fuel_level;
    d->fuel_reserve      = (sim->fuel_level < 15.0f);
    d->running_clock_min = *clock_min;
    d->fuel_level_driver = (uint8_t)(sim->fuel_level * 0.6f);
}

static int run_e46_simulator(void)
{
    printf("OpenCluster E46 CAN simulator started (BMW 500kb/s bus)\n");
    printf("  Sending: DME1(0x316) DME2(0x329) DME4(0x545) ASC1(0x153) ICL2(0x613)\n");
    printf("  Press Ctrl+C to stop\n\n");

    sim_state_t sim;
    sim_init(&sim);
    can_frame_t frame;

    uint32_t last_dme1_ms = 0;   /* 10ms */
    uint32_t last_dme2_ms = 0;   /* 10ms */
    uint32_t last_dme4_ms = 0;   /* 10ms */
    uint32_t last_asc1_ms = 0;   /* 10ms */
    uint32_t last_icl2_ms = 0;   /* 200ms */
    uint32_t last_print_ms = 0;
    uint32_t start_ms = hal_tick_ms();

    uint8_t  asc_alive = 0;
    uint16_t clock_min = 0;

    while (s_running) {
        uint32_t now = hal_tick_ms();
        float dt = 0.010f;  /* 10ms step to match E46 bus timing */

        sim_step(&sim, dt);

        /* DME1 0x316 at 100Hz (10ms) */
        if (now - last_dme1_ms >= 10) {
            e46_dme1_t dme1;
            sim_to_e46_dme1(&sim, &dme1);
            e46_encode_dme1(&dme1, &frame);
            hal_can_send(&frame);
            last_dme1_ms = now;
        }

        /* DME2 0x329 at 100Hz (10ms) */
        if (now - last_dme2_ms >= 10) {
            e46_dme2_t dme2;
            sim_to_e46_dme2(&sim, &dme2);
            e46_encode_dme2(&dme2, &frame);
            hal_can_send(&frame);
            last_dme2_ms = now;
        }

        /* DME4 0x545 at 100Hz (10ms) */
        if (now - last_dme4_ms >= 10) {
            e46_dme4_t dme4;
            sim_to_e46_dme4(&sim, &dme4);
            e46_encode_dme4(&dme4, &frame);
            hal_can_send(&frame);
            last_dme4_ms = now;
        }

        /* ASC1 0x153 at 100Hz (10ms) */
        if (now - last_asc1_ms >= 10) {
            e46_asc1_t asc1;
            sim_to_e46_asc1(&sim, &asc1, &asc_alive);
            e46_encode_asc1(&asc1, &frame);
            hal_can_send(&frame);
            last_asc1_ms = now;
        }

        /* ICL2 0x613 at 5Hz (200ms) */
        if (now - last_icl2_ms >= 200) {
            e46_icl2_t icl2;
            sim_to_e46_icl2(&sim, &icl2, &clock_min);
            e46_encode_icl2(&icl2, &frame);
            hal_can_send(&frame);
            last_icl2_ms = now;
            /* Advance running clock every 60 * 200ms = 12s of sim time */
            if (((now - start_ms) / 60000) > clock_min)
                clock_min = (uint16_t)((now - start_ms) / 60000);
        }

        /* Print status every 2 seconds */
        if (now - last_print_ms >= 2000) {
            float elapsed = (float)(now - start_ms) / 1000.0f;
            printf("[%6.1fs] RPM: %5.0f | Speed: %5.1f km/h | "
                   "Temp: %4.1f C | Throttle: %4.1f%% | Fuel: %4.1f%%\n",
                   elapsed,
                   sim.rpm,
                   sim.speed,
                   sim.coolant_temp,
                   sim.throttle,
                   sim.fuel_level);
            last_print_ms = now;
        }

        hal_delay_ms(10);
    }

    return 0;
}

static int run_default_simulator(void)
{
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
        float dt = 0.020f;

        sim_step(&sim, dt);
        sim_to_vehicle_data(&sim, &vd);

        if (now - last_engine_ms >= 50) {
            can_encode_engine(&vd, &frame);
            hal_can_send(&frame);
            last_engine_ms = now;
        }

        if (now - last_drive_ms >= 50) {
            can_encode_drivetrain(&vd, &frame);
            hal_can_send(&frame);
            last_drive_ms = now;
        }

        if (now - last_fuel_ms >= 200) {
            can_encode_fuel_elec(&vd, &frame);
            hal_can_send(&frame);
            can_encode_extended(&vd, &frame);
            hal_can_send(&frame);
            can_encode_command(&vd, &frame);
            hal_can_send(&frame);
            last_fuel_ms = now;
        }

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

    return 0;
}

int main(int argc, char **argv)
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    bool e46_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--e46") == 0 || strcmp(argv[i], "-e46") == 0) {
            e46_mode = true;
        }
    }

    if (hal_can_init() != 0) {
        fprintf(stderr, "Failed to initialize CAN transport\n");
        return 1;
    }

    int ret;
    if (e46_mode) {
        ret = run_e46_simulator();
    } else {
        ret = run_default_simulator();
    }

    hal_can_deinit();
    printf("\nCAN simulator stopped.\n");

    return ret;
}
