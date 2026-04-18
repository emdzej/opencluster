# BMW E46 CAN Bus Protocol Reference

Reference implementation for the BMW E46 chassis CAN bus (500 kbit/s), as used by the
Siemens MS43 ECU. All message definitions are based on the
[MS4X Wiki](https://www.ms4x.net/index.php?title=Siemens_MS43_CAN_Bus).

**Bus parameters:**

| Parameter | Value |
|-----------|-------|
| Standard | CAN 2.0B |
| Bit rate | 500 kbps |
| ID format | 11-bit standard |
| Max payload | 8 bytes |

---

## Message Overview

### Sent by DME (Engine Control Unit)

| CAN ID | Name | Rate | Description |
|--------|------|------|-------------|
| `0x316` | DME1 | 10 ms | Engine RPM, torque values, status flags |
| `0x329` | DME2 | 10 ms | Coolant temp, throttle, cruise control, brake |
| `0x338` | DME3 | 1000 ms | Sport button state (Alpina Roadster only) |
| `0x545` | DME4 | 10 ms | Warning lights, fuel consumption, oil temp |

### Received by DME

| CAN ID | Name | Source | Rate | Description |
|--------|------|--------|------|-------------|
| `0x153` | ASC1 | DSC/ASC | 10-20 ms | Vehicle speed, traction intervention |
| `0x1F3` | ASC3 | DSC | 20 ms | Lateral/longitudinal acceleration |
| `0x613` | ICL2 | Cluster | 200 ms | Odometer, fuel level, running clock |
| `0x615` | ICL3 | Cluster | 200 ms | AC, ambient temp, lighting, displayed speed |
| `0x43F` | EGS1 | Gearbox | — | Automatic transmission |
| `0x43D` | AMT1 | SMG | 10 ms | Gear info, shift state, clutch, torque requests |
| `0x1F5` | LWS1 | Steering | — | Steering angle sensor |

### Other messages on bus (not used by DME)

| CAN ID | Name | Source | Description |
|--------|------|--------|-------------|
| `0x1F0` | ASC2 | DSC/ASC | Traction control |
| `0x1F8` | ASC4 | DSC/ASC | Traction control |
| `0x610` | ICL1 | Cluster | VIN (remote frame) |

---

## DME1 — `0x316`

Engine status, RPM, and torque values. Refresh rate: **10 ms**.

### Byte Layout

| Byte | Field | Type | Unit | Formula |
|------|-------|------|------|---------|
| 0 | Status bitfield | bits | — | See below |
| 1 | TQI_TQR_CAN | uint8 | % of C_TQ_STND | `value = raw * 0.390625` |
| 2 | N_ENG [LSB] | uint16 | RPM | `value = raw * 0.15625` |
| 3 | N_ENG [MSB] | | | (raw = byte3 << 8 \| byte2) |
| 4 | TQI_CAN | uint8 | % of C_TQ_STND | `value = raw * 0.390625` |
| 5 | TQ_LOSS_CAN | uint8 | % of C_TQ_STND | `value = raw * 0.390625` |
| 6 | ERR_AMT_CAN | bits 6-7 | — | AMT error code |
| 7 | TQI_MAF_CAN | uint8 | % of C_TQ_STND | `value = raw * 0.390625` |

### Byte 0 — Status Bitfield

| Bit | Signal | Description |
|-----|--------|-------------|
| 0 | LV_SWI_IGK | `0` = terminal 15 off, `1` = terminal 15 on |
| 1 | LV_F_N_ENG | `0` = CRK sensor OK, `1` = CRK error |
| 2 | LV_ACK_TCS | `0` = ASC1 timeout/error, `1` = ASC1 received OK |
| 3 | LV_ERR_GC | `0` = gear change not possible, `1` = gear change OK |
| 4-5 | SF_TQD | Charge intervention state (see table) |
| 6 | — | Unused |
| 7 | LV_F_SUB_TQI | `0` = MAF OK, `1` = MAF error |

**SF_TQD (Charge Intervention State):**

| Value | Meaning |
|-------|---------|
| 0 | Intervention can be performed completely |
| 1 | IGA retard limit reached |
| 2 | Charge actuators fully closed |
| 3 | Limited vehicle dynamics (MTC/ISA error) |

### Torque Fields

All torque values are percentages of `C_TQ_STND` (a calibration constant representing
the engine's nominal max torque).

- **TQI_TQR_CAN** — Indexed torque including all interventions (ASR/MSR/ETCU/LIM/AMT/GEAR)
- **TQI_CAN** — Indicated torque based on PVS, RPM, ambient pressure, intake temp, etc.
- **TQ_LOSS_CAN** — Torque loss from engine friction, AC compressor, alternator load
- **TQI_MAF_CAN** — Theoretical torque after charge intervention (based on MAF & IGA)

---

## DME2 — `0x329`

Temperatures, pedal position, cruise control. Refresh rate: **10 ms**.

### Byte Layout

| Byte | Field | Type | Unit | Formula |
|------|-------|------|------|---------|
| 0 | Multiplexed | bits | — | See below |
| 1 | TEMP_ENG | uint8 | °C | `value = raw * 0.75 - 48` |
| 2 | AMP_CAN | uint8 | hPa | `value = raw * 2 + 598` |
| 3 | Status bitfield | bits | — | See below |
| 4 | TPS_VIRT_CRU_CAN | uint8 | % | `value = raw * 0.390625` |
| 5 | TPS_CAN | uint8 | % of PVS_MAX | `value = raw * 0.390625` |
| 6 | Status bitfield | bits | — | See below |
| 7 | — | — | — | Unused |

### Byte 0 — Multiplexed Data

Bits 6-7 select which data is in bits 0-5:

| MUX_CODE (bits 6-7) | Bits 0-5 contain | Notes |
|----------------------|------------------|-------|
| 0 | CAN_LEVEL | Always `0x11` for MS43 |
| 2 | OBD_STEUER | GS diagnosis state |
| 3 | MD_NORM | Refactored C_TQ_STND (0–1008 Nm, 16 Nm resolution) |

### Byte 1 — Coolant Temperature

| Property | Value |
|----------|-------|
| Init | `0xFF` |
| Min | `0x01` → −47.25 °C |
| Max | `0xFF` → 143.25 °C |

### Byte 2 — Ambient Pressure

| Property | Value |
|----------|-------|
| Init | `0x00` |
| Min | `0x01` → 600 hPa |
| Max | `0xFE` → 1106 hPa |
| Error | `0xFF` |

### Byte 3 — Status Bitfield

| Bit | Signal | Description |
|-----|--------|-------------|
| 0 | LV_SWI_CLU | `0` = clutch released, `1` = clutch depressed |
| 1 | LV_LEVEL_IS | `0` = idle above threshold, `1` = idle below threshold |
| 2 | LV_ACK_CRU_AD_ECU | `0` = ACC1 nACK, `1` = ACC1 ACK |
| 3 | LV_ERU_CAN | `0` = engine stopped, `1` = engine running |
| 4 | (reserved) | — |
| 5-7 | STATE_MSW_CAN | Cruise switch state (see table) |

**STATE_MSW_CAN (Cruise Control Switch):**

| Value | Meaning |
|-------|---------|
| 0 | No button pressed (init) |
| 1 | Set / Acceleration (tip-up) |
| 2 | Deceleration (tip-down) |
| 3 | Resume |
| 4 | Deactivate (I/O) |
| 7 | Error condition |

### Byte 5 — Accelerator Pedal

| Property | Value |
|----------|-------|
| Init | `0x00` |
| Min | `0x01` → 0.39% |
| Max | `0xFE` → 99.2% |
| PVS Error | `0xFF` |

### Byte 6 — Status Bitfield

| Bit | Signal | Description |
|-----|--------|-------------|
| 0 | LV_BS | `0` = brake not actuated, `1` = brake actuated |
| 1 | LV_ERR_BS | `0` = brake switch OK, `1` = brake switch faulty |
| 2 | LV_KD_CAN | `0` = kickdown inactive, `1` = kickdown active |
| 3-5 | STATE_CRU_CAN | Cruise control active state (see table) |
| 6-7 | REQ_SHIFTLOCK | `0` = no actuation, `3` = ISA/MTC/N_SP_IS active |

**STATE_CRU_CAN (Cruise Active State):**

| Value | Meaning |
|-------|---------|
| 0 | Cruise not active |
| 1 | Constant drive (or tip-up / tip-down) |
| 3 | Resume |
| 5 | Set / Acceleration |
| 7 | Deceleration |

---

## DME3 — `0x338`

Sport button status. Refresh rate: **1000 ms** and on signal change.
**Alpina Roadster only.**

### Byte Layout

| Byte | Field | Description |
|------|-------|-------------|
| 0 | — | Unused |
| 1 | — | Unused |
| 2 | STATE_SOF_CAN | Sport button state (bits 0-1) |
| 3-7 | — | Unused |

**STATE_SOF_CAN:**

| Value | Meaning |
|-------|---------|
| 0 | Sport on (requested by SMG) |
| 1 | Sport off (init value) |
| 2 | Sport on |
| 3 | Sport error |

---

## DME4 — `0x545`

Instrument cluster warning lights, fuel consumption, oil data. Refresh rate: **10 ms**.

### Byte Layout

| Byte | Field | Type | Unit | Formula |
|------|-------|------|------|---------|
| 0 | Warning lights | bits | — | See below |
| 1 | FCO [LSB] | uint16 | — | Fuel consumption (raw) |
| 2 | FCO [MSB] | | | |
| 3 | Oil/cluster indicators | bits | — | See below |
| 4 | TOIL_CAN | uint8 | °C | `value = raw - 48` |
| 5 | Alpina bitfield | bits | — | See below |
| 6 | Oil level | uint8 | L | `value = (raw - 158) / 10` (MSS54HP only) |
| 7 | Misc bitfield | bits | — | See below |

### Byte 0 — Warning Lights

| Bit | Signal | Description |
|-----|--------|-------------|
| 0 | — | Unused |
| 1 | LV_MIL | Check Engine Light (CEL) |
| 2 | — | Unused |
| 3 | LV_MAIN_SWI_CRU | Cruise control main switch indicator |
| 4 | LV_ETC_DIAG | EML (Electronic Throttle) warning light |
| 5 | — | Unused |
| 6 | LV_FUC_CAN | Fuel tank cap warning light |
| 7 | — | Unused |

### Byte 3 — Oil & Cluster Indicators

| Bit | Signal | Description |
|-----|--------|-------------|
| 0 | Oil consumption | Oil level indicator LED |
| 1 | Oil loss | Oil level indicator LED |
| 2 | Oil sensor fault | Oil level indicator LED |
| 3 | LV_TEMP_ENG | Coolant overheating light |
| 4-6 | Warm-up LEDs | M-Cluster warm-up indicator (0-7) |
| 7 | Upshift indicator | Upshift recommendation |

### Byte 4 — Oil Temperature

| Property | Value |
|----------|-------|
| Min | `0x00` → −48 °C |
| Max | `0xFE` → 206 °C |

### Byte 7 — Miscellaneous

| Bit | Signal | Description |
|-----|--------|-------------|
| 0 | Tire pressure | State tire pressure warning (MSS54 only) |
| 1-6 | — | Unused |
| 7 | Oil pressure low | Engine oil pressure warning |

---

## ASC1 — `0x153`

Traction control data sent by the DSC/ASC module. Refresh rate: **10 ms** (ASC) / **20 ms** (DSC).

### Byte Layout

| Byte | Field | Type | Unit | Formula |
|------|-------|------|------|---------|
| 0 | Status bitfield | bits | — | See below |
| 1 | VSS [bits 3-7] + status | bits | km/h | See below |
| 2 | VSS [MSB] | uint8 | | `speed = raw_13bit * 0.0625` |
| 3 | MD_IND_ASC | uint8 | % of C_TQ_STND | `value = raw * 0.390625` |
| 4 | MD_IND_MSR | uint8 | % of C_TQ_STND | `value = raw * 0.390625` |
| 5 | — | — | — | Unused |
| 6 | MD_IND_ASC_LM | uint8 | % of C_TQ_STND | `value = raw * 0.390625` |
| 7 | ASC ALIVE | uint8 | — | Counter 0x00–0x0F |

### Byte 0 — Status Bitfield

| Bit | Signal | Description |
|-----|--------|-------------|
| 0 | LV_ASC_REQ | ASC intervention request |
| 1 | LV_MSR_REQ | MSR intervention request |
| 2 | LV_ASC_PASV | ASC passive status (for EGS) |
| 3 | LV_ASC_SW_INT | ASC switching influence |
| 4 | LV_BLS | Brake light switch status |
| 5 | LV_BAS | Brake assist |
| 6 | LV_EBV | Electronic brake force distribution |
| 7 | LV_ABS_LED | ABS warning LED |

### Vehicle Speed (VSS)

The vehicle speed is a 13-bit value split across bytes 1 and 2:
- Byte 1, bits 3-7: VSS[0..4] (LSB part)
- Byte 2: VSS[5..12] (MSB part)

```
speed_kmh = ((byte2 << 5) | ((byte1 >> 3) & 0x1F)) * 0.0625
```

Minimum raw value: `0x160` (0 km/h).

### Torque Interventions

- **MD_IND_ASC** — `0x00` = maximum reduction, `0xFF` = no reduction
- **MD_IND_MSR** — `0x00` = no engine torque increase, `0xFF` = max increase
- **MD_IND_ASC_LM** — ASC LM torque intervention (same scale as MD_IND_ASC)

---

## ICL2 — `0x613`

Instrument cluster data. Refresh rate: **200 ms**.

### Byte Layout

| Byte | Field | Type | Unit | Formula |
|------|-------|------|------|---------|
| 0 | KM_CTR_CAN [LSB] | uint16 | km | `odometer = raw * 10` |
| 1 | KM_CTR_CAN [MSB] | | | |
| 2 | FTL_CAN (bits 0-6), FTL_RES_CAN (bit 7) | uint8 | — | Fuel level + reserve flag |
| 3 | T_REL_CAN [LSB] | uint16 | min | Running clock in minutes |
| 4 | T_REL_CAN [MSB] | | | |
| 5 | FTL_CAN_L (bits 0-5) | uint8 | — | Fuel level, driver side |
| 6-7 | — | — | — | Unused |

---

## ICL1 — `0x610`

VIN data from instrument cluster. This is a **remote frame** — only sent on request
(typically by DSC at startup for VIN integrity check).

### Byte Layout

| Byte | Field | Description |
|------|-------|-------------|
| 0 | VIN digit | Last digit of VIN (drop trailing zero) |
| 1 | VIN digits | 3rd and 2nd from end |
| 2 | VIN digits | 5th and 4th from end |
| 3 | VIN char | ASCII, 6th from end |
| 4 | VIN char | ASCII, 7th from end |
| 5-7 | — | Unused |

---

## ICL3 — `0x615`

Instrument cluster data: AC compressor, ambient temperature, lighting, displayed speed.
Refresh rate: **200 ms**.

### Byte Layout

| Byte | Field | Type | Description |
|------|-------|------|-------------|
| 0 | AC / cooling bitfield | bits | See below |
| 1 | Fan / lighting bitfield | bits | See below |
| 2 | Misc bitfield | bits | See below |
| 3 | TAM_CAN | uint8 | Ambient temperature |
| 4 | Door / suspension bitfield | bits | See below |
| 5 | VSS_DIS [bits 6-7 = LSB] | bits | Displayed vehicle speed (low bits) |
| 6 | VSS_DIS [bits 2-9] | uint8 | Displayed vehicle speed (high byte) |
| 7 | — | — | Unused / unknown |

### Byte 0 — AC & Cooling

| Bit | Signal | Description |
|-----|--------|-------------|
| 0-4 | TQ_ACCIN_CAN | Torque offset for AC compressor (0–31 Nm) |
| 5 | LV_REQ_TCO_L | Request for lowering cooling temperature |
| 6 | LV_ACCIN | AC compressor status (`0` = off, `1` = on) |
| 7 | LV_ACIN | AC request (`0` = off, `1` = on) |

### Byte 1 — Fan & Lighting

| Bit | Signal | Description |
|-----|--------|-------------|
| 0 | LV_REQ_HEAT | Increased heat request |
| 1 | LV_TOW | Trailer operation mode |
| 2 | LV_LGT | Day / Night lighting (`0` = day, `1` = night) |
| 3 | LV_HS | Hood switch |
| 4-7 | N_ECF | Electric cooling fan level (0–15) |

### Byte 2 — Miscellaneous

| Bit | Signal | Description |
|-----|--------|-------------|
| 0-5 | — | Unknown / reserved |
| 6 | Raised idle | Request raised idle |
| 7 | — | Unused |

### Byte 4 — Door & Suspension

| Bit | Signal | Description |
|-----|--------|-------------|
| 0 | LV_DOOR | Door switch |
| 1 | LV_HBR | Hand brake switch |
| 2-3 | LV_SUSP | Suspension switch (2-bit) |
| 4 | — | Unknown |
| 5 | LV_REQ_TCO_L | Request for lowering cooling temp (duplicate) |
| 6 | LV_ACCIN | AC compressor status (duplicate) |
| 7 | LV_ACIN | AC request (duplicate) |

### Displayed Vehicle Speed (VSS_DIS)

10-bit value split across bytes 5 and 6:
- Byte 5, bits 6-7: VSS_DIS[0..1] (LSB)
- Byte 6: VSS_DIS[2..9] (MSB)

```
speed_raw = ((byte5 >> 6) & 0x03) | (byte6 << 2)
```

---

## AMT1 — `0x43D`

SMG (Sequential Manual Gearbox) transmission data. Refresh rate: **10 ms**.
Sent by the SMG/SSG/AMT control module.

### Byte Layout

| Byte | Field | Type | Description |
|------|-------|------|-------------|
| 0 | Gear / shift bitfield | bits | See below |
| 1 | Display gear / OBD | bits | See below |
| 2 | Status bitfield | bits | See below |
| 3 | TQI_AMT_CAN | uint8 | AMT torque request |
| 4 | TQI_MAF_AMT_CAN | uint8 | AMT MAF-based torque request |
| 5 | Protection / clutch bitfield | bits | See below |
| 6 | TQI_AMT_CPL_CAN | uint8 | AMT coupled torque request |
| 7 | N_MAX_AMT_CAN | uint8 | Engine speed limiter override during gearshift |

### Byte 0 — Gear & Shift Status

| Bit | Signal | Description |
|-----|--------|-------------|
| 0-2 | GEAR_INFO | Active gear (0–7) |
| 3 | LV_GS | Gear shift active |
| 4-5 | STATE_AMT | Shift process status (2-bit) |
| 6 | — | Unknown |
| 7 | LV_AMT_ES | Engine stop request from SMG TCU |

### Byte 1 — Display Gear & OBD

| Bit | Signal | Description |
|-----|--------|-------------|
| 0-3 | GEAR_SEL_DISP | Selected gear for cluster display |
| 4-7 | STATE_AMT_OBD | AMT OBD diagnostic state |

### Byte 2 — Status Flags

| Bit | Signal | Description |
|-----|--------|-------------|
| 0-2 | — | Unknown |
| 3 | LV_CITY | City mode active |
| 4 | — | Unknown |
| 5 | LV_HPA | Hydraulic pump active |
| 6 | bitfield_FD20_20 | Unknown flag |
| 7 | — | Unknown |

### Byte 5 — Protection, Counter & Clutch

| Bit | Signal | Description |
|-----|--------|-------------|
| 0 | LV_GP | Gearbox protection (triggers `c_vs_max_amt` speed limit) |
| 1 | — | Unknown |
| 2-5 | CTR_AMT | Message counter for AMT torque request (0–15) |
| 6-7 | STATE_CLU_AMT | SMG clutch state (see table) |

**STATE_CLU_AMT (Clutch State):**

| Value | Meaning |
|-------|---------|
| 0 | Clutch open |
| 1 | Clutch creeping |
| 2 | Clutch drive-off |
| 3 | Clutch closed |

---

## Implementation

The codec is implemented in `core/e46_can_protocol.h` as a header-only library with:

- **Typed structs** for each message (`e46_dme1_t`, `e46_dme2_t`, etc.)
- **Decode functions** (`e46_decode_dme1`, ...) — CAN frame → struct
- **Encode functions** (`e46_encode_dme1`, ...) — struct → CAN frame
- **Bus state aggregator** (`e46_bus_state_t`) with per-message timestamps
- **Dispatch decoder** (`e46_decode_frame`) — routes any frame by CAN ID
- **Bridge function** (`e46_to_vehicle_data`) — maps E46 bus state to `vehicle_data_t`

### Simulator

Run the CAN simulator in E46 mode to emit authentic BMW CAN frames:

```bash
./build/can_simulator --e46
```

This broadcasts DME1/DME2/DME4 at 10 ms, ASC1 at 10 ms, and ICL2 at 200 ms using the
same drive-cycle simulation as the default mode.

### Decoding Example (C)

```c
#include "e46_can_protocol.h"

e46_bus_state_t bus;
memset(&bus, 0, sizeof(bus));

can_frame_t frame;
while (hal_can_receive(&frame, 100) == 0) {
    e46_decode_frame(&frame, &bus, hal_tick_ms());
}

// Read decoded values
printf("RPM: %.0f\n", bus.dme1.engine_rpm);
printf("Coolant: %.1f C\n", bus.dme2.coolant_temp_c);
printf("Speed: %.1f km/h\n", bus.asc1.vehicle_speed_kmh);
printf("Oil temp: %d C\n", bus.dme4.oil_temp_c);
```

### Encoding Example (C)

```c
#include "e46_can_protocol.h"

// Build a DME1 frame to simulate 3000 RPM at 40% torque
e46_dme1_t dme1 = {
    .ignition_on          = true,
    .engine_rpm           = 3000.0f,
    .torque_request_pct   = 40.0f,
    .torque_indicated_pct = 38.0f,
    .torque_loss_pct      = 15.0f,
    .gear_change_ok       = true,
    .traction_ack         = true,
};

can_frame_t frame;
e46_encode_dme1(&dme1, &frame);
hal_can_send(&frame);  // frame.id == 0x316
```

---

## References

- [MS4X Wiki — Siemens MS43 CAN Bus](https://www.ms4x.net/index.php?title=Siemens_MS43_CAN_Bus)
- [MS4X Wiki — CAN Bus ID 0x153 ASC1](https://www.ms4x.net/index.php?title=CAN_Bus_ID_0x153_ASC1)
- [MS4X Wiki — CAN Bus ID 0x613 ICL2](https://www.ms4x.net/index.php?title=CAN_Bus_ID_0x613_ICL2)
- [MS4X Wiki — CAN Bus ID 0x610 ICL1](https://www.ms4x.net/index.php?title=CAN_Bus_ID_0x610_ICL1)
