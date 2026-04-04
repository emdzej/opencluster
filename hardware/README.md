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

## Contributing

Hardware designs use KiCad 8.0+. When submitting:
1. Include schematic (.kicad_sch)
2. Include PCB layout (.kicad_pcb)
3. Include BOM in README
4. Test with JLCPCB/PCBWay DRC rules
