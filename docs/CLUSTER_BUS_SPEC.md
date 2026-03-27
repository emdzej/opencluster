# OpenCluster -- Cluster Bus CAN Specification

## Overview

The Cluster Bus is a dedicated CAN 2.0B network connecting the master node to display
nodes within an OpenCluster system. It carries normalized vehicle data and cluster
commands.

**Bus parameters:**

| Parameter | Value |
|-----------|-------|
| Standard | CAN 2.0B |
| Bit rate | 500 kbps |
| ID format | 11-bit standard |
| Max payload | 8 bytes |
| Termination | 120 ohm at each end of bus |

---

## CAN ID Allocation

| ID Range | Purpose |
|----------|---------|
| `0x100 - 0x1FF` | Vehicle data (master -> display nodes) |
| `0x200 - 0x2FF` | Cluster commands (any node -> all nodes) |
| `0x300 - 0x3FF` | Node management / heartbeat |
| `0x400 - 0x7FF` | Reserved for future use |

---

## Vehicle Data Messages

These are broadcast by the master node at a fixed rate. Display nodes are passive
listeners.

### 0x100 -- Engine Data

**Broadcast rate:** 50 ms (20 Hz)

| Byte | Field | Type | Unit | Range | Description |
|------|-------|------|------|-------|-------------|
| 0 | `rpm_hi` | uint8 | - | - | RPM high byte |
| 1 | `rpm_lo` | uint8 | - | 0-16000 | RPM = (rpm_hi << 8) | rpm_lo |
| 2 | `throttle_pct` | uint8 | % | 0-100 | Throttle position |
| 3 | `coolant_temp` | int8 | deg C | -40 to 215 | Coolant temperature (offset: raw + (-40)) |
| 4 | `oil_pressure` | uint8 | PSI | 0-255 | Oil pressure |
| 5 | `engine_flags` | uint8 | bitmask | - | See engine flags table |
| 6-7 | reserved | - | - | - | Set to 0x00 |

**Engine flags (byte 5):**

| Bit | Flag |
|-----|------|
| 0 | Engine running |
| 1 | Check engine light |
| 2 | Oil pressure warning |
| 3 | Overheating |
| 4-7 | Reserved |

### 0x101 -- Drivetrain Data

**Broadcast rate:** 50 ms (20 Hz)

| Byte | Field | Type | Unit | Range | Description |
|------|-------|------|------|-------|-------------|
| 0 | `speed_hi` | uint8 | - | - | Speed high byte |
| 1 | `speed_lo` | uint8 | - | 0-3000 | Speed = (speed_hi << 8) | speed_lo, in km/h * 10 (0.1 km/h resolution) |
| 2 | `gear` | uint8 | - | 0-8, 255 | 0=Neutral, 1-8=Forward, 255=Reverse |
| 3 | `odo_hi` | uint8 | - | - | Odometer bits 23-16 |
| 4 | `odo_mid` | uint8 | - | - | Odometer bits 15-8 |
| 5 | `odo_lo` | uint8 | km | 0-16777215 | Odometer = (hi << 16) | (mid << 8) | lo |
| 6-7 | reserved | - | - | - | Set to 0x00 |

### 0x102 -- Fuel / Electrical

**Broadcast rate:** 200 ms (5 Hz)

| Byte | Field | Type | Unit | Range | Description |
|------|-------|------|------|-------|-------------|
| 0 | `fuel_level_pct` | uint8 | % | 0-100 | Fuel level |
| 1 | `battery_mv_hi` | uint8 | - | - | Battery voltage high byte |
| 2 | `battery_mv_lo` | uint8 | - | 0-20000 | Battery = (hi << 8) | lo, in millivolts |
| 3-4 | `warning_flags` | uint16 | bitmask | - | See warning flags table (big-endian) |
| 5-7 | reserved | - | - | - | Set to 0x00 |

**Warning flags (bytes 3-4, big-endian):**

| Bit | Flag |
|-----|------|
| 0 | Low fuel |
| 1 | Low battery |
| 2 | Brake warning |
| 3 | ABS fault |
| 4 | Airbag fault |
| 5 | Door open |
| 6 | Seatbelt |
| 7 | High beam active |
| 8 | Left turn signal |
| 9 | Right turn signal |
| 10 | Parking brake |
| 11-15 | Reserved |

### 0x103 -- Extended Data (optional)

**Broadcast rate:** 500 ms (2 Hz)

| Byte | Field | Type | Unit | Range | Description |
|------|-------|------|------|-------|-------------|
| 0 | `intake_temp` | int8 | deg C | -40 to 215 | Intake air temperature (offset: raw + (-40)) |
| 1 | `ambient_temp` | int8 | deg C | -40 to 215 | Ambient temperature (offset: raw + (-40)) |
| 2 | `boost_psi` | uint8 | PSI | 0-50 | Turbo/supercharger boost |
| 3 | `afr_x10` | uint8 | - | 0-255 | Air/fuel ratio * 10 (e.g., 147 = 14.7:1) |
| 4-7 | reserved | - | - | - | Set to 0x00 |

---

## Cluster Command Messages

Commands can be sent by any node (display node, master, external controller).
All nodes on the bus receive commands and act accordingly.

### 0x200 -- Cluster Command

| Byte | Field | Type | Description |
|------|-------|------|-------------|
| 0 | `command_id` | uint8 | Command identifier (see table below) |
| 1 | `param1` | uint8 | First parameter (command-specific) |
| 2 | `param2` | uint8 | Second parameter (command-specific) |
| 3 | `source_node_id` | uint8 | ID of the node that originated this command |
| 4 | `sequence` | uint8 | Incrementing sequence number (for dedup) |
| 5-7 | reserved | - | Set to 0x00 |

**Command IDs:**

| ID | Name | param1 | param2 | Description |
|----|------|--------|--------|-------------|
| `0x01` | `CMD_SKIN_NEXT` | - | - | Switch to next skin |
| `0x02` | `CMD_SKIN_PREV` | - | - | Switch to previous skin |
| `0x03` | `CMD_SKIN_SET` | skin_index | - | Set specific skin by index |
| `0x10` | `CMD_LAYOUT_NEXT` | - | - | Switch to next layout template |
| `0x11` | `CMD_LAYOUT_SET` | layout_index | - | Set specific layout by index |
| `0x20` | `CMD_NIGHT_MODE` | 0=off, 1=on | - | Toggle night/day color scheme |
| `0x21` | `CMD_BRIGHTNESS` | 0-255 | - | Set display backlight brightness |
| `0x30` | `CMD_WARNING_ACK` | warning_bit | - | Acknowledge a warning |
| `0xF0` | `CMD_NODE_IDENTIFY` | node_id | node_type | Node announces itself on the bus |

**Node types (for CMD_NODE_IDENTIFY param2):**

| Value | Type |
|-------|------|
| `0x00` | Master / gateway |
| `0x01` | Display node |
| `0x02` | Input-only node (buttons, encoder) |
| `0xFF` | Desktop simulator |

---

## Node Management Messages

### 0x300 -- Heartbeat

**Broadcast rate:** 1000 ms (1 Hz), sent by every node.

| Byte | Field | Type | Description |
|------|-------|------|-------------|
| 0 | `node_id` | uint8 | Unique node ID (0-254, 255=broadcast) |
| 1 | `node_type` | uint8 | Same as CMD_NODE_IDENTIFY types |
| 2 | `status` | uint8 | 0=OK, 1=warning, 2=error |
| 3 | `uptime_min` | uint8 | Minutes since boot (wraps at 255) |
| 4-7 | reserved | - | Set to 0x00 |

---

## Byte Ordering

All multi-byte values are **big-endian** (MSB first), consistent with CAN/automotive
convention.

## Timing

| Message | Rate | Priority |
|---------|------|----------|
| Engine (0x100) | 20 Hz | High -- RPM/speed need fast updates |
| Drivetrain (0x101) | 20 Hz | High |
| Fuel/Electrical (0x102) | 5 Hz | Medium -- slow-changing values |
| Extended (0x103) | 2 Hz | Low |
| Commands (0x200) | On-demand | Medium |
| Heartbeat (0x300) | 1 Hz | Low |

At 500 kbps with standard frames, the total bus load for one master + two display nodes
is approximately 5-8%, well within CAN capacity.

---

## Desktop Simulation

On desktop, CAN frames are transported via **UDP multicast**:

| Parameter | Value |
|-----------|-------|
| Multicast group | `224.0.0.100` |
| Port | `4200` |
| Payload | Raw `can_frame_t` struct (13 bytes: 4 ID + 1 len + 8 data) |
| Byte order | Little-endian (host native, desktop only) |

Each UDP packet contains exactly one CAN frame. The CAN ID, length, and data fields
mirror the `can_frame_t` struct used in the codebase.

---

## Extending the Protocol

To add new vehicle data:

1. Choose a CAN ID in the `0x100-0x1FF` range
2. Define byte layout in this spec
3. Add fields to `vehicle_data_t` in `core/vehicle_data.h`
4. Add encode/decode functions in `core/can_protocol.h`
5. Update the CAN simulator to broadcast the new message

To add new commands:

1. Add an entry to the `cluster_command_t` enum in `core/commands.h`
2. Document it in this spec under Command IDs
3. Implement the handler in `core/commands.c`
