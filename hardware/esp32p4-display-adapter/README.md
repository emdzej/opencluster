# ESP32-P4 DSI to Round Display Adapter

Adapter PCB to connect ESP32-P4 DSI output (22-pin) to QT021WQT 2.1" round display (30-pin, 480x480).

## Connectors

### J1 - ESP32-P4 DSI Input (22-pin, 0.5mm pitch)

| Pin | Signal | GPIO | Description |
|-----|--------|------|-------------|
| 1 | ESP_3V3 | — | 3.3V Power |
| 2 | I2C_SDA | GPIO8 | I2C Data |
| 3 | I2C_SCL | GPIO9 | I2C Clock |
| 4 | TP_INT | GPIO? | Touch Interrupt (optional) |
| 5 | RESET | GPIO? | Display Reset |
| 6 | TP_RESET | GPIO? | Touch Reset |
| 7-12 | GND | — | Ground |
| 13 | DSI_CLK_P | — | MIPI Clock+ |
| 14 | DSI_CLK_N | — | MIPI Clock- |
| 15 | GND | — | Ground |
| 16 | DSI_D1_P | — | MIPI Data Lane 1+ |
| 17 | DSI_D1_N | — | MIPI Data Lane 1- |
| 18 | GND | — | Ground |
| 19 | DSI_D0_P | — | MIPI Data Lane 0+ |
| 20 | DSI_D0_N | — | MIPI Data Lane 0- |
| 21-22 | GND | — | Ground |

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

## Wiring Summary

```
ESP32-P4 (J1)             Adapter                Display (J2)
─────────────             ───────                ────────────
Pin 13 (CLK+) ─────────────────────────────────► Pin 16 (TCP)
Pin 14 (CLK-) ─────────────────────────────────► Pin 17 (TCN)
Pin 16 (D1+)  ─────────────────────────────────► Pin 13 (DIP)
Pin 17 (D1-)  ─────────────────────────────────► Pin 14 (DIN)
Pin 19 (D0+)  ─────────────────────────────────► Pin 10 (DOP)
Pin 20 (D0-)  ─────────────────────────────────► Pin 11 (DON)
Pin 2 (SDA)   ─────────────────────────────────► Pin 25 (TP-SDA)
Pin 3 (SCL)   ─────────────────────────────────► Pin 26 (TP-SCL)
Pin 4 (TP_INT)◄────────────────────────────────── Pin 24 (TP-INT)
Pin 5 (RESET) ───[Level 3.3V→1.8V]─────────────► Pin 6 (RESET)
Pin 6 (TP_RST)───[Level 3.3V→1.8V]─────────────► Pin 27 (TP-RESET)
Pin 7-12,15,18,21-22 (GND) ────────────────────► Pin 9,12,15,18,29 (GND)
Pin 1 (3V3)   ──┬──► U1 (LDO) ──► 2.8V ────────► Pin 4,28 (VCI)
                └──► U2 (LDO) ──► 1.8V ────────► Pin 5,30 (IOVCC)
```

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
- CH1: RESET (3.3V ESP → 1.8V display)
- CH2: TP_RESET (3.3V ESP → 1.8V touch)

Note: TP_INT is 1.8V input to ESP but ESP32-P4 GPIO is 3.3V tolerant with 1.8V threshold, so direct connection OK.

### Backlight Driver

| Ref | Part | Package | Description |
|-----|------|---------|-------------|
| Q1 | SI2301 | SOT-23 | P-MOSFET for LED switching |
| R1 | TBD | 0402 | LED current limit (check display datasheet) |

### Connectors

| Ref | Part | Description |
|-----|------|-------------|
| J1 | AFC07-S22 or similar | 22-pin 0.5mm FPC |
| J2 | AFC07-S30 or similar | 30-pin 0.5mm FPC |

## PCB Notes

1. **MIPI differential pairs** - maintain 100Ω differential impedance
2. **Keep CLK, D0, D1 pairs matched in length** (within 0.5mm)
3. **Ground plane** - solid ground under MIPI traces
4. **Decoupling caps** - place close to LDO outputs
5. **Board size** - ~20x30mm (both FPC connectors same pitch)
6. **2-lane MIPI** - ESP32-P4 supports 2 data lanes, use both for better bandwidth

## Pin Mapping Table

| Function | ESP32-P4 J1 Pin | Display J2 Pin | Notes |
|----------|-----------------|----------------|-------|
| MIPI CLK+ | 13 | 16 | Diff pair |
| MIPI CLK- | 14 | 17 | Diff pair |
| MIPI D0+ | 19 | 10 | Diff pair |
| MIPI D0- | 20 | 11 | Diff pair |
| MIPI D1+ | 16 | 13 | Diff pair |
| MIPI D1- | 17 | 14 | Diff pair |
| I2C SDA | 2 | 25 | Touch panel |
| I2C SCL | 3 | 26 | Touch panel |
| Touch INT | 4 | 24 | Input to ESP |
| Reset | 5 | 6 | Via level shifter |
| Touch Reset | 6 | 27 | Via level shifter |
| GND | 7-12,15,18,21-22 | 9,12,15,18,29 | Multiple |
| 3.3V | 1 | — | To LDOs |
| 2.8V | — | 4, 28 | From U1 |
| 1.8V | — | 5, 30 | From U2 |
| LED+ | — | 1 | Backlight |
| LED- | — | 2-3 | Backlight |
