# RPi DSI to Round Display Adapter

Adapter PCB to connect Raspberry Pi 4 DSI output (15-pin) to QT021WQT 2.1" round display (30-pin, 480x480).

## Connectors

### J1 - RPi DSI Input (15-pin, 1.0mm pitch)

| Pin | Signal | Description |
|-----|--------|-------------|
| 1 | GND | Ground |
| 2 | D1_N | MIPI Data Lane 1- (optional for 1-lane) |
| 3 | D1_P | MIPI Data Lane 1+ (optional for 1-lane) |
| 4 | GND | Ground |
| 5 | CLK_N | MIPI Clock- |
| 6 | CLK_P | MIPI Clock+ |
| 7 | GND | Ground |
| 8 | D0_N | MIPI Data Lane 0- |
| 9 | D0_P | MIPI Data Lane 0+ |
| 10 | GND | Ground |
| 11 | GND | Ground |
| 12 | SCL | I2C Clock (touch panel) |
| 13 | SDA | I2C Data (touch panel) |
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

### J3 - GPIO Header (6-pin, 2.54mm)

Connect to Raspberry Pi GPIO header for control signals.

| Pin | Signal | RPi GPIO | Description |
|-----|--------|----------|-------------|
| 1 | 3V3 | 3.3V | Power from RPi |
| 2 | GND | GND | Ground |
| 3 | RESET | GPIO17 | Display Reset |
| 4 | TP_RESET | GPIO22 | Touch Reset |
| 5 | TP_INT | GPIO27 | Touch Interrupt |
| 6 | PWM | GPIO18 | Backlight PWM |

## Wiring Summary

```
RPi DSI (J1)              Adapter                Display (J2)
────────────              ───────                ────────────
Pin 5 (CLK-)  ─────────────────────────────────► Pin 17 (TCN)
Pin 6 (CLK+)  ─────────────────────────────────► Pin 16 (TCP)
Pin 8 (D0-)   ─────────────────────────────────► Pin 11 (DON)
Pin 9 (D0+)   ─────────────────────────────────► Pin 10 (DOP)
Pin 2 (D1-)   ─────────────────────────────────► Pin 14 (DIN)  [optional]
Pin 3 (D1+)   ─────────────────────────────────► Pin 13 (DIP)  [optional]
Pin 12 (SCL)  ─────────────────────────────────► Pin 26 (TP-SCL)
Pin 13 (SDA)  ─────────────────────────────────► Pin 25 (TP-SDA)
Pin 1,4,7,10,11 (GND) ─────────────────────────► Pin 9,12,15,18,29 (GND)
Pin 14-15 (3V3) ──┬──► U1 (LDO) ──► 2.8V ──────► Pin 4,28 (VCI)
                  └──► U2 (LDO) ──► 1.8V ──────► Pin 5,30 (IOVCC)

RPi GPIO (J3)
─────────────
GPIO17 (RESET)    ──► U3 CH1 ──► 1.8V ─────────► Pin 6 (RESET)
GPIO22 (TP_RESET) ──► U3 CH2 ──► 1.8V ─────────► Pin 27 (TP-RESET)
GPIO27 (TP_INT)   ◄── U3 CH3 ◄── 1.8V ◄────────── Pin 24 (TP-INT)
GPIO18 (PWM)      ──► Q1 gate ──► LED driver ──► Pin 1-3 (LED)
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
| U3 | TXB0104 | SOIC-14 | 4-bit level shifter |

Channels:
- CH1: RESET (3.3V GPIO → 1.8V display)
- CH2: TP_RESET (3.3V GPIO → 1.8V touch)
- CH3: TP_INT (1.8V touch → 3.3V GPIO)
- CH4: spare

### Backlight Driver

| Ref | Part | Package | Description |
|-----|------|---------|-------------|
| Q1 | SI2301 | SOT-23 | P-MOSFET for LED switching |
| R1 | TBD | 0402 | LED current limit (check display datasheet) |

### Connectors

| Ref | Part | Description |
|-----|------|-------------|
| J1 | Hirose FH12-15S-0.5SH or similar | 15-pin 1.0mm FPC |
| J2 | AFC07-S30 or similar | 30-pin 0.5mm FPC |
| J3 | Pin header 1x6 | 2.54mm pitch |

## PCB Notes

1. **MIPI differential pairs** - maintain 100Ω differential impedance
2. **Keep CLK and D0 pairs matched in length** (within 0.5mm)
3. **Ground plane** - solid ground under MIPI traces
4. **Decoupling caps** - place close to LDO outputs
5. **Board size** - ~25x35mm
6. **1-lane vs 2-lane** - Display may work with 1 lane (D0 only), D1 optional

## Pin Mapping Table

| Function | RPi J1 Pin | Display J2 Pin | Notes |
|----------|------------|----------------|-------|
| MIPI CLK+ | 6 | 16 | Diff pair |
| MIPI CLK- | 5 | 17 | Diff pair |
| MIPI D0+ | 9 | 10 | Diff pair |
| MIPI D0- | 8 | 11 | Diff pair |
| MIPI D1+ | 3 | 13 | Diff pair (optional) |
| MIPI D1- | 2 | 14 | Diff pair (optional) |
| I2C SDA | 13 | 25 | Touch panel |
| I2C SCL | 12 | 26 | Touch panel |
| Touch INT | J3.5 | 24 | Via level shifter |
| Reset | J3.3 | 6 | Via level shifter |
| Touch Reset | J3.4 | 27 | Via level shifter |
| GND | 1,4,7,10,11 | 9,12,15,18,29 | Multiple |
| 3.3V | 14-15 | — | To LDOs |
| 2.8V | — | 4, 28 | From U1 |
| 1.8V | — | 5, 30 | From U2 |
| LED+ | — | 1 | Backlight |
| LED- | — | 2-3 | Backlight |

## Software Configuration

Add to `/boot/firmware/config.txt`:

```
dtoverlay=vc4-kms-dsi-generic,clock-frequency=27000000,hactive=480,vactive=480,hfp=10,hsync=4,hbp=10,vfp=10,vsync=2,vbp=14
```

Touch panel I2C address: typically 0x38 or 0x5D (check datasheet)

## Status

⚠️ **Untested prototype** - Schematic and PCB layout need manual completion in KiCad.
