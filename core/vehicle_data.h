/**
 * @file vehicle_data.h
 * OpenCluster vehicle data model.
 *
 * Central data struct holding all gauge values. Updated by the CAN
 * receiver, read by the LVGL render loop each frame.
 */

#ifndef VEHICLE_DATA_H
#define VEHICLE_DATA_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Warning flag bits (matches CAN ID 0x102 bytes 3-4). */
enum {
    WARN_LOW_FUEL     = (1 << 0),
    WARN_LOW_BATTERY  = (1 << 1),
    WARN_BRAKE        = (1 << 2),
    WARN_ABS          = (1 << 3),
    WARN_AIRBAG       = (1 << 4),
    WARN_DOOR_OPEN    = (1 << 5),
    WARN_SEATBELT     = (1 << 6),
    WARN_HIGH_BEAM    = (1 << 7),
    WARN_TURN_LEFT    = (1 << 8),
    WARN_TURN_RIGHT   = (1 << 9),
    WARN_PARK_BRAKE   = (1 << 10),
};

/** Engine flags (matches CAN ID 0x100 byte 5). */
enum {
    ENG_RUNNING       = (1 << 0),
    ENG_CHECK_ENGINE  = (1 << 1),
    ENG_OIL_WARN      = (1 << 2),
    ENG_OVERHEAT      = (1 << 3),
};

/**
 * Vehicle data snapshot.  All values are in the units specified
 * in CLUSTER_BUS_SPEC.md.
 */
typedef struct {
    /* Engine -- CAN ID 0x100 */
    uint16_t rpm;              /**< 0-16000 */
    uint8_t  throttle_pct;     /**< 0-100 */
    int8_t   coolant_temp_c;   /**< -40 to 215 */
    uint8_t  oil_pressure_psi; /**< 0-255 */
    uint8_t  engine_flags;     /**< Bitmask, see ENG_* */

    /* Drivetrain -- CAN ID 0x101 */
    uint16_t speed_kmh_x10;    /**< 0-3000 (speed * 10) */
    uint8_t  gear;             /**< 0=N, 1-8=fwd, 255=R */
    uint32_t odometer_km;      /**< Total km (24-bit in CAN) */

    /* Fuel / Electrical -- CAN ID 0x102 */
    uint8_t  fuel_level_pct;   /**< 0-100 */
    uint16_t battery_mv;       /**< 0-20000 millivolts */
    uint16_t warning_flags;    /**< Bitmask, see WARN_* */

    /* Metadata */
    uint32_t last_update_ms;   /**< Timestamp of last CAN update */
} vehicle_data_t;

/**
 * Initialize the vehicle data store (creates mutex, zeroes data).
 */
void vehicle_data_init(void);

/**
 * Get a thread-safe copy of the current vehicle data.
 */
void vehicle_data_get(vehicle_data_t *out);

/**
 * Get a direct pointer for read-only access. The caller must
 * hold the lock (use vehicle_data_lock/unlock).
 */
const vehicle_data_t *vehicle_data_get_ptr(void);

/** Lock the vehicle data mutex. */
void vehicle_data_lock(void);

/** Unlock the vehicle data mutex. */
void vehicle_data_unlock(void);

/**
 * Update the vehicle data store (thread-safe).
 * Typically called from the CAN receiver task.
 */
void vehicle_data_update(const vehicle_data_t *new_data);

#ifdef __cplusplus
}
#endif

#endif /* VEHICLE_DATA_H */
