# Hardware Designs

This directory contains KiCad PCB designs for OpenCluster-compatible hardware.

## Available Boards

### esp32p4-automotive

Round PCB (⌀80mm) for BMW retrofit with:
- ESP32-P4 module (MIPI CSI + DSI)
- 2× CAN bus (TJA1051T/3)
- BMW IBUS (TH3122)
- USB-C programming/debug
- 12-14V automotive power input

Perfect for mounting behind a round display in a BMW retrofit project.

[View details](esp32p4-automotive/README.md)

### esp32p4-display-adapter

Breakout adapter (20×30mm) for ESP32-P4 dev boards with **22-pin 0.5mm** DSI connector:
- ESP32-P4 DSI input (22-pin, 0.5mm FPC)
- Display output (30-pin, 0.5mm FPC) for QT021WQT or similar
- Onboard LDOs: 2.8V (VCI) + 1.8V (IOVCC)
- Level shifter for RESET signals (3.3V ↔ 1.8V)
- 2-lane MIPI DSI support

[View details](esp32p4-display-adapter/README.md)

### esp32p4-display-adapter-15pin

Breakout adapter (25×30mm) for ESP32-P4 dev boards with **15-pin 1.0mm** DSI connector:
- ESP32-P4 DSI input (15-pin, 1.0mm FPC) — same pinout as RPi
- Display output (30-pin, 0.5mm FPC) for QT021WQT or similar
- GPIO header for RESET/TP_RESET signals
- Onboard LDOs: 2.8V (VCI) + 1.8V (IOVCC)
- Level shifter TXB0102 (3.3V ↔ 1.8V)
- 2-lane MIPI DSI support

[View details](esp32p4-display-adapter-15pin/README.md)

### rpi-display-adapter

Breakout adapter (25×35mm) to connect Raspberry Pi 4 DSI output to round displays:
- RPi DSI input (15-pin, 1.0mm FPC)
- Display output (30-pin, 0.5mm FPC) for QT021WQT or similar
- GPIO header for control signals (RESET, TP_INT, PWM)
- Onboard LDOs: 2.8V (VCI) + 1.8V (IOVCC)
- 4-channel level shifter TXB0104 (3.3V ↔ 1.8V)
- 1 or 2-lane MIPI DSI support

[View details](rpi-display-adapter/README.md)

## Contributing

Hardware designs use KiCad 8.0+. When submitting:
1. Include schematic (.kicad_sch)
2. Include PCB layout (.kicad_pcb)
3. Include BOM in README
4. Test with JLCPCB/PCBWay DRC rules
