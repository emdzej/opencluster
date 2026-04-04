# ESP32-P4 Automotive Display Controller

Round PCB (⌀80mm) for BMW retrofit with:
- ESP32-P4 module (MIPI CSI + DSI)
- 2× CAN bus (TJA1051T/3)
- BMW IBUS (TH3122)
- USB-C programming/debug
- 12-14V automotive power input

## Specifications

| Parameter | Value |
|-----------|-------|
| Board shape | Circle ⌀80mm |
| Layers | 2 (TOP + BOT) |
| Thickness | 1.6mm |
| Copper | 1oz |
| Finish | HASL or ENIG |

## Block Diagram

```
                    12-14V (OBD/Battery)
                           │
                    ┌──────▼──────┐
                    │ TVS + Diode │
                    │  Protection │
                    └──────┬──────┘
                           │
                    ┌──────▼──────┐
                    │  TPS62827   │
                    │  DC-DC 3.3V │
                    └──────┬──────┘
                           │
     ┌─────────────────────┼─────────────────────┐
     │                     │                     │
┌────▼────┐         ┌──────▼──────┐        ┌─────▼─────┐
│  USB-C  │         │  ESP32-P4   │        │   CAN ×2  │
│  JTAG   │◄───────►│   Module    │◄──────►│ TJA1051   │
│  Debug  │         │             │        └───────────┘
└─────────┘         │  MIPI CSI ──┼──────► FPC 15-pin
                    │  MIPI DSI ──┼──────► FPC 22-pin
                    │  UART     ──┼──────► TH3122 (IBUS)
                    └─────────────┘
```

## Connectors

### J1 - USB-C
- Programming via USB-JTAG (native ESP32-P4)
- Debug console (USB-CDC)
- Alternative 5V power (dev mode)

### J2 - FPC 15-pin CSI (0.5mm pitch)
Standard Raspberry Pi camera pinout:
| Pin | Signal |
|-----|--------|
| 1 | GND |
| 2 | CSI_D0_N |
| 3 | CSI_D0_P |
| 4 | GND |
| 5 | CSI_D1_N |
| 6 | CSI_D1_P |
| 7 | GND |
| 8 | CSI_CLK_N |
| 9 | CSI_CLK_P |
| 10 | GND |
| 11-15 | I2C, GPIO, Power |

### J3 - FPC 22-pin DSI (0.5mm pitch)
| Pin | Signal |
|-----|--------|
| 1 | GND |
| 2-3 | DSI_D1_N/P |
| 4 | GND |
| 5-6 | DSI_CLK_N/P |
| 7 | GND |
| 8-9 | DSI_D0_N/P |
| 10 | GND |
| 11-14 | DSI_D2, DSI_D3 (if 4-lane) |
| 15-18 | NC/GND |
| 19 | RESET |
| 20 | NC |
| 21 | VCC_IO |
| 22 | VCC_BL |

### J4 - JST-SH 8-pin (Automotive)
| Pin | Signal | Description |
|-----|--------|-------------|
| 1 | +12V | Automotive power (10-16V) |
| 2 | GND | Ground |
| 3 | CAN1_H | CAN bus 1 High |
| 4 | CAN1_L | CAN bus 1 Low |
| 5 | CAN2_H | CAN bus 2 High |
| 6 | CAN2_L | CAN bus 2 Low |
| 7 | IBUS | BMW IBUS (single wire) |
| 8 | GND | Ground |

## BOM

| Ref | Value | Package | Qty | Description |
|-----|-------|---------|-----|-------------|
| U1 | ESP32-P4-WROOM-01 | Module | 1 | Main MCU module |
| U2 | TJA1051T/3 | SOIC-8 | 1 | CAN transceiver 1 |
| U3 | TJA1051T/3 | SOIC-8 | 1 | CAN transceiver 2 |
| U4 | TH3122 | SOIC-8 | 1 | BMW IBUS transceiver |
| U5 | TPS62827DLCR | WSON-11 | 1 | 3.3V DC-DC converter |
| U6 | USBLC6-2SC6Y | SOT-23-6 | 1 | USB ESD protection |
| D1 | SMBJ18CA | SMB | 1 | TVS diode 18V |
| D2 | SS34 | SMA | 1 | Reverse polarity diode |
| D3 | PESD2CAN | SOT-23 | 1 | CAN1 ESD protection |
| D4 | PESD2CAN | SOT-23 | 1 | CAN2 ESD protection |
| J1 | USB4110-GF-A | SMD | 1 | USB-C receptacle |
| J2 | FPC-15-0.5mm | SMD | 1 | MIPI CSI connector |
| J3 | FPC-22-0.5mm | SMD | 1 | MIPI DSI connector |
| J4 | JST-SH-8 | SMD | 1 | Automotive connector |
| L1 | 2.2µH | 1210 | 1 | DC-DC inductor |
| C1-C4 | 10µF | 0603 | 4 | Decoupling caps |
| C5-C10 | 100nF | 0402 | 6 | Bypass caps |
| C11 | 22µF | 0805 | 1 | DC-DC input cap |
| C12 | 22µF | 0805 | 1 | DC-DC output cap |
| R1 | 120Ω | 0402 | 1 | CAN1 termination (optional) |
| R2 | 120Ω | 0402 | 1 | CAN2 termination (optional) |
| R3 | 1kΩ | 0402 | 1 | IBUS pull-up |
| R4-R5 | 5.1kΩ | 0402 | 2 | USB-C CC resistors |

## Design Notes

### 2-Layer MIPI Strategy
- FPC connectors placed close to ESP32-P4 module (<15mm traces)
- Coplanar waveguide for differential pairs
- Solid GND pour on bottom layer
- Via stitching around MIPI traces

### Power
- Input: 10-16V (automotive)
- TVS protection for load dump transients (42V)
- Reverse polarity protection (Schottky diode)
- TPS62827: 3A capability, 3-17V input

### CAN Bus
- TJA1051T/3: 3.3V logic, automotive grade
- PESD2CAN for ESD protection
- Optional 120Ω termination resistors (populate if end-of-line)

### IBUS
- TH3122: native BMW IBUS transceiver
- Single-wire bus, 9600 baud
- 1kΩ pull-up to 12V on bus line

## Files

```
esp32p4-automotive/
├── esp32p4-automotive.kicad_pro    # Project file
├── esp32p4-automotive.kicad_sch    # Schematic
├── esp32p4-automotive.kicad_pcb    # PCB layout
└── README.md                        # This file
```

## TODO

- [ ] Add proper ESP32-P4-WROOM-01 symbol/footprint
- [ ] Add TH3122 symbol (not in standard KiCad libs)
- [ ] Complete schematic wiring
- [ ] PCB layout with placement
- [ ] MIPI trace impedance calculation
- [ ] Generate Gerbers

## License

MIT
