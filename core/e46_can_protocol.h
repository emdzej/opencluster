/**
 * @file e46_can_protocol.h
 * BMW E46 CAN bus protocol (500 kbit/s) - MS43 ECU message definitions.
 *
 * Covers all DME-sent messages (0x316, 0x329, 0x338, 0x545) and
 * received messages (0x153 ASC1, 0x1F3 ASC3, 0x613 ICL2, 0x610 ICL1).
 *
 * Reference: https://www.ms4x.net/index.php?title=Siemens_MS43_CAN_Bus
 */

#ifndef E46_CAN_PROTOCOL_H
#define E46_CAN_PROTOCOL_H

#include "hal_can.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * CAN Arbitration IDs
 * ======================================================================== */

/* DME (Engine Control) - Sent by ECU */
#define E46_CAN_ID_DME1         0x316   /**< 10ms - Engine torque, RPM, status */
#define E46_CAN_ID_DME2         0x329   /**< 10ms - Temp, pedal, cruise, mux */
#define E46_CAN_ID_DME3         0x338   /**< 1000ms - Sport button (Alpina) */
#define E46_CAN_ID_DME4         0x545   /**< 10ms - Lights, fuel, oil, cluster */

/* ASC (Traction Control) - Received by ECU */
#define E46_CAN_ID_ASC1         0x153   /**< 10ms ASC / 20ms DSC */
#define E46_CAN_ID_ASC3         0x1F3   /**< 20ms DSC */

/* ICL (Instrument Cluster) */
#define E46_CAN_ID_ICL1         0x610   /**< Remote frame - VIN */
#define E46_CAN_ID_ICL2         0x613   /**< 200ms - Odometer, fuel, clock */
#define E46_CAN_ID_ICL3         0x615   /**< 200ms - AC, ambient temp, lighting, speed */
#define E46_CAN_ID_AMT1         0x43D   /**< 10ms - SMG gear, shift, clutch, torque */

/* ========================================================================
 * DME1 0x316 - Engine Status & Torque (10ms)
 * ======================================================================== */

/** Charge intervention state (SF_TQD, bits 4-5 of byte 0). */
typedef enum {
    E46_CHARGE_OK           = 0,  /**< Intervention can be performed fully */
    E46_CHARGE_IGA_LIMIT    = 1,  /**< IGA retard limit reached */
    E46_CHARGE_ACTUATOR_MAX = 2,  /**< Charge actuators fully closed */
    E46_CHARGE_LIM_DYNAMICS = 3,  /**< Limited vehicle dynamics (MTC/ISA err) */
} e46_charge_state_t;

typedef struct {
    /* Byte 0 - Bitfield */
    bool    ignition_on;            /**< LV_SWI_IGK: terminal 15 state */
    bool    crankshaft_error;       /**< LV_F_N_ENG: CRK sensor error */
    bool    traction_ack;           /**< LV_ACK_TCS: ASC1 received OK */
    bool    gear_change_ok;         /**< LV_ERR_GC: gear change possible */
    e46_charge_state_t charge_state;/**< SF_TQD: charge intervention state */
    bool    maf_error;              /**< LV_F_SUB_TQI: MAF sensor error */

    /* Byte 1 */
    float   torque_request_pct;     /**< TQI_TQR_CAN: indexed torque % of C_TQ_STND */

    /* Bytes 2-3 */
    float   engine_rpm;             /**< N_ENG: engine speed in RPM */

    /* Byte 4 */
    float   torque_indicated_pct;   /**< TQI_CAN: indicated torque % */

    /* Byte 5 */
    float   torque_loss_pct;        /**< TQ_LOSS_CAN: friction/AC/elec loss % */

    /* Byte 6 */
    uint8_t err_amt_can;            /**< ERR_AMT_CAN: bits 6-7 */

    /* Byte 7 */
    float   torque_after_charge_pct;/**< TQI_MAF_CAN: torque after charge % */
} e46_dme1_t;

/* ========================================================================
 * DME2 0x329 - Temperatures, Pedal, Cruise Control (10ms)
 * ======================================================================== */

/** Multiplex code for byte 0 of 0x329. */
typedef enum {
    E46_MUX_CAN_LEVEL   = 0,   /**< CAN bus function level (always 0x11) */
    E46_MUX_OBD_STEUER  = 2,   /**< GS diagnosis state */
    E46_MUX_MD_NORM     = 3,   /**< Refactored C_TQ_STND */
} e46_mux_code_t;

/** Cruise control switch state (STATE_MSW_CAN). */
typedef enum {
    E46_CRU_SW_NONE     = 0,
    E46_CRU_SW_SET_ACCEL= 1,   /**< Tip-up */
    E46_CRU_SW_DECEL    = 2,   /**< Tip-down */
    E46_CRU_SW_RESUME   = 3,
    E46_CRU_SW_DEACTIVATE=4,   /**< I/O button */
    E46_CRU_SW_ERROR    = 7,
} e46_cruise_switch_t;

/** Cruise control active state (STATE_CRU_CAN). */
typedef enum {
    E46_CRU_INACTIVE    = 0,
    E46_CRU_CONSTANT    = 1,   /**< Constant drive / tip up-down */
    E46_CRU_RESUME      = 3,
    E46_CRU_SET_ACCEL   = 5,
    E46_CRU_DECEL       = 7,
} e46_cruise_state_t;

typedef struct {
    /* Byte 0 - Multiplexed */
    e46_mux_code_t  mux_code;       /**< MUX_CODE: which info is in bits 0-5 */
    uint8_t         mux_info;       /**< MUX_INFO: 6-bit payload */

    /* Byte 1 */
    float           coolant_temp_c; /**< TEMP_ENG: (HEX * 0.75) - 48 */

    /* Byte 2 */
    uint16_t        ambient_pressure_hpa; /**< AMP_CAN: (HEX * 2) + 598 */

    /* Byte 3 - Bitfield */
    bool            clutch_depressed;   /**< LV_SWI_CLU */
    bool            idle_below_thresh;  /**< LV_LEVEL_IS */
    bool            acc_ack;            /**< LV_ACK_CRU_AD_ECU */
    bool            engine_running;     /**< LV_ERU_CAN */
    e46_cruise_switch_t cruise_switch;  /**< STATE_MSW_CAN */

    /* Byte 4 */
    float           tps_virtual_pct;    /**< TPS_VIRT_CRU_CAN: % */

    /* Byte 5 */
    float           accelerator_pct;    /**< TPS_CAN: pedal position % */

    /* Byte 6 - Bitfield */
    bool            brake_actuated;     /**< LV_BS */
    bool            brake_switch_faulty;/**< LV_ERR_BS */
    bool            kickdown_active;    /**< LV_KD_CAN */
    e46_cruise_state_t cruise_state;    /**< STATE_CRU_CAN */
    uint8_t         req_shiftlock;      /**< REQ_SHIFTLOCK: 0 or 3 */
} e46_dme2_t;

/* ========================================================================
 * DME3 0x338 - Sport Button (1000ms / on change, Alpina only)
 * ======================================================================== */

typedef enum {
    E46_SPORT_ON_REQ    = 0,    /**< Sport on (requested by SMG) */
    E46_SPORT_OFF       = 1,
    E46_SPORT_ON        = 2,
    E46_SPORT_ERROR     = 3,
} e46_sport_state_t;

typedef struct {
    e46_sport_state_t sport_state;  /**< STATE_SOF_CAN (byte 2) */
} e46_dme3_t;

/* ========================================================================
 * DME4 0x545 - Cluster Lights, Fuel, Oil, Warnings (10ms)
 * ======================================================================== */

typedef struct {
    /* Byte 0 - Bitfield */
    bool    check_engine;       /**< LV_MIL: CEL */
    bool    cruise_main_light;  /**< LV_MAIN_SWI_CRU */
    bool    eml_light;          /**< LV_ETC_DIAG */
    bool    fuel_cap_light;     /**< LV_FUC_CAN */

    /* Bytes 1-2 */
    uint16_t fuel_consumption_raw; /**< FCO: raw 16-bit value */

    /* Byte 3 - Bitfield */
    bool    oil_consumption_led;    /**< Oil level - consumption */
    bool    oil_loss_led;           /**< Oil level - loss */
    bool    oil_sensor_fault_led;   /**< Oil level - sensor fault */
    bool    coolant_overheat;       /**< LV_TEMP_ENG */
    uint8_t warmup_leds;            /**< M-Cluster warm-up (bits 4-6, 0-7) */
    bool    upshift_indicator;      /**< Bit 7 */

    /* Byte 4 */
    int16_t oil_temp_c;             /**< TOIL_CAN: HEX - 48 */

    /* Byte 5 */
    bool    battery_charge_light;   /**< Alpina only */

    /* Byte 6 */
    float   oil_level_l;            /**< (HEX - 158) / 10, MSS54HP only */

    /* Byte 7 - Bitfield */
    bool    tire_pressure_warning;  /**< MSS54 only */
    bool    oil_pressure_low;       /**< Bit 7 */
} e46_dme4_t;

/* ========================================================================
 * ASC1 0x153 - Traction Control (10ms ASC / 20ms DSC)
 * ======================================================================== */

typedef struct {
    /* Byte 0 - Bitfield */
    bool    asc_request;        /**< LV_ASC_REQ */
    bool    msr_request;        /**< LV_MSR_REQ */
    bool    asc_passive;        /**< LV_ASC_PASV */
    bool    asc_switch_influence; /**< LV_ASC_SW_INT */
    bool    brake_light_switch; /**< LV_BLS */

    /* Bytes 1-2: Vehicle speed */
    float   vehicle_speed_kmh;  /**< VSS: ((MSB*256)+LSB) * 0.0625, offset 0x160 */

    /* Byte 3 */
    float   torque_intervention_asc_pct; /**< MD_IND_ASC: HEX * 0.390625 */

    /* Byte 4 */
    float   torque_intervention_msr_pct; /**< MD_IND_MSR: HEX * 0.390625 */

    /* Byte 6 */
    float   torque_intervention_asc_lm_pct; /**< MD_IND_ASC_LM */

    /* Byte 7 */
    uint8_t alive_counter;      /**< 0x00 - 0x0F */
} e46_asc1_t;

/* ========================================================================
 * ICL2 0x613 - Instrument Cluster (200ms)
 * ======================================================================== */

typedef struct {
    uint32_t odometer_km;       /**< KM_CTR_CAN: HEX * 10 */
    uint8_t  fuel_tank_level;   /**< FTL_CAN: bits 0-6 */
    bool     fuel_reserve;      /**< FTL_RES_CAN: bit 7 */
    uint16_t running_clock_min; /**< T_REL_CAN: minutes */
    uint8_t  fuel_level_driver; /**< FTL_CAN_L: bits 0-5 */
} e46_icl2_t;

/* ========================================================================
 * ICL3 0x615 - Instrument Cluster: AC, Temp, Lighting, Speed (200ms)
 * ======================================================================== */

typedef struct {
    /* Byte 0 */
    uint8_t ac_torque_offset_nm;    /**< TQ_ACCIN_CAN: 0-31 Nm (bits 0-4) */
    bool    req_lower_coolant_temp; /**< LV_REQ_TCO_L */
    bool    ac_compressor_on;       /**< LV_ACCIN */
    bool    ac_request;             /**< LV_ACIN */

    /* Byte 1 */
    bool    increased_heat_req;     /**< LV_REQ_HEAT */
    bool    trailer_mode;           /**< LV_TOW */
    bool    night_lighting;         /**< LV_LGT: 0=day, 1=night */
    bool    hood_switch;            /**< LV_HS */
    uint8_t cooling_fan_level;      /**< N_ECF: 0-15 (bits 4-7) */

    /* Byte 2 */
    bool    req_raised_idle;        /**< Bit 6 */

    /* Byte 3 */
    uint8_t ambient_temp_raw;       /**< TAM_CAN */

    /* Byte 4 */
    bool    door_switch;            /**< LV_DOOR */
    bool    handbrake_switch;       /**< LV_HBR */
    uint8_t suspension_switch;      /**< LV_SUSP: 2-bit */

    /* Bytes 5-6 */
    uint16_t displayed_speed_raw;   /**< VSS_DIS: 10-bit value */
} e46_icl3_t;

/* ========================================================================
 * AMT1 0x43D - SMG Transmission (10ms)
 * ======================================================================== */

/** SMG clutch state (STATE_CLU_AMT). */
typedef enum {
    E46_CLU_OPEN     = 0,   /**< Clutch open */
    E46_CLU_CREEPING = 1,   /**< Clutch creeping */
    E46_CLU_DRIVEOFF = 2,   /**< Clutch drive-off */
    E46_CLU_CLOSED   = 3,   /**< Clutch closed */
} e46_clutch_state_t;

typedef struct {
    /* Byte 0 */
    uint8_t active_gear;        /**< GEAR_INFO: 0-7 (bits 0-2) */
    bool    gear_shift_active;  /**< LV_GS */
    uint8_t shift_process_state;/**< STATE_AMT: 2-bit (bits 4-5) */
    bool    engine_stop_request;/**< LV_AMT_ES: SMG requests engine stop */

    /* Byte 1 */
    uint8_t display_gear;       /**< GEAR_SEL_DISP: 0-15 (bits 0-3) */
    uint8_t obd_state;          /**< STATE_AMT_OBD: 0-15 (bits 4-7) */

    /* Byte 2 */
    bool    city_mode;          /**< LV_CITY */
    bool    hydraulic_pump_active; /**< LV_HPA */

    /* Byte 3 */
    uint8_t torque_request;     /**< TQI_AMT_CAN */

    /* Byte 4 */
    uint8_t torque_maf_request; /**< TQI_MAF_AMT_CAN */

    /* Byte 5 */
    bool    gearbox_protection; /**< LV_GP: triggers speed limit */
    uint8_t message_counter;    /**< CTR_AMT: 0-15 (bits 2-5) */
    e46_clutch_state_t clutch_state; /**< STATE_CLU_AMT (bits 6-7) */

    /* Byte 6 */
    uint8_t torque_coupled;     /**< TQI_AMT_CPL_CAN */

    /* Byte 7 */
    uint8_t rpm_limit_override; /**< N_MAX_AMT_CAN: rev limiter during shift */
} e46_amt1_t;

/* ========================================================================
 * Decode functions: CAN frame -> struct
 * ======================================================================== */

static inline void e46_decode_dme1(const can_frame_t *f, e46_dme1_t *d)
{
    memset(d, 0, sizeof(*d));

    /* Byte 0 */
    d->ignition_on      = (f->data[0] >> 0) & 1;
    d->crankshaft_error  = (f->data[0] >> 1) & 1;
    d->traction_ack      = (f->data[0] >> 2) & 1;
    d->gear_change_ok    = (f->data[0] >> 3) & 1;
    d->charge_state      = (e46_charge_state_t)((f->data[0] >> 4) & 0x03);
    d->maf_error         = (f->data[0] >> 7) & 1;

    /* Byte 1: torque = HEX * 0.390625 */
    d->torque_request_pct = (float)f->data[1] * 0.390625f;

    /* Bytes 2-3: RPM = ((MSB * 256) + LSB) * 0.15625 -- LSB first */
    uint16_t rpm_raw = (uint16_t)f->data[2] | ((uint16_t)f->data[3] << 8);
    d->engine_rpm = (float)rpm_raw * 0.15625f;

    /* Byte 4 */
    d->torque_indicated_pct = (float)f->data[4] * 0.390625f;

    /* Byte 5 */
    d->torque_loss_pct = (float)f->data[5] * 0.390625f;

    /* Byte 6 */
    d->err_amt_can = (f->data[6] >> 6) & 0x03;

    /* Byte 7 */
    d->torque_after_charge_pct = (float)f->data[7] * 0.390625f;
}

static inline void e46_decode_dme2(const can_frame_t *f, e46_dme2_t *d)
{
    memset(d, 0, sizeof(*d));

    /* Byte 0 - Multiplexed */
    d->mux_info = f->data[0] & 0x3F;
    d->mux_code = (e46_mux_code_t)((f->data[0] >> 6) & 0x03);

    /* Byte 1: temp = (HEX * 0.75) - 48 */
    d->coolant_temp_c = (float)f->data[1] * 0.75f - 48.0f;

    /* Byte 2: pressure = (HEX * 2) + 598 */
    d->ambient_pressure_hpa = (uint16_t)(f->data[2] * 2 + 598);

    /* Byte 3 */
    d->clutch_depressed  = (f->data[3] >> 0) & 1;
    d->idle_below_thresh = (f->data[3] >> 1) & 1;
    d->acc_ack           = (f->data[3] >> 2) & 1;
    d->engine_running    = (f->data[3] >> 3) & 1;
    d->cruise_switch     = (e46_cruise_switch_t)((f->data[3] >> 5) & 0x07);

    /* Byte 4 */
    d->tps_virtual_pct = (float)f->data[4] * 0.390625f;

    /* Byte 5 */
    d->accelerator_pct = (float)f->data[5] * 0.390625f;

    /* Byte 6 */
    d->brake_actuated      = (f->data[6] >> 0) & 1;
    d->brake_switch_faulty = (f->data[6] >> 1) & 1;
    d->kickdown_active     = (f->data[6] >> 2) & 1;
    d->cruise_state        = (e46_cruise_state_t)((f->data[6] >> 3) & 0x07);
    d->req_shiftlock       = (f->data[6] >> 6) & 0x03;
}

static inline void e46_decode_dme3(const can_frame_t *f, e46_dme3_t *d)
{
    d->sport_state = (e46_sport_state_t)(f->data[2] & 0x03);
}

static inline void e46_decode_dme4(const can_frame_t *f, e46_dme4_t *d)
{
    memset(d, 0, sizeof(*d));

    /* Byte 0 */
    d->check_engine      = (f->data[0] >> 1) & 1;
    d->cruise_main_light = (f->data[0] >> 3) & 1;
    d->eml_light         = (f->data[0] >> 4) & 1;
    d->fuel_cap_light    = (f->data[0] >> 6) & 1;

    /* Bytes 1-2: fuel consumption (LSB first) */
    d->fuel_consumption_raw = (uint16_t)f->data[1] | ((uint16_t)f->data[2] << 8);

    /* Byte 3 */
    d->oil_consumption_led  = (f->data[3] >> 0) & 1;
    d->oil_loss_led         = (f->data[3] >> 1) & 1;
    d->oil_sensor_fault_led = (f->data[3] >> 2) & 1;
    d->coolant_overheat     = (f->data[3] >> 3) & 1;
    d->warmup_leds          = (f->data[3] >> 4) & 0x07;
    d->upshift_indicator    = (f->data[3] >> 7) & 1;

    /* Byte 4: oil temp = HEX - 48 */
    d->oil_temp_c = (int16_t)f->data[4] - 48;

    /* Byte 5 */
    d->battery_charge_light = (f->data[5] >> 0) & 1;

    /* Byte 6: oil level = (HEX - 158) / 10.0 */
    d->oil_level_l = ((float)f->data[6] - 158.0f) / 10.0f;

    /* Byte 7 */
    d->tire_pressure_warning = (f->data[7] >> 0) & 1;
    d->oil_pressure_low      = (f->data[7] >> 7) & 1;
}

static inline void e46_decode_asc1(const can_frame_t *f, e46_asc1_t *d)
{
    memset(d, 0, sizeof(*d));

    /* Byte 0 */
    d->asc_request          = (f->data[0] >> 0) & 1;
    d->msr_request          = (f->data[0] >> 1) & 1;
    d->asc_passive          = (f->data[0] >> 2) & 1;
    d->asc_switch_influence = (f->data[0] >> 3) & 1;
    d->brake_light_switch   = (f->data[0] >> 4) & 1;

    /* Bytes 1-2: speed = ((MSB*256)+bits_from_byte1) * 0.0625
     * Byte 1 bits 3-7 are VSS[0..4], byte 2 is VSS[MSB] */
    uint16_t vss_raw = ((uint16_t)(f->data[1] >> 3) & 0x1F)
                     | ((uint16_t)f->data[2] << 5);
    d->vehicle_speed_kmh = (float)vss_raw * 0.0625f;

    /* Byte 3 */
    d->torque_intervention_asc_pct = (float)f->data[3] * 0.390625f;

    /* Byte 4 */
    d->torque_intervention_msr_pct = (float)f->data[4] * 0.390625f;

    /* Byte 6 */
    d->torque_intervention_asc_lm_pct = (float)f->data[6] * 0.390625f;

    /* Byte 7 */
    d->alive_counter = f->data[7] & 0x0F;
}

static inline void e46_decode_icl2(const can_frame_t *f, e46_icl2_t *d)
{
    memset(d, 0, sizeof(*d));

    /* Bytes 0-1: odometer = HEX * 10 */
    d->odometer_km = ((uint32_t)f->data[0] | ((uint32_t)f->data[1] << 8)) * 10;

    /* Byte 2 */
    d->fuel_tank_level = f->data[2] & 0x7F;
    d->fuel_reserve    = (f->data[2] >> 7) & 1;

    /* Bytes 3-4: running clock in minutes */
    d->running_clock_min = (uint16_t)f->data[3] | ((uint16_t)f->data[4] << 8);

    /* Byte 5 */
    d->fuel_level_driver = f->data[5] & 0x3F;
}

static inline void e46_decode_icl3(const can_frame_t *f, e46_icl3_t *d)
{
    memset(d, 0, sizeof(*d));

    /* Byte 0 */
    d->ac_torque_offset_nm    = f->data[0] & 0x1F;
    d->req_lower_coolant_temp = (f->data[0] >> 5) & 1;
    d->ac_compressor_on       = (f->data[0] >> 6) & 1;
    d->ac_request             = (f->data[0] >> 7) & 1;

    /* Byte 1 */
    d->increased_heat_req = (f->data[1] >> 0) & 1;
    d->trailer_mode       = (f->data[1] >> 1) & 1;
    d->night_lighting     = (f->data[1] >> 2) & 1;
    d->hood_switch        = (f->data[1] >> 3) & 1;
    d->cooling_fan_level  = (f->data[1] >> 4) & 0x0F;

    /* Byte 2 */
    d->req_raised_idle = (f->data[2] >> 6) & 1;

    /* Byte 3 */
    d->ambient_temp_raw = f->data[3];

    /* Byte 4 */
    d->door_switch       = (f->data[4] >> 0) & 1;
    d->handbrake_switch  = (f->data[4] >> 1) & 1;
    d->suspension_switch = (f->data[4] >> 2) & 0x03;

    /* Bytes 5-6: VSS_DIS 10-bit = byte5 bits 6-7 (LSB) | byte6 << 2 */
    d->displayed_speed_raw = (uint16_t)(((f->data[5] >> 6) & 0x03)
                           | ((uint16_t)f->data[6] << 2));
}

static inline void e46_decode_amt1(const can_frame_t *f, e46_amt1_t *d)
{
    memset(d, 0, sizeof(*d));

    /* Byte 0 */
    d->active_gear         = f->data[0] & 0x07;
    d->gear_shift_active   = (f->data[0] >> 3) & 1;
    d->shift_process_state = (f->data[0] >> 4) & 0x03;
    d->engine_stop_request = (f->data[0] >> 7) & 1;

    /* Byte 1 */
    d->display_gear = f->data[1] & 0x0F;
    d->obd_state    = (f->data[1] >> 4) & 0x0F;

    /* Byte 2 */
    d->city_mode            = (f->data[2] >> 3) & 1;
    d->hydraulic_pump_active= (f->data[2] >> 5) & 1;

    /* Byte 3 */
    d->torque_request = f->data[3];

    /* Byte 4 */
    d->torque_maf_request = f->data[4];

    /* Byte 5 */
    d->gearbox_protection = (f->data[5] >> 0) & 1;
    d->message_counter    = (f->data[5] >> 2) & 0x0F;
    d->clutch_state       = (e46_clutch_state_t)((f->data[5] >> 6) & 0x03);

    /* Byte 6 */
    d->torque_coupled = f->data[6];

    /* Byte 7 */
    d->rpm_limit_override = f->data[7];
}

/* ========================================================================
 * Encode functions: struct -> CAN frame (for simulation / emulation)
 * ======================================================================== */

static inline void e46_encode_dme1(const e46_dme1_t *d, can_frame_t *f)
{
    memset(f, 0, sizeof(*f));
    f->id  = E46_CAN_ID_DME1;
    f->len = 8;

    /* Byte 0 */
    f->data[0] = (uint8_t)(
        ((d->ignition_on     ? 1 : 0) << 0) |
        ((d->crankshaft_error? 1 : 0) << 1) |
        ((d->traction_ack    ? 1 : 0) << 2) |
        ((d->gear_change_ok  ? 1 : 0) << 3) |
        (((uint8_t)d->charge_state & 0x03) << 4) |
        ((d->maf_error       ? 1 : 0) << 7)
    );

    /* Byte 1 */
    f->data[1] = (uint8_t)(d->torque_request_pct / 0.390625f);

    /* Bytes 2-3: LSB first */
    uint16_t rpm_raw = (uint16_t)(d->engine_rpm / 0.15625f);
    f->data[2] = (uint8_t)(rpm_raw & 0xFF);
    f->data[3] = (uint8_t)(rpm_raw >> 8);

    /* Byte 4 */
    f->data[4] = (uint8_t)(d->torque_indicated_pct / 0.390625f);

    /* Byte 5 */
    f->data[5] = (uint8_t)(d->torque_loss_pct / 0.390625f);

    /* Byte 6 */
    f->data[6] = (d->err_amt_can & 0x03) << 6;

    /* Byte 7 */
    f->data[7] = (uint8_t)(d->torque_after_charge_pct / 0.390625f);
}

static inline void e46_encode_dme2(const e46_dme2_t *d, can_frame_t *f)
{
    memset(f, 0, sizeof(*f));
    f->id  = E46_CAN_ID_DME2;
    f->len = 8;

    /* Byte 0 */
    f->data[0] = (d->mux_info & 0x3F)
               | (((uint8_t)d->mux_code & 0x03) << 6);

    /* Byte 1: temp = (val + 48) / 0.75 */
    float temp_raw = (d->coolant_temp_c + 48.0f) / 0.75f;
    if (temp_raw < 0.0f) temp_raw = 0.0f;
    if (temp_raw > 255.0f) temp_raw = 255.0f;
    f->data[1] = (uint8_t)temp_raw;

    /* Byte 2: pressure = (val - 598) / 2 */
    int16_t press_raw = (int16_t)((d->ambient_pressure_hpa - 598) / 2);
    if (press_raw < 0) press_raw = 0;
    if (press_raw > 254) press_raw = 254;
    f->data[2] = (uint8_t)press_raw;

    /* Byte 3 */
    f->data[3] = (uint8_t)(
        ((d->clutch_depressed  ? 1 : 0) << 0) |
        ((d->idle_below_thresh ? 1 : 0) << 1) |
        ((d->acc_ack           ? 1 : 0) << 2) |
        ((d->engine_running    ? 1 : 0) << 3) |
        (((uint8_t)d->cruise_switch & 0x07) << 5)
    );

    /* Byte 4 */
    f->data[4] = (uint8_t)(d->tps_virtual_pct / 0.390625f);

    /* Byte 5 */
    f->data[5] = (uint8_t)(d->accelerator_pct / 0.390625f);

    /* Byte 6 */
    f->data[6] = (uint8_t)(
        ((d->brake_actuated      ? 1 : 0) << 0) |
        ((d->brake_switch_faulty ? 1 : 0) << 1) |
        ((d->kickdown_active     ? 1 : 0) << 2) |
        (((uint8_t)d->cruise_state & 0x07) << 3) |
        ((d->req_shiftlock & 0x03) << 6)
    );
}

static inline void e46_encode_dme3(const e46_dme3_t *d, can_frame_t *f)
{
    memset(f, 0, sizeof(*f));
    f->id  = E46_CAN_ID_DME3;
    f->len = 8;
    f->data[2] = (uint8_t)d->sport_state & 0x03;
}

static inline void e46_encode_dme4(const e46_dme4_t *d, can_frame_t *f)
{
    memset(f, 0, sizeof(*f));
    f->id  = E46_CAN_ID_DME4;
    f->len = 8;

    /* Byte 0 */
    f->data[0] = (uint8_t)(
        ((d->check_engine      ? 1 : 0) << 1) |
        ((d->cruise_main_light ? 1 : 0) << 3) |
        ((d->eml_light         ? 1 : 0) << 4) |
        ((d->fuel_cap_light    ? 1 : 0) << 6)
    );

    /* Bytes 1-2 */
    f->data[1] = (uint8_t)(d->fuel_consumption_raw & 0xFF);
    f->data[2] = (uint8_t)(d->fuel_consumption_raw >> 8);

    /* Byte 3 */
    f->data[3] = (uint8_t)(
        ((d->oil_consumption_led  ? 1 : 0) << 0) |
        ((d->oil_loss_led         ? 1 : 0) << 1) |
        ((d->oil_sensor_fault_led ? 1 : 0) << 2) |
        ((d->coolant_overheat     ? 1 : 0) << 3) |
        ((d->warmup_leds & 0x07) << 4) |
        ((d->upshift_indicator    ? 1 : 0) << 7)
    );

    /* Byte 4: oil temp = val + 48 */
    int16_t oil_raw = d->oil_temp_c + 48;
    if (oil_raw < 0) oil_raw = 0;
    if (oil_raw > 254) oil_raw = 254;
    f->data[4] = (uint8_t)oil_raw;

    /* Byte 5 */
    f->data[5] = (d->battery_charge_light ? 1 : 0);

    /* Byte 6: oil level = (val * 10) + 158 */
    float oil_lvl_raw = d->oil_level_l * 10.0f + 158.0f;
    if (oil_lvl_raw < 0.0f) oil_lvl_raw = 0.0f;
    if (oil_lvl_raw > 254.0f) oil_lvl_raw = 254.0f;
    f->data[6] = (uint8_t)oil_lvl_raw;

    /* Byte 7 */
    f->data[7] = (uint8_t)(
        ((d->tire_pressure_warning ? 1 : 0) << 0) |
        ((d->oil_pressure_low      ? 1 : 0) << 7)
    );
}

static inline void e46_encode_asc1(const e46_asc1_t *d, can_frame_t *f)
{
    memset(f, 0, sizeof(*f));
    f->id  = E46_CAN_ID_ASC1;
    f->len = 8;

    /* Byte 0 */
    f->data[0] = (uint8_t)(
        ((d->asc_request          ? 1 : 0) << 0) |
        ((d->msr_request          ? 1 : 0) << 1) |
        ((d->asc_passive          ? 1 : 0) << 2) |
        ((d->asc_switch_influence ? 1 : 0) << 3) |
        ((d->brake_light_switch   ? 1 : 0) << 4)
    );

    /* Bytes 1-2: speed */
    uint16_t vss_raw = (uint16_t)(d->vehicle_speed_kmh / 0.0625f);
    f->data[1] = (f->data[1] & 0x07) | (uint8_t)((vss_raw & 0x1F) << 3);
    f->data[2] = (uint8_t)(vss_raw >> 5);

    /* Byte 3 */
    f->data[3] = (uint8_t)(d->torque_intervention_asc_pct / 0.390625f);

    /* Byte 4 */
    f->data[4] = (uint8_t)(d->torque_intervention_msr_pct / 0.390625f);

    /* Byte 6 */
    f->data[6] = (uint8_t)(d->torque_intervention_asc_lm_pct / 0.390625f);

    /* Byte 7 */
    f->data[7] = d->alive_counter & 0x0F;
}

static inline void e46_encode_icl2(const e46_icl2_t *d, can_frame_t *f)
{
    memset(f, 0, sizeof(*f));
    f->id  = E46_CAN_ID_ICL2;
    f->len = 8;

    /* Bytes 0-1: odometer / 10 */
    uint16_t odo_raw = (uint16_t)(d->odometer_km / 10);
    f->data[0] = (uint8_t)(odo_raw & 0xFF);
    f->data[1] = (uint8_t)(odo_raw >> 8);

    /* Byte 2 */
    f->data[2] = (d->fuel_tank_level & 0x7F) | ((d->fuel_reserve ? 1 : 0) << 7);

    /* Bytes 3-4 */
    f->data[3] = (uint8_t)(d->running_clock_min & 0xFF);
    f->data[4] = (uint8_t)(d->running_clock_min >> 8);

    /* Byte 5 */
    f->data[5] = d->fuel_level_driver & 0x3F;
}

static inline void e46_encode_icl3(const e46_icl3_t *d, can_frame_t *f)
{
    memset(f, 0, sizeof(*f));
    f->id  = E46_CAN_ID_ICL3;
    f->len = 8;

    /* Byte 0 */
    f->data[0] = (uint8_t)(
        (d->ac_torque_offset_nm & 0x1F) |
        ((d->req_lower_coolant_temp ? 1 : 0) << 5) |
        ((d->ac_compressor_on       ? 1 : 0) << 6) |
        ((d->ac_request             ? 1 : 0) << 7)
    );

    /* Byte 1 */
    f->data[1] = (uint8_t)(
        ((d->increased_heat_req ? 1 : 0) << 0) |
        ((d->trailer_mode       ? 1 : 0) << 1) |
        ((d->night_lighting     ? 1 : 0) << 2) |
        ((d->hood_switch        ? 1 : 0) << 3) |
        ((d->cooling_fan_level & 0x0F) << 4)
    );

    /* Byte 2 */
    f->data[2] = (uint8_t)((d->req_raised_idle ? 1 : 0) << 6);

    /* Byte 3 */
    f->data[3] = d->ambient_temp_raw;

    /* Byte 4 */
    f->data[4] = (uint8_t)(
        ((d->door_switch      ? 1 : 0) << 0) |
        ((d->handbrake_switch ? 1 : 0) << 1) |
        ((d->suspension_switch & 0x03) << 2) |
        ((d->req_lower_coolant_temp ? 1 : 0) << 5) |
        ((d->ac_compressor_on       ? 1 : 0) << 6) |
        ((d->ac_request             ? 1 : 0) << 7)
    );

    /* Bytes 5-6: VSS_DIS */
    f->data[5] = (uint8_t)((d->displayed_speed_raw & 0x03) << 6);
    f->data[6] = (uint8_t)((d->displayed_speed_raw >> 2) & 0xFF);
}

static inline void e46_encode_amt1(const e46_amt1_t *d, can_frame_t *f)
{
    memset(f, 0, sizeof(*f));
    f->id  = E46_CAN_ID_AMT1;
    f->len = 8;

    /* Byte 0 */
    f->data[0] = (uint8_t)(
        (d->active_gear & 0x07) |
        ((d->gear_shift_active   ? 1 : 0) << 3) |
        ((d->shift_process_state & 0x03) << 4) |
        ((d->engine_stop_request ? 1 : 0) << 7)
    );

    /* Byte 1 */
    f->data[1] = (d->display_gear & 0x0F) | ((d->obd_state & 0x0F) << 4);

    /* Byte 2 */
    f->data[2] = (uint8_t)(
        ((d->city_mode             ? 1 : 0) << 3) |
        ((d->hydraulic_pump_active ? 1 : 0) << 5)
    );

    /* Byte 3 */
    f->data[3] = d->torque_request;

    /* Byte 4 */
    f->data[4] = d->torque_maf_request;

    /* Byte 5 */
    f->data[5] = (uint8_t)(
        ((d->gearbox_protection ? 1 : 0) << 0) |
        ((d->message_counter & 0x0F) << 2) |
        (((uint8_t)d->clutch_state & 0x03) << 6)
    );

    /* Byte 6 */
    f->data[6] = d->torque_coupled;

    /* Byte 7 */
    f->data[7] = d->rpm_limit_override;
}

/* ========================================================================
 * Dispatch decoder: route any E46 CAN frame to the appropriate struct
 * ======================================================================== */

/** Container holding all decoded E46 bus state. */
typedef struct {
    e46_dme1_t  dme1;
    e46_dme2_t  dme2;
    e46_dme3_t  dme3;
    e46_dme4_t  dme4;
    e46_asc1_t  asc1;
    e46_icl2_t  icl2;
    e46_icl3_t  icl3;
    e46_amt1_t  amt1;

    uint32_t    dme1_last_ms;
    uint32_t    dme2_last_ms;
    uint32_t    dme3_last_ms;
    uint32_t    dme4_last_ms;
    uint32_t    asc1_last_ms;
    uint32_t    icl2_last_ms;
    uint32_t    icl3_last_ms;
    uint32_t    amt1_last_ms;
} e46_bus_state_t;

/**
 * Decode a CAN frame and update the appropriate field in the bus state.
 * @return true if the frame was recognized, false otherwise.
 */
static inline bool e46_decode_frame(const can_frame_t *f, e46_bus_state_t *s, uint32_t now_ms)
{
    switch (f->id) {
    case E46_CAN_ID_DME1:
        e46_decode_dme1(f, &s->dme1);
        s->dme1_last_ms = now_ms;
        return true;
    case E46_CAN_ID_DME2:
        e46_decode_dme2(f, &s->dme2);
        s->dme2_last_ms = now_ms;
        return true;
    case E46_CAN_ID_DME3:
        e46_decode_dme3(f, &s->dme3);
        s->dme3_last_ms = now_ms;
        return true;
    case E46_CAN_ID_DME4:
        e46_decode_dme4(f, &s->dme4);
        s->dme4_last_ms = now_ms;
        return true;
    case E46_CAN_ID_ASC1:
        e46_decode_asc1(f, &s->asc1);
        s->asc1_last_ms = now_ms;
        return true;
    case E46_CAN_ID_ICL2:
        e46_decode_icl2(f, &s->icl2);
        s->icl2_last_ms = now_ms;
        return true;
    case E46_CAN_ID_ICL3:
        e46_decode_icl3(f, &s->icl3);
        s->icl3_last_ms = now_ms;
        return true;
    case E46_CAN_ID_AMT1:
        e46_decode_amt1(f, &s->amt1);
        s->amt1_last_ms = now_ms;
        return true;
    default:
        return false;
    }
}

/**
 * Map decoded E46 bus state into the generic vehicle_data_t used by skins.
 */
static inline void e46_to_vehicle_data(const e46_bus_state_t *s, vehicle_data_t *vd)
{
    /* Engine data from DME1 */
    vd->rpm            = (uint16_t)s->dme1.engine_rpm;
    vd->engine_flags   = 0;
    if (s->dme2.engine_running) vd->engine_flags |= ENG_RUNNING;
    if (s->dme4.check_engine)   vd->engine_flags |= ENG_CHECK_ENGINE;
    if (s->dme4.oil_pressure_low) vd->engine_flags |= ENG_OIL_WARN;
    if (s->dme4.coolant_overheat) vd->engine_flags |= ENG_OVERHEAT;

    /* Temperatures from DME2/DME4 */
    vd->coolant_temp_c = (int8_t)s->dme2.coolant_temp_c;

    /* Throttle from DME2 */
    vd->throttle_pct   = (uint8_t)s->dme2.accelerator_pct;

    /* Oil from DME4 */
    vd->oil_pressure_psi = 0; /* Not directly available on E46 CAN */

    /* Speed from ASC1 */
    vd->speed_kmh_x10 = (uint16_t)(s->asc1.vehicle_speed_kmh * 10.0f);

    /* Cluster data from ICL2 */
    vd->odometer_km    = s->icl2.odometer_km;
    vd->fuel_level_pct = s->icl2.fuel_tank_level;

    /* Fuel consumption from DME4 */
    vd->fuel_consumption_x10 = s->dme4.fuel_consumption_raw;

    /* Warnings */
    vd->warning_flags = 0;
    if (s->icl2.fuel_reserve)           vd->warning_flags |= WARN_LOW_FUEL;
    if (s->dme4.oil_pressure_low)       vd->warning_flags |= WARN_BRAKE; /* reuse */

    /* Ambient temp approximation from DME2 pressure (not directly available) */
    vd->ambient_temp_c = 22; /* placeholder */
}

#ifdef __cplusplus
}
#endif

#endif /* E46_CAN_PROTOCOL_H */
