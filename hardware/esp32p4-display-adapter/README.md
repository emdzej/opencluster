# ESP32-P4 DSI to Round Display Adapter

Adapter PCB to connect ESP32-P4 DSI output (15-pin, 1.0mm) to QT021WQT 2.1" round display (30-pin, 0.5mm, 480x480).

## Connectors

### J1 - ESP32-P4 DSI Input (15-pin, 1.0mm pitch)

| Pin | Signal | Description |
|-----|--------|-------------|
| 1 | GND | Ground |
| 2 | D1_N | MIPI Data Lane 1- |
| 3 | D1_P | MIPI Data Lane 1+ |
| 4 | GND | Ground |
| 5 | CLK_N | MIPI Clock- |
| 6 | CLK_P | MIPI Clock+ |
| 7 | GND | Ground |
| 8 | D0_N | MIPI Data Lane 0- |
| 9 | D0_P | MIPI Data Lane 0+ |
| 10 | GND | Ground |
| 11 | GND | Ground |
| 12 | SCL | I2C Clock |
| 13 | SDA | I2C Data |
| 14 | 3V3 | 3.3V Power |
| 15 | 3V3 | 3.3V Power |

### J2 - Display Output (30-pin, 0.5mm pitch)

| Pin | Signal | Description |
|-----|--------|-------------|
| 1 | LED-A | Backlight Anode |
| 2-3 | LED-K | Backlight Cathode |
| 4 | VCI | 2.8V Power |
| 5 | IOVCC | 1.8V I/O Power |
| 6 | RESET | Display Reset (1.8V logic) |
| 7 | TE | Tearing Effect (optional) |
| 8 | PWM | Backlight PWM control |
| 9 | GND | Ground |
| 10 | DOP | MIPI Data Lane 0+ |
| 11 | DON | MIPI Data Lane 0- |
| 12 | GND | Ground |
| 13 | DIP | MIPI Data Lane 1+ |
| 14 | DIN | MIPI Data Lane 1- |
| 15 | GND | Ground |
| 16 | TCP | MIPI Clock+ |
| 17 | TCN | MIPI Clock- |
| 18-23 | GND/NC | Ground / Not Connected |
| 24 | TP-INT | Touch Panel Interrupt |
| 25 | TP-SDA | Touch Panel I2C Data |
| 26 | TP-SCL | Touch Panel I2C Clock |
| 27 | TP-RESET | Touch Panel Reset (1.8V logic) |
| 28 | TP-VCI | Touch Panel 2.8V Power |
| 29 | TP-GND | Touch Panel Ground |
| 30 | TP-IOVCC | Touch Panel 1.8V I/O Power |

### J3 - GPIO Header (optional, 4-pin, 2.54mm)

For signals not on DSI connector (if needed):

| Pin | Signal | Description |
|-----|--------|-------------|
| 1 | 3V3 | Power |
| 2 | GND | Ground |
| 3 | RESET | Display Reset (GPIO) |
| 4 | TP_RESET | Touch Reset (GPIO) |

## Wiring Summary

```
ESP32-P4 (J1)             Adapter                Display (J2)
─────────────             ───────                ────────────
Pin 5 (CLK-)  ─────────────────────────────────► Pin 17 (TCN)
Pin 6 (CLK+)  ─────────────────────────────────► Pin 16 (TCP)
Pin 8 (D0-)   ─────────────────────────────────► Pin 11 (DON)
Pin 9 (D0+)   ─────────────────────────────────► Pin 10 (DOP)
Pin 2 (D1-)   ─────────────────────────────────► Pin 14 (DIN)
Pin 3 (D1+)   ─────────────────────────────────► Pin 13 (DIP)
Pin 12 (SCL)  ─────────────────────────────────► Pin 26 (TP-SCL)
Pin 13 (SDA)  ─────────────────────────────────► Pin 25 (TP-SDA)
Pin 1,4,7,10,11 (GND) ─────────────────────────► Pin 9,12,15,18,29 (GND)
Pin 14-15 (3V3) ──┬──► U1 (LDO) ──► 2.8V ──────► Pin 4,28 (VCI)
                  └──► U2 (LDO) ──► 1.8V ──────► Pin 5,30 (IOVCC)

GPIO (J3) - optional
────────────────────
RESET     ──► U3 CH1 ──► 1.8V ─────────────────► Pin 6 (RESET)
TP_RESET  ──► U3 CH2 ──► 1.8V ─────────────────► Pin 27 (TP-RESET)
```

**Note:** The 15-pin DSI connector doesn't include RESET/TP_RESET/TP_INT signals. These must come from separate GPIOs via J3 header, or directly from ESP32-P4 board GPIO.

## Components

### Power Supply

| Ref | Part | Package | Description |
|-----|------|---------|-------------|
| U1 | AP2127K-2.8 | SOT-23-5 | 2.8V 300mA LDO |
| U2 | AP2127K-1.8 | SOT-23-5 | 1.8V 300mA LDO |
| C1-C2 | 1µF | 0402 | Input caps |
| C3-C4 | 1µF | 0402 | Output caps |

### Level Shifting (3.3V ↔ 1.8V)

| Ref | Part | Package | Description |
|-----|------|---------|-------------|
| U3 | TXB0102 | SOT-23-6 | 2-bit level shifter |

Channels:
- CH1: RESET (3.3V GPIO → 1.8V display)
- CH2: TP_RESET (3.3V GPIO → 1.8V touch)

Note: TP_INT is 1.8V input; ESP32-P4 GPIO is 3.3V tolerant with 1.8V threshold — direct connection OK.

### Backlight Driver

| Ref | Part | Package | Description |
|-----|------|---------|-------------|
| Q1 | SI2301 | SOT-23 | P-MOSFET for LED switching |
| R1 | TBD | 0402 | LED current limit (check display datasheet) |

### Connectors

| Ref | Part | Description |
|-----|------|-------------|
| J1 | FCI SFW15R-2STE1LF or similar | 15-pin 1.0mm FPC |
| J2 | AFC07-S30 or similar | 30-pin 0.5mm FPC |
| J3 | Pin header 1x4 (optional) | 2.54mm pitch |

## PCB Notes

1. **MIPI differential pairs** - maintain 100Ω differential impedance
2. **Keep CLK, D0, D1 pairs matched in length** (within 0.5mm)
3. **Ground plane** - solid ground under MIPI traces
4. **Decoupling caps** - place close to LDO outputs
5. **Board size** - ~25x30mm
6. **2-lane MIPI** - ESP32-P4 supports 2 data lanes, use both for better bandwidth

## Pin Mapping Table

| Function | ESP32-P4 J1 Pin | Display J2 Pin | Notes |
|----------|-----------------|----------------|-------|
| MIPI CLK+ | 6 | 16 | Diff pair |
| MIPI CLK- | 5 | 17 | Diff pair |
| MIPI D0+ | 9 | 10 | Diff pair |
| MIPI D0- | 8 | 11 | Diff pair |
| MIPI D1+ | 3 | 13 | Diff pair |
| MIPI D1- | 2 | 14 | Diff pair |
| I2C SDA | 13 | 25 | Touch panel |
| I2C SCL | 12 | 26 | Touch panel |
| Touch INT | J3/GPIO | 24 | Via level shifter (optional) |
| Reset | J3.3 | 6 | Via level shifter |
| Touch Reset | J3.4 | 27 | Via level shifter |
| GND | 1,4,7,10,11 | 9,12,15,18,29 | Multiple |
| 3.3V | 14-15 | — | To LDOs |
| 2.8V | — | 4, 28 | From U1 |
| 1.8V | — | 5, 30 | From U2 |
| LED+ | — | 1 | Backlight |
| LED- | — | 2-3 | Backlight |

## Status

⚠️ **Untested prototype** - Schematic contains component symbols only; wiring must be completed manually in KiCad.
