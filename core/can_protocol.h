/**
 * @file can_protocol.h
 * Cluster bus CAN message encoding/decoding.
 *
 * Defines CAN IDs and provides inline encode/decode functions
 * for the cluster bus protocol (see CLUSTER_BUS_SPEC.md).
 */

#ifndef CAN_PROTOCOL_H
#define CAN_PROTOCOL_H

#include "hal_can.h"
#include "vehicle_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- CAN ID assignments ---- */
#define CAN_ID_ENGINE       0x100
#define CAN_ID_DRIVETRAIN   0x101
#define CAN_ID_FUEL_ELEC    0x102
#define CAN_ID_COMMAND      0x200
#define CAN_ID_HEARTBEAT    0x300

/* ---- Encoding (vehicle_data_t -> CAN frames) ---- */

static inline void can_encode_engine(const vehicle_data_t *d, can_frame_t *f)
{
    f->id  = CAN_ID_ENGINE;
    f->len = 8;
    f->data[0] = (uint8_t)(d->rpm >> 8);
    f->data[1] = (uint8_t)(d->rpm & 0xFF);
    f->data[2] = d->throttle_pct;
    f->data[3] = (uint8_t)d->coolant_temp_c;
    f->data[4] = d->oil_pressure_psi;
    f->data[5] = d->engine_flags;
    f->data[6] = 0;
    f->data[7] = 0;
}

static inline void can_encode_drivetrain(const vehicle_data_t *d, can_frame_t *f)
{
    f->id  = CAN_ID_DRIVETRAIN;
    f->len = 8;
    f->data[0] = (uint8_t)(d->speed_kmh_x10 >> 8);
    f->data[1] = (uint8_t)(d->speed_kmh_x10 & 0xFF);
    f->data[2] = d->gear;
    f->data[3] = (uint8_t)((d->odometer_km >> 16) & 0xFF);
    f->data[4] = (uint8_t)((d->odometer_km >> 8) & 0xFF);
    f->data[5] = (uint8_t)(d->odometer_km & 0xFF);
    f->data[6] = 0;
    f->data[7] = 0;
}

static inline void can_encode_fuel_elec(const vehicle_data_t *d, can_frame_t *f)
{
    f->id  = CAN_ID_FUEL_ELEC;
    f->len = 8;
    f->data[0] = d->fuel_level_pct;
    f->data[1] = (uint8_t)(d->battery_mv >> 8);
    f->data[2] = (uint8_t)(d->battery_mv & 0xFF);
    f->data[3] = (uint8_t)(d->warning_flags >> 8);
    f->data[4] = (uint8_t)(d->warning_flags & 0xFF);
    f->data[5] = 0;
    f->data[6] = 0;
    f->data[7] = 0;
}

/* ---- Decoding (CAN frame -> vehicle_data_t fields) ---- */

static inline void can_decode_engine(const can_frame_t *f, vehicle_data_t *d)
{
    d->rpm             = (uint16_t)((f->data[0] << 8) | f->data[1]);
    d->throttle_pct    = f->data[2];
    d->coolant_temp_c  = (int8_t)f->data[3];
    d->oil_pressure_psi = f->data[4];
    d->engine_flags    = f->data[5];
}

static inline void can_decode_drivetrain(const can_frame_t *f, vehicle_data_t *d)
{
    d->speed_kmh_x10 = (uint16_t)((f->data[0] << 8) | f->data[1]);
    d->gear          = f->data[2];
    d->odometer_km   = (uint32_t)((f->data[3] << 16) | (f->data[4] << 8) | f->data[5]);
}

static inline void can_decode_fuel_elec(const can_frame_t *f, vehicle_data_t *d)
{
    d->fuel_level_pct = f->data[0];
    d->battery_mv     = (uint16_t)((f->data[1] << 8) | f->data[2]);
    d->warning_flags  = (uint16_t)((f->data[3] << 8) | f->data[4]);
}

#ifdef __cplusplus
}
#endif

#endif /* CAN_PROTOCOL_H */
